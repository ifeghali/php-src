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
#include "php_pdo_oci.h"
#include "php_pdo_oci_int.h"

#define STMT_CALL(name, params)	\
	S->last_err = name params; \
	S->last_err = _oci_error(S->err, stmt->dbh, stmt, #name, S->last_err, __FILE__, __LINE__ TSRMLS_CC); \
	if (S->last_err) { \
		return 0; \
	}

#define STMT_CALL_MSG(name, msg, params)	\
	S->last_err = name params; \
	S->last_err = _oci_error(S->err, stmt->dbh, stmt, #name ": " #msg, S->last_err, __FILE__, __LINE__ TSRMLS_CC); \
	if (S->last_err) { \
		return 0; \
	}


static int oci_stmt_dtor(pdo_stmt_t *stmt TSRMLS_DC)
{
	pdo_oci_stmt *S = (pdo_oci_stmt*)stmt->driver_data;
	int i;

	if (S->stmt) {
		/* cancel server side resources for the statement if we didn't
		 * fetch it all */
		OCIStmtFetch(S->stmt, S->err, 0, OCI_FETCH_NEXT, OCI_DEFAULT);

		/* free the handle */
		OCIHandleFree(S->stmt, OCI_HTYPE_STMT);
		S->stmt = NULL;
	}
	if (S->err) {
		OCIHandleFree(S->err, OCI_HTYPE_ERROR);
		S->err = NULL;
	}

	if (S->cols) {
		for (i = 0; i < stmt->column_count; i++) {
			if (S->cols[i].data) {
				efree(S->cols[i].data);
			}
		}
		efree(S->cols);
		S->cols = NULL;
	}
	efree(S);

	return 1;
}

static int oci_stmt_execute(pdo_stmt_t *stmt TSRMLS_DC)
{
	pdo_oci_stmt *S = (pdo_oci_stmt*)stmt->driver_data;
	ub4 rowcount;

	if (!S->stmt_type) {
		STMT_CALL_MSG(OCIAttrGet, "OCI_ATTR_STMT_TYPE",
				(S->stmt, OCI_HTYPE_STMT, &S->stmt_type, 0, OCI_ATTR_STMT_TYPE, S->err));
	}

	if (stmt->executed) {
		/* ensure that we cancel the cursor from a previous fetch */
		OCIStmtFetch(S->stmt, S->err, 0, OCI_FETCH_NEXT, OCI_DEFAULT);
	}

	STMT_CALL(OCIStmtExecute, (S->H->svc, S->stmt, S->err,
				S->stmt_type == OCI_STMT_SELECT ? 0 : 1, 0, NULL, NULL,
				(stmt->dbh->auto_commit && !stmt->dbh->in_txn) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT));

	if (!stmt->executed) {
		ub4 colcount;
		/* do first-time-only definition of bind/mapping stuff */

		/* how many columns do we have ? */
		STMT_CALL_MSG(OCIAttrGet, "ATTR_PARAM_COUNT",
				(S->stmt, OCI_HTYPE_STMT, &colcount, 0, OCI_ATTR_PARAM_COUNT, S->err));

		stmt->column_count = (int)colcount;

		S->cols = ecalloc(colcount, sizeof(pdo_oci_column));
	}
	
	STMT_CALL_MSG(OCIAttrGet, "ATTR_ROW_COUNT",
			(S->stmt, OCI_HTYPE_STMT, &rowcount, 0, OCI_ATTR_ROW_COUNT, S->err));
	stmt->row_count = (long)rowcount;

	return 1;
}

static sb4 oci_bind_input_cb(dvoid *ctx, OCIBind *bindp, ub4 iter, ub4 index, dvoid **bufpp,
		ub4 *alenp, ub1 *piecep, dvoid **indpp)
{
	struct pdo_bound_param_data *param = (struct pdo_bound_param_data*)ctx;
	pdo_oci_bound_param *P = (pdo_oci_bound_param*)ecalloc(1, sizeof(pdo_oci_bound_param));
	TSRMLS_FETCH();

	if (!param || !param->parameter) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "param is NULL in oci_bind_input_cb; this should not happen");
		return OCI_ERROR;
	}
	
	*indpp = &P->indicator;

	if (ZVAL_IS_NULL(param->parameter)) {
		/* insert a NULL value into the column */
		P->indicator = -1; /* NULL */
		*bufpp = 0;
		*alenp = -1;
	} else if (!P->thing) {
		/* regular string bind */
		convert_to_string(param->parameter);
		*bufpp = Z_STRVAL_P(param->parameter);
		*alenp = Z_STRLEN_P(param->parameter);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "P->thing should not be set??");
		return OCI_ERROR;
	}

	*piecep = OCI_ONE_PIECE;
	return OCI_CONTINUE;
}

static sb4 oci_bind_output_cb(dvoid *ctx, OCIBind *bindp, ub4 iter, ub4 index, dvoid **bufpp, ub4 **alenpp,
		ub1 *piecep, dvoid **indpp, ub2 **rcodepp)
{
	struct pdo_bound_param_data *param = (struct pdo_bound_param_data*)ctx;
	pdo_oci_bound_param *P = (pdo_oci_bound_param*)ecalloc(1, sizeof(pdo_oci_bound_param));
	TSRMLS_FETCH();

	if (!param || !param->parameter) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "param is NULL in oci_bind_output_cb; this should not happen");
		return OCI_ERROR;
	}
	
	if (Z_TYPE_P(param->parameter) == IS_OBJECT || Z_TYPE_P(param->parameter) == IS_RESOURCE) {
		return OCI_CONTINUE;
	}

	convert_to_string(param->parameter);
	zval_dtor(param->parameter);

	Z_STRLEN_P(param->parameter) = param->max_value_len;
	Z_STRVAL_P(param->parameter) = emalloc(Z_STRLEN_P(param->parameter)+1);

	
	*alenpp = &P->actual_len;
	*bufpp = Z_STRVAL_P(param->parameter);
	*piecep = OCI_ONE_PIECE;
	*rcodepp = &P->retcode;
	*indpp = &P->indicator;

	return OCI_CONTINUE;
}

static int oci_stmt_param_hook(pdo_stmt_t *stmt, struct pdo_bound_param_data *param,
		enum pdo_param_event event_type TSRMLS_DC)
{
	pdo_oci_stmt *S = (pdo_oci_stmt*)stmt->driver_data;

	/* we're only interested in parameters for prepared SQL right now */
	if (param->is_param) {
		pdo_oci_bound_param *P;
		sb4 value_sz = -1;
		
		P = (pdo_oci_bound_param*)param->driver_data;

		switch (event_type) {
			case PDO_PARAM_EVT_ALLOC:
				P = (pdo_oci_bound_param*)ecalloc(1, sizeof(pdo_oci_bound_param));
				param->driver_data = P;
			
				/* figure out what we're doing */
				switch (param->param_type) {
					case PDO_PARAM_LOB:
					case PDO_PARAM_STMT:
						return 0;

					case PDO_PARAM_STR:
					default:
						P->oci_type = SQLT_CHR;
						convert_to_string(param->parameter);
						value_sz = param->max_value_len + 1;
						P->actual_len = Z_STRLEN_P(param->parameter);
						
				}
				
				if (param->name) {
					STMT_CALL(OCIBindByName, (S->stmt,
							&P->bind, S->err, (text*)param->name,
							param->namelen, 0, value_sz, P->oci_type,
							&P->indicator, 0, &P->retcode, 0, 0,
							OCI_DATA_AT_EXEC));
				} else {
					STMT_CALL(OCIBindByPos, (S->stmt,
							&P->bind, S->err, param->paramno+1,
							0, value_sz, P->oci_type,
							&P->indicator, 0, &P->retcode, 0, 0,
							OCI_DATA_AT_EXEC));
				}
			
				STMT_CALL(OCIBindDynamic, (P->bind,
							S->err,
							param, oci_bind_input_cb,
							param, oci_bind_output_cb));

				return 1;

			case PDO_PARAM_EVT_EXEC_PRE:
				P->indicator = 0;
				return 1;

			case PDO_PARAM_EVT_EXEC_POST:
				/* fixup stuff set in motion in oci_bind_output_cb */
				if (P->indicator == -1) {
					/* set up a NULL value */
					if (Z_TYPE_P(param->parameter) == IS_STRING && Z_STRVAL_P(param->parameter) != empty_string) {
						/* OCI likes to stick non-terminated strings in things */
						*Z_STRVAL_P(param->parameter) = '\0';
					}
					zval_dtor(param->parameter);
					ZVAL_NULL(param->parameter);
				} else if (Z_TYPE_P(param->parameter) == IS_STRING && Z_STRVAL_P(param->parameter) != empty_string) {
					Z_STRLEN_P(param->parameter) = P->actual_len;
					Z_STRVAL_P(param->parameter) = erealloc(Z_STRVAL_P(param->parameter), P->actual_len+1);
					Z_STRVAL_P(param->parameter)[P->actual_len] = '\0';
				}

				return 1;
		}
	}
	
	return 1;
}

static int oci_stmt_fetch(pdo_stmt_t *stmt TSRMLS_DC)
{
	pdo_oci_stmt *S = (pdo_oci_stmt*)stmt->driver_data;

	S->last_err = OCIStmtFetch(S->stmt, S->err, 1, OCI_FETCH_NEXT, OCI_DEFAULT);

	if (S->last_err == OCI_NO_DATA) {
		/* no (more) data */
		return 0;
	}

	if (S->last_err == OCI_NEED_DATA) {
		oci_stmt_error("OCI_NEED_DATA");
		return 0;
	}

	if (S->last_err == OCI_SUCCESS_WITH_INFO || S->last_err == OCI_SUCCESS) {
		return 1;
	}
	
	oci_stmt_error("OCIStmtFetch");

	return 0;
}

static int oci_stmt_describe(pdo_stmt_t *stmt, int colno TSRMLS_DC)
{
	pdo_oci_stmt *S = (pdo_oci_stmt*)stmt->driver_data;
	OCIParam *param = NULL;
	text *colname;
	ub2 dtype, data_size, scale, precis;
	ub4 namelen;
	struct pdo_column_data *col = &stmt->columns[colno];
	zend_bool dyn = FALSE;

	/* describe the column */
	STMT_CALL(OCIParamGet, (S->stmt, OCI_HTYPE_STMT, S->err, (dvoid*)&param, colno+1));

	/* what type ? */
	STMT_CALL_MSG(OCIAttrGet, "OCI_ATTR_DATA_TYPE",
			(param, OCI_DTYPE_PARAM, &dtype, 0, OCI_ATTR_DATA_TYPE, S->err));

	/* how big ? */
	STMT_CALL_MSG(OCIAttrGet, "OCI_ATTR_DATA_SIZE",
			(param, OCI_DTYPE_PARAM, &data_size, 0, OCI_ATTR_DATA_SIZE, S->err));

	/* scale ? */
	STMT_CALL_MSG(OCIAttrGet, "OCI_ATTR_SCALE",
			(param, OCI_DTYPE_PARAM, &scale, 0, OCI_ATTR_SCALE, S->err));

	/* precision ? */
	STMT_CALL_MSG(OCIAttrGet, "OCI_ATTR_PRECISION",
			(param, OCI_DTYPE_PARAM, &precis, 0, OCI_ATTR_PRECISION, S->err));

	/* name ? */
	STMT_CALL_MSG(OCIAttrGet, "OCI_ATTR_NAME",
			(param, OCI_DTYPE_PARAM, &colname, &namelen, OCI_ATTR_NAME, S->err));

	col->precision = scale;
	col->maxlen = data_size;
	col->namelen = namelen;
	col->name = estrndup(colname, namelen);

	/* how much room do we need to store the field */
	switch (dtype) {
		case SQLT_BIN:
		default:
			dyn = FALSE;
			if (dtype == SQLT_DAT || dtype == SQLT_NUM
#ifdef SQLT_TIMESTAMP
					|| dtype == SQLT_TIMESTAMP
#endif
#ifdef SQLT_TIMESTAMP_TZ
					|| dtype == SQLT_TIMESTAMP_TZ
#endif
					) {
				/* should be big enough for most date formats and numbers */
				S->cols[colno].datalen = 512;
			} else {
				S->cols[colno].datalen = col->maxlen + 1; /* 1 for NUL terminator */
			}
			if (dtype == SQLT_BIN) {
				S->cols[colno].datalen *= 3;
			}
			S->cols[colno].data = emalloc(S->cols[colno].datalen);
			dtype = SQLT_CHR;

			/* returning data as a string */
			col->param_type = PDO_PARAM_STR;
	}

	if (!dyn) {
		STMT_CALL(OCIDefineByPos, (S->stmt, &S->cols[colno].def, S->err, colno+1,
					S->cols[colno].data, S->cols[colno].datalen, dtype, &S->cols[colno].indicator,
					&S->cols[colno].fetched_len, &S->cols[colno].retcode, OCI_DEFAULT));
	}
	
	return 1;
}

static int oci_stmt_get_col(pdo_stmt_t *stmt, int colno, char **ptr, unsigned long *len TSRMLS_DC)
{
	pdo_oci_stmt *S = (pdo_oci_stmt*)stmt->driver_data;
	pdo_oci_column *C = &S->cols[colno];

	/* check the indicator to ensure that the data is intact */
	if (C->indicator == -1) {
		/* A NULL value */
		*ptr = NULL;
		*len = 0;
		return 1;
	} else if (C->indicator == 0) {
		/* it was stored perfectly */
		*ptr = C->data;
		*len = C->fetched_len;
		return 1;
	} else {
		/* it was truncated */
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "column %d data was too large for buffer and was truncated to fit it", colno);

		*ptr = C->data;
		*len = C->fetched_len;
		return 1;
	}
}

struct pdo_stmt_methods oci_stmt_methods = {
	oci_stmt_dtor,
	oci_stmt_execute,
	oci_stmt_fetch,
	oci_stmt_describe,
	oci_stmt_get_col,
	oci_stmt_param_hook
};

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
