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
   | Authors: Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_spl.h"
#include "spl_functions.h"
#include "spl_engine.h"
#include "spl_array.h"
#include "spl_directory.h"
#include "spl_iterators.h"
#include "spl_sxe.h"
#include "spl_exceptions.h"
#include "spl_observer.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"

#ifdef COMPILE_DL_SPL
ZEND_GET_MODULE(spl)
#endif

ZEND_DECLARE_MODULE_GLOBALS(spl)

/* {{{ spl_functions_none
 */
function_entry spl_functions_none[] = {
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ spl_init_globals
 */
static void spl_init_globals(zend_spl_globals *spl_globals)
{
	spl_globals->autoload_extensions = NULL;
	spl_globals->autoload_functions  = NULL;
}
/* }}} */

static zend_class_entry * spl_find_ce_by_name(zend_uchar ztype, void *name, int len, zend_bool autoload TSRMLS_DC)
{
	zend_class_entry **ce;
	int found;

	if (!autoload) {
		char *lc_name;

		lc_name = zend_u_str_tolower_dup(ztype, name, len);
		found = zend_u_hash_find(EG(class_table), ztype, lc_name, len +1, (void **) &ce);
		efree(lc_name);
	} else {
 		found = zend_u_lookup_class(ztype, name, len, &ce TSRMLS_CC);
 	}
 	if (found != SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Class %v does not exist%s", name, autoload ? " and could not be loaded" : "");
		return NULL;
	}
	
	return *ce;
}

/* {{{ array class_parents(object instance)
 Return an array containing the names of all parent classes */
PHP_FUNCTION(class_parents)
{
	zval *obj;
	zend_class_entry *parent_class, *ce;
	zend_bool autoload = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|b", &obj, &autoload) == FAILURE) {
		RETURN_FALSE;
	}
	
	if (Z_TYPE_P(obj) != IS_OBJECT && Z_TYPE_P(obj) != IS_STRING && Z_TYPE_P(obj) != IS_UNICODE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "object or string expected");
		RETURN_FALSE;
	}
	
	if (Z_TYPE_P(obj) == IS_STRING || Z_TYPE_P(obj) == IS_UNICODE) {
		if (NULL == (ce = spl_find_ce_by_name(Z_TYPE_P(obj), Z_UNIVAL_P(obj), Z_UNILEN_P(obj), autoload TSRMLS_CC))) {
			RETURN_FALSE;
		}
	} else {
		ce = Z_OBJCE_P(obj);
	}
	
	array_init(return_value);
	parent_class = ce->parent;
	while (parent_class) {
		spl_add_class_name(return_value, parent_class, 0, 0 TSRMLS_CC);
		parent_class = parent_class->parent;
	}
}
/* }}} */

/* {{{ proto array class_implements(mixed what [, bool autoload ])
 Return all classes and interfaces implemented by SPL */
PHP_FUNCTION(class_implements)
{
	zval *obj;
	zend_bool autoload = 1;
	zend_class_entry *ce;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|b", &obj, &autoload) == FAILURE) {
		RETURN_FALSE;
	}
	if (Z_TYPE_P(obj) != IS_OBJECT && Z_TYPE_P(obj) != IS_STRING && Z_TYPE_P(obj) != IS_UNICODE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "object or string expected");
		RETURN_FALSE;
	}
	
	if (Z_TYPE_P(obj) == IS_STRING || Z_TYPE_P(obj) == IS_UNICODE) {
		if (NULL == (ce = spl_find_ce_by_name(Z_TYPE_P(obj), Z_UNIVAL_P(obj), Z_UNILEN_P(obj), autoload TSRMLS_CC))) {
			RETURN_FALSE;
		}
	} else {
		ce = Z_OBJCE_P(obj);
	}
	
	array_init(return_value);
	spl_add_interfaces(return_value, ce, 1, ZEND_ACC_INTERFACE TSRMLS_CC);
}
/* }}} */

#define SPL_ADD_CLASS(class_name, z_list, sub, allow, ce_flags) \
	spl_add_classes(U_CLASS_ENTRY(spl_ce_ ## class_name), z_list, sub, allow, ce_flags TSRMLS_CC)

#define SPL_LIST_CLASSES(z_list, sub, allow, ce_flags) \
	SPL_ADD_CLASS(AppendIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(ArrayIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(ArrayObject, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(BadFunctionCallException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(BadMethodCallException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(CachingIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(Countable, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(DirectoryIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(DomainException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(EmptyIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(FilterIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(InfiniteIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(InvalidArgumentException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(IteratorIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(LengthException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(LimitIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(LogicException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(NoRewindIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(OuterIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(OutOfBoundsException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(OutOfRangeException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(OverflowException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(ParentIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RangeException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveArrayIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveCachingIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveDirectoryIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveFilterIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveIteratorIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveRegExIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RegExIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RuntimeException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SeekableIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SimpleXMLIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplFileInfo, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplFileObject, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplObjectStorage, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplObserver, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplSubject, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplTempFileObject, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(UnderflowException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(UnexpectedValueException, z_list, sub, allow, ce_flags); \

/* {{{ proto array spl_classes()
 Return an array containing the names of all clsses and interfaces defined in SPL */
PHP_FUNCTION(spl_classes)
{
	array_init(return_value);
	
	SPL_LIST_CLASSES(return_value, 0, 0, 0)
}
/* }}} */

int spl_autoload(const char *class_name, const char * lc_name, int class_name_len, const char * file_extension TSRMLS_DC) /* {{{ */
{
	char *class_file;
	int class_file_len;
	int dummy = 1;
	zend_file_handle file_handle;
	zend_op_array *new_op_array;
	zval *result = NULL;

	class_file_len = spprintf(&class_file, 0, "%s%s", lc_name, file_extension);

	if (zend_stream_open(class_file, &file_handle TSRMLS_CC) == SUCCESS) {
		if (!file_handle.opened_path) {
			file_handle.opened_path = estrndup(class_file, class_file_len);
		}
		if (zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path)+1, (void *)&dummy, sizeof(int), NULL)==SUCCESS) {
			new_op_array = zend_compile_file(&file_handle, ZEND_REQUIRE TSRMLS_CC);
			zend_destroy_file_handle(&file_handle TSRMLS_CC);
		} else {
			new_op_array = NULL;
			zend_file_handle_dtor(&file_handle);
		}
		if (new_op_array) {
			EG(return_value_ptr_ptr) = &result;
			EG(active_op_array) = new_op_array;
	
			zend_execute(new_op_array TSRMLS_CC);
	
			destroy_op_array(new_op_array TSRMLS_CC);
			efree(new_op_array);
			if (!EG(exception)) {
				if (EG(return_value_ptr_ptr)) {
					zval_ptr_dtor(EG(return_value_ptr_ptr));
				}
			}

			efree(class_file);
			return zend_hash_exists(EG(class_table), (char*)lc_name, class_name_len+1);
		}
	}
	efree(class_file);
	return 0;
} /* }}} */

/* {{{ void spl_autoload(string class_name [, string file_extensions])
 Default implementation for __autoload() */
PHP_FUNCTION(spl_autoload)
{
	char *class_name, *lc_name, *file_exts;
	int class_name_len, file_exts_len, found = 0;
	char *copy, *pos1, *pos2;
	zval **original_return_value = EG(return_value_ptr_ptr);
	zend_op **original_opline_ptr = EG(opline_ptr);
	zend_op_array *original_active_op_array = EG(active_op_array);
	zend_function_state *original_function_state_ptr = EG(function_state_ptr);
	zval err_mode;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &class_name, &class_name_len, &file_exts, &file_exts_len) == FAILURE) {
		RETURN_FALSE;
	}

	ZVAL_LONG(&err_mode, EG(error_reporting));
	php_alter_ini_entry("error_reporting", sizeof("error_reporting"), "0", 1, ZEND_INI_USER, ZEND_INI_STAGE_RUNTIME); 

	copy = pos1 = estrdup(ZEND_NUM_ARGS() > 1 ? file_exts : SPL_G(autoload_extensions));
	lc_name = zend_str_tolower_dup(class_name, class_name_len);
	while(pos1 && *pos1 && !EG(exception)) {
		EG(return_value_ptr_ptr) = original_return_value;
		EG(opline_ptr) = original_opline_ptr;
		EG(active_op_array) = original_active_op_array;
		EG(function_state_ptr) = original_function_state_ptr;
		pos2 = strchr(pos1, ',');
		if (pos2) *pos2 = '\0';
		if (spl_autoload(class_name, lc_name, class_name_len, pos1 TSRMLS_CC)) {
			found = 1;
			break; /* loaded */
		}
		pos1 = pos2 ? pos2 + 1 : NULL;
	}
	efree(lc_name);
	if (copy) {
		efree(copy);
	}

	if (!EG(error_reporting) && Z_LVAL(err_mode) != EG(error_reporting)) {
		convert_to_string(&err_mode);
		zend_alter_ini_entry("error_reporting", sizeof("error_reporting"), Z_STRVAL(err_mode), Z_STRLEN(err_mode), ZEND_INI_USER, ZEND_INI_STAGE_RUNTIME);
		zendi_zval_dtor(err_mode);
	}

	EG(return_value_ptr_ptr) = original_return_value;
	EG(opline_ptr) = original_opline_ptr;
	EG(active_op_array) = original_active_op_array;
	EG(function_state_ptr) = original_function_state_ptr;

	if (!found) {
		zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_LogicException), 0 TSRMLS_CC, "Class %s could not be loaded", class_name);
	}
} /* }}} */

/* {{{ void string spl_autoload_extensions([string file_extensions])
 Register and return default file extensions for spl_autoload */
PHP_FUNCTION(spl_autoload_extensions)
{
	char *file_exts;
	int file_exts_len;

	if (ZEND_NUM_ARGS() > 0) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &file_exts, &file_exts_len) == FAILURE) {
			return;
		}
	
		if (SPL_G(autoload_extensions)) {
			efree(SPL_G(autoload_extensions));
		}
		SPL_G(autoload_extensions) = estrdup(file_exts);
	}

	RETURN_STRING(SPL_G(autoload_extensions), 1);
} /* }}} */

typedef struct {
	zend_function *func_ptr;
	zval *obj;
	zend_class_entry *ce;
} autoload_func_info;

static void autoload_func_info_dtor(autoload_func_info *alfi)
{
	if (alfi->obj) {
		zval_ptr_dtor(&alfi->obj);
	}
}

/* {{{ void spl_autoload_call(string class_name)
 Try all registerd autoload function to load the requested class */
PHP_FUNCTION(spl_autoload_call)
{
	zval **class_name, *retval = NULL;
	char *func_name, *lc_name;
	uint func_name_len;
	ulong dummy;
	HashPosition function_pos;
	autoload_func_info *alfi;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &class_name) == FAILURE || 
	    Z_TYPE_PP(class_name) != (UG(unicode)?IS_UNICODE:IS_STRING)) {
		return;
	}

	if (SPL_G(autoload_functions)) {
		lc_name = zend_u_str_tolower_dup(Z_TYPE_PP(class_name), Z_UNIVAL_PP(class_name), Z_UNILEN_PP(class_name));
		zend_hash_internal_pointer_reset_ex(SPL_G(autoload_functions), &function_pos);
		while(zend_hash_has_more_elements_ex(SPL_G(autoload_functions), &function_pos) == SUCCESS && !EG(exception)) {
			zend_hash_get_current_key_ex(SPL_G(autoload_functions), &func_name, &func_name_len, &dummy, 0, &function_pos);
			zend_hash_get_current_data_ex(SPL_G(autoload_functions), (void **) &alfi, &function_pos);
			zend_call_method(alfi->obj ? &alfi->obj : NULL, alfi->ce, &alfi->func_ptr, func_name, func_name_len, &retval, 1, *class_name, NULL TSRMLS_CC);
			if (retval) {
				zval_ptr_dtor(&retval);					
			}
			if (zend_u_hash_exists(EG(class_table), Z_TYPE_PP(class_name), lc_name, Z_UNILEN_PP(class_name)+1)) {
				break;
			}
			zend_hash_move_forward_ex(SPL_G(autoload_functions), &function_pos);
		}
		efree(lc_name);
	} else {
		/* do not use or overwrite &EG(autoload_func) here */
		zend_call_method_with_1_params(NULL, NULL, NULL, "spl_autoload", NULL, *class_name);
	}
} /* }}} */

/* {{{ void spl_autoload_register([string autoload_function = "spl_autoload" [, throw = true]])
 Register given function as __autoload() implementation */
PHP_FUNCTION(spl_autoload_register)
{
	zval zfunc_name;
	char *func_name;
	uint func_name_len;
	char *lc_name = NULL;
	zval *zcallable = NULL;
	zend_bool do_throw = 1;
	zend_function *spl_func_ptr;
	autoload_func_info alfi;
	zval **obj_ptr;
	zend_uchar func_name_type;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "|tb", &func_name, &func_name_len, &func_name_type, &do_throw) == FAILURE) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|b", &zcallable, &do_throw) == FAILURE) {
			return;
		}
		if (!zend_is_callable_ex(zcallable, IS_CALLABLE_CHECK_IS_STATIC, &zfunc_name, &alfi.ce, &alfi.func_ptr, &obj_ptr TSRMLS_CC)) {
			if (do_throw) {
				zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_LogicException), 0 TSRMLS_CC, "Passed array does not specify a callable static method");
			}
			zval_dtor(&zfunc_name);
			return;
		} else if (!obj_ptr && alfi.func_ptr && !(alfi.func_ptr->common.fn_flags & ZEND_ACC_STATIC)) {
			if (do_throw) {
				zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_LogicException), 0 TSRMLS_CC, "Passed array specifies a non static method but no object");
			}
			zval_dtor(&zfunc_name);
			return;
		}
		func_name_type = Z_TYPE(zfunc_name);
		func_name_len = Z_UNILEN(zfunc_name);
		lc_name = zend_u_str_tolower_dup(func_name_type, Z_UNIVAL(zfunc_name), func_name_len);
		zval_dtor(&zfunc_name);
		if (obj_ptr && !(alfi.func_ptr->common.fn_flags & ZEND_ACC_STATIC)) {
			alfi.obj = *obj_ptr;
			alfi.obj->refcount++;
		} else {
			alfi.obj = NULL;
		}
	} else if (ZEND_NUM_ARGS()) {
		lc_name = zend_u_str_tolower_dup(func_name_type, func_name, func_name_len);
		
		if ((func_name_len == sizeof("spl_autoload_call")-1) &&
		    (ZEND_U_EQUAL(func_name_type, lc_name, func_name_len, "spl_autoload_call", sizeof("spl_autoload_call")-1))) {
			if (do_throw) {
				zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_LogicException), 0 TSRMLS_CC, "Function spl_autoload_call() cannot be registered");
			}
			efree(lc_name);
			return;
		}

		if (zend_u_hash_find(EG(function_table), func_name_type, lc_name, func_name_len+1, (void **) &alfi.func_ptr) == FAILURE) {
			if (do_throw) {
				zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_LogicException), 0 TSRMLS_CC, "Function '%R' not found", func_name_type, func_name);
			}
			efree(lc_name);
			return;
		}
		alfi.obj = NULL;
		alfi.ce = NULL;
	}
	
	if (ZEND_NUM_ARGS()) {
		if (!SPL_G(autoload_functions)) {
			ALLOC_HASHTABLE(SPL_G(autoload_functions));
			zend_u_hash_init(SPL_G(autoload_functions), 1, NULL, (dtor_func_t) autoload_func_info_dtor, 0, UG(unicode));
		}

		zend_hash_find(EG(function_table), "spl_autoload", sizeof("spl_autoload"), (void **) &spl_func_ptr);

		if (EG(autoload_func) == spl_func_ptr) { /* registered already, so we insert that first */
			autoload_func_info spl_alfi;

			spl_alfi.func_ptr = spl_func_ptr;
			spl_alfi.obj = NULL;
			spl_alfi.ce = NULL;
			zend_hash_add(SPL_G(autoload_functions), "spl_autoload", sizeof("spl_autoload"), &spl_alfi, sizeof(autoload_func_info), NULL);
		}

		zend_u_hash_add(SPL_G(autoload_functions), func_name_type, lc_name, func_name_len+1, &alfi, sizeof(autoload_func_info), NULL);

		efree(lc_name);
	}

	if (SPL_G(autoload_functions)) {
		zend_hash_find(EG(function_table), "spl_autoload_call", sizeof("spl_autoload_call"), (void **) &EG(autoload_func));
	} else {
		zend_hash_find(EG(function_table), "spl_autoload", sizeof("spl_autoload"), (void **) &EG(autoload_func));
	}
} /* }}} */

/* {{{ bool spl_autoload_unregister(string autoload_function)
 Unregister given function as __autoload() implementation */
PHP_FUNCTION(spl_autoload_unregister)
{
	char *func_name, *lc_name;
	int func_name_len, success = FAILURE;
	zend_function *spl_func_ptr;
	zend_uchar func_name_type;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "t", &func_name, &func_name_len, &func_name_type) == FAILURE) {
		return;
	}

	lc_name = zend_u_str_tolower_dup(func_name_type, func_name, func_name_len);

	if (SPL_G(autoload_functions)) {
		if ((func_name_len == sizeof("spl_autoload_call")-1) &&
		    (ZEND_U_EQUAL(func_name_type, lc_name, func_name_len, "spl_autoload_call", sizeof("spl_autoload_call")-1))) {
			/* remove all */
			zend_hash_destroy(SPL_G(autoload_functions));
			FREE_HASHTABLE(SPL_G(autoload_functions));
			SPL_G(autoload_functions) = NULL;
			EG(autoload_func) = NULL;
			success = SUCCESS;
		} else {
			/* remove specific */
			success = zend_u_hash_del(SPL_G(autoload_functions), func_name_type, lc_name, func_name_len+1);
		}
	} else if ((func_name_len == sizeof("spl_autoload")-1) &&
	           (ZEND_U_EQUAL(func_name_type, lc_name, func_name_len, "spl_autoload", sizeof("spl_autoload")-1))) {
		/* register single spl_autoload() */
		zend_hash_find(EG(function_table), "spl_autoload", sizeof("spl_autoload"), (void **) &spl_func_ptr);

		if (EG(autoload_func) == spl_func_ptr) {
			success = SUCCESS;
			EG(autoload_func) = NULL;
		}
	}

	efree(lc_name);
	
	RETURN_BOOL(success == SUCCESS);
} /* }}} */

/* {{{ false|array spl_autoload_functions()
 Return all registered __autoload() functionns */
PHP_FUNCTION(spl_autoload_functions)
{
	zend_function *fptr, **func_ptr_ptr;
	HashPosition function_pos;

	if (!EG(autoload_func)) {
		if (zend_hash_find(EG(function_table), ZEND_AUTOLOAD_FUNC_NAME, sizeof(ZEND_AUTOLOAD_FUNC_NAME), (void **) &fptr) == SUCCESS) {
			array_init(return_value);
			add_next_index_stringl(return_value, ZEND_AUTOLOAD_FUNC_NAME, sizeof(ZEND_AUTOLOAD_FUNC_NAME)-1, 1);
			return;
		}
		RETURN_FALSE;
	}

	zend_hash_find(EG(function_table), "spl_autoload_call", sizeof("spl_autoload_call"), (void **) &fptr);

	if (EG(autoload_func) == fptr) {
		array_init(return_value);
		zend_hash_internal_pointer_reset_ex(SPL_G(autoload_functions), &function_pos);
		while(zend_hash_has_more_elements_ex(SPL_G(autoload_functions), &function_pos) == SUCCESS) {
			zend_hash_get_current_data_ex(SPL_G(autoload_functions), (void **) &func_ptr_ptr, &function_pos);
			if ((*func_ptr_ptr)->common.scope) {
				zval *tmp;
				MAKE_STD_ZVAL(tmp);
				array_init(tmp);

				add_next_index_text(tmp, (*func_ptr_ptr)->common.scope->name, 1);
				add_next_index_text(tmp, (*func_ptr_ptr)->common.function_name, 1);
				add_next_index_zval(return_value, tmp);
			} else
				add_next_index_text(return_value, (*func_ptr_ptr)->common.function_name, 1);

			zend_hash_move_forward_ex(SPL_G(autoload_functions), &function_pos);
		}
		return;
	}

	array_init(return_value);
	add_next_index_text(return_value, EG(autoload_func)->common.function_name, 1);
} /* }}} */

int spl_build_class_list_string(zval **entry, char **list TSRMLS_DC) /* {{{ */
{
	char *res;
	
	spprintf(&res, 0, "%s, %v", *list, Z_STRVAL_PP(entry));
	efree(*list);
	*list = res;
	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

/* {{{ PHP_MINFO(spl)
 */
PHP_MINFO_FUNCTION(spl)
{
	zval list;
	char *strg;

	php_info_print_table_start();
	php_info_print_table_header(2, "SPL support",        "enabled");

	INIT_PZVAL(&list);
	array_init(&list);
	SPL_LIST_CLASSES(&list, 0, 1, ZEND_ACC_INTERFACE)
	strg = estrdup("");
	zend_hash_apply_with_argument(Z_ARRVAL_P(&list), (apply_func_arg_t)spl_build_class_list_string, &strg TSRMLS_CC);
	zval_dtor(&list);
	php_info_print_table_row(2, "Interfaces", strg + 2);
	efree(strg);

	INIT_PZVAL(&list);
	array_init(&list);
	SPL_LIST_CLASSES(&list, 0, -1, ZEND_ACC_INTERFACE)
	strg = estrdup("");
	zend_hash_apply_with_argument(Z_ARRVAL_P(&list), (apply_func_arg_t)spl_build_class_list_string, &strg TSRMLS_CC);
	zval_dtor(&list);
	php_info_print_table_row(2, "Classes", strg + 2);
	efree(strg);

	php_info_print_table_end();
}
/* }}} */

/* {{{ spl_functions
 */
function_entry spl_functions[] = {
	PHP_FE(spl_classes,             NULL)
	PHP_FE(spl_autoload,            NULL)
	PHP_FE(spl_autoload_extensions, NULL)
	PHP_FE(spl_autoload_register,   NULL)
	PHP_FE(spl_autoload_unregister, NULL)
	PHP_FE(spl_autoload_functions,  NULL)
	PHP_FE(spl_autoload_call,       NULL)
	PHP_FE(class_parents,           NULL)
	PHP_FE(class_implements,        NULL)
#ifdef SPL_ITERATORS_H
	PHP_FE(iterator_to_array,       NULL)
	PHP_FE(iterator_count,          NULL)
#endif /* SPL_ITERATORS_H */
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION(spl)
 */
PHP_MINIT_FUNCTION(spl)
{
	ZEND_INIT_MODULE_GLOBALS(spl, spl_init_globals, NULL);

	PHP_MINIT(spl_iterators)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_array)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_directory)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_sxe)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_exceptions)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_observer)(INIT_FUNC_ARGS_PASSTHRU);

	return SUCCESS;
}
/* }}} */

PHP_RINIT_FUNCTION(spl) /* {{{ */
{
	SPL_G(autoload_extensions) = estrndup(".inc,.php", sizeof(".inc,.php")-1);
	SPL_G(autoload_functions) = NULL;
	return SUCCESS;
} /* }}} */

PHP_RSHUTDOWN_FUNCTION(spl) /* {{{ */
{
	if (SPL_G(autoload_extensions)) {
		efree(SPL_G(autoload_extensions));
		SPL_G(autoload_extensions) = NULL;
	}
	if (SPL_G(autoload_functions)) {
		zend_hash_destroy(SPL_G(autoload_functions));
		FREE_HASHTABLE(SPL_G(autoload_functions));
		SPL_G(autoload_functions) = NULL;
	}
	return SUCCESS;
} /* }}} */

#ifdef HAVE_SIMPLEXML
static zend_module_dep spl_deps[] = {
	ZEND_MOD_REQUIRED("libxml")
	ZEND_MOD_REQUIRED("simplexml")
	{NULL, NULL, NULL}
};
#endif

/* {{{ spl_module_entry
 */
zend_module_entry spl_module_entry = {
#ifdef HAVE_SIMPLEXML
	STANDARD_MODULE_HEADER_EX, NULL,
	spl_deps,
#else
	STANDARD_MODULE_HEADER,
#endif
	"SPL",
	spl_functions,
	PHP_MINIT(spl),
	NULL,
	PHP_RINIT(spl),
	PHP_RSHUTDOWN(spl),
	PHP_MINFO(spl),
	"0.2",
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
