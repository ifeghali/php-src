/* 
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999 The PHP Group                         |
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
#include "ext/standard/php3_standard.h"
#include "snprintf.h"
#if WIN32|WINNT
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

#if USE_SAPI
#include "serverapi/sapi.h"
void *gLock;
#ifndef THREAD_SAFE
struct sapi_request_info *sapi_rqst;
#endif
#endif


#include "SAPI.h"

#if MSVC5 || !defined(HAVE_GETOPT)
#include "php_getopt.h"
#endif


#ifndef ZTS
php_core_globals core_globals;
#else
PHPAPI int core_globals_id;
#endif

void _php3_build_argv(char * ELS_DC);
static void php3_timeout(int dummy);
static void php3_set_timeout(long seconds);

void *gLock;					/*mutex variable */


/* True globals (no need for thread safety) */
HashTable configuration_hash;
PHPAPI char *php3_ini_path = NULL;
#ifdef ZTS
php_core_globals *main_core_globals=NULL;
#endif


#define SAFE_FILENAME(f) ((f)?(f):"-")

static PHP_INI_MH(OnSetPrecision)
{
	ELS_FETCH();

	EG(precision) = atoi(new_value);
	return SUCCESS;
}


static PHP_INI_MH(OnChangeMaxExecutionTime)
{
	int new_timeout;
	
	if (new_value) {
		new_timeout = atoi(new_value);
	} else {
		new_timeout = 0;
	}
	php3_set_timeout(new_timeout);
	return SUCCESS;
}


static PHP_INI_MH(OnChangeMemoryLimit)
{
	int new_limit;

	if (new_value) {
		new_limit = atoi(new_value);
	} else {
		new_limit = 2<<30;		/* effectively, no limit */
	}
	return zend_set_memory_limit(new_limit);
}


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
	STD_PHP_INI_BOOLEAN("short_open_tag",	"1",		PHP_INI_ALL,		OnUpdateInt,		short_tags,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("asp_tags",			"0",		PHP_INI_ALL,		OnUpdateInt,		asp_tags,		php_core_globals,	core_globals)
	PHP_INI_ENTRY("precision",			"14",		PHP_INI_ALL,		OnSetPrecision)

	PHP_INI_ENTRY_EX("highlight.comment",	HL_COMMENT_COLOR,	PHP_INI_ALL,		NULL,		php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.default",	HL_DEFAULT_COLOR,	PHP_INI_ALL,		NULL,		php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.html",		HL_HTML_COLOR,		PHP_INI_ALL,		NULL,		php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.string",	HL_STRING_COLOR,	PHP_INI_ALL,		NULL,		php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.bg",		HL_BG_COLOR,		PHP_INI_ALL,		NULL,		php_ini_color_displayer_cb)
	PHP_INI_ENTRY_EX("highlight.keyword",	HL_KEYWORD_COLOR,	PHP_INI_ALL,		NULL,		php_ini_color_displayer_cb)

	STD_PHP_INI_BOOLEAN("magic_quotes_gpc",		"1",	PHP_INI_ALL,		OnUpdateInt,		magic_quotes_gpc,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("magic_quotes_runtime",	"0",	PHP_INI_ALL,		OnUpdateInt,		magic_quotes_runtime,	php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("magic_quotes_sybase",	"0",	PHP_INI_ALL,		OnUpdateInt,		magic_quotes_sybase,	php_core_globals,	core_globals)

	STD_PHP_INI_BOOLEAN("safe_mode",		"0",		PHP_INI_SYSTEM,			OnUpdateInt,		safe_mode,				php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("sql.safe_mode",	"0",		PHP_INI_SYSTEM,			OnUpdateInt,		sql_safe_mode,			php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("safe_mode_exec_dir",	"1",	PHP_INI_SYSTEM,			OnUpdateString,	safe_mode_exec_dir,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("enable_dl",		"1",		PHP_INI_SYSTEM,			OnUpdateInt,		enable_dl,				php_core_globals,	core_globals)

	PHP_INI_ENTRY("SMTP",			"localhost",			PHP_INI_ALL,		NULL)
	PHP_INI_ENTRY("sendmail_path",	DEFAULT_SENDMAIL_PATH,	PHP_INI_SYSTEM,		NULL)
	PHP_INI_ENTRY("sendmail_from",	NULL,					PHP_INI_ALL,		NULL)

	PHP_INI_ENTRY("error_reporting",	NULL,				PHP_INI_ALL,		OnUpdateErrorReporting)
	STD_PHP_INI_BOOLEAN("display_errors",		"1",			PHP_INI_ALL,		OnUpdateInt,		display_errors,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("track_errors",		"0",			PHP_INI_ALL,		OnUpdateInt,		track_errors,		php_core_globals,	core_globals)
	STD_PHP_INI_BOOLEAN("log_errors",			"0",			PHP_INI_ALL,		OnUpdateInt,		log_errors,			php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("error_log",			NULL,			PHP_INI_ALL,		OnUpdateString,	error_log,			php_core_globals,	core_globals)


	STD_PHP_INI_ENTRY("auto_prepend_file",	NULL,			PHP_INI_ALL,		OnUpdateString,	auto_prepend_file,	php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("auto_append_file",	NULL,			PHP_INI_ALL,		OnUpdateString,	auto_append_file,	php_core_globals,	core_globals)

	STD_PHP_INI_BOOLEAN("y2k_compliance",		"0",			PHP_INI_ALL,		OnUpdateInt,		y2k_compliance,		php_core_globals,	core_globals)

	STD_PHP_INI_ENTRY("doc_root",			NULL,			PHP_INI_SYSTEM,		OnUpdateStringUnempty,	doc_root,		php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("user_dir",			NULL,			PHP_INI_SYSTEM,		OnUpdateStringUnempty,	user_dir,		php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("include_path",		NULL,			PHP_INI_ALL,		OnUpdateStringUnempty,	include_path,	php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("open_basedir",		NULL,			PHP_INI_SYSTEM,		OnUpdateStringUnempty,	open_basedir,	php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("extension_dir",		NULL,			PHP_INI_SYSTEM,		OnUpdateStringUnempty,	extension_dir,	php_core_globals,	core_globals)

	STD_PHP_INI_ENTRY("upload_tmp_dir",			NULL,			PHP_INI_SYSTEM,		OnUpdateStringUnempty,	upload_tmp_dir,			php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("upload_max_filesize",	"2097152",		PHP_INI_ALL,		OnUpdateInt,				upload_max_filesize,	php_core_globals,	core_globals)

	PHP_INI_ENTRY("browscap",			NULL,				PHP_INI_SYSTEM,		NULL)

	PHP_INI_ENTRY_EX("define_syslog_variables",	"0",		PHP_INI_ALL,		NULL,	php_ini_boolean_displayer_cb)

	PHP_INI_ENTRY("max_execution_time",		"30",			PHP_INI_ALL,		OnChangeMaxExecutionTime)
	PHP_INI_ENTRY("memory_limit",			"8388608",		PHP_INI_ALL,		OnChangeMemoryLimit)

	STD_PHP_INI_BOOLEAN("track_vars",			(PHP_TRACK_VARS?"1":"0"),			PHP_INI_ALL,		OnUpdateInt,				track_vars,		php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("gpc_order",				"GPC",			PHP_INI_ALL,		OnUpdateStringUnempty,	gpc_order,		php_core_globals,	core_globals)
	STD_PHP_INI_ENTRY("arg_separator",			"&",			PHP_INI_ALL,		OnUpdateStringUnempty,	arg_separator,	php_core_globals,	core_globals)
PHP_INI_END()



/* True global (no need for thread safety */
static int module_initialized = 0;


#if APACHE
void php3_apache_puts(const char *s)
{
	SLS_FETCH();
	
	if (SG(server_context)) {
		rputs(s, (request_rec *) SG(server_context));
	} else {
		fputs(s, stdout);
	}
}

void php3_apache_putc(char c)
{
	SLS_FETCH();
	
	if (SG(server_context)) {
		rputc(c, (request_rec *) SG(server_context));
	} else {
		fputc(c, stdout);
	}
}
#endif

void php3_log_err(char *log_message)
{
	FILE *log_file;
	PLS_FETCH();
	SLS_FETCH();

	/* Try to use the specified logging location. */
	if (PG(error_log) != NULL) {
#if HAVE_SYSLOG_H
		if (strcmp(PG(error_log), "syslog")) {
			syslog(LOG_NOTICE, log_message);
			return;
		} else {
#endif
			log_file = fopen(PG(error_log), "a");
			if (log_file != NULL) {
				fprintf(log_file, log_message);
				fprintf(log_file, "\n");
				fclose(log_file);
				return;
			}
#if HAVE_SYSLOG_H
		}
#endif
	}
	/* Otherwise fall back to the default logging location. */
#if APACHE
	if (SG(server_context)) {
#if MODULE_MAGIC_NUMBER >= 19970831
		aplog_error(NULL, 0, APLOG_ERR | APLOG_NOERRNO, ((request_rec *) SG(server_context))->server, log_message);
#else
		log_error(log_message, ((requset_rec *) SG(server_context))->server);
#endif
	} else {
		fprintf(stderr, log_message);
		fprintf(stderr, "\n");
	}
#endif							/*APACHE */

#if CGI_BINARY
	if (php3_header()) {
		fprintf(stderr, log_message);
		fprintf(stderr, "\n");
	}
#endif
}


/* is 4K big enough? */
#define PRINTF_BUFFER_SIZE 1024*4

/* wrapper for modules to use PHPWRITE */
PHPAPI int php3_write(void *buf, int size)
{
	return PHPWRITE(buf, size);
}

PHPAPI int php3_printf(const char *format,...)
{
	va_list args;
	int ret;
	char buffer[PRINTF_BUFFER_SIZE];
	int size;

	va_start(args, format);
	size = vsprintf(buffer, format, args);
	ret = PHPWRITE(buffer, size);
	va_end(args);
	
	return ret;
}


/* extended error handling function */
PHPAPI void php3_error(int type, const char *format,...)
{
	va_list args;
	char *error_filename = NULL;
	uint error_lineno;
	char buffer[1024];
	int size = 0;
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
		case E_COMPILE_WARNING: {
				CLS_FETCH();

				error_filename = zend_get_compiled_filename();
				error_lineno = CG(zend_lineno);
				if (!error_filename) {
					error_filename = zend_get_executed_filename(ELS_C);
				}
			}
			break;
		case E_ERROR:
		case E_NOTICE:
		case E_WARNING:
			error_filename = zend_get_executed_filename(ELS_C);
			error_lineno = zend_get_executed_lineno(ELS_C);
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
		if (PG(log_errors) || PG(display_errors)) {
			va_start(args, format);
			size = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
			va_end(args);
			buffer[sizeof(buffer) - 1] = 0;

			if (PG(log_errors)) {
				char log_buffer[1024];

				snprintf(log_buffer, 1024, "PHP 3 %s:  %s in %s on line %d", error_type_str, buffer, error_filename, error_lineno);
				php3_log_err(log_buffer);
			}
			if (PG(display_errors)) {
				char *prepend_string = INI_STR("error_prepend_string");
				char *append_string = INI_STR("error_append_string");

				if (prepend_string) {
					PUTS(prepend_string);
				}		
				php3_printf("<br>\n<b>%s</b>:  %s in <b>%s</b> on line <b>%d</b><br>\n", error_type_str, buffer, error_filename, error_lineno);
				if (append_string) {
					PUTS(append_string);
				}		
			}
		}
	}
	if (PG(track_errors)) {
		pval *tmp;

		va_start(args, format);
		size = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
		va_end(args);
		buffer[sizeof(buffer) - 1] = 0;

		tmp = (pval *)emalloc(sizeof(pval));
		INIT_PZVAL(tmp);
		tmp->value.str.val = (char *) estrndup(buffer, size);
		tmp->value.str.len = size;
		tmp->type = IS_STRING;

		_php3_hash_update(EG(active_symbol_table), "php_errormsg", sizeof("php_errormsg"), (void **) & tmp, sizeof(pval *), NULL);
	}

	switch (type) {
		case E_ERROR:
		case E_CORE_ERROR:
		/*case E_PARSE: the parser would return 1 (failure), we can bail out nicely */
		case E_COMPILE_ERROR:
			zend_bailout();
			break;
	}
}




#ifdef HAVE_SETITIMER
static void php3_timeout(int dummy)
{
	PLS_FETCH();

	php3_error(E_ERROR, "Maximum execution time of %s seconds exceeded", INI_STR("max_execution_time"));
}
#endif

/* This one doesn't exists on QNX */
#ifndef SIGPROF
#define SIGPROF 27
#endif

static void php3_set_timeout(long seconds)
{
#if WIN32|WINNT
#else
#	ifdef HAVE_SETITIMER
	struct itimerval t_r;		/* timeout requested */

	t_r.it_value.tv_sec = seconds;
	t_r.it_value.tv_usec = t_r.it_interval.tv_sec = t_r.it_interval.tv_usec = 0;

	setitimer(ITIMER_PROF, &t_r, NULL);
	signal(SIGPROF, php3_timeout);
#	endif
#endif
}


static void php3_unset_timeout()
{
#if WIN32|WINNT
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
		php3_error(E_WARNING, "Cannot set time limit in safe mode");
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
	php3_unset_timeout();
	php3_set_timeout(new_timeout->value.lval);
}


static FILE *php_fopen_wrapper_for_zend(const char *filename)
{
	int issock=0, socketd=0;

	FILE *retval=php3_fopen_wrapper((char *) filename, "r", USE_PATH|IGNORE_URL_WIN, &issock, &socketd);
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

				php3_error(E_WARNING, "Failed opening '%s' for inclusion (include_path='%s')", php3_strip_url_passwd((char *) data), STR_PRINT(PG(include_path)));
			}
			break;
		case ZMSG_FAILED_REQUIRE_FOPEN: {
				PLS_FETCH();

				php3_error(E_COMPILE_ERROR, "Failed opening required '%s' (include_path='%s')", php3_strip_url_passwd((char *) data), STR_PRINT(PG(include_path)));
			}
			break;
		case ZMSG_FAILED_HIGHLIGHT_FOPEN:
			php3_error(E_WARNING, "Failed opening '%s' for highlighting", php3_strip_url_passwd((char *) data));
			break;
		case ZMSG_MEMORY_LEAK_DETECTED:
		case ZMSG_MEMORY_LEAK_REPEATED: {
				ELS_FETCH();
				SLS_FETCH();

				if (EG(error_reporting)&E_WARNING) {
#if ZEND_DEBUG
					char memory_leak_buf[512];
					SLS_FETCH();

					if (message==ZMSG_MEMORY_LEAK_DETECTED) {
						mem_header *t = (mem_header *) data;
						void *ptr = (void *)((char *)t+sizeof(mem_header)+PLATFORM_PADDING);

#if WIN32||WINNT
						snprintf(memory_leak_buf, 512, "%s(%d) :  Freeing 0x%0.8X (%d bytes), script=%s\n", t->filename, t->lineno, (unsigned long)ptr, t->size, SAFE_FILENAME(SG(request_info).path_translated));
#else
						snprintf(memory_leak_buf, 512, "%s:  Freeing 0x%0.8lX (%d bytes), allocated in %s on line %d<br>\n", SAFE_FILENAME(SG(request_info).path_translated), (unsigned long)ptr, t->size,t->filename,t->lineno);
#endif
					} else {
						unsigned long leak_count = (unsigned long) data;

#if WIN32||WINNT
						snprintf(memory_leak_buf, 512, "Last leak repeated %ld time%s\n", leak_count, (leak_count>1?"s":""));
#else
						snprintf(memory_leak_buf, 512, "%s:  Last leak repeated %ld time%s\n", SAFE_FILENAME(SG(request_info).path_translated), leak_count, (leak_count>1?"s":""));
#endif
					}
#	if WIN32||WINNT
					OutputDebugString(memory_leak_buf);
#	else
					fprintf(stderr, memory_leak_buf);
#	endif
#endif
				}
			}
			break;
		case ZMSG_LOG_SCRIPT_NAME: {
				SLS_FETCH();

				fprintf(stderr, "Script:  '%s'\n", SAFE_FILENAME(SG(request_info).path_translated));
			}
			break;
	}
}



int php_request_startup(CLS_D ELS_DC PLS_DC SLS_DC)
{
	zend_output_startup();

#if APACHE
	/*
	 * For the Apache module version, this bit of code registers a cleanup
	 * function that gets triggered when our request pool is destroyed.
	 * We need this because at any point in our code we can be interrupted
	 * and that may happen before we have had time to free our memory.
	 * The php3_shutdown function needs to free all outstanding allocated
	 * memory.  
	 */
	block_alarms();
	register_cleanup(((request_rec *) SG(server_context))->pool, NULL, php_request_shutdown, php_request_shutdown_for_exec);
	unblock_alarms();
#endif

	/* initialize global variables */
	{
		PG(header_is_being_sent)=0;
	}

	if (php3_init_request_info(NULL)) {
		php3_printf("Unable to initialize request info.\n");
		return FAILURE;
	}
	
	zend_activate(CLS_C ELS_CC);
	sapi_activate(SLS_C);	

	if (SG(request_info).auth_user) {
		zval *auth_user;

		MAKE_STD_ZVAL(auth_user);
		auth_user->type = IS_STRING;
		auth_user->value.str.val = SG(request_info).auth_user;
		auth_user->value.str.len = strlen(auth_user->value.str.val);

		zend_hash_update(&EG(symbol_table), "PHP_AUTH_USER", sizeof("PHP_AUTH_USER"), &auth_user, sizeof(zval *), NULL);
	}
	if (SG(request_info).auth_password) {
		zval *auth_password;

		MAKE_STD_ZVAL(auth_password);
		auth_password->type = IS_STRING;
		auth_password->value.str.val = SG(request_info).auth_password;
		auth_password->value.str.len = strlen(auth_password->value.str.val);

		zend_hash_update(&EG(symbol_table), "PHP_AUTH_PW", sizeof("PHP_AUTH_PW"), &auth_password, sizeof(zval *), NULL);
	}
	return SUCCESS;
}


void php_request_shutdown_for_exec(void *dummy)
{
	/* used to close fd's in the 3..255 range here, but it's problematic
	 */
	shutdown_memory_manager(1, 1);
}


int return_one(void *p)
{
	return 1;
}


void php_request_shutdown(void *dummy)
{
	CLS_FETCH();
	ELS_FETCH();
	PLS_FETCH();
	SLS_FETCH();

	sapi_send_headers();
	zend_end_ob_buffering(1);

	php3_call_shutdown_functions();
	
	php_ini_rshutdown();

	zend_deactivate(CLS_C ELS_CC);
	sapi_deactivate(SLS_C);

	php3_destroy_request_info(NULL);
	shutdown_memory_manager(CG(unclean_shutdown), 0);
	php3_unset_timeout();


#if CGI_BINARY
	fflush(stdout);
	if(request_info.php_argv0) {
		free(request_info.php_argv0);
		request_info.php_argv0 = NULL;
	}
#endif
}


static int php3_config_ini_startup()
{
	if (php3_init_config() == FAILURE) {
		php3_printf("PHP:  Unable to parse configuration file.\n");
		return FAILURE;
	}
	return SUCCESS;
}

static void php3_config_ini_shutdown()
{
	php3_shutdown_config();
}


#ifdef ZTS
static core_globals_ctor(php_core_globals *core_globals)
{
	if (main_core_globals) {
		*core_globals = *main_core_globals;
	}
}
#endif


static int zend_body_write_wrapper(const char *str, uint str_length)
{
	return zend_body_write(str, str_length);
}


int php_module_startup(sapi_module_struct *sf)
{
	zend_utility_functions zuf;
	zend_utility_values zuv;
	int module_number=0;	/* for REGISTER_INI_ENTRIES() */
#ifdef ZTS
	php_core_globals *core_globals;
	sapi_globals_struct *sapi_globals = ts_resource(sapi_globals_id);
#endif
#if (WIN32|WINNT) && !(USE_SAPI)
	WORD wVersionRequested = MAKEWORD(2, 0);
	WSADATA wsaData;
#endif
	ELS_FETCH();

	SG(server_context) = NULL;
	SG(request_info).request_method = NULL;
	sapi_activate(SLS_C);

	if (module_initialized) {
		return SUCCESS;
	}

	EG(error_reporting) = E_ALL & ~E_NOTICE;
	
	sapi_module = *sf;

	zend_output_startup();

	zuf.error_function = php3_error;
	zuf.printf_function = php3_printf;
	zuf.write_function = zend_body_write_wrapper;
	zuf.fopen_function = php_fopen_wrapper_for_zend;
	zuf.message_handler = php_message_handler_for_zend;
	zuf.block_interruptions = BLOCK_INTERRUPTIONS;
	zuf.unblock_interruptions = UNBLOCK_INTERRUPTIONS;
	zuf.get_ini_entry = php_get_ini_entry_for_zend;
	zend_startup(&zuf, NULL);

#ifdef ZTS
	core_globals_id = ts_allocate_id(sizeof(php_core_globals), core_globals_ctor, NULL);
	core_globals = ts_resource(core_globals_id);
	main_core_globals = core_globals;
#endif

	PG(header_is_being_sent) = 0;
	SG(request_info).headers_only = 0;

#if HAVE_SETLOCALE
	setlocale(LC_CTYPE, "");
#endif

#if (WIN32|WINNT) && !(USE_SAPI)
	/* start up winsock services */
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		php3_printf("\nwinsock.dll unusable. %d\n", WSAGetLastError());
		return FAILURE;
	}
#endif

	SET_MUTEX(gLock);
	le_index_ptr = _register_list_destructors(NULL, NULL, 0);
	FREE_MUTEX(gLock);

	if (php3_config_ini_startup() == FAILURE) {
		return FAILURE;
	}

	php_ini_mstartup();
	REGISTER_INI_ENTRIES();

	zuv.short_tags = (unsigned char) PG(short_tags);
	zuv.asp_tags = (unsigned char) PG(asp_tags);
	zend_set_utility_values(&zuv);

	if (module_startup_modules() == FAILURE) {
		php3_printf("Unable to start modules\n");
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
	CLS_FETCH();
	ELS_FETCH();

	if (!module_initialized) {
		return;
	}
#if !USE_SAPI
	/* close down the ini config */
	php3_config_ini_shutdown();
#endif

#if (WIN32|WINNT) && !(USE_SAPI)
	/*close winsock */
	WSACleanup();
#endif

#if CGI_BINARY
	fflush(stdout);
#endif
#if 0							/* SAPI */
	sapi_rqst->flush(sapi_rqst->scid);
#endif

	zend_shutdown();
	UNREGISTER_INI_ENTRIES();
	php_ini_mshutdown();
	shutdown_memory_manager(0, 1);
	module_initialized = 0;
}


/* in 3.1 some of this should move into sapi */
int _php3_hash_environment(PLS_D ELS_DC)
{
	char **env, *p, *t;
	unsigned char _gpc_flags[3] = {0,0,0};
	pval *tmp;
	SLS_FETCH();
	
	p = PG(gpc_order);
	while(*p) {
		switch(*p++) {
			case 'p':
			case 'P':
				if (!_gpc_flags[0] && !SG(headers_sent) && SG(request_info).request_method && !strcasecmp(SG(request_info).request_method, "POST")) {
					php3_treat_data(PARSE_POST, NULL);	/* POST Data */
					_gpc_flags[0]=1;
				}
				break;
			case 'c':
			case 'C':
				if (!_gpc_flags[1]) {
					php3_treat_data(PARSE_COOKIE, NULL);	/* Cookie Data */
					_gpc_flags[1]=1;
				}
				break;
			case 'g':
			case 'G':
				if (!_gpc_flags[2]) {
					php3_treat_data(PARSE_GET, NULL);	/* GET Data */
					_gpc_flags[2]=1;
				}
				break;
		}
	}

	
	for (env = environ; env != NULL && *env != NULL; env++) {
		p = strchr(*env, '=');
		if (!p) {				/* malformed entry? */
			continue;
		}
		t = estrndup(*env, p - *env);
		tmp = (pval *) emalloc(sizeof(pval));
		tmp->value.str.len = strlen(p + 1);
		tmp->value.str.val = estrndup(p + 1, tmp->value.str.len);
		tmp->type = IS_STRING;
		INIT_PZVAL(tmp);
		/* environmental variables never take precedence over get/post/cookie variables */
		_php3_hash_add(&EG(symbol_table), t, p - *env + 1, &tmp, sizeof(pval *), NULL);
		efree(t);
	}

#if APACHE
	{
		pval **tmp_ptr;
		register int i;
		array_header *arr = table_elts(((request_rec *) SG(server_context))->subprocess_env);
		table_entry *elts = (table_entry *) arr->elts;
		int len;

		for (i = 0; i < arr->nelts; i++) {
			len = strlen(elts[i].key);
			t = elts[i].key;
			tmp = (pval *) emalloc(sizeof(pval));
			if (elts[i].val) {
				tmp->value.str.len = strlen(elts[i].val);
				tmp->value.str.val = estrndup(elts[i].val, tmp->value.str.len);
			} else {
				tmp->value.str.len = 0;
				tmp->value.str.val = empty_string;
			}
			INIT_PZVAL(tmp);
			tmp->type = IS_STRING;
			_php3_hash_update(&EG(symbol_table), t, strlen(t)+1, &tmp, sizeof(pval *), NULL);
		}
		/* insert special variables */
		if (_php3_hash_find(&EG(symbol_table), "SCRIPT_FILENAME", sizeof("SCRIPT_FILENAME"), (void **) &tmp_ptr) == SUCCESS) {
			(*tmp_ptr)->refcount++;
			_php3_hash_update(&EG(symbol_table), "PATH_TRANSLATED", sizeof("PATH_TRANSLATED"), tmp_ptr, sizeof(pval *), NULL);
		}
		tmp = (pval *) emalloc(sizeof(pval));
		tmp->value.str.len = strlen(((request_rec *) SG(server_context))->uri);
		tmp->value.str.val = estrndup(((request_rec *) SG(server_context))->uri, tmp->value.str.len);
		INIT_PZVAL(tmp);
		tmp->type = IS_STRING;
		_php3_hash_update(&EG(symbol_table), "PHP_SELF", sizeof("PHP_SELF"), (void *) &tmp, sizeof(pval *), NULL);
	}
#else
	{
		/* Build the special-case PHP_SELF variable for the CGI version */
		char *pi;
#if FORCE_CGI_REDIRECT
		pi = SG(request_info).request_uri;
		tmp = (pval *) emalloc(sizeof(pval));
		tmp->value.str.val = emalloc(((pi)?strlen(pi):0) + 1);
		tmp->value.str.len = _php3_sprintf(tmp->value.str.val, "%s", (pi ? pi : ""));	/* SAFE */
		tmp->type = IS_STRING;
		INIT_PZVAL(tmp);
#else
		int l = 0;
		char *sn;
		sn = request_info.script_name;
		pi = SG(request_info).request_uri;
		if (sn)
			l += strlen(sn);
		if (pi)
			l += strlen(pi);
		if (pi && sn && !strcmp(pi, sn)) {
			l -= strlen(pi);
			pi = NULL;
		}
		tmp = (pval *) emalloc(sizeof(pval));
		tmp->value.str.val = emalloc(l + 1);
		tmp->value.str.len = _php3_sprintf(tmp->value.str.val, "%s%s", (sn ? sn : ""), (pi ? pi : ""));	/* SAFE */
		tmp->type = IS_STRING;
		INIT_PZVAL(tmp);
#endif
		_php3_hash_update(&EG(symbol_table), "PHP_SELF", sizeof("PHP_SELF"), (void *) & tmp, sizeof(pval *), NULL);
	}
#endif


	/* need argc/argv support as well */
	_php3_build_argv(SG(request_info).query_string ELS_CC);

	return SUCCESS;
}

void _php3_build_argv(char *s ELS_DC)
{
	pval *arr, *tmp;
	int count = 0;
	char *ss, *space;

	arr = (pval *) emalloc(sizeof(pval));
	arr->value.ht = (HashTable *) emalloc(sizeof(HashTable));
	if (_php3_hash_init(arr->value.ht, 0, NULL, PVAL_PTR_DTOR, 0) == FAILURE) {
		php3_error(E_WARNING, "Unable to create argv array");
	} else {
		arr->type = IS_ARRAY;
		INIT_PZVAL(arr);
		_php3_hash_update(&EG(symbol_table), "argv", sizeof("argv"), &arr, sizeof(pval *), NULL);
	}
	/* now pick out individual entries */
	ss = s;
	while (ss) {
		space = strchr(ss, '+');
		if (space) {
			*space = '\0';
		}
		/* auto-type */
		tmp = (pval *) emalloc(sizeof(pval));
		tmp->type = IS_STRING;
		tmp->value.str.len = strlen(ss);
		tmp->value.str.val = estrndup(ss, tmp->value.str.len);
		INIT_PZVAL(tmp);
		count++;
		if (_php3_hash_next_index_insert(arr->value.ht, &tmp, sizeof(pval *), NULL)==FAILURE) {
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
	tmp = (pval *) emalloc(sizeof(pval));
	tmp->value.lval = count;
	tmp->type = IS_LONG;
	INIT_PZVAL(tmp);
	_php3_hash_add(&EG(symbol_table), "argc", sizeof("argc"), &tmp, sizeof(pval *), NULL);
}


#include "logos.h"

PHPAPI void php_execute_script(zend_file_handle *primary_file CLS_DC ELS_DC PLS_DC)
{
	zend_file_handle *prepend_file_p, *append_file_p;
	zend_file_handle prepend_file, append_file;
	SLS_FETCH();

	if (SG(request_info).query_string && SG(request_info).query_string[0]=='=') {
		if (!strcmp(SG(request_info).query_string+1, "PHPE9568F34-D428-11d2-A769-00AA001ACF42")) {
			char *header_line = estrndup(CONTEXT_TYPE_IMAGE_GIF, sizeof(CONTEXT_TYPE_IMAGE_GIF));

			php4i_add_header_information(header_line, sizeof(CONTEXT_TYPE_IMAGE_GIF)-1);
			PHPWRITE(php4_logo, sizeof(php4_logo));
			return;
		} else if (!strcmp(SG(request_info).query_string+1, "PHPE9568F35-D428-11d2-A769-00AA001ACF42")) {
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
	_php3_hash_environment(PLS_C ELS_CC);

#if WIN32||WINNT
	UpdateIniFromRegistry(primary_file->filename);
#endif

	if (PG(auto_prepend_file) && PG(auto_prepend_file)[0]) {
		prepend_file.filename = PG(auto_prepend_file);
		prepend_file.type = ZEND_HANDLE_FILENAME;
		prepend_file_p = &prepend_file;
	} else {
		prepend_file_p = NULL;
	}
	if (PG(auto_append_file) && PG(auto_append_file)[0]) {
		append_file.filename = PG(auto_append_file);
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



/* some systems are missing these from their header files */

#if APACHE
PHPAPI int apache_php_module_main(request_rec *r, int fd, int display_source_mode SLS_DC)
{
	zend_file_handle file_handle;
#ifdef ZTS
	zend_compiler_globals cg;
	zend_executor_globals eg;
	php_core_globals pcg;
	zend_compiler_globals *compiler_globals=&cg;
	zend_executor_globals *executor_globals=&eg;
	php_core_globals *core_globals=&pcg;
#endif
	SLS_FETCH();

	if (php_request_startup(CLS_C ELS_CC PLS_CC SLS_CC) == FAILURE) {
		return FAILURE;
	}
	php3_TreatHeaders();
	file_handle.type = ZEND_HANDLE_FD;
	file_handle.handle.fd = fd;
	file_handle.filename = SG(request_info).path_translated;

	if (display_source_mode) {
		zend_syntax_highlighter_ini syntax_highlighter_ini;

		if (open_file_for_scanning(&file_handle CLS_CC)==SUCCESS) {
			php_get_highlight_struct(&syntax_highlighter_ini);
			zend_highlight(&syntax_highlighter_ini);
			fclose(file_handle.handle.fp);
			return OK;
		} else {
			return NOT_FOUND;
		}
	} else {
		(void) php_execute_script(&file_handle CLS_CC ELS_CC);
	}
	
	php3_header();			/* Make sure headers have been sent */
	zend_end_ob_buffering(1);
	return (OK);
}
#endif							/* APACHE */


#if WIN32||WINNT
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
