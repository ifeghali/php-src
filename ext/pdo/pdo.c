/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2005 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Wez Furlong <wez@php.net>                                    |
  |         Marcus Boerger <helly@php.net>                               |
  |         Sterling Hughes <sterling@php.net>                           |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_pdo.h"
#include "php_pdo_driver.h"
#include "php_pdo_int.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"

#if defined(HAVE_SPL) && ((PHP_MAJOR_VERSION > 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 1))
extern PHPAPI zend_class_entry *spl_ce_RuntimeException;
#endif

ZEND_DECLARE_MODULE_GLOBALS(pdo)

/* True global resources - no need for thread safety here */

/* the registry of PDO drivers */
static HashTable pdo_driver_hash;

/* we use persistent resources for the driver connection stuff */
static int le_ppdo;

int php_pdo_list_entry(void)
{
	return le_ppdo;
}

/* for exceptional circumstances */
zend_class_entry *pdo_exception_ce;

PDO_API zend_class_entry *php_pdo_get_exception(void)
{
	return pdo_exception_ce;
}

zend_class_entry *pdo_dbh_ce, *pdo_dbstmt_ce, *pdo_row_ce;

/* {{{ pdo_functions[] */
function_entry pdo_functions[] = {
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ pdo_module_entry */
zend_module_entry pdo_module_entry = {
	STANDARD_MODULE_HEADER,
	"PDO",
	pdo_functions,
	PHP_MINIT(pdo),
	PHP_MSHUTDOWN(pdo),
	PHP_RINIT(pdo),
	PHP_RSHUTDOWN(pdo),
	PHP_MINFO(pdo),
	"0.3",
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PDO
ZEND_GET_MODULE(pdo)
#endif

/* {{{ PHP_INI */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("pdo.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_pdo_globals, pdo_globals)
PHP_INI_END()
/* }}} */

/* {{{ php_pdo_init_globals */
static void php_pdo_init_globals(zend_pdo_globals *pdo_globals)
{
	pdo_globals->global_value = 0;
}
/* }}} */

PDO_API int php_pdo_register_driver(pdo_driver_t *driver)
{
	if (driver->api_version != PDO_DRIVER_API) {
		zend_error(E_ERROR, "PDO: driver %s requires PDO API version %ld; this is PDO version %d",
			driver->driver_name, driver->api_version, PDO_DRIVER_API);
		return FAILURE;
	}
	if (!zend_hash_exists(&module_registry, "pdo", sizeof("pdo"))) {
		zend_error(E_ERROR, "You MUST load PDO before loading any PDO drivers");
		return FAILURE;	/* NOTREACHED */
	}

	return zend_hash_add(&pdo_driver_hash, (char*)driver->driver_name, driver->driver_name_len,
			(void**)&driver, sizeof(driver), NULL);
}

PDO_API void php_pdo_unregister_driver(pdo_driver_t *driver)
{
	if (!zend_hash_exists(&module_registry, "pdo", sizeof("pdo"))) {
		return;
	}

	zend_hash_del(&pdo_driver_hash, (char*)driver->driver_name, driver->driver_name_len);
}

pdo_driver_t *pdo_find_driver(const char *name, int namelen)
{
	pdo_driver_t **driver = NULL;
	
	zend_hash_find(&pdo_driver_hash, (char*)name, namelen, (void**)&driver);

	return driver ? *driver : NULL;
}

PDO_API int php_pdo_parse_data_source(const char *data_source,
		unsigned long data_source_len, struct pdo_data_src_parser *parsed,
		int nparams)
{
	int i, j;
	int valstart = -1;
	int semi = -1;
	int optstart = 0;
	int nlen;
	int n_matches = 0;

	i = 0;
	while (i < data_source_len) {
		/* looking for NAME= */

		if (data_source[i] == '\0') {
			break;
		}

		if (data_source[i] != '=') {
			++i;
			continue;
		}

		valstart = ++i;

		/* now we're looking for VALUE; or just VALUE<NUL> */
		semi = -1;
		while (i < data_source_len) {
			if (data_source[i] == '\0') {
				semi = i++;
				break;
			}
			if (data_source[i] == ';') {
				semi = i++;
				break;
			}
			++i;
		}

		if (semi == -1) {
			semi = i;
		}

		/* find the entry in the array */
		nlen = valstart - optstart - 1;
		for (j = 0; j < nparams; j++) {
			if (0 == strncmp(data_source + optstart, parsed[j].optname, nlen) && parsed[j].optname[nlen] == '\0') {
				/* got a match */
				if (parsed[j].freeme) {
					efree(parsed[j].optval);
				}
				parsed[j].optval = estrndup(data_source + valstart, semi - valstart);
				parsed[j].freeme = 1;
				++n_matches;
				break;
			}
		}

		while (i < data_source_len && isspace(data_source[i])) {
			i++;
		}

		optstart = i;
	}

	return n_matches;
}

static const char digit_vec[] = "0123456789";
PDO_API char *php_pdo_int64_to_str(pdo_int64_t i64 TSRMLS_DC)
{
	char buffer[65];
	char outbuf[65] = "";
	register char *p;
	long long_val;
	char *dst = outbuf;

	if (i64 < 0) {
		i64 = -i64;
		*dst++ = '-';
	}

	if (i64 == 0) {
		*dst++ = '0';
		*dst++ = '\0';
		return estrdup(outbuf);
	}

	p = &buffer[sizeof(buffer)-1];
	*p = '\0';

	while ((pdo_uint64_t)i64 > (pdo_uint64_t)LONG_MAX) {
		pdo_uint64_t quo = (pdo_uint64_t)i64 / (unsigned int)10;
		unsigned int rem = (unsigned int)(i64 - quo*10U);
		*--p = digit_vec[rem];
		i64 = (pdo_int64_t)quo;
	}
	long_val = (long)i64;
	while (long_val != 0) {
		long quo = long_val / 10;
		*--p = digit_vec[(unsigned int)(long_val - quo * 10)];
		long_val = quo;
	}
	while ((*dst++ = *p++) != 0)
		;
	*dst = '\0';
	return estrdup(outbuf);
}

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(pdo)
{
	zend_class_entry ce;

	ZEND_INIT_MODULE_GLOBALS(pdo, php_pdo_init_globals, NULL);
	REGISTER_INI_ENTRIES();

	if (FAILURE == pdo_sqlstate_init_error_table()) {
		return FAILURE;
	}

	zend_hash_init(&pdo_driver_hash, 0, NULL, NULL, 1);

	le_ppdo = zend_register_list_destructors_ex(NULL, php_pdo_pdbh_dtor,
		"PDO persistent database", module_number);

	REGISTER_LONG_CONSTANT("PDO_PARAM_NULL", (long)PDO_PARAM_NULL,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_PARAM_INT",  (long)PDO_PARAM_INT,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_PARAM_STR",  (long)PDO_PARAM_STR,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_PARAM_LOB",  (long)PDO_PARAM_LOB,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_PARAM_STMT", (long)PDO_PARAM_STMT,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_PARAM_INPUT_OUTPUT", (long)PDO_PARAM_INPUT_OUTPUT,	CONST_CS|CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("PDO_FETCH_LAZY", (long)PDO_FETCH_LAZY,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_ASSOC",(long)PDO_FETCH_ASSOC,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_NUM",  (long)PDO_FETCH_NUM,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_BOTH", (long)PDO_FETCH_BOTH,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_OBJ",  (long)PDO_FETCH_OBJ,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_BOUND",(long)PDO_FETCH_BOUND,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_COLUMN",(long)PDO_FETCH_COLUMN,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_CLASS",(long)PDO_FETCH_CLASS,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_INTO", (long)PDO_FETCH_INTO,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_FUNC", (long)PDO_FETCH_FUNC,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_GROUP",(long)PDO_FETCH_GROUP, CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_UNIQUE",(long)PDO_FETCH_UNIQUE, CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_CLASSTYPE",(long)PDO_FETCH_CLASSTYPE, CONST_CS|CONST_PERSISTENT);
#if PHP_MAJOR_VERSION > 5 || PHP_MINOR_VERSION >= 1
	REGISTER_LONG_CONSTANT("PDO_FETCH_SERIALIZE",(long)PDO_FETCH_SERIALIZE, CONST_CS|CONST_PERSISTENT);
#endif

	REGISTER_LONG_CONSTANT("PDO_ATTR_AUTOCOMMIT",	(long)PDO_ATTR_AUTOCOMMIT,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_PREFETCH",		(long)PDO_ATTR_PREFETCH,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_TIMEOUT", 		(long)PDO_ATTR_TIMEOUT,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_ERRMODE", 		(long)PDO_ATTR_ERRMODE,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_SERVER_VERSION",	(long)PDO_ATTR_SERVER_VERSION,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_CLIENT_VERSION", 	(long)PDO_ATTR_CLIENT_VERSION,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_SERVER_INFO",		(long)PDO_ATTR_SERVER_INFO,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_CONNECTION_STATUS", 	(long)PDO_ATTR_CONNECTION_STATUS,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_CASE",		 	(long)PDO_ATTR_CASE,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_CURSOR_NAME", 	(long)PDO_ATTR_CURSOR_NAME,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_CURSOR",	 	(long)PDO_ATTR_CURSOR,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_ORACLE_NULLS",	(long)PDO_ATTR_ORACLE_NULLS,	CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_PERSISTENT",	(long)PDO_ATTR_PERSISTENT,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ATTR_STATEMENT_CLASS",		(long)PDO_ATTR_STATEMENT_CLASS,			CONST_CS|CONST_PERSISTENT);
	
	REGISTER_LONG_CONSTANT("PDO_ERRMODE_SILENT",	(long)PDO_ERRMODE_SILENT,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ERRMODE_WARNING",	(long)PDO_ERRMODE_WARNING,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ERRMODE_EXCEPTION",	(long)PDO_ERRMODE_EXCEPTION,	CONST_CS|CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("PDO_CASE_NATURAL",	(long)PDO_CASE_NATURAL,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_CASE_LOWER",	(long)PDO_CASE_LOWER,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_CASE_UPPER",	(long)PDO_CASE_UPPER,		CONST_CS|CONST_PERSISTENT);
		
	REGISTER_STRING_CONSTANT("PDO_ERR_NONE",	PDO_ERR_NONE,		CONST_CS|CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("PDO_FETCH_ORI_NEXT", (long)PDO_FETCH_ORI_NEXT, CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_ORI_PRIOR", (long)PDO_FETCH_ORI_PRIOR, CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_ORI_FIRST", (long)PDO_FETCH_ORI_FIRST, CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_ORI_LAST", (long)PDO_FETCH_ORI_LAST, CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_ORI_ABS", (long)PDO_FETCH_ORI_ABS, CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_FETCH_ORI_REL", (long)PDO_FETCH_ORI_REL, CONST_CS|CONST_PERSISTENT);
	
	REGISTER_LONG_CONSTANT("PDO_CURSOR_FWDONLY", (long)PDO_CURSOR_FWDONLY,CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_CURSOR_SCROLL", (long)PDO_CURSOR_SCROLL,CONST_CS|CONST_PERSISTENT);

#if 0
	REGISTER_LONG_CONSTANT("PDO_ERR_CANT_MAP", 			(long)PDO_ERR_CANT_MAP,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ERR_SYNTAX", 			(long)PDO_ERR_SYNTAX,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ERR_CONSTRAINT", 		(long)PDO_ERR_CONSTRAINT,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ERR_NOT_FOUND", 		(long)PDO_ERR_NOT_FOUND,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ERR_ALREADY_EXISTS", 	(long)PDO_ERR_ALREADY_EXISTS,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ERR_NOT_IMPLEMENTED", 	(long)PDO_ERR_NOT_IMPLEMENTED,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ERR_MISMATCH", 			(long)PDO_ERR_MISMATCH,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ERR_TRUNCATED", 		(long)PDO_ERR_TRUNCATED,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ERR_DISCONNECTED", 		(long)PDO_ERR_DISCONNECTED,		CONST_CS|CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PDO_ERR_NO_PERM",			(long)PDO_ERR_NO_PERM,			CONST_CS|CONST_PERSISTENT);
#endif

	INIT_CLASS_ENTRY(ce, "PDOException", NULL);
#if defined(HAVE_SPL) && ((PHP_MAJOR_VERSION > 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 1))
	pdo_exception_ce = zend_register_internal_class_ex(&ce, spl_ce_RuntimeException, NULL TSRMLS_CC);
#else
	pdo_exception_ce = zend_register_internal_class_ex(&ce, zend_exception_get_default(), NULL TSRMLS_CC);
#endif
	zend_declare_property_null(pdo_exception_ce, "errorInfo", sizeof("errorInfo")-1, ZEND_ACC_PUBLIC TSRMLS_CC);

	INIT_CLASS_ENTRY(ce, "PDO", pdo_dbh_functions);
	pdo_dbh_ce = zend_register_internal_class(&ce TSRMLS_CC);
	pdo_dbh_ce->create_object = pdo_dbh_new;

	INIT_CLASS_ENTRY(ce, "PDOStatement", pdo_dbstmt_functions);
	pdo_dbstmt_ce = zend_register_internal_class(&ce TSRMLS_CC);
	pdo_dbstmt_ce->get_iterator = pdo_stmt_iter_get;
	pdo_dbstmt_ce->create_object = pdo_dbstmt_new;
	zend_class_implements(pdo_dbstmt_ce TSRMLS_CC, 1, zend_ce_traversable); 

	INIT_CLASS_ENTRY(ce, "PDORow", pdo_row_functions);
	pdo_row_ce = zend_register_internal_class(&ce TSRMLS_CC);
	pdo_row_ce->create_object = pdo_row_new;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(pdo)
{
	UNREGISTER_INI_ENTRIES();
	zend_hash_destroy(&pdo_driver_hash);
	pdo_sqlstate_fini_error_table();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(pdo)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION */
PHP_RSHUTDOWN_FUNCTION(pdo)
{
	/* TODO: visit persistent handles: for each persistent statement handle,
	 * remove bound parameter associations */
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(pdo)
{
	HashPosition pos;
	ulong num_key;
	char *name;
	int namelen;
	
	php_info_print_table_start();
	php_info_print_table_header(2, "pdo support", "enabled");
	php_info_print_table_header(1, "Available PDO Drivers");

	zend_hash_internal_pointer_reset_ex(&pdo_driver_hash, &pos);
	while (HASH_KEY_IS_STRING == zend_hash_get_current_key_ex(&pdo_driver_hash, &name, &namelen, &num_key, 0, &pos)) {
		php_info_print_table_row(1, name);
		zend_hash_move_forward_ex(&pdo_driver_hash, &pos);
	}
	
	php_info_print_table_end();

#if 0
	DISPLAY_INI_ENTRIES();
#endif
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
