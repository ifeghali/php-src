/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2004 The PHP Group                                |
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

/* The PDO Database Handle Class */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_pdo.h"
#include "php_pdo_driver.h"
#include "php_pdo_int.h"
#include "zend_exceptions.h"

/* {{{ proto object PDO::__construct(string dsn, string username, string passwd [, array driver_opts])
   */
static PHP_FUNCTION(dbh_constructor)
{
	zval *object = getThis();
	pdo_dbh_t *dbh = NULL;
	zend_bool is_persistent = FALSE;
	char *data_source;
	long data_source_len;
	char *driver_name_length;
	char *colon;
	char *username, *password;
	long usernamelen, passwordlen;
	pdo_driver_t *driver = NULL;
	zval *driver_options = NULL;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss|a!", &data_source, &data_source_len,
				&username, &usernamelen, &password, &passwordlen, &driver_options)) {
		ZVAL_NULL(object);
		return;
	}

	/* parse the data source name */
	colon = strchr(data_source, ':');

	if (!colon) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "invalid data source name");
		ZVAL_NULL(object);
		return;
	}

	driver = pdo_find_driver(data_source, colon - data_source);

	if (!driver) {
		/* NB: don't want to include the data_source in the error message as
		 * it might contain a password */
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not find driver");
		ZVAL_NULL(object);
		return;
	}
	
	dbh = pemalloc(sizeof(*dbh), is_persistent);

	if (dbh == NULL) {
		/* need this check for persistent allocations */
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "out of memory");
	}
	
	memset(dbh, 0, sizeof(*dbh));
	dbh->is_persistent = is_persistent;

	dbh->data_source_len = strlen(colon + 1);
	/* when persistent stuff is done, we should check the return values here
	 * too */
	dbh->data_source = (const char*)pestrdup(colon + 1, is_persistent);
	dbh->username = pestrdup(username, is_persistent);
	dbh->password = pestrdup(password, is_persistent);

	if (driver_options) {
		dbh->auto_commit = pdo_attr_lval(driver_options, PDO_ATTR_AUTOCOMMIT, 1 TSRMLS_CC);
	} else {
		dbh->auto_commit = 1;
	}
	
	if (driver->db_handle_factory(dbh, driver_options TSRMLS_CC)) {
		/* all set */

		if (is_persistent) {
			/* register in the persistent list etc. */
			;
		}

		zend_object_store_set_object(object, dbh TSRMLS_CC);
		return;	
	}

	/* the connection failed; tidy things up */
	pefree((char*)dbh->data_source, is_persistent);
	pefree(dbh, is_persistent);

	/* XXX raise exception */
	ZVAL_NULL(object);
}
/* }}} */

/* {{{ proto object PDO::prepare(string statment [, int options [, array driver_options]])
   Prepares a statement for execution and returns a statement object */
/* TODO: options will be a PDO specific bitmask controlling such things as
 * cursor type. */
static PHP_METHOD(PDO, prepare)
{
	pdo_dbh_t *dbh = zend_object_store_get_object(getThis() TSRMLS_CC);
	pdo_stmt_t *stmt;
	char *statement;
	long statement_len;
	zval *driver_options = NULL;
	long options = 0;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|la", &statement,
			&statement_len, &options, &driver_options)) {
		RETURN_FALSE;
	}
	
	stmt = ecalloc(1, sizeof(*stmt));
	/* unconditionally keep this for later reference */
	stmt->query_string = estrndup(statement, statement_len);
	stmt->query_stringlen = statement_len;
	if (dbh->methods->preparer(dbh, statement, statement_len, stmt, options, driver_options TSRMLS_CC)) {
		/* prepared; create a statement object for PHP land to access it */
		Z_TYPE_P(return_value) = IS_OBJECT;
		Z_OBJ_HANDLE_P(return_value) = zend_objects_store_put(stmt, NULL, pdo_dbstmt_free_storage, NULL TSRMLS_CC);
		Z_OBJ_HT_P(return_value) = &pdo_dbstmt_object_handlers;

		/* give it a reference to me */
		stmt->database_object_handle = *getThis();
		zend_objects_store_add_ref(getThis() TSRMLS_CC);
		stmt->dbh = dbh;
		return;
	}
	efree(stmt);
	RETURN_FALSE;
}

/* {{{ proto bool PDO::beginTransaction()
   Initiates a transaction */
static PHP_METHOD(PDO, beginTransaction)
{
	pdo_dbh_t *dbh = zend_object_store_get_object(getThis() TSRMLS_CC);

	if (dbh->in_txn) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "There is already an active transaction");
		RETURN_FALSE;
	}
	
	if (!dbh->methods->begin) {
		/* TODO: this should be an exception; see the auto-commit mode
		 * comments below */
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "This driver does not support transactions");
		RETURN_FALSE;
	}

	if (dbh->methods->begin(dbh TSRMLS_CC)) {
		dbh->in_txn = 1;
		RETURN_TRUE;
	}

	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool PDO::commit()
   Commit a transaction */
static PHP_METHOD(PDO, commit)
{
	pdo_dbh_t *dbh = zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!dbh->in_txn) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "There is no active transaction");
		RETURN_FALSE;
	}

	if (dbh->methods->commit(dbh TSRMLS_CC)) {
		dbh->in_txn = 0;
		RETURN_TRUE;
	}
	
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool PDO::rollBack()
   roll back a transaction */
static PHP_METHOD(PDO, rollBack)
{
	pdo_dbh_t *dbh = zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!dbh->in_txn) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "There is no active transaction");
		RETURN_FALSE;
	}

	if (dbh->methods->rollback(dbh TSRMLS_CC)) {
		dbh->in_txn = 0;
		RETURN_TRUE;
	}
		
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool PDO::setAttribute(long attribute, mixed value)
   Set an attribute */
static PHP_METHOD(PDO, setAttribute)
{
	pdo_dbh_t *dbh = zend_object_store_get_object(getThis() TSRMLS_CC);
	long attr;
	zval *value = NULL;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz!", &attr, &value)) {
		RETURN_FALSE;
	}

	if (!dbh->methods->set_attribute) {
		goto fail;
	}

	if (dbh->methods->set_attribute(dbh, attr, value TSRMLS_CC)) {
		RETURN_TRUE;
	}

fail:
	if (attr == PDO_ATTR_AUTOCOMMIT) {
		/* Feature: if the auto-commit mode cannot be changed, throw an
		 * exception.  Until I've added the code for that, raise an
		 * E_ERROR */
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "The auto commit mode cannot be changed for this driver");
	} else if (!dbh->methods->set_attribute) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "This driver doesn't support setting attributes");
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool PDO::exec(string query)
   Execute a query that does not return a row set */
static PHP_METHOD(PDO, exec)
{
	pdo_dbh_t *dbh = zend_object_store_get_object(getThis() TSRMLS_CC);
	char *statement;
	long statement_len;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &statement, &statement_len)) {
		RETURN_FALSE;
	}

	if (!statement_len) {
		RETURN_FALSE;
	}

	RETURN_BOOL(dbh->methods->doer(dbh, statement, statement_len TSRMLS_CC));
}
/* }}} */

/* {{{ proto int PDO::affectedRows()
   Returns the number of rows that we affected by the last call to PDO::exec().  Not always meaningful. */
static PHP_METHOD(PDO, affectedRows)
{
	pdo_dbh_t *dbh = zend_object_store_get_object(getThis() TSRMLS_CC);

	if (ZEND_NUM_ARGS()) {
		RETURN_FALSE;
	}

	if (!dbh->methods->affected) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "This driver affected rows retrieval.");
	} else {
		RETURN_LONG(dbh->methods->affected(dbh));
	}
}
/* }}} */

/* {{{ proto int PDO::lastInsertId()
   Returns the number id of rows that we affected by the last call to PDO::exec().  Not always meaningful. */
static PHP_METHOD(PDO, lastInsertId)
{
	pdo_dbh_t *dbh = zend_object_store_get_object(getThis() TSRMLS_CC);

	if (ZEND_NUM_ARGS()) {
		RETURN_FALSE;
	}

	if (!dbh->methods->last_id) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "This driver last inserted id retrieval.");
	} else {
		RETURN_LONG(dbh->methods->last_id(dbh));
	}
}
/* }}} */

function_entry pdo_dbh_functions[] = {
	PHP_ME(PDO, prepare, 		NULL, 					ZEND_ACC_PUBLIC)
	PHP_ME(PDO, beginTransaction,NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDO, commit,			NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDO, rollBack,		NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDO, setAttribute,	NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDO, exec,			NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDO, affectedRows,	NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDO, lastInsertId,	NULL,					ZEND_ACC_PUBLIC)

	{NULL, NULL, NULL}
};

/* {{{ overloaded object handlers for PDO class */
static zval *dbh_prop_read(zval *object, zval *member, int type TSRMLS_DC)
{
	zval *return_value;

	MAKE_STD_ZVAL(return_value);
	ZVAL_NULL(return_value);

	return return_value;
}

static void dbh_prop_write(zval *object, zval *member, zval *value TSRMLS_DC)
{
	return;
}

static zval *dbh_read_dim(zval *object, zval *offset, int type TSRMLS_DC)
{
	zval *return_value;

	MAKE_STD_ZVAL(return_value);
	ZVAL_NULL(return_value);

	return return_value;
}

static void dbh_write_dim(zval *object, zval *offset, zval *value TSRMLS_DC)
{
	return;
}

static int dbh_prop_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
{
	return 0;
}

static int dbh_dim_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
{
	return 0;
}

static void dbh_prop_delete(zval *object, zval *offset TSRMLS_DC)
{
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot delete properties from a PDO DBH");
}

static void dbh_dim_delete(zval *object, zval *offset TSRMLS_DC)
{
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot delete dimensions from a PDO DBH");
}

static HashTable *dbh_get_properties(zval *object TSRMLS_DC)
{
	return NULL;
}

static union _zend_function *dbh_method_get(zval *object, char *method_name, int method_len TSRMLS_DC)
{
	zend_function *fbc;
	char *lc_method_name;

	lc_method_name = do_alloca(method_len + 1);
	zend_str_tolower_copy(lc_method_name, method_name, method_len);

	if (zend_hash_find(&pdo_dbh_ce->function_table, lc_method_name, method_len+1, (void**)&fbc) == FAILURE) {
		free_alloca(lc_method_name);
		return NULL;
	}
	
	free_alloca(lc_method_name);
	return fbc;
}

static int dbh_call_method(char *method, INTERNAL_FUNCTION_PARAMETERS)
{
	return FAILURE;
}

static union _zend_function *dbh_get_ctor(zval *object TSRMLS_DC)
{
	static zend_internal_function ctor = {0};

	ctor.type = ZEND_INTERNAL_FUNCTION;
	ctor.function_name = "__construct";
	ctor.scope = pdo_dbh_ce;
	ctor.handler = ZEND_FN(dbh_constructor);

	return (union _zend_function*)&ctor;
}

static zend_class_entry *dbh_get_ce(zval *object TSRMLS_DC)
{
	return pdo_dbh_ce;
}

static int dbh_get_classname(zval *object, char **class_name, zend_uint *class_name_len, int parent TSRMLS_DC)
{
	*class_name = estrndup("PDO", sizeof("PDO")-1);
	*class_name_len = sizeof("PDO")-1;
	return 0;
}

static int dbh_compare(zval *object1, zval *object2 TSRMLS_DC)
{
	return -1;
}

static zend_object_handlers pdo_dbh_object_handlers = {
	ZEND_OBJECTS_STORE_HANDLERS,
	dbh_prop_read,
	dbh_prop_write,
	dbh_read_dim,
	dbh_write_dim,
	NULL,
	NULL,
	NULL,
	dbh_prop_exists,
	dbh_prop_delete,
	dbh_dim_exists,
	dbh_dim_delete,
	dbh_get_properties,
	dbh_method_get,
	dbh_call_method,
	dbh_get_ctor,
	dbh_get_ce,
	dbh_get_classname,
	dbh_compare,
	NULL, /* cast */
	NULL
};

static void pdo_dbh_free_storage(zend_object *object TSRMLS_DC)
{
	pdo_dbh_t *dbh = (pdo_dbh_t*)object;

	if (!dbh) {
		return;
	}

	if (dbh->is_persistent) {
		/* XXX: don't really free it, just delete the rsrc id */
		return;
	}

	dbh->methods->closer(dbh TSRMLS_CC);
	efree(dbh);
}

zend_object_value pdo_dbh_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;

	retval.handle = zend_objects_store_put(NULL, NULL, pdo_dbh_free_storage, NULL TSRMLS_CC);
	retval.handlers = &pdo_dbh_object_handlers;

	return retval;
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
