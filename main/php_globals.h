/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


#ifndef _PHP_GLOBALS_H
#define _PHP_GLOBALS_H

#include "zend_globals.h"

typedef struct _php_core_globals php_core_globals;

#ifdef ZTS
# define PLS_D	php_core_globals *core_globals
# define PLS_DC	, PLS_D
# define PLS_C	core_globals
# define PLS_CC , PLS_C
# define PG(v) (core_globals->v)
# define PLS_FETCH()	php_core_globals *core_globals = ts_resource(core_globals_id)
extern PHPAPI int core_globals_id;
#else
# define PLS_D
# define PLS_DC
# define PLS_C
# define PLS_CC
# define PG(v) (core_globals.v)
# define PLS_FETCH()
extern ZEND_API struct _php_core_globals core_globals;
#endif

typedef struct _php_http_globals {
	zval *post;
	zval *get;
	zval *cookie;
	zval *server;
	zval *environment;
	zval *post_files;
} php_http_globals;

struct _php_tick_function_entry;

struct _php_core_globals {
	zend_bool magic_quotes_gpc;
	zend_bool magic_quotes_runtime;
	zend_bool magic_quotes_sybase;

	zend_bool asp_tags;
	zend_bool short_tags;
	zend_bool allow_call_time_pass_reference;
	zend_bool zend_set_utility_values;
	zend_bool output_buffering;
	zend_bool implicit_flush;

	zend_bool safe_mode;
	zend_bool sql_safe_mode;
	char *safe_mode_exec_dir;
	zend_bool enable_dl;

	long memory_limit;

	int error_reporting;
	zend_bool track_errors;
	zend_bool display_errors;
	zend_bool log_errors;
	char *error_log;

	char *doc_root;
	char *user_dir;
	char *include_path;
	char *open_basedir;
	char *extension_dir;

	char *upload_tmp_dir;
	long upload_max_filesize;
	
	char *error_append_string;
	char *error_prepend_string;

	char *auto_prepend_file;
	char *auto_append_file;

	char *arg_separator;
	char *gpc_order;
	char *variables_order;

	zend_bool expose_php;

	zend_bool track_vars;
	zend_bool register_globals;
	zend_bool register_argc_argv;

	zend_bool y2k_compliance;

	short connection_status;
	short ignore_user_abort;

	long max_execution_time;

	unsigned char header_is_being_sent;

	zend_llist tick_functions;

	php_http_globals http_globals;
};


#endif /* _PHP_GLOBALS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
