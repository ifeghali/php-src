e/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_01.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors:                                                             |
   | PHP 4.0 patches by Thies C. Arntzen (thies@digicol.de)               |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

/* {{{ includes/startup/misc */

#include "php.h"
#include "fopen-wrappers.h"

#include "php_dir.h"

#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>

#ifdef PHP_WIN32
#define NEEDRDH 1
#include "win32/readdir.h"
#endif

typedef struct {
	int default_dir;
} php_dir_globals;

#ifdef ZTS
#define DIR(v) (dir_globals->v)
#define DIRLS_FETCH() php_dir_globals *dir_globals = ts_resource(dir_globals_id)
int dir_globals_id;
#else
#define DIR(v) (dir_globals.v)
#define DIRLS_FETCH()
php_dir_globals dir_globals;
#endif

typedef struct {
	int id;
	DIR *dir;
} php_dir;

static int le_dirp;

static zend_class_entry *dir_class_entry_ptr;

#define FETCH_DIRP() \
	if (ARG_COUNT(ht) == 0) { \
		myself = getThis(); \
		if (myself) { \
			if (zend_hash_find(myself->value.obj.properties, "handle", sizeof("handle"), (void **)&tmp) == FAILURE) { \
				php_error(E_WARNING, "unable to find my handle property"); \
				RETURN_FALSE; \
			} \
			ZEND_FETCH_RESOURCE(dirp,php_dir *,tmp,-1, "Directory", le_dirp); \
		} else { \
			ZEND_FETCH_RESOURCE(dirp,php_dir *,0,DIR(default_dir), "Directory", le_dirp); \
		} \
	} else if ((ARG_COUNT(ht) != 1) || zend_get_parameters_ex(1, &id) == FAILURE) { \
		WRONG_PARAM_COUNT; \
	} else { \
		ZEND_FETCH_RESOURCE(dirp,php_dir *,id,-1, "Directory", le_dirp); \
	} 

static zend_function_entry php_dir_functions[] = {
	PHP_FE(opendir,				NULL)
	PHP_FE(closedir,			NULL)
	PHP_FE(chdir,				NULL)
	PHP_FE(getcwd,				NULL)
	PHP_FE(rewinddir,			NULL)
	PHP_FE(readdir,				NULL)
	PHP_FALIAS(dir,		getdir,	NULL)
	{NULL, NULL, NULL}
};


static zend_function_entry php_dir_class_functions[] = {
	PHP_FALIAS(close,	closedir,	NULL)
	PHP_FALIAS(rewind,	rewinddir,	NULL)
	PHP_FALIAS(read,	readdir,	NULL)
	{NULL, NULL, NULL}
};


zend_module_entry php_dir_module_entry = {
	"PHP_dir", php_dir_functions, PHP_MINIT(dir), NULL, NULL, NULL, NULL, STANDARD_MODULE_PROPERTIES
};

static void _dir_dtor(php_dir *dirp)
{
	closedir(dirp->dir);
	efree(dirp);
}

#ifdef ZTS
static void php_dir_init_globals(php_dir_globals *dir_globals)
{
	DIR(default_dir) = 0;
}
#endif

PHP_MINIT_FUNCTION(dir)
{
	zend_class_entry dir_class_entry;

	le_dirp = register_list_destructors(_dir_dtor,NULL);

	INIT_CLASS_ENTRY(dir_class_entry, "Directory", php_dir_class_functions);
	dir_class_entry_ptr = register_internal_class(&dir_class_entry);

#ifdef ZTS
	dir_globals_id = ts_allocate_id(sizeof(php_dir_globals), (ts_allocate_ctor) php_dir_init_globals, NULL);
#else
	DIR(default_dir) = 0;
#endif

	return SUCCESS;
}

/* }}} */
/* {{{ internal functions */

static void _php_do_opendir(INTERNAL_FUNCTION_PARAMETERS, int createobject)
{
	pval **arg;
	php_dir *dirp;
	DIRLS_FETCH();
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);

	if (php_check_open_basedir((*arg)->value.str.val)) {
		RETURN_FALSE;
	}
	
	dirp = emalloc(sizeof(php_dir));

	dirp->dir = opendir((*arg)->value.str.val);
	
	if (! dirp->dir) {
		efree(dirp);
		php_error(E_WARNING, "OpenDir: %s (errno %d)", strerror(errno), errno);
		RETURN_FALSE;
	}

	dirp->id = zend_list_insert(dirp,le_dirp);

	DIR(default_dir) = dirp->id;

	if (createobject) {
		object_init_ex(return_value, dir_class_entry_ptr);
		add_property_stringl(return_value, "path", (*arg)->value.str.val, (*arg)->value.str.len, 1);
		add_property_resource(return_value, "handle", dirp->id);
		zend_list_addref(dirp->id);
	} else {
		RETURN_RESOURCE(dirp->id);
	}
}

/* }}} */
/* {{{ proto int opendir(string path)
   Open a directory and return a dir_handle */

PHP_FUNCTION(opendir)
{
	_php_do_opendir(INTERNAL_FUNCTION_PARAM_PASSTHRU,0);
}

/* }}} */
/* {{{ proto class dir(string directory)
   Directory class with properties, handle and class and methods read, rewind and close */

PHP_FUNCTION(getdir)
{
	_php_do_opendir(INTERNAL_FUNCTION_PARAM_PASSTHRU,1);
}

/* }}} */
/* {{{ proto void closedir([int dir_handle])
   Close directory connection identified by the dir_handle */

PHP_FUNCTION(closedir)
{
	pval **id, **tmp, *myself;
	php_dir *dirp;
	DIRLS_FETCH();

	FETCH_DIRP();

	zend_list_delete(dirp->id);
}

/* }}} */
/* {{{ proto int chdir(string directory)
   Change the current directory */

PHP_FUNCTION(chdir)
{
	pval **arg;
	int ret;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);

	ret = chdir((*arg)->value.str.val);
	
	if (ret < 0) {
		php_error(E_WARNING, "ChDir: %s (errno %d)", strerror(errno), errno);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}

/* }}} */
/* {{{ proto string getcwd(void)
   Gets the current directory */

PHP_FUNCTION(getcwd)
{
	char path[MAXPATHLEN];
	char *ret=NULL;
	
	if (ARG_COUNT(ht) != 0) {
		WRONG_PARAM_COUNT;
	}

#if HAVE_GETCWD
	ret = getcwd(path,MAXPATHLEN-1);
#elif HAVE_GETWD
	ret = getwd(path);
/*
 * #warning is not ANSI C
 * #else
 * #warning no proper getcwd support for your site
 */
#endif

	if (ret) {
		RETURN_STRING(path,1);
	} else {
		RETURN_FALSE;
	}
}

/* }}} */
/* {{{ proto void rewinddir([int dir_handle])
   Rewind dir_handle back to the start */

PHP_FUNCTION(rewinddir)
{
	pval **id, **tmp, *myself;
	php_dir *dirp;
	DIRLS_FETCH();
	
	FETCH_DIRP();

	rewinddir(dirp->dir);
}
/* }}} */
/* {{{ proto string readdir([int dir_handle])
   Read directory entry from dir_handle */

PHP_FUNCTION(readdir)
{
	pval **id, **tmp, *myself;
	php_dir *dirp;
	struct dirent *direntp;
	DIRLS_FETCH();

	FETCH_DIRP();
	
	direntp = readdir(dirp->dir);
	if (direntp) {
		RETURN_STRINGL(direntp->d_name, strlen(direntp->d_name), 1);
	}
	RETURN_FALSE;
}

/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
