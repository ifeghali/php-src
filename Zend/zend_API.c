/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2005 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   |          Andrei Zmievski <andrei@php.net>                            |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "zend.h"
#include "zend_execute.h"
#include "zend_API.h"
#include "zend_modules.h"
#include "zend_constants.h"

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

/* these variables are true statics/globals, and have to be mutex'ed on every access */
static int module_count=0;
ZEND_API HashTable module_registry;

/* this function doesn't check for too many parameters */
ZEND_API int zend_get_parameters(int ht, int param_count, ...)
{
	void **p;
	int arg_count;
	va_list ptr;
	zval **param, *param_ptr;
	TSRMLS_FETCH();

	p = EG(argument_stack).top_element-2;
	arg_count = (ulong) *p;

	if (param_count>arg_count) {
		return FAILURE;
	}

	va_start(ptr, param_count);

	while (param_count-->0) {
		param = va_arg(ptr, zval **);
		param_ptr = *(p-arg_count);
		if (!PZVAL_IS_REF(param_ptr) && param_ptr->refcount>1) {
			zval *new_tmp;

			ALLOC_ZVAL(new_tmp);
			*new_tmp = *param_ptr;
			zval_copy_ctor(new_tmp);
			INIT_PZVAL(new_tmp);
			param_ptr = new_tmp;
			((zval *) *(p-arg_count))->refcount--;
			*(p-arg_count) = param_ptr;
		}
		*param = param_ptr;
		arg_count--;
	}
	va_end(ptr);

	return SUCCESS;
}


ZEND_API int _zend_get_parameters_array(int ht, int param_count, zval **argument_array TSRMLS_DC)
{
	void **p;
	int arg_count;
	zval *param_ptr;

	p = EG(argument_stack).top_element-2;
	arg_count = (ulong) *p;

	if (param_count>arg_count) {
		return FAILURE;
	}

	while (param_count-->0) {
		param_ptr = *(p-arg_count);
		if (!PZVAL_IS_REF(param_ptr) && param_ptr->refcount>1) {
			zval *new_tmp;

			ALLOC_ZVAL(new_tmp);
			*new_tmp = *param_ptr;
			zval_copy_ctor(new_tmp);
			INIT_PZVAL(new_tmp);
			param_ptr = new_tmp;
			((zval *) *(p-arg_count))->refcount--;
			*(p-arg_count) = param_ptr;
		}
		*(argument_array++) = param_ptr;
		arg_count--;
	}

	return SUCCESS;
}




/* Zend-optimized Extended functions */
/* this function doesn't check for too many parameters */
ZEND_API int zend_get_parameters_ex(int param_count, ...)
{
	void **p;
	int arg_count;
	va_list ptr;
	zval ***param;
	TSRMLS_FETCH();

	p = EG(argument_stack).top_element-2;
	arg_count = (ulong) *p;

	if (param_count>arg_count) {
		return FAILURE;
	}

	va_start(ptr, param_count);
	while (param_count-->0) {
		param = va_arg(ptr, zval ***);
		*param = (zval **) p-(arg_count--);
	}
	va_end(ptr);

	return SUCCESS;
}


ZEND_API int _zend_get_parameters_array_ex(int param_count, zval ***argument_array TSRMLS_DC)
{
	void **p;
	int arg_count;

	p = EG(argument_stack).top_element-2;
	arg_count = (ulong) *p;

	if (param_count>arg_count) {
		return FAILURE;
	}

	while (param_count-->0) {
		zval **value = (zval**)(p-arg_count);

		if (EG(ze1_compatibility_mode) && Z_TYPE_PP(value) == IS_OBJECT) {
			zval *value_ptr;
			char *class_name;
			zend_uint class_name_len;
			int dup;
			
			dup = zend_get_object_classname(*value, &class_name, &class_name_len TSRMLS_CC);

			ALLOC_ZVAL(value_ptr);
			*value_ptr = **value;
			INIT_PZVAL(value_ptr);
			zend_error(E_STRICT, "Implicit cloning object of class '%v' because of 'zend.ze1_compatibility_mode'", class_name);
			if(!dup) {
				efree(class_name);
			}
			value_ptr->value.obj = Z_OBJ_HANDLER_PP(value, clone_obj)(*value TSRMLS_CC);
			zval_ptr_dtor(value);
			*value = value_ptr;
		}
		*(argument_array++) = value;
		arg_count--;
	}

	return SUCCESS;
}


ZEND_API int zend_copy_parameters_array(int param_count, zval *argument_array TSRMLS_DC)
{
	void **p;
	int arg_count;

	p = EG(argument_stack).top_element-2;
	arg_count = (ulong) *p;

	if (param_count>arg_count) {
		return FAILURE;
	}

	while (param_count-->0) {
		zval **param = (zval **) p-(arg_count--);
		zval_add_ref(param);
		add_next_index_zval(argument_array, *param);		
	}

	return SUCCESS;
}


ZEND_API void zend_wrong_param_count(TSRMLS_D)
{
	char *space;
	char *class_name = get_active_class_name(&space TSRMLS_CC);
	
	zend_error(E_WARNING, "Wrong parameter count for %v%s%v()", class_name, space, get_active_function_name(TSRMLS_C));
}


/* Argument parsing API -- andrei */

ZEND_API char *zend_zval_type_name(zval *arg)
{
	switch (Z_TYPE_P(arg)) {
		case IS_NULL:
			return "null";

		case IS_LONG:
			return "integer";

		case IS_DOUBLE:
			return "double";

		case IS_STRING:
			return "native string";

		case IS_ARRAY:
			return "array";

		case IS_OBJECT:
			return "object";

		case IS_BOOL:
			return "boolean";

		case IS_RESOURCE:
			return "resource";

		case IS_UNICODE:
			return "Unicode string";

		case IS_BINARY:
			return "binary data";

		default:
			return "unknown";
	}
}

ZEND_API zend_class_entry *zend_get_class_entry(zval *zobject TSRMLS_DC)
{
	if (Z_OBJ_HT_P(zobject)->get_class_entry) {
		return Z_OBJ_HT_P(zobject)->get_class_entry(zobject TSRMLS_CC);
	} else {
		zend_error(E_ERROR, "Class entry requested for an object without PHP class");
		return NULL;
	}
}

/* returns 1 if you need to copy result, 0 if it's already a copy */
ZEND_API int zend_get_object_classname(zval *object, char **class_name, zend_uint *class_name_len TSRMLS_DC)
{
	if (Z_OBJ_HT_P(object)->get_class_name == NULL ||
		Z_OBJ_HT_P(object)->get_class_name(object, class_name, class_name_len, 0 TSRMLS_CC) != SUCCESS) {
		zend_class_entry *ce = Z_OBJCE_P(object);
		
		*class_name = ce->name;
		*class_name_len = ce->name_length;
		return 1;
	}
	return 0;
}

#define RETURN_AS_STRING(arg, p, pl, type) \
	*(char**)p = Z_STRVAL_PP(arg); \
	*pl = Z_STRLEN_PP(arg); \
	*type = IS_STRING; 

#define RETURN_AS_BINARY(arg, p, pl, type) \
	*(char**)p = Z_BINVAL_PP(arg); \
	*pl = Z_BINLEN_PP(arg); \
	*type = IS_BINARY; 

#define RETURN_AS_UNICODE(arg, p, pl, type) \
	*(UChar**)p = Z_USTRVAL_PP(arg); \
	*pl = Z_USTRLEN_PP(arg); \
	*type = IS_UNICODE;

static char *zend_parse_arg_impl(zval **arg, va_list *va, char **spec, char T_arg_type TSRMLS_DC)
{
	char *spec_walk = *spec;
	char c = *spec_walk++;
	int return_null = 0;

	while (*spec_walk == '/' || *spec_walk == '!') {
		if (*spec_walk == '/') {
			SEPARATE_ZVAL_IF_NOT_REF(arg);
		} else if (*spec_walk == '!' && Z_TYPE_PP(arg) == IS_NULL) {
			return_null = 1;
		}
		spec_walk++;
	}

	switch (c) {
		case 'l':
			{
				long *p = va_arg(*va, long *);
				switch (Z_TYPE_PP(arg)) {
					case IS_STRING:
						{
							double d;
							int type;

							if ((type = is_numeric_string(Z_STRVAL_PP(arg), Z_STRLEN_PP(arg), p, &d, 0)) == 0) {
								return "long";
							} else if (type == IS_DOUBLE) {
								*p = (long) d;
							}
						}
						break;

					case IS_UNICODE:
						{
							double d;
							int type;

							if ((type = is_numeric_unicode(Z_USTRVAL_PP(arg), Z_USTRLEN_PP(arg), p, &d, 0)) == 0) {
								return "long";
							} else if (type == IS_DOUBLE) {
								*p = (long) d;
							}
						}
						break;

					case IS_NULL:
					case IS_LONG:
					case IS_DOUBLE:
					case IS_BOOL:
						convert_to_long_ex(arg);
						*p = Z_LVAL_PP(arg);
						break;

					case IS_ARRAY:
					case IS_OBJECT:
					case IS_RESOURCE:
					default:
						return "long";
				}
			}
			break;

		case 'd':
			{
				double *p = va_arg(*va, double *);
				switch (Z_TYPE_PP(arg)) {
					case IS_STRING:
						{
							long l;
							int type;

							if ((type = is_numeric_string(Z_STRVAL_PP(arg), Z_STRLEN_PP(arg), &l, p, 0)) == 0) {
								return "double";
							} else if (type == IS_LONG) {
								*p = (double) l;
							}
						}
						break;

					case IS_UNICODE:
						{
							long l;
							int type;

							if ((type = is_numeric_unicode(Z_USTRVAL_PP(arg), Z_USTRLEN_PP(arg), &l, p, 0)) == 0) {
								return "double";
							} else if (type == IS_LONG) {
								*p = (double) l;
							}
						}
						break;

					case IS_NULL:
					case IS_LONG:
					case IS_DOUBLE:
					case IS_BOOL:
						convert_to_double_ex(arg);
						*p = Z_DVAL_PP(arg);
						break;

					case IS_ARRAY:
					case IS_OBJECT:
					case IS_RESOURCE:
					default:
						return "double";
				}
			}
			break;

		case 's':
			{
				char **p = va_arg(*va, char **);
				int *pl = va_arg(*va, int *);
				switch (Z_TYPE_PP(arg)) {
					case IS_NULL:
						if (return_null) {
							*p = NULL;
							*pl = 0;
							break;
						}
						/* break omitted intentionally */

					case IS_STRING:
					case IS_LONG:
					case IS_DOUBLE:
					case IS_BOOL:
					case IS_UNICODE:
						convert_to_string_ex(arg);
						*p = Z_STRVAL_PP(arg);
						*pl = Z_STRLEN_PP(arg);
						break;

					case IS_BINARY:
						*p = Z_BINVAL_PP(arg);
						*pl = Z_BINLEN_PP(arg);
						break;

					case IS_OBJECT: {
						if (Z_OBJ_HANDLER_PP(arg, cast_object)) {
							SEPARATE_ZVAL_IF_NOT_REF(arg);
							if (Z_OBJ_HANDLER_PP(arg, cast_object)(*arg, *arg, IS_STRING, 0 TSRMLS_CC) == SUCCESS) {
								*pl = Z_STRLEN_PP(arg);
								*p = Z_STRVAL_PP(arg);
								break;
							} else {
								return "native string";
							}
						}
					}
						
					case IS_ARRAY:
					case IS_RESOURCE:
					default:
						return "native string";
				}
			}
			break;

		case 'y':
			{
				char **p = va_arg(*va, char **);
				int *pl = va_arg(*va, int *);
				switch (Z_TYPE_PP(arg)) {
					case IS_NULL:
						if (return_null) {
							*p = NULL;
							*pl = 0;
							break;
						}
						/* break omitted intentionally */

					case IS_STRING:
					case IS_LONG:
					case IS_DOUBLE:
					case IS_BOOL:
					case IS_UNICODE:
						convert_to_binary_ex(arg);
						*p = Z_BINVAL_PP(arg);
						*pl = Z_BINLEN_PP(arg);
						break;

					case IS_BINARY:
						*p = Z_BINVAL_PP(arg);
						*pl = Z_BINLEN_PP(arg);
						break;

					case IS_OBJECT: {
						if (Z_OBJ_HANDLER_PP(arg, cast_object)) {
							SEPARATE_ZVAL_IF_NOT_REF(arg);
							if (Z_OBJ_HANDLER_PP(arg, cast_object)(*arg, *arg, IS_BINARY, 0 TSRMLS_CC) == SUCCESS) {
								*pl = Z_BINLEN_PP(arg);
								*p = Z_BINVAL_PP(arg);
								break;
							} else {
								return "binary data";
							}
						}
					}
						
					case IS_ARRAY:
					case IS_RESOURCE:
					default:
						return "binary data";
				}
			}
			break;

		case 'u':
			{
				UChar **p = va_arg(*va, UChar **);
				int32_t *pl = va_arg(*va, int32_t *);
				switch (Z_TYPE_PP(arg)) {
					case IS_NULL:
						if (return_null) {
							*p = NULL;
							*pl = 0;
							break;
						}
						/* break omitted intentionally */

					case IS_STRING:
					case IS_LONG:
					case IS_DOUBLE:
					case IS_BOOL:
					case IS_UNICODE:
						convert_to_unicode_ex(arg);
						*p = Z_USTRVAL_PP(arg);
						*pl = Z_USTRLEN_PP(arg);
						break;

					case IS_OBJECT: {
						if (Z_OBJ_HANDLER_PP(arg, cast_object)) {
							SEPARATE_ZVAL_IF_NOT_REF(arg);
							if (Z_OBJ_HANDLER_PP(arg, cast_object)(*arg, *arg, IS_UNICODE, 0 TSRMLS_CC) == SUCCESS) {
								*pl = Z_USTRLEN_PP(arg);
								*p = Z_USTRVAL_PP(arg);
								break;
							} else {
								return "Unicode string";
							}
						}
					}
						
					case IS_ARRAY:
					case IS_RESOURCE:
					default:
						return "Unicode string";
				}
			}
			break;

		case 'T':
			if (T_arg_type != -1)
			{
				void **p = va_arg(*va, void **);
				int32_t *pl = va_arg(*va, int32_t *);
				zend_uchar *type = va_arg(*va, zend_uchar *);
				switch (Z_TYPE_PP(arg)) {
					case IS_NULL:
						if (return_null) {
							*p = NULL;
							*pl = 0;
							*type = T_arg_type;
							break;
						}
						/* break omitted intentionally */

					case IS_LONG:
					case IS_DOUBLE:
					case IS_BOOL:
					case IS_STRING:
					case IS_UNICODE:
					case IS_BINARY:
						if (T_arg_type == IS_UNICODE) {
							convert_to_unicode_ex(arg);
							RETURN_AS_UNICODE(arg, p, pl, type);
						} else if (T_arg_type == IS_STRING) {
							convert_to_string_ex(arg);
							RETURN_AS_STRING(arg, p, pl, type);
						} else {
							if (Z_TYPE_PP(arg) == IS_UNICODE) {
								char *space;
								char *class_name = get_active_class_name(&space TSRMLS_CC);
								zend_error(E_WARNING, "%v%s%v() does not allow mixing binary and Unicode parameters",
										   class_name, space, get_active_function_name(TSRMLS_C));
								return "native or binary string";
							} else {
								convert_to_binary_ex(arg);
								RETURN_AS_BINARY(arg, p, pl, type);
							}
						}
						break;

					case IS_OBJECT: {
						if (Z_OBJ_HANDLER_PP(arg, cast_object)) {
							SEPARATE_ZVAL_IF_NOT_REF(arg);
							if (Z_OBJ_HANDLER_PP(arg, cast_object)(*arg, *arg, T_arg_type, 0 TSRMLS_CC) == SUCCESS) {
								*(char**)p = Z_UNIVAL_PP(arg);
								*pl = Z_UNILEN_PP(arg);
								*type = Z_TYPE_PP(arg);
								RETURN_AS_UNICODE(arg, p, pl, type);
								break;
							}
						}
					}
						
					case IS_ARRAY:
					case IS_RESOURCE:
					default:
						return "string (native, Unicode, or binary)";
				}

				break;
			}
			/* break omitted intentionally */

		case 't':
			{
				void **p = va_arg(*va, void **);
				int32_t *pl = va_arg(*va, int32_t *);
				zend_uchar *type = va_arg(*va, zend_uchar *);
				switch (Z_TYPE_PP(arg)) {
					case IS_NULL:
						if (return_null) {
							*p = NULL;
							*pl = 0;
							if (UG(unicode)) {
								*type = IS_UNICODE;
							} else {
								*type = IS_STRING;
							}
							break;
						}
						/* break omitted intentionally */

					case IS_LONG:
					case IS_DOUBLE:
					case IS_BOOL:
						if (UG(unicode)) {
							convert_to_unicode_ex(arg);
							RETURN_AS_UNICODE(arg, p, pl, type);
						} else {
							convert_to_string_ex(arg);
							RETURN_AS_STRING(arg, p, pl, type);
						}
						break;

					case IS_STRING:
						RETURN_AS_STRING(arg, p, pl, type);
						break;

					case IS_BINARY:
						RETURN_AS_BINARY(arg, p, pl, type);
						break;

					case IS_UNICODE:
						RETURN_AS_UNICODE(arg, p, pl, type);
						break;

					case IS_OBJECT: {
						if (Z_OBJ_HANDLER_PP(arg, cast_object)) {
							SEPARATE_ZVAL_IF_NOT_REF(arg);
							if (UG(unicode)) {
								if (Z_OBJ_HANDLER_PP(arg, cast_object)(*arg, *arg, IS_UNICODE, 0 TSRMLS_CC) == SUCCESS) {
									RETURN_AS_UNICODE(arg, p, pl, type);
									break;
								}
							} else {
								if (Z_OBJ_HANDLER_PP(arg, cast_object)(*arg, *arg, IS_STRING, 0 TSRMLS_CC) == SUCCESS) {
									RETURN_AS_STRING(arg, p, pl, type);
									break;
								}
							}
						}
					}
						
					case IS_ARRAY:
					case IS_RESOURCE:
					default:
						return "string (native, Unicode, or binary)";
				}
			}
			break;

		case 'b':
			{
				zend_bool *p = va_arg(*va, zend_bool *);
				switch (Z_TYPE_PP(arg)) {
					case IS_NULL:
					case IS_STRING:
					case IS_LONG:
					case IS_DOUBLE:
					case IS_BOOL:
						convert_to_boolean_ex(arg);
						*p = Z_BVAL_PP(arg);
						break;

					case IS_ARRAY:
					case IS_OBJECT:
					case IS_RESOURCE:
					default:
						return "boolean";
				}
			}
			break;

		case 'r':
			{
				zval **p = va_arg(*va, zval **);
				if (Z_TYPE_PP(arg) != IS_RESOURCE) {
					if (Z_TYPE_PP(arg) == IS_NULL && return_null) {
						*p = NULL;
					} else {
						return "resource";
					}
				} else
					*p = *arg;
			}
			break;

		case 'a':
			{
				zval **p = va_arg(*va, zval **);
				if (Z_TYPE_PP(arg) != IS_ARRAY) {
					if (Z_TYPE_PP(arg) == IS_NULL && return_null) {
						*p = NULL;
					} else {
						return "array";
					}
				} else
					*p = *arg;
			}
			break;

		case 'o':
			{
				zval **p = va_arg(*va, zval **);
				if (Z_TYPE_PP(arg) != IS_OBJECT) {
					if (Z_TYPE_PP(arg) == IS_NULL && return_null) {
						*p = NULL;
					} else {
						return "object";
					}
				} else
					*p = *arg;
			}
			break;

		case 'O':
			{
				zval **p = va_arg(*va, zval **);
				zend_class_entry *ce = va_arg(*va, zend_class_entry *);

				if (Z_TYPE_PP(arg) == IS_OBJECT &&
						(!ce || instanceof_function(Z_OBJCE_PP(arg), ce TSRMLS_CC))) {
					*p = *arg;
				} else {
					if (Z_TYPE_PP(arg) == IS_NULL && return_null) {
						*p = NULL;
					} else {
						return ce ? ce->name : "object";
					}
				}
			}
			break;

		case 'z':
			{
				zval **p = va_arg(*va, zval **);
				if (Z_TYPE_PP(arg) == IS_NULL && return_null) {
					*p = NULL;
				} else {
					*p = *arg;
				}
			}
			break;
		case 'Z':
			{
				zval ***p = va_arg(*va, zval ***);
				if (Z_TYPE_PP(arg) == IS_NULL && return_null) {
					*p = NULL;
				} else {
					*p = arg;
				}
			}
			break;
		default:
			return "unknown";
	}

	*spec = spec_walk;

	return NULL;
}

static int zend_parse_arg(int arg_num, zval **arg, va_list *va, char **spec, int quiet, char T_arg_type TSRMLS_DC)
{
	char *expected_type = NULL;

	expected_type = zend_parse_arg_impl(arg, va, spec, T_arg_type TSRMLS_CC);
	if (expected_type) {
		if (!quiet) {
			char *space;
			char *class_name = get_active_class_name(&space TSRMLS_CC);

			zend_error(E_WARNING, "%v%s%v() expects parameter %d to be %s, %s given",
					class_name, space, get_active_function_name(TSRMLS_C), arg_num, expected_type,
					zend_zval_type_name(*arg));
		}
		return FAILURE;
	}
	
	return SUCCESS;
}

static int zend_parse_va_args(int num_args, char *type_spec, va_list *va, int flags TSRMLS_DC)
{
	char *spec_walk;
	int c, i;
	int min_num_args = -1;
	int max_num_args = 0;
	zval **arg;
	void **p;
	int arg_count;
	int quiet = flags & ZEND_PARSE_PARAMS_QUIET;
	zend_bool T_present = 0;
	char T_arg_type = -1;

	for (spec_walk = type_spec; *spec_walk; spec_walk++) {
		c = *spec_walk;
		switch (c) {
			case 'T':
				T_present++;
				/* break omitted intentionally */
			case 'l': case 'd':
			case 's': case 'b':
			case 'r': case 'a':
			case 'o': case 'O':
			case 'z': case 'Z':
			case 't': case 'y':
			case 'u':
				max_num_args++;
				break;

			case '|':
				min_num_args = max_num_args;
				break;

			case '/':
			case '!':
				/* Pass */
				break;

			default:
				if (!quiet) {
					zend_function *active_function = EG(function_state_ptr)->function;
					char *class_name = active_function->common.scope ? active_function->common.scope->name : EMPTY_STR;
					zend_error(E_WARNING, "%v%s%v(): bad type specifier while parsing parameters", 
							class_name,
							class_name[0] ? "::" : "",
							get_active_function_name(TSRMLS_C));
				}
				return FAILURE;
		}
	}

	if (min_num_args < 0) {
		min_num_args = max_num_args;
	}

	if (num_args < min_num_args || num_args > max_num_args) {
		if (!quiet) {
			zend_function *active_function = EG(function_state_ptr)->function;
			char *class_name = active_function->common.scope ? active_function->common.scope->name : EMPTY_STR;
			zend_error(E_WARNING, "%v%s%v() expects %s %d parameter%s, %d given",
					class_name,
					class_name[0] ? "::" : "",
					get_active_function_name(TSRMLS_C),
					min_num_args == max_num_args ? "exactly" : num_args < min_num_args ? "at least" : "at most",
					num_args < min_num_args ? min_num_args : max_num_args,
					(num_args < min_num_args ? min_num_args : max_num_args) == 1 ? "" : "s",
					num_args);
		}
		return FAILURE;
	}

	p = EG(argument_stack).top_element-2;
	arg_count = (ulong) *p;

	if (num_args > arg_count) {
		zend_error(E_WARNING, "%v(): could not obtain parameters for parsing",
			get_active_function_name(TSRMLS_C));
		return FAILURE;
	}

	if (T_present > 1) {
		/* determine 'T' target argument type */
		for (spec_walk = type_spec, i = 0; *spec_walk && i < num_args; spec_walk++) {
			switch (*spec_walk) {
				case 'T':
					arg = (zval **) p - (arg_count-i);
					if (Z_TYPE_PP(arg) == IS_BINARY) {
						/* we can always convert to binary */
						T_arg_type = IS_BINARY;
					} else if (Z_TYPE_PP(arg) == IS_UNICODE && (T_arg_type == -1 || T_arg_type == IS_STRING)) {
						/* we can upgrade from strings to Unicode */
						T_arg_type = IS_UNICODE;
					} else if (Z_TYPE_PP(arg) == IS_STRING && T_arg_type == -1) {
						T_arg_type = IS_STRING;
					}
					i++;
					break;
	
				case '|': case '!':
				case '/':
					/* pass */
					break;

				default:
					i++;
					break;
			}
		}
	}

	i = 0;
	while (num_args-- > 0) {
		arg = (zval **) p - (arg_count-i);
		if (*type_spec == '|') {
			type_spec++;
		}
		if (zend_parse_arg(i+1, arg, va, &type_spec, quiet, T_arg_type TSRMLS_CC) == FAILURE) {
			return FAILURE;
		}
		i++;
	}

	return SUCCESS;
}

ZEND_API int zend_parse_parameters_ex(int flags, int num_args TSRMLS_DC, char *type_spec, ...)
{
	va_list va;
	int retval;
	
	va_start(va, type_spec);
	retval = zend_parse_va_args(num_args, type_spec, &va, flags TSRMLS_CC);
	va_end(va);

	return retval;
}

ZEND_API int zend_parse_parameters(int num_args TSRMLS_DC, char *type_spec, ...)
{
	va_list va;
	int retval;
	
	va_start(va, type_spec);
	retval = zend_parse_va_args(num_args, type_spec, &va, 0 TSRMLS_CC);
	va_end(va);

	return retval;
}

ZEND_API int zend_parse_method_parameters(int num_args TSRMLS_DC, zval *this_ptr, char *type_spec, ...)
{
	va_list va;
	int retval;
	char *p = type_spec;
	zval **object;
	zend_class_entry *ce;

	if (!this_ptr) {
		va_start(va, type_spec);
		retval = zend_parse_va_args(num_args, type_spec, &va, 0 TSRMLS_CC);
		va_end(va);
	} else {
		p++;
		va_start(va, type_spec);

		object = va_arg(va, zval **);
		ce = va_arg(va, zend_class_entry *);
		*object = this_ptr;
		if (ce && !instanceof_function(Z_OBJCE_P(this_ptr), ce TSRMLS_CC)) {
			zend_error(E_CORE_ERROR, "%v::%v() must be derived from %v::%v",
				ce->name, get_active_function_name(TSRMLS_C), Z_OBJCE_P(this_ptr)->name, get_active_function_name(TSRMLS_C));
		}

		retval = zend_parse_va_args(num_args, p, &va, 0 TSRMLS_CC);
		va_end(va);
	}
	return retval;
}


ZEND_API int zend_parse_method_parameters_ex(int flags, int num_args TSRMLS_DC, zval *this_ptr, char *type_spec, ...)
{
	va_list va;
	int retval;
	char *p = type_spec;
	zval **object;
	zend_class_entry *ce;
	int quiet = flags & ZEND_PARSE_PARAMS_QUIET;

	if (!this_ptr) {
		va_start(va, type_spec);
		retval = zend_parse_va_args(num_args, type_spec, &va, 0 TSRMLS_CC);
		va_end(va);
	} else {
		p++;
		va_start(va, type_spec);

		object = va_arg(va, zval **);
		ce = va_arg(va, zend_class_entry *);
		*object = this_ptr;
		if (ce && !instanceof_function(Z_OBJCE_P(this_ptr), ce TSRMLS_CC)) {
			if (!quiet) {
				zend_error(E_CORE_ERROR, "%v::%v() must be derived from %v::%v",
					ce->name, get_active_function_name(TSRMLS_C), Z_OBJCE_P(this_ptr)->name, get_active_function_name(TSRMLS_C));
			}
			return FAILURE;
		}

		retval = zend_parse_va_args(num_args, p, &va, flags TSRMLS_CC);
		va_end(va);
	}
	return retval;
}


/* Argument parsing API -- andrei */


ZEND_API int _array_init(zval *arg ZEND_FILE_LINE_DC)
{
	TSRMLS_FETCH();

	ALLOC_HASHTABLE_REL(arg->value.ht);

	zend_u_hash_init(arg->value.ht, 0, NULL, ZVAL_PTR_DTOR, 0, UG(unicode));
	arg->type = IS_ARRAY;
	return SUCCESS;
}


static int zend_merge_property(zval **value, int num_args, va_list args, zend_hash_key *hash_key)
{
	/* which name should a numeric property have ? */
	if (hash_key->nKeyLength) {
		zval *obj = va_arg(args, zval *);
		zend_object_handlers *obj_ht = va_arg(args, zend_object_handlers *);
		zval member;
		TSRMLS_FETCH();

		if (hash_key->type == IS_STRING) {
			ZVAL_STRINGL(&member, hash_key->u.string, hash_key->nKeyLength-1, 0);
		} else if (hash_key->type == IS_BINARY) {
			ZVAL_BINARYL(&member, hash_key->u.string, hash_key->nKeyLength-1, 0);
		} else if (hash_key->type == IS_UNICODE) {
			ZVAL_UNICODEL(&member, hash_key->u.unicode, hash_key->nKeyLength-1, 0);
		}

		obj_ht->write_property(obj, &member, *value TSRMLS_CC);
	}
	return ZEND_HASH_APPLY_KEEP;
}


/* This function should be called after the constructor has been called 
 * because it may call __set from the uninitialized object otherwise. */
ZEND_API void zend_merge_properties(zval *obj, HashTable *properties, int destroy_ht TSRMLS_DC)
{
	zend_object_handlers *obj_ht = Z_OBJ_HT_P(obj);
	zend_class_entry *old_scope = EG(scope);

	EG(scope) = Z_OBJCE_P(obj);
	zend_hash_apply_with_arguments(properties, (apply_func_args_t)zend_merge_property, 2, obj, obj_ht);
	EG(scope) = old_scope;

	if (destroy_ht) {
		zend_hash_destroy(properties);
		FREE_HASHTABLE(properties);
	}
}


ZEND_API void zend_update_class_constants(zend_class_entry *class_type TSRMLS_DC)
{
	if (!class_type->constants_updated) {
		zend_class_entry **scope = EG(in_execution)?&EG(scope):&CG(active_class_entry);
		zend_class_entry *old_scope = *scope;

		*scope = class_type;
		zend_hash_apply_with_argument(&class_type->default_properties, (apply_func_arg_t) zval_update_constant, (void *) 1 TSRMLS_CC);
		zend_hash_apply_with_argument(class_type->static_members, (apply_func_arg_t) zval_update_constant, (void *) 1 TSRMLS_CC);
		*scope = old_scope;
		class_type->constants_updated = 1;
	}
}


/* This function requires 'properties' to contain all props declared in the
 * class and all props being public. If only a subset is given or the class 
 * has protected members then you need to merge the properties seperately by
 * calling zend_merge_properties(). */
ZEND_API int _object_and_properties_init(zval *arg, zend_class_entry *class_type, HashTable *properties ZEND_FILE_LINE_DC TSRMLS_DC)
{
	zval *tmp;
	zend_object *object;

	if (class_type->ce_flags & (ZEND_ACC_INTERFACE|ZEND_ACC_IMPLICIT_ABSTRACT_CLASS|ZEND_ACC_EXPLICIT_ABSTRACT_CLASS)) {
		char *what = class_type->ce_flags & ZEND_ACC_INTERFACE ? "interface" : "abstract class";
		zend_error(E_ERROR, "Cannot instantiate %s %v", what, class_type->name);
	}

	zend_update_class_constants(class_type TSRMLS_CC);
	
	arg->type = IS_OBJECT;
	if (class_type->create_object == NULL) {
		arg->value.obj = zend_objects_new(&object, class_type TSRMLS_CC);
		if (properties) {
			object->properties = properties;
		} else {
			ALLOC_HASHTABLE_REL(object->properties);
			zend_u_hash_init(object->properties, 0, NULL, ZVAL_PTR_DTOR, 0, UG(unicode));
			zend_hash_copy(object->properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
		}
	} else {
		arg->value.obj = class_type->create_object(class_type TSRMLS_CC);
	}
	return SUCCESS;
}

ZEND_API int _object_init_ex(zval *arg, zend_class_entry *class_type ZEND_FILE_LINE_DC TSRMLS_DC)
{
	return _object_and_properties_init(arg, class_type, 0 ZEND_FILE_LINE_RELAY_CC TSRMLS_CC);
}

ZEND_API int _object_init(zval *arg ZEND_FILE_LINE_DC TSRMLS_DC)
{
	return _object_init_ex(arg, U_CLASS_ENTRY(zend_standard_class_def) ZEND_FILE_LINE_RELAY_CC TSRMLS_CC);
}


ZEND_API int add_assoc_function(zval *arg, char *key, void (*function_ptr)(INTERNAL_FUNCTION_PARAMETERS))
{
	zend_error(E_WARNING, "add_assoc_function() is no longer supported");
	return FAILURE;
}


ZEND_API int add_assoc_long_ex(zval *arg, char *key, uint key_len, long n)
{
	zval *tmp;

	MAKE_STD_ZVAL(tmp);
	ZVAL_LONG(tmp, n);
	
	return zend_symtable_update(Z_ARRVAL_P(arg), key, key_len, (void *) &tmp, sizeof(zval *), NULL);
}

ZEND_API int add_assoc_null_ex(zval *arg, char *key, uint key_len)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_NULL(tmp);
	
	return zend_symtable_update(Z_ARRVAL_P(arg), key, key_len, (void *) &tmp, sizeof(zval *), NULL);
}

ZEND_API int add_assoc_bool_ex(zval *arg, char *key, uint key_len, int b)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_BOOL(tmp, b);

	return zend_symtable_update(Z_ARRVAL_P(arg), key, key_len, (void *) &tmp, sizeof(zval *), NULL);
}

ZEND_API int add_assoc_resource_ex(zval *arg, char *key, uint key_len, int r)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_RESOURCE(tmp, r);
	
	return zend_symtable_update(Z_ARRVAL_P(arg), key, key_len, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_assoc_double_ex(zval *arg, char *key, uint key_len, double d)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_DOUBLE(tmp, d);

	return zend_symtable_update(Z_ARRVAL_P(arg), key, key_len, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_assoc_string_ex(zval *arg, char *key, uint key_len, char *str, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_STRING(tmp, str, duplicate);

	return zend_symtable_update(Z_ARRVAL_P(arg), key, key_len, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_assoc_stringl_ex(zval *arg, char *key, uint key_len, char *str, uint length, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_STRINGL(tmp, str, length, duplicate);

	return zend_symtable_update(Z_ARRVAL_P(arg), key, key_len, (void *) &tmp, sizeof(zval *), NULL);
}

ZEND_API int add_assoc_unicode_ex(zval *arg, char *key, uint key_len, void *str, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_UNICODE(tmp, str, duplicate);

	return zend_symtable_update(Z_ARRVAL_P(arg), key, key_len, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_assoc_unicodel_ex(zval *arg, char *key, uint key_len, void *str, uint length, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_UNICODEL(tmp, str, length, duplicate);

	return zend_symtable_update(Z_ARRVAL_P(arg), key, key_len, (void *) &tmp, sizeof(zval *), NULL);
}

ZEND_API int add_assoc_zval_ex(zval *arg, char *key, uint key_len, zval *value)
{
	return zend_symtable_update(Z_ARRVAL_P(arg), key, key_len, (void *) &value, sizeof(zval *), NULL);
}

ZEND_API int add_u_assoc_zval_ex(zval *arg, zend_uchar type, void *key, uint key_len, zval *value)
{
	return zend_u_symtable_update(Z_ARRVAL_P(arg), type, key, key_len, (void *) &value, sizeof(zval *), NULL);
}

ZEND_API int add_index_long(zval *arg, uint index, long n)
{
	zval *tmp;

	MAKE_STD_ZVAL(tmp);
	ZVAL_LONG(tmp, n);

	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_index_null(zval *arg, uint index)
{
	zval *tmp;

	MAKE_STD_ZVAL(tmp);
	ZVAL_NULL(tmp);

	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), NULL);
}

ZEND_API int add_index_bool(zval *arg, uint index, int b)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_BOOL(tmp, b);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_index_resource(zval *arg, uint index, int r)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_RESOURCE(tmp, r);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_index_double(zval *arg, uint index, double d)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_DOUBLE(tmp, d);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_index_string(zval *arg, uint index, char *str, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_STRING(tmp, str, duplicate);

	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_index_stringl(zval *arg, uint index, char *str, uint length, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_STRINGL(tmp, str, length, duplicate);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_index_binary(zval *arg, uint index, char *str, int duplicate TSRMLS_DC)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_BINARY(tmp, str, duplicate);

	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_index_binaryl(zval *arg, uint index, char *str, uint length, int duplicate TSRMLS_DC)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_BINARYL(tmp, str, length, duplicate);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_index_unicode(zval *arg, uint index, UChar *str, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_UNICODE(tmp, str, duplicate);

	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_index_unicodel(zval *arg, uint index, UChar *str, uint length, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_UNICODEL(tmp, str, length, duplicate);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_index_zval(zval *arg, uint index, zval *value)
{
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &value, sizeof(zval *), NULL);
}


ZEND_API int add_next_index_long(zval *arg, long n)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_LONG(tmp, n);
	
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_next_index_null(zval *arg)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_NULL(tmp);
	
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_next_index_bool(zval *arg, int b)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_BOOL(tmp, b);
	
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_next_index_resource(zval *arg, int r)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_RESOURCE(tmp, r);
	
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_next_index_double(zval *arg, double d)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_DOUBLE(tmp, d);
	
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_next_index_string(zval *arg, char *str, int duplicate)
{
	zval *tmp;

	MAKE_STD_ZVAL(tmp);
	ZVAL_STRING(tmp, str, duplicate);

	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_next_index_stringl(zval *arg, char *str, uint length, int duplicate)
{
	zval *tmp;

	MAKE_STD_ZVAL(tmp);
	ZVAL_STRINGL(tmp, str, length, duplicate);

	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_next_index_binary(zval *arg, char *str, int duplicate)
{
	zval *tmp;
	TSRMLS_FETCH();

	MAKE_STD_ZVAL(tmp);
	ZVAL_BINARY(tmp, str, duplicate);

	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_next_index_binaryl(zval *arg, char *str, uint length, int duplicate)
{
	zval *tmp;
	TSRMLS_FETCH();

	MAKE_STD_ZVAL(tmp);
	ZVAL_BINARYL(tmp, str, length, duplicate);

	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp, sizeof(zval *), NULL);
}

ZEND_API int add_next_index_unicode(zval *arg, UChar *str, int duplicate)
{
	zval *tmp;

	MAKE_STD_ZVAL(tmp);
	ZVAL_UNICODE(tmp, str, duplicate);

	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_next_index_unicodel(zval *arg, UChar *str, uint length, int duplicate)
{
	zval *tmp;

	MAKE_STD_ZVAL(tmp);
	ZVAL_UNICODEL(tmp, str, length, duplicate);

	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &tmp, sizeof(zval *), NULL);
}


ZEND_API int add_next_index_zval(zval *arg, zval *value)
{
	return zend_hash_next_index_insert(Z_ARRVAL_P(arg), &value, sizeof(zval *), NULL);
}


ZEND_API int add_get_assoc_string_ex(zval *arg, char *key, uint key_len, char *str, void **dest, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_STRING(tmp, str, duplicate);
	
	return zend_symtable_update(Z_ARRVAL_P(arg), key, key_len, (void *) &tmp, sizeof(zval *), dest);
}


ZEND_API int add_get_assoc_stringl_ex(zval *arg, char *key, uint key_len, char *str, uint length, void **dest, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_STRINGL(tmp, str, length, duplicate);

	return zend_symtable_update(Z_ARRVAL_P(arg), key, key_len, (void *) &tmp, sizeof(zval *), dest);
}


ZEND_API int add_get_index_long(zval *arg, uint index, long l, void **dest)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_LONG(tmp, l);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), dest);
}


ZEND_API int add_get_index_double(zval *arg, uint index, double d, void **dest)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_DOUBLE(tmp, d);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), dest);
}


ZEND_API int add_get_index_string(zval *arg, uint index, char *str, void **dest, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_STRING(tmp, str, duplicate);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), dest);
}


ZEND_API int add_get_index_stringl(zval *arg, uint index, char *str, uint length, void **dest, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_STRINGL(tmp, str, length, duplicate);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), dest);
}

ZEND_API int add_get_index_binary(zval *arg, uint index, char *str, void **dest, int duplicate)
{
	zval *tmp;
	TSRMLS_FETCH();
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_BINARY(tmp, str, duplicate);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), dest);
}


ZEND_API int add_get_index_binaryl(zval *arg, uint index, char *str, uint length, void **dest, int duplicate)
{
	zval *tmp;
	TSRMLS_FETCH();
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_BINARYL(tmp, str, length, duplicate);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), dest);
}


ZEND_API int add_get_index_unicode(zval *arg, uint index, UChar *str, void **dest, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_UNICODE(tmp, str, duplicate);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), dest);
}


ZEND_API int add_get_index_unicodel(zval *arg, uint index, UChar *str, uint length, void **dest, int duplicate)
{
	zval *tmp;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_UNICODEL(tmp, str, length, duplicate);
	
	return zend_hash_index_update(Z_ARRVAL_P(arg), index, (void *) &tmp, sizeof(zval *), dest);
}

ZEND_API int add_property_long_ex(zval *arg, char *key, uint key_len, long n TSRMLS_DC)
{
	zval *tmp;
	zval z_key;

	MAKE_STD_ZVAL(tmp);
	ZVAL_LONG(tmp, n);
	ZVAL_STRINGL(&z_key, key, key_len-1, 0);

	Z_OBJ_HANDLER_P(arg, write_property)(arg, &z_key, tmp TSRMLS_CC);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
	return SUCCESS;
}

ZEND_API int add_property_bool_ex(zval *arg, char *key, uint key_len, int b TSRMLS_DC)
{
	zval *tmp;
	zval z_key;

	MAKE_STD_ZVAL(tmp);
	ZVAL_BOOL(tmp, b);

	ZVAL_STRINGL(&z_key, key, key_len-1, 0);

	Z_OBJ_HANDLER_P(arg, write_property)(arg, &z_key, tmp TSRMLS_CC);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
	return SUCCESS;
}

ZEND_API int add_property_null_ex(zval *arg, char *key, uint key_len TSRMLS_DC)
{
	zval *tmp;
	zval z_key;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_NULL(tmp);
	
	ZVAL_STRINGL(&z_key, key, key_len-1, 0);

	Z_OBJ_HANDLER_P(arg, write_property)(arg, &z_key, tmp TSRMLS_CC);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
	return SUCCESS;
}

ZEND_API int add_property_resource_ex(zval *arg, char *key, uint key_len, long n TSRMLS_DC)
{
	zval *tmp;
	zval z_key;
	
	MAKE_STD_ZVAL(tmp);
	ZVAL_RESOURCE(tmp, n);
	ZVAL_STRINGL(&z_key, key, key_len-1, 0);

	Z_OBJ_HANDLER_P(arg, write_property)(arg, &z_key, tmp TSRMLS_CC);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
	return SUCCESS;
}


ZEND_API int add_property_double_ex(zval *arg, char *key, uint key_len, double d TSRMLS_DC)
{
	zval *tmp;
	zval z_key;

	MAKE_STD_ZVAL(tmp);
	ZVAL_DOUBLE(tmp, d);
	ZVAL_STRINGL(&z_key, key, key_len-1, 0);

	Z_OBJ_HANDLER_P(arg, write_property)(arg, &z_key, tmp TSRMLS_CC);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
	return SUCCESS;
}


ZEND_API int add_property_string_ex(zval *arg, char *key, uint key_len, char *str, int duplicate TSRMLS_DC)
{
	zval *tmp;
	zval z_key;

	MAKE_STD_ZVAL(tmp);
	ZVAL_STRING(tmp, str, duplicate);

	ZVAL_STRINGL(&z_key, key, key_len-1, 0);

	Z_OBJ_HANDLER_P(arg, write_property)(arg, &z_key, tmp TSRMLS_CC);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
	return SUCCESS;
}

ZEND_API int add_property_stringl_ex(zval *arg, char *key, uint key_len, char *str, uint length, int duplicate TSRMLS_DC)
{
	zval *tmp;
	zval z_key;

	MAKE_STD_ZVAL(tmp);
	ZVAL_STRINGL(tmp, str, length, duplicate);

	ZVAL_STRINGL(&z_key, key, key_len-1, 0);

	Z_OBJ_HANDLER_P(arg, write_property)(arg, &z_key, tmp TSRMLS_CC);
	zval_ptr_dtor(&tmp); /* write_property will add 1 to refcount */
	return SUCCESS;
}

ZEND_API int add_property_zval_ex(zval *arg, char *key, uint key_len, zval *value TSRMLS_DC)
{
	zval z_key;

	ZVAL_STRINGL(&z_key, key, key_len-1, 0);

	Z_OBJ_HANDLER_P(arg, write_property)(arg, &z_key, value TSRMLS_CC);
	return SUCCESS;
}

ZEND_API int zend_startup_module_ex(zend_module_entry *module TSRMLS_DC)
{
	int name_len;
	char *lcname;

	if (module->module_started) {
		return SUCCESS;
	}
	module->module_started = 1;

	/* Check module dependencies */
	if (module->deps) {
		zend_module_dep *dep = module->deps;

		while (dep->name) {
			if (dep->type == MODULE_DEP_REQUIRED) {
				zend_module_entry *req_mod;

				name_len = strlen(dep->name);
				lcname = zend_str_tolower_dup(dep->name, name_len);

				if (zend_hash_find(&module_registry, lcname, name_len+1, (void**)&req_mod) == FAILURE ||
				    !req_mod->module_started) {
					efree(lcname);
					/* TODO: Check version relationship */
					zend_error(E_CORE_WARNING, "Cannot load module '%s' because required module '%s' is not loaded", module->name, dep->name);
					module->module_started = 0;
					return FAILURE;
				}			
				efree(lcname);
			}
			++dep;
		}
	}

	if (module->module_startup_func) {
		EG(current_module) = module;
		if (module->module_startup_func(module->type, module->module_number TSRMLS_CC)==FAILURE) {
			zend_error(E_CORE_ERROR,"Unable to start %s module", module->name);
			EG(current_module) = NULL;
			return FAILURE;
		}
		EG(current_module) = NULL;
	}
	return SUCCESS;
}

static void zend_sort_modules(void *base, size_t count, size_t siz, compare_func_t compare TSRMLS_DC)
{
	Bucket **b1 = base;
	Bucket **b2;
	Bucket **end = b1 + count;
	Bucket *tmp;				  		
	zend_module_entry *m, *r;

	while (b1 < end) {
try_again:
		m = (zend_module_entry*)(*b1)->pData;
		if (!m->module_started && m->deps) {
			zend_module_dep *dep = m->deps;
			while (dep->name) {
				if (dep->type == MODULE_DEP_REQUIRED || dep->type == MODULE_DEP_OPTIONAL) {
					b2 = b1 + 1;
					while (b2 < end) {
				  	r = (zend_module_entry*)(*b2)->pData;
				  	if (strcasecmp(dep->name, r->name) == 0) {
				  		tmp  = *b1;
				  		*b1 = *b2;
				  		*b2 = tmp;				  		
				  	  goto try_again;
				  	}
				  	b2++;
				  }
			  }
			  dep++;
			}			
		}
		b1++;
	}
}

ZEND_API int zend_startup_modules(TSRMLS_D)
{
	zend_hash_sort(&module_registry, zend_sort_modules, NULL, 0 TSRMLS_CC);
	zend_hash_apply(&module_registry, (apply_func_t)zend_startup_module_ex TSRMLS_CC);
	return SUCCESS;
}

ZEND_API zend_module_entry* zend_register_module_ex(zend_module_entry *module TSRMLS_DC)
{
	int name_len;
	char *lcname;
	zend_module_entry *module_ptr;
	
	if (!module) {
		return NULL;
	}

#if 0
	zend_printf("%s:  Registering module %d\n", module->name, module->module_number);
#endif

	/* Check module dependencies */
	if (module->deps) {
		zend_module_dep *dep = module->deps;

		while (dep->name) {
			if (dep->type == MODULE_DEP_CONFLICTS) {
				name_len = strlen(dep->name);
				lcname = zend_str_tolower_dup(dep->name, name_len);

				if (zend_hash_exists(&module_registry, lcname, name_len+1)) {
					efree(lcname);
					/* TODO: Check version relationship */
					zend_error(E_CORE_WARNING, "Cannot load module '%s' because conflicting module '%s' is already loaded", module->name, dep->name);
					return NULL;
				}			
				efree(lcname);
			}
			++dep;
		}
	}

	name_len = strlen(module->name);
	lcname = zend_str_tolower_dup(module->name, name_len);

	if (zend_hash_add(&module_registry, lcname, name_len+1, (void *)module, sizeof(zend_module_entry), (void**)&module_ptr)==FAILURE) {
		zend_error(E_CORE_WARNING, "Module '%s' already loaded", module->name);
		efree(lcname);
		return NULL;
	}
	efree(lcname);
	module = module_ptr;

	if (module->functions && zend_register_functions(NULL, module->functions, NULL, module->type TSRMLS_CC)==FAILURE) {
		zend_error(E_CORE_WARNING,"%s:  Unable to register functions, unable to load", module->name);
		return NULL;
	}

	return module;
}

ZEND_API zend_module_entry* zend_register_internal_module(zend_module_entry *module TSRMLS_DC)
{
	module->module_number = zend_next_free_module();
	module->type = MODULE_PERSISTENT;
	return zend_register_module_ex(module TSRMLS_CC);
}

ZEND_API void zend_check_magic_method_implementation(zend_class_entry *ce, zend_function *fptr, int error_type TSRMLS_DC)
{
	unsigned int lcname_len;
	char *lcname;
	int name_len;
	zend_uchar utype = UG(unicode)?IS_UNICODE:IS_STRING;

	/* we don't care if the function name is longer, in fact lowercasing only 
	 * the beginning of the name speeds up the check process */
	if (UG(unicode)) {
		name_len = u_strlen((UChar*)fptr->common.function_name);
	} else {
		name_len = strlen(fptr->common.function_name);
	}
	lcname = zend_u_str_case_fold(utype, fptr->common.function_name, name_len, 0, &lcname_len);

	if (lcname_len == sizeof(ZEND_DESTRUCTOR_FUNC_NAME) - 1 && 
	    ZEND_U_EQUAL(utype, lcname, lcname_len, ZEND_DESTRUCTOR_FUNC_NAME, sizeof(ZEND_DESTRUCTOR_FUNC_NAME)-1) && fptr->common.num_args != 0) {
		zend_error(error_type, "Destuctor %v::%s() cannot take arguments", ce->name, ZEND_DESTRUCTOR_FUNC_NAME);
	} else if (lcname_len == sizeof(ZEND_CLONE_FUNC_NAME) - 1 &&
	    ZEND_U_EQUAL(utype, lcname, lcname_len, ZEND_CLONE_FUNC_NAME, sizeof(ZEND_CLONE_FUNC_NAME)-1) && fptr->common.num_args != 0) {
		zend_error(error_type, "Method %v::%s() cannot accept any arguments", ce->name, ZEND_CLONE_FUNC_NAME);
	} else if (lcname_len == sizeof(ZEND_GET_FUNC_NAME) - 1 &&
	    ZEND_U_EQUAL(utype, lcname, lcname_len, ZEND_GET_FUNC_NAME, sizeof(ZEND_GET_FUNC_NAME)-1) && fptr->common.num_args != 1) {
		zend_error(error_type, "Method %v::%s() must take exactly 1 argument", ce->name, ZEND_GET_FUNC_NAME);
	} else if (lcname_len == sizeof(ZEND_SET_FUNC_NAME) - 1 &&
	    ZEND_U_EQUAL(utype, lcname, lcname_len, ZEND_SET_FUNC_NAME, sizeof(ZEND_SET_FUNC_NAME)-1) && fptr->common.num_args != 2) {
		zend_error(error_type, "Method %v::%s() must take exactly 2 arguments", ce->name, ZEND_SET_FUNC_NAME);
	} else if (lcname_len == sizeof(ZEND_UNSET_FUNC_NAME) - 1 &&
	    ZEND_U_EQUAL(utype, lcname, lcname_len, ZEND_UNSET_FUNC_NAME, sizeof(ZEND_UNSET_FUNC_NAME)-1) && fptr->common.num_args != 1) {
		zend_error(error_type, "Method %v::%s() must take exactly 1 arguments", ce->name, ZEND_UNSET_FUNC_NAME);
	} else if (lcname_len == sizeof(ZEND_ISSET_FUNC_NAME) - 1 &&
	    ZEND_U_EQUAL(utype, lcname, lcname_len, ZEND_ISSET_FUNC_NAME, sizeof(ZEND_ISSET_FUNC_NAME)-1) && fptr->common.num_args != 1) {
		zend_error(error_type, "Method %v::%s() must take exactly 1 arguments", ce->name, ZEND_ISSET_FUNC_NAME);
	} else if (lcname_len == sizeof(ZEND_CALL_FUNC_NAME) - 1 &&
	    ZEND_U_EQUAL(utype, lcname, lcname_len, ZEND_CALL_FUNC_NAME, sizeof(ZEND_CALL_FUNC_NAME)-1) && fptr->common.num_args != 2) {
		zend_error(error_type, "Method %v::%s() must take exactly 2 arguments", ce->name, ZEND_CALL_FUNC_NAME);
	}
	efree(lcname);
}

/* registers all functions in *library_functions in the function hash */
ZEND_API int zend_register_functions(zend_class_entry *scope, zend_function_entry *functions, HashTable *function_table, int type TSRMLS_DC)
{
	zend_function_entry *ptr = functions;
	zend_function function, *reg_function;
	zend_internal_function *internal_function = (zend_internal_function *)&function;
	int count=0, unload=0;
	HashTable *target_function_table = function_table;
	int error_type;
	zend_function *ctor = NULL, *dtor = NULL, *clone = NULL, *__get = NULL, *__set = NULL, *__unset = NULL, *__isset = NULL, *__call = NULL;
	char *lowercase_name;
	int fname_len;
	char *lc_class_name;
	int class_name_len;

	if (type==MODULE_PERSISTENT) {
		error_type = E_CORE_WARNING;
	} else {
		error_type = E_WARNING;
	}

	if (!target_function_table) {
		target_function_table = CG(function_table);
	}
	internal_function->type = ZEND_INTERNAL_FUNCTION;
	
	if (scope) {
		class_name_len = scope->name_length;
		lc_class_name = zend_str_tolower_dup(scope->name, class_name_len);
	}

	while (ptr->fname) {
		internal_function->handler = ptr->handler;
		internal_function->function_name = ptr->fname;
		internal_function->scope = scope;
		internal_function->prototype = NULL;
		if (ptr->arg_info) {
			internal_function->arg_info = ptr->arg_info+1;
			internal_function->num_args = ptr->num_args;
			/* Currently you cannot denote that the function can accept less arguments than num_args */
			if (ptr->arg_info[0].required_num_args == -1) {
				internal_function->required_num_args = ptr->num_args;
			} else {
				internal_function->required_num_args = ptr->arg_info[0].required_num_args;
			}
			internal_function->pass_rest_by_reference = ptr->arg_info[0].pass_by_reference;
			internal_function->return_reference = ptr->arg_info[0].return_reference;
		} else {
			internal_function->arg_info = NULL;
			internal_function->num_args = 0;
			internal_function->required_num_args = 0;
			internal_function->pass_rest_by_reference = 0;
			internal_function->return_reference = 0;
		}
		if (ptr->flags) {
			if (!(ptr->flags & ZEND_ACC_PPP_MASK)) {
				zend_error(error_type, "Invalid access level for %s%s%s() - access must be exactly one of public, protected or private", scope ? scope->name : "", scope ? "::" : "", ptr->fname);
				internal_function->fn_flags = ZEND_ACC_PUBLIC;
			} else {
				internal_function->fn_flags = ptr->flags;
			}
		} else {
			internal_function->fn_flags = ZEND_ACC_PUBLIC;
		}
		if (ptr->flags & ZEND_ACC_ABSTRACT) {
			if (scope) {
				/* This is a class that must be abstract itself. Here we set the check info. */
				scope->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
				if (!(scope->ce_flags & ZEND_ACC_INTERFACE)) {
					/* Since the class is not an interface it needs to be declared as a abstract class. */
					/* Since here we are handling internal functions only we can add the keyword flag. */
					/* This time we set the flag for the keyword 'abstratc'. */
					scope->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
				}
			}
		} else {
			if (scope && (scope->ce_flags & ZEND_ACC_INTERFACE)) {
				zend_error(error_type, "Interface %s cannot contain non abstract method %s()", scope->name, ptr->fname);
				return FAILURE;
			}
			if (!internal_function->handler) {
				zend_error(error_type, "Method %s%s%s() cannot be a NULL function", scope ? scope->name : "", scope ? "::" : "", ptr->fname);
				zend_unregister_functions(functions, count, target_function_table TSRMLS_CC);
				return FAILURE;
			}
		}
		fname_len = strlen(ptr->fname);
		lowercase_name = do_alloca(fname_len+1);
		zend_str_tolower_copy(lowercase_name, ptr->fname, fname_len);
		if (zend_hash_add(target_function_table, lowercase_name, fname_len+1, &function, sizeof(zend_function), (void**)&reg_function) == FAILURE) {
			unload=1;
			free_alloca(lowercase_name);
			break;
		}
		if (scope) {
			/* Look for ctor, dtor, clone
			 * If it's an old-style constructor, store it only if we don't have
			 * a constructor already.
			 */
			if ((fname_len == class_name_len) && !memcmp(lowercase_name, lc_class_name, class_name_len+1) && !ctor) {
				ctor = reg_function;
			} else if ((fname_len == sizeof(ZEND_CONSTRUCTOR_FUNC_NAME)-1) && !memcmp(lowercase_name, ZEND_CONSTRUCTOR_FUNC_NAME, sizeof(ZEND_CONSTRUCTOR_FUNC_NAME))) {
				ctor = reg_function;
			} else if ((fname_len == sizeof(ZEND_DESTRUCTOR_FUNC_NAME)-1) && !memcmp(lowercase_name, ZEND_DESTRUCTOR_FUNC_NAME, sizeof(ZEND_DESTRUCTOR_FUNC_NAME))) {
				dtor = reg_function;
				if (internal_function->num_args) {
					zend_error(error_type, "Destructor %s::%s() cannot take arguments", scope->name, ptr->fname);
				}
			} else if ((fname_len == sizeof(ZEND_CLONE_FUNC_NAME)-1) && !memcmp(lowercase_name, ZEND_CLONE_FUNC_NAME, sizeof(ZEND_CLONE_FUNC_NAME))) {
				clone = reg_function;
			} else if ((fname_len == sizeof(ZEND_CALL_FUNC_NAME)-1) && !memcmp(lowercase_name, ZEND_CALL_FUNC_NAME, sizeof(ZEND_CALL_FUNC_NAME))) {
				__call = reg_function;
			} else if ((fname_len == sizeof(ZEND_GET_FUNC_NAME)-1) && !memcmp(lowercase_name, ZEND_GET_FUNC_NAME, sizeof(ZEND_GET_FUNC_NAME))) {
				__get = reg_function;
			} else if ((fname_len == sizeof(ZEND_SET_FUNC_NAME)-1) && !memcmp(lowercase_name, ZEND_SET_FUNC_NAME, sizeof(ZEND_SET_FUNC_NAME))) {
				__set = reg_function;
			} else if ((fname_len == sizeof(ZEND_UNSET_FUNC_NAME)-1) && !memcmp(lowercase_name, ZEND_UNSET_FUNC_NAME, sizeof(ZEND_UNSET_FUNC_NAME))) {
				__unset = reg_function;
			} else if ((fname_len == sizeof(ZEND_ISSET_FUNC_NAME)-1) && !memcmp(lowercase_name, ZEND_ISSET_FUNC_NAME, sizeof(ZEND_ISSET_FUNC_NAME))) {
				__isset = reg_function;
			} else {
				reg_function = NULL;
			}
			if (reg_function) {
				zend_check_magic_method_implementation(scope, reg_function, error_type TSRMLS_CC);
			}
		}
		ptr++;
		count++;
		free_alloca(lowercase_name);
	}
	if (unload) { /* before unloading, display all remaining bad function in the module */
		if (scope) {
			free_alloca(lc_class_name);
		}
		while (ptr->fname) {
			if (zend_hash_exists(target_function_table, ptr->fname, strlen(ptr->fname)+1)) {
				zend_error(error_type, "Function registration failed - duplicate name - %s%s%s", scope ? scope->name : "", scope ? "::" : "", ptr->fname);
			}
			ptr++;
		}
		zend_unregister_functions(functions, count, target_function_table TSRMLS_CC);
		return FAILURE;
	}
	if (scope) {
		scope->constructor = ctor;
		scope->destructor = dtor;
		scope->clone = clone;
		scope->__call = __call;
		scope->__get = __get;
		scope->__set = __set;
		scope->__unset = __unset;
		scope->__isset = __isset;
		if (ctor) {
			ctor->common.fn_flags |= ZEND_ACC_CTOR;
			if (ctor->common.fn_flags & ZEND_ACC_STATIC) {
				zend_error(error_type, "Constructor %s::%s() cannot be static", scope->name, ctor->common.function_name);
			}
			ctor->common.fn_flags &= ~ZEND_ACC_ALLOW_STATIC;
		}
		if (dtor) {
			dtor->common.fn_flags |= ZEND_ACC_DTOR;
			if (dtor->common.fn_flags & ZEND_ACC_STATIC) {
				zend_error(error_type, "Destructor %s::%s() cannot be static", scope->name, dtor->common.function_name);
			}
			dtor->common.fn_flags &= ~ZEND_ACC_ALLOW_STATIC;
		}
		if (clone) {
			clone->common.fn_flags |= ZEND_ACC_CLONE;
			if (clone->common.fn_flags & ZEND_ACC_STATIC) {
				zend_error(error_type, "Constructor %s::%s() cannot be static", scope->name, clone->common.function_name);
			}
			clone->common.fn_flags &= ~ZEND_ACC_ALLOW_STATIC;
		}
		if (__call) {
			if (__call->common.fn_flags & ZEND_ACC_STATIC) {
				zend_error(error_type, "Method %s::%s() cannot be static", scope->name, __call->common.function_name);
			}
			__call->common.fn_flags &= ~ZEND_ACC_ALLOW_STATIC;
		}
		if (__get) {
			if (__get->common.fn_flags & ZEND_ACC_STATIC) {
				zend_error(error_type, "Method %s::%s() cannot be static", scope->name, __get->common.function_name);
			}
			__get->common.fn_flags &= ~ZEND_ACC_ALLOW_STATIC;
		}
		if (__set) {
			if (__set->common.fn_flags & ZEND_ACC_STATIC) {
				zend_error(error_type, "Method %s::%s() cannot be static", scope->name, __set->common.function_name);
			}
			__set->common.fn_flags &= ~ZEND_ACC_ALLOW_STATIC;
		}
		if (__unset) {
			if (__unset->common.fn_flags & ZEND_ACC_STATIC) {
				zend_error(error_type, "Method %s::%s() cannot be static", scope->name, __unset->common.function_name);
			}
			__unset->common.fn_flags &= ~ZEND_ACC_ALLOW_STATIC;
		}
		if (__isset) {
			if (__isset->common.fn_flags & ZEND_ACC_STATIC) {
				zend_error(error_type, "Method %s::%s() cannot be static", scope->name, __isset->common.function_name);
			}
			__isset->common.fn_flags &= ~ZEND_ACC_ALLOW_STATIC;
		}
		efree(lc_class_name);
	}
	return SUCCESS;
}

/* count=-1 means erase all functions, otherwise, 
 * erase the first count functions
 */
ZEND_API void zend_unregister_functions(zend_function_entry *functions, int count, HashTable *function_table TSRMLS_DC)
{
	zend_function_entry *ptr = functions;
	int i=0;
	HashTable *target_function_table = function_table;

	if (!target_function_table) {
		target_function_table = CG(function_table);
	}
	while (ptr->fname) {
		if (count!=-1 && i>=count) {
			break;
		}
#if 0
		zend_printf("Unregistering %s()\n", ptr->fname);
#endif
		zend_hash_del(target_function_table, ptr->fname, strlen(ptr->fname)+1);
		ptr++;
		i++;
	}
}


ZEND_API int zend_startup_module(zend_module_entry *module)
{
	TSRMLS_FETCH();
	
	if ((module = zend_register_internal_module(module TSRMLS_CC)) != NULL &&
	    zend_startup_module_ex(module TSRMLS_CC) == SUCCESS) {
		return SUCCESS;
	}
	return FAILURE;
}


ZEND_API int zend_get_module_started(char *module_name)
{
	zend_module_entry *module;
	
	return (zend_hash_find(&module_registry, module_name, strlen(module_name)+1, (void**)&module) == SUCCESS && module->module_started) ? SUCCESS : FAILURE;
}


void module_destructor(zend_module_entry *module)
{
	TSRMLS_FETCH();

	if (module->type == MODULE_TEMPORARY) {
		zend_clean_module_rsrc_dtors(module->module_number TSRMLS_CC);
		clean_module_constants(module->module_number TSRMLS_CC);
	}

	if (module->module_started && module->module_shutdown_func) {
#if 0
		zend_printf("%s:  Module shutdown\n", module->name);
#endif
		module->module_shutdown_func(module->type, module->module_number TSRMLS_CC);
	}
	module->module_started=0;
	if (module->functions) {
		zend_unregister_functions(module->functions, -1, NULL TSRMLS_CC);
	}

#if HAVE_LIBDL || defined(HAVE_MACH_O_DYLD_H)
#if !(defined(NETWARE) && defined(APACHE_1_BUILD))
	if (module->handle) {
		DL_UNLOAD(module->handle);
	}
#endif
#endif
}


/* call request startup for all modules */
int module_registry_request_startup(zend_module_entry *module TSRMLS_DC)
{
	if (module->request_startup_func) {
#if 0
		zend_printf("%s:  Request startup\n", module->name);
#endif
		if (module->request_startup_func(module->type, module->module_number TSRMLS_CC)==FAILURE) {
			zend_error(E_WARNING, "request_startup() for %s module failed", module->name);
			exit(1);
		}
	}
	return 0;
}


/* call request shutdown for all modules */
int module_registry_cleanup(zend_module_entry *module TSRMLS_DC)
{
	if (module->request_shutdown_func) {
#if 0
		zend_printf("%s:  Request shutdown\n", module->name);
#endif
		module->request_shutdown_func(module->type, module->module_number TSRMLS_CC);
	}
	return 0;
}

int module_registry_unload_temp(zend_module_entry *module TSRMLS_DC)
{
	switch (module->type) {
		case MODULE_TEMPORARY:
			return 1;
	}
	return 0;
}


/* return the next free module number */
int zend_next_free_module(void)
{
	return ++module_count;
}

static zend_class_entry *do_register_internal_class(zend_class_entry *orig_class_entry, zend_uint ce_flags TSRMLS_DC)
{
	zend_class_entry *class_entry = malloc(sizeof(zend_class_entry));
	char *lowercase_name = malloc(orig_class_entry->name_length + 1);
	*class_entry = *orig_class_entry;

	class_entry->type = ZEND_INTERNAL_CLASS;
	zend_initialize_class_data(class_entry, 0 TSRMLS_CC);
	class_entry->ce_flags = ce_flags;
	class_entry->module = EG(current_module);

	if (class_entry->builtin_functions) {
		zend_register_functions(class_entry, class_entry->builtin_functions, &class_entry->function_table, MODULE_PERSISTENT TSRMLS_CC);
	}

	zend_str_tolower_copy(lowercase_name, orig_class_entry->name, class_entry->name_length);
	zend_hash_update(CG(class_table), lowercase_name, class_entry->name_length+1, &class_entry, sizeof(zend_class_entry *), NULL);
	free(lowercase_name);
	return class_entry;
}

/* If parent_ce is not NULL then it inherits from parent_ce
 * If parent_ce is NULL and parent_name isn't then it looks for the parent and inherits from it
 * If both parent_ce and parent_name are NULL it does a regular class registration
 * If parent_name is specified but not found NULL is returned
 */
ZEND_API zend_class_entry *zend_register_internal_class_ex(zend_class_entry *class_entry, zend_class_entry *parent_ce, char *parent_name TSRMLS_DC)
{
	zend_class_entry *register_class;

	if (!parent_ce && parent_name) {
		zend_class_entry **pce;
		if (zend_hash_find(CG(class_table), parent_name, strlen(parent_name)+1, (void **) &pce)==FAILURE) {
			return NULL;
		} else {
			parent_ce = *pce;
		}
	}

	register_class = zend_register_internal_class(class_entry TSRMLS_CC);

	if (parent_ce) {
		zend_do_inheritance(register_class, parent_ce TSRMLS_CC);
	}
	return register_class;
}

ZEND_API void zend_class_implements(zend_class_entry *class_entry TSRMLS_DC, int num_interfaces, ...)
{
	zend_class_entry *interface_entry;
	int ce_num = class_entry->num_interfaces, impl_num;
	va_list interface_list;
	va_start(interface_list, num_interfaces);

	if (class_entry->type & ZEND_INTERNAL_CLASS) {
		class_entry->interfaces = realloc(class_entry->interfaces, sizeof(zend_class_entry*) * (class_entry->num_interfaces+num_interfaces));
	} else {
		class_entry->interfaces = erealloc(class_entry->interfaces, sizeof(zend_class_entry*) * (class_entry->num_interfaces+num_interfaces));
	}
	
	while (num_interfaces--) {
		interface_entry = va_arg(interface_list, zend_class_entry *);
		class_entry->interfaces[class_entry->num_interfaces++] = interface_entry;
	}
	impl_num = class_entry->num_interfaces;
	while(ce_num < impl_num) {
		zend_do_implement_interface(class_entry, class_entry->interfaces[ce_num++] TSRMLS_CC);
	}
	va_end(interface_list);
}

/* A class that contains at least one abstract method automatically becomes an abstract class.
 */
ZEND_API zend_class_entry *zend_register_internal_class(zend_class_entry *orig_class_entry TSRMLS_DC)
{
	return do_register_internal_class(orig_class_entry, 0 TSRMLS_CC);
}

ZEND_API zend_class_entry *zend_register_internal_interface(zend_class_entry *orig_class_entry TSRMLS_DC)
{
	return do_register_internal_class(orig_class_entry, ZEND_ACC_INTERFACE TSRMLS_CC);
}

ZEND_API int zend_set_hash_symbol(zval *symbol, char *name, int name_length,
                                  zend_bool is_ref, int num_symbol_tables, ...)
{
	HashTable  *symbol_table;
	va_list     symbol_table_list;

	if (num_symbol_tables <= 0) return FAILURE;

	symbol->is_ref = is_ref;

	va_start(symbol_table_list, num_symbol_tables);
	while (num_symbol_tables-- > 0) {
		symbol_table = va_arg(symbol_table_list, HashTable *);
		zend_hash_update(symbol_table, name, name_length + 1, &symbol, sizeof(zval *), NULL);
		zval_add_ref(&symbol);
	}
	va_end(symbol_table_list);
	return SUCCESS;
}




/* Disabled functions support */

ZEND_API ZEND_FUNCTION(display_disabled_function)
{
	zend_error(E_WARNING, "%v() has been disabled for security reasons", get_active_function_name(TSRMLS_C));
}


static zend_function_entry disabled_function[] =  {
	ZEND_FE(display_disabled_function,			NULL)
	{ NULL, NULL, NULL }
};


ZEND_API int zend_disable_function(char *function_name, uint function_name_length TSRMLS_DC)
{
	if (zend_hash_del(CG(function_table), function_name, function_name_length+1)==FAILURE) {
		return FAILURE;
	}
	disabled_function[0].fname = function_name;
	return zend_register_functions(NULL, disabled_function, CG(function_table), MODULE_PERSISTENT TSRMLS_CC);
}

static zend_object_value display_disabled_class(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	zend_object *intern;
	retval = zend_objects_new(&intern, class_type TSRMLS_CC);
	ALLOC_HASHTABLE(intern->properties);
	zend_u_hash_init(intern->properties, 0, NULL, ZVAL_PTR_DTOR, 0, UG(unicode));
	zend_error(E_WARNING, "%v() has been disabled for security reasons", class_type->name);
	return retval;
}

static zend_function_entry disabled_class_new[] =  {
	{ NULL, NULL, NULL }
};

ZEND_API int zend_disable_class(char *class_name, uint class_name_length TSRMLS_DC)
{
	zend_class_entry *disabled_class;
	disabled_class = (zend_class_entry *) emalloc(sizeof(zend_class_entry));
	
	zend_str_tolower(class_name, class_name_length);
	if (zend_hash_del(CG(class_table), class_name, class_name_length+1)==FAILURE) {
		return FAILURE;
	}
	INIT_CLASS_ENTRY((*disabled_class), class_name, disabled_class_new);
	disabled_class->create_object = display_disabled_class;
	zend_register_internal_class(disabled_class TSRMLS_CC);
	return 1;
}

ZEND_API zend_bool zend_is_callable_ex(zval *callable, uint check_flags, char **callable_name, int *callable_name_len, zend_function **fptr_ptr, zval ***zobj_ptr_ptr TSRMLS_DC)
{
	unsigned int lcname_len;
	char *lcname;
	zend_bool retval = 0; 
	int callable_name_len_local;
	zend_function *fptr_local;
	zval **zobj_ptr_local;

	if (callable_name_len == NULL) {
		callable_name_len = &callable_name_len_local;
	}
	if (fptr_ptr == NULL) {
		fptr_ptr = &fptr_local;
	}
	if (zobj_ptr_ptr == NULL) {
		zobj_ptr_ptr = &zobj_ptr_local;
	}
	*fptr_ptr = NULL;
	*zobj_ptr_ptr = NULL;

	switch (Z_TYPE_P(callable)) {
		case IS_STRING:
		case IS_UNICODE:
			if (callable_name) {
				/* UTODO: we need to return callable name type as well */
				if (Z_TYPE_P(callable) == IS_UNICODE) {
					*callable_name = (char*)eustrndup(Z_USTRVAL_P(callable), Z_USTRLEN_P(callable));
				} else {
					*callable_name = estrndup(Z_STRVAL_P(callable), Z_STRLEN_P(callable));
				}
				*callable_name_len = Z_UNILEN_P(callable);
			}
			if (check_flags & IS_CALLABLE_CHECK_SYNTAX_ONLY) {
				return 1;
			}

			lcname = zend_u_str_case_fold(Z_TYPE_P(callable), Z_UNIVAL_P(callable), Z_UNILEN_P(callable), 1, &lcname_len);
			if (zend_u_hash_find(EG(function_table), Z_TYPE_P(callable), lcname, lcname_len+1, (void**)fptr_ptr) == SUCCESS) {
				retval = 1;
			}
			efree(lcname);
			break;

		case IS_ARRAY:
			{
				zval **method;
				zval **obj;
				zend_class_entry *ce = NULL, **pce;
				
				if (zend_hash_num_elements(Z_ARRVAL_P(callable)) == 2 &&
					zend_hash_index_find(Z_ARRVAL_P(callable), 0, (void **) &obj) == SUCCESS &&
					zend_hash_index_find(Z_ARRVAL_P(callable), 1, (void **) &method) == SUCCESS &&
					(Z_TYPE_PP(obj) == IS_OBJECT ||
					Z_TYPE_PP(obj) == IS_STRING ||
					Z_TYPE_PP(obj) == IS_UNICODE) &&
					(Z_TYPE_PP(method) == IS_STRING ||
					 Z_TYPE_PP(method) == IS_UNICODE)) {

					if (Z_TYPE_PP(obj) == IS_STRING || Z_TYPE_PP(obj) == IS_UNICODE) {
						if (callable_name) {
							char *ptr;

							*callable_name_len = Z_STRLEN_PP(obj) + Z_STRLEN_PP(method) + sizeof("::") - 1;
							ptr = *callable_name = emalloc(*callable_name_len + 1);
							memcpy(ptr, Z_STRVAL_PP(obj), Z_STRLEN_PP(obj));
							ptr += Z_STRLEN_PP(obj);
							memcpy(ptr, "::", sizeof("::") - 1);
							ptr += sizeof("::") - 1;
							memcpy(ptr, Z_STRVAL_PP(method), Z_STRLEN_PP(method) + 1);
						}

						if (check_flags & IS_CALLABLE_CHECK_SYNTAX_ONLY)
							return 1;

						lcname = zend_u_str_case_fold(Z_TYPE_PP(obj), Z_UNIVAL_PP(obj), Z_UNILEN_PP(obj), 1, &lcname_len);

						if (EG(active_op_array) &&
						    lcname_len == sizeof("self")-1 &&
						    ZEND_U_EQUAL(Z_TYPE_PP(obj), lcname, lcname_len, "self", sizeof("self")-1)) {
							ce = EG(active_op_array)->scope;
						} else if (EG(active_op_array) && EG(active_op_array)->scope &&
						    lcname_len == sizeof("parent")-1 &&
						    ZEND_U_EQUAL(Z_TYPE_PP(obj), lcname, lcname_len, "parent", sizeof("parent")-1)) {
							ce = EG(active_op_array)->scope->parent;
						} else if (zend_u_lookup_class(Z_TYPE_PP(obj), Z_UNIVAL_PP(obj), Z_UNILEN_PP(obj), &pce TSRMLS_CC) == SUCCESS) {
							ce = *pce;
						}
						
						efree(lcname);
					} else {
						ce = Z_OBJCE_PP(obj); /* TBFixed: what if it's overloaded? */
						
						*zobj_ptr_ptr = obj;

						if (callable_name) {
							char *ptr;

							*callable_name_len = ce->name_length + Z_STRLEN_PP(method) + sizeof("::") - 1;
							ptr = *callable_name = emalloc(*callable_name_len + 1);
							memcpy(ptr, ce->name, ce->name_length);
							ptr += ce->name_length;
							memcpy(ptr, "::", sizeof("::") - 1);
							ptr += sizeof("::") - 1;
							memcpy(ptr, Z_STRVAL_PP(method), Z_STRLEN_PP(method) + 1);
						}

						if (check_flags & IS_CALLABLE_CHECK_SYNTAX_ONLY)
							return 1;
					}

					if (ce) {
						zend_function *fbc;

						lcname = zend_u_str_case_fold(Z_TYPE_PP(method), Z_UNIVAL_PP(method), Z_UNILEN_PP(method), 1, &lcname_len);
						if (zend_u_hash_find(&ce->function_table, Z_TYPE_PP(method), lcname, lcname_len+1, (void **)&fbc) == SUCCESS) {
 							retval = 1;
							if ((check_flags & IS_CALLABLE_CHECK_NO_ACCESS) == 0) {
								if (fbc->op_array.fn_flags & ZEND_ACC_PRIVATE) {
									if (!zend_check_private(fbc, (Z_TYPE_PP(obj) != IS_OBJECT)?EG(scope):(*obj)->value.obj.handlers->get_class_entry(*obj TSRMLS_CC), lcname, Z_UNILEN_PP(method) TSRMLS_CC)) {
										retval = 0;
									}
								} else if ((fbc->common.fn_flags & ZEND_ACC_PROTECTED)) {
									if (!zend_check_protected(fbc->common.scope, EG(scope))) {
										retval = 0;
									}
								}
							}
							if (retval) {
								if ((check_flags & IS_CALLABLE_CHECK_IS_STATIC) != 0 && (fbc->common.fn_flags & ZEND_ACC_STATIC) == 0) {
									retval = 0;
								} else {
									*fptr_ptr = fbc;
								}
							}
						}
						/* check for __call too */
						if (retval == 0 && ce->__call != 0) {
							retval = 1;
						}
						efree(lcname);
					}
				} else if (callable_name) {
					*callable_name = estrndup("Array", sizeof("Array")-1);
					*callable_name_len = sizeof("Array") - 1;
				}
			}
			break;

		default:
			if (callable_name) {
				zval expr_copy;
				int use_copy;

				zend_make_printable_zval(callable, &expr_copy, &use_copy);
				*callable_name = estrndup(Z_STRVAL(expr_copy), Z_STRLEN(expr_copy));
				*callable_name_len = Z_STRLEN(expr_copy);
				zval_dtor(&expr_copy);
			}
			break;
	}

	return retval;
}


ZEND_API zend_bool zend_is_callable(zval *callable, uint check_flags, char **callable_name)
{
	TSRMLS_FETCH();

	return zend_is_callable_ex(callable, check_flags, callable_name, NULL, NULL, NULL TSRMLS_CC);
}


ZEND_API zend_bool zend_make_callable(zval *callable, char **callable_name TSRMLS_DC)
{
	char *lcname, *func, *class_name;
	zend_bool retval = 0;
	zend_class_entry **pce;
	int class_name_len;

	if (zend_is_callable_ex(callable, 0, callable_name, NULL, NULL, NULL TSRMLS_CC)) {
		return 1;
	}
	switch (Z_TYPE_P(callable)) {
		case IS_STRING:
			lcname = zend_str_tolower_dup(Z_STRVAL_P(callable), Z_STRLEN_P(callable));
			
			if ((func = strstr(lcname, "::")) != NULL) {
				*func = '\0';
				class_name_len = func - lcname;
				class_name = estrndup(Z_STRVAL_P(callable), class_name_len);
				if (zend_lookup_class(class_name, class_name_len, &pce TSRMLS_CC) == SUCCESS) {
					zval_dtor(callable);
					array_init(callable);
					add_next_index_stringl(callable, lcname, class_name_len, 1);
					func += 2;
					add_next_index_stringl(callable, func, strlen(func), 1);
					retval = 1; 
				}
				efree(class_name);
			}
			efree(lcname);
			break;
	}
	return retval;
}


ZEND_API char *zend_get_module_version(char *module_name)
{
	zend_module_entry *module;

	if (zend_hash_find(&module_registry, module_name, strlen(module_name) + 1,
	                   (void**)&module) == FAILURE) {
		return NULL;
	}
	return module->version;
}

ZEND_API int zend_u_declare_property_ex(zend_class_entry *ce, zend_uchar type, void *name, int name_length, zval *property, int access_type, char *doc_comment, int doc_comment_len TSRMLS_DC)
{
	zend_property_info property_info;
	HashTable *target_symbol_table;

	if (!(access_type & ZEND_ACC_PPP_MASK)) {
		access_type |= ZEND_ACC_PUBLIC;
	}
	if (access_type & ZEND_ACC_STATIC) {
		target_symbol_table = ce->static_members;
	} else {
		target_symbol_table = &ce->default_properties;
	}
	if (ce->type & ZEND_INTERNAL_CLASS) {
		switch(Z_TYPE_P(property)) {
			case IS_ARRAY:
			case IS_CONSTANT_ARRAY:
			case IS_OBJECT:
			case IS_RESOURCE:
				zend_error(E_CORE_ERROR, "Internal zval's can't be arrays, objects or resources");
				break;
			default:
				break;
		}
	}
	switch (access_type & ZEND_ACC_PPP_MASK) {
		case ZEND_ACC_PRIVATE: {
				char *priv_name;
				int priv_name_length;

				zend_u_mangle_property_name(&priv_name, &priv_name_length, type, ce->name, ce->name_length, name, name_length, ce->type & ZEND_INTERNAL_CLASS);
				zend_u_hash_update(target_symbol_table, type, priv_name, priv_name_length+1, &property, sizeof(zval *), NULL);
				property_info.name = priv_name;
				property_info.name_length = priv_name_length;
			}
			break;
		case ZEND_ACC_PROTECTED: {
				char *prot_name;
				int prot_name_length;

				zend_u_mangle_property_name(&prot_name, &prot_name_length, type, "*", 1, name, name_length, ce->type & ZEND_INTERNAL_CLASS);
				zend_u_hash_update(target_symbol_table, type, prot_name, prot_name_length+1, &property, sizeof(zval *), NULL);
				property_info.name = prot_name;
				property_info.name_length = prot_name_length;
			}
			break;
		case ZEND_ACC_PUBLIC:
			if (ce->parent) {
				char *prot_name;
				int prot_name_length;

				zend_u_mangle_property_name(&prot_name, &prot_name_length, type, "*", 1, name, name_length, ce->type & ZEND_INTERNAL_CLASS);
				zend_u_hash_del(target_symbol_table, type, prot_name, prot_name_length+1);
				pefree(prot_name, ce->type & ZEND_INTERNAL_CLASS);
			}
			zend_u_hash_update(target_symbol_table, type, name, name_length+1, &property, sizeof(zval *), NULL);
			property_info.name = ce->type & ZEND_INTERNAL_CLASS ? 
				(type==IS_UNICODE?(char*)zend_ustrndup(name, name_length):zend_strndup(name, name_length)) : 
				(type==IS_UNICODE?(char*)eustrndup(name, name_length):estrndup(name, name_length));
			property_info.name_length = name_length;
			break;
	}
	property_info.flags = access_type;
	property_info.h = zend_u_get_hash_value(type, property_info.name, property_info.name_length+1);

	property_info.doc_comment = doc_comment;
	property_info.doc_comment_len = doc_comment_len;
	
	zend_u_hash_update(&ce->properties_info, type, name, name_length + 1, &property_info, sizeof(zend_property_info), NULL);

	return SUCCESS;
}

ZEND_API int zend_declare_property_ex(zend_class_entry *ce, char *name, int name_length, zval *property, int access_type, char *doc_comment, int doc_comment_len TSRMLS_DC)
{
	return zend_u_declare_property_ex(ce, IS_STRING, name, name_length, property, access_type, NULL, 0 TSRMLS_CC);
}

ZEND_API int zend_u_declare_property(zend_class_entry *ce, zend_uchar type, void *name, int name_length, zval *property, int access_type TSRMLS_DC)
{
	return zend_u_declare_property_ex(ce, type, name, name_length, property, access_type, NULL, 0 TSRMLS_CC);
}

ZEND_API int zend_declare_property(zend_class_entry *ce, char *name, int name_length, zval *property, int access_type TSRMLS_DC)
{
	return zend_u_declare_property_ex(ce, IS_STRING, name, name_length, property, access_type, NULL, 0 TSRMLS_CC);
}

ZEND_API int zend_declare_property_null(zend_class_entry *ce, char *name, int name_length, int access_type TSRMLS_DC)
{
	zval *property;
	
	if (ce->type & ZEND_INTERNAL_CLASS) {
		property = malloc(sizeof(zval));
	} else {
		ALLOC_ZVAL(property);
	}
	INIT_ZVAL(*property);
	return zend_declare_property(ce, name, name_length, property, access_type TSRMLS_CC);
}

ZEND_API int zend_declare_property_bool(zend_class_entry *ce, char *name, int name_length, long value, int access_type TSRMLS_DC)
{
	zval *property;
	
	if (ce->type & ZEND_INTERNAL_CLASS) {
		property = malloc(sizeof(zval));
	} else {
		ALLOC_ZVAL(property);
	}
	INIT_PZVAL(property);
	ZVAL_BOOL(property, value);
	return zend_declare_property(ce, name, name_length, property, access_type TSRMLS_CC);
}

ZEND_API int zend_declare_property_long(zend_class_entry *ce, char *name, int name_length, long value, int access_type TSRMLS_DC)
{
	zval *property;
	
	if (ce->type & ZEND_INTERNAL_CLASS) {
		property = malloc(sizeof(zval));
	} else {
		ALLOC_ZVAL(property);
	}
	INIT_PZVAL(property);
	ZVAL_LONG(property, value);
	return zend_declare_property(ce, name, name_length, property, access_type TSRMLS_CC);
}

ZEND_API int zend_declare_property_double(zend_class_entry *ce, char *name, int name_length, double value, int access_type TSRMLS_DC)
{
	zval *property;
	
	if (ce->type & ZEND_INTERNAL_CLASS) {
		property = malloc(sizeof(zval));
	} else {
		ALLOC_ZVAL(property);
	}
	INIT_PZVAL(property);
	ZVAL_DOUBLE(property, value);
	return zend_declare_property(ce, name, name_length, property, access_type TSRMLS_CC);
}

ZEND_API int zend_declare_property_string(zend_class_entry *ce, char *name, int name_length, char *value, int access_type TSRMLS_DC)
{
	zval *property;
	int len = strlen(value);
	
	if (ce->type & ZEND_INTERNAL_CLASS) {
		property = malloc(sizeof(zval));
		ZVAL_STRINGL(property, zend_strndup(value, len), len, 0);
	} else {
		ALLOC_ZVAL(property);
		ZVAL_STRINGL(property, value, len, 1);
	}
	INIT_PZVAL(property);
	return zend_declare_property(ce, name, name_length, property, access_type TSRMLS_CC);
}

ZEND_API int zend_declare_property_stringl(zend_class_entry *ce, char *name, int name_length, char *value, int value_len, int access_type TSRMLS_DC)
{
	zval *property;
	
	if (ce->type & ZEND_INTERNAL_CLASS) {
		property = malloc(sizeof(zval));
		ZVAL_STRINGL(property, zend_strndup(value, value_len), value_len, 0);
	} else {
		ALLOC_ZVAL(property);
		ZVAL_STRINGL(property, value, value_len, 1);
	}
	INIT_PZVAL(property);
	return zend_declare_property(ce, name, name_length, property, access_type TSRMLS_CC);
}

ZEND_API void zend_update_property(zend_class_entry *scope, zval *object, char *name, int name_length, zval *value TSRMLS_DC)
{
	zval property;
	zend_class_entry *old_scope = EG(scope);
	
	EG(scope) = scope;

	if (!Z_OBJ_HT_P(object)->write_property) {
		char *class_name;
		zend_uint class_name_len;

		zend_get_object_classname(object, &class_name, &class_name_len TSRMLS_CC);
		
		zend_error(E_CORE_ERROR, "Property %s of class %v cannot be updated", name, class_name);
	}
	ZVAL_STRINGL(&property, name, name_length, 0);
	Z_OBJ_HT_P(object)->write_property(object, &property, value TSRMLS_CC);

	EG(scope) = old_scope;
}

ZEND_API void zend_update_property_null(zend_class_entry *scope, zval *object, char *name, int name_length TSRMLS_DC)
{
	zval *tmp;

	ALLOC_ZVAL(tmp);
	tmp->is_ref = 0;
	tmp->refcount = 0;
	ZVAL_NULL(tmp);
	zend_update_property(scope, object, name, name_length, tmp TSRMLS_CC);
}

ZEND_API void zend_update_property_bool(zend_class_entry *scope, zval *object, char *name, int name_length, long value TSRMLS_DC)
{
	zval *tmp;
	
	ALLOC_ZVAL(tmp);
	tmp->is_ref = 0;
	tmp->refcount = 0;
	ZVAL_BOOL(tmp, value);
	zend_update_property(scope, object, name, name_length, tmp TSRMLS_CC);
}

ZEND_API void zend_update_property_long(zend_class_entry *scope, zval *object, char *name, int name_length, long value TSRMLS_DC)
{
	zval *tmp;
	
	ALLOC_ZVAL(tmp);
	tmp->is_ref = 0;
	tmp->refcount = 0;
	ZVAL_LONG(tmp, value);
	zend_update_property(scope, object, name, name_length, tmp TSRMLS_CC);
}

ZEND_API void zend_update_property_double(zend_class_entry *scope, zval *object, char *name, int name_length, double value TSRMLS_DC)
{
	zval *tmp;
	
	ALLOC_ZVAL(tmp);
	tmp->is_ref = 0;
	tmp->refcount = 0;
	ZVAL_DOUBLE(tmp, value);
	zend_update_property(scope, object, name, name_length, tmp TSRMLS_CC);
}

ZEND_API void zend_update_property_string(zend_class_entry *scope, zval *object, char *name, int name_length, char *value TSRMLS_DC)
{
	zval *tmp;
	
	ALLOC_ZVAL(tmp);
	tmp->is_ref = 0;
	tmp->refcount = 0;
	ZVAL_STRING(tmp, value, 1);
	zend_update_property(scope, object, name, name_length, tmp TSRMLS_CC);
}

ZEND_API void zend_update_property_stringl(zend_class_entry *scope, zval *object, char *name, int name_length, char *value, int value_len TSRMLS_DC)
{
	zval *tmp;
	
	ALLOC_ZVAL(tmp);
	tmp->is_ref = 0;
	tmp->refcount = 0;
	ZVAL_STRINGL(tmp, value, value_len, 1);
	zend_update_property(scope, object, name, name_length, tmp TSRMLS_CC);
}

ZEND_API void zend_update_property_unicode(zend_class_entry *scope, zval *object, UChar *name, int name_length, char *value TSRMLS_DC)
{
	zval *tmp;
	
	ALLOC_ZVAL(tmp);
	tmp->is_ref = 0;
	tmp->refcount = 0;
	ZVAL_UNICODE(tmp, value, 1);
	zend_update_property(scope, object, name, name_length, tmp TSRMLS_CC);
}

ZEND_API void zend_update_property_unicodel(zend_class_entry *scope, zval *object, UChar *name, int name_length, char *value, int value_len TSRMLS_DC)
{
	zval *tmp;
	
	ALLOC_ZVAL(tmp);
	tmp->is_ref = 0;
	tmp->refcount = 0;
	ZVAL_UNICODEL(tmp, value, value_len, 1);
	zend_update_property(scope, object, name, name_length, tmp TSRMLS_CC);
}

ZEND_API zval *zend_read_property(zend_class_entry *scope, zval *object, char *name, int name_length, zend_bool silent TSRMLS_DC)
{
	zval property, *value;
	zend_class_entry *old_scope = EG(scope);
	
	EG(scope) = scope;

	if (!Z_OBJ_HT_P(object)->read_property) {
		char *class_name;
		zend_uint class_name_len;

		zend_get_object_classname(object, &class_name, &class_name_len TSRMLS_CC);
		zend_error(E_CORE_ERROR, "Property %s of class %v cannot be read", name, class_name);
	}
	ZVAL_STRINGL(&property, name, name_length, 0);
	value = Z_OBJ_HT_P(object)->read_property(object, &property, silent TSRMLS_CC);

	EG(scope) = old_scope;
	return value;
}

/*
 * Return the most precise string type out of the list.
 * If none of the types are string types, IS_STRING is returned.
 * Binary > Unicode > string.
 */
ZEND_API zend_uchar zend_get_best_string_type(int num_args, ...)
{
	va_list ap;
	int best_type = IS_STRING;
	int type;

	if (num_args <= 0) return -1;

	va_start(ap, num_args);
	while (num_args--) {
		type = va_arg(ap, int);
		if (type == IS_BINARY) {
			best_type = IS_BINARY;
			break;
		} else if (type == IS_UNICODE && best_type == IS_STRING) {
			best_type = IS_UNICODE;
		}
	}
	va_end(ap);

	return best_type;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
