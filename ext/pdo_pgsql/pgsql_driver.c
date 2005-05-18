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
  | Author: Edin Kadribasic <edink@emini.dk>                             |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "pdo/php_pdo.h"
#include "pdo/php_pdo_driver.h"

#include "pg_config.h" /* needed for PG_VERSION */
#include "php_pdo_pgsql.h"
#include "php_pdo_pgsql_int.h"
#include "zend_exceptions.h"

static char * _pdo_pgsql_trim_message(const char *message, int persistent)
{
	register int i = strlen(message)-1;
	char *tmp;

	if (i>1 && (message[i-1] == '\r' || message[i-1] == '\n') && message[i] == '.') {
		--i;
	}
	while (i>0 && (message[i] == '\r' || message[i] == '\n')) {
		--i;
	}
	++i;
	tmp = pemalloc(i + 1, persistent);
	memcpy(tmp, message, i);
	tmp[i] = '\0';
	
	return tmp;
}

int _pdo_pgsql_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, int errcode, const char *sqlstate, const char *file, int line TSRMLS_DC) /* {{{ */
{
	pdo_pgsql_db_handle *H = (pdo_pgsql_db_handle *)dbh->driver_data;
	pdo_error_type *pdo_err = stmt ? &stmt->error_code : &dbh->error_code;
	pdo_pgsql_error_info *einfo = &H->einfo;
	char *errmsg = PQerrorMessage(H->server);

	einfo->errcode = errcode;
	einfo->file = file;
	einfo->line = line;

	if (einfo->errmsg) {
		pefree(einfo->errmsg, dbh->is_persistent);
		einfo->errmsg = NULL;
	}

	if (sqlstate == NULL) {
		strcpy(*pdo_err, "HY000");
	}
	else {
		strcpy(*pdo_err, sqlstate);
	}

	if (errmsg) {
		einfo->errmsg = _pdo_pgsql_trim_message(errmsg, dbh->is_persistent);
	}

	if (!dbh->methods) {
		zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "SQLSTATE[%s] [%d] %s",
				*pdo_err, einfo->errcode, einfo->errmsg);
	}
	
	return errcode;
}
/* }}} */

static void _pdo_pgsql_notice(pdo_dbh_t *dbh, const char *message) /* {{{ */
{
/*	pdo_pgsql_db_handle *H = (pdo_pgsql_db_handle *)dbh->driver_data; */
}
/* }}} */

static int pdo_pgsql_fetch_error_func(pdo_dbh_t *dbh, pdo_stmt_t *stmt, zval *info TSRMLS_DC) /* {{{ */
{
	pdo_pgsql_db_handle *H = (pdo_pgsql_db_handle *)dbh->driver_data;
	pdo_pgsql_error_info *einfo = &H->einfo;

	if (einfo->errcode) {
		add_next_index_long(info, einfo->errcode);
		add_next_index_string(info, einfo->errmsg, 1);
	}

	return 1;
}
/* }}} */

static int pgsql_handle_closer(pdo_dbh_t *dbh TSRMLS_DC) /* {{{ */
{
	pdo_pgsql_db_handle *H = (pdo_pgsql_db_handle *)dbh->driver_data;
	if (H) {
		if (H->server) {
			PQfinish(H->server);
			H->server = NULL;
		}
		if (H->einfo.errmsg) {
			pefree(H->einfo.errmsg, dbh->is_persistent);
			H->einfo.errmsg = NULL;
		}
		pefree(H, dbh->is_persistent);
		dbh->driver_data = NULL;
	}
	return 0;
}
/* }}} */

static int pgsql_handle_preparer(pdo_dbh_t *dbh, const char *sql, long sql_len, pdo_stmt_t *stmt, zval *driver_options TSRMLS_DC)
{
	pdo_pgsql_db_handle *H = (pdo_pgsql_db_handle *)dbh->driver_data;
	pdo_pgsql_stmt *S = ecalloc(1, sizeof(pdo_pgsql_stmt));
	int scrollable;

	S->H = H;
	stmt->driver_data = S;
	stmt->methods = &pgsql_stmt_methods;
	stmt->supports_placeholders = PDO_PLACEHOLDER_NONE;

	scrollable = pdo_attr_lval(driver_options, PDO_ATTR_CURSOR,
		PDO_CURSOR_FWDONLY TSRMLS_CC) == PDO_CURSOR_SCROLL;

	if (scrollable) {
		spprintf(&S->cursor_name, 0, "pdo_pgsql_cursor_%08x", (unsigned int) stmt);
	}

	return 1;
}

static long pgsql_handle_doer(pdo_dbh_t *dbh, const char *sql, long sql_len TSRMLS_DC)
{
	pdo_pgsql_db_handle *H = (pdo_pgsql_db_handle *)dbh->driver_data;
	PGresult *res;
	
	if (!(res = PQexec(H->server, sql))) {
		/* fatal error */
		pdo_pgsql_error(dbh, PGRES_FATAL_ERROR, NULL);
		return 0;
	} else {
		ExecStatusType qs = PQresultStatus(res);
		if (qs != PGRES_COMMAND_OK && qs != PGRES_TUPLES_OK) {
#if HAVE_PQRESULTERRORFIELD
			char * sqlstate = PQresultErrorField(res, PG_DIAG_SQLSTATE);
			pdo_pgsql_error(dbh, qs, (const char *)sqlstate);
			PQfreemem(sqlstate);
#else
			pdo_pgsql_error(dbh, qs, NULL);
#endif
			PQclear(res);
			return 0;
		}
		H->pgoid = PQoidValue(res);
		PQclear(res);
	}

	return 1;
}

static int pgsql_handle_quoter(pdo_dbh_t *dbh, const char *unquoted, int unquotedlen, char **quoted, int *quotedlen, enum pdo_param_type paramtype TSRMLS_DC)
{
	unsigned char *escaped;
	
	switch (paramtype) {
		case PDO_PARAM_LOB:
			/* escapedlen returned by PQescapeBytea() accounts for trailing 0 */
			escaped = PQescapeBytea(unquoted, unquotedlen, quotedlen);
			*quotedlen += 1;
			*quoted = emalloc(*quotedlen + 1);
			memcpy((*quoted)+1, escaped, *quotedlen-2);
			(*quoted)[0] = '\'';
			(*quoted)[*quotedlen-1] = '\'';
			(*quoted)[*quotedlen] = '\0';
			free(escaped);
			break;
		default:
			*quoted = emalloc(2*unquotedlen + 3);
			(*quoted)[0] = '\'';
			*quotedlen = PQescapeString(*quoted + 1, unquoted, unquotedlen);
			(*quoted)[*quotedlen + 1] = '\'';
			(*quoted)[*quotedlen + 2] = '\0';
			*quotedlen += 2;
	}
	return 1;
}

static char *pdo_pgsql_last_insert_id(pdo_dbh_t *dbh, const char *name, unsigned int *len TSRMLS_DC)
{
	pdo_pgsql_db_handle *H = (pdo_pgsql_db_handle *)dbh->driver_data;
	char *id = NULL;
	
	if (H->pgoid == InvalidOid) {
		return NULL;
	}

	/* TODO: if name != NULL, pull out last value for that sequence/column */

	*len = spprintf(&id, 0, "%lld", H->pgoid);
	return id;
}

static int pdo_pgsql_get_attribute(pdo_dbh_t *dbh, long attr, zval *return_value TSRMLS_DC)
{
	pdo_pgsql_db_handle *H = (pdo_pgsql_db_handle *)dbh->driver_data;

	switch (attr) {
		case PDO_ATTR_CLIENT_VERSION:
			ZVAL_STRING(return_value, PG_VERSION, 1);
			break;

		case PDO_ATTR_SERVER_VERSION:
#ifdef HAVE_PQPROTOCOLVERSION
			if (PQprotocolVersion(H->server) >= 3) { /* PostgreSQL 7.4 or later */
				ZVAL_STRING(return_value, (char*)PQparameterStatus(H->server, "server_version"), 1);
			} else /* emulate above via a query */
#endif
			{
				PGresult *res = PQexec(H->server, "SELECT VERSION()");
				if (res && PQresultStatus(res) == PGRES_TUPLES_OK) {
					ZVAL_STRING(return_value, (char *)PQgetvalue(res, 0, 0), 1);
				}

				if (res) {
					PQclear(res);
				}
			}
			break;

		case PDO_ATTR_CONNECTION_STATUS:
			switch (PQstatus(H->server)) {
				case CONNECTION_STARTED:
					ZVAL_STRINGL(return_value, "Waiting for connection to be made.", sizeof("Waiting for connection to be made.")-1, 1);
					break;

				case CONNECTION_MADE:
				case CONNECTION_OK:
					ZVAL_STRINGL(return_value, "Connection OK; waiting to send.", sizeof("Connection OK; waiting to send.")-1, 1);
					break;

				case CONNECTION_AWAITING_RESPONSE:
					ZVAL_STRINGL(return_value, "Waiting for a response from the server.", sizeof("Waiting for a response from the server.")-1, 1);
					break;

				case CONNECTION_AUTH_OK:
					ZVAL_STRINGL(return_value, "Received authentication; waiting for backend start-up to finish.", sizeof("Received authentication; waiting for backend start-up to finish.")-1, 1);
					break;
#ifdef CONNECTION_SSL_STARTUP
				case CONNECTION_SSL_STARTUP:
					ZVAL_STRINGL(return_value, "Negotiating SSL encryption.", sizeof("Negotiating SSL encryption.")-1, 1);
					break;
#endif
				case CONNECTION_SETENV:
					ZVAL_STRINGL(return_value, "Negotiating environment-driven parameter settings.", sizeof("Negotiating environment-driven parameter settings.")-1, 1);
					break;

				case CONNECTION_BAD:
				default:
					ZVAL_STRINGL(return_value, "Bad connection.", sizeof("Bad connection.")-1, 1);
					break;
			}
			break;

		case PDO_ATTR_SERVER_INFO: {
			int spid = PQbackendPID(H->server);
			char *tmp;
#ifdef HAVE_PQPROTOCOLVERSION
			spprintf(&tmp, 0, 
				"PID: %d; Client Encoding: %s; Is Superuser: %s; Session Authorization: %s; Date Style: %s", 
				spid,
				(char*)PQparameterStatus(H->server, "client_encoding"),
				(char*)PQparameterStatus(H->server, "is_superuser"),
				(char*)PQparameterStatus(H->server, "session_authorization"),
				(char*)PQparameterStatus(H->server, "DateStyle"));
#else 
			spprintf(&tmp, 0, "PID: %d", spid);
#endif
			ZVAL_STRING(return_value, tmp, 0);
		}
			break;

		default:
			return 0;	
	}

	return 1;
}

static int pdo_pgsql_transaction_cmd(const char *cmd, pdo_dbh_t *dbh TSRMLS_DC)
{
	pdo_pgsql_db_handle *H = (pdo_pgsql_db_handle *)dbh->driver_data;
	PGresult *res;
	int ret = 1;

	res = PQexec(H->server, cmd);

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		ret = 0;
	}

	PQclear(res);
	return ret;
}

static int pgsql_handle_begin(pdo_dbh_t *dbh TSRMLS_DC)
{
	return pdo_pgsql_transaction_cmd("BEGIN", dbh TSRMLS_CC);
}

static int pgsql_handle_commit(pdo_dbh_t *dbh TSRMLS_DC)
{
	return pdo_pgsql_transaction_cmd("COMMIT", dbh TSRMLS_CC);
}

static int pgsql_handle_rollback(pdo_dbh_t *dbh TSRMLS_DC)
{
	return pdo_pgsql_transaction_cmd("ROLLBACK", dbh TSRMLS_CC);
}

static struct pdo_dbh_methods pgsql_methods = {
	pgsql_handle_closer,
	pgsql_handle_preparer,
	pgsql_handle_doer,
	pgsql_handle_quoter,
	pgsql_handle_begin,
	pgsql_handle_commit,
	pgsql_handle_rollback,
	NULL, /* set_attr */
	pdo_pgsql_last_insert_id,
	pdo_pgsql_fetch_error_func,
	pdo_pgsql_get_attribute,
	NULL,	/* check_liveness */
	NULL  /* get_driver_methods */
};

static int pdo_pgsql_handle_factory(pdo_dbh_t *dbh, zval *driver_options TSRMLS_DC) /* {{{ */
{
	pdo_pgsql_db_handle *H;
	int ret = 0;
	char *conn_str, *p, *e;

	H = pecalloc(1, sizeof(pdo_pgsql_db_handle), dbh->is_persistent);
	dbh->driver_data = H;

	H->einfo.errcode = 0;
	H->einfo.errmsg = NULL;
	
	/* PostgreSQL wants params in the connect string to be separated by spaces,
	 * if the PDO standard semicolons are used, we convert them to spaces
	 */
	e = (char *) dbh->data_source + strlen(dbh->data_source);
	p = (char *) dbh->data_source;
	while ((p = memchr(p, ';', (e - p)))) {
		*p = ' ';
	}

	/* support both full connection string & connection string + login and/or password */
	if (!dbh->username || !dbh->password) {
		conn_str = (char *) dbh->data_source;
	} else if (dbh->username && dbh->password) {
		spprintf(&conn_str, 0, "%s user=%s password=%s", dbh->data_source, dbh->username, dbh->password);
	} else if (dbh->username) {
		spprintf(&conn_str, 0, "%s user=%s", dbh->data_source, dbh->username);
	} else {
		spprintf(&conn_str, 0, "%s password=%s", dbh->data_source, dbh->password);
	}

	H->server = PQconnectdb(conn_str);
	
	if (conn_str != dbh->data_source) {
		efree(conn_str);
	}
	
	if (PQstatus(H->server) != CONNECTION_OK) {
		pdo_pgsql_error(dbh, PGRES_FATAL_ERROR, PHP_PDO_PGSQL_CONNECTION_FAILURE_SQLSTATE);
		goto cleanup;
	}

	PQsetNoticeProcessor(H->server, (void(*)(void*,const char*))_pdo_pgsql_notice, (void *)&dbh);

	H->attached = 1;
	H->pgoid = -1;

	dbh->methods = &pgsql_methods;
	dbh->alloc_own_columns = 1;
	dbh->max_escaped_char_length = 2;

	ret = 1;
	
cleanup:
	dbh->methods = &pgsql_methods;
	if (!ret) {
		pgsql_handle_closer(dbh TSRMLS_CC);
	}

	return ret;
}
/* }}} */

pdo_driver_t pdo_pgsql_driver = {
	PDO_DRIVER_HEADER(pgsql),
	pdo_pgsql_handle_factory
};

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
