/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Sterling Hughes <sterling@php.net>                          |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "php.h"
#include "php_ini.h"
#include "php_zip.h"

#if HAVE_ZZIPLIB

#include "ext/standard/info.h"
#include <zziplib.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

static int le_zip_dir;
#define le_zip_dir_name "Zip Directory"
static int le_zip_entry;
#define le_zip_entry_name "Zip Entry"

/* {{{ zip_functions[]
 */
function_entry zip_functions[] = {
	PHP_FE(zip_open,                    NULL)
	PHP_FE(zip_read,                    NULL)
	PHP_FE(zip_close,                   NULL)
	PHP_FE(zip_entry_name,              NULL)
	PHP_FE(zip_entry_compressedsize,    NULL)
	PHP_FE(zip_entry_filesize,          NULL)
	PHP_FE(zip_entry_compressionmethod, NULL)
	PHP_FE(zip_entry_open,              NULL)
	PHP_FE(zip_entry_read,              NULL)
	PHP_FE(zip_entry_close,             NULL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ zip_module_entry
 */
zend_module_entry zip_module_entry = {
	"zip",
	zip_functions,
	PHP_MINIT(zip),
	NULL,
	NULL,		
	NULL,
	PHP_MINFO(zip),
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_ZIP
ZEND_GET_MODULE(zip)
#endif

/* {{{ php_zip_free_dir
 */
static void php_zip_free_dir(zend_rsrc_list_entry *rsrc)
{
	ZZIP_DIR *z_dir = (ZZIP_DIR *) rsrc->ptr;
	zzip_closedir(z_dir);
}
/* }}} */

/* {{{ php_zip_free_entry
 */
static void php_zip_free_entry(zend_rsrc_list_entry *rsrc)
{
	php_zzip_dirent *entry = (php_zzip_dirent *) rsrc->ptr;

	if (entry->fp) {
		zzip_close(entry->fp);
	}

	efree(entry);
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(zip)
{
	le_zip_dir   = zend_register_list_destructors_ex(php_zip_free_dir, NULL, le_zip_dir_name, module_number);
	le_zip_entry = zend_register_list_destructors_ex(php_zip_free_entry, NULL, le_zip_entry_name, module_number);

	return(SUCCESS);
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(zip)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Zip support", "enabled");
	php_info_print_table_end();

}
/* }}} */

/* {{{ proto resource zip_opendir(string filename)
   Open a new zip archive for reading */
PHP_FUNCTION(zip_open)
{
	zval **filename;
	ZZIP_DIR *archive_p = NULL;

	if (ZEND_NUM_ARGS() != 1 ||
		zend_get_parameters_ex(1, &filename) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	archive_p = zzip_opendir(Z_STRVAL_PP(filename));
	if (archive_p == NULL) {
		php_error(E_WARNING, "Cannot open zip archive %s", Z_STRVAL_PP(filename));
		RETURN_FALSE;
	}

	ZEND_REGISTER_RESOURCE(return_value, archive_p, le_zip_dir);
}
/* }}} */

/* {{{ proto resource zip_readdir(resource zip)
   Returns the next file in the archive */
PHP_FUNCTION(zip_read)
{
	zval **zzip_dp;
	ZZIP_DIR *archive_p = NULL;
	php_zzip_dirent *entry = NULL;
	int ret;

	if (ZEND_NUM_ARGS() != 1 ||
		zend_get_parameters_ex(1, &zzip_dp) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(archive_p, ZZIP_DIR *, zzip_dp, -1, le_zip_dir_name, le_zip_dir);

	entry = emalloc(sizeof(php_zzip_dirent));

	ret = zzip_dir_read(archive_p, &entry->dirent);
	if (ret == 0) {
		efree(entry);
		RETURN_FALSE;
	}

	entry->fp = NULL;

	ZEND_REGISTER_RESOURCE(return_value, entry, le_zip_entry);
}
/* }}} */

/* {{{ proto void zip_closedir(resource zip)
   Close a Zip archive */
PHP_FUNCTION(zip_close)
{
	zval **zzip_dp;
	ZZIP_DIR *archive_p = NULL;

	if (ZEND_NUM_ARGS() != 1 ||
		zend_get_parameters_ex(1, &zzip_dp) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(archive_p, ZZIP_DIR *, zzip_dp, -1, le_zip_dir_name, le_zip_dir);

	zend_list_delete(Z_LVAL_PP(zzip_dp));
}
/* }}} */

/* {{{ php_zzip_get_entry
 */
static void php_zzip_get_entry(INTERNAL_FUNCTION_PARAMETERS, int opt)
{
	zval **zzip_ent;
	php_zzip_dirent *entry = NULL;

	if (ZEND_NUM_ARGS() != 1 ||
		zend_get_parameters_ex(1, &zzip_ent) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(entry, php_zzip_dirent *, zzip_ent, -1, le_zip_entry_name, le_zip_entry);

	switch (opt) {
	case 0:
		RETURN_STRING(entry->dirent.d_name, 1);
		break;
	case 1:
		RETURN_LONG(entry->dirent.d_csize);
		break;
	case 2:
		RETURN_LONG(entry->dirent.st_size);
		break;
	case 3:
		RETURN_STRING((char *) zzip_compr_str(entry->dirent.d_compr), 1);
		break;
	}
}
/* }}} */

/* {{{ proto string zip_entry_name(resource zip_entry)
   Return the name given a ZZip entry */
PHP_FUNCTION(zip_entry_name)
{
	php_zzip_get_entry(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto int zip_entry_compressedsize(resource zip_entry)
   Return the compressed size of a ZZip entry */
PHP_FUNCTION(zip_entry_compressedsize)
{
	php_zzip_get_entry(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto int zip_entry_filesize(resource zip_entry)
   Return the actual filesize of a ZZip entry */
PHP_FUNCTION(zip_entry_filesize)
{
	php_zzip_get_entry(INTERNAL_FUNCTION_PARAM_PASSTHRU, 2);
}
/* }}} */

/* {{{ proto string zip_entry_compressionmethod(resource zip_entry)
   Return a string containing the compression method used on a particular entry */
PHP_FUNCTION(zip_entry_compressionmethod)
{
	php_zzip_get_entry(INTERNAL_FUNCTION_PARAM_PASSTHRU, 3);
}
/* }}} */

/* {{{ proto bool zip_entry_open(resource zip_dp, resource zip_entry, string mode)
   Open a Zip File, pointed by the resource entry */
PHP_FUNCTION(zip_entry_open)
{
	zval **zzip_dp, **zzip_ent, **mode;
	ZZIP_DIR *archive_p = NULL;
	php_zzip_dirent *entry = NULL;

	if (ZEND_NUM_ARGS() < 2 || ZEND_NUM_ARGS() > 3 || 
		zend_get_parameters_ex(ZEND_NUM_ARGS(), &zzip_dp, &zzip_ent, &mode) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(archive_p, ZZIP_DIR *, zzip_dp, -1, le_zip_dir_name, le_zip_dir);
	ZEND_FETCH_RESOURCE(entry, php_zzip_dirent *, zzip_ent, -1, le_zip_entry_name, le_zip_entry);

	entry->fp = zzip_file_open(archive_p, entry->dirent.d_name, O_RDONLY | O_BINARY);

	if (entry->fp) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string zip_read(resource zip_ent)
   Read X bytes from an opened zip entry */
PHP_FUNCTION(zip_entry_read)
{
	zval **zzip_ent, **length;
	php_zzip_dirent *entry = NULL;
	char *buf = NULL;
	int len = 1024,
		argc = ZEND_NUM_ARGS(),
		ret = 0;

	if (argc < 1 || argc > 2 ||
		zend_get_parameters_ex(argc, &zzip_ent, &length) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(entry, php_zzip_dirent *, zzip_ent, -1, le_zip_entry_name, le_zip_entry);
	if (argc > 1) {
		convert_to_long_ex(length);
		len = Z_LVAL_PP(length);
	}

	buf = emalloc(len + 1);

	ret = zzip_read(entry->fp, buf, len);
	if (ret == 0) {
		RETURN_FALSE;
	}
	
	RETURN_STRINGL(buf, len, 0);
}
/* }}} */

/* {{{ proto void zip_close(resource zip_ent)
   Close a zip entry */
PHP_FUNCTION(zip_entry_close)
{
	zval **zzip_ent;
	php_zzip_dirent *entry = NULL;

	if (ZEND_NUM_ARGS() != 1 ||
		zend_get_parameters_ex(1, &zzip_ent) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(entry, php_zzip_dirent *, zzip_ent, -1, le_zip_entry_name, le_zip_entry);

	zend_list_delete(Z_LVAL_PP(zzip_ent));
}
/* }}} */

#endif	/* HAVE_ZZIPLIB */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 tw=78 fdm=marker
 * vim<600: sw=4 ts=4 tw=78
 */
