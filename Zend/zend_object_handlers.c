#include "zend.h"
#include "zend_globals.h"
#include "zend_variables.h"
#include "zend_API.h"
#include "zend_objects.h"
#include "zend_object_handlers.h"

#define DEBUG_OBJECT_HANDLERS 1

static HashTable *zend_std_get_properties(zval *object TSRMLS_DC)
{
	zend_object *zobj;
	zobj = Z_GET_OBJ(object);
	return zobj->properties;
}

zval *zend_std_read_property(zval *object, zval *member, int type TSRMLS_DC)
{
	zend_object *zobj;
	zval tmp_member;
	zval **retval;
	
	zobj = Z_GET_OBJ(object);

 	if (member->type != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

#if DEBUG_OBJECT_HANDLERS
	fprintf(stderr, "Read object #%d property: %s\n", Z_OBJ_HANDLE_P(object), Z_STRVAL_P(member));
#endif			
	
	if (zend_hash_find(zobj->properties, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, (void **) &retval) == FAILURE) {
		switch (type) {
			case BP_VAR_R: 
				zend_error(E_NOTICE,"Undefined property:  %s", Z_STRVAL_P(member));
				/* break missing intentionally */
			case BP_VAR_IS:
				retval = &EG(uninitialized_zval_ptr);
				break;
			case BP_VAR_RW:
				zend_error(E_NOTICE,"Undefined property:  %s", Z_STRVAL_P(member));
				/* break missing intentionally */
			case BP_VAR_W: {
					zval *new_zval = &EG(uninitialized_zval);
				
					new_zval->refcount++;
					zend_hash_update(zobj->properties, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, &new_zval, sizeof(zval *), (void **) &retval);
			}
				break;
				EMPTY_SWITCH_DEFAULT_CASE()
				
		}
	}
	if (member == &tmp_member) {
		zval_dtor(member);
	}
	return *retval;
}

static void zend_std_write_property(zval *object, zval *member, zval *value TSRMLS_DC)
{
	zend_object *zobj;
	zval tmp_member;
	zval *variable_ptr;
	
	zobj = Z_GET_OBJ(object);

 	if (member->type != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

	if (zend_hash_find(zobj->properties, Z_STRVAL_P(member), Z_STRLEN_P(member), (void **) &variable_ptr) == SUCCESS) {
		if (variable_ptr == EG(error_zval_ptr) || member == EG(error_zval_ptr)) {
			/* variable_ptr = EG(uninitialized_zval_ptr); */
/*	} else if (variable_ptr==&EG(uninitialized_zval) || variable_ptr!=value_ptr) { */
		} else if (variable_ptr != value) {
			variable_ptr->refcount--;
			if (variable_ptr->refcount == 0) {
				zendi_zval_dtor(*variable_ptr);
				FREE_ZVAL(variable_ptr);
			}
		}
	}

	zend_hash_update(zobj->properties, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, &value, sizeof(zval *), NULL); 
	if (member == &tmp_member) {
		zval_dtor(member);
	}
}

static zval **zend_std_get_property_ptr(zval *object, zval *member TSRMLS_DC)
{
	zend_object *zobj;
	zval tmp_member;
	zval **retval;
	
	zobj = Z_GET_OBJ(object);

 	if (member->type != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}

#if DEBUG_OBJECT_HANDLERS
	fprintf(stderr, "Ptr object #%d property: %s\n", Z_OBJ_HANDLE_P(object), Z_STRVAL_P(member));
#endif			

	if (zend_hash_find(zobj->properties, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, (void **) &retval) == FAILURE) {
		zval *new_zval = &EG(uninitialized_zval);

		// zend_error(E_NOTICE, "Undefined property: %s", Z_STRVAL_P(member));
		new_zval->refcount++;
		zend_hash_update(zobj->properties, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, &new_zval, sizeof(zval *), (void **) &retval);
	}
	if (member == &tmp_member) {
		zval_dtor(member);
	}
	return retval;
}

static void zend_std_unset_property(zval *object, zval *member TSRMLS_DC)
{
	zend_object *zobj;
	zval tmp_member;
	
	zobj = Z_GET_OBJ(object);

 	if (member->type != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}
	zend_hash_del(zobj->properties, Z_STRVAL_P(member), Z_STRLEN_P(member)+1);
	if (member == &tmp_member) {
		zval_dtor(member);
	}
}

static union _zend_function *zend_std_get_method(zval *object, char *method_name, int method_len TSRMLS_DC)
{
	zend_object *zobj;
	zend_function *func_method;
	
	zobj = Z_GET_OBJ(object);
	if(zend_hash_find(&zobj->ce->function_table, method_name, method_len+1, (void **)&func_method) == FAILURE) {
		zend_error(E_ERROR, "Call to undefined function %s()", method_name);
	}
	
	return func_method;
}

static union _zend_function *zend_std_get_constructor(zval *object TSRMLS_DC)
{
	zend_object *zobj;
	
	zobj = Z_GET_OBJ(object);
	return zobj->ce->constructor;
}

static int zend_std_get_class(zval *object, char **class_name, zend_uint *class_name_len, int parent TSRMLS_DC)
{
	zend_object *zobj;
	zend_class_entry *ce;
	
	zobj = Z_GET_OBJ(object);
	if(parent) {
		ce = zobj->ce->parent;
	} else {
		ce = zobj->ce;
	}
	if(!ce) {
		return FAILURE;
	}
	
	*class_name = zobj->ce->name;
	*class_name_len = zobj->ce->name_length;
	return SUCCESS;
}

int zend_compare_symbol_tables_i(HashTable *ht1, HashTable *ht2 TSRMLS_DC);

static int zend_std_compare_objects(zval *o1, zval *o2 TSRMLS_DC)
{
	zend_object *zobj1, *zobj2;
	
	zobj1 = Z_GET_OBJ(o1);
	zobj2 = Z_GET_OBJ(o2);

	if(zobj1->ce != zobj2->ce) {
		return 1; /* different classes */
	}
	return zend_compare_symbol_tables_i(zobj1->properties, zobj2->properties TSRMLS_CC);
}

static int zend_std_has_property(zval *object, zval *member, int check_empty TSRMLS_DC)
{
	zend_object *zobj;
	int result;
	zval **value;
	zval tmp_member;
	
	zobj = Z_GET_OBJ(object);
	
 	if (member->type != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
	}
	
	if(zend_hash_find(zobj->properties, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, (void **)&value) == SUCCESS) {
		if(check_empty) {
			result = zend_is_true(*value);
		} else {
			result = (Z_TYPE_PP(value) != IS_NULL);
		}
	} else {
		result = 0;
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}
	return result;
}

zend_object_handlers std_object_handlers = {
	zend_objects_add_ref,                    /* add_ref */
	zend_objects_del_ref,                    /* del_ref */
	zend_objects_delete_obj,                 /* delete_obj */
	zend_objects_clone_obj,                  /* clone_obj */
	
	zend_std_read_property,                  /* read_property */
	zend_std_write_property,                 /* write_property */
	zend_std_get_property_ptr,               /* get_property_ptr */
	NULL,                                    /* get */
	NULL,                                    /* set */
	zend_std_has_property,                   /* has_property */
	zend_std_unset_property,                 /* unset_property */
	zend_std_get_properties,                 /* get_properties */
	zend_std_get_method,                     /* get_method */
	NULL,                                    /* call_method */
	zend_std_get_constructor,                /* get_constructor */
	zend_std_get_class,                      /* get_class */
	zend_std_compare_objects                 /* compare_objects */
};

