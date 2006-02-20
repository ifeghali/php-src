/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2006 Zend Technologies Ltd. (http://www.zend.com) |
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

#ifndef ZEND_API_H
#define ZEND_API_H

#include "zend_modules.h"
#include "zend_list.h"
#include "zend_fast_cache.h"
#include "zend_operators.h"
#include "zend_variables.h"
#include "zend_execute.h"


BEGIN_EXTERN_C()

typedef struct _zend_function_entry {
	char *fname;
	void (*handler)(INTERNAL_FUNCTION_PARAMETERS);
	struct _zend_arg_info *arg_info;
	zend_uint num_args;
	zend_uint flags;
} zend_function_entry;

#define ZEND_FN(name) zif_##name
#define ZEND_NAMED_FUNCTION(name)		void name(INTERNAL_FUNCTION_PARAMETERS)
#define ZEND_FUNCTION(name)				ZEND_NAMED_FUNCTION(ZEND_FN(name))
#define ZEND_METHOD(classname, name)	ZEND_NAMED_FUNCTION(ZEND_FN(classname##_##name))

#define ZEND_FENTRY(zend_name, name, arg_info, flags)	{ #zend_name, name, arg_info, (zend_uint) (sizeof(arg_info)/sizeof(struct _zend_arg_info)-1), flags },

#define ZEND_NAMED_FE(zend_name, name, arg_info)	ZEND_FENTRY(zend_name, name, arg_info, 0)
#define ZEND_FE(name, arg_info)						ZEND_FENTRY(name, ZEND_FN(name), arg_info, 0)
#define ZEND_FALIAS(name, alias, arg_info)			ZEND_FENTRY(name, ZEND_FN(alias), arg_info, 0)
#define ZEND_ME(classname, name, arg_info, flags)	ZEND_FENTRY(name, ZEND_FN(classname##_##name), arg_info, flags)
#define ZEND_ABSTRACT_ME(classname, name, arg_info)	ZEND_FENTRY(name, NULL, arg_info, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
#define ZEND_MALIAS(classname, name, alias, arg_info, flags) \
                                                    ZEND_FENTRY(name, ZEND_FN(classname##_##alias), arg_info, flags)
#define ZEND_ME_MAPPING(name, func_name, arg_types) ZEND_NAMED_FE(name, ZEND_FN(func_name), arg_types)

#define ZEND_ARG_INFO(pass_by_ref, name)							{ #name, sizeof(#name)-1, NULL, 0, 0, 0, pass_by_ref, 0, 0 },
#define ZEND_ARG_PASS_INFO(pass_by_ref)								{ NULL, 0, NULL, 0, 0, 0, pass_by_ref, 0, 0 },
#define ZEND_ARG_OBJ_INFO(pass_by_ref, name, classname, allow_null) { #name, sizeof(#name)-1, #classname, sizeof(#classname)-1, 0, allow_null, pass_by_ref, 0, 0 },
#define ZEND_ARG_ARRAY_INFO(pass_by_ref, name, allow_null) { #name, sizeof(#name)-1, NULL, 0, 1, allow_null, pass_by_ref, 0, 0 },
#define ZEND_BEGIN_ARG_INFO_EX(name, pass_rest_by_reference, return_reference, required_num_args)	\
	zend_arg_info name[] = {																		\
		{ NULL, 0, NULL, 0, 0, 0, pass_rest_by_reference, return_reference, required_num_args },
#define ZEND_BEGIN_ARG_INFO(name, pass_rest_by_reference)	\
	ZEND_BEGIN_ARG_INFO_EX(name, pass_rest_by_reference, ZEND_RETURN_VALUE, -1)
#define ZEND_END_ARG_INFO()		};

/* Name macros */
#define ZEND_MODULE_STARTUP_N(module)       zm_startup_##module
#define ZEND_MODULE_SHUTDOWN_N(module)		zm_shutdown_##module
#define ZEND_MODULE_ACTIVATE_N(module)		zm_activate_##module
#define ZEND_MODULE_DEACTIVATE_N(module)	zm_deactivate_##module
#define ZEND_MODULE_POST_ZEND_DEACTIVATE_N(module)	zm_post_zend_deactivate_##module
#define ZEND_MODULE_INFO_N(module)			zm_info_##module

/* Declaration macros */
#define ZEND_MODULE_STARTUP_D(module)		int ZEND_MODULE_STARTUP_N(module)(INIT_FUNC_ARGS)
#define ZEND_MODULE_SHUTDOWN_D(module)		int ZEND_MODULE_SHUTDOWN_N(module)(SHUTDOWN_FUNC_ARGS)
#define ZEND_MODULE_ACTIVATE_D(module)		int ZEND_MODULE_ACTIVATE_N(module)(INIT_FUNC_ARGS)
#define ZEND_MODULE_DEACTIVATE_D(module)	int ZEND_MODULE_DEACTIVATE_N(module)(SHUTDOWN_FUNC_ARGS)
#define ZEND_MODULE_POST_ZEND_DEACTIVATE_D(module)	int ZEND_MODULE_POST_ZEND_DEACTIVATE_N(module)(void)
#define ZEND_MODULE_INFO_D(module)			void ZEND_MODULE_INFO_N(module)(ZEND_MODULE_INFO_FUNC_ARGS)

#define ZEND_GET_MODULE(name) \
    BEGIN_EXTERN_C()\
	ZEND_DLEXPORT zend_module_entry *get_module(void) { return &name##_module_entry; }\
    END_EXTERN_C()

#define ZEND_BEGIN_MODULE_GLOBALS(module_name)		\
	typedef struct _zend_##module_name##_globals {
#define ZEND_END_MODULE_GLOBALS(module_name)		\
	} zend_##module_name##_globals;

#ifdef ZTS

#define ZEND_DECLARE_MODULE_GLOBALS(module_name)							\
	ts_rsrc_id module_name##_globals_id;
#define ZEND_EXTERN_MODULE_GLOBALS(module_name)								\
	extern ts_rsrc_id module_name##_globals_id;
#define ZEND_INIT_MODULE_GLOBALS(module_name, globals_ctor, globals_dtor)	\
	ts_allocate_id(&module_name##_globals_id, sizeof(zend_##module_name##_globals), (ts_allocate_ctor) globals_ctor, (ts_allocate_dtor) globals_dtor);

#else

#define ZEND_DECLARE_MODULE_GLOBALS(module_name)							\
	zend_##module_name##_globals module_name##_globals;
#define ZEND_EXTERN_MODULE_GLOBALS(module_name)								\
	extern zend_##module_name##_globals module_name##_globals;
#define ZEND_INIT_MODULE_GLOBALS(module_name, globals_ctor, globals_dtor)	\
	globals_ctor(&module_name##_globals);

#endif

#define INIT_CLASS_ENTRY(class_container, class_name, functions) INIT_OVERLOADED_CLASS_ENTRY(class_container, class_name, functions, NULL, NULL, NULL)

#define INIT_OVERLOADED_CLASS_ENTRY_EX(class_container, class_name, functions, handle_fcall, handle_propget, handle_propset, handle_propunset, handle_propisset) \
	{															\
		class_container.name = strdup(class_name);				\
		class_container.name_length = sizeof(class_name) - 1;	\
		class_container.builtin_functions = functions;			\
		class_container.constructor = NULL;						\
		class_container.destructor = NULL;						\
		class_container.clone = NULL;							\
		class_container.serialize = NULL;						\
		class_container.unserialize = NULL;						\
		class_container.create_object = NULL;					\
		class_container.interface_gets_implemented = NULL;		\
		class_container.__call = handle_fcall;					\
		class_container.__tostring = NULL;						\
		class_container.__get = handle_propget;					\
		class_container.__set = handle_propset;					\
		class_container.__unset = handle_propunset;				\
		class_container.__isset = handle_propisset;				\
		class_container.serialize_func = NULL;					\
		class_container.unserialize_func = NULL;				\
		class_container.serialize = NULL;						\
		class_container.unserialize = NULL;						\
		class_container.parent = NULL;							\
		class_container.num_interfaces = 0;						\
		class_container.interfaces = NULL;						\
		class_container.get_iterator = NULL;					\
		class_container.iterator_funcs.funcs = NULL;			\
		class_container.module = NULL;							\
	}

#define INIT_OVERLOADED_CLASS_ENTRY(class_container, class_name, functions, handle_fcall, handle_propget, handle_propset) \
	INIT_OVERLOADED_CLASS_ENTRY_EX(class_container, class_name, functions, handle_fcall, handle_propget, handle_propset, NULL, NULL)

#ifdef ZTS
#	define CE_STATIC_MEMBERS(ce) (((ce)->type==ZEND_USER_CLASS)?(ce)->static_members:CG(static_members)[(long)(ce)->static_members])
#else
#	define CE_STATIC_MEMBERS(ce) ((ce)->static_members)
#endif

int zend_next_free_module(void);

BEGIN_EXTERN_C()
ZEND_API int zend_get_parameters(int ht, int param_count, ...);
ZEND_API int _zend_get_parameters_array(int ht, int param_count, zval **argument_array TSRMLS_DC);
ZEND_API int zend_get_parameters_ex(int param_count, ...);
ZEND_API int _zend_get_parameters_array_ex(int param_count, zval ***argument_array TSRMLS_DC);

/* internal function to efficiently copy parameters when executing __call() */
ZEND_API int zend_copy_parameters_array(int param_count, zval *argument_array TSRMLS_DC);

#define zend_get_parameters_array(ht, param_count, argument_array)			\
	_zend_get_parameters_array(ht, param_count, argument_array TSRMLS_CC)
#define zend_get_parameters_array_ex(param_count, argument_array)			\
	_zend_get_parameters_array_ex(param_count, argument_array TSRMLS_CC)


/* Parameter parsing API -- andrei */

#define ZEND_PARSE_PARAMS_QUIET 1<<1
ZEND_API int zend_parse_parameters(int num_args TSRMLS_DC, char *type_spec, ...);
ZEND_API int zend_parse_parameters_ex(int flags, int num_args TSRMLS_DC, char *type_spec, ...);
ZEND_API char *zend_zval_type_name(zval *arg);

ZEND_API int zend_parse_method_parameters(int num_args TSRMLS_DC, zval *this_ptr, char *type_spec, ...);
ZEND_API int zend_parse_method_parameters_ex(int flags, int num_args TSRMLS_DC, zval *this_ptr, char *type_spec, ...);

/* End of parameter parsing API -- andrei */

ZEND_API int zend_register_functions(zend_class_entry *scope, zend_function_entry *functions, HashTable *function_table, int type TSRMLS_DC);
ZEND_API void zend_unregister_functions(zend_function_entry *functions, int count, HashTable *function_table TSRMLS_DC);
ZEND_API int zend_startup_module(zend_module_entry *module_entry);
ZEND_API zend_module_entry* zend_register_internal_module(zend_module_entry *module_entry TSRMLS_DC);
ZEND_API zend_module_entry* zend_register_module_ex(zend_module_entry *module TSRMLS_DC);
ZEND_API int zend_startup_module_ex(zend_module_entry *module TSRMLS_DC);
ZEND_API int zend_startup_modules(TSRMLS_D);
ZEND_API void zend_check_magic_method_implementation(zend_class_entry *ce, zend_function *fptr, int error_type TSRMLS_DC);

ZEND_API zend_class_entry *zend_register_internal_class(zend_class_entry *class_entry TSRMLS_DC);
ZEND_API zend_class_entry *zend_register_internal_class_ex(zend_class_entry *class_entry, zend_class_entry *parent_ce, char *parent_name TSRMLS_DC);
ZEND_API zend_class_entry *zend_register_internal_interface(zend_class_entry *orig_class_entry TSRMLS_DC);
ZEND_API void zend_class_implements(zend_class_entry *class_entry TSRMLS_DC, int num_interfaces, ...);

ZEND_API int zend_disable_function(char *function_name, uint function_name_length TSRMLS_DC);
ZEND_API int zend_disable_class(char *class_name, uint class_name_length TSRMLS_DC);

ZEND_API void zend_wrong_param_count(TSRMLS_D);

#define IS_CALLABLE_CHECK_SYNTAX_ONLY (1<<0)
#define IS_CALLABLE_CHECK_NO_ACCESS   (1<<1)
#define IS_CALLABLE_CHECK_IS_STATIC   (1<<2)

#define IS_CALLABLE_STRICT  (IS_CALLABLE_CHECK_IS_STATIC)

ZEND_API zend_bool zend_is_callable_ex(zval *callable, uint check_flags, zval *callable_name, zend_class_entry **ce_ptr, zend_function **fptr_ptr, zval ***zobj_ptr_ptr TSRMLS_DC);
ZEND_API zend_bool zend_is_callable(zval *callable, uint check_flags, zval *callable_name);
ZEND_API zend_bool zend_make_callable(zval *callable, zval *callable_name TSRMLS_DC);
ZEND_API char *zend_get_module_version(char *module_name);
ZEND_API int zend_get_module_started(char *module_name);
ZEND_API int zend_declare_property(zend_class_entry *ce, char *name, int name_length, zval *property, int access_type TSRMLS_DC);
ZEND_API int zend_declare_property_ex(zend_class_entry *ce, char *name, int name_length, zval *property, int access_type, char *doc_comment, int doc_comment_len TSRMLS_DC);
ZEND_API int zend_declare_property_null(zend_class_entry *ce, char *name, int name_length, int access_type TSRMLS_DC);
ZEND_API int zend_declare_property_bool(zend_class_entry *ce, char *name, int name_length, long value, int access_type TSRMLS_DC);
ZEND_API int zend_declare_property_long(zend_class_entry *ce, char *name, int name_length, long value, int access_type TSRMLS_DC);
ZEND_API int zend_declare_property_double(zend_class_entry *ce, char *name, int name_length, double value, int access_type TSRMLS_DC);
ZEND_API int zend_declare_property_string(zend_class_entry *ce, char *name, int name_length, char *value, int access_type TSRMLS_DC);
ZEND_API int zend_declare_property_stringl(zend_class_entry *ce, char *name, int name_length, char *value, int value_len, int access_type TSRMLS_DC);

ZEND_API int zend_u_declare_property(zend_class_entry *ce, zend_uchar type, void *name, int name_length, zval *property, int access_type TSRMLS_DC);
ZEND_API int zend_u_declare_property_ex(zend_class_entry *ce, zend_uchar type, void *name, int name_length, zval *property, int access_type, char *doc_comment, int doc_comment_len TSRMLS_DC);

ZEND_API int zend_declare_class_constant(zend_class_entry *ce, char *name, size_t name_length, zval *value TSRMLS_DC);
ZEND_API int zend_declare_class_constant_long(zend_class_entry *ce, char *name, size_t name_length, long value TSRMLS_DC);
ZEND_API int zend_declare_class_constant_bool(zend_class_entry *ce, char *name, size_t name_length, zend_bool value TSRMLS_DC);
ZEND_API int zend_declare_class_constant_double(zend_class_entry *ce, char *name, size_t name_length, double value TSRMLS_DC);
ZEND_API int zend_declare_class_constant_stringl(zend_class_entry *ce, char *name, size_t name_length, char *value, size_t value_length TSRMLS_DC);
ZEND_API int zend_declare_class_constant_string(zend_class_entry *ce, char *name, size_t name_length, char *value TSRMLS_DC);

ZEND_API void zend_update_class_constants(zend_class_entry *class_type TSRMLS_DC);
ZEND_API void zend_update_property(zend_class_entry *scope, zval *object, char *name, int name_length, zval *value TSRMLS_DC);
ZEND_API void zend_update_property_null(zend_class_entry *scope, zval *object, char *name, int name_length TSRMLS_DC);
ZEND_API void zend_update_property_bool(zend_class_entry *scope, zval *object, char *name, int name_length, long value TSRMLS_DC);
ZEND_API void zend_update_property_long(zend_class_entry *scope, zval *object, char *name, int name_length, long value TSRMLS_DC);
ZEND_API void zend_update_property_double(zend_class_entry *scope, zval *object, char *name, int name_length, double value TSRMLS_DC);
ZEND_API void zend_update_property_string(zend_class_entry *scope, zval *object, char *name, int name_length, char *value TSRMLS_DC);
ZEND_API void zend_update_property_stringl(zend_class_entry *scope, zval *object, char *name, int name_length, char *value, int value_length TSRMLS_DC);
ZEND_API void zend_update_property_ascii_string(zend_class_entry *scope, zval *object, char *name, int name_length, char *value TSRMLS_DC);
ZEND_API void zend_update_property_ascii_stringl(zend_class_entry *scope, zval *object, char *name, int name_length, char *value, int value_length TSRMLS_DC);
ZEND_API void zend_update_property_rt_string(zend_class_entry *scope, zval *object, char *name, int name_length, char *value TSRMLS_DC);
ZEND_API void zend_update_property_rt_stringl(zend_class_entry *scope, zval *object, char *name, int name_length, char *value, int value_length TSRMLS_DC);
ZEND_API void zend_update_property_unicode(zend_class_entry *scope, zval *object, char *name, int name_length, UChar *value TSRMLS_DC);
ZEND_API void zend_update_property_unicodel(zend_class_entry *scope, zval *object, char *name, int name_length, UChar *value, int value_length TSRMLS_DC);

ZEND_API int zend_update_static_property(zend_class_entry *scope, char *name, int name_length, zval *value TSRMLS_DC);
ZEND_API int zend_update_static_property_null(zend_class_entry *scope, char *name, int name_length TSRMLS_DC);
ZEND_API int zend_update_static_property_bool(zend_class_entry *scope, char *name, int name_length, long value TSRMLS_DC);
ZEND_API int zend_update_static_property_long(zend_class_entry *scope, char *name, int name_length, long value TSRMLS_DC);
ZEND_API int zend_update_static_property_double(zend_class_entry *scope, char *name, int name_length, double value TSRMLS_DC);
ZEND_API int zend_update_static_property_string(zend_class_entry *scope, char *name, int name_length, char *value TSRMLS_DC);
ZEND_API int zend_update_static_property_stringl(zend_class_entry *scope, char *name, int name_length, char *value, int value_length TSRMLS_DC);
ZEND_API int zend_update_static_property_ascii_string(zend_class_entry *scope, char *name, int name_length, char *value TSRMLS_DC);
ZEND_API int zend_update_static_property_ascii_stringl(zend_class_entry *scope, char *name, int name_length, char *value, int value_length TSRMLS_DC);
ZEND_API int zend_update_static_property_rt_string(zend_class_entry *scope, char *name, int name_length, char *value TSRMLS_DC);
ZEND_API int zend_update_static_property_rt_stringl(zend_class_entry *scope, char *name, int name_length, char *value, int value_length TSRMLS_DC);
ZEND_API int zend_update_static_property_unicode(zend_class_entry *scope, char *name, int name_length, UChar *value TSRMLS_DC);
ZEND_API int zend_update_static_property_unicodel(zend_class_entry *scope, char *name, int name_length, UChar *value, int value_length TSRMLS_DC);

ZEND_API zval *zend_read_property(zend_class_entry *scope, zval *object, char *name, int name_length, zend_bool silent TSRMLS_DC);

ZEND_API zval *zend_read_static_property(zend_class_entry *scope, char *name, int name_length, zend_bool silent TSRMLS_DC);

ZEND_API zend_class_entry *zend_get_class_entry(zval *zobject TSRMLS_DC);
ZEND_API int zend_get_object_classname(zval *object, char **class_name, zend_uint *class_name_len TSRMLS_DC);
ZEND_API zend_uchar zend_get_unified_string_type(int num_args TSRMLS_DC, ...);

#define getThis() (this_ptr)

#define WRONG_PARAM_COUNT					ZEND_WRONG_PARAM_COUNT()
#define WRONG_PARAM_COUNT_WITH_RETVAL(ret)	ZEND_WRONG_PARAM_COUNT_WITH_RETVAL(ret)
#define ARG_COUNT(dummy)	(ht)
#define ZEND_NUM_ARGS()		(ht)
#define ZEND_WRONG_PARAM_COUNT()					{ zend_wrong_param_count(TSRMLS_C); return; }
#define ZEND_WRONG_PARAM_COUNT_WITH_RETVAL(ret)		{ zend_wrong_param_count(TSRMLS_C); return ret; }

#ifndef ZEND_WIN32
#define DLEXPORT
#endif

#define array_init(arg)			_array_init((arg) ZEND_FILE_LINE_CC)
#define object_init(arg)		_object_init((arg) ZEND_FILE_LINE_CC TSRMLS_CC)
#define object_init_ex(arg, ce)	_object_init_ex((arg), (ce) ZEND_FILE_LINE_CC TSRMLS_CC)
#define object_and_properties_init(arg, ce, properties)	_object_and_properties_init((arg), (ce), (properties) ZEND_FILE_LINE_CC TSRMLS_CC)
ZEND_API int _array_init(zval *arg ZEND_FILE_LINE_DC);
ZEND_API int _object_init(zval *arg ZEND_FILE_LINE_DC TSRMLS_DC);
ZEND_API int _object_init_ex(zval *arg, zend_class_entry *ce ZEND_FILE_LINE_DC TSRMLS_DC);
ZEND_API int _object_and_properties_init(zval *arg, zend_class_entry *ce, HashTable *properties ZEND_FILE_LINE_DC TSRMLS_DC);

ZEND_API void zend_merge_properties(zval *obj, HashTable *properties, int destroy_ht TSRMLS_DC);

/* no longer supported */
ZEND_API int add_assoc_function(zval *arg, char *key, void (*function_ptr)(INTERNAL_FUNCTION_PARAMETERS));

ZEND_API int add_assoc_long_ex(zval *arg, char *key, uint key_len, long n);
ZEND_API int add_assoc_null_ex(zval *arg, char *key, uint key_len);
ZEND_API int add_assoc_bool_ex(zval *arg, char *key, uint key_len, int b);
ZEND_API int add_assoc_resource_ex(zval *arg, char *key, uint key_len, int r);
ZEND_API int add_assoc_double_ex(zval *arg, char *key, uint key_len, double d);
ZEND_API int add_assoc_string_ex(zval *arg, char *key, uint key_len, char *str, int duplicate);
ZEND_API int add_assoc_stringl_ex(zval *arg, char *key, uint key_len, char *str, uint length, int duplicate);
ZEND_API int add_assoc_unicode_ex(zval *arg, char *key, uint key_len, void *str, int duplicate);
ZEND_API int add_assoc_unicodel_ex(zval *arg, char *key, uint key_len, void *str, uint length, int duplicate);
ZEND_API int add_assoc_zval_ex(zval *arg, char *key, uint key_len, zval *value);

#define add_assoc_text_ex(arg, key, key_len, str, duplicate) \
	if (UG(unicode)) { \
		add_assoc_unicode_ex(arg, key, key_len, (UChar*)(str), duplicate); \
	} else { \
		add_assoc_string_ex(arg, key, key_len, (char*)(str), duplicate); \
	}

#define add_assoc_textl_ex(arg, key, key_len, str, length, duplicate) \
	if (UG(unicode)) { \
		add_assoc_unicodel_ex(arg, key, key_len, (UChar*)(str), length, duplicate); \
	} else { \
		add_assoc_stringl_ex(arg, key, key_len, (char*)(str), length, duplicate); \
	}

#define add_assoc_ascii_string_ex(arg, key, key_len, str, duplicate) \
	if (UG(unicode)) { \
		uint length = strlen(str); \
		UChar *u_str = zend_ascii_to_unicode((str), length+1 ZEND_FILE_LINE_CC); \
		add_assoc_unicodel_ex(arg, key, key_len, u_str, length, 0); \
	} else { \
		add_assoc_string_ex(arg, key, key_len, (char*)(str), duplicate); \
	}

#define add_assoc_ascii_stringl_ex(arg, key, key_len, str, length, duplicate) \
	if (UG(unicode)) { \
		UChar *u_str = zend_ascii_to_unicode((str), (length)+1 ZEND_FILE_LINE_CC); \
		add_assoc_unicodel_ex(arg, key, key_len, u_str, length, 0); \
	} else { \
		add_assoc_stringl_ex(arg, key, key_len, (char*)(str), length, duplicate); \
	}

#define add_assoc_rt_string_ex(arg, key, key_len, str, duplicate) \
	if (UG(unicode)) { \
    UErrorCode status = U_ZERO_ERROR; \
		UChar *u_str; \
		int32_t u_len; \
		int length = strlen(str); \
		zend_convert_to_unicode(ZEND_U_CONVERTER(UG(runtime_encoding_conv)), &u_str, &u_len, str, length, &status); \
		add_assoc_unicodel_ex(arg, key, key_len, u_str, u_len, 0); \
	} else { \
		add_assoc_string_ex(arg, key, key_len, (char*)(str), duplicate); \
	}

#define add_assoc_rt_stringl_ex(arg, key, key_len, str, length, duplicate) \
	if (UG(unicode)) { \
    UErrorCode status = U_ZERO_ERROR; \
		UChar *u_str; \
		int32_t u_len; \
		zend_convert_to_unicode(ZEND_U_CONVERTER(UG(runtime_encoding_conv)), &u_str, &u_len, str, length, &status); \
		add_assoc_unicodel_ex(arg, key, key_len, u_str, u_len, 0); \
	} else { \
		add_assoc_stringl_ex(arg, key, key_len, (char*)(str), length, duplicate); \
	}

#define add_assoc_long(__arg, __key, __n) add_assoc_long_ex(__arg, __key, strlen(__key)+1, __n)
#define add_assoc_null(__arg, __key) add_assoc_null_ex(__arg, __key, strlen(__key) + 1)
#define add_assoc_bool(__arg, __key, __b) add_assoc_bool_ex(__arg, __key, strlen(__key)+1, __b)
#define add_assoc_resource(__arg, __key, __r) add_assoc_resource_ex(__arg, __key, strlen(__key)+1, __r)
#define add_assoc_double(__arg, __key, __d) add_assoc_double_ex(__arg, __key, strlen(__key)+1, __d)
#define add_assoc_string(__arg, __key, __str, __duplicate) add_assoc_string_ex(__arg, __key, strlen(__key)+1, __str, __duplicate)
#define add_assoc_stringl(__arg, __key, __str, __length, __duplicate) add_assoc_stringl_ex(__arg, __key, strlen(__key)+1, __str, __length, __duplicate)
#define add_assoc_unicode(__arg, __key, __str, __duplicate) add_assoc_unicode_ex(__arg, __key, strlen(__key)+1, __str, __duplicate)
#define add_assoc_unicodel(__arg, __key, __str, __length, __duplicate) add_assoc_unicodel_ex(__arg, __key, strlen(__key)+1, __str, __length, __duplicate)
#define add_assoc_zval(__arg, __key, __value) add_assoc_zval_ex(__arg, __key, strlen(__key)+1, __value)
#define add_assoc_rt_string(__arg, __key, __str, __duplicate) add_assoc_rt_string_ex(__arg, __key, strlen(__key)+1, __str, __duplicate)
#define add_assoc_rt_stringl(__arg, __key, __str, __length, __duplicate) add_assoc_rt_stringl_ex(__arg, __key, strlen(__key)+1, __str, __length, __duplicate)

#define add_assoc_text(arg, key, str, duplicate) \
	if (UG(unicode)) { \
		add_assoc_unicode(arg, key, (UChar*)(str), duplicate); \
	} else { \
		add_assoc_string(arg, key, (char*)(str), duplicate); \
	}

#define add_assoc_textl(arg, key, str, length, duplicate) \
	if (UG(unicode)) { \
		add_assoc_unicodel(arg, key, (UChar*)(str), length, duplicate); \
	} else { \
		add_assoc_stringl(arg, key, (char*)(str), length, duplicate); \
	}

#define add_assoc_ascii_string(arg, key, str, duplicate) \
	if (UG(unicode)) { \
		uint length = strlen(str); \
		UChar *u_str = zend_ascii_to_unicode((str), length+1 ZEND_FILE_LINE_CC); \
		add_assoc_unicodel(arg, key, u_str, length, 0); \
	} else { \
		add_assoc_string(arg, key, (char*)(str), duplicate); \
	}

#define add_assoc_ascii_stringl(arg, key, str, length, duplicate) \
	if (UG(unicode)) { \
		UChar *u_str = zend_ascii_to_unicode((str), (length)+1 ZEND_FILE_LINE_CC); \
		add_assoc_unicodel(arg, key, u_str, length, 0); \
	} else { \
		add_assoc_stringl(arg, key, (char*)(str), length, duplicate); \
	}

ZEND_API int add_u_assoc_zval_ex(zval *arg, zend_uchar type, void *key, uint key_len, zval *value);

#define add_u_assoc_zval(__arg, __type, __key, __value) add_u_assoc_zval_ex(__arg, __type, __key, (((__type)==IS_UNICODE)?u_strlen((UChar*)__key):strlen(__key))+1, __value)

/* unset() functions are only suported for legacy modules and null() functions should be used */
#define add_assoc_unset(__arg, __key) add_assoc_null_ex(__arg, __key, strlen(__key) + 1)
#define add_index_unset(__arg, __key) add_index_null(__arg, __key)
#define add_next_index_unset(__arg) add_next_index_null(__arg)
#define add_property_unset(__arg, __key) add_property_null(__arg, __key)

ZEND_API int add_index_long(zval *arg, ulong idx, long n);
ZEND_API int add_index_null(zval *arg, ulong idx);
ZEND_API int add_index_bool(zval *arg, ulong idx, int b);
ZEND_API int add_index_resource(zval *arg, ulong idx, int r);
ZEND_API int add_index_double(zval *arg, ulong idx, double d);
ZEND_API int add_index_string(zval *arg, ulong idx, char *str, int duplicate);
ZEND_API int add_index_stringl(zval *arg, ulong idx, char *str, uint length, int duplicate);
ZEND_API int add_index_unicode(zval *arg, ulong idx, UChar *str, int duplicate);
ZEND_API int add_index_unicodel(zval *arg, ulong idx, UChar *str, uint length, int duplicate);
ZEND_API int add_index_zval(zval *arg, ulong index, zval *value);

#define add_index_text(arg, idx, str, duplicate) \
	if (UG(unicode)) { \
		add_index_unicode(arg, idx, (UChar*)(str), duplicate); \
	} else { \
		add_index_string(arg, idx, (char*)(str), duplicate); \
	}

#define add_index_textl(arg, idx, str, length, duplicate) \
	if (UG(unicode)) { \
		add_index_unicodel(arg, idx, (UChar*)(str), length, duplicate); \
	} else { \
		add_index_stringl(arg, idx, (char*)(str), length, duplicate); \
	}

#define add_index_ascii_string(arg, idx, str, duplicate) \
	if (UG(unicode)) { \
		uint length = strlen(str); \
		UChar *u_str = zend_ascii_to_unicode((str), length+1 ZEND_FILE_LINE_CC); \
		add_index_unicodel(arg, idx, u_str, length, 0); \
	} else { \
		add_index_string(arg, idx, (char*)(str), duplicate); \
	}

#define add_index_ascii_stringl(arg, idx, str, length, duplicate) \
	if (UG(unicode)) { \
		UChar *u_str = zend_ascii_to_unicode((str), length+1 ZEND_FILE_LINE_CC); \
		add_index_unicodel(arg, idx, u_str, length, 0); \
	} else { \
		add_index_stringl(arg, idx, (char*)(str), length, duplicate); \
	}

ZEND_API int add_next_index_long(zval *arg, long n);
ZEND_API int add_next_index_null(zval *arg);
ZEND_API int add_next_index_bool(zval *arg, int b);
ZEND_API int add_next_index_resource(zval *arg, int r);
ZEND_API int add_next_index_double(zval *arg, double d);
ZEND_API int add_next_index_string(zval *arg, char *str, int duplicate);
ZEND_API int add_next_index_stringl(zval *arg, char *str, uint length, int duplicate);
ZEND_API int add_next_index_unicode(zval *arg, UChar *str, int duplicate);
ZEND_API int add_next_index_unicodel(zval *arg, UChar *str, uint length, int duplicate);
ZEND_API int add_next_index_zval(zval *arg, zval *value);

#define add_next_index_text(arg, str, duplicate) \
	if (UG(unicode)) { \
		add_next_index_unicode(arg, (UChar*)(str), duplicate); \
	} else { \
		add_next_index_string(arg, (char*)(str), duplicate); \
	}

#define add_next_index_textl(arg, str, length, duplicate) \
	if (UG(unicode)) { \
		add_next_index_unicodel(arg, (UChar*)(str), length, duplicate); \
	} else { \
		add_next_index_stringl(arg, (char*)(str), length, duplicate); \
	}

#define add_next_index_ascii_string(arg, str, duplicate) \
	if (UG(unicode)) { \
		uint length = strlen(str); \
		UChar *u_str = zend_ascii_to_unicode((str), length+1 ZEND_FILE_LINE_CC); \
		add_next_index_unicodel(arg, u_str, length, 0); \
	} else { \
		add_next_index_string(arg, (char*)(str), duplicate); \
	}

#define add_next_index_ascii_stringl(arg, str, length, duplicate) \
	if (UG(unicode)) { \
		UChar *u_str = zend_ascii_to_unicode((str), length+1 ZEND_FILE_LINE_CC); \
		add_next_index_unicodel(arg, u_str, length, 0); \
	} else { \
		add_next_index_stringl(arg, (char*)(str), length, duplicate); \
	}

#define add_next_index_rt_string(arg, str, duplicate) \
	if (UG(unicode)) { \
    UErrorCode status = U_ZERO_ERROR; \
		UChar *u_str; \
		int32_t u_len; \
		int length = strlen(str); \
		zend_convert_to_unicode(ZEND_U_CONVERTER(UG(runtime_encoding_conv)), &u_str, &u_len, str, length, &status); \
		add_next_index_unicodel(arg, u_str, u_len, 0); \
	} else { \
		add_next_index_string(arg, (char*)(str), duplicate); \
	}

#define add_next_index_rt_stringl(arg, str, length, duplicate) \
	if (UG(unicode)) { \
    UErrorCode status = U_ZERO_ERROR; \
		UChar *u_str; \
		int32_t u_len; \
		zend_convert_to_unicode(ZEND_U_CONVERTER(UG(runtime_encoding_conv)), &u_str, &u_len, str, length, &status); \
		add_next_index_unicodel(arg, u_str, u_len, 0); \
	} else { \
		add_next_index_stringl(arg, (char*)(str), length, duplicate); \
	}

ZEND_API int add_get_assoc_string_ex(zval *arg, char *key, uint key_len, char *str, void **dest, int duplicate);
ZEND_API int add_get_assoc_stringl_ex(zval *arg, char *key, uint key_len, char *str, uint length, void **dest, int duplicate);

#define add_get_assoc_string(__arg, __key, __str, __dest, __duplicate) add_get_assoc_string_ex(__arg, __key, strlen(__key)+1, __str, __dest, __duplicate)
#define add_get_assoc_stringl(__arg, __key, __str, __length, __dest, __duplicate) add_get_assoc_stringl_ex(__arg, __key, strlen(__key)+1, __str, __length, __dest, __duplicate)

ZEND_API int add_get_index_long(zval *arg, ulong idx, long l, void **dest);
ZEND_API int add_get_index_double(zval *arg, ulong idx, double d, void **dest);
ZEND_API int add_get_index_string(zval *arg, ulong idx, char *str, void **dest, int duplicate);
ZEND_API int add_get_index_stringl(zval *arg, ulong idx, char *str, uint length, void **dest, int duplicate);
ZEND_API int add_get_index_unicode(zval *arg, ulong idx, UChar *str, void **dest, int duplicate);
ZEND_API int add_get_index_unicodel(zval *arg, ulong idx, UChar *str, uint length, void **dest, int duplicate);

ZEND_API int add_property_long_ex(zval *arg, char *key, uint key_len, long l TSRMLS_DC);
ZEND_API int add_property_null_ex(zval *arg, char *key, uint key_len TSRMLS_DC);
ZEND_API int add_property_bool_ex(zval *arg, char *key, uint key_len, int b TSRMLS_DC);
ZEND_API int add_property_resource_ex(zval *arg, char *key, uint key_len, long r TSRMLS_DC);
ZEND_API int add_property_double_ex(zval *arg, char *key, uint key_len, double d TSRMLS_DC);
ZEND_API int add_property_string_ex(zval *arg, char *key, uint key_len, char *str, int duplicate TSRMLS_DC);
ZEND_API int add_property_stringl_ex(zval *arg, char *key, uint key_len,  char *str, uint length, int duplicate TSRMLS_DC);
ZEND_API int add_property_zval_ex(zval *arg, char *key, uint key_len, zval *value TSRMLS_DC);
ZEND_API int add_property_ascii_string_ex(zval *arg, char *key, uint key_len, char *str, int duplicate TSRMLS_DC);
ZEND_API int add_property_ascii_stringl_ex(zval *arg, char *key, uint key_len, char *str, uint length, int duplicate TSRMLS_DC);
ZEND_API int add_property_rt_string_ex(zval *arg, char *key, uint key_len, char *str, int duplicate TSRMLS_DC);
ZEND_API int add_property_rt_stringl_ex(zval *arg, char *key, uint key_len, char *str, uint length, int duplicate TSRMLS_DC);
ZEND_API int add_property_unicode_ex(zval *arg, char *key, uint key_len, UChar *str, int duplicate TSRMLS_DC);
ZEND_API int add_property_unicodel_ex(zval *arg, char *key, uint key_len,  UChar *str, uint length, int duplicate TSRMLS_DC);

#define add_property_long(__arg, __key, __n) add_property_long_ex(__arg, __key, strlen(__key)+1, __n TSRMLS_CC)
#define add_property_null(__arg, __key) add_property_null_ex(__arg, __key, strlen(__key) + 1 TSRMLS_CC)
#define add_property_bool(__arg, __key, __b) add_property_bool_ex(__arg, __key, strlen(__key)+1, __b TSRMLS_CC)
#define add_property_resource(__arg, __key, __r) add_property_resource_ex(__arg, __key, strlen(__key)+1, __r TSRMLS_CC)
#define add_property_double(__arg, __key, __d) add_property_double_ex(__arg, __key, strlen(__key)+1, __d TSRMLS_CC) 
#define add_property_string(__arg, __key, __str, __duplicate) add_property_string_ex(__arg, __key, strlen(__key)+1, __str, __duplicate TSRMLS_CC)
#define add_property_stringl(__arg, __key, __str, __length, __duplicate) add_property_stringl_ex(__arg, __key, strlen(__key)+1, __str, __length, __duplicate TSRMLS_CC)
#define add_property_ascii_string(__arg, __key, __str, __duplicate) add_property_string_ex(__arg, __key, strlen(__key)+1, __str, __duplicate TSRMLS_CC)
#define add_property_ascii_stringl(__arg, __key, __str, __length, __duplicate) add_property_stringl_ex(__arg, __key, strlen(__key)+1, __str, __length, __duplicate TSRMLS_CC)
#define add_property_rt_string(__arg, __key, __str, __duplicate) add_property_ascii_string_ex(__arg, __key, strlen(__key)+1, __str, __duplicate TSRMLS_CC)
#define add_property_rt_stringl(__arg, __key, __str, __length, __duplicate) add_property_rt_stringl_ex(__arg, __key, strlen(__key)+1, __str, __length, __duplicate TSRMLS_CC)
#define add_property_zval(__arg, __key, __value) add_property_zval_ex(__arg, __key, strlen(__key)+1, __value TSRMLS_CC)       
#define add_property_unicode(__arg, __key, __str, __duplicate) add_property_unicode_ex(__arg, __key, strlen(__key)+1, __str, __duplicate TSRMLS_CC)
#define add_property_unicodel(__arg, __key, __str, __length, __duplicate) add_property_unicodel_ex(__arg, __key, strlen(__key)+1, __str, __length, __duplicate TSRMLS_CC)


ZEND_API int call_user_function(HashTable *function_table, zval **object_pp, zval *function_name, zval *retval_ptr, zend_uint param_count, zval *params[] TSRMLS_DC);
ZEND_API int call_user_function_ex(HashTable *function_table, zval **object_pp, zval *function_name, zval **retval_ptr_ptr, zend_uint param_count, zval **params[], int no_separation, HashTable *symbol_table TSRMLS_DC);
END_EXTERN_C()

typedef struct _zend_fcall_info {
	size_t size;
	HashTable *function_table;
	zval *function_name;
	HashTable *symbol_table;
	zval **retval_ptr_ptr;
	zend_uint param_count;
	zval ***params;
	zval **object_pp;
	zend_bool no_separation;
} zend_fcall_info;

typedef struct _zend_fcall_info_cache {
	zend_bool initialized;
	zend_function *function_handler;
	zend_class_entry *calling_scope;
	zval **object_pp;
} zend_fcall_info_cache;

BEGIN_EXTERN_C()
ZEND_API extern zend_fcall_info_cache empty_fcall_info_cache;

ZEND_API int zend_call_function(zend_fcall_info *fci, zend_fcall_info_cache *fci_cache TSRMLS_DC);


ZEND_API int zend_set_hash_symbol(zval *symbol, char *name, int name_length,
                                  zend_bool is_ref, int num_symbol_tables, ...);

ZEND_API int zend_delete_global_variable(char *name, int name_len TSRMLS_DC);
ZEND_API int zend_u_delete_global_variable(zend_uchar type, void *name, int name_len TSRMLS_DC);

ZEND_API void zend_reset_all_cv(HashTable *symbol_table TSRMLS_DC);

#define add_method(arg, key, method)	add_assoc_function((arg), (key), (method))

ZEND_API ZEND_FUNCTION(display_disabled_function);
ZEND_API ZEND_FUNCTION(display_disabled_class);
END_EXTERN_C()


#if ZEND_DEBUG
#define CHECK_ZVAL_STRING(z) \
	if (Z_STRVAL_P(z)[Z_STRLEN_P(z)] != '\0') { zend_error(E_WARNING, "String is not zero-terminated (%s)", Z_STRVAL_P(z)); }
#define CHECK_ZVAL_STRING_REL(z) \
	if (Z_STRVAL_P(z)[Z_STRLEN_P(z)] != '\0') { zend_error(E_WARNING, "String is not zero-terminated (%s) (source: %s:%d)", Z_STRVAL_P(z) ZEND_FILE_LINE_RELAY_CC); }
#define CHECK_ZVAL_UNICODE(z) \
	if (Z_USTRVAL_P(z)[Z_USTRLEN_P(z)] != 0 ) { zend_error(E_WARNING, "String is not zero-terminated"); }
#define CHECK_ZVAL_UNICODE_REL(z) \
	if (Z_USTRVAL_P(z)[Z_USTRLEN_P(z)] != 0) { zend_error(E_WARNING, "String is not zero-terminated (source: %s:%d)", ZEND_FILE_LINE_RELAY_C); }
#else
#define CHECK_ZVAL_STRING(z)
#define CHECK_ZVAL_STRING_REL(z)
#define CHECK_ZVAL_UNICODE(z)
#define CHECK_ZVAL_UNICODE_REL(z)
#endif

#define ZVAL_RESOURCE(z, l) {		\
		Z_TYPE_P(z) = IS_RESOURCE;	\
		Z_LVAL_P(z) = l;			\
	}

#define ZVAL_BOOL(z, b) {			\
		Z_TYPE_P(z) = IS_BOOL;		\
		Z_LVAL_P(z) = ((b) != 0);   \
	}

#define ZVAL_NULL(z) {				\
		Z_TYPE_P(z) = IS_NULL;		\
	}

#define ZVAL_LONG(z, l) {			\
		Z_TYPE_P(z) = IS_LONG;		\
		Z_LVAL_P(z) = l;			\
	}

#define ZVAL_DOUBLE(z, d) {			\
		Z_TYPE_P(z) = IS_DOUBLE;	\
		Z_DVAL_P(z) = d;			\
	}

#define ZVAL_STRING(z, s, duplicate) {	\
		char *__s=(s);					\
		Z_STRLEN_P(z) = strlen(__s);	\
		Z_STRVAL_P(z) = (duplicate?estrndup(__s, Z_STRLEN_P(z)):__s);	\
		Z_TYPE_P(z) = IS_STRING;        \
	}

#define ZVAL_STRINGL(z, s, l, duplicate) {	\
		char *__s=(s); int __l=l;		\
		Z_STRLEN_P(z) = __l;		    \
		Z_STRVAL_P(z) = (duplicate?estrndup(__s, __l):__s);	\
		Z_TYPE_P(z) = IS_STRING;		    \
	}

#define ZVAL_ASCII_STRING(z, s, duplicate) \
	if (UG(unicode)) { \
		uint length = strlen(s); \
		UChar *u_str = zend_ascii_to_unicode((s), length+1 ZEND_FILE_LINE_CC); \
		ZVAL_UNICODEL(z, u_str, length, 0); \
	} else { \
		char *__s=(s);					\
		Z_STRLEN_P(z) = strlen(__s);	\
		Z_STRVAL_P(z) = (duplicate?estrndup(__s, Z_STRLEN_P(z)):__s);	\
		Z_TYPE_P(z) = IS_STRING;        \
	}

#define ZVAL_ASCII_STRINGL(z, s, l, duplicate) \
	if (UG(unicode)) { \
		UChar *u_str = zend_ascii_to_unicode((s), (l)+1 ZEND_FILE_LINE_CC); \
		ZVAL_UNICODEL(z, u_str, l, 0); \
	} else { \
		char *__s=(s); int __l=l;	\
		Z_STRLEN_P(z) = __l;	    \
		Z_STRVAL_P(z) = (duplicate?estrndup(__s, __l):__s);	\
		Z_TYPE_P(z) = IS_STRING;    \
	}

#define ZVAL_U_STRING(conv, z, s, duplicate) \
	if (UG(unicode)) { \
    UErrorCode status = U_ZERO_ERROR; \
		UChar *u_str; \
		int32_t u_len; \
		uint length = strlen(s); \
		zend_convert_to_unicode(conv, &u_str, &u_len, s, length, &status); \
		ZVAL_UNICODEL(z, u_str, u_len, 0); \
	} else { \
		char *__s=(s);					\
		Z_STRLEN_P(z) = strlen(__s);	\
		Z_STRVAL_P(z) = (duplicate?estrndup(__s, Z_STRLEN_P(z)):__s);	\
		Z_TYPE_P(z) = IS_STRING;        \
	}

#define ZVAL_U_STRINGL(conv, z, s, l, duplicate) \
	if (UG(unicode)) { \
    UErrorCode status = U_ZERO_ERROR; \
		UChar *u_str; \
		int32_t u_len; \
		zend_convert_to_unicode(conv, &u_str, &u_len, s, l, &status); \
		ZVAL_UNICODEL(z, u_str, u_len, 0); \
	} else { \
		char *__s=(s); int __l=l;	\
		Z_STRLEN_P(z) = __l;	    \
		Z_STRVAL_P(z) = (duplicate?estrndup(__s, __l):__s);	\
		Z_TYPE_P(z) = IS_STRING;    \
	}

#define ZVAL_RT_STRING(z, s, duplicate) \
	ZVAL_U_STRING(ZEND_U_CONVERTER(UG(runtime_encoding_conv)), z, s, duplicate)

#define ZVAL_RT_STRINGL(z, s, l, duplicate) \
	ZVAL_U_STRINGL(ZEND_U_CONVERTER(UG(runtime_encoding_conv)), z, s, l, duplicate)

#define ZVAL_UNICODE(z, u, duplicate) {	\
		UChar *__u=(u); 				\
		Z_USTRLEN_P(z) = u_strlen(__u); \
		Z_USTRVAL_P(z) = (duplicate?eustrndup(__u, Z_USTRLEN_P(z)):__u);	\
		Z_TYPE_P(z) = IS_UNICODE;		\
}

#define ZVAL_UNICODEL(z, u, l, duplicate) {	\
	UChar *__u=(u); int32_t __l=l; 			\
		Z_USTRLEN_P(z) = __l;	    \
		Z_USTRVAL_P(z) = (duplicate?eustrndup(__u, __l):__u);	\
		Z_TYPE_P(z) = IS_UNICODE;		    \
}

#define ZVAL_EMPTY_STRING(z) {				\
		Z_STRLEN_P(z) = 0;					\
		Z_STRVAL_P(z) = STR_EMPTY_ALLOC();	\
		Z_TYPE_P(z) = IS_STRING;			\
	}

#define ZVAL_EMPTY_UNICODE(z) {	        \
		Z_USTRLEN_P(z) = 0;  	    	\
		Z_USTRVAL_P(z) = USTR_MAKE(""); \
		Z_TYPE_P(z) = IS_UNICODE;		\
	}

#define ZVAL_ZVAL(z, zv, copy, dtor) {  \
		int is_ref, refcount;           \
		is_ref = (z)->is_ref;           \
		refcount = (z)->refcount;       \
		*(z) = *(zv);                   \
		if (copy) {                     \
			zval_copy_ctor(z);          \
	    }                               \
		if (dtor) {                     \
			if (!copy) {                \
				ZVAL_NULL(zv);          \
			}                           \
			zval_ptr_dtor(&zv);         \
	    }                               \
		(z)->is_ref = is_ref;           \
		(z)->refcount = refcount;       \
	}

#define ZVAL_TEXT(z, t, duplicate)					\
		do {										\
			if (UG(unicode)) {						\
				ZVAL_UNICODE(z, (UChar*)(t), duplicate);		\
			} else {								\
				ZVAL_STRING(z, (char*)(t), duplicate);		\
			}										\
		} while (0);

#define ZVAL_TEXTL(z, t, l, duplicate)				\
		do {										\
			if (UG(unicode)) {						\
				ZVAL_UNICODEL(z, (UChar*)(t), l, duplicate); 	\
			} else {								\
				ZVAL_STRINGL(z, (char*)(t), l, duplicate);	\
			}										\
		} while (0);

#define ZVAL_EMPTY_TEXT(z) \
		if (UG(unicode)) { \
			ZVAL_EMPTY_UNICODE(z); \
		} else { \
			ZVAL_EMPTY_STRING(z); \
		}

#define ZVAL_FALSE(z)  					ZVAL_BOOL(z, 0)
#define ZVAL_TRUE(z)  					ZVAL_BOOL(z, 1)

#define RETVAL_RESOURCE(l)				ZVAL_RESOURCE(return_value, l)
#define RETVAL_BOOL(b)					ZVAL_BOOL(return_value, b)
#define RETVAL_NULL() 					ZVAL_NULL(return_value)
#define RETVAL_LONG(l) 					ZVAL_LONG(return_value, l)
#define RETVAL_DOUBLE(d) 				ZVAL_DOUBLE(return_value, d)
#define RETVAL_STRING(s, duplicate) 		ZVAL_STRING(return_value, s, duplicate)
#define RETVAL_STRINGL(s, l, duplicate) 	ZVAL_STRINGL(return_value, s, l, duplicate)
#define RETVAL_ASCII_STRING(s, duplicate) 		ZVAL_ASCII_STRING(return_value, s, duplicate)
#define RETVAL_ASCII_STRINGL(s, l, duplicate) 	ZVAL_ASCII_STRINGL(return_value, s, l, duplicate)
#define RETVAL_U_STRING(conv, s, duplicate) 		ZVAL_U_STRING(conv, return_value, s, duplicate)
#define RETVAL_U_STRINGL(conv, s, l, duplicate) 	ZVAL_U_STRINGL(conv, return_value, s, l, duplicate)
#define RETVAL_RT_STRING(s, duplicate) 		ZVAL_RT_STRING(return_value, s, duplicate)
#define RETVAL_RT_STRINGL(s, l, duplicate) 	ZVAL_RT_STRINGL(return_value, s, l, duplicate)
#define RETVAL_EMPTY_STRING() 			ZVAL_EMPTY_STRING(return_value)
#define RETVAL_UNICODE(u, duplicate) 		ZVAL_UNICODE(return_value, u, duplicate)
#define RETVAL_UNICODEL(u, l, duplicate) 	ZVAL_UNICODEL(return_value, u, l, duplicate)
#define RETVAL_EMPTY_UNICODE() 			ZVAL_EMPTY_UNICODE(return_value)
#define RETVAL_ZVAL(zv, copy, dtor)		ZVAL_ZVAL(return_value, zv, copy, dtor)
#define RETVAL_FALSE  					ZVAL_BOOL(return_value, 0)
#define RETVAL_TRUE   					ZVAL_BOOL(return_value, 1)
#define RETVAL_TEXT(t, duplicate) ZVAL_TEXT(return_value, t, duplicate)
#define RETVAL_TEXTL(t, l, duplicate) ZVAL_TEXTL(return_value, t, l, duplicate)

#define RETURN_RESOURCE(l) 				{ RETVAL_RESOURCE(l); return; }
#define RETURN_BOOL(b) 					{ RETVAL_BOOL(b); return; }
#define RETURN_NULL() 					{ RETVAL_NULL(); return;}
#define RETURN_LONG(l) 					{ RETVAL_LONG(l); return; }
#define RETURN_DOUBLE(d) 				{ RETVAL_DOUBLE(d); return; }
#define RETURN_STRING(s, duplicate) 	{ RETVAL_STRING(s, duplicate); return; }
#define RETURN_STRINGL(s, l, duplicate) { RETVAL_STRINGL(s, l, duplicate); return; }
#define RETURN_EMPTY_STRING() 			{ RETVAL_EMPTY_STRING(); return; }
#define RETURN_UNICODE(u, duplicate) 	{ RETVAL_UNICODE(u, duplicate); return; }
#define RETURN_UNICODEL(u, l, duplicate) { RETVAL_UNICODEL(u, l, duplicate); return; }
#define RETURN_EMPTY_UNICODE() 			{ RETVAL_EMPTY_UNICODE(); return; }
#define RETURN_ZVAL(zv, copy, dtor)		{ RETVAL_ZVAL(zv, copy, dtor); return; }
#define RETURN_FALSE  					{ RETVAL_FALSE; return; }
#define RETURN_TRUE   					{ RETVAL_TRUE; return; }
#define RETURN_TEXT(t, duplicate)		{ RETVAL_TEXT(t, duplicate); return; }
#define RETURN_TEXTL(t, l, duplicate)	{ RETVAL_TEXTL(t, l, duplicate); return; }
#define RETURN_ASCII_STRING(t, duplicate)		{ RETVAL_ASCII_STRING(t, duplicate); return; }
#define RETURN_ASCII_STRINGL(t, l, duplicate)	{ RETVAL_ASCII_STRINGL(t, l, duplicate); return; }
#define RETURN_U_STRING(conv, t, duplicate)		{ RETVAL_U_STRING(conv, t, duplicate); return; }
#define RETURN_U_STRINGL(conv, t, l, duplicate)	{ RETVAL_U_STRINGL(conv, t, l, duplicate); return; }
#define RETURN_RT_STRING(t, duplicate)		{ RETVAL_RT_STRING(t, duplicate); return; }
#define RETURN_RT_STRINGL(t, l, duplicate)	{ RETVAL_RT_STRINGL(t, l, duplicate); return; }

#define SET_VAR_STRING(n, v) {																				\
								{																			\
									zval *var;																\
									ALLOC_ZVAL(var);														\
									ZVAL_STRING(var, v, 0);													\
									ZEND_SET_GLOBAL_VAR(n, var);											\
								}																			\
							}

#define SET_VAR_STRINGL(n, v, l) {														\
									{													\
										zval *var;										\
										ALLOC_ZVAL(var);								\
										ZVAL_STRINGL(var, v, l, 0);						\
										ZEND_SET_GLOBAL_VAR(n, var);					\
									}													\
								}

#define SET_VAR_LONG(n, v)	{															\
								{														\
									zval *var;											\
									ALLOC_ZVAL(var);									\
									ZVAL_LONG(var, v);									\
									ZEND_SET_GLOBAL_VAR(n, var);						\
								}														\
							}

#define SET_VAR_DOUBLE(n, v) {															\
								{														\
									zval *var;											\
									ALLOC_ZVAL(var);									\
									ZVAL_DOUBLE(var, v);								\
									ZEND_SET_GLOBAL_VAR(n, var);						\
								}														\
							}


#define ZEND_SET_SYMBOL(symtable, name, var)										\
	{																				\
		char *_name = (name);														\
																					\
		ZEND_SET_SYMBOL_WITH_LENGTH(symtable, _name, strlen(_name)+1, var, 1, 0);	\
	}

#define ZEND_U_SET_SYMBOL_WITH_LENGTH(symtable, type, name, name_length, var, _refcount, _is_ref)				\
	{																									\
		zval **orig_var;																				\
																										\
		if (zend_u_hash_find(symtable, (type), (name), (name_length), (void **) &orig_var)==SUCCESS				\
			&& PZVAL_IS_REF(*orig_var)) {																\
			(var)->refcount = (*orig_var)->refcount;													\
			(var)->is_ref = 1;																			\
																										\
			if (_refcount) {																			\
				(var)->refcount += _refcount-1;															\
			}																							\
			zval_dtor(*orig_var);																		\
			**orig_var = *(var);																		\
			FREE_ZVAL(var);																					\
		} else {																						\
			(var)->is_ref = _is_ref;																	\
			if (_refcount) {																			\
				(var)->refcount = _refcount;															\
			}																							\
			zend_u_hash_update(symtable, (type), (name), (name_length), &(var), sizeof(zval *), NULL);			\
		}																								\
	}

#define ZEND_SET_SYMBOL_WITH_LENGTH(symtable, name, name_length, var, _refcount, _is_ref)				\
	ZEND_U_SET_SYMBOL_WITH_LENGTH(symtable, IS_STRING, name, name_length, var, _refcount, _is_ref)

#define ZEND_SET_GLOBAL_VAR(name, var)				\
	ZEND_SET_SYMBOL(&EG(symbol_table), name, var)

#define ZEND_SET_GLOBAL_VAR_WITH_LENGTH(name, name_length, var, _refcount, _is_ref)		\
	ZEND_SET_SYMBOL_WITH_LENGTH(&EG(symbol_table), name, name_length, var, _refcount, _is_ref)

#define ZEND_DEFINE_PROPERTY(class_ptr, name, value, mask)								\
{																					\
	char *_name = (name);															\
	int namelen = strlen(_name);													\
	zend_declare_property(class_ptr, _name, namelen, value, mask TSRMLS_CC);		\
}

#define HASH_OF(p) (Z_TYPE_P(p)==IS_ARRAY ? Z_ARRVAL_P(p) : ((Z_TYPE_P(p)==IS_OBJECT ? Z_OBJ_HT_P(p)->get_properties((p) TSRMLS_CC) : NULL)))
#define ZVAL_IS_NULL(z) (Z_TYPE_P(z)==IS_NULL)

/* For compatibility */
#define ZEND_MINIT			ZEND_MODULE_STARTUP_N
#define ZEND_MSHUTDOWN		ZEND_MODULE_SHUTDOWN_N
#define ZEND_RINIT			ZEND_MODULE_ACTIVATE_N
#define ZEND_RSHUTDOWN		ZEND_MODULE_DEACTIVATE_N
#define ZEND_MINFO			ZEND_MODULE_INFO_N

#define ZEND_MINIT_FUNCTION			ZEND_MODULE_STARTUP_D
#define ZEND_MSHUTDOWN_FUNCTION		ZEND_MODULE_SHUTDOWN_D
#define ZEND_RINIT_FUNCTION			ZEND_MODULE_ACTIVATE_D
#define ZEND_RSHUTDOWN_FUNCTION		ZEND_MODULE_DEACTIVATE_D
#define ZEND_MINFO_FUNCTION			ZEND_MODULE_INFO_D

END_EXTERN_C()
	
#endif /* ZEND_API_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
