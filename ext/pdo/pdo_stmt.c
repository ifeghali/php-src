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

/* The PDO Statement Handle Class */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_var.h"
#include "php_pdo.h"
#include "php_pdo_driver.h"
#include "php_pdo_int.h"
#include "zend_exceptions.h"
#include "php_memory_streams.h"

#if COMPILE_DL_PDO
/* {{{ content from zend_arg_defs.c:
 * since it is a .c file, it won't be installed for use by PECL extensions, so we include it here. */
ZEND_BEGIN_ARG_INFO(first_arg_force_ref, 0)
	ZEND_ARG_PASS_INFO(1)
ZEND_END_ARG_INFO();


ZEND_BEGIN_ARG_INFO(second_arg_force_ref, 0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(1)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO(third_arg_force_ref, 0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(1)
ZEND_END_ARG_INFO();


ZEND_BEGIN_ARG_INFO(fourth_arg_force_ref, 0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(1)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO(all_args_by_ref, 1)
ZEND_END_ARG_INFO();
/* }}} */
#endif

static PHP_FUNCTION(dbstmt_constructor) /* {{{ */
{
	php_error_docref(NULL TSRMLS_CC, E_ERROR, "You should not create a PDOStatement manually");
}
/* }}} */

static inline int rewrite_name_to_position(pdo_stmt_t *stmt, struct pdo_bound_param_data *param TSRMLS_DC) /* {{{ */
{
	if (stmt->bound_param_map) {
		/* rewriting :name to ? style.
		 * We need to fixup the parameter numbers on the parameters.
		 * If we find that a given named parameter has been used twice,
		 * we will raise an error, as we can't be sure that it is safe
		 * to bind multiple parameters onto the same zval in the underlying
		 * driver */
		char *name;
		int position = 0;
		zend_hash_internal_pointer_reset(stmt->bound_param_map);
		while (SUCCESS == zend_hash_get_current_data(stmt->bound_param_map, (void**)&name)) {
			if (strcmp(name, param->name)) {
				position++;
				zend_hash_move_forward(stmt->bound_param_map);
				continue;
			}
			if (param->paramno >= 0) {
				pdo_raise_impl_error(stmt->dbh, stmt, "IM001", "PDO refuses to handle repeating the same :named parameter for multiple positions with this driver, as it might be unsafe to do so.  Consider using a separate name for each parameter instead" TSRMLS_CC);
				return -1;
			}
			param->paramno = position;
			return 1;
		}
		pdo_raise_impl_error(stmt->dbh, stmt, "HY093", "parameter was not defined" TSRMLS_CC);
		return 0;
	}
	return 1;	
}
/* }}} */

/* trigger callback hook for parameters */
static int dispatch_param_event(pdo_stmt_t *stmt, enum pdo_param_event event_type TSRMLS_DC) /* {{{ */
{
	int ret = 1, is_param = 1;
	struct pdo_bound_param_data *param;
	HashTable *ht;

	if (!stmt->methods->param_hook) {
		return 1;
	}

	ht = stmt->bound_params;

iterate:
	if (ht) {
		zend_hash_internal_pointer_reset(ht);
		while (SUCCESS == zend_hash_get_current_data(ht, (void**)&param)) {
			if (!stmt->methods->param_hook(stmt, param, event_type TSRMLS_CC)) {
				ret = 0;
				break;
			}
			
			zend_hash_move_forward(ht);
		}
	}
	if (ret && is_param) {
		ht = stmt->bound_columns;
		is_param = 0;
		goto iterate;
	}

	return ret;
}
/* }}} */

int pdo_stmt_describe_columns(pdo_stmt_t *stmt TSRMLS_DC) /* {{{ */
{
	int col;

	stmt->columns = ecalloc(stmt->column_count, sizeof(struct pdo_column_data));

	for (col = 0; col < stmt->column_count; col++) {
		if (!stmt->methods->describer(stmt, col TSRMLS_CC)) {
			return 0;
		}

		/* if we are applying case conversions on column names, do so now */
		if (stmt->dbh->native_case != stmt->dbh->desired_case && stmt->dbh->desired_case != PDO_CASE_NATURAL) {
			char *s = stmt->columns[col].name;

			switch (stmt->dbh->desired_case) {
				case PDO_CASE_UPPER:
					while (*s != '\0') {
						*s = toupper(*s);
						s++;
					}
					break;
				case PDO_CASE_LOWER:
					while (*s != '\0') {
						*s = tolower(*s);
						s++;
					}
					break;
				default:
					;
			}
		}

#if 0
		/* update the column index on named bound parameters */
		if (stmt->bound_params) {
			struct pdo_bound_param_data *param;

			if (SUCCESS == zend_hash_find(stmt->bound_params, stmt->columns[col].name,
						stmt->columns[col].namelen, (void**)&param)) {
				param->paramno = col;
			}
		}
#endif
		if (stmt->bound_columns) {
			struct pdo_bound_param_data *param;

			if (SUCCESS == zend_hash_find(stmt->bound_columns, stmt->columns[col].name,
						stmt->columns[col].namelen, (void**)&param)) {
				param->paramno = col;
			}
		}

	}
	return 1;
}
/* }}} */

static void get_lazy_object(pdo_stmt_t *stmt, zval *return_value TSRMLS_DC) /* {{{ */
{
	if (Z_TYPE(stmt->lazy_object_ref) == IS_NULL) {
		Z_TYPE(stmt->lazy_object_ref) = IS_OBJECT;
		Z_OBJ_HANDLE(stmt->lazy_object_ref) = zend_objects_store_put(stmt, (zend_objects_store_dtor_t)zend_objects_destroy_object, pdo_row_free_storage, NULL TSRMLS_CC);
		Z_OBJ_HT(stmt->lazy_object_ref) = &pdo_row_object_handlers;
		stmt->refcount++;
	}
	Z_TYPE_P(return_value) = IS_OBJECT;
	Z_OBJ_HANDLE_P(return_value) = Z_OBJ_HANDLE(stmt->lazy_object_ref);
	Z_OBJ_HT_P(return_value) = Z_OBJ_HT(stmt->lazy_object_ref);
}
/* }}} */

static void param_dtor(void *data) /* {{{ */
{
	struct pdo_bound_param_data *param = (struct pdo_bound_param_data *)data;
	TSRMLS_FETCH();

	/* tell the driver that it is going away */
	if (param->stmt->methods->param_hook) {
		param->stmt->methods->param_hook(param->stmt, param, PDO_PARAM_EVT_FREE TSRMLS_CC);
	}

	if (param->name) {
		efree(param->name);
	}

	zval_ptr_dtor(&(param->parameter));
	if (param->driver_params) {
		zval_ptr_dtor(&(param->driver_params));
	}
}
/* }}} */

static int really_register_bound_param(struct pdo_bound_param_data *param, pdo_stmt_t *stmt, int is_param TSRMLS_DC) /* {{{ */
{
	HashTable *hash;
	struct pdo_bound_param_data *pparam = NULL;

	hash = is_param ? stmt->bound_params : stmt->bound_columns;

	if (!hash) {
		ALLOC_HASHTABLE(hash);
		zend_hash_init(hash, 13, NULL, param_dtor, 0);

		if (is_param) {
			stmt->bound_params = hash;
		} else {
			stmt->bound_columns = hash;
		}
	}

	if (PDO_PARAM_TYPE(param->param_type) == PDO_PARAM_STR && param->max_value_len <= 0) {
		convert_to_string(param->parameter);
	}

	param->stmt = stmt;
	param->is_param = is_param;

	ZVAL_ADDREF(param->parameter);
	if (param->driver_params) {
		ZVAL_ADDREF(param->driver_params);
	}

	if (param->name && stmt->columns) {
		/* try to map the name to the column */
		int i;

		for (i = 0; i < stmt->column_count; i++) {
			if (strcmp(stmt->columns[i].name, param->name) == 0) {
				param->paramno = i;
				break;
			}
		}

#if 0
		/* if you prepare and then execute passing an array of params keyed by names,
		 * then this will trigger, and we don't want that */
		if (param->paramno == -1) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Did not found column name '%s' in the defined columns; it will not be bound", param->name);
		}
#endif
	}

	if (param->name) {
		param->name = estrndup(param->name, param->namelen);
		zend_hash_update(hash, param->name, param->namelen, param, sizeof(*param), (void**)&pparam);
	} else {
		zend_hash_index_update(hash, param->paramno, param, sizeof(*param), (void**)&pparam);
	}

	if (!rewrite_name_to_position(stmt, pparam TSRMLS_CC)) {
		return 0;
	}
	
	/* tell the driver we just created a parameter */
	if (stmt->methods->param_hook) {
		return stmt->methods->param_hook(stmt, pparam, PDO_PARAM_EVT_ALLOC TSRMLS_CC);
	}

	return 1;
}
/* }}} */

/* {{{ proto bool PDOStatement::execute([array $bound_input_params])
   Execute a prepared statement, optionally binding parameters */
static PHP_METHOD(PDOStatement, execute)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
	zval *input_params = NULL;
	int ret = 1;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a!", &input_params)) {
		RETURN_FALSE;
	}

	PDO_STMT_CLEAR_ERR();
	
	if (input_params) {
		struct pdo_bound_param_data param;
		zval **tmp;
		uint str_length;
		ulong num_index;

		zend_hash_internal_pointer_reset(Z_ARRVAL_P(input_params));
		while (SUCCESS == zend_hash_get_current_data(Z_ARRVAL_P(input_params), (void*)&tmp)) {
			memset(&param, 0, sizeof(param));

			if (HASH_KEY_IS_STRING == zend_hash_get_current_key_ex(Z_ARRVAL_P(input_params),
						&param.name, &str_length, &num_index, 0, NULL)) {
				/* yes this is correct.  we don't want to count the null byte.  ask wez */
				param.namelen = str_length - 1;
				param.paramno = -1;
			} else {
				/* we're okay to be zero based here */
				if (num_index < 0) {
					pdo_raise_impl_error(stmt->dbh, stmt, "HY093", NULL TSRMLS_CC);
					RETURN_FALSE;
				}
				param.paramno = num_index;
			}

			param.param_type = PDO_PARAM_STR;
			param.parameter = *tmp;

			if (!really_register_bound_param(&param, stmt, 1 TSRMLS_CC)) {
				RETURN_FALSE;
			}

			zend_hash_move_forward(Z_ARRVAL_P(input_params));
		}
	}

	if (PDO_PLACEHOLDER_NONE == stmt->supports_placeholders) {
		/* handle the emulated parameter binding,
         * stmt->active_query_string holds the query with binds expanded and 
		 * quoted.
         */

		ret = pdo_parse_params(stmt, stmt->query_string, stmt->query_stringlen,
			&stmt->active_query_string, &stmt->active_query_stringlen TSRMLS_CC);

		if (ret == 0) {
			/* no changes were made */
			stmt->active_query_string = stmt->query_string;
			stmt->active_query_stringlen = stmt->query_stringlen;
		} else if (ret == -1) {
			/* something broke */
			PDO_HANDLE_STMT_ERR();
			RETURN_FALSE;
		}
	} else if (!dispatch_param_event(stmt, PDO_PARAM_EVT_EXEC_PRE TSRMLS_CC)) {
		PDO_HANDLE_STMT_ERR();
		RETURN_FALSE;
	}
	if (stmt->methods->executer(stmt TSRMLS_CC)) {
		if (stmt->active_query_string && stmt->active_query_string != stmt->query_string) {
			efree(stmt->active_query_string);
		}
		stmt->active_query_string = NULL;
		if (!stmt->executed) {
			/* this is the first execute */

			if (stmt->dbh->alloc_own_columns) {
				/* for "big boy" drivers, we need to allocate memory to fetch
				 * the results into, so lets do that now */
				ret = pdo_stmt_describe_columns(stmt TSRMLS_CC);
			}

			stmt->executed = 1;
		}

		if (ret && !dispatch_param_event(stmt, PDO_PARAM_EVT_EXEC_POST TSRMLS_CC)) {
			RETURN_FALSE;
		}
			
		RETURN_BOOL(ret);
	}
	if (stmt->active_query_string && stmt->active_query_string != stmt->query_string) {
		efree(stmt->active_query_string);
	}
	stmt->active_query_string = NULL;
	PDO_HANDLE_STMT_ERR();
	RETURN_FALSE;
}
/* }}} */

static inline void fetch_value(pdo_stmt_t *stmt, zval *dest, int colno TSRMLS_DC) /* {{{ */
{
	struct pdo_column_data *col;
	char *value = NULL;
	unsigned long value_len = 0;
	int caller_frees = 0;

	col = &stmt->columns[colno];

	value = NULL;
	value_len = 0;

	stmt->methods->get_col(stmt, colno, &value, &value_len, &caller_frees TSRMLS_CC);

	switch (PDO_PARAM_TYPE(col->param_type)) {
		case PDO_PARAM_INT:
			if (value && value_len == sizeof(long)) {
				ZVAL_LONG(dest, *(long*)value);
				break;
			}
			ZVAL_NULL(dest);
			break;

		case PDO_PARAM_BOOL:
			if (value && value_len == sizeof(zend_bool)) {
				ZVAL_BOOL(dest, *(zend_bool*)value);
				break;
			}
			ZVAL_NULL(dest);
			break;

		case PDO_PARAM_LOB:
			if (value == NULL) {
				ZVAL_NULL(dest);
			} else if (value_len == 0) {
				php_stream_to_zval((php_stream*)value, dest);
			} else {
				/* they gave us a string, but LOBs are represented as streams in PDO */
				php_stream *stm;
#ifdef TEMP_STREAM_TAKE_BUFFER
				if (caller_frees) {
					stm = php_stream_memory_open(TEMP_STREAM_TAKE_BUFFER, value, value_len);
					if (stm) {
						caller_frees = 0;
					}
				} else
#endif
				{
					stm = php_stream_memory_open(TEMP_STREAM_READONLY, value, value_len);
				}
				if (stm) {
					php_stream_to_zval(stm, dest);
				} else {
					ZVAL_NULL(dest);
				}
			}
			break;
		
		case PDO_PARAM_STR:
			if (value && !(value_len == 0 && stmt->dbh->oracle_nulls)) {
				ZVAL_STRINGL(dest, value, value_len, !caller_frees);
				if (caller_frees) {
					caller_frees = 0;
				}
				break;
			}
		default:
			ZVAL_NULL(dest);
	}

	if (caller_frees && value) {
		efree(value);
	}
}
/* }}} */

static int do_fetch_common(pdo_stmt_t *stmt, enum pdo_fetch_orientation ori,
	long offset, int do_bind TSRMLS_DC) /* {{{ */
{
	if (!dispatch_param_event(stmt, PDO_PARAM_EVT_FETCH_PRE TSRMLS_CC)) {
		return 0;
	}

	if (!stmt->methods->fetcher(stmt, ori, offset TSRMLS_CC)) {
		return 0;
	}

	/* some drivers might need to describe the columns now */
	if (!stmt->columns && !pdo_stmt_describe_columns(stmt TSRMLS_CC)) {
		return 0;
	}
	
	if (!dispatch_param_event(stmt, PDO_PARAM_EVT_FETCH_POST TSRMLS_CC)) {
		return 0;
	}

	if (do_bind && stmt->bound_columns) {
		/* update those bound column variables now */
		struct pdo_bound_param_data *param;

		zend_hash_internal_pointer_reset(stmt->bound_columns);
		while (SUCCESS == zend_hash_get_current_data(stmt->bound_columns, (void**)&param)) {
			if (param->paramno >= 0) {
				convert_to_string(param->parameter);

				/* delete old value */
				zval_dtor(param->parameter);

				/* set new value */
				fetch_value(stmt, param->parameter, param->paramno TSRMLS_CC);

				/* TODO: some smart thing that avoids duplicating the value in the
				 * general loop below.  For now, if you're binding output columns,
				 * it's better to use LAZY or BOUND fetches if you want to shave
				 * off those cycles */
			}

			zend_hash_move_forward(stmt->bound_columns);
		}
	}

	return 1;
}
/* }}} */

static int do_fetch_class_prepare(pdo_stmt_t *stmt TSRMLS_DC) /* {{{ */
{
	zend_class_entry * ce = stmt->fetch.cls.ce;
	zend_fcall_info * fci = &stmt->fetch.cls.fci;
	zend_fcall_info_cache * fcc = &stmt->fetch.cls.fcc;

	fci->size = sizeof(zend_fcall_info);

	if (!ce) {
		stmt->fetch.cls.ce = ZEND_STANDARD_CLASS_DEF_PTR;
	}
	
	if (ce->constructor) {
		fci->function_table = &ce->function_table;
		fci->function_name = NULL;
		fci->symbol_table = NULL;
		fci->retval_ptr_ptr = &stmt->fetch.cls.retval_ptr;
		if (stmt->fetch.cls.ctor_args) {
			HashTable *ht = Z_ARRVAL_P(stmt->fetch.cls.ctor_args);
			Bucket *p;

			fci->param_count = 0;
			fci->params = safe_emalloc(sizeof(zval**), ht->nNumOfElements, 0);
			p = ht->pListHead;
			while (p != NULL) {
				fci->params[fci->param_count++] = (zval**)p->pData;
				p = p->pListNext;
			}
		} else {
			fci->param_count = 0;
			fci->params = NULL;
		}
		fci->no_separation = 1;

		fcc->initialized = 1;
		fcc->function_handler = ce->constructor;
		fcc->calling_scope = EG(scope);
		return 1;
	} else if (stmt->fetch.cls.ctor_args) {
		zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Class %s does not have a constructor, use NULL for parameter ctor_params or omit it", ce->name);
		return 0;
	} else {
		return 1; /* no ctor no args is also ok */
	}
}
/* }}} */

static int make_callable_ex(zval *callable, zend_fcall_info * fci, zend_fcall_info_cache * fcc, int num_args TSRMLS_DC) /* {{{ */
{
	zval **object = NULL, **method;
	char *fname, *cname;
	zend_class_entry * ce = NULL, **pce;
	zend_function *function_handler;
	
	if (Z_TYPE_P(callable) == IS_ARRAY) {
		if (Z_ARRVAL_P(callable)->nNumOfElements < 2) {
			zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Argument function must be a valid function name or an array specifying object or class and method name");
			return 0;
		}
		object = (zval**)Z_ARRVAL_P(callable)->pListHead->pData;
		method = (zval**)Z_ARRVAL_P(callable)->pListHead->pListNext->pData;

		if (Z_TYPE_PP(object) == IS_STRING) { /* static call */
			object = NULL;
		} else if (Z_TYPE_PP(object) == IS_OBJECT) { /* object call */
			ce = Z_OBJCE_PP(object);
		} else {
			zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Argument function may be an array but the provided array did neither contain a class name nor an object as first member");
			return 0;
		}
		
		if (Z_TYPE_PP(method) != IS_STRING) {
			zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Argument function may be an array but the provided array did not contain a method name as second member");
		}
	}
	
	if (!zend_is_callable(callable, 0, &fname)) {
		zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Argument function must contain something callable");
		return 0;
	}
	
	/* ATM we do not support array($obj, "CLASS::FUNC") or "CLASS_FUNC" */
	cname = fname;
	if ((fname = strstr(fname, "::")) == NULL) {
		fname = cname;
		cname = NULL;
	} else {
		*fname = '\0';
		fname += 2;
	}
	if (cname) {
		if (zend_lookup_class(cname, strlen(cname), &pce TSRMLS_CC) == FAILURE) {
			zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Argument function references non existing class %s", cname);
		} else {
			if (ce) {
				/* pce must be base of ce or ce itself */
				if (ce != *pce && !instanceof_function(ce, *pce TSRMLS_CC)) {
					zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Argument function would result in calling %s::%s but class %s is not base of %s", (*pce)->name, fname, (*pce)->name, ce->name);
				}
			}
			ce = *pce;
		}
	}
	
	fci->function_table = ce ? &ce->function_table : EG(function_table);
	if (zend_hash_find(fci->function_table, fname, strlen(fname)+1, (void **)&function_handler) == FAILURE) {
		zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Argument function '%s%s%s' not found", cname ? cname : "", cname ? "::" : "", fname);
		efree(cname ? cname : fname);
		return 0;
	}
	efree(cname ? cname : fname);

	fci->size = sizeof(zend_fcall_info);
	fci->function_name = NULL;
	fci->symbol_table = NULL;
	fci->param_count = num_args; /* probably less */
	fci->params = safe_emalloc(sizeof(zval**), num_args, 0);
	fci->object_pp = object;

	fcc->initialized = 1;
	fcc->function_handler = function_handler;
	fcc->calling_scope = EG(scope);
	fcc->object_pp = object;
	
	return 1;
}
/* }}} */

static int do_fetch_func_prepare(pdo_stmt_t *stmt TSRMLS_DC) /* {{{ */
{
	zend_fcall_info * fci = &stmt->fetch.cls.fci;
	zend_fcall_info_cache * fcc = &stmt->fetch.cls.fcc;

	if (!make_callable_ex(stmt->fetch.func.function, fci, fcc, stmt->column_count TSRMLS_CC)) {
		return 0;
	} else {
		stmt->fetch.func.values = safe_emalloc(sizeof(zval*), stmt->column_count, 0);
		return 1;
	}
}
/* }}} */

static int do_fetch_opt_finish(pdo_stmt_t *stmt, int free_ctor_agrs TSRMLS_DC) /* {{{ */
{
	/* fci.size is used to check if it is valid */
	if (stmt->fetch.cls.fci.size && stmt->fetch.cls.fci.params) {
		efree(stmt->fetch.cls.fci.params);
		stmt->fetch.cls.fci.params = NULL;
	}
	stmt->fetch.cls.fci.size = 0;
	if (stmt->fetch.cls.ctor_args && free_ctor_agrs) {
		zval_ptr_dtor(&stmt->fetch.cls.ctor_args);
		stmt->fetch.cls.ctor_args = NULL;
		stmt->fetch.cls.fci.param_count = 0;
	}
	if (stmt->fetch.func.values && free_ctor_agrs) {
		FREE_ZVAL(stmt->fetch.func.values);
		stmt->fetch.func.values = NULL;
	}
	return 1;
}
/* }}} */

/* perform a fetch.  If do_bind is true, update any bound columns.
 * If return_value is not null, store values into it according to HOW. */
static int do_fetch(pdo_stmt_t *stmt, int do_bind, zval *return_value,
	enum pdo_fetch_type how, enum pdo_fetch_orientation ori, long offset, zval *return_all TSRMLS_DC) /* {{{ */
{
	int flags = how & PDO_FETCH_FLAGS, idx, old_arg_count;
	zend_class_entry * ce, * old_ce;
	zval grp_val, *grp, **pgrp, *retval, *old_ctor_args;

	how = how & ~PDO_FETCH_FLAGS;
	if (how == PDO_FETCH_USE_DEFAULT) {
		how = stmt->default_fetch_type;
	}

	if (!do_fetch_common(stmt, ori, offset, do_bind TSRMLS_CC)) {
		return 0;
	}

	if (how == PDO_FETCH_BOUND) {
		RETVAL_TRUE;
		return 1;
	}

	if (return_value) {
		int i = 0;

		if (how == PDO_FETCH_LAZY) {
			get_lazy_object(stmt, return_value TSRMLS_CC);
			return 1;
		}

		switch (how) {
			case PDO_FETCH_ASSOC:
			case PDO_FETCH_BOTH:
			case PDO_FETCH_NUM:
				array_init(return_value);
				break;

			case PDO_FETCH_COLUMN:
				if (stmt->fetch.column >= 0 && stmt->fetch.column < stmt->column_count) {
					fetch_value(stmt, return_value, stmt->fetch.column TSRMLS_CC);
					if (!return_all) {
						return 1;
					} else {
						break;
					}
				}
				fprintf(stderr, "FAIL\n");
				return 0;

			case PDO_FETCH_OBJ:
				object_init_ex(return_value, ZEND_STANDARD_CLASS_DEF_PTR);
				break;

			case PDO_FETCH_CLASS:
				if (flags & PDO_FETCH_CLASSTYPE) {
					zval val;
					zend_class_entry **cep;

					old_ce = stmt->fetch.cls.ce;
					old_ctor_args = stmt->fetch.cls.ctor_args;
					old_arg_count = stmt->fetch.cls.fci.param_count;
					do_fetch_opt_finish(stmt, 0 TSRMLS_CC);

					INIT_PZVAL(&val);
					fetch_value(stmt, &val, i++ TSRMLS_CC);
					if (Z_TYPE(val) != IS_NULL) {
						convert_to_string(&val);
						if (zend_lookup_class(Z_STRVAL(val), Z_STRLEN(val), &cep TSRMLS_CC) == FAILURE) {
							stmt->fetch.cls.ce = ZEND_STANDARD_CLASS_DEF_PTR;
						} else {
							stmt->fetch.cls.ce = *cep;
						}
					}

					do_fetch_class_prepare(stmt TSRMLS_CC);
					zval_dtor(&val);
				}
				ce = stmt->fetch.cls.ce;
				if ((flags & PDO_FETCH_SERIALIZE) == 0) {
					object_init_ex(return_value, ce);
					if (!stmt->fetch.cls.fci.size) {
						if (!do_fetch_class_prepare(stmt TSRMLS_CC))
						{
							return 0;
						}
					}
				}
				break;
			
			case PDO_FETCH_INTO:
				Z_TYPE_P(return_value) = IS_OBJECT;
				Z_OBJ_HANDLE_P(return_value) = Z_OBJ_HANDLE_P(stmt->fetch.into);
				Z_OBJ_HT_P(return_value) = Z_OBJ_HT_P(stmt->fetch.into);
				zend_objects_store_add_ref(stmt->fetch.into TSRMLS_CC);

				if (zend_get_class_entry(return_value TSRMLS_CC) == ZEND_STANDARD_CLASS_DEF_PTR) {
					how = PDO_FETCH_OBJ;
				}
				break;

			case PDO_FETCH_FUNC:
				if (!stmt->fetch.func.fci.size) {
					if (!do_fetch_func_prepare(stmt TSRMLS_CC))
					{
						return 0;
					}
				}
				break;
				

			default:
				/* shouldn't happen */
				return 0;
		}
		
		if (return_all) {
			INIT_PZVAL(&grp_val);
			fetch_value(stmt, &grp_val, i TSRMLS_CC);
			convert_to_string(&grp_val);
			if (how == PDO_FETCH_COLUMN) {
				i = stmt->column_count; /* no more data to fetch */
			} else {
				i++;
			}
		}

		for (idx = 0; i < stmt->column_count; i++, idx++) {
			zval *val;
			MAKE_STD_ZVAL(val);
			fetch_value(stmt, val, i TSRMLS_CC);

			switch (how) {
				case PDO_FETCH_ASSOC:
					add_assoc_zval(return_value, stmt->columns[i].name, val);
					break;

				case PDO_FETCH_BOTH:
					add_assoc_zval(return_value, stmt->columns[i].name, val);
					ZVAL_ADDREF(val);
					add_next_index_zval(return_value, val);
					break;

				case PDO_FETCH_NUM:
					add_next_index_zval(return_value, val);
					break;

				case PDO_FETCH_OBJ:
				case PDO_FETCH_INTO:
					zend_update_property(NULL, return_value,
						stmt->columns[i].name, stmt->columns[i].namelen,
						val TSRMLS_CC);
					break;

				case PDO_FETCH_CLASS:
					if ((flags & PDO_FETCH_SERIALIZE) == 0 || idx) {
						zend_update_property(ce, return_value,
							stmt->columns[i].name, stmt->columns[i].namelen,
							val TSRMLS_CC);
					} else {
#ifdef MBO_0
						php_unserialize_data_t var_hash;

						PHP_VAR_UNSERIALIZE_INIT(var_hash);
						if (php_var_unserialize(&return_value, (const unsigned char**)&Z_STRVAL_P(val), Z_STRVAL_P(val)+Z_STRLEN_P(val), NULL TSRMLS_CC) == FAILURE) {
							zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Cannot unserialize data");
							PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
							return 0;
						}
						PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
#else
						if (!ce->unserialize) {
							zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Class %s cannot be unserialized", ce->name);
							return 0;
						} else if (ce->unserialize(&return_value, ce, Z_TYPE_P(val) == IS_STRING ? Z_STRVAL_P(val) : "", Z_TYPE_P(val) == IS_STRING ? Z_STRLEN_P(val) : 0, NULL TSRMLS_CC) == FAILURE) {
							zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Class %s cannot be unserialized", ce->name);
							zval_dtor(return_value);
							ZVAL_NULL(return_value);
						}
#endif
					}
					break;
				
				case PDO_FETCH_FUNC:
					stmt->fetch.func.values[idx] = val;
					stmt->fetch.cls.fci.params[idx] = &stmt->fetch.func.values[idx];
					break;
				
				default:
					zval_ptr_dtor(&val);
					pdo_raise_impl_error(stmt->dbh, stmt, "22003", "mode is out of range" TSRMLS_CC);
					return 0;
					break;
			}
		}
		
		switch (how) {
			case PDO_FETCH_CLASS:
				if (ce->constructor) {
					stmt->fetch.cls.fci.object_pp = &return_value;
					stmt->fetch.cls.fcc.object_pp = &return_value;
					if (zend_call_function(&stmt->fetch.cls.fci, &stmt->fetch.cls.fcc TSRMLS_CC) == FAILURE) {
						zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Could not execute %s::%s()", ce->name, ce->constructor->common.function_name);
						return 0;
					} else {
						if (stmt->fetch.cls.retval_ptr) {
							zval_ptr_dtor(&stmt->fetch.cls.retval_ptr);
						}
					}
				}
				if (flags & PDO_FETCH_CLASSTYPE) {
					do_fetch_opt_finish(stmt, 0 TSRMLS_CC);
					stmt->fetch.cls.ce = old_ce;
					stmt->fetch.cls.ctor_args = old_ctor_args;
					stmt->fetch.cls.fci.param_count = old_arg_count;
				}
				break;

			case PDO_FETCH_FUNC:
				stmt->fetch.func.fci.param_count = idx;
				stmt->fetch.func.fci.retval_ptr_ptr = &retval;
				if (zend_call_function(&stmt->fetch.func.fci, &stmt->fetch.func.fcc TSRMLS_CC) == FAILURE) {
					zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Could not execute %s%s%s()", ce->name, ce->constructor->common.function_name);
					return 0;
				} else {
					if (return_all) {
						zval_ptr_dtor(&return_value); /* we don't need that */
						return_value = retval;
					} else {
						*return_value = *retval;
						zval_copy_ctor(return_value);
						zval_ptr_dtor(&retval);
					}
				}
				while(idx--) {
					zval_ptr_dtor(&stmt->fetch.func.values[idx]);
				}
				break;
			
			default:
				break;
		}

		if (return_all) {
			if ((flags & PDO_FETCH_UNIQUE) == PDO_FETCH_UNIQUE) {
				add_assoc_zval(return_all, Z_STRVAL(grp_val), return_value);
			} else {
				if (zend_symtable_find(Z_ARRVAL_P(return_all), Z_STRVAL(grp_val), Z_STRLEN(grp_val)+1, (void**)&pgrp) == FAILURE) {
					MAKE_STD_ZVAL(grp);
					array_init(grp);
					add_assoc_zval(return_all, Z_STRVAL(grp_val), grp);
				} else {
					grp = *pgrp;
				}
				add_next_index_zval(grp, return_value);
			}
			zval_dtor(&grp_val);
		}

	}

	return 1;
}
/* }}} */

static int pdo_stmt_verify_mode(pdo_stmt_t *stmt, int mode, int fetch_all TSRMLS_DC) /* {{{ */
{
	int flags = mode & PDO_FETCH_FLAGS;
	
	mode = mode & ~PDO_FETCH_FLAGS;

	if (mode == PDO_FETCH_USE_DEFAULT) {
		mode = stmt->default_fetch_type;
	}

	switch(mode) {
	case PDO_FETCH_FUNC:
		if (!fetch_all) {
			zend_throw_exception(pdo_exception_ce, "Fetch mode PDO_FETCH_FUNC is only allowed in PDOStatement::fetchAll()", 0 TSRMLS_CC);
			return 0;
		}
		return 1;
	
	default:
		if ((flags & PDO_FETCH_SERIALIZE) == PDO_FETCH_SERIALIZE) {
			zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Fetch mode flag PDO_FETCH_SERIALIZE requires mode PDO_FETCH_CLASS", mode);
			return 0;
		}
		if ((flags & PDO_FETCH_CLASSTYPE) == PDO_FETCH_CLASSTYPE) {
			zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Fetch mode flag PDO_FETCH_CLASSTYPE requires mode PDO_FETCH_CLASS", mode);
			return 0;
		}
		if (mode >= PDO_FETCH__MAX) {
			zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Fetch mode %d is invalid", mode);
			return 0;
		}
		/* no break; */

	case PDO_FETCH_CLASS:
		return 1;
	}
}
/* }}} */

/* {{{ proto mixed PDOStatement::fetch([int $how = PDO_FETCH_BOTH [, int $orientation [, int $offset]]])
   Fetches the next row and returns it, or false if there are no more rows */
static PHP_METHOD(PDOStatement, fetch)
{
	long how = PDO_FETCH_USE_DEFAULT;
	long ori = PDO_FETCH_ORI_NEXT;
	long off = 0;
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|lll", &how,
			&ori, &off)) {
		RETURN_FALSE;
	}

	PDO_STMT_CLEAR_ERR();

	if (!pdo_stmt_verify_mode(stmt, how, 0 TSRMLS_CC)) {
		RETURN_FALSE;
	}

	if (!do_fetch(stmt, TRUE, return_value, how, ori, off, 0 TSRMLS_CC)) {
		PDO_HANDLE_STMT_ERR();
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto mixed PDOStatement::fetchObject(string class_name [, NULL|array ctor_args])
   Fetches the next row and returns it as an object. */
static PHP_METHOD(PDOStatement, fetchObject)
{
	long how = PDO_FETCH_CLASS;
	long ori = PDO_FETCH_ORI_NEXT;
	long off = 0;
	char *class_name;
	int class_name_len;
	zend_class_entry *old_ce;
	zval *old_ctor_args, *ctor_args;
	int error = 0, old_arg_count;

	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz", 
		&class_name, &class_name_len, &ctor_args)) {
		RETURN_FALSE;
	}

	PDO_STMT_CLEAR_ERR();

	if (!pdo_stmt_verify_mode(stmt, how, 0 TSRMLS_CC)) {
		RETURN_FALSE;
	}

	old_ce = stmt->fetch.cls.ce;
	old_ctor_args = stmt->fetch.cls.ctor_args;
	old_arg_count = stmt->fetch.cls.fci.param_count;
	
	do_fetch_opt_finish(stmt, 0 TSRMLS_CC);
	
	switch(ZEND_NUM_ARGS()) {
	case 0:
		stmt->fetch.cls.ce = zend_standard_class_def;
		break;
	case 2:
		if (Z_TYPE_P(ctor_args) != IS_NULL && Z_TYPE_P(ctor_args) != IS_ARRAY) {
			zend_throw_exception(pdo_exception_ce, "Parameter ctor_args must either be NULL or an array ", 0 TSRMLS_CC);
			error = 1;
			break;
		}
		if (Z_TYPE_P(ctor_args) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(ctor_args))) {
			ALLOC_ZVAL(stmt->fetch.cls.ctor_args);
			*stmt->fetch.cls.ctor_args = *ctor_args;
			zval_copy_ctor(stmt->fetch.cls.ctor_args);
		} else {
			stmt->fetch.cls.ctor_args = NULL;
		}
		/* no break */
	case 1:
		stmt->fetch.cls.ce = zend_fetch_class(class_name, class_name_len, ZEND_FETCH_CLASS_AUTO TSRMLS_CC);

		if (!stmt->fetch.cls.ce) {
			zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Could not find class '%s'", class_name);
			error = 1;
			break;
		}
	}

	if (!error && !do_fetch(stmt, TRUE, return_value, how, ori, off, 0 TSRMLS_CC)) {
		error = 1;
		PDO_HANDLE_STMT_ERR();
	}
	do_fetch_opt_finish(stmt, 1 TSRMLS_CC);

	stmt->fetch.cls.ce = old_ce;
	stmt->fetch.cls.ctor_args = old_ctor_args;
	stmt->fetch.cls.fci.param_count = old_arg_count;
	if (error) {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string PDOStatement::fetchSingle()
   Returns a data of the 1st column in the result set. */
static PHP_METHOD(PDOStatement, fetchSingle)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (ZEND_NUM_ARGS()) {
		RETURN_FALSE;
	}

	PDO_STMT_CLEAR_ERR();

	if (!do_fetch_common(stmt, PDO_FETCH_ORI_NEXT, 0, TRUE TSRMLS_CC)) {
		PDO_HANDLE_STMT_ERR();
		RETURN_FALSE;
	}

	fetch_value(stmt, return_value, 0 TSRMLS_CC);
}
/* }}} */

/* {{{ proto array PDOStatement::fetchAll([int $how = PDO_FETCH_BOTH [, string class_name [, NULL|array ctor_args]]])
   Returns an array of all of the results. */
static PHP_METHOD(PDOStatement, fetchAll)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
	long how = PDO_FETCH_USE_DEFAULT;
	zval *data, *return_all;
	zval *arg2;
	zend_class_entry *old_ce;
	zval *old_ctor_args, *ctor_args = NULL;
	int error = 0, old_arg_count;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|lzz", &how, &arg2, &ctor_args)) {
		RETURN_FALSE;
	}

	if (!pdo_stmt_verify_mode(stmt, how, 1 TSRMLS_CC)) {
		RETURN_FALSE;
	}

	old_ce = stmt->fetch.cls.ce;
	old_ctor_args = stmt->fetch.cls.ctor_args;
	old_arg_count = stmt->fetch.cls.fci.param_count;

	do_fetch_opt_finish(stmt, 0 TSRMLS_CC);

	switch(how & ~PDO_FETCH_FLAGS) {
	case PDO_FETCH_CLASS:
		switch(ZEND_NUM_ARGS()) {
		case 0:
		case 1:
			stmt->fetch.cls.ce = zend_standard_class_def;
			break;
		case 3:
			if (Z_TYPE_P(ctor_args) != IS_NULL && Z_TYPE_P(ctor_args) != IS_ARRAY) {
				zend_throw_exception(pdo_exception_ce, "Parameter ctor_args must either be NULL or an array ", 0 TSRMLS_CC);
				error = 1;
				break;
			}
			if (Z_TYPE_P(ctor_args) != IS_ARRAY || !zend_hash_num_elements(Z_ARRVAL_P(ctor_args))) {
				ctor_args = NULL;
			}
			/* no break */
		case 2:
			stmt->fetch.cls.ctor_args = ctor_args; /* we're not going to free these */
			if (Z_TYPE_P(arg2) != IS_STRING) {
				zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "In fetch mode PDO_FETCH_CLASS the 2nd parameter must be a class name string", Z_TYPE_P(arg2));
				error = 1;
				break;
			} else {
				stmt->fetch.cls.ce = zend_fetch_class(Z_STRVAL_P(arg2), Z_STRLEN_P(arg2), ZEND_FETCH_CLASS_AUTO TSRMLS_CC);
				if (!stmt->fetch.cls.ce) {
					zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Could not find class '%s'", Z_TYPE_P(arg2));
					error = 1;
					break;
				}
			}
		}
		do_fetch_class_prepare(stmt TSRMLS_CC);
		break;

	case PDO_FETCH_FUNC:
		switch(ZEND_NUM_ARGS()) {
		case 0:
		case 1:
			break;
		case 3:
		case 2:
			stmt->fetch.func.function = arg2;
			break;
		}
		do_fetch_func_prepare(stmt TSRMLS_CC);
		break;
	
	case PDO_FETCH_COLUMN:
		switch(ZEND_NUM_ARGS()) {
		case 0:
		case 1:
			stmt->fetch.column = how & PDO_FETCH_GROUP ? 1 : 0;
			break;
		case 2:
			convert_to_long(arg2);
			stmt->fetch.column = Z_LVAL_P(arg2);
			break;
		case 3:
			zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Third parameter not allowed for specified fetch mode");
			error = 1;
		}
		break;

	default:
		if (ZEND_NUM_ARGS() > 1) {
			zend_throw_exception_ex(pdo_exception_ce, 0 TSRMLS_CC, "Additional parameters not allowed for specified fetch mode");
			error = 1;
		}
	}

	if (!error)	{
		PDO_STMT_CLEAR_ERR();
		MAKE_STD_ZVAL(data);
		if (how & PDO_FETCH_GROUP) {
			array_init(return_value);
			return_all = return_value;
		} else {
			return_all = 0;
		}
		if (!do_fetch(stmt, TRUE, data, how, PDO_FETCH_ORI_NEXT, 0, return_all TSRMLS_CC)) {
			FREE_ZVAL(data);
			PDO_HANDLE_STMT_ERR();
			zval_dtor(return_value);
			error = 1;
		}
	}
	if (!error) {
		if ((how & PDO_FETCH_GROUP)) {
			do {
				MAKE_STD_ZVAL(data);
			} while (do_fetch(stmt, TRUE, data, how, PDO_FETCH_ORI_NEXT, 0, return_all TSRMLS_CC));
		} else {
			array_init(return_value);
			do {
				add_next_index_zval(return_value, data);
				MAKE_STD_ZVAL(data);
			} while (do_fetch(stmt, TRUE, data, how, PDO_FETCH_ORI_NEXT, 0, 0 TSRMLS_CC));
		}
		FREE_ZVAL(data);
	}
	
	do_fetch_opt_finish(stmt, 0 TSRMLS_CC);

	stmt->fetch.cls.ce = old_ce;
	stmt->fetch.cls.ctor_args = old_ctor_args;
	stmt->fetch.cls.fci.param_count = old_arg_count;
	
	if (error) {
		RETURN_FALSE;
	}
}
/* }}} */

static int register_bound_param(INTERNAL_FUNCTION_PARAMETERS, pdo_stmt_t *stmt, int is_param) /* {{{ */
{
	struct pdo_bound_param_data param = {0};

	param.paramno = -1;
	param.param_type = PDO_PARAM_STR;

	if (FAILURE == zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC,
			"lz|llz!", &param.paramno, &param.parameter, &param.param_type, &param.max_value_len,
			&param.driver_params)) {
		if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|llz!", &param.name,
				&param.namelen, &param.parameter, &param.param_type, &param.max_value_len, 
				&param.driver_params)) {
			return 0;
		}	
	}

	if (param.paramno > 0) {
		--param.paramno; /* make it zero-based internally */
	} else if (!param.name) {
		pdo_raise_impl_error(stmt->dbh, stmt, "HY093", "Columns/Parameters are 1-based" TSRMLS_CC);
		return 0;
	}

	return really_register_bound_param(&param, stmt, is_param TSRMLS_CC);
} /* }}} */

/* {{{ proto bool PDOStatement::bindParam(mixed $paramno, mixed &$param [, int $type [, int $maxlen [, mixed $driverdata]]])
   bind a parameter to a PHP variable.  $paramno is the 1-based position of the placeholder in the SQL statement (but can be the parameter name for drivers that support named placeholders).  This isn't supported by all drivers.  It should be called prior to execute(). */
static PHP_METHOD(PDOStatement, bindParam)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_BOOL(register_bound_param(INTERNAL_FUNCTION_PARAM_PASSTHRU, stmt, TRUE));
}
/* }}} */

/* {{{ proto bool PDOStatement::bindColumn(mixed $column, mixed &$param [, int $type [, int $maxlen [, mixed $driverdata]]])
   bind a column to a PHP variable.  On each row fetch $param will contain the value of the corresponding column.  $column is the 1-based offset of the column, or the column name.  For portability, don't call this before execute(). */
static PHP_METHOD(PDOStatement, bindColumn)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_BOOL(register_bound_param(INTERNAL_FUNCTION_PARAM_PASSTHRU, stmt, FALSE));
}
/* }}} */

/* {{{ proto int PDOStatement::rowCount()
   Returns the number of rows in a result set, or the number of rows affected by the last execute().  It is not always meaningful. */
static PHP_METHOD(PDOStatement, rowCount)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_LONG(stmt->row_count);
}
/* }}} */

/* {{{ proto string PDOStatement::errorCode()
   Fetch the error code associated with the last operation on the statement handle */
static PHP_METHOD(PDOStatement, errorCode)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (ZEND_NUM_ARGS()) {
		RETURN_FALSE;
	}

	RETURN_STRING(stmt->error_code, 1);
}
/* }}} */

/* {{{ proto array PDOStatement::errorInfo()
   Fetch extended error information associated with the last operation on the statement handle */
static PHP_METHOD(PDOStatement, errorInfo)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (ZEND_NUM_ARGS()) {
		RETURN_FALSE;
	}

	array_init(return_value);
	add_next_index_string(return_value, stmt->error_code, 1);

	if (stmt->dbh->methods->fetch_err) {
		stmt->dbh->methods->fetch_err(stmt->dbh, stmt, return_value TSRMLS_CC);
	}
}
/* }}} */

/* {{{ proto bool PDOStatement::setAttribute(long attribute, mixed value)
   Set an attribute */
static PHP_METHOD(PDOStatement, setAttribute)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
	long attr;
	zval *value = NULL;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz!", &attr, &value)) {
		RETURN_FALSE;
	}

	if (!stmt->methods->set_attribute) {
		goto fail;
	}

	PDO_STMT_CLEAR_ERR();
	if (stmt->methods->set_attribute(stmt, attr, value TSRMLS_CC)) {
		RETURN_TRUE;
	}

fail:
	if (!stmt->methods->set_attribute) {
		pdo_raise_impl_error(stmt->dbh, stmt, "IM001", "This driver doesn't support setting attributes" TSRMLS_CC);
	} else {
		PDO_HANDLE_STMT_ERR();
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto mixed PDOStatement::getAttribute(long attribute)
   Get an attribute */
static PHP_METHOD(PDOStatement, getAttribute)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
	long attr;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &attr)) {
		RETURN_FALSE;
	}

	if (!stmt->methods->get_attribute) {
		pdo_raise_impl_error(stmt->dbh, stmt, "IM001", "This driver doesn't support getting attributes" TSRMLS_CC);
		RETURN_FALSE;
	}

	PDO_STMT_CLEAR_ERR();
	switch (stmt->methods->get_attribute(stmt, attr, return_value TSRMLS_CC)) {
		case -1:
			PDO_HANDLE_STMT_ERR();
			RETURN_FALSE;

		case 0:
			/* XXX: should do something better here */
			pdo_raise_impl_error(stmt->dbh, stmt, "IM001", "driver doesn't support getting that attribute" TSRMLS_CC);
			RETURN_FALSE;

		default:
			return;
	}
}
/* }}} */

/* {{{ proto int PDOStatement::columnCount()
   Returns the number of columns in the result set */
static PHP_METHOD(PDOStatement, columnCount)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
	if (ZEND_NUM_ARGS()) {
		RETURN_FALSE;
	}
	RETURN_LONG(stmt->column_count);
}
/* }}} */

/* {{{ proto array PDOStatement::getColumnMeta(int $column)
   Returns meta data for a numbered column */
static PHP_METHOD(PDOStatement, getColumnMeta)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);
	long colno;
	struct pdo_column_data *col;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &colno)) {
		RETURN_FALSE;
	}

	if (!stmt->methods->get_column_meta) {
		pdo_raise_impl_error(stmt->dbh, stmt, "IM001", "driver doesn't support meta data" TSRMLS_CC);
		RETURN_FALSE;
	}

	PDO_STMT_CLEAR_ERR();
	if (FAILURE == stmt->methods->get_column_meta(stmt, colno, return_value TSRMLS_CC)) {
		PDO_HANDLE_STMT_ERR();
		RETURN_FALSE;
	}

	/* add stock items */
	col = &stmt->columns[colno];
	add_assoc_string(return_value, "name", col->name, 1);
	add_assoc_long(return_value, "len", col->maxlen); /* FIXME: unsigned ? */
	add_assoc_long(return_value, "precision", col->precision);
	add_assoc_long(return_value, "pdo_type", col->param_type);
}
/* }}} */

/* {{{ proto bool PDOStatement::setFetchMode(int mode [mixed* params])
   Changes the default fetch mode for subsequent fetches (params have different meaning for different fetch modes) */

int pdo_stmt_setup_fetch_mode(INTERNAL_FUNCTION_PARAMETERS, pdo_stmt_t *stmt, int skip)
{
	long mode = PDO_FETCH_BOTH;
	int argc = ZEND_NUM_ARGS() - skip;
	zval ***args;
	zend_class_entry **cep;
	
	do_fetch_opt_finish(stmt, 1 TSRMLS_CC);

	switch (stmt->default_fetch_type) {
		case PDO_FETCH_INTO:
			if (stmt->fetch.into) {
				ZVAL_DELREF(stmt->fetch.into);
				stmt->fetch.into = NULL;
			}
			break;
		default:
			;
	}
	
	stmt->default_fetch_type = PDO_FETCH_BOTH;

	if (argc == 0) {
		return SUCCESS;
	}

	args = safe_emalloc(ZEND_NUM_ARGS(), sizeof(zval*), 0);

	if (FAILURE == zend_get_parameters_array_ex(ZEND_NUM_ARGS(), args)) {
fail_out:
		efree(args);
		return FAILURE;
	}
	
	convert_to_long_ex(args[skip]);
	mode = Z_LVAL_PP(args[skip]);
	
	if (!pdo_stmt_verify_mode(stmt, mode, 0 TSRMLS_CC)) {
		return FAILURE;
	}

	switch (mode & ~PDO_FETCH_FLAGS) {
		case PDO_FETCH_LAZY:
		case PDO_FETCH_ASSOC:
		case PDO_FETCH_NUM:
		case PDO_FETCH_BOTH:
		case PDO_FETCH_OBJ:
		case PDO_FETCH_BOUND:
			break;

		case PDO_FETCH_COLUMN:
			if (argc != 2) {
				goto fail_out;
			}
			convert_to_long_ex(args[skip+1]);
			stmt->fetch.column = Z_LVAL_PP(args[skip+1]);
			break;

		case PDO_FETCH_CLASS:
			if (argc < 2 || argc > 3) {
				goto fail_out;
			}
			convert_to_string_ex(args[skip+1]);

			if (FAILURE == zend_lookup_class(Z_STRVAL_PP(args[skip+1]),
					Z_STRLEN_PP(args[skip+1]), &cep TSRMLS_CC)) {
				goto fail_out;
			}
				
			if (!cep || !*cep) {
				goto fail_out;
			}
			
			stmt->fetch.cls.ce = *cep;
			stmt->fetch.cls.ctor_args = NULL;

			if (stmt->dbh->is_persistent) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "PHP might crash if you don't call $stmt->setFetchMode() to reset to defaults on this persistent statement.  This will be fixed in a later release");
			}
			
			if (argc == 3) {
				if (Z_TYPE_PP(args[skip+2]) != IS_NULL && Z_TYPE_PP(args[skip+2]) != IS_ARRAY) {
					zend_throw_exception(pdo_exception_ce, "Parameter ctor_args must either be NULL or an array ", 0 TSRMLS_CC);
				} else if (Z_TYPE_PP(args[skip+2]) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_PP(args[skip+2]))) {
					ALLOC_ZVAL(stmt->fetch.cls.ctor_args);
					*stmt->fetch.cls.ctor_args = **args[skip+2];
					zval_copy_ctor(stmt->fetch.cls.ctor_args);
				}
			}
			
			do_fetch_class_prepare(stmt TSRMLS_CC);
			break;

		case PDO_FETCH_INTO:
			if (argc != 2) {
				goto fail_out;
			}
			if (Z_TYPE_PP(args[skip+1]) != IS_OBJECT) {
				goto fail_out;
			}

			if (stmt->dbh->is_persistent) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "PHP might crash if you don't call $stmt->setFetchMode() to reset to defaults on this persistent statement.  This will be fixed in a later release");
			}
	
			MAKE_STD_ZVAL(stmt->fetch.into);

			Z_TYPE_P(stmt->fetch.into) = IS_OBJECT;
			Z_OBJ_HANDLE_P(stmt->fetch.into) = Z_OBJ_HANDLE_PP(args[skip+1]);
			Z_OBJ_HT_P(stmt->fetch.into) = Z_OBJ_HT_PP(args[skip+1]);
			zend_objects_store_add_ref(stmt->fetch.into TSRMLS_CC);
			break;
		
		default:
			pdo_raise_impl_error(stmt->dbh, stmt, "22003", "mode is out of range" TSRMLS_CC);
			return FAILURE;
	}

	stmt->default_fetch_type = mode;
	efree(args);

	return SUCCESS;
}
   
static PHP_METHOD(PDOStatement, setFetchMode)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

	RETVAL_BOOL(
		pdo_stmt_setup_fetch_mode(INTERNAL_FUNCTION_PARAM_PASSTHRU,
			stmt, 0) == SUCCESS ? 1 : 0
		);
}
/* }}} */

/* {{{ proto bool PDOStatement::nextRowset()
   Advances to the next rowset in a multi-rowset statement handle. Returns true if it succeded, false otherwise */
static PHP_METHOD(PDOStatement, nextRowset)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!stmt->methods->next_rowset) {
		pdo_raise_impl_error(stmt->dbh, stmt, "IM001", "driver does not support multiple rowsets" TSRMLS_CC);
		RETURN_FALSE;
	}

	PDO_STMT_CLEAR_ERR();

	/* un-describe */
	if (stmt->columns) {
		int i;
		struct pdo_column_data *cols = stmt->columns;
		
		for (i = 0; i < stmt->column_count; i++) {
			efree(cols[i].name);
		}
		efree(stmt->columns);
	}
	stmt->columns = NULL;

	if (!stmt->methods->next_rowset(stmt TSRMLS_CC)) {
		stmt->column_count = 0;
		PDO_HANDLE_STMT_ERR();
		RETURN_FALSE;
	}
	
	pdo_stmt_describe_columns(stmt TSRMLS_CC);

	RETURN_TRUE;
}
/* }}} */

function_entry pdo_dbstmt_functions[] = {
	PHP_ME(PDOStatement, execute,		NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, fetch,			NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, bindParam,		second_arg_force_ref,	ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, bindColumn,	second_arg_force_ref,	ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, rowCount,		NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, fetchSingle,	NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, fetchAll,		NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, fetchObject,	NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, errorCode,		NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, errorInfo,		NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, setAttribute,	NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, getAttribute,	NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, columnCount,	NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, getColumnMeta,	NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, setFetchMode,	NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDOStatement, nextRowset,	NULL,					ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

/* {{{ overloaded handlers for PDOStatement class */
static zval *dbstmt_prop_read(zval *object, zval *member, int type TSRMLS_DC)
{
	zval *return_value;
	pdo_stmt_t * stmt = (pdo_stmt_t *) zend_object_store_get_object(object TSRMLS_CC);

	convert_to_string(member);

	if(strcmp(Z_STRVAL_P(member), "queryString") == 0) {
		MAKE_STD_ZVAL(return_value);
		ZVAL_STRINGL(return_value, stmt->query_string, stmt->query_stringlen, 1);
	} else {
		MAKE_STD_ZVAL(return_value);
		ZVAL_NULL(return_value);
	}
	return return_value;
}

static void dbstmt_prop_write(zval *object, zval *member, zval *value TSRMLS_DC)
{
	return;
}

static zval *dbstmt_read_dim(zval *object, zval *offset, int type TSRMLS_DC)
{
	zval *return_value;

	MAKE_STD_ZVAL(return_value);
	ZVAL_NULL(return_value);

	return return_value;
}

static void dbstmt_write_dim(zval *object, zval *offset, zval *value TSRMLS_DC)
{
	return;
}

static int dbstmt_prop_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
{
	return 0;
}

static int dbstmt_dim_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
{
	return 0;
}

static void dbstmt_prop_delete(zval *object, zval *offset TSRMLS_DC)
{
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot delete properties from a PDOStatement");
}

static void dbstmt_dim_delete(zval *object, zval *offset TSRMLS_DC)
{
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot delete dimensions from a PDOStatement");
}

static HashTable *dbstmt_get_properties(zval *object TSRMLS_DC)
{
	return NULL;
}

static union _zend_function *dbstmt_method_get(
#if PHP_API_VERSION >= 20041225
	zval **object_pp,
#else
	zval *object,
#endif
   	char *method_name, int method_len TSRMLS_DC)
{
	zend_function *fbc = NULL;
	char *lc_method_name;
#if PHP_API_VERSION >= 20041225
	zval *object = *object_pp;
#endif

	lc_method_name = emalloc(method_len + 1);
	zend_str_tolower_copy(lc_method_name, method_name, method_len);

	if (zend_hash_find(&pdo_dbstmt_ce->function_table, lc_method_name, 
			method_len+1, (void**)&fbc) == FAILURE) {
		pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(object TSRMLS_CC);
		/* not a pre-defined method, nor a user-defined method; check
		 * the driver specific methods */
		if (!stmt->dbh->cls_methods[PDO_DBH_DRIVER_METHOD_KIND_STMT]) {
			if (!pdo_hash_methods(stmt->dbh, 
				PDO_DBH_DRIVER_METHOD_KIND_STMT TSRMLS_CC)
				|| !stmt->dbh->cls_methods[PDO_DBH_DRIVER_METHOD_KIND_STMT]) {
				goto out;
			}
		}

		if (zend_hash_find(stmt->dbh->cls_methods[PDO_DBH_DRIVER_METHOD_KIND_STMT],
				lc_method_name, method_len+1, (void**)&fbc) == FAILURE) {
			fbc = NULL;
			goto out;
		}
		/* got it */
	}
	
out:
	efree(lc_method_name);
	return fbc;
}

static int dbstmt_call_method(char *method, INTERNAL_FUNCTION_PARAMETERS)
{
	return FAILURE;
}

static union _zend_function *dbstmt_get_ctor(zval *object TSRMLS_DC)
{
	return std_object_handlers.get_constructor(object TSRMLS_CC);
}

static zend_class_entry *dbstmt_get_ce(zval *object TSRMLS_DC)
{
	return std_object_handlers.get_class_entry(object TSRMLS_CC);
}

static int dbstmt_get_classname(zval *object, char **class_name, zend_uint *class_name_len, int parent TSRMLS_DC)
{
	return std_object_handlers.get_class_name(object, class_name, class_name_len, parent TSRMLS_CC);
}

static int dbstmt_compare(zval *object1, zval *object2 TSRMLS_DC)
{
	return -1;
}

zend_object_handlers pdo_dbstmt_object_handlers = {
	ZEND_OBJECTS_STORE_HANDLERS,
	dbstmt_prop_read,
	dbstmt_prop_write,
	dbstmt_read_dim,
	dbstmt_write_dim,
	NULL,
	NULL,
	NULL,
	dbstmt_prop_exists,
	dbstmt_prop_delete,
	dbstmt_dim_exists,
	dbstmt_dim_delete,
	dbstmt_get_properties,
	dbstmt_method_get,
	dbstmt_call_method,
	dbstmt_get_ctor,
	dbstmt_get_ce,
	dbstmt_get_classname,
	dbstmt_compare,
	NULL, /* cast */
	NULL
};

static void free_statement(pdo_stmt_t *stmt TSRMLS_DC)
{
	if (stmt->properties) {
		zend_hash_destroy(stmt->properties);
		efree(stmt->properties);
		stmt->properties = NULL;
	}

	if (stmt->methods && stmt->methods->dtor) {
		stmt->methods->dtor(stmt TSRMLS_CC);
	}
	if (stmt->query_string) {
		efree(stmt->query_string);
	}

	if (stmt->columns) {
		int i;
		struct pdo_column_data *cols = stmt->columns;

		for (i = 0; i < stmt->column_count; i++) {
			efree(cols[i].name);
		}
		efree(stmt->columns);
	}

	if (stmt->bound_params) {
		zend_hash_destroy(stmt->bound_params);
		FREE_HASHTABLE(stmt->bound_params);
	}
	if (stmt->bound_param_map) {
		zend_hash_destroy(stmt->bound_param_map);
		FREE_HASHTABLE(stmt->bound_param_map);
	}
	if (stmt->bound_columns) {
		zend_hash_destroy(stmt->bound_columns);
		FREE_HASHTABLE(stmt->bound_columns);
	}
	
	do_fetch_opt_finish(stmt, 1 TSRMLS_CC);

	zend_objects_store_del_ref(&stmt->database_object_handle TSRMLS_CC);
	efree(stmt);
}

void pdo_dbstmt_free_storage(void *object TSRMLS_DC)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)object;

	if (--stmt->refcount == 0) {
		free_statement(stmt TSRMLS_CC);
	}
}

zend_object_value pdo_dbstmt_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;

	pdo_stmt_t *stmt;
	stmt = emalloc(sizeof(*stmt));
	memset(stmt, 0, sizeof(*stmt));
	stmt->ce = ce;
	stmt->refcount = 1;
	ALLOC_HASHTABLE(stmt->properties);
	zend_hash_init(stmt->properties, 0, NULL, ZVAL_PTR_DTOR, 0);

	retval.handle = zend_objects_store_put(stmt, (zend_objects_store_dtor_t)zend_objects_destroy_object, pdo_dbstmt_free_storage, NULL TSRMLS_CC);
	retval.handlers = &pdo_dbstmt_object_handlers;

	return retval;
}
/* }}} */

/* {{{ statement iterator */

struct php_pdo_iterator {
	zend_object_iterator iter;
	pdo_stmt_t *stmt;
	ulong key;
	zval *fetch_ahead;
};

static void pdo_stmt_iter_dtor(zend_object_iterator *iter TSRMLS_DC)
{
	struct php_pdo_iterator *I = (struct php_pdo_iterator*)iter->data;

	if (--I->stmt->refcount == 0) {
		free_statement(I->stmt TSRMLS_CC);
	}
		
	if (I->fetch_ahead) {
		ZVAL_DELREF(I->fetch_ahead);
	}

	efree(I);
}

static int pdo_stmt_iter_valid(zend_object_iterator *iter TSRMLS_DC)
{
	struct php_pdo_iterator *I = (struct php_pdo_iterator*)iter->data;

	return I->fetch_ahead ? SUCCESS : FAILURE;
}

static void pdo_stmt_iter_get_data(zend_object_iterator *iter, zval ***data TSRMLS_DC)
{
	struct php_pdo_iterator *I = (struct php_pdo_iterator*)iter->data;
	zval **ptr_ptr;

	/* sanity */
	if (!I->fetch_ahead) {
		*data = NULL;
		return;
	}

	ptr_ptr = emalloc(sizeof(*ptr_ptr));
	*ptr_ptr = I->fetch_ahead;
	ZVAL_ADDREF(I->fetch_ahead);
	
	*data = ptr_ptr;
}

static int pdo_stmt_iter_get_key(zend_object_iterator *iter, char **str_key, uint *str_key_len,
	ulong *int_key TSRMLS_DC)
{
	struct php_pdo_iterator *I = (struct php_pdo_iterator*)iter->data;

	if (I->key == (ulong)-1) {
		return HASH_KEY_NON_EXISTANT;
	}
	*int_key = I->key;
	return HASH_KEY_IS_LONG;
}

static void pdo_stmt_iter_move_forwards(zend_object_iterator *iter TSRMLS_DC)
{
	struct php_pdo_iterator *I = (struct php_pdo_iterator*)iter->data;

	if (I->fetch_ahead) {
		ZVAL_DELREF(I->fetch_ahead);
		I->fetch_ahead = NULL;
	}

	MAKE_STD_ZVAL(I->fetch_ahead);

	if (!do_fetch(I->stmt, TRUE, I->fetch_ahead, PDO_FETCH_USE_DEFAULT,
			PDO_FETCH_ORI_NEXT, 0, 0 TSRMLS_CC)) {
		pdo_stmt_t *stmt = I->stmt; /* for PDO_HANDLE_STMT_ERR() */

		PDO_HANDLE_STMT_ERR();
		I->key = (ulong)-1;
		FREE_ZVAL(I->fetch_ahead);
		I->fetch_ahead = NULL;

		return;
	}

	I->key++;
}

static zend_object_iterator_funcs pdo_stmt_iter_funcs = {
	pdo_stmt_iter_dtor,
	pdo_stmt_iter_valid,
	pdo_stmt_iter_get_data,
	pdo_stmt_iter_get_key,
	pdo_stmt_iter_move_forwards,
	NULL
};

zend_object_iterator *pdo_stmt_iter_get(zend_class_entry *ce, zval *object TSRMLS_DC)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)zend_object_store_get_object(object TSRMLS_CC);
	struct php_pdo_iterator *I;

	I = ecalloc(1, sizeof(*I));
	I->iter.funcs = &pdo_stmt_iter_funcs;
	I->iter.data = I;
	I->stmt = stmt;
	stmt->refcount++;

	MAKE_STD_ZVAL(I->fetch_ahead);
	if (!do_fetch(I->stmt, TRUE, I->fetch_ahead, PDO_FETCH_USE_DEFAULT,
			PDO_FETCH_ORI_NEXT, 0, 0 TSRMLS_CC)) {
		PDO_HANDLE_STMT_ERR();
		I->key = (ulong)-1;
		FREE_ZVAL(I->fetch_ahead);
		I->fetch_ahead = NULL;
	}

	return &I->iter;
}

/* }}} */

/* {{{ overloaded handlers for PDORow class (used by PDO_FETCH_LAZY) */

function_entry pdo_row_functions[] = {
	{NULL, NULL, NULL}
};

static zval *row_prop_or_dim_read(zval *object, zval *member, int type TSRMLS_DC)
{
	zval *return_value;
	pdo_stmt_t * stmt = (pdo_stmt_t *) zend_object_store_get_object(object TSRMLS_CC);
	int colno = -1;

	MAKE_STD_ZVAL(return_value);
		
	if (Z_TYPE_P(member) == IS_LONG) {
		if (Z_LVAL_P(member) >= 0 && Z_LVAL_P(member) < stmt->column_count) {
			fetch_value(stmt, return_value, Z_LVAL_P(member) TSRMLS_CC);
		}
	} else {
		convert_to_string(member);
		/* TODO: replace this with a hash of available column names to column
		 * numbers */
		for (colno = 0; colno < stmt->column_count; colno++) {
			if (strcmp(stmt->columns[colno].name, Z_STRVAL_P(member)) == 0) {
				fetch_value(stmt, return_value, colno TSRMLS_CC);
				break;
			}
		}
	}
	
	return return_value;
}

static void row_prop_or_dim_write(zval *object, zval *member, zval *value TSRMLS_DC)
{
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "This PDORow is not from a writable result set");
}

static int row_prop_or_dim_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
{
	pdo_stmt_t * stmt = (pdo_stmt_t *) zend_object_store_get_object(object TSRMLS_CC);
	int colno = -1;

	if (Z_TYPE_P(member) == IS_LONG) {
		return Z_LVAL_P(member) >= 0 && Z_LVAL_P(member) < stmt->column_count;
	} else {
		convert_to_string(member);

		/* TODO: replace this with a hash of available column names to column
		 * numbers */
		for (colno = 0; colno < stmt->column_count; colno++) {
			if (strcmp(stmt->columns[colno].name, Z_STRVAL_P(member)) == 0) {
				return 1;
			}
		}
	}

	return 0;
}

static void row_prop_or_dim_delete(zval *object, zval *offset TSRMLS_DC)
{
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot delete properties from a PDORow");
}

static HashTable *row_get_properties(zval *object TSRMLS_DC)
{
	zval *tmp;
	pdo_stmt_t * stmt = (pdo_stmt_t *) zend_object_store_get_object(object TSRMLS_CC);
	int i;
	HashTable *ht;

	MAKE_STD_ZVAL(tmp);
	array_init(tmp);

	for (i = 0; i < stmt->column_count; i++) {
		zval *val;
		MAKE_STD_ZVAL(val);
		fetch_value(stmt, val, i TSRMLS_CC);

		add_assoc_zval(tmp, stmt->columns[i].name, val);
	}

	ht = Z_ARRVAL_P(tmp);

	ZVAL_NULL(tmp);
	FREE_ZVAL(tmp);

	return ht;
}

static union _zend_function *row_method_get(
#if PHP_API_VERSION >= 20041225
	zval **object_pp,
#else
	zval *object,
#endif
	char *method_name, int method_len TSRMLS_DC)
{
	zend_function *fbc;
	char *lc_method_name;

	lc_method_name = emalloc(method_len + 1);
	zend_str_tolower_copy(lc_method_name, method_name, method_len);

	if (zend_hash_find(&pdo_row_ce->function_table, lc_method_name, method_len+1, (void**)&fbc) == FAILURE) {
		efree(lc_method_name);
		return NULL;
	}
	
	efree(lc_method_name);
	return fbc;
}

static int row_call_method(char *method, INTERNAL_FUNCTION_PARAMETERS)
{
	return FAILURE;
}

static union _zend_function *row_get_ctor(zval *object TSRMLS_DC)
{
	static zend_internal_function ctor = {0};

	ctor.type = ZEND_INTERNAL_FUNCTION;
	ctor.function_name = "__construct";
	ctor.scope = pdo_row_ce;
	ctor.handler = ZEND_FN(dbstmt_constructor);

	return (union _zend_function*)&ctor;
}

static zend_class_entry *row_get_ce(zval *object TSRMLS_DC)
{
	return pdo_dbstmt_ce;
}

static int row_get_classname(zval *object, char **class_name, zend_uint *class_name_len, int parent TSRMLS_DC)
{
	*class_name = estrndup("PDORow", sizeof("PDORow")-1);
	*class_name_len = sizeof("PDORow")-1;
	return 0;
}

static int row_compare(zval *object1, zval *object2 TSRMLS_DC)
{
	return -1;
}

zend_object_handlers pdo_row_object_handlers = {
	ZEND_OBJECTS_STORE_HANDLERS,
	row_prop_or_dim_read,
	row_prop_or_dim_write,
	row_prop_or_dim_read,
	row_prop_or_dim_write,
	NULL,
	NULL,
	NULL,
	row_prop_or_dim_exists,
	row_prop_or_dim_delete,
	row_prop_or_dim_exists,
	row_prop_or_dim_delete,
	row_get_properties,
	row_method_get,
	row_call_method,
	row_get_ctor,
	row_get_ce,
	row_get_classname,
	row_compare,
	NULL, /* cast */
	NULL
};

void pdo_row_free_storage(void  *object TSRMLS_DC)
{
	pdo_stmt_t *stmt = (pdo_stmt_t*)object;

	ZVAL_NULL(&stmt->lazy_object_ref);
	
	if (--stmt->refcount == 0) {
		free_statement(stmt TSRMLS_CC);
	}
}

zend_object_value pdo_row_new(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value retval;

	retval.handle = zend_objects_store_put(NULL, (zend_objects_store_dtor_t)zend_objects_destroy_object, pdo_row_free_storage, NULL TSRMLS_CC);
	retval.handlers = &pdo_row_object_handlers;

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
