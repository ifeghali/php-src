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
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef ZEND_OBJECT_HANDLERS_H
#define ZEND_OBJECT_HANDLERS_H

union _zend_function;

/* Used to fetch property from the object, read-only */
typedef zval *(*zend_object_read_property_t)(zval *object, zval *member, zend_bool silent TSRMLS_DC);

/* Used to fetch dimension from the object, read-only */
typedef zval *(*zend_object_read_dimension_t)(zval *object, zval *offset TSRMLS_DC);

/* Used to set property of the object */
typedef void (*zend_object_write_property_t)(zval *object, zval *member, zval *value TSRMLS_DC);

/* Used to set dimension of the object */
typedef void (*zend_object_write_dimension_t)(zval *object, zval *offset, zval *value TSRMLS_DC);

/* Used to create pointer to the property of the object, for future r/w access via get/set */
typedef zval **(*zend_object_get_property_ptr_t)(zval *object, zval *member TSRMLS_DC);

/* Used to create pointer to the property of the object, for future direct r/w access */
typedef zval **(*zend_object_get_property_zval_ptr_t)(zval *object, zval *member TSRMLS_DC);

/* Used to set object value (most probably used in combination with
 * typedef the result of the get_property_ptr)
 */
typedef void (*zend_object_set_t)(zval **property, zval *value TSRMLS_DC);

/* Used to get object value (most probably used in combination with
 * the result of the get_property_ptr or when converting object value to
 * one of the basic types)
 */
typedef zval* (*zend_object_get_t)(zval *property TSRMLS_DC);

/* Used to check if a property of the object exists */
typedef int (*zend_object_has_property_t)(zval *object, zval *member, int check_empty TSRMLS_DC);

/* Used to remove a property of the object */
typedef void (*zend_object_unset_property_t)(zval *object, zval *member TSRMLS_DC);

/* Used to remove a property of the object */
typedef void (*zend_object_unset_dimension_t)(zval *object, zval *offset TSRMLS_DC);

/* Used to get hash of the properties of the object, as hash of zval's */
typedef HashTable *(*zend_object_get_properties_t)(zval *object TSRMLS_DC);

/* Used to call methods */
/* args on stack! */
/* Andi - EX(fbc) (function being called) needs to be initialized already in the INIT fcall opcode so that the parameters can be parsed the right way. We need to add another callback for this.
 */
typedef int (*zend_object_call_method_t)(char *method, INTERNAL_FUNCTION_PARAMETERS);
typedef union _zend_function *(*zend_object_get_method_t)(zval *object, char *method, int method_len TSRMLS_DC);
typedef union _zend_function *(*zend_object_get_constructor_t)(zval *object TSRMLS_DC);

/* Object maintenance/destruction */
typedef void (*zend_object_add_ref_t)(zval *object TSRMLS_DC);
typedef void (*zend_object_del_ref_t)(zval *object TSRMLS_DC);
typedef void (*zend_object_delete_obj_t)(zval *object TSRMLS_DC);
typedef zend_object_value (*zend_object_clone_obj_t)(zval *object TSRMLS_DC);

typedef zend_class_entry *(*zend_object_get_class_entry_t)(zval *object TSRMLS_DC);
typedef int (*zend_object_get_class_name_t)(zval *object, char **class_name, zend_uint *class_name_len, int parent TSRMLS_DC);
typedef int (*zend_object_compare_t)(zval *object1, zval *object2 TSRMLS_DC);
typedef int (*zend_object_cast_t)(zval *readobj, zval *writeobj, int type, int should_free TSRMLS_DC);


typedef struct _zend_object_handlers {
	/* general object functions */
	zend_object_add_ref_t					add_ref;
	zend_object_del_ref_t					del_ref;
	zend_object_delete_obj_t				delete_obj;
	zend_object_clone_obj_t					clone_obj;
	/* individual object functions */
	zend_object_read_property_t				read_property;
	zend_object_write_property_t			write_property;
	zend_object_read_dimension_t			read_dimension;
	zend_object_write_dimension_t			write_dimension;
	zend_object_get_property_ptr_t			get_property_ptr;
	zend_object_get_property_zval_ptr_t		get_property_zval_ptr;
	zend_object_get_t						get;
	zend_object_set_t						set;
	zend_object_has_property_t				has_property;
	zend_object_unset_property_t			unset_property;
	zend_object_unset_dimension_t			unset_dimension;
	zend_object_get_properties_t			get_properties;
	zend_object_get_method_t				get_method;
	zend_object_call_method_t				call_method;
	zend_object_get_constructor_t			get_constructor;
	zend_object_get_class_entry_t			get_class_entry;
	zend_object_get_class_name_t			get_class_name;
	zend_object_compare_t					compare_objects;
	zend_object_cast_t						cast_object;
} zend_object_handlers;

extern zend_object_handlers std_object_handlers;
union _zend_function *zend_std_get_static_method(zend_class_entry *ce, char *function_name_strval, int function_name_strlen TSRMLS_DC);
zval **zend_std_get_static_property(zend_class_entry *ce, char *property_name, int property_name_len, zend_bool silent TSRMLS_DC);
zend_bool zend_std_unset_static_property(zend_class_entry *ce, char *property_name, int property_name_len TSRMLS_DC);


#define IS_ZEND_STD_OBJECT(z)  ((z).type == IS_OBJECT && (Z_OBJ_HT((z))->get_class_entry != NULL))
#define HAS_CLASS_ENTRY(z) (Z_OBJ_HT(z)->get_class_entry != NULL)

int zend_check_protected(zend_class_entry *ce, zend_class_entry *scope);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
