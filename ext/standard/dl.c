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
   | Authors: Brian Schaffner <brian@tool.net>                            |
   |          Shane Caraveo <shane@caraveo.com>                           |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
 */

#include "php.h"
#include "dl.h"
#include "php_globals.h"

#if HAVE_LIBDL
#include <stdlib.h>
#include <stdio.h>
#ifndef RTLD_LAZY
# define RTLD_LAZY 1    /* Solaris 1, FreeBSD's (2.1.7.1 and older) */
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#if MSVC5
#include "win32/param.h"
#include "win32/winutil.h"
#else
#include <sys/param.h>
#endif

function_entry dl_functions[] = {
	{"dl",		dl,		NULL},
	{NULL, NULL, NULL}
};


php3_module_entry dl_module_entry = {
	"PHP_DL", dl_functions, NULL, NULL, NULL, NULL, php3_info_dl, STANDARD_MODULE_PROPERTIES
};

#endif

/* {{{ proto int dl(string extension_filename)
   Load a PHP extension at runtime */
void dl(INTERNAL_FUNCTION_PARAMETERS)
{
	pval *file;
	PLS_FETCH();

	/* obtain arguments */
	if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &file) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string(file);

	if (!PG(enable_dl)) {
		php3_error(E_ERROR, "Dynamically loaded extentions aren't enabled.");
	} else if (PG(safe_mode)) {
		php3_error(E_ERROR, "Dynamically loaded extensions aren't allowed when running in SAFE MODE.");
	} else {
		php3_dl(file,MODULE_TEMPORARY,return_value);
	}
}
/* }}} */


#if HAVE_LIBDL

void php3_dl(pval *file,int type,pval *return_value)
{
	void *handle;
	char libpath[MAXPATHLEN + 1];
	php3_module_entry *module_entry,*tmp;
	php3_module_entry *(*get_module)(void);
	PLS_FETCH();
	
	if (cfg_get_string("extension_dir",&PG(extension_dir))==SUCCESS && PG(extension_dir)){
		int extension_dir_len = strlen(PG(extension_dir));

		if (PG(extension_dir)[extension_dir_len-1]=='/' || PG(extension_dir)[extension_dir_len-1]=='\\') {
			sprintf(libpath,"%s%s",PG(extension_dir),file->value.str.val);  /* SAFE */
		} else {
			sprintf(libpath,"%s/%s",PG(extension_dir),file->value.str.val);  /* SAFE */
		}
	} else {
		sprintf(libpath,"%s",file->value.str.val);  /* SAFE */
	}

	/* load dynamic symbol */
	handle = dlopen(libpath, RTLD_LAZY);
	if (!handle) {
#if MSVC5
		php3_error(E_ERROR,"Unable to load dynamic library '%s'<br>\n%s",libpath,php3_win_err());
#else
		printf("dlerror = %s\n", dlerror());
		php3_error(E_ERROR,"Unable to load dynamic library '%s' - %s",libpath,dlerror());
#endif
		RETURN_FALSE;
	}
	get_module = (php3_module_entry *(*)(void)) dlsym(handle, "get_module");
	
	if (!get_module) {
		dlclose(handle);
		php3_error(E_CORE_WARNING,"Invalid library (maybe not a PHP3 library) '%s' ",file->value.str.val);
		RETURN_FALSE;
	}
	module_entry = get_module();
	module_entry->type = type;
	module_entry->module_number = zend_next_free_module();
	if (module_entry->module_startup_func) {
		if (module_entry->module_startup_func(type, module_entry->module_number)==FAILURE) {
			php3_error(E_CORE_WARNING,"%s:  Unable to initialize module",module_entry->name);
			dlclose(handle);
			RETURN_FALSE;
		}
	}
	zend_register_module(module_entry);


	if (module_entry->request_startup_func) {
		if (module_entry->request_startup_func(type, module_entry->module_number)) {
			php3_error(E_CORE_WARNING,"%s:  Unable to initialize module",module_entry->name);
			dlclose(handle);
			RETURN_FALSE;
		}
	}
	
	/* update the .request_started property... */
	if (_php3_hash_find(&module_registry,module_entry->name,strlen(module_entry->name)+1,(void **) &tmp)==FAILURE) {
		php3_error(E_ERROR,"%s:  Loaded module got lost",module_entry->name);
		RETURN_FALSE;
	}
	tmp->request_started=1;
	tmp->handle = handle;
	
	RETURN_TRUE;
}


void php3_info_dl(ZEND_MODULE_INFO_FUNC_ARGS)
{
	PUTS("Dynamic Library support enabled.\n");
}

#else

void php3_dl(pval *file,int type,pval *return_value)
{
	php3_error(E_WARNING,"Cannot dynamically load %s - dynamic modules are not supported",file->value.str.val);
	RETURN_FALSE;
}

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
