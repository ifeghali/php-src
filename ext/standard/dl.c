/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2007 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Brian Schaffner <brian@tool.net>                            |
   |          Shane Caraveo <shane@caraveo.com>                           |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php.h"
#include "dl.h"
#include "php_globals.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/file.h"

#include "SAPI.h"

#if defined(HAVE_LIBDL) || HAVE_MACH_O_DYLD_H
#include <stdlib.h>
#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef PHP_WIN32
#include "win32/param.h"
#include "win32/winutil.h"
#define GET_DL_ERROR()	php_win_err()
#elif defined(NETWARE)
#include <sys/param.h>
#define GET_DL_ERROR()	dlerror()
#else
#include <sys/param.h>
#define GET_DL_ERROR()	DL_ERROR()
#endif

#endif /* defined(HAVE_LIBDL) || HAVE_MACH_O_DYLD_H */


/* {{{ proto int dl(string extension_filename) U
   Load a PHP extension at runtime */
PHP_FUNCTION(dl)
{
	zval *filename;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &filename) == FAILURE) {
		return;
	}

	if (Z_STRLEN_P(filename) >= MAXPATHLEN) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "File name exceeds the maximum allowed length of %d characters", MAXPATHLEN);
		RETURN_FALSE;
	}

	php_dl(filename, MODULE_TEMPORARY, return_value, 0 TSRMLS_CC);
	EG(full_tables_cleanup) = 1;
}

/* }}} */


#if defined(HAVE_LIBDL) || HAVE_MACH_O_DYLD_H

#ifdef ZTS
#define USING_ZTS 1
#else
#define USING_ZTS 0
#endif

/* {{{ php_dl
 */
void php_dl(zval *file, int type, zval *return_value, int start_now TSRMLS_DC)
{
	void *handle;
	char *libpath;
	zend_module_entry *module_entry;
	zend_module_entry *(*get_module)(void);
	int error_type;
	char *extension_dir;
	char *filename;
	int filename_len;

	if (type == MODULE_PERSISTENT) {
		extension_dir = INI_STR("extension_dir");
	} else {
		extension_dir = PG(extension_dir);
	}

	if (type == MODULE_TEMPORARY) {
		error_type = E_WARNING;
	} else {
		error_type = E_CORE_WARNING;
	}

	if (Z_TYPE_P(file) == IS_UNICODE) {
		if (FAILURE == php_stream_path_encode(NULL, &filename, &filename_len, Z_USTRVAL_P(file), Z_USTRLEN_P(file), REPORT_ERRORS, FG(default_context))) {
			return;
		}
	} else {
		filename = Z_STRVAL_P(file);
		filename_len = Z_STRLEN_P(file);
	}

	if (extension_dir && extension_dir[0]){
		int extension_dir_len = strlen(extension_dir);

		if (type == MODULE_TEMPORARY) {
			if (strchr(filename, '/') != NULL || strchr(filename, DEFAULT_SLASH) != NULL) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Temporary module name should contain only filename");
				RETURN_FALSE;
			}
		}

		if (IS_SLASH(extension_dir[extension_dir_len-1])) {
			spprintf(&libpath, 0, "%s%s", extension_dir, filename); /* SAFE */
		} else {
			spprintf(&libpath, 0, "%s%c%s", extension_dir, DEFAULT_SLASH, filename); /* SAFE */
		}
	} else {
		libpath = estrndup(filename, filename_len);
	}

	if (Z_TYPE_P(file) == IS_UNICODE) {
		efree(filename);
	}

	/* load dynamic symbol */
	handle = DL_LOAD(libpath);
	if (!handle) {
		php_error_docref(NULL TSRMLS_CC, error_type, "Unable to load dynamic library '%s' - %s", libpath, GET_DL_ERROR());
		GET_DL_ERROR(); /* free the buffer storing the error */
		efree(libpath);
		RETURN_FALSE;
	}

	efree(libpath);

	get_module = (zend_module_entry *(*)(void)) DL_FETCH_SYMBOL(handle, "get_module");

	/*
	 * some OS prepend _ to symbol names while their dynamic linker
	 * does not do that automatically. Thus we check manually for
	 * _get_module.
	 */

	if (!get_module)
		get_module = (zend_module_entry *(*)(void)) DL_FETCH_SYMBOL(handle, "_get_module");

	if (!get_module) {
		DL_UNLOAD(handle);
		php_error_docref(NULL TSRMLS_CC, error_type, "Invalid library (maybe not a PHP library) '%R'", Z_TYPE_P(file), Z_UNIVAL_P(file));
		RETURN_FALSE;
	}
	module_entry = get_module();
	if ((module_entry->zend_debug != ZEND_DEBUG) || (module_entry->zts != USING_ZTS)
		|| (module_entry->zend_api != ZEND_MODULE_API_NO)) {
		/* Check for pre-4.1.0 module which has a slightly different module_entry structure :( */
			struct pre_4_1_0_module_entry {
				  char *name;
				  zend_function_entry *functions;
				  int (*module_startup_func)(INIT_FUNC_ARGS);
				  int (*module_shutdown_func)(SHUTDOWN_FUNC_ARGS);
				  int (*request_startup_func)(INIT_FUNC_ARGS);
				  int (*request_shutdown_func)(SHUTDOWN_FUNC_ARGS);
				  void (*info_func)(ZEND_MODULE_INFO_FUNC_ARGS);
				  int (*global_startup_func)(void);
				  int (*global_shutdown_func)(void);
				  int globals_id;
				  int module_started;
				  unsigned char type;
				  void *handle;
				  int module_number;
				  unsigned char zend_debug;
				  unsigned char zts;
				  unsigned int zend_api;
			};

			char *name;
			int zend_api;
			unsigned char zend_debug, zts;

			if ((((struct pre_4_1_0_module_entry *)module_entry)->zend_api > 20000000) &&
				(((struct pre_4_1_0_module_entry *)module_entry)->zend_api < 20010901)
			) {
				name       = ((struct pre_4_1_0_module_entry *)module_entry)->name;
				zend_api   = ((struct pre_4_1_0_module_entry *)module_entry)->zend_api;
				zend_debug = ((struct pre_4_1_0_module_entry *)module_entry)->zend_debug;
				zts        = ((struct pre_4_1_0_module_entry *)module_entry)->zts; 
			} else {			
				name       = module_entry->name;
				zend_api   = module_entry->zend_api;
				zend_debug = module_entry->zend_debug;
				zts        = module_entry->zts; 
			}

			php_error_docref(NULL TSRMLS_CC, error_type,
					  "%s: Unable to initialize module\n"
					  "Module compiled with module API=%d, debug=%d, thread-safety=%d\n"
					  "PHP    compiled with module API=%d, debug=%d, thread-safety=%d\n"
					  "These options need to match\n",
					  name, zend_api, zend_debug, zts,
					  ZEND_MODULE_API_NO, ZEND_DEBUG, USING_ZTS);
			DL_UNLOAD(handle);
			RETURN_FALSE;
	}
	module_entry->type = type;
	module_entry->module_number = zend_next_free_module();
	module_entry->handle = handle;

	if ((module_entry = zend_register_module_ex(module_entry TSRMLS_CC)) == NULL) {
		DL_UNLOAD(handle);
		RETURN_FALSE;
	}

	if ((type == MODULE_TEMPORARY || start_now) && zend_startup_module_ex(module_entry TSRMLS_CC) == FAILURE) {
		DL_UNLOAD(handle);
		RETURN_FALSE;
	}

	if ((type == MODULE_TEMPORARY || start_now) && module_entry->request_startup_func) {
		if (module_entry->request_startup_func(type, module_entry->module_number TSRMLS_CC) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, error_type, "Unable to initialize module '%s'", module_entry->name);
			DL_UNLOAD(handle);
			RETURN_FALSE;
		}
	}
	RETURN_TRUE;
}
/* }}} */

PHP_MINFO_FUNCTION(dl)
{
	php_info_print_table_row(2, "Dynamic Library Support", "enabled");
}

#else

void php_dl(zval *file, int type, zval *return_value, int start_now TSRMLS_DC)
{
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot dynamically load %R - dynamic modules are not supported", Z_TYPE_P(file), Z_UNIVAL_P(file));
	RETURN_FALSE;
}

PHP_MINFO_FUNCTION(dl)
{
	PUTS("Dynamic Library support not available<br />.\n");
}

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
