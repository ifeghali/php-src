/* 
   +----------------------------------------------------------------------
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


/* $Id$ */


#include <stdio.h>
#include "php.h"
#ifdef MSVC5
#include "win32/time.h"
#include "win32/signal.h"
#include <process.h>
#else
#include "build-defs.h"
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_SETLOCALE
#include <locale.h>
#endif
#include "zend.h"
#include "php_ini.h"
#include "php_globals.h"
#include "main.h"
#include "fopen-wrappers.h"
#include "ext/standard/php_standard.h"
#include "snprintf.h"
#include "php_variables.h"
#if PHP_WIN32
#include <io.h>
#include <fcntl.h>
#include "win32/syslog.h"
#include "win32/php_registry.h"
#else
#include <syslog.h>
#endif

#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_highlight.h"
#include "zend_indent.h"

#include "php_content_types.h"

#include "SAPI.h"

#ifndef ZTS
php_core_globals core_globals;
#else
PHPAPI int core_globals_id;
#endif

#define NO_GLOBAL_LOCK

/* temporary workaround for thread-safety issues in Zend */
#if defined(ZTS) && !defined(NO_GLOBAL_LOCK)
static MUTEX_T global_lock;
#define global_lock() tsrm_mutex_lock(global_lock)
#define global_unlock() tsrm_mutex_unlock(global_lock);
#define global_lock_init() global_lock = tsrm_mutex_alloc()
#define global_lock_destroy() tsrm_mutex_free(global_lock)
#else
#define global_lock()
#define global_unlock()
#define global_lock_init()
#define global_lock_destroy()
#endif
 

static void php_build_argv(char *s, zval *track_vars_array ELS_DC PLS_DC);
static void php_timeout(int dummy);
static void php_set_timeout(long seconds);

void *gLock;					/*mutex variable */


/* True globals (no need for thread safety) */
HashTable configuration_hash;
PHPAPI char *php_ini_path = NULL;


#define SAFE_FILENAME(f) ((f)?(f):"-")

static PHP_INI_MH(OnSetPrecision)
{
	ELS_FETCH();

	EG(precision) = atoi(new_value);
	return SUCCESS;
}


#if MEMORY_LIMIT
static PHP_INI_MH(OnChangeMemoryLimit)
{
	int new_limit;

	if (new_value) {
		new_limit = atoi(new_value);
	} else {
		new_limit = 1<<30;		/* effectively, no limit */
	}
	return zend_set_memory_limit(new_limit);
}
#endif


static PHP_INI_MH(OnUpdateErrorReporting)
{
	ELS_FETCH();

	if (!new_value) {
		EG(error_reporting) = E_ALL & ~E_NOTICE;
	} else {
		EG(error_reporting) = atoi(new_value);
	}
	return SUCCESS;
}


/* Need to convert to strings and make use of:
 * DEFAULT_SHORT_OPEN_TAG
 * PHP_SAFE_MODE
 *
 * Need to be read from the environment (?):
 * PHP_AUTO_PREPEND_FILE
 * PHP_AUTO_APPEND_FILE
 * PHP_DOCUMENT_ROOT
 * PHP_USER_DIR
 * PHP_INCLUDE_PATH
 */

#ifndef SAFE_MODE_EXEC_DIR
#	define SAFE_MODE_EXEC_DIR "/"
#endif

#ifdef PHP_PROG_SENDMAIL
#	define DEFAULT_SENDMAIL_PATH PHP_PROG_SENDMAIL " -t"
#else
#	define DEFAULT_SENDMAIL_PATH NULL
#endif
PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN("short_open_tag",	"1",		PHP_INI_ALL,		OnUpdateBool,		short_tags,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("asp_tags",			"0",		PHP_INI_ALL,		OnUpdateBool,		asp_tags,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("allow_call_time_pass_reference",	"1",	PHP_INI_ALL,	OnUpdateBool,	allow_call_time_pass_reference, php_core_globals, core_globals)
	PHP_INI_ENTRY("precision",			"14",		PHP_INI_ALL,		OnSetPrecision)
	STD_PHP_INI_BOOLEAN("output_buffering",	"0",	PHP_INI_PERDIR|PHP_INI_SYSTEM,	OnUpdateBool,	output_buffering,	php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("implicit_flush",	"0",	PHP_INI_PERDIR|PHP_INI_SYSTEM,	OnUpdateBool,	implicit_flush,		php_core_globals,	core_globals)

	PHP_INI_ENTRY_EX("highlight.comment",	HL_COMMENT_COLOR,	PHP_INI_ALL,		NULL,		php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.default",	HL_DEFAULT_COLOR,	PHP_INI_ALL,		NULL,		php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.html",		HL_HTML_COLOR,		PHP_INI_ALL,		NULL,		php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.string",	HL_STRING_COLOR,	PHP_INI_ALL,		NULL,		php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.bg",		HL_BG_COLOR,		PHP_INI_ALL,		NULL,		php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.keyword",	HL_KEYWORD_COLOR,	PHP_INI_ALL,		NULL,		php_ini_color_displayer_cb)

	STD_PHP_INI_BOOLEAN("magic_quotes_gpc",		"1",	PHP_INI_ALL,		OnUpdateBool,		magic_quotes_gpc,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("magic_quotes_runtime",	"0",	PHP_INI_ALL,		OnUpdateBool,		magic_quotes_runtime,	php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("magic_quotes_sybase",	"0",	PHP_INI_ALL,		OnUpdateBool,		magic_quotes_sybase,	php_core_globals,	core_globals)

	STD_PHP_INI_BOOLEAN("safe_mode",		"0",	PHP_INI_SYSTEM,			OnUpdateBool,	safe_mode,				php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("sql.safe_mode",	"0",	PHP_INI_SYSTEM,			OnUpdateBool,	sql_safe_mode,			php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("safe_mode_exec_dir",	"1",	PHP_INI_SYSTEM,			OnUpdateString,	safe_mode_exec_dir,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("enable_dl",		"1",	PHP_INI_SYSTEM,			OnUpdateBool,	enable_dl,				php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("expose_php",		"1",	PHP_INI_SYSTEM,			OnUpdateBool,	expose_php,				php_core_globals,	core_globals)


	PHP_INI_ENTRY("SMTP",			"localhost",			PHP_INI_ALL,		NULL)
	PHP_INI_ENTRY("sendmail_path",	DEFAULT_SENDMAIL_PATH,	PHP_INI_SYSTEM,		NULL)
	PHP_INI_ENTRY("sendmail_from",	NULL,					PHP_INI_ALL,		NULL)

	PHP_INI_ENTRY("error_reporting",	NULL,				PHP_INI_ALL,		OnUpdateErrorReporting)
	STD_PHP_INI_BOOLEAN("display_errors",		"1",			PHP_INI_ALL,		OnUpdateBool,		display_errors,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("track_errors",		"0",			PHP_INI_ALL,		OnUpdateBool,		track_errors,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("log_errors",			"0",			PHP_INI_ALL,		OnUpdateBool,		log_errors,			php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("error_log",			NULL,			PHP_INI_ALL,		OnUpdateString,	error_log,			php_core_globals,	core_globals)


	STD_PHP_INI_ENTRY("auto_prepend_file",	NULL,			PHP_INI_ALL,		OnUpdateString,	auto_prepend_file,	php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("auto_append_file",	NULL,			PHP_INI_ALL,		OnUpdateString,	auto_append_file,	php_core_globals,	core_globals)

	STD_PHP_INI_BOOLEAN("y2k_compliance",		"0",			PHP_INI_ALL,		OnUpdateBool,		y2k_compliance,		php_core_globals,	core_globals)

	STD_PHP_INI_ENTRY("doc_root",			NULL,			PHP_INI_SYSTEM,		OnUpdateStringUnempty,	doc_root,		php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("user_dir",			NULL,			PHP_INI_SYSTEM,		OnUpdateStringUnempty,	user_dir,		php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("include_path",		NULL,			PHP_INI_ALL,		OnUpdateStringUnempty,	include_path,	php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("open_basedir",		NULL,			PHP_INI_SYSTEM,		OnUpdateStringUnempty,	open_basedir,	php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("extension_dir",		NULL,			PHP_INI_SYSTEM,		OnUpdateStringUnempty,	extension_dir,	php_core_globals,	core_globals)

	STD_PHP_INI_ENTRY("upload_tmp_dir",			NULL,			PHP_INI_SYSTEM,		OnUpdateStringUnempty,	upload_tmp_dir,			php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("upload_max_filesize",	"2097152",		PHP_INI_ALL,		OnUpdateInt,				upload_max_filesize,	php_core_globals,	core_globals)

	PHP_INI_ENTRY("browscap",			NULL,				PHP_INI_SYSTEM,		NULL)

	PHP_INI_ENTRY_EX("define_syslog_variables",	"0",		PHP_INI_ALL,		NULL,	php_ini_boolean_displayer_cb)

	STD_PHP_INI_ENTRY("max_execution_time",		"30",			PHP_INI_ALL,		OnUpdateInt, 				max_execution_time,	php_core_globals,	core_globals)
#if MEMORY_LIMIT
	PHP_INI_ENTRY("memory_limit",			"8388608",		PHP_INI_ALL,		OnChangeMemoryLimit)
#endif

#if PHP_TRACK_VARS /* "cc -32" on IRIX 6.4 does not like (PHP_TRACK_VARS?"1":"0") - thies 991004 */
	STD_PHP_INI_BOOLEAN("track_vars",			"1",			PHP_INI_ALL,		OnUpdateBool,				track_vars,		php_core_globals,	core_globals)
#else
	STD_PHP_INI_BOOLEAN("track_vars",			"0",			PHP_INI_ALL,		OnUpdateBool,				track_vars,		php_core_globals,	core_globals)
#endif

	STD_PHP_INI_BOOLEAN("register_globals",		"1",			PHP_INI_ALL,		OnUpdateBool,			register_globals,	php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("register_argc_argv",	"1",			PHP_INI_ALL,		OnUpdateBool,			register_argc_argv,	php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("gpc_order",				"GPC",			PHP_INI_ALL,		OnUpdateStringUnempty,	gpc_order,			php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("variables_order",		NULL,			PHP_INI_ALL,		OnUpdateStringUnempty,	variables_order,	php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("arg_separator",			"&",			PHP_INI_ALL,		OnUpdateStringUnempty,	arg_separator,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("ignore_user_abort",	"1",			PHP_INI_ALL,		OnUpdateBool,			ignore_user_abort,		php_core_globals,	core_globals)
PHP_INI_END()



/* True global (no need for thread safety */
static int module_initialized = 0;

void php_log_err(char *log_message)
{
	FILE *log_file;
	char error_time_str[128];
	struct tm tmbuf;
	time_t error_time;
	PLS_FETCH();

	/* Try to use the specified logging location. */
	if (PG(error_log) != NULL) {
#if HAVE_SYSLOG_H
		if (!strcmp(PG(error_log), "syslog")) {
			syslog(LOG_NOTICE, log_message);
			return;
		}
#endif
		log_file = fopen(PG(error_log), "a");
		if (log_file != NULL) {
			time(&error_time);
			strftime(error_time_str, 128, "%d-%b-%Y %H:%M:%S", localtime_r(&error_time, &tmbuf)); 
			fprintf(log_file, "[%s] ", error_time_str);
			fprintf(log_file, log_message);
			fprintf(log_file, "\n");
			fclose(log_file);
			return;
		}
	}

	/* Otherwise fall back to the default logging location, if we have one */

	if (sapi_module.log_message) {
		sapi_module.log_message(log_message);
	}
}


/* is 4K big enough? */
#define PRINTF_BUFFER_SIZE 1024*4

/* wrapper for modules to use PHPWRITE */
PHPAPI int php_write(void *buf, int size)
{
	return PHPWRITE(buf, size);
}

PHPAPI int php_printf(const char *format,...)
{
	va_list args;
	int ret;
	char buffer[PRINTF_BUFFER_SIZE];
	int size;

	va_start(args, format);
	size = vsnprintf(buffer, sizeof(buffer), format, args);
	ret = PHPWRITE(buffer, size);
	va_end(args);
	
	return ret;
}


/* extended error handling function */
PHPAPI void php_error(int type, const char *format,...)
{
	va_list args;
	char *error_filename = NULL;
	uint error_lineno;
	char buffer[1024];
	int size = 0;
	CLS_FETCH();
	ELS_FETCH();
	PLS_FETCH();

	switch (type) {
		case E_CORE_ERROR:
		case E_CORE_WARNING:
			error_filename = NULL;
			error_lineno = 0;
			break;
		case E_PARSE:
		case E_COMPILE_ERROR:
		case E_COMPILE_WARNING:
		case E_ERROR:
		case E_NOTICE:
		case E_WARNING:
			if (zend_is_compiling()) {
				error_filename = zend_get_compiled_filename(CLS_C);
				error_lineno = zend_get_compiled_lineno(CLS_C);
			} else if (zend_is_executing()) {
				error_filename = zend_get_executed_filename(ELS_C);
				error_lineno = zend_get_executed_lineno(ELS_C);
			} else {
				error_filename = NULL;
				error_lineno = 0;
			}
			break;
		default:
			error_filename = NULL;
			error_lineno = 0;
			break;
	}

	if (!error_filename) {
		error_filename = "Unknown";
	}
		
	if (EG(error_reporting) & type || (type & E_CORE)) {
		char *error_type_str;

		switch (type) {
			case E_ERROR:
			case E_CORE_ERROR:
			case E_COMPILE_ERROR:
				error_type_str = "Fatal error";
				break;
			case E_WARNING:
			case E_CORE_WARNING:
			case E_COMPILE_WARNING:
				error_type_str = "Warning";
				break;
			case E_PARSE:
				error_type_str = "Parse error";
				break;
			case E_NOTICE:
				error_type_str = "Warning";
				break;
			default:
				error_type_str = "Unknown error";
				break;
		}

		/* get include file name */
		if (PG(log_errors) || PG(display_errors) || (!module_initialized)) {
			va_start(args, format);
			size = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
			va_end(args);
			buffer[sizeof(buffer) - 1] = 0;

			if (PG(log_errors) || (!module_initialized)) {
				char log_buffer[1024];

				snprintf(log_buffer, 1024, "PHP %s:  %s in %s on line %d", error_type_str, buffer, error_filename, error_lineno);
				php_log_err(log_buffer);
			}
			if (PG(display_errors)) {
				char *prepend_string = INI_STR("error_prepend_string");
				char *append_string = INI_STR("error_append_string");

				if (prepend_string) {
					PUTS(prepend_string);
				}		
				php_printf("<br>\n<b>%s</b>:  %s in <b>%s</b> on line <b>%d</b><br>\n", error_type_str, buffer, error_filename, error_lineno);
				if (append_string) {
					PUTS(append_string);
				}		
			}
		}
	}

	switch (type) {
		case E_ERROR:
		case E_CORE_ERROR:
		/*case E_PARSE: the parser would return 1 (failure), we can bail out nicely */
		case E_COMPILE_ERROR:
			if (module_initialized) {
				zend_bailout();
				return;
			}
			break;
	}

	if (PG(track_errors)) {
		pval *tmp;

		va_start(args, format);
		size = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
		va_end(args);
		buffer[sizeof(buffer) - 1] = 0;

		ALLOC_ZVAL(tmp);
		INIT_PZVAL(tmp);
		tmp->value.str.val = (char *) estrndup(buffer, size);
		tmp->value.str.len = size;
		tmp->type = IS_STRING;

		zend_hash_update(EG(active_symbol_table), "php_errormsg", sizeof("php_errormsg"), (void **) & tmp, sizeof(pval *), NULL);
	}
}


static long php_timeout_seconds;

#ifdef HAVE_SETITIMER
static void php_timeout(int dummy)
{
	PLS_FETCH();

	PG(connection_status) |= PHP_CONNECTION_TIMEOUT;
	php_error(E_ERROR, "Maximum execution time of %d second%s exceeded",
			  php_timeout_seconds, php_timeout_seconds == 1 ? "" : "s");
}
#endif

/* This one doesn't exists on QNX */
#ifndef SIGPROF
#define SIGPROF 27
#endif

static void php_set_timeout(long seconds)
{
#if PHP_WIN32
#else
#	ifdef HAVE_SETITIMER
	struct itimerval t_r;		/* timeout requested */

	t_r.it_value.tv_sec = seconds;
	t_r.it_value.tv_usec = t_r.it_interval.tv_sec = t_r.it_interval.tv_usec = 0;

	php_timeout_seconds = seconds;
	setitimer(ITIMER_PROF, &t_r, NULL);
	signal(SIGPROF, php_timeout);
#	endif
#endif
}


static void php_unset_timeout(void)
{
#if PHP_WIN32
#else
#	ifdef HAVE_SETITIMER
	struct itimerval no_timeout;

	no_timeout.it_value.tv_sec = no_timeout.it_value.tv_usec = no_timeout.it_interval.tv_sec = no_timeout.it_interval.tv_usec = 0;

	setitimer(ITIMER_PROF, &no_timeout, NULL);
#	endif
#endif
}


PHP_FUNCTION(set_time_limit)
{
	pval *new_timeout;
	PLS_FETCH();

	if (PG(safe_mode)) {
		php_error(E_WARNING, "Cannot set time limit in safe mode");
		RETURN_FALSE;
	}
	if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &new_timeout) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long(new_timeout);
	/* FIXME ** This is BAD...in a threaded situation, any user
	   can set the timeout for php on a server wide basis. 
	   INI variables should not be reset via a user script

	   Fix what?  At least on Unix, timers like these are
	   per-thread timers.  Well, with a little work they will
	   be.  If we use a bound thread and proper masking it
	   should work fine.  Is this FIXME a WIN32 problem?  Is
	   there no way to do per-thread timers on WIN32?
	 */
	php_unset_timeout();
	php_set_timeout(new_timeout->value.lval);
}


static FILE *php_fopen_wrapper_for_zend(const char *filename, char **opened_path)
{
	int issock=0, socketd=0;
	int old_chunk_size;
	FILE *retval;
	
	old_chunk_size = php_sock_set_def_chunk_size(1);
	retval=php_fopen_wrapper((char *) filename, "r", USE_PATH|IGNORE_URL_WIN, &issock, &socketd, opened_path);
	php_sock_set_def_chunk_size(old_chunk_size);
	
	if (issock) {
		retval = fdopen(socketd, "r");
	}
	return retval;
}


static int php_get_ini_entry_for_zend(char *name, uint name_length, zval *contents)
{
	zval *retval = cfg_get_entry(name, name_length);

	if (retval) {
		*contents = *retval;
		return SUCCESS;
	} else {
		return FAILURE;
	}
}


static void php_message_handler_for_zend(long message, void *data)
{
	switch (message) {
		case ZMSG_ENABLE_TRACK_VARS: {
				PLS_FETCH();
				
				PG(track_vars) = 1;
			}
			break;
		case ZMSG_FAILED_INCLUDE_FOPEN: {
				PLS_FETCH();

				php_error(E_WARNING, "Failed opening '%s' for inclusion (include_path='%s')", php_strip_url_passwd((char *) data), STR_PRINT(PG(include_path)));
			}
			break;
		case ZMSG_FAILED_REQUIRE_FOPEN: {
				PLS_FETCH();

				php_error(E_COMPILE_ERROR, "Failed opening required '%s' (include_path='%s')", php_strip_url_passwd((char *) data), STR_PRINT(PG(include_path)));
			}
			break;
		case ZMSG_FAILED_HIGHLIGHT_FOPEN:
			php_error(E_WARNING, "Failed opening '%s' for highlighting", php_strip_url_passwd((char *) data));
			break;
		case ZMSG_MEMORY_LEAK_DETECTED:
		case ZMSG_MEMORY_LEAK_REPEATED: {
				ELS_FETCH();

				if (EG(error_reporting)&E_WARNING) {
#if ZEND_DEBUG
					char memory_leak_buf[512];
					SLS_FETCH();

					if (message==ZMSG_MEMORY_LEAK_DETECTED) {
						zend_mem_header *t = (zend_mem_header *) data;
						void *ptr = (void *)((char *)t+sizeof(zend_mem_header)+PLATFORM_PADDING);

						snprintf(memory_leak_buf, 512, "%s(%d) :  Freeing 0x%.8lX (%d bytes), script=%s\n", t->filename, t->lineno, (unsigned long)ptr, t->size, SAFE_FILENAME(SG(request_info).path_translated));
						if (t->orig_filename) {
							char relay_buf[512];

							snprintf(relay_buf, 512, "%s(%d) : Actual location (location was relayed)\n", t->orig_filename, t->orig_lineno);
							strcat(memory_leak_buf, relay_buf);
						}
					} else {
						unsigned long leak_count = (unsigned long) data;

						snprintf(memory_leak_buf, 512, "Last leak repeated %ld time%s\n", leak_count, (leak_count>1?"s":""));
					}
#	if PHP_WIN32
					OutputDebugString(memory_leak_buf);
#	else
					fprintf(stderr, memory_leak_buf);
#	endif
#endif
				}
			}
			break;
		case ZMSG_LOG_SCRIPT_NAME: {
				struct tm *ta, tmbuf;
				time_t curtime;
				char *datetime_str, asctimebuf[52];
				SLS_FETCH();

				time(&curtime);
				ta = localtime_r(&curtime, &tmbuf);
				datetime_str = asctime_r(ta, asctimebuf);
				datetime_str[strlen(datetime_str)-1]=0;	/* get rid of the trailing newline */
				fprintf(stderr, "[%s]  Script:  '%s'\n", datetime_str, SAFE_FILENAME(SG(request_info).path_translated));
			}
			break;
	}
}


int php_request_startup(CLS_D ELS_DC PLS_DC SLS_DC)
{
	global_lock();
	
	php_output_startup();

	/* initialize global variables */
	PG(header_is_being_sent)=0;

	if (php_init_request_info(NULL)) {
		php_printf("Unable to initialize request info.\n");
		return FAILURE;
	}
	
	zend_activate(CLS_C ELS_CC);
	sapi_activate(SLS_C);	

	php_set_timeout(PG(max_execution_time));
	
	if (PG(expose_php)) {
		sapi_add_header(estrdup(SAPI_PHP_VERSION_HEADER), sizeof(SAPI_PHP_VERSION_HEADER) - 1);
	}

	if (PG(output_buffering)) {
		php_start_ob_buffering();
	} else if (PG(implicit_flush)) {
		php_start_implicit_flush();
	}
	
	return SUCCESS;
}


void php_request_shutdown_for_exec(void *dummy)
{
	/* used to close fd's in the 3..255 range here, but it's problematic
	 */
	shutdown_memory_manager(1, 1);
}


void php_request_shutdown(void *dummy)
{
	CLS_FETCH();
	ELS_FETCH();
	SLS_FETCH();

	sapi_send_headers();
	php_end_ob_buffering(SG(request_info).headers_only?0:1);

	php_call_shutdown_functions();
	
	php_ini_rshutdown();

	zend_deactivate(CLS_C ELS_CC);
	sapi_deactivate(SLS_C);

	php_destroy_request_info(NULL);
	shutdown_memory_manager(CG(unclean_shutdown), 0);
	php_unset_timeout();

	global_unlock();
}


static int php_config_ini_startup(void)
{
	if (php_init_config() == FAILURE) {
		php_printf("PHP:  Unable to parse configuration file.\n");
		return FAILURE;
	}
	return SUCCESS;
}

static void php_config_ini_shutdown(void)
{
	php_shutdown_config();
}


static int php_body_write_wrapper(const char *str, uint str_length)
{
	return php_body_write(str, str_length);
}

#ifdef ZTS
static void php_new_thread_end_handler(THREAD_T thread_id)
{
	php_ini_refresh_caches();
}
#endif


#ifdef ZTS
static void core_globals_ctor(php_core_globals *core_globals)
{
	memset(core_globals,0,sizeof(*core_globals));
}

#endif


int php_startup_extensions(zend_module_entry **ptr, int count)
{
	zend_module_entry **end = ptr+count;

	while (ptr < end) {
		if (*ptr) {
			if (zend_startup_module(*ptr)==FAILURE) {
				return FAILURE;
			}
		}
		ptr++;
	}
	return SUCCESS;
}

int php_global_startup_extensions(zend_module_entry **ptr, int count)
{
	zend_module_entry **end = ptr+count;

	while (ptr < end) {
		if (*ptr) {
			if ((*ptr)->global_startup_func && 
					(*ptr)->global_startup_func()==FAILURE) {
				return FAILURE;
			}
		}
		ptr++;
	}
	return SUCCESS;
}

int php_global_shutdown_extensions(zend_module_entry **ptr, int count)
{
	zend_module_entry **end = ptr+count;

	while (ptr < end) {
		if (*ptr) {
			if ((*ptr)->global_shutdown_func && 
					(*ptr)->global_shutdown_func()==FAILURE) {
				return FAILURE;
			}
		}
		ptr++;
	}
	return SUCCESS;
}


int php_module_startup(sapi_module_struct *sf)
{
	zend_utility_functions zuf;
	zend_utility_values zuv;
	int module_number=0;	/* for REGISTER_INI_ENTRIES() */
	char *php_os;
#ifdef ZTS
	zend_executor_globals *executor_globals;
	php_core_globals *core_globals;
	sapi_globals_struct *sapi_globals = ts_resource(sapi_globals_id);
#endif
#if (PHP_WIN32) && !(USE_SAPI)
	WORD wVersionRequested = MAKEWORD(2, 0);
	WSADATA wsaData;
#endif
#if PHP_WIN32
	{
		DWORD dwVersion = GetVersion();

		/* Get build numbers for Windows NT or Win95 */
		if (dwVersion < 0x80000000){
			php_os="WINNT";
		} else {
			php_os="WIN32";
		}
	}
#else
    php_os=PHP_OS;
#endif

	global_lock_init();
	SG(server_context) = NULL;
	SG(request_info).request_method = NULL;
	sapi_activate(SLS_C);

	if (module_initialized) {
		return SUCCESS;
	}

	sapi_module = *sf;

	php_output_startup();

	zuf.error_function = php_error;
	zuf.printf_function = php_printf;
	zuf.write_function = php_body_write_wrapper;
	zuf.fopen_function = php_fopen_wrapper_for_zend;
	zuf.message_handler = php_message_handler_for_zend;
	zuf.block_interruptions = sapi_module.block_interruptions;
	zuf.unblock_interruptions = sapi_module.unblock_interruptions;
	zuf.get_ini_entry = php_get_ini_entry_for_zend;
	zuf.ticks_function = NULL;
	zend_startup(&zuf, NULL);

#ifdef ZTS
	tsrm_set_new_thread_end_handler(php_new_thread_end_handler);
	executor_globals = ts_resource(executor_globals_id);
	core_globals_id = ts_allocate_id(sizeof(php_core_globals), (ts_allocate_ctor) core_globals_ctor, NULL);
	core_globals = ts_resource(core_globals_id);
#endif
	EG(error_reporting) = E_ALL & ~E_NOTICE;
	
	PG(header_is_being_sent) = 0;
	SG(request_info).headers_only = 0;
	SG(request_info).argv0 = NULL;
	PG(connection_status) = PHP_CONNECTION_NORMAL;

#if HAVE_SETLOCALE
	setlocale(LC_CTYPE, "");
#endif

#if (PHP_WIN32) && !(USE_SAPI)
	/* start up winsock services */
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		php_printf("\nwinsock.dll unusable. %d\n", WSAGetLastError());
		return FAILURE;
	}
#endif

	SET_MUTEX(gLock);
	le_index_ptr = _register_list_destructors(NULL, NULL, 0);
	FREE_MUTEX(gLock);

	php_ini_mstartup();

	if (php_config_ini_startup() == FAILURE) {
		return FAILURE;
	}

	REGISTER_INI_ENTRIES();

	zuv.short_tags = (zend_bool) PG(short_tags);
	zuv.asp_tags = (zend_bool) PG(asp_tags);
	zuv.allow_call_time_pass_reference = PG(allow_call_time_pass_reference);
	zuv.import_use_extension = ".php";
	zend_set_utility_values(&zuv);
	php_startup_SAPI_content_types();

    REGISTER_MAIN_STRINGL_CONSTANT("PHP_VERSION", PHP_VERSION, sizeof(PHP_VERSION)-1, CONST_PERSISTENT | CONST_CS);
    REGISTER_MAIN_STRINGL_CONSTANT("PHP_OS", php_os, strlen(php_os), CONST_PERSISTENT | CONST_CS);

	if (php_startup_internal_extensions() == FAILURE) {
		php_printf("Unable to start builtin modules\n");
		return FAILURE;
	}
	module_initialized = 1;
	sapi_deactivate(SLS_C);
	return SUCCESS;
}



void php_module_shutdown_for_exec()
{
	/* used to close fd's in the range 3.255 here, but it's problematic */
}


int php_module_shutdown_wrapper(sapi_module_struct *sapi_globals)
{
	php_module_shutdown();
	return SUCCESS;
}


void php_module_shutdown()
{
	int module_number=0;	/* for UNREGISTER_INI_ENTRIES() */

	if (!module_initialized) {
		return;
	}

	/* close down the ini config */
	php_config_ini_shutdown();

#if (PHP_WIN32)
	/*close winsock */
	WSACleanup();
#endif

	sapi_flush();

	global_lock_destroy();
	zend_shutdown();
	UNREGISTER_INI_ENTRIES();
	php_ini_mshutdown();
	shutdown_memory_manager(0, 1);
	module_initialized = 0;
}


static inline void php_register_server_variables(ELS_D SLS_DC PLS_DC)
{
	zval *array_ptr=NULL;

	if (PG(track_vars)) {
		ALLOC_ZVAL(array_ptr);
		array_init(array_ptr);
		INIT_PZVAL(array_ptr);
		zend_hash_add(&EG(symbol_table), "HTTP_SERVER_VARS", sizeof("HTTP_SERVER_VARS"), &array_ptr, sizeof(pval *),NULL);
	}

	/* Server variables */
	if (sapi_module.register_server_variables) {
		sapi_module.register_server_variables(array_ptr ELS_CC SLS_CC PLS_CC);
	}

	/* argv/argc support */
	if (PG(register_argc_argv)) {
		php_build_argv(SG(request_info).query_string, array_ptr ELS_CC PLS_CC);
	}

	/* PHP Authentication support */
	if (SG(request_info).auth_user) {
		php_register_variable("PHP_AUTH_USER", SG(request_info).auth_user, array_ptr ELS_CC PLS_CC);
	}
	if (SG(request_info).auth_password) {
		php_register_variable("PHP_AUTH_PW", SG(request_info).auth_password, array_ptr ELS_CC PLS_CC);
	}
}


static int php_hash_environment(ELS_D SLS_DC PLS_DC)
{
	char *p;
	unsigned char _gpc_flags[3] = {0,0,0};
	zend_bool have_variables_order;

	if (PG(variables_order)) {
		p = PG(variables_order);
		have_variables_order=1;
	} else {
		p = PG(gpc_order);
		have_variables_order=0;
		php_import_environment_variables(ELS_C PLS_CC);
	}

	while(*p) {
		switch(*p++) {
			case 'p':
			case 'P':
				if (!_gpc_flags[0] && !SG(headers_sent) && SG(request_info).request_method && !strcasecmp(SG(request_info).request_method, "POST")) {
					php_treat_data(PARSE_POST, NULL ELS_CC PLS_CC SLS_CC);	/* POST Data */
					_gpc_flags[0]=1;
				}
				break;
			case 'c':
			case 'C':
				if (!_gpc_flags[1]) {
					php_treat_data(PARSE_COOKIE, NULL ELS_CC PLS_CC SLS_CC);	/* Cookie Data */
					_gpc_flags[1]=1;
				}
				break;
			case 'g':
			case 'G':
				if (!_gpc_flags[2]) {
					php_treat_data(PARSE_GET, NULL ELS_CC PLS_CC SLS_CC);	/* GET Data */
					_gpc_flags[2]=1;
				}
				break;
			case 'e':
			case 'E':
				if (have_variables_order) {
					php_import_environment_variables(ELS_C PLS_CC);
				} else {
					php_error(E_CORE_WARNING, "Unsupported 'e' element (environment) used in gpc_order - use variables_order instead");
				}
				break;
			case 's':
			case 'S':
				if (sapi_module.register_server_variables) {
					php_register_server_variables(ELS_C SLS_CC PLS_CC);
				}
				break;
		}
	}


	if (!have_variables_order) {
		php_register_server_variables(ELS_C SLS_CC PLS_CC);
	}

	return SUCCESS;
}


static void php_build_argv(char *s, zval *track_vars_array ELS_DC PLS_DC)
{
	pval *arr, *argc, *tmp;
	int count = 0;
	char *ss, *space;

	ALLOC_ZVAL(arr);
	array_init(arr);
	INIT_PZVAL(arr);

	/* Prepare argv */
	ss = s;
	while (ss) {
		space = strchr(ss, '+');
		if (space) {
			*space = '\0';
		}
		/* auto-type */
		ALLOC_ZVAL(tmp);
		tmp->type = IS_STRING;
		tmp->value.str.len = strlen(ss);
		tmp->value.str.val = estrndup(ss, tmp->value.str.len);
		INIT_PZVAL(tmp);
		count++;
		if (zend_hash_next_index_insert(arr->value.ht, &tmp, sizeof(pval *), NULL)==FAILURE) {
			if (tmp->type == IS_STRING) {
				efree(tmp->value.str.val);
			}
		}
		if (space) {
			*space = '+';
			ss = space + 1;
		} else {
			ss = space;
		}
	}

	/* prepare argc */
	ALLOC_ZVAL(argc);
	argc->value.lval = count;
	argc->type = IS_LONG;
	INIT_PZVAL(argc);

	if (PG(register_globals)) {
		zend_hash_update(&EG(symbol_table), "argv", sizeof("argv"), &arr, sizeof(pval *), NULL);
		zend_hash_add(&EG(symbol_table), "argc", sizeof("argc"), &argc, sizeof(pval *), NULL);
	}

	if (PG(track_vars)) {
		if (PG(register_globals)) {
			arr->refcount++;
			argc->refcount++;
		}
		zend_hash_update(track_vars_array->value.ht, "argv", sizeof("argv"), &arr, sizeof(pval *), NULL);
		zend_hash_update(track_vars_array->value.ht, "argc", sizeof("argc"), &argc, sizeof(pval *), NULL);
	}

}


#include "logos.h"

PHPAPI void php_execute_script(zend_file_handle *primary_file CLS_DC ELS_DC PLS_DC)
{
	zend_file_handle *prepend_file_p, *append_file_p;
	zend_file_handle prepend_file, append_file;
	SLS_FETCH();

	php_hash_environment(ELS_C SLS_CC PLS_CC);
	zend_activate_modules();
	if (SG(request_info).query_string && SG(request_info).query_string[0]=='=' 
		&& PG(expose_php)) {
		if (!strcmp(SG(request_info).query_string+1, PHP_LOGO_GUID)) {
			char *header_line = estrndup(CONTEXT_TYPE_IMAGE_GIF, sizeof(CONTEXT_TYPE_IMAGE_GIF));

			php4i_add_header_information(header_line, sizeof(CONTEXT_TYPE_IMAGE_GIF)-1);
			PHPWRITE(php4_logo, sizeof(php4_logo));
			return;
		} else if (!strcmp(SG(request_info).query_string+1, ZEND_LOGO_GUID)) {
			char *header_line = estrndup(CONTEXT_TYPE_IMAGE_GIF, sizeof(CONTEXT_TYPE_IMAGE_GIF));

			php4i_add_header_information(header_line, sizeof(CONTEXT_TYPE_IMAGE_GIF)-1);
			PHPWRITE(zend_logo, sizeof(zend_logo));
			return;
		} else if (!strcmp(SG(request_info).query_string+1, "PHPB8B5F2A0-3C92-11d3-A3A9-4C7B08C10000")) {
			php_print_credits(PHP_CREDITS_ALL);
			return;
		}
	}

	if (setjmp(EG(bailout))!=0) {
		return;
	}

#if PHP_WIN32
	UpdateIniFromRegistry(primary_file->filename);
#endif

	if (PG(auto_prepend_file) && PG(auto_prepend_file)[0]) {
		prepend_file.filename = PG(auto_prepend_file);
		prepend_file.free_filename = 0;
		prepend_file.type = ZEND_HANDLE_FILENAME;
		prepend_file_p = &prepend_file;
	} else {
		prepend_file_p = NULL;
	}
	if (PG(auto_append_file) && PG(auto_append_file)[0]) {
		append_file.filename = PG(auto_append_file);
		append_file.free_filename = 0;
		append_file.type = ZEND_HANDLE_FILENAME;
		append_file_p = &append_file;
	} else {
		append_file_p = NULL;
	}	
	EG(main_op_array) = zend_compile_files(0 CLS_CC, 3, prepend_file_p, primary_file, append_file_p);
	if (EG(main_op_array)) {
		EG(active_op_array) = EG(main_op_array);
		zend_execute(EG(main_op_array) ELS_CC);
	}
}

#if PHP_WIN32
/* just so that this symbol gets exported... */
PHPAPI void dummy_indent()
{
	zend_indent();
}
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
