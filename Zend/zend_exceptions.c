/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2003 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Sterling Hughes <sterling@php.net>                          |
   |          Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "zend.h"
#include "zend_API.h"
#include "zend_reflection_api.h"

zend_class_entry *default_exception_ptr;

static zend_object_value zend_default_exception_new(zend_class_entry *class_type TSRMLS_DC)
{
	zval tmp, obj;
	zend_object *object;

	obj.value.obj = zend_objects_new(&object, class_type TSRMLS_CC);

	ALLOC_HASHTABLE(object->properties);
	zend_hash_init(object->properties, 0, NULL, ZVAL_PTR_DTOR, 0);
	zend_hash_copy(object->properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

	zend_update_property_string(class_type, &obj, "message", sizeof("message")-1, "Unknown exception" TSRMLS_CC);
	zend_update_property_string(class_type, &obj, "file", sizeof("file")-1, zend_get_executed_filename(TSRMLS_C) TSRMLS_CC);
	zend_update_property_long(class_type, &obj, "line", sizeof("line")-1, zend_get_executed_lineno(TSRMLS_C) TSRMLS_CC);

	return obj.value.obj;
}

ZEND_FUNCTION(exception)
{
	char  *message = NULL;
	long   code = 0;
	zval  *object;
	int    argc = ZEND_NUM_ARGS(), message_len;

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, argc TSRMLS_CC, "|sl", &message, &message_len, &code) == FAILURE) {
		zend_error(E_CORE_ERROR, "Wrong parameter count for exception([string $exception [, long $code ]])");
	}

	object = getThis();

	if (message) {
		zend_update_property_string(Z_OBJCE_P(object), object, "message", sizeof("message")-1, message TSRMLS_CC);
	}

	if (code) {
		zend_update_property_long(Z_OBJCE_P(object), object, "code", sizeof("code")-1, code TSRMLS_CC);
	}
}

#define DEFAULT_0_PARAMS \
	if (ZEND_NUM_ARGS() > 0) { \
		ZEND_WRONG_PARAM_COUNT(); \
	}

static void _default_exception_get_entry(zval *object, char *name, int name_len, zval *return_value TSRMLS_DC)
{
	zval *value;

	value = zend_read_property(Z_OBJCE_P(object), object, name, name_len, 0 TSRMLS_CC);

	*return_value = *value;
	zval_copy_ctor(return_value);
}

ZEND_FUNCTION(getfile)
{
	DEFAULT_0_PARAMS;

	_default_exception_get_entry(getThis(), "file", sizeof("file")-1, return_value TSRMLS_CC);
}

ZEND_FUNCTION(getline)
{
	DEFAULT_0_PARAMS;

	_default_exception_get_entry(getThis(), "line", sizeof("line")-1, return_value TSRMLS_CC);
}

ZEND_FUNCTION(getmessage)
{
	DEFAULT_0_PARAMS;

	_default_exception_get_entry(getThis(), "message", sizeof("message")-1, return_value TSRMLS_CC);
}

ZEND_FUNCTION(getcode)
{
	DEFAULT_0_PARAMS;

	_default_exception_get_entry(getThis(), "code", sizeof("code")-1, return_value TSRMLS_CC);
}

static zend_function_entry default_exception_functions[] = {
	ZEND_FE(exception, NULL)
	ZEND_FE(getmessage, NULL)
	ZEND_FE(getcode, NULL)
	ZEND_FE(getfile, NULL)
	ZEND_FE(getline, NULL)
	{NULL, NULL, NULL}
};

static void zend_register_default_exception(TSRMLS_D)
{
	zend_class_entry default_exception;

	INIT_CLASS_ENTRY(default_exception, "exception", default_exception_functions);
	default_exception_ptr = zend_register_internal_class(&default_exception TSRMLS_CC);
	default_exception_ptr->create_object = zend_default_exception_new;

	zend_declare_property_null(default_exception_ptr, "message", sizeof("message")-1, ZEND_ACC_PROTECTED);
	zend_declare_property_long(default_exception_ptr, "code", sizeof("code")-1, 0, ZEND_ACC_PROTECTED);
	zend_declare_property_null(default_exception_ptr, "file", sizeof("file")-1, ZEND_ACC_PROTECTED);
	zend_declare_property_null(default_exception_ptr, "line", sizeof("line")-1, ZEND_ACC_PROTECTED);
}

ZEND_API zend_class_entry *zend_exception_get_default(void)
{
	return default_exception_ptr;
}

ZEND_API void zend_throw_exception(char *message, long code TSRMLS_DC)
{
	zval *ex;

	MAKE_STD_ZVAL(ex);
	object_init_ex(ex, default_exception_ptr);

	if (message) {
		zend_update_property_string(default_exception_ptr, ex, "message", sizeof("message")-1, message TSRMLS_CC);
	}
	if (code) {
		zend_update_property_long(default_exception_ptr, ex, "code", sizeof("code")-1, code TSRMLS_CC);
	}

	EG(exception) = ex;
}

ZEND_API void zend_register_default_classes(TSRMLS_D)
{
	zend_register_default_exception(TSRMLS_C);
	zend_register_reflection_api(TSRMLS_C);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
