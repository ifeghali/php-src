/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2002 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available at through the world-wide-web at                           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

#define ZEND_INTENSIVE_DEBUGGING 0

#include <stdio.h>
#include <signal.h>

#include "zend.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_API.h"
#include "zend_ptr_stack.h"
#include "zend_constants.h"
#include "zend_extensions.h"
#include "zend_fast_cache.h"
#include "zend_execute_locks.h"

#define get_zval_ptr(node, Ts, should_free, type) _get_zval_ptr(node, Ts, should_free TSRMLS_CC)
#define get_zval_ptr_ptr(node, Ts, type) _get_zval_ptr_ptr(node, Ts TSRMLS_CC)

#define get_incdec_op(op, opcode) \
	switch (opcode) { \
		case ZEND_PRE_INC: \
		case ZEND_POST_INC: \
		case ZEND_PRE_INC_OBJ: \
		case ZEND_POST_INC_OBJ: \
			(op) = increment_function; \
			break; \
		case ZEND_PRE_DEC: \
		case ZEND_POST_DEC: \
		case ZEND_PRE_DEC_OBJ: \
		case ZEND_POST_DEC_OBJ: \
			(op) = decrement_function; \
			break; \
		default: \
			(op) = NULL; \
			break; \
	} \

/* These globals don't have to be thread safe since they're never modified */


/* Prototypes */
static void zend_fetch_var_address(zend_op *opline, temp_variable *Ts, int type TSRMLS_DC);
static void zend_fetch_dimension_address(znode *result, znode *op1, znode *op2, temp_variable *Ts, int type TSRMLS_DC);
static void zend_fetch_property_address(znode *result, znode *op1, znode *op2, temp_variable *Ts, int type TSRMLS_DC);
static void zend_fetch_dimension_address_from_tmp_var(znode *result, znode *op1, znode *op2, temp_variable *Ts TSRMLS_DC);
static void zend_extension_statement_handler(zend_extension *extension, zend_op_array *op_array TSRMLS_DC);
static void zend_extension_fcall_begin_handler(zend_extension *extension, zend_op_array *op_array TSRMLS_DC);
static void zend_extension_fcall_end_handler(zend_extension *extension, zend_op_array *op_array TSRMLS_DC);

#define RETURN_VALUE_USED(opline) (!((opline)->result.u.EA.type & EXT_TYPE_UNUSED))

static inline zval *_get_zval_ptr(znode *node, temp_variable *Ts, int *should_free TSRMLS_DC)
{
	switch(node->op_type) {
		case IS_CONST:
			*should_free = 0;
			return &node->u.constant;
			break;
		case IS_TMP_VAR:
			*should_free = 1;
			return &Ts[node->u.var].tmp_var;
			break;
		case IS_VAR:
			if (Ts[node->u.var].var.ptr) {
				PZVAL_UNLOCK(Ts[node->u.var].var.ptr);
				*should_free = 0;
				return Ts[node->u.var].var.ptr;
			} else {
				*should_free = 1;

				switch (Ts[node->u.var].EA.type) {
					case IS_STRING_OFFSET: {
							temp_variable *T = &Ts[node->u.var];
							zval *str = T->EA.data.str_offset.str;

							if (T->EA.data.str_offset.str->type != IS_STRING
								|| (T->EA.data.str_offset.offset<0)
								|| (T->EA.data.str_offset.str->value.str.len <= T->EA.data.str_offset.offset)) {
								zend_error(E_NOTICE, "Uninitialized string offset:  %d", T->EA.data.str_offset.offset);
								T->tmp_var.value.str.val = empty_string;
								T->tmp_var.value.str.len = 0;
							} else {
								char c = str->value.str.val[T->EA.data.str_offset.offset];

								T->tmp_var.value.str.val = estrndup(&c, 1);
								T->tmp_var.value.str.len = 1;
							}
							PZVAL_UNLOCK(str);
							T->tmp_var.refcount=1;
							T->tmp_var.is_ref=1;
							T->tmp_var.type = IS_STRING;
							return &T->tmp_var;
						}
						break;
				}
			}
			break;
		case IS_UNUSED:
			*should_free = 0;
			return NULL;
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	return NULL;
}

static inline zval **_get_zval_ptr_ptr(znode *node, temp_variable *Ts TSRMLS_DC)
{
	if (node->op_type==IS_VAR) {
		if (Ts[node->u.var].var.ptr_ptr) {
			PZVAL_UNLOCK(*Ts[node->u.var].var.ptr_ptr);
		} else if (Ts[node->u.var].EA.type==IS_STRING_OFFSET) {
			PZVAL_UNLOCK(Ts[node->u.var].EA.data.str_offset.str);
		}
		return Ts[node->u.var].var.ptr_ptr;
	} else {
		return NULL;
	}
}

static inline zval **zend_fetch_property_address_inner(zval *object, znode *op2, temp_variable *Ts, int type TSRMLS_DC)
{
	zval *prop_ptr = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
	zval **retval = NULL;
	zval tmp;


	switch (op2->op_type) {
		case IS_CONST:
			/* already a constant string */
			break;
		case IS_VAR:
			tmp = *prop_ptr;
			zval_copy_ctor(&tmp);
			convert_to_string(&tmp);
			prop_ptr = &tmp;
			break;
		case IS_TMP_VAR:
			convert_to_string(prop_ptr);
			break;
	}

	if(Z_OBJ_HT_P(object)->get_property_ptr != NULL) {
		retval = Z_OBJ_HT_P(object)->get_property_ptr(object, prop_ptr TSRMLS_CC);
	}

	if(retval == NULL) {
		zend_error(E_WARNING, "This object doesn't support property references");
		retval = &EG(error_zval_ptr);
	}
	
	if (prop_ptr == &tmp) {
		zval_dtor(prop_ptr);
	}
	FREE_OP(Ts, op2, EG(free_op2));
	return retval;
}



static inline void zend_switch_free(zend_op *opline, temp_variable *Ts TSRMLS_DC)
{
	switch (opline->op1.op_type) {
		case IS_VAR:
			if (!Ts[opline->op1.u.var].var.ptr_ptr) {
				get_zval_ptr(&opline->op1, Ts, &EG(free_op1), BP_VAR_R);
				FREE_OP(Ts, &opline->op1, EG(free_op1));
			} else {
				zval_ptr_dtor(&Ts[opline->op1.u.var].var.ptr);
				if (opline->extended_value) { /* foreach() free */
					zval_ptr_dtor(&Ts[opline->op1.u.var].var.ptr);
				}
			}
			break;
		case IS_TMP_VAR:
			zendi_zval_dtor(Ts[opline->op1.u.var].tmp_var);
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

void zend_assign_to_variable_reference(znode *result, zval **variable_ptr_ptr, zval **value_ptr_ptr, temp_variable *Ts TSRMLS_DC)
{
	zval *variable_ptr;
	zval *value_ptr;

	if (!value_ptr_ptr || !variable_ptr_ptr) {
		zend_error(E_ERROR, "Cannot create references to/from string offsets nor overloaded objects");
		return;
	}

	variable_ptr = *variable_ptr_ptr;
	value_ptr = *value_ptr_ptr;

	if (variable_ptr == EG(error_zval_ptr) || value_ptr==EG(error_zval_ptr)) {
		variable_ptr_ptr = &EG(uninitialized_zval_ptr);
/*	} else if (variable_ptr==&EG(uninitialized_zval) || variable_ptr!=value_ptr) { */
	} else if (variable_ptr_ptr != value_ptr_ptr) {
		variable_ptr->refcount--;
		if (variable_ptr->refcount==0) {
			zendi_zval_dtor(*variable_ptr);
			FREE_ZVAL(variable_ptr);
		}

		if (!PZVAL_IS_REF(value_ptr)) {
			/* break it away */
			value_ptr->refcount--;
			if (value_ptr->refcount>0) {
				ALLOC_ZVAL(*value_ptr_ptr);
				**value_ptr_ptr = *value_ptr;
				value_ptr = *value_ptr_ptr;
				zendi_zval_copy_ctor(*value_ptr);
			}
			value_ptr->refcount = 1;
			value_ptr->is_ref = 1;
		}

		*variable_ptr_ptr = value_ptr;
		value_ptr->refcount++;
	} else {
		if (variable_ptr->refcount>1) { /* we need to break away */
			SEPARATE_ZVAL(variable_ptr_ptr);
		}
		(*variable_ptr_ptr)->is_ref = 1;
	}

	if (result && !(result->u.EA.type & EXT_TYPE_UNUSED)) {
		Ts[result->u.var].var.ptr_ptr = variable_ptr_ptr;
		SELECTIVE_PZVAL_LOCK(*variable_ptr_ptr, result);
		AI_USE_PTR(Ts[result->u.var].var);
	}
}

static inline void make_real_object(zval **object_ptr TSRMLS_DC)
{
	if ((*object_ptr)->type == IS_NULL
		|| ((*object_ptr)->type == IS_BOOL && (*object_ptr)->value.lval==0)
		|| ((*object_ptr)->type == IS_STRING && (*object_ptr)->value.str.len == 0)) {
		if (!PZVAL_IS_REF(*object_ptr)) {
			SEPARATE_ZVAL(object_ptr);
		}
		zend_error(E_NOTICE, "Creating default object from empty value");
		object_init(*object_ptr);
	}
}
	
static inline void zend_assign_to_object(znode *result, znode *op1, znode *op2, zval *value, temp_variable *Ts TSRMLS_DC)
{
	zval **object_ptr = get_zval_ptr_ptr(op1, Ts, BP_VAR_W);
	zval *object;
	zval *property = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
	zval tmp;
	zval **retval = &Ts[result->u.var].var.ptr;

	make_real_object(object_ptr TSRMLS_CC);
	object = *object_ptr;
	
	if (object->type != IS_OBJECT) {
		zend_error(E_WARNING, "Attempt to assign property of non-object");
		FREE_OP(Ts, op2, EG(free_op2));
		*retval = EG(uninitialized_zval_ptr);

		SELECTIVE_PZVAL_LOCK(*retval, result);
		PZVAL_UNLOCK(value);
		return;
	}
	
	/* here we are sure we are dealing with an object */
	switch (op2->op_type) {
		case IS_CONST:
			/* already a constant string */
			break;
		case IS_VAR:
			tmp = *property;
			zval_copy_ctor(&tmp);
			convert_to_string(&tmp);
			property = &tmp;
			break;
		case IS_TMP_VAR:
			convert_to_string(property);
			break;
	}

	/* here property is a string */
	PZVAL_UNLOCK(value);
	
	Z_OBJ_HT_P(object)->write_property(object, property, value TSRMLS_CC);
	if (property == &tmp) {
		zval_dtor(property);
	}
	
	FREE_OP(Ts, op2, EG(free_op2));
	if (result) {
		Ts[result->u.var].var.ptr = value;
		Ts[result->u.var].var.ptr_ptr = NULL; /* see if we can nuke this */
		SELECTIVE_PZVAL_LOCK(value, result);
	}
}

static inline void zend_assign_to_object_op(znode *result, znode *op1, znode *op2, zval *value, temp_variable *Ts, int (*binary_op)(zval *result, zval *op1, zval *op2 TSRMLS_DC) TSRMLS_DC)
{
	zval **object_ptr = get_zval_ptr_ptr(op1, Ts, BP_VAR_W);
	zval *object;
	zval *property = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
	zval tmp;
	zval **retval = &Ts[result->u.var].var.ptr;
	int have_get_ptr = 0;

	Ts[result->u.var].var.ptr_ptr = NULL;
	make_real_object(object_ptr TSRMLS_CC);
	object = *object_ptr;
	
	if (object->type != IS_OBJECT) {
		zend_error(E_WARNING, "Attempt to assign property of non-object");
		FREE_OP(Ts, op2, EG(free_op2));
		*retval = EG(uninitialized_zval_ptr);

		SELECTIVE_PZVAL_LOCK(*retval, result);
		return;
	}
	
	/* here we are sure we are dealing with an object */
	switch (op2->op_type) {
		case IS_CONST:
			/* already a constant string */
			break;
		case IS_VAR:
			tmp = *property;
			zval_copy_ctor(&tmp);
			convert_to_string(&tmp);
			property = &tmp;
			break;
		case IS_TMP_VAR:
			convert_to_string(property);
			break;
	}

	/* here property is a string */
	PZVAL_UNLOCK(value);
	
	if(Z_OBJ_HT_P(object)->get_property_zval_ptr) {
		zval **zptr = Z_OBJ_HT_P(object)->get_property_zval_ptr(object, property TSRMLS_CC);
		if(zptr != NULL) { 			/* NULL means no success in getting PTR */
			SEPARATE_ZVAL_IF_NOT_REF(zptr);

			have_get_ptr = 1;
			binary_op(*zptr, *zptr, value TSRMLS_CC);
			*retval = *zptr;
			SELECTIVE_PZVAL_LOCK(*retval, result);
		}
	}

	if(!have_get_ptr) {
		zval *z = Z_OBJ_HT_P(object)->read_property(object, property, BP_VAR_RW TSRMLS_CC);
		SEPARATE_ZVAL_IF_NOT_REF(&z);
		binary_op(z, z, value TSRMLS_CC);
		Z_OBJ_HT_P(object)->write_property(object, property, z TSRMLS_CC);
		*retval = z;
		SELECTIVE_PZVAL_LOCK(*retval, result);
		if(z->refcount <= 1) {
			zval_dtor(z);
		}
	}

	if (property == &tmp) {
		zval_dtor(property);
	}
	
	FREE_OP(Ts, op2, EG(free_op2));
}

static inline void zend_assign_to_variable(znode *result, znode *op1, znode *op2, zval *value, int type, temp_variable *Ts TSRMLS_DC)
{
	zval **variable_ptr_ptr = get_zval_ptr_ptr(op1, Ts, BP_VAR_W);
	zval *variable_ptr;
	
	if (!variable_ptr_ptr) {
		switch (Ts[op1->u.var].EA.type) {
			case IS_STRING_OFFSET: {
					temp_variable *T = &Ts[op1->u.var];

					if (T->EA.data.str_offset.str->type == IS_STRING) do {
						zval tmp;
						zval *final_value = value;

						if ((T->EA.data.str_offset.offset < 0)) {
							zend_error(E_WARNING, "Illegal string offset:  %d", T->EA.data.str_offset.offset);
							break;
						}
						if (T->EA.data.str_offset.offset >= T->EA.data.str_offset.str->value.str.len) {
							zend_uint i;

							if (T->EA.data.str_offset.str->value.str.len==0) {
								STR_FREE(T->EA.data.str_offset.str->value.str.val);
								T->EA.data.str_offset.str->value.str.val = (char *) emalloc(T->EA.data.str_offset.offset+1+1);
							} else {
								T->EA.data.str_offset.str->value.str.val = (char *) erealloc(T->EA.data.str_offset.str->value.str.val, T->EA.data.str_offset.offset+1+1);
							}
							for (i=T->EA.data.str_offset.str->value.str.len; i<T->EA.data.str_offset.offset; i++) {
								T->EA.data.str_offset.str->value.str.val[i] = ' ';
							}
							T->EA.data.str_offset.str->value.str.val[T->EA.data.str_offset.offset+1] = 0;
							T->EA.data.str_offset.str->value.str.len = T->EA.data.str_offset.offset+1;
						}

						if (value->type!=IS_STRING) {
							tmp = *value;
							if (op2 && op2->op_type == IS_VAR) {
								zval_copy_ctor(&tmp);
							}
							convert_to_string(&tmp);
							final_value = &tmp;
						}

						T->EA.data.str_offset.str->value.str.val[T->EA.data.str_offset.offset] = final_value->value.str.val[0];
						if (op2
							&& op2->op_type == IS_VAR
							&& value==&Ts[op2->u.var].tmp_var) {
							STR_FREE(value->value.str.val);
						}
						if (final_value == &tmp) {
							zval_dtor(final_value);
						}
						/*
						 * the value of an assignment to a string offset is undefined
						Ts[result->u.var].var = &T->EA.data.str_offset.str;
						*/
					} while(0);
					/* zval_ptr_dtor(&T->EA.data.str_offset.str); Nuke this line if it doesn't cause a leak */
					T->tmp_var.type = IS_STRING;
				}
				break;
			EMPTY_SWITCH_DEFAULT_CASE()
		}
		Ts[result->u.var].var.ptr_ptr = &EG(uninitialized_zval_ptr);
		SELECTIVE_PZVAL_LOCK(*Ts[result->u.var].var.ptr_ptr, result);
		AI_USE_PTR(Ts[result->u.var].var);
		return;
	}

	variable_ptr = *variable_ptr_ptr;

	if (variable_ptr == EG(error_zval_ptr)) {
		if (result) {
			Ts[result->u.var].var.ptr_ptr = &EG(uninitialized_zval_ptr);
			SELECTIVE_PZVAL_LOCK(*Ts[result->u.var].var.ptr_ptr, result);
			AI_USE_PTR(Ts[result->u.var].var);
		}
		if (type==IS_TMP_VAR) {
			zval_dtor(value);
		}
		return;
	}
	
	if (PZVAL_IS_REF(variable_ptr)) {
		if (variable_ptr!=value) {
			zend_uint refcount = variable_ptr->refcount;
			zval garbage;
	
			if (type!=IS_TMP_VAR) {
				value->refcount++;
			}
			garbage = *variable_ptr;
			*variable_ptr = *value;
			variable_ptr->refcount = refcount;
			variable_ptr->is_ref = 1;
			if (type!=IS_TMP_VAR) {
				zendi_zval_copy_ctor(*variable_ptr);
				value->refcount--;
			}
			zendi_zval_dtor(garbage);
		}
	} else {
		variable_ptr->refcount--;
		if (variable_ptr->refcount==0) {
			switch (type) {
				case IS_VAR:
					/* break missing intentionally */
				case IS_CONST:
					if (variable_ptr==value) {
						variable_ptr->refcount++;
					} else if (PZVAL_IS_REF(value)) {
						zval tmp;

						tmp = *value;
						zval_copy_ctor(&tmp);
						tmp.refcount=1;
						zendi_zval_dtor(*variable_ptr);
						*variable_ptr = tmp;
					} else {
						value->refcount++;
						zendi_zval_dtor(*variable_ptr);
						safe_free_zval_ptr(variable_ptr);
						*variable_ptr_ptr = value;
					}
					break;
				case IS_TMP_VAR:
					zendi_zval_dtor(*variable_ptr);
					value->refcount=1;
					*variable_ptr = *value;
					break;
				EMPTY_SWITCH_DEFAULT_CASE()
			}
		} else { /* we need to split */
			switch (type) {
				case IS_VAR:
					/* break missing intentionally */
				case IS_CONST:
					if (PZVAL_IS_REF(value) && value->refcount > 0) {
						ALLOC_ZVAL(variable_ptr);
						*variable_ptr_ptr = variable_ptr;
						*variable_ptr = *value;
						zval_copy_ctor(variable_ptr);
						variable_ptr->refcount=1;
						break;
					}
					*variable_ptr_ptr = value;
					value->refcount++;
					break;
				case IS_TMP_VAR:
					ALLOC_ZVAL(*variable_ptr_ptr);
					value->refcount=1;
					**variable_ptr_ptr = *value;
					break;
				EMPTY_SWITCH_DEFAULT_CASE()
			}
		}
		(*variable_ptr_ptr)->is_ref=0;
	}
	if (result) {
		Ts[result->u.var].var.ptr_ptr = variable_ptr_ptr;
		SELECTIVE_PZVAL_LOCK(*variable_ptr_ptr, result);
		AI_USE_PTR(Ts[result->u.var].var);
	} 
}


/* Utility Functions for Extensions */
static void zend_extension_statement_handler(zend_extension *extension, zend_op_array *op_array TSRMLS_DC)
{
	if (extension->statement_handler) {
		extension->statement_handler(op_array);
	}
}


static void zend_extension_fcall_begin_handler(zend_extension *extension, zend_op_array *op_array TSRMLS_DC)
{
	if (extension->fcall_begin_handler) {
		extension->fcall_begin_handler(op_array);
	}
}


static void zend_extension_fcall_end_handler(zend_extension *extension, zend_op_array *op_array TSRMLS_DC)
{
	if (extension->fcall_end_handler) {
		extension->fcall_end_handler(op_array);
	}
}


static void print_refcount(zval *p, char *str)
{
	print_refcount(NULL, NULL);
}

static inline HashTable *zend_get_target_symbol_table(zend_op *opline, temp_variable *Ts, int type TSRMLS_DC)
{
	switch (opline->op2.u.EA.type) {
		case ZEND_FETCH_LOCAL:
			return EG(active_symbol_table);
			break;
		case ZEND_FETCH_GLOBAL:
			/* Don't think this is actually needed.
			   if (opline->op1.op_type == IS_VAR) {
				PZVAL_LOCK(varname);
			}
+ */
			return &EG(symbol_table);
			break;
		case ZEND_FETCH_STATIC:
			if (!EG(active_op_array)->static_variables) {
				ALLOC_HASHTABLE(EG(active_op_array)->static_variables);
				zend_hash_init(EG(active_op_array)->static_variables, 2, NULL, ZVAL_PTR_DTOR, 0);
			}
			return EG(active_op_array)->static_variables;
			break;
		case ZEND_FETCH_STATIC_MEMBER:
			return Ts[opline->op2.u.var].EA.class_entry->static_members;
			break;
		case ZEND_FETCH_FROM_THIS:
			if (!EG(This)) {
				zend_error(E_ERROR, "Using $this when not in object context");
			}
			/* HACK!! 'this' should be always zend_object */
			return Z_OBJPROP_P(EG(This));
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	return NULL;
}


static void zend_fetch_var_address(zend_op *opline, temp_variable *Ts, int type TSRMLS_DC)
{
	int free_op1;
	zval *varname = get_zval_ptr(&opline->op1, Ts, &free_op1, BP_VAR_R);
	zval **retval;
	zval tmp_varname;
	HashTable *target_symbol_table;

	target_symbol_table = zend_get_target_symbol_table(opline, Ts, type TSRMLS_CC);
	if (!target_symbol_table) {
		return;
	}

 	if (varname->type != IS_STRING) {
		tmp_varname = *varname;
		zval_copy_ctor(&tmp_varname);
		convert_to_string(&tmp_varname);
		varname = &tmp_varname;
	}

	if (zend_hash_find(target_symbol_table, varname->value.str.val, varname->value.str.len+1, (void **) &retval) == FAILURE) {
		switch (type) {
			case BP_VAR_R: 
				zend_error(E_NOTICE,"Undefined variable:  %s", varname->value.str.val);
				/* break missing intentionally */
			case BP_VAR_IS:
				retval = &EG(uninitialized_zval_ptr);
				break;
			case BP_VAR_RW:
				zend_error(E_NOTICE,"Undefined variable:  %s", varname->value.str.val);
				/* break missing intentionally */
			case BP_VAR_W: {
					zval *new_zval = &EG(uninitialized_zval);

					new_zval->refcount++;
					zend_hash_update(target_symbol_table, varname->value.str.val, varname->value.str.len+1, &new_zval, sizeof(zval *), (void **) &retval);
				}
				break;
			EMPTY_SWITCH_DEFAULT_CASE()
		}
	}
	if (opline->op2.u.EA.type == ZEND_FETCH_LOCAL) {
		FREE_OP(Ts, &opline->op1, free_op1);
	} else if (opline->op2.u.EA.type == ZEND_FETCH_STATIC || opline->op2.u.EA.type == ZEND_FETCH_STATIC_MEMBER) {
		zval_update_constant(retval, (void *) 1 TSRMLS_CC);
	}

	if (varname == &tmp_varname) {
		zval_dtor(varname);
	}
	Ts[opline->result.u.var].var.ptr_ptr = retval;
	SELECTIVE_PZVAL_LOCK(*retval, &opline->result);
}


static inline zval **zend_fetch_dimension_address_inner(HashTable *ht, znode *op2, temp_variable *Ts, int type TSRMLS_DC)
{
	zval *dim = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
	zval **retval;
	char *offset_key;
	int offset_key_length;

	switch (dim->type) {
		case IS_NULL:
			offset_key = "";
			offset_key_length = 0;
			goto fetch_string_dim;
		case IS_STRING:
			offset_key = dim->value.str.val;
			offset_key_length = dim->value.str.len;

fetch_string_dim:
			if (zend_hash_find(ht, offset_key, offset_key_length+1, (void **) &retval) == FAILURE) {
				switch (type) {
					case BP_VAR_R: 
						zend_error(E_NOTICE,"Undefined index:  %s", offset_key);
						/* break missing intentionally */
					case BP_VAR_IS:
						retval = &EG(uninitialized_zval_ptr);
						break;
					case BP_VAR_RW:
						zend_error(E_NOTICE,"Undefined index:  %s", offset_key);
						/* break missing intentionally */
					case BP_VAR_W: {
							zval *new_zval = &EG(uninitialized_zval);

							new_zval->refcount++;
							zend_hash_update(ht, offset_key, offset_key_length+1, &new_zval, sizeof(zval *), (void **) &retval);
						}
						break;
				}
			}
			break;
		case IS_DOUBLE:
		case IS_RESOURCE:
		case IS_BOOL: 
		case IS_LONG: {
				long index;

				if (dim->type == IS_DOUBLE) {
					index = (long)dim->value.dval;
				} else {
					index = dim->value.lval;
				}
				if (zend_hash_index_find(ht, index, (void **) &retval) == FAILURE) {
					switch (type) {
						case BP_VAR_R: 
							zend_error(E_NOTICE,"Undefined offset:  %d", index);
							/* break missing intentionally */
						case BP_VAR_IS:
							retval = &EG(uninitialized_zval_ptr);
							break;
						case BP_VAR_RW:
							zend_error(E_NOTICE,"Undefined offset:  %d", index);
							/* break missing intentionally */
						case BP_VAR_W: {
								zval *new_zval = &EG(uninitialized_zval);

								new_zval->refcount++;
								zend_hash_index_update(ht, index, &new_zval, sizeof(zval *), (void **) &retval);
							}
							break;
					}
				}
			}
			break;
		default: 
			zend_error(E_WARNING, "Illegal offset type");
			if (type == BP_VAR_R || type == BP_VAR_IS) {
				retval = &EG(uninitialized_zval_ptr);
			} else {
				retval = &EG(error_zval_ptr);
			}
			break;
	}
	FREE_OP(Ts, op2, EG(free_op2));
	return retval;
}

static void zend_fetch_dimension_address(znode *result, znode *op1, znode *op2, temp_variable *Ts, int type TSRMLS_DC)
{
	zval **container_ptr = get_zval_ptr_ptr(op1, Ts, type);
	zval *container;
	zval ***retval = &Ts[result->u.var].var.ptr_ptr;

	container = *container_ptr;

	if (container == EG(error_zval_ptr)) {
		*retval = &EG(error_zval_ptr);
		SELECTIVE_PZVAL_LOCK(**retval, result);
		return;
	}

	if (container->type==IS_NULL
		|| (container->type==IS_BOOL && container->value.lval==0)
		|| (container->type==IS_STRING && container->value.str.len==0)) {
		switch (type) {
			case BP_VAR_RW:
			case BP_VAR_W:
				if (!PZVAL_IS_REF(container)) {
					SEPARATE_ZVAL(container_ptr);
					container = *container_ptr;
				}
				array_init(container);
				break;
		}
	}

	switch (container->type) {
		case IS_ARRAY:
			if ((type==BP_VAR_W || type==BP_VAR_RW) && container->refcount>1 && !PZVAL_IS_REF(container)) {
				SEPARATE_ZVAL(container_ptr);
				container = *container_ptr;
			}
			if (op2->op_type == IS_UNUSED) {
				zval *new_zval = &EG(uninitialized_zval);

				new_zval->refcount++;
				if (zend_hash_next_index_insert(container->value.ht, &new_zval, sizeof(zval *), (void **) retval) == FAILURE) {
					zend_error(E_WARNING, "Cannot add element to the array as the nexxt element is already occupied");
					*retval = &EG(uninitialized_zval_ptr);
					new_zval->refcount--; 
				}
			} else {
				*retval = zend_fetch_dimension_address_inner(container->value.ht, op2, Ts, type TSRMLS_CC);
			}
			SELECTIVE_PZVAL_LOCK(**retval, result);
			break;
		case IS_NULL:
			/* for read-mode only */
			get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
			*retval = &EG(uninitialized_zval_ptr);
			SELECTIVE_PZVAL_LOCK(**retval, result);
			FREE_OP(Ts, op2, EG(free_op2));
			if (type==BP_VAR_W || type==BP_VAR_RW) {
				zend_error(E_WARNING, "Cannot use a NULL value as an array");
			}
			break;
		case IS_STRING: {
				zval *offset;
				zval tmp;

				if (op2->op_type==IS_UNUSED) {
					zend_error(E_ERROR, "[] operator not supported for strings");
				}

				offset = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);

				if (offset->type != IS_LONG) {
					tmp = *offset;
					zval_copy_ctor(&tmp);
					convert_to_long(&tmp);
					offset = &tmp;
				}
				if (type!=BP_VAR_R && type!=BP_VAR_IS) {
					SEPARATE_ZVAL_IF_NOT_REF(container_ptr);
				}
				container = *container_ptr;
				Ts[result->u.var].EA.data.str_offset.str = container;
				PZVAL_LOCK(container);
				Ts[result->u.var].EA.data.str_offset.offset = offset->value.lval;
				Ts[result->u.var].EA.type = IS_STRING_OFFSET;
				FREE_OP(Ts, op2, EG(free_op2));
				*retval = NULL;
				return;
			}
			break;
		default: {
				zval *offset;

				offset = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
				if (type==BP_VAR_R || type==BP_VAR_IS) {
					*retval = &EG(uninitialized_zval_ptr);
				} else {
					*retval = &EG(error_zval_ptr);
				}
				FREE_OP(Ts, op2, EG(free_op2));
				SELECTIVE_PZVAL_LOCK(**retval, result);
				if (type==BP_VAR_W || type==BP_VAR_RW) {
					zend_error(E_WARNING, "Cannot use a scalar value as an array");
				}
			}
			break;
	}
}


static void zend_fetch_dimension_address_from_tmp_var(znode *result, znode *op1, znode *op2, temp_variable *Ts TSRMLS_DC)
{
	int free_op1;
	zval *container = get_zval_ptr(op1, Ts, &free_op1, BP_VAR_R);

	if (container->type != IS_ARRAY) {
		Ts[result->u.var].var.ptr_ptr = &EG(uninitialized_zval_ptr);
		SELECTIVE_PZVAL_LOCK(*Ts[result->u.var].var.ptr_ptr, result);
		return;
	}

	Ts[result->u.var].var.ptr_ptr = zend_fetch_dimension_address_inner(container->value.ht, op2, Ts, BP_VAR_R TSRMLS_CC);
	SELECTIVE_PZVAL_LOCK(*Ts[result->u.var].var.ptr_ptr, result);
}

static void zend_fetch_property_address(znode *result, znode *op1, znode *op2, temp_variable *Ts, int type TSRMLS_DC)
{
	zval **container_ptr = get_zval_ptr_ptr(op1, Ts, type);
	zval *container;
	zval ***retval = &Ts[result->u.var].var.ptr_ptr;
	
	container = *container_ptr;
	if (container == EG(error_zval_ptr)) {
		*retval = &EG(error_zval_ptr);
		SELECTIVE_PZVAL_LOCK(**retval, result);
		return;
	}
	
	if (container->type == IS_NULL
		|| (container->type == IS_BOOL && container->value.lval==0)
		|| (container->type == IS_STRING && container->value.str.len == 0)) {
		switch (type) {
			case BP_VAR_RW:
			case BP_VAR_W:
				if (!PZVAL_IS_REF(container)) {
					SEPARATE_ZVAL(container_ptr);
					container = *container_ptr;
				}
				object_init(container);
				break;
		}
	}
	
	if (container->type != IS_OBJECT) {
		zval *offset;
		
		offset = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
		FREE_OP(Ts, op2, EG(free_op2));
		if (type == BP_VAR_R || type == BP_VAR_IS) {
			*retval = &EG(uninitialized_zval_ptr);
		} else {
			*retval = &EG(error_zval_ptr);
		}
		SELECTIVE_PZVAL_LOCK(**retval, result);
		return;
	}
	
	
	if ((type==BP_VAR_W || type==BP_VAR_RW) && container->refcount>1 && !PZVAL_IS_REF(container)) {
		SEPARATE_ZVAL(container_ptr);
		container = *container_ptr;
	}
	*retval = zend_fetch_property_address_inner(container, op2, Ts, type TSRMLS_CC);
	SELECTIVE_PZVAL_LOCK(**retval, result);
}

static void zend_fetch_property_address_read(znode *result, znode *op1, znode *op2, temp_variable *Ts, int type TSRMLS_DC)
{
	zval *container;
	zval **retval;

	retval = &Ts[result->u.var].var.ptr;
	Ts[result->u.var].var.ptr_ptr = retval;

	container = get_zval_ptr(op1, Ts, &EG(free_op1), type);

	if (container == EG(error_zval_ptr)) {
		*retval = EG(error_zval_ptr);
		SELECTIVE_PZVAL_LOCK(*retval, result);
		return;
	}

	
	if(container->type != IS_OBJECT) {
		zend_error(E_NOTICE, "Trying to get property of non-object");

		if (type==BP_VAR_R || type==BP_VAR_IS) {
			*retval = EG(uninitialized_zval_ptr);
		} else {
			*retval = EG(error_zval_ptr);
		}
	} else {
		zval *offset;
		zval tmp;
		
		offset = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
		switch (op2->op_type) {
			case IS_CONST:
				/* already a constant string */
				break;
			case IS_VAR:
				tmp = *offset;
				zval_copy_ctor(&tmp);
				convert_to_string(&tmp);
				offset = &tmp;
				break;
			case IS_TMP_VAR:
				convert_to_string(offset);
				break;
		}

		/* here we are sure we are dealing with an object */
		*retval = Z_OBJ_HT_P(container)->read_property(container, offset, type TSRMLS_CC);
		if (offset == &tmp) {
			zval_dtor(offset);
		}
		FREE_OP(Ts, op2, EG(free_op2));
	}
	
	SELECTIVE_PZVAL_LOCK(*retval, result);
	return;
}

static void zend_pre_incdec_property(znode *result, znode *op1, znode *op2, temp_variable * Ts, int (*incdec_op)(zval *) TSRMLS_DC)
{
	zval **object_ptr = get_zval_ptr_ptr(op1, Ts, BP_VAR_W);
	zval *object;
	zval *property = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
	zval **retval = &Ts[result->u.var].var.ptr;
	int have_get_ptr = 0;

	make_real_object(object_ptr TSRMLS_CC);
	object = *object_ptr;
	
	if (object->type != IS_OBJECT) {
		zend_error(E_WARNING, "Attempt to increment/decrement property of non-object");
		FREE_OP(Ts, op2, EG(free_op2));
		*retval = EG(uninitialized_zval_ptr);

		SELECTIVE_PZVAL_LOCK(*retval, result);
		return;
	}
	
	/* here we are sure we are dealing with an object */

	if(Z_OBJ_HT_P(object)->get_property_zval_ptr) {
		zval **zptr = Z_OBJ_HT_P(object)->get_property_zval_ptr(object, property TSRMLS_CC);
		if(zptr != NULL) { 			/* NULL means no success in getting PTR */
			SEPARATE_ZVAL_IF_NOT_REF(zptr);

			have_get_ptr = 1;
			incdec_op(*zptr);
			*retval = *zptr;
			SELECTIVE_PZVAL_LOCK(*retval, result);
		}
	}

	if(!have_get_ptr) {
		zval *z = Z_OBJ_HT_P(object)->read_property(object, property, BP_VAR_RW TSRMLS_CC);
		SEPARATE_ZVAL_IF_NOT_REF(&z);
		incdec_op(z);
		Z_OBJ_HT_P(object)->write_property(object, property, z TSRMLS_CC);
		if(z->refcount <= 1) {
			zval_dtor(z);
		}
	}
	
	FREE_OP(Ts, op2, EG(free_op2));
}

static void zend_post_incdec_property(znode *result, znode *op1, znode *op2, temp_variable * Ts, int (*incdec_op)(zval *) TSRMLS_DC)
{
	zval **object_ptr = get_zval_ptr_ptr(op1, Ts, BP_VAR_W);
	zval *object;
	zval *property = get_zval_ptr(op2, Ts, &EG(free_op2), BP_VAR_R);
	zval *retval = &Ts[result->u.var].tmp_var;
	int have_get_ptr = 0;

	make_real_object(object_ptr TSRMLS_CC);
	object = *object_ptr;
	
	if (object->type != IS_OBJECT) {
		zend_error(E_WARNING, "Attempt to increment/decrement property of non-object");
		FREE_OP(Ts, op2, EG(free_op2));
		*retval = *EG(uninitialized_zval_ptr);
		return;
	}
	
	/* here we are sure we are dealing with an object */

	if(Z_OBJ_HT_P(object)->get_property_zval_ptr) {
		zval **zptr = Z_OBJ_HT_P(object)->get_property_zval_ptr(object, property TSRMLS_CC);
		if(zptr != NULL) { 			/* NULL means no success in getting PTR */
			have_get_ptr = 1;
			SEPARATE_ZVAL_IF_NOT_REF(zptr);
			
			*retval = **zptr;
			zendi_zval_copy_ctor(*retval);
			
			incdec_op(*zptr);

		}
	}

	if(!have_get_ptr) {
		zval *z = Z_OBJ_HT_P(object)->read_property(object, property, BP_VAR_RW TSRMLS_CC);
		SEPARATE_ZVAL_IF_NOT_REF(&z);
		*retval = *z;
		zendi_zval_copy_ctor(*retval);
		incdec_op(z);
		Z_OBJ_HT_P(object)->write_property(object, property, z TSRMLS_CC);
		if(z->refcount <= 1) {
			zval_dtor(z);
		}
	}
	
	FREE_OP(Ts, op2, EG(free_op2));
}


#if ZEND_INTENSIVE_DEBUGGING

#define CHECK_SYMBOL_TABLES()														\
	zend_hash_apply(&EG(symbol_table), (apply_func_t) zend_check_symbol TSRMLS_CC);	\
	if (&EG(symbol_table)!=EG(active_symbol_table)) {								\
		zend_hash_apply(EG(active_symbol_table), (apply_func_t) zend_check_symbol TSRMLS_CC);	\
	}

static int zend_check_symbol(zval **pz TSRMLS_DC)
{
	if (Z_TYPE_PP(pz) > 9) {
		fprintf(stderr, "Warning!  %x has invalid type!\n", *pz);
	} else if (Z_TYPE_PP(pz) == IS_ARRAY) {
		zend_hash_apply(Z_ARRVAL_PP(pz), (apply_func_t) zend_check_symbol TSRMLS_CC);
	} else if (Z_TYPE_PP(pz) == IS_OBJECT) {
		
		/* OBJ-TBI - doesn't support new object model! */
		zend_hash_apply(Z_OBJPROP_PP(pz), (apply_func_t) zend_check_symbol TSRMLS_CC);
	}

	return 0;
}


#else
#define CHECK_SYMBOL_TABLES()
#endif

#define NEXT_OPCODE()		\
	CHECK_SYMBOL_TABLES()	\
	EX(opline)++;			\
	continue;

#define RETURN_FROM_EXECUTE_LOOP(execute_data)								\
	free_alloca(EX(Ts));													\
	EG(in_execution) = EX(original_in_execution);							\
	EG(current_execute_data) = EX(prev_execute_data);						\
	return;

ZEND_API void execute(zend_op_array *op_array TSRMLS_DC)
{
	zend_execute_data execute_data;

	/* Initialize execute_data */
	EX(fbc) = NULL;
	EX(object) = NULL;
	EX(Ts) = (temp_variable *) do_alloca(sizeof(temp_variable)*op_array->T);
	EX(op_array) = op_array;
	EX(original_in_execution) = EG(in_execution);
	EX(prev_execute_data) = EG(current_execute_data);
	EG(current_execute_data) = &execute_data;

	EG(in_execution) = 1;
	if (op_array->start_op) {
		EX(opline) = op_array->start_op;
	} else {
		EX(opline) = op_array->opcodes;
	}

	if (op_array->uses_this && EG(This)) {
		EG(This)->refcount++; /* For $this pointer */
		if (zend_hash_add(EG(active_symbol_table), "this", sizeof("this"), &EG(This), sizeof(zval *), NULL)==FAILURE) {
			EG(This)->refcount--;
		}
	}

	EG(opline_ptr) = &EX(opline);

	EX(function_state).function = (zend_function *) op_array;
	EG(function_state_ptr) = &EX(function_state);
#if ZEND_DEBUG
	/* function_state.function_symbol_table is saved as-is to a stack,
	 * which is an intentional UMR.  Shut it up if we're in DEBUG.
	 */
	EX(function_state).function_symbol_table = NULL;
#endif
	
	while (1) {
#ifdef ZEND_WIN32
		if (EG(timed_out)) {
			zend_timeout(0);
		}
#endif

		zend_clean_garbage(TSRMLS_C);

		switch(EX(opline)->opcode) {
			case ZEND_ADD:
				EG(binary_op) = add_function;
				goto binary_op_addr;
			case ZEND_SUB:
				EG(binary_op) = sub_function;
				goto binary_op_addr;
			case ZEND_MUL:
				EG(binary_op) = mul_function;
				goto binary_op_addr;
			case ZEND_DIV:
				EG(binary_op) = div_function;
				goto binary_op_addr;
			case ZEND_MOD:
				EG(binary_op) = mod_function;
				goto binary_op_addr;
			case ZEND_SL:
				EG(binary_op) = shift_left_function;
				goto binary_op_addr;
			case ZEND_SR:
				EG(binary_op) = shift_right_function;
				goto binary_op_addr;
			case ZEND_CONCAT:
				EG(binary_op) = concat_function;
				goto binary_op_addr;
			case ZEND_IS_IDENTICAL:
				EG(binary_op) = is_identical_function;
				goto binary_op_addr;
			case ZEND_IS_NOT_IDENTICAL:
				EG(binary_op) = is_not_identical_function;
				goto binary_op_addr;
			case ZEND_IS_EQUAL:
				EG(binary_op) = is_equal_function;
				goto binary_op_addr;
			case ZEND_IS_NOT_EQUAL:
				EG(binary_op) = is_not_equal_function;
				goto binary_op_addr;
			case ZEND_IS_SMALLER:
				EG(binary_op) = is_smaller_function;
				goto binary_op_addr;
			case ZEND_IS_SMALLER_OR_EQUAL:
				EG(binary_op) = is_smaller_or_equal_function;
				goto binary_op_addr;
			case ZEND_BW_OR:
				EG(binary_op) = bitwise_or_function;
				goto binary_op_addr;
			case ZEND_BW_AND:
				EG(binary_op) = bitwise_and_function;
				goto binary_op_addr;
			case ZEND_BW_XOR:
				EG(binary_op) = bitwise_xor_function;
				goto binary_op_addr;
			case ZEND_BOOL_XOR:
				EG(binary_op) = boolean_xor_function;
		  	    /* Fall through */
binary_op_addr:
				EG(binary_op)(&EX(Ts)[EX(opline)->result.u.var].tmp_var, 
					get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R),
					get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R) TSRMLS_CC);
				FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));
				FREE_OP(EX(Ts), &EX(opline)->op2, EG(free_op2));
				NEXT_OPCODE();
			case ZEND_BW_NOT:
			case ZEND_BOOL_NOT:
				EG(unary_op) = get_unary_op(EX(opline)->opcode);
				EG(unary_op)(&EX(Ts)[EX(opline)->result.u.var].tmp_var,
							get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R) );
				FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));
				NEXT_OPCODE();
			case ZEND_ASSIGN_ADD:
				EG(binary_op) = add_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_SUB:
				EG(binary_op) = sub_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_MUL:
				EG(binary_op) = mul_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_DIV:
				EG(binary_op) = div_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_MOD:
				EG(binary_op) = mod_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_SL:
				EG(binary_op) = shift_left_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_SR:
				EG(binary_op) = shift_right_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_CONCAT:
				EG(binary_op) = concat_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_BW_OR:
				EG(binary_op) = bitwise_or_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_BW_AND:
				EG(binary_op) = bitwise_and_function;
				goto binary_assign_op_addr;
			case ZEND_ASSIGN_BW_XOR:
				EG(binary_op) = bitwise_xor_function;
				/* Fall through */
binary_assign_op_addr: {
					zval **var_ptr = get_zval_ptr_ptr(&EX(opline)->op1, EX(Ts), BP_VAR_RW);
				
					if (!var_ptr) {
						zend_error(E_ERROR, "Cannot use assign-op operators with overloaded objects nor string offsets");
					}
					if (*var_ptr == EG(error_zval_ptr)) {
						EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr = &EG(uninitialized_zval_ptr);
						SELECTIVE_PZVAL_LOCK(*EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr, &EX(opline)->result);
						AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
						EX(opline)++;
						continue;
					}
					
					SEPARATE_ZVAL_IF_NOT_REF(var_ptr);

					EG(binary_op)(*var_ptr, *var_ptr, get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R) TSRMLS_CC);
					EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr = var_ptr;
					SELECTIVE_PZVAL_LOCK(*var_ptr, &EX(opline)->result);
					FREE_OP(EX(Ts), &EX(opline)->op2, EG(free_op2));
					AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
				}
				NEXT_OPCODE();

			case ZEND_ASSIGN_ADD_OBJ:
				EG(binary_op) = add_function;
				goto binary_assign_op_addr_obj;
			case ZEND_ASSIGN_SUB_OBJ:
				EG(binary_op) = sub_function;
				goto binary_assign_op_addr_obj;
			case ZEND_ASSIGN_MUL_OBJ:
				EG(binary_op) = mul_function;
				goto binary_assign_op_addr_obj;
			case ZEND_ASSIGN_DIV_OBJ:
				EG(binary_op) = div_function;
				goto binary_assign_op_addr_obj;
			case ZEND_ASSIGN_MOD_OBJ:
				EG(binary_op) = mod_function;
				goto binary_assign_op_addr_obj;
			case ZEND_ASSIGN_SL_OBJ:
				EG(binary_op) = shift_left_function;
				goto binary_assign_op_addr_obj;
			case ZEND_ASSIGN_SR_OBJ:
				EG(binary_op) = shift_right_function;
				goto binary_assign_op_addr_obj;
			case ZEND_ASSIGN_CONCAT_OBJ:
				EG(binary_op) = concat_function;
				goto binary_assign_op_addr_obj;
			case ZEND_ASSIGN_BW_OR_OBJ:
				EG(binary_op) = bitwise_or_function;
				goto binary_assign_op_addr_obj;
			case ZEND_ASSIGN_BW_AND_OBJ:
				EG(binary_op) = bitwise_and_function;
				goto binary_assign_op_addr_obj;
			case ZEND_ASSIGN_BW_XOR_OBJ:
				EG(binary_op) = bitwise_xor_function;
				/* Fall through */
binary_assign_op_addr_obj:
				zend_assign_to_object_op(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts)[EX(opline)->extended_value].var.ptr, EX(Ts), EG(binary_op) TSRMLS_CC);
				NEXT_OPCODE();
			case ZEND_PRE_INC_OBJ:
			case ZEND_PRE_DEC_OBJ: {
				int (*incdec_op)(zval *op);

				get_incdec_op(incdec_op, EX(opline)->opcode);
				zend_pre_incdec_property(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), incdec_op TSRMLS_CC);
			}
			NEXT_OPCODE();
			case ZEND_POST_INC_OBJ:
			case ZEND_POST_DEC_OBJ: {
				int (*incdec_op)(zval *op);

				get_incdec_op(incdec_op, EX(opline)->opcode);
				zend_post_incdec_property(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), incdec_op TSRMLS_CC);
			}
			NEXT_OPCODE();
			case ZEND_PRE_INC:
			case ZEND_PRE_DEC:
			case ZEND_POST_INC:
			case ZEND_POST_DEC: {
					int (*incdec_op)(zval *op);
					zval **var_ptr = get_zval_ptr_ptr(&EX(opline)->op1, EX(Ts), BP_VAR_RW);

					if (!var_ptr) {
						zend_error(E_ERROR, "Cannot increment/decrement overloaded objects nor string offsets");
					}
					if (*var_ptr == EG(error_zval_ptr)) {
						EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr = &EG(uninitialized_zval_ptr);
						SELECTIVE_PZVAL_LOCK(*EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr, &EX(opline)->result);
						AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
						EX(opline)++;
						continue;
					}

					get_incdec_op(incdec_op, EX(opline)->opcode);

					switch (EX(opline)->opcode) {
						case ZEND_POST_INC:
						case ZEND_POST_DEC:
							EX(Ts)[EX(opline)->result.u.var].tmp_var = **var_ptr;
							zendi_zval_copy_ctor(EX(Ts)[EX(opline)->result.u.var].tmp_var);
							break;
					}
					
					SEPARATE_ZVAL_IF_NOT_REF(var_ptr);

					incdec_op(*var_ptr);
					switch (EX(opline)->opcode) {
						case ZEND_PRE_INC:
						case ZEND_PRE_DEC:
							EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr = var_ptr;
							SELECTIVE_PZVAL_LOCK(*var_ptr, &EX(opline)->result);
							AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
							break;
					}
				}
				NEXT_OPCODE();
			case ZEND_PRINT:
				zend_print_variable(get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R));
				EX(Ts)[EX(opline)->result.u.var].tmp_var.value.lval = 1;
				EX(Ts)[EX(opline)->result.u.var].tmp_var.type = IS_LONG;
				FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));
				NEXT_OPCODE();
			case ZEND_ECHO:
				zend_print_variable(get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R));
				FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));
				NEXT_OPCODE();
			case ZEND_FETCH_R:
				zend_fetch_var_address(EX(opline), EX(Ts), BP_VAR_R TSRMLS_CC);
				AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_FETCH_W:
				zend_fetch_var_address(EX(opline), EX(Ts), BP_VAR_W TSRMLS_CC);
				NEXT_OPCODE();
			case ZEND_FETCH_RW:
				zend_fetch_var_address(EX(opline), EX(Ts), BP_VAR_RW TSRMLS_CC);
				NEXT_OPCODE();
			case ZEND_FETCH_FUNC_ARG:
				if (ARG_SHOULD_BE_SENT_BY_REF(EX(opline)->extended_value, EX(fbc), EX(fbc)->common.arg_types)) {
					/* Behave like FETCH_W */
					zend_fetch_var_address(EX(opline), EX(Ts), BP_VAR_W TSRMLS_CC);
				} else {
					/* Behave like FETCH_R */
					zend_fetch_var_address(EX(opline), EX(Ts), BP_VAR_R TSRMLS_CC);
					AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
				}
				NEXT_OPCODE();
			case ZEND_FETCH_UNSET:
				zend_fetch_var_address(EX(opline), EX(Ts), BP_VAR_R TSRMLS_CC);
				PZVAL_UNLOCK(*EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr);
				if (EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr != &EG(uninitialized_zval_ptr)) {
					SEPARATE_ZVAL_IF_NOT_REF(EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr);
				}
				PZVAL_LOCK(*EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr);
				NEXT_OPCODE();
			case ZEND_FETCH_IS:
				zend_fetch_var_address(EX(opline), EX(Ts), BP_VAR_IS TSRMLS_CC);
				AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_R:
				if (EX(opline)->extended_value == ZEND_FETCH_ADD_LOCK) {
					PZVAL_LOCK(*EX(Ts)[EX(opline)->op1.u.var].var.ptr_ptr);
				}
				zend_fetch_dimension_address(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_R TSRMLS_CC);
				AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_W:
				zend_fetch_dimension_address(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_W TSRMLS_CC);
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_RW:
				zend_fetch_dimension_address(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_RW TSRMLS_CC);
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_IS:
				zend_fetch_dimension_address(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_IS TSRMLS_CC);
				AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_FUNC_ARG:
				if (ARG_SHOULD_BE_SENT_BY_REF(EX(opline)->extended_value, EX(fbc), EX(fbc)->common.arg_types)) {
					/* Behave like FETCH_DIM_W */
					zend_fetch_dimension_address(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_W TSRMLS_CC);
				} else {
					/* Behave like FETCH_DIM_R, except for locking used for list() */
					zend_fetch_dimension_address(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_R TSRMLS_CC);
					AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
				}
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_UNSET:
				/* Not needed in DIM_UNSET
				if (EX(opline)->extended_value == ZEND_FETCH_ADD_LOCK) {
					PZVAL_LOCK(*EX(Ts)[EX(opline)->op1.u.var].var.ptr_ptr);
				}
				*/
				zend_fetch_dimension_address(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_R TSRMLS_CC);
				PZVAL_UNLOCK(*EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr);
				if (EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr != &EG(uninitialized_zval_ptr)) {
					SEPARATE_ZVAL_IF_NOT_REF(EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr);
				}
				PZVAL_LOCK(*EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr);
				NEXT_OPCODE();
			case ZEND_FETCH_OBJ_R:
				zend_fetch_property_address_read(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_R TSRMLS_CC);
				AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_FETCH_OBJ_W:
				zend_fetch_property_address(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_W TSRMLS_CC);
				NEXT_OPCODE();
			case ZEND_FETCH_OBJ_RW:
				zend_fetch_property_address(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_RW TSRMLS_CC);
				NEXT_OPCODE();
			case ZEND_FETCH_OBJ_IS:
				zend_fetch_property_address_read(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_IS TSRMLS_CC);
				AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_FETCH_OBJ_FUNC_ARG:
				if (ARG_SHOULD_BE_SENT_BY_REF(EX(opline)->extended_value, EX(fbc), EX(fbc)->common.arg_types)) {
					/* Behave like FETCH_OBJ_W */
					zend_fetch_property_address(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_W TSRMLS_CC);
				} else {
					zend_fetch_property_address_read(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_R TSRMLS_CC);
					AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
				}
				NEXT_OPCODE();
			case ZEND_FETCH_OBJ_UNSET:
				zend_fetch_property_address(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts), BP_VAR_R TSRMLS_CC);

				PZVAL_UNLOCK(*EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr);
				if (EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr != &EG(uninitialized_zval_ptr)) {
					SEPARATE_ZVAL_IF_NOT_REF(EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr);
				}
				PZVAL_LOCK(*EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr);
				NEXT_OPCODE();
			case ZEND_FETCH_DIM_TMP_VAR:
				zend_fetch_dimension_address_from_tmp_var(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts) TSRMLS_CC);
				AI_USE_PTR(EX(Ts)[EX(opline)->result.u.var].var);
				NEXT_OPCODE();
			case ZEND_MAKE_VAR: {
				zval *value, *value2;

				value = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
				switch(EX(opline)->op1.op_type) {
					case IS_TMP_VAR:
						value2 = value;
						ALLOC_ZVAL(value);
						*value = *value2;
						value->is_ref = 0;
						value->refcount = 0; /* lock will increase this */
						break;
					case IS_CONST:
						value2 = value;
						ALLOC_ZVAL(value);
						*value = *value2;
						zval_copy_ctor(value);
						value->is_ref = 0;
						value->refcount = 0; /* lock will increase this */
						break;
				}
				
				EX(Ts)[EX(opline)->result.u.var].var.ptr = value;
				PZVAL_LOCK(EX(Ts)[EX(opline)->result.u.var].var.ptr);
			}
			NEXT_OPCODE();
			case ZEND_ASSIGN_OBJ:
				zend_assign_to_object(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, EX(Ts)[EX(opline)->extended_value].var.ptr, EX(Ts) TSRMLS_CC);
				NEXT_OPCODE();
			case ZEND_ASSIGN: {
					zval *value;
					value = get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R);

 					zend_assign_to_variable(&EX(opline)->result, &EX(opline)->op1, &EX(opline)->op2, value, (EG(free_op2)?IS_TMP_VAR:EX(opline)->op2.op_type), EX(Ts) TSRMLS_CC);
					/* zend_assign_to_variable() always takes care of op2, never free it! */
				}
				NEXT_OPCODE();
			case ZEND_ASSIGN_REF:
				{
					zval **value_ptr_ptr = get_zval_ptr_ptr(&EX(opline)->op2, EX(Ts), BP_VAR_W);

					zend_assign_to_variable_reference(&EX(opline)->result, get_zval_ptr_ptr(&EX(opline)->op1, EX(Ts), BP_VAR_W), value_ptr_ptr, EX(Ts) TSRMLS_CC);
					NEXT_OPCODE();
				}
			case ZEND_JMP:
#if DEBUG_ZEND>=2
				printf("Jumping to %d\n", EX(opline)->op1.u.opline_num);
#endif
				EX(opline) = &op_array->opcodes[EX(opline)->op1.u.opline_num];
				continue;
				break;
			case ZEND_JMPZ: {
					znode *op1 = &EX(opline)->op1;
					
					if (!i_zend_is_true(get_zval_ptr(op1, EX(Ts), &EG(free_op1), BP_VAR_R))) {
#if DEBUG_ZEND>=2
						printf("Conditional jmp to %d\n", EX(opline)->op2.u.opline_num);
#endif
						EX(opline) = &op_array->opcodes[EX(opline)->op2.u.opline_num];
						FREE_OP(EX(Ts), op1, EG(free_op1));
						continue;
					}
					FREE_OP(EX(Ts), op1, EG(free_op1));
				}
				NEXT_OPCODE();
			case ZEND_JMPNZ: {
					znode *op1 = &EX(opline)->op1;
					
					if (zend_is_true(get_zval_ptr(op1, EX(Ts), &EG(free_op1), BP_VAR_R))) {
#if DEBUG_ZEND>=2
						printf("Conditional jmp to %d\n", EX(opline)->op2.u.opline_num);
#endif
						EX(opline) = &op_array->opcodes[EX(opline)->op2.u.opline_num];
						FREE_OP(EX(Ts), op1, EG(free_op1));
						continue;
					}
					FREE_OP(EX(Ts), op1, EG(free_op1));
				}
				NEXT_OPCODE();
			case ZEND_JMPZNZ: {
					znode *res = &EX(opline)->op1;
					
					if (zend_is_true(get_zval_ptr(res, EX(Ts), &EG(free_op1), BP_VAR_R))) {
#if DEBUG_ZEND>=2
						printf("Conditional jmp on true to %d\n", EX(opline)->extended_value);
#endif
						EX(opline) = &op_array->opcodes[EX(opline)->extended_value];
					} else {
#if DEBUG_ZEND>=2
						printf("Conditional jmp on false to %d\n", EX(opline)->op2.u.opline_num);
#endif
						EX(opline) = &op_array->opcodes[EX(opline)->op2.u.opline_num];
					}
					FREE_OP(EX(Ts), res, EG(free_op1));
				}
				continue;
				break;
			case ZEND_JMPZ_EX: {
					zend_op *original_opline = EX(opline);
					int retval = zend_is_true(get_zval_ptr(&original_opline->op1, EX(Ts), &EG(free_op1), BP_VAR_R));
					
					FREE_OP(EX(Ts), &original_opline->op1, EG(free_op1));
					EX(Ts)[original_opline->result.u.var].tmp_var.value.lval = retval;
					EX(Ts)[original_opline->result.u.var].tmp_var.type = IS_BOOL;
					if (!retval) {
#if DEBUG_ZEND>=2
						printf("Conditional jmp to %d\n", EX(opline)->op2.u.opline_num);
#endif
						EX(opline) = &op_array->opcodes[EX(opline)->op2.u.opline_num];
						continue;
					}
				}
				NEXT_OPCODE();
			case ZEND_JMPNZ_EX: {
					zend_op *original_opline = EX(opline);
					int retval = zend_is_true(get_zval_ptr(&original_opline->op1, EX(Ts), &EG(free_op1), BP_VAR_R));
					
					FREE_OP(EX(Ts), &original_opline->op1, EG(free_op1));
					EX(Ts)[original_opline->result.u.var].tmp_var.value.lval = retval;
					EX(Ts)[original_opline->result.u.var].tmp_var.type = IS_BOOL;
					if (retval) {
#if DEBUG_ZEND>=2
						printf("Conditional jmp to %d\n", EX(opline)->op2.u.opline_num);
#endif
						EX(opline) = &op_array->opcodes[EX(opline)->op2.u.opline_num];
						continue;
					}
				}
				NEXT_OPCODE();
			case ZEND_FREE:
				zendi_zval_dtor(EX(Ts)[EX(opline)->op1.u.var].tmp_var);
				NEXT_OPCODE();
			case ZEND_INIT_STRING:
				EX(Ts)[EX(opline)->result.u.var].tmp_var.value.str.val = emalloc(1);
				EX(Ts)[EX(opline)->result.u.var].tmp_var.value.str.val[0] = 0;
				EX(Ts)[EX(opline)->result.u.var].tmp_var.value.str.len = 0;
				EX(Ts)[EX(opline)->result.u.var].tmp_var.refcount = 1;
				EX(Ts)[EX(opline)->result.u.var].tmp_var.type = IS_STRING;
				EX(Ts)[EX(opline)->result.u.var].tmp_var.is_ref = 0;
				NEXT_OPCODE();
			case ZEND_ADD_CHAR:
				add_char_to_string(	&EX(Ts)[EX(opline)->result.u.var].tmp_var,
									get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_NA),
									&EX(opline)->op2.u.constant);
				/* FREE_OP is missing intentionally here - we're always working on the same temporary variable */
				NEXT_OPCODE();
			case ZEND_ADD_STRING:
				add_string_to_string(	&EX(Ts)[EX(opline)->result.u.var].tmp_var,
										get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_NA),
										&EX(opline)->op2.u.constant);
				/* FREE_OP is missing intentionally here - we're always working on the same temporary variable */
				NEXT_OPCODE();
			case ZEND_ADD_VAR: {
					zval *var = get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R);
					zval var_copy;
					int use_copy;

					zend_make_printable_zval(var, &var_copy, &use_copy);
					if (use_copy) {
						var = &var_copy;
					}
					add_string_to_string(	&EX(Ts)[EX(opline)->result.u.var].tmp_var,
											get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_NA),
											var);
					if (use_copy) {
						zval_dtor(var);
					}
					/* original comment, possibly problematic:
					 * FREE_OP is missing intentionally here - we're always working on the same temporary variable
					 * (Zeev):  I don't think it's problematic, we only use variables
					 * which aren't affected by FREE_OP(Ts, )'s anyway, unless they're
					 * string offsets or overloaded objects
					 */
					FREE_OP(EX(Ts), &EX(opline)->op2, EG(free_op2));
				}
				NEXT_OPCODE();
			case ZEND_IMPORT_FUNCTION:
				{
					zend_class_entry *ce;
					zend_function *function;
					
					ce = EX(Ts)[EX(opline)->op1.u.var].EA.class_entry;

					if (EX(opline)->op2.op_type != IS_UNUSED) {
						char *function_name_strval;
						int function_name_strlen;

						function_name_strval = EX(opline)->op2.u.constant.value.str.val;
						function_name_strlen = EX(opline)->op2.u.constant.value.str.len;

						if (zend_hash_find(&ce->function_table, function_name_strval, function_name_strlen + 1, (void **) &function)==FAILURE) {
							zend_error(E_ERROR, "Import: function %s() not found", function_name_strval);
						}
						if (zend_hash_add(EG(function_table), function_name_strval, function_name_strlen + 1, function, sizeof(zend_function), NULL) == FAILURE) {
							zend_error(E_ERROR, "Import: function %s() already exists in current scope", function_name_strval);
						}
						function_add_ref(function);
					} else {
						zend_function tmp_zend_function;

						zend_hash_copy(EG(function_table), &ce->function_table, (copy_ctor_func_t) function_add_ref, &tmp_zend_function, sizeof(zend_function));
					}
					NEXT_OPCODE();
				}
			case ZEND_IMPORT_CLASS:
				{
					zend_class_entry *ce;
					zend_class_entry **import_ce;
					
					ce = EX(Ts)[EX(opline)->op1.u.var].EA.class_entry;

					if (EX(opline)->op2.op_type != IS_UNUSED) {
						char *class_name_strval;
						int class_name_strlen;
						
						class_name_strval = EX(opline)->op2.u.constant.value.str.val;
						class_name_strlen = EX(opline)->op2.u.constant.value.str.len;
						
						if (zend_hash_find(&ce->class_table, class_name_strval, class_name_strlen + 1, (void **) &import_ce)==FAILURE) {
							zend_error(E_ERROR, "Import: class %s not found", class_name_strval);
						}
						if (zend_hash_add(EG(class_table), class_name_strval, class_name_strlen + 1, import_ce, sizeof(zend_class_entry *), NULL) == FAILURE) {
							zend_error(E_ERROR, "Import: class %s already exists in current scope", class_name_strval);
						}
						zend_class_add_ref(import_ce);
					} else {
						zend_class_entry *tmp_zend_class_entry;

						zend_hash_copy(EG(class_table), &ce->class_table, (copy_ctor_func_t) zend_class_add_ref, &tmp_zend_class_entry, sizeof(zend_class_entry *));
					}

					NEXT_OPCODE();
				}
			case ZEND_IMPORT_CONST:
				{
					zend_class_entry *ce;
					zval **import_constant;
					zend_constant c;
					
					ce = EX(Ts)[EX(opline)->op1.u.var].EA.class_entry;
					if (EX(opline)->op2.op_type != IS_UNUSED) {
						char *const_name_strval;
						int const_name_strlen;
						
						const_name_strval = EX(opline)->op2.u.constant.value.str.val;
						const_name_strlen = EX(opline)->op2.u.constant.value.str.len;
						
						if (zend_hash_find(&ce->constants_table, const_name_strval, const_name_strlen + 1, (void **) &import_constant)==FAILURE) {
							zend_error(E_ERROR, "Import: constant %s not found", const_name_strval);
						}
						c.value = **import_constant;
						zval_copy_ctor(&c.value);
						c.flags = CONST_CS;
						c.name = zend_strndup(const_name_strval, const_name_strlen);
						c.name_len = const_name_strlen + 1;

						if (zend_register_constant(&c TSRMLS_CC) == FAILURE) {
							zend_error(E_ERROR, "Import: unable to register constant %s", const_name_strval);
						}
					} else {
						HashPosition pos;
						char *key;
						uint key_length;
						int key_type;
						ulong dummy;

						zend_hash_internal_pointer_reset_ex(&ce->constants_table, &pos);
						while(zend_hash_get_current_data_ex(&ce->constants_table, (void **)&import_constant, &pos) == SUCCESS) {
							key_type = zend_hash_get_current_key_ex(&ce->constants_table, &key, &key_length, &dummy, 0, &pos);

							c.value = **import_constant;
							zval_copy_ctor(&c.value);
							c.flags = CONST_CS;
							c.name = zend_strndup(key, key_length - 1);
							c.name_len = key_length;

							if (zend_register_constant(&c TSRMLS_CC) == FAILURE) {
								zend_error(E_ERROR, "Import: unable to register constant %s", key);
							}
							zend_hash_move_forward_ex(&ce->constants_table, &pos);
						}
					}
					NEXT_OPCODE();
				}
			case ZEND_FETCH_CLASS:
				{
					zend_class_entry **pce;
					
					if (EX(opline)->op1.op_type == IS_UNUSED) {
						zval tmp;
						zval *class_name;
						zend_bool is_const;
						char *class_name_strval;
						zend_uint class_name_strlen;
						
						if (EX(opline)->extended_value == ZEND_FETCH_CLASS_SELF) {
							if (!EG(scope)) {
								zend_error(E_ERROR, "Cannot fetch self:: when no class scope is active");
							}
							EX(Ts)[EX(opline)->result.u.var].EA.class_entry = EG(scope);
							NEXT_OPCODE();
						} else if (EX(opline)->extended_value == ZEND_FETCH_CLASS_MAIN) {
							EX(Ts)[EX(opline)->result.u.var].EA.class_entry = EG(main_class_ptr);
							NEXT_OPCODE();
						} else if (EX(opline)->extended_value == ZEND_FETCH_CLASS_PARENT) {
							if (!EG(scope)) {
								zend_error(E_ERROR, "Cannot fetch parent:: when no class scope is active");
							}
							if (!EG(scope)->parent) {
								zend_error(E_ERROR, "Cannot fetch parent:: as current class scope has no parent");
							}
							EX(Ts)[EX(opline)->result.u.var].EA.class_entry = EG(scope)->parent;
							NEXT_OPCODE();
						} 

						is_const = (EX(opline)->op2.op_type == IS_CONST);

						if (is_const) {
							class_name_strval = EX(opline)->op2.u.constant.value.str.val;
							class_name_strlen = EX(opline)->op2.u.constant.value.str.len;
						} else {
							class_name = get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R);

							tmp = *class_name;
							zval_copy_ctor(&tmp);
							convert_to_string(&tmp);
							zend_str_tolower(tmp.value.str.val, tmp.value.str.len);

							class_name_strval = tmp.value.str.val;
							class_name_strlen = tmp.value.str.len;
						}
					
						if (zend_lookup_class(class_name_strval, class_name_strlen, &pce TSRMLS_CC) == FAILURE) {
							zend_error(E_ERROR, "Class '%s' not found", class_name_strval);
						} else {
							EX(Ts)[EX(opline)->result.u.var].EA.class_entry = *pce;
						}
						if (!is_const) {
							zval_dtor(&tmp);
							FREE_OP(EX(Ts), &EX(opline)->op2, EG(free_op2));
						}
					} else {
						if (zend_hash_find(&EX(Ts)[EX(opline)->op1.u.var].EA.class_entry->class_table, EX(opline)->op2.u.constant.value.str.val, EX(opline)->op2.u.constant.value.str.len+1, (void **)&pce) == FAILURE) {
							zend_error(E_ERROR, "Class '%s' not found", EX(opline)->op2.u.constant.value.str.val);
						} else {
							EX(Ts)[EX(opline)->result.u.var].EA.class_entry = *pce;
						}
					}
					NEXT_OPCODE();
				}
			case ZEND_INIT_CTOR_CALL:
				{
					zend_ptr_stack_n_push(&EG(arg_types_stack), 3, EX(fbc), EX(object), EX(calling_scope));

					if (EX(opline)->op1.op_type == IS_VAR) {
						SELECTIVE_PZVAL_LOCK(*EX(Ts)[EX(opline)->op1.u.var].var.ptr_ptr, &EX(opline)->op1);
					}

					/* We are not handling overloaded classes right now */
					EX(object) = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
					if (!PZVAL_IS_REF(EX(object))) {
						EX(object)->refcount++; /* For $this pointer */
					} else {
						zval *this_ptr;

						ALLOC_ZVAL(this_ptr);
						*this_ptr = *EX(object);
						INIT_PZVAL(this_ptr);
						zval_copy_ctor(this_ptr);
						EX(object) = this_ptr;
					}

					EX(fbc) = EX(fbc_constructor);
					if(EX(fbc)->type == ZEND_USER_FUNCTION) { /* HACK!! */
						/*  The scope should be the scope of the class where the constructor
							was initially declared in */
						EX(calling_scope) = EX(fbc)->common.scope;
					} else {
						EX(calling_scope) = NULL;
					}

					NEXT_OPCODE();
				}
			case ZEND_INIT_METHOD_CALL:
				{
					zval *function_name;
					char *function_name_strval;
					int function_name_strlen;
					
					zend_ptr_stack_n_push(&EG(arg_types_stack), 3, EX(fbc), EX(object), EX(calling_scope));

					function_name = get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R);
					function_name_strval = function_name->value.str.val;
					function_name_strlen = function_name->value.str.len;

					EX(calling_scope) = EG(scope);

					if (EX(opline)->extended_value == ZEND_FETCH_FROM_THIS) {
						if (!EG(This)) {
							zend_error(E_ERROR, "Can't fetch $this as not in object context");
						}
						EX(object) = EG(This);
					} else {
						EX(object) = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
					}
							
					if (EX(object) && EX(object)->type == IS_OBJECT) {
						EX(fbc) = Z_OBJ_HT_P(EX(object))->get_method(EX(object), function_name_strval, function_name_strlen TSRMLS_CC);
					} else {
						zend_error(E_ERROR, "Call to a member function on a non-object");
					}
					if (!EX(fbc)) {
						zend_error(E_ERROR, "Call to undefined function:  %s()", function_name_strval);
					}

					if (!PZVAL_IS_REF(EX(object))) {
						EX(object)->refcount++; /* For $this pointer */
					} else {
						zval *this_ptr;
						ALLOC_ZVAL(this_ptr);
						*this_ptr = *EX(object);
						INIT_PZVAL(this_ptr);
						zval_copy_ctor(this_ptr);
						EX(object) = this_ptr;
					}

					if(EX(fbc)->type == ZEND_USER_FUNCTION) {
						EX(calling_scope) = EX(fbc)->common.scope;
					} else {
						EX(calling_scope) = NULL;
					}

					FREE_OP(EX(Ts), &EX(opline)->op2, EG(free_op2));
					
					NEXT_OPCODE();
				}
			case ZEND_INIT_STATIC_METHOD_CALL:
				{
					zval *function_name;
					zend_function *function;
					zval tmp;
					zend_class_entry *ce;
					zend_bool is_const;
					char *function_name_strval;
					int function_name_strlen;

					zend_ptr_stack_n_push(&EG(arg_types_stack), 3, EX(fbc), EX(object), EX(calling_scope));

					is_const = (EX(opline)->op2.op_type == IS_CONST);

					if (is_const) {
						function_name_strval = EX(opline)->op2.u.constant.value.str.val;
						function_name_strlen = EX(opline)->op2.u.constant.value.str.len;
					} else {
						function_name = get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R);

						tmp = *function_name;
						zval_copy_ctor(&tmp);
						convert_to_string(&tmp);
						zend_str_tolower(tmp.value.str.val, tmp.value.str.len);

						function_name_strval = tmp.value.str.val;
						function_name_strlen = tmp.value.str.len;
					}
					
					if ((EX(object) = EG(This))) {
						EX(object)->refcount++;
					}

					ce = EX(Ts)[EX(opline)->op1.u.var].EA.class_entry;

					EX(calling_scope) = ce;

					if (zend_hash_find(&ce->function_table, function_name_strval, function_name_strlen+1, (void **) &function)==FAILURE) {
						zend_error(E_ERROR, "Call to undefined function:  %s()", function_name_strval);
					}

					if (!is_const) {
						zval_dtor(&tmp);
						FREE_OP(EX(Ts), &EX(opline)->op2, EG(free_op2));
					}

					EX(fbc) = function;

					NEXT_OPCODE();
				}
			case ZEND_INIT_FCALL_BY_NAME:
				{
					zval *function_name;
					zend_function *function;
					zval tmp;
					zend_bool is_const;
					char *function_name_strval;
					int function_name_strlen;

					zend_ptr_stack_n_push(&EG(arg_types_stack), 3, EX(fbc), EX(object), EX(calling_scope));

					is_const = (EX(opline)->op2.op_type == IS_CONST);

					if (is_const) {
						function_name_strval = EX(opline)->op2.u.constant.value.str.val;
						function_name_strlen = EX(opline)->op2.u.constant.value.str.len;
					} else {
						function_name = get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R);

						tmp = *function_name;
						zval_copy_ctor(&tmp);
						convert_to_string(&tmp);
						zend_str_tolower(tmp.value.str.val, tmp.value.str.len);

						function_name_strval = tmp.value.str.val;
						function_name_strlen = tmp.value.str.len;
					}
					

					do {
						if (EG(scope)) {
							if (zend_hash_find(&EG(scope)->function_table, function_name_strval, function_name_strlen+1, (void **) &function) == SUCCESS) {
								if ((EX(object) = EG(This))) {
									EX(object)->refcount++;
								}
								EX(calling_scope) = EG(scope);
								break;
							}
						}
						if (zend_hash_find(EG(function_table), function_name_strval, function_name_strlen+1, (void **) &function)==FAILURE) {
							zend_error(E_ERROR, "Call to undefined function:  %s()", function_name_strval);
						}
						EX(calling_scope) = function->common.scope;
						EX(object) = NULL;
					} while (0);
					
					if (!is_const) {
						zval_dtor(&tmp);
						FREE_OP(EX(Ts), &EX(opline)->op2, EG(free_op2));
					}
					EX(fbc) = function;

					NEXT_OPCODE();
				}
			case ZEND_DO_FCALL_BY_NAME:
				EX(function_state).function = EX(fbc);
				goto do_fcall_common;
			case ZEND_DO_FCALL: {
					zval *fname = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
					
					zend_ptr_stack_n_push(&EG(arg_types_stack), 3, EX(fbc), EX(object), EX(calling_scope));

					do {
						if (EG(scope)) {
							if (zend_hash_find(&EG(scope)->function_table, fname->value.str.val, fname->value.str.len+1, (void **) &EX(function_state).function) == SUCCESS) {
								if ((EX(object) = EG(This))) {
									EX(object)->refcount++;
								}
								EX(calling_scope) = EG(scope);
								break;
							}
						}
						if (zend_hash_find(EG(function_table), fname->value.str.val, fname->value.str.len+1, (void **) &EX(function_state).function)==FAILURE) {
							zend_error(E_ERROR, "Unknown function:  %s()\n", fname->value.str.val);
						}
						EX(object) = NULL;
						EX(calling_scope) = EX(function_state).function->common.scope;
					} while (0);

					FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));
					
					goto do_fcall_common;
				}
do_fcall_common:
				{
					zval **original_return_value;
					zend_class_entry *current_scope;
					zval *current_this;
					int return_value_used = RETURN_VALUE_USED(EX(opline));

					zend_ptr_stack_n_push(&EG(argument_stack), 2, (void *) EX(opline)->extended_value, NULL);
					current_scope = EG(scope);
					EG(scope) = EX(calling_scope);

					current_this = EG(This);
					EG(This) = EX(object);

					EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr = &EX(Ts)[EX(opline)->result.u.var].var.ptr;

					if (EX(function_state).function->type==ZEND_INTERNAL_FUNCTION) {	
						ALLOC_ZVAL(EX(Ts)[EX(opline)->result.u.var].var.ptr);
						INIT_ZVAL(*(EX(Ts)[EX(opline)->result.u.var].var.ptr));
						((zend_internal_function *) EX(function_state).function)->handler(EX(opline)->extended_value, EX(Ts)[EX(opline)->result.u.var].var.ptr, EX(object), return_value_used TSRMLS_CC);
						EG(current_execute_data) = &execute_data;
						EX(Ts)[EX(opline)->result.u.var].var.ptr->is_ref = 0;
						EX(Ts)[EX(opline)->result.u.var].var.ptr->refcount = 1;
						if (!return_value_used) {
							zval_ptr_dtor(&EX(Ts)[EX(opline)->result.u.var].var.ptr);
						}
					} else if (EX(function_state).function->type==ZEND_USER_FUNCTION) {
						HashTable *calling_symbol_table;

						EX(Ts)[EX(opline)->result.u.var].var.ptr = NULL;
						if (EG(symtable_cache_ptr)>=EG(symtable_cache)) {
							/*printf("Cache hit!  Reusing %x\n", symtable_cache[symtable_cache_ptr]);*/
							EX(function_state).function_symbol_table = *(EG(symtable_cache_ptr)--);
						} else {
							ALLOC_HASHTABLE(EX(function_state).function_symbol_table);
							zend_hash_init(EX(function_state).function_symbol_table, 0, NULL, ZVAL_PTR_DTOR, 0);
							/*printf("Cache miss!  Initialized %x\n", function_state.function_symbol_table);*/
						}
						calling_symbol_table = EG(active_symbol_table);
						EG(active_symbol_table) = EX(function_state).function_symbol_table;
						original_return_value = EG(return_value_ptr_ptr);
						EG(return_value_ptr_ptr) = EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr;
						EG(active_op_array) = (zend_op_array *) EX(function_state).function;

						zend_execute(EG(active_op_array) TSRMLS_CC);

						if (return_value_used && !EX(Ts)[EX(opline)->result.u.var].var.ptr) {
							if (!EG(exception)) {
								ALLOC_ZVAL(EX(Ts)[EX(opline)->result.u.var].var.ptr);
								INIT_ZVAL(*EX(Ts)[EX(opline)->result.u.var].var.ptr);
							}
						} else if (!return_value_used && EX(Ts)[EX(opline)->result.u.var].var.ptr) {
							zval_ptr_dtor(&EX(Ts)[EX(opline)->result.u.var].var.ptr);
						}

						EG(opline_ptr) = &EX(opline);
						EG(active_op_array) = op_array;
						EG(return_value_ptr_ptr)=original_return_value;
						if (EG(symtable_cache_ptr)>=EG(symtable_cache_limit)) {
							zend_hash_destroy(EX(function_state).function_symbol_table);
							FREE_HASHTABLE(EX(function_state).function_symbol_table);
						} else {
							*(++EG(symtable_cache_ptr)) = EX(function_state).function_symbol_table;
							zend_hash_clean(*EG(symtable_cache_ptr));
						}
						EG(active_symbol_table) = calling_symbol_table;
					} else { /* ZEND_OVERLOADED_FUNCTION */
						ALLOC_ZVAL(EX(Ts)[EX(opline)->result.u.var].var.ptr);
						INIT_ZVAL(*(EX(Ts)[EX(opline)->result.u.var].var.ptr));

						if(EX(object)) {
							Z_OBJ_HT_P(EX(object))->call_method(EX(fbc)->common.function_name, EX(opline)->extended_value, EX(Ts)[EX(opline)->result.u.var].var.ptr, EX(object), return_value_used TSRMLS_CC);
						} else {
							zend_error(E_ERROR, "Cannot call overloaded function for non-object");
						}
							
						efree(EX(fbc));
						if (!return_value_used) {
							zval_ptr_dtor(&EX(Ts)[EX(opline)->result.u.var].var.ptr);
						}
					}
					zend_ptr_stack_n_pop(&EG(arg_types_stack), 3, &EX(calling_scope), &EX(object), &EX(fbc));
					
					EX(function_state).function = (zend_function *) op_array;
					EG(function_state_ptr) = &EX(function_state);
					zend_ptr_stack_clear_multiple(TSRMLS_C);

					EG(scope) = current_scope;

					if (EG(This)) {
						zval_ptr_dtor(&EG(This));
					}

					EG(This) = current_this;
					
					if (EG(exception)) {
						if (EX(opline)->op2.u.opline_num == -1) {
							RETURN_FROM_EXECUTE_LOOP(execute_data);
						} else {
							EX(opline) = &op_array->opcodes[EX(opline)->op2.u.opline_num];
							continue;
						}
					}
 				}
				NEXT_OPCODE();
			case ZEND_RETURN: {
					zval *retval_ptr;
					zval **retval_ptr_ptr;
					
					if ((EG(active_op_array)->return_reference == ZEND_RETURN_REF) &&
						(EX(opline)->op1.op_type != IS_CONST) && 
						(EX(opline)->op1.op_type != IS_TMP_VAR)) {
						
						retval_ptr_ptr = get_zval_ptr_ptr(&EX(opline)->op1, EX(Ts), BP_VAR_W);

						if (!retval_ptr_ptr) {
							zend_error(E_ERROR, "Cannot return overloaded elements or string offsets by reference");
						}

						SEPARATE_ZVAL_TO_MAKE_IS_REF(retval_ptr_ptr);
						(*retval_ptr_ptr)->refcount++;
						
						(*EG(return_value_ptr_ptr)) = (*retval_ptr_ptr);
					} else {
						retval_ptr = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
					
						if (!EG(free_op1)) { /* Not a temp var */
							if (PZVAL_IS_REF(retval_ptr) && retval_ptr->refcount > 0) {
								ALLOC_ZVAL(*(EG(return_value_ptr_ptr)));
								**EG(return_value_ptr_ptr) = *retval_ptr;
								(*EG(return_value_ptr_ptr))->is_ref = 0;
								(*EG(return_value_ptr_ptr))->refcount = 1;
								zval_copy_ctor(*EG(return_value_ptr_ptr));
							} else {
								*EG(return_value_ptr_ptr) = retval_ptr;
								retval_ptr->refcount++;
							}
						} else {
							ALLOC_ZVAL(*(EG(return_value_ptr_ptr)));
							**EG(return_value_ptr_ptr) = *retval_ptr;
							(*EG(return_value_ptr_ptr))->refcount = 1;
							(*EG(return_value_ptr_ptr))->is_ref = 0;
						}
					}
					RETURN_FROM_EXECUTE_LOOP(execute_data);
				}
				break;
			case ZEND_THROW:
				{
					zval *value;
					zval *exception;

					value = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
			
					if (value->type != IS_OBJECT) {
						zend_error(E_ERROR, "Can only throw objects");
					}
					/* Not sure if a complete copy is what we want here */
					MAKE_STD_ZVAL(exception);
					*exception = *value;
					if (!EG(free_op1)) {
						zval_copy_ctor(exception);
					}
					INIT_PZVAL(exception);
					EG(exception) = exception;
					
					if (EX(opline)->op2.u.opline_num == -1) {
						RETURN_FROM_EXECUTE_LOOP(execute_data);
					} else {
						EX(opline) = &op_array->opcodes[EX(opline)->op2.u.opline_num];
						continue;
					}
				}
				NEXT_OPCODE();
			case ZEND_CATCH:
				{
					zend_class_entry *ce;

					/* Check if this is really an exception, if not, jump over code */
					if (EG(exception) == NULL) {
						EX(opline) = &op_array->opcodes[EX(opline)->extended_value];
						continue;
					}
					ce = Z_OBJCE_P(EG(exception));
					if (ce != EX(Ts)[EX(opline)->op1.u.var].EA.class_entry) {
						while (ce->parent) {
							if (ce->parent == EX(Ts)[EX(opline)->op1.u.var].EA.class_entry) {
								goto exception_should_be_taken;
							}
							ce = ce->parent;
						}
						EX(opline) = &op_array->opcodes[EX(opline)->extended_value];
						continue;
					}
exception_should_be_taken:
					zend_hash_update(EG(active_symbol_table), EX(opline)->op2.u.constant.value.str.val,
						EX(opline)->op2.u.constant.value.str.len+1, &EG(exception), sizeof(zval *), (void **) NULL);
					EG(exception) = NULL;
					NEXT_OPCODE();
				}
			case ZEND_SEND_VAL: 
				if (EX(opline)->extended_value==ZEND_DO_FCALL_BY_NAME
					&& ARG_SHOULD_BE_SENT_BY_REF(EX(opline)->op2.u.opline_num, EX(fbc), EX(fbc)->common.arg_types)) {
						zend_error(E_ERROR, "Cannot pass parameter %d by reference", EX(opline)->op2.u.opline_num);
				}
				{
					zval *valptr;
					zval *value;

					value = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);

					ALLOC_ZVAL(valptr);
					*valptr = *value;
					if (!EG(free_op1)) {
						zval_copy_ctor(valptr);
					}
					INIT_PZVAL(valptr);
					zend_ptr_stack_push(&EG(argument_stack), valptr);
				}
				NEXT_OPCODE();
			case ZEND_SEND_VAR_NO_REF:
				if (EX(opline)->extended_value & ZEND_ARG_COMPILE_TIME_BOUND) { /* Had function_ptr at compile_time */
					if (!(EX(opline)->extended_value & ZEND_ARG_SEND_BY_REF)) {
						goto send_by_var;
					}
				} else if (!ARG_SHOULD_BE_SENT_BY_REF(EX(opline)->op2.u.opline_num, EX(fbc), EX(fbc)->common.arg_types)) {
					goto send_by_var;
				}
				{
					zval *varptr;
					varptr = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);

					if (varptr != &EG(uninitialized_zval) && (PZVAL_IS_REF(varptr) || varptr->refcount == 1)) {
						varptr->is_ref = 1;
						varptr->refcount++;
						zend_ptr_stack_push(&EG(argument_stack), varptr);
						NEXT_OPCODE();
					}
					zend_error(E_ERROR, "Only variables can be passed by reference");
				}
				NEXT_OPCODE();
			case ZEND_SEND_VAR:
					if ((EX(opline)->extended_value == ZEND_DO_FCALL_BY_NAME)
						&& ARG_SHOULD_BE_SENT_BY_REF(EX(opline)->op2.u.opline_num, EX(fbc), EX(fbc)->common.arg_types)) {
						goto send_by_ref;
					}
send_by_var:
					{
						zval *varptr;
						varptr = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);

						if (varptr == &EG(uninitialized_zval)) {
							ALLOC_ZVAL(varptr);
							INIT_ZVAL(*varptr);
							varptr->refcount = 0;
						} else if (PZVAL_IS_REF(varptr)) {
							zval *original_var = varptr;

							ALLOC_ZVAL(varptr);
							*varptr = *original_var;
							varptr->is_ref = 0;
							varptr->refcount = 0;
							zval_copy_ctor(varptr);
						}
						varptr->refcount++;
						zend_ptr_stack_push(&EG(argument_stack), varptr);
						FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));  /* for string offsets */
					}
					NEXT_OPCODE();
send_by_ref:
			case ZEND_SEND_REF: {
					zval **varptr_ptr;
					zval *varptr;
					varptr_ptr = get_zval_ptr_ptr(&EX(opline)->op1, EX(Ts), BP_VAR_W);

					if (!varptr_ptr) {
						zend_error(E_ERROR, "Only variables can be passed by reference");
					}

					SEPARATE_ZVAL_TO_MAKE_IS_REF(varptr_ptr);
					varptr = *varptr_ptr;
					varptr->refcount++;
					zend_ptr_stack_push(&EG(argument_stack), varptr);
				}
				NEXT_OPCODE();
			case ZEND_RECV: {
					zval **param;

					if (zend_ptr_stack_get_arg(EX(opline)->op1.u.constant.value.lval, (void **) &param TSRMLS_CC)==FAILURE) {
						zend_error(E_WARNING, "Missing argument %d for %s()\n", EX(opline)->op1.u.constant.value.lval, get_active_function_name(TSRMLS_C));
						if (EX(opline)->result.op_type == IS_VAR) {
							PZVAL_UNLOCK(*EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr);
						}
					} else if (PZVAL_IS_REF(*param)) {
						zend_assign_to_variable_reference(NULL, get_zval_ptr_ptr(&EX(opline)->result, EX(Ts), BP_VAR_W), param, NULL TSRMLS_CC);
					} else {
						zend_assign_to_variable(NULL, &EX(opline)->result, NULL, *param, IS_VAR, EX(Ts) TSRMLS_CC);
					}
				}
				NEXT_OPCODE();
			case ZEND_RECV_INIT: {
					zval **param, *assignment_value;

					if (zend_ptr_stack_get_arg(EX(opline)->op1.u.constant.value.lval, (void **) &param TSRMLS_CC)==FAILURE) {
						if (EX(opline)->op2.u.constant.type == IS_CONSTANT || EX(opline)->op2.u.constant.type==IS_CONSTANT_ARRAY) {
							zval *default_value;

							ALLOC_ZVAL(default_value);
							*default_value = EX(opline)->op2.u.constant;
							if (EX(opline)->op2.u.constant.type==IS_CONSTANT_ARRAY) {
								zval_copy_ctor(default_value);
							}
							default_value->refcount=1;
							zval_update_constant(&default_value, 0 TSRMLS_CC);
							default_value->refcount=0;
							default_value->is_ref=0;
							param = &default_value;
							assignment_value = default_value;
						} else {
							param = NULL;
							assignment_value = &EX(opline)->op2.u.constant;
						}
						zend_assign_to_variable(NULL, &EX(opline)->result, NULL, assignment_value, IS_VAR, EX(Ts) TSRMLS_CC);
					} else {
						assignment_value = *param;
						if (PZVAL_IS_REF(assignment_value)) {
							zend_assign_to_variable_reference(NULL, get_zval_ptr_ptr(&EX(opline)->result, EX(Ts), BP_VAR_W), param, NULL TSRMLS_CC);
						} else {
							zend_assign_to_variable(NULL, &EX(opline)->result, NULL, assignment_value, IS_VAR, EX(Ts) TSRMLS_CC);
						}
					}
				}
				NEXT_OPCODE();
			case ZEND_BOOL:
				/* PHP 3.0 returned "" for false and 1 for true, here we use 0 and 1 for now */
				EX(Ts)[EX(opline)->result.u.var].tmp_var.value.lval = zend_is_true(get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R));
				EX(Ts)[EX(opline)->result.u.var].tmp_var.type = IS_BOOL;
				FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));
				NEXT_OPCODE();
			case ZEND_BRK:
			case ZEND_CONT: {
					zval *nest_levels_zval = get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R);
					zval tmp;
					int array_offset, nest_levels, original_nest_levels;
					zend_brk_cont_element *jmp_to;

					if (nest_levels_zval->type != IS_LONG) {
						tmp = *nest_levels_zval;
						zval_copy_ctor(&tmp);
						convert_to_long(&tmp);
						nest_levels = tmp.value.lval;
					} else {
						nest_levels = nest_levels_zval->value.lval;
					}
					original_nest_levels = nest_levels;
					array_offset = EX(opline)->op1.u.opline_num;
					do {
						if (array_offset==-1) {
							zend_error(E_ERROR, "Cannot break/continue %d level%s", original_nest_levels, (original_nest_levels == 1) ? "" : "s");
						}
						jmp_to = &op_array->brk_cont_array[array_offset];
						if (nest_levels>1) {
							zend_op *brk_opline = &op_array->opcodes[jmp_to->brk];

							switch (brk_opline->opcode) {
								case ZEND_SWITCH_FREE:
									zend_switch_free(brk_opline, EX(Ts) TSRMLS_CC);
									break;
								case ZEND_FREE:
									zendi_zval_dtor(EX(Ts)[brk_opline->op1.u.var].tmp_var);
									break;
							}
						}
						array_offset = jmp_to->parent;
					} while (--nest_levels > 0);

					if (EX(opline)->opcode == ZEND_BRK) {
						EX(opline) = op_array->opcodes+jmp_to->brk;
					} else {
						EX(opline) = op_array->opcodes+jmp_to->cont;
					}
					FREE_OP(EX(Ts), &EX(opline)->op2, EG(free_op2));
					continue;
				}
				/* Never reaches this point */
			case ZEND_CASE: {
					int switch_expr_is_overloaded=0;

					if (EX(opline)->op1.op_type==IS_VAR) {
						if (EX(Ts)[EX(opline)->op1.u.var].var.ptr_ptr) {
							PZVAL_LOCK(*EX(Ts)[EX(opline)->op1.u.var].var.ptr_ptr);
						} else {
							switch_expr_is_overloaded = 1;
							if (EX(Ts)[EX(opline)->op1.u.var].EA.type==IS_STRING_OFFSET) {
								EX(Ts)[EX(opline)->op1.u.var].EA.data.str_offset.str->refcount++;
							}
						}
					}
					is_equal_function(&EX(Ts)[EX(opline)->result.u.var].tmp_var, 
								 get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R),
								 get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R) TSRMLS_CC);

					FREE_OP(EX(Ts), &EX(opline)->op2, EG(free_op2));
					if (switch_expr_is_overloaded) {
						/* We only free op1 if this is a string offset,
						 * Since if it is a TMP_VAR, it'll be reused by
						 * other CASE opcodes (whereas string offsets
						 * are allocated at each get_zval_ptr())
						 */
						FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));
						EX(Ts)[EX(opline)->op1.u.var].var.ptr_ptr = NULL;
						AI_USE_PTR(EX(Ts)[EX(opline)->op1.u.var].var);
					}
				}
				NEXT_OPCODE();
			case ZEND_SWITCH_FREE:
				zend_switch_free(EX(opline), EX(Ts) TSRMLS_CC);
				NEXT_OPCODE();
			case ZEND_NEW:
				{
					EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr = &EX(Ts)[EX(opline)->result.u.var].var.ptr;
					ALLOC_ZVAL(EX(Ts)[EX(opline)->result.u.var].var.ptr);
					object_init_ex(EX(Ts)[EX(opline)->result.u.var].var.ptr, EX(Ts)[EX(opline)->op1.u.var].EA.class_entry);
					EX(Ts)[EX(opline)->result.u.var].var.ptr->refcount=1;
					EX(Ts)[EX(opline)->result.u.var].var.ptr->is_ref=1;
				}
				NEXT_OPCODE();
			case ZEND_CLONE:
				{
					zval *obj = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);

					EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr = &EX(Ts)[EX(opline)->result.u.var].var.ptr;
					ALLOC_ZVAL(EX(Ts)[EX(opline)->result.u.var].var.ptr);
					EX(Ts)[EX(opline)->result.u.var].var.ptr->value.obj = Z_OBJ_HT_P(obj)->clone_obj(obj TSRMLS_CC);
					EX(Ts)[EX(opline)->result.u.var].var.ptr->type = IS_OBJECT;
					EX(Ts)[EX(opline)->result.u.var].var.ptr->refcount=1;
					EX(Ts)[EX(opline)->result.u.var].var.ptr->is_ref=1;
					NEXT_OPCODE();
				}
			case ZEND_FETCH_CONSTANT:
				{
					zend_class_entry *ce;
					zval **value;

					if (EX(opline)->op1.op_type == IS_UNUSED) {
						if (EG(scope)) {
							ce = EG(scope);
							if (zend_hash_find(&ce->constants_table, EX(opline)->op2.u.constant.value.str.val, EX(opline)->op2.u.constant.value.str.len+1, (void **) &value) == SUCCESS) {
								zval_update_constant(value, (void *) 1 TSRMLS_CC);
								EX(Ts)[EX(opline)->result.u.var].tmp_var = **value;
								zval_copy_ctor(&EX(Ts)[EX(opline)->result.u.var].tmp_var);
								NEXT_OPCODE();
							}
						}
						if (!zend_get_constant(EX(opline)->op2.u.constant.value.str.val, EX(opline)->op2.u.constant.value.str.len, &EX(Ts)[EX(opline)->result.u.var].tmp_var TSRMLS_CC)) {
							zend_error(E_NOTICE, "Use of undefined constant %s - assumed '%s'",
										EX(opline)->op2.u.constant.value.str.val,
										EX(opline)->op2.u.constant.value.str.val);
							EX(Ts)[EX(opline)->result.u.var].tmp_var = EX(opline)->op2.u.constant;
							zval_copy_ctor(&EX(Ts)[EX(opline)->result.u.var].tmp_var);
						}
						NEXT_OPCODE();
					}
					
					ce = EX(Ts)[EX(opline)->op1.u.var].EA.class_entry;
					
					if (&ce->constants_table == &EG(main_class_ptr)->constants_table) {
						if (!zend_get_constant(EX(opline)->op2.u.constant.value.str.val, EX(opline)->op2.u.constant.value.str.len, &EX(Ts)[EX(opline)->result.u.var].tmp_var TSRMLS_CC)) {
							zend_error(E_NOTICE, "Use of undefined constant %s - assumed '%s'",
										EX(opline)->op2.u.constant.value.str.val,
										EX(opline)->op2.u.constant.value.str.val);
							EX(Ts)[EX(opline)->result.u.var].tmp_var = EX(opline)->op2.u.constant;
							zval_copy_ctor(&EX(Ts)[EX(opline)->result.u.var].tmp_var);
						}
						NEXT_OPCODE();
					}
					if (zend_hash_find(&ce->constants_table, EX(opline)->op2.u.constant.value.str.val, EX(opline)->op2.u.constant.value.str.len+1, (void **) &value) == SUCCESS) {
						zval_update_constant(value, (void *) 1 TSRMLS_CC);
						EX(Ts)[EX(opline)->result.u.var].tmp_var = **value;
						zval_copy_ctor(&EX(Ts)[EX(opline)->result.u.var].tmp_var);
					} else {
						zend_error(E_ERROR, "Undefined class constant '%s'", EX(opline)->op2.u.constant.value.str.val);
					}
				}
				NEXT_OPCODE();
			case ZEND_INIT_ARRAY:
			case ZEND_ADD_ARRAY_ELEMENT: {
					zval *array_ptr = &EX(Ts)[EX(opline)->result.u.var].tmp_var;
					zval *expr_ptr, **expr_ptr_ptr = NULL;
					zval *offset=get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R);

					if (EX(opline)->extended_value) {
						expr_ptr_ptr=get_zval_ptr_ptr(&EX(opline)->op1, EX(Ts), BP_VAR_R);
						expr_ptr = *expr_ptr_ptr;
					} else {
						expr_ptr=get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
					}
					
					if (EX(opline)->opcode==ZEND_INIT_ARRAY) {
						array_init(array_ptr);
						if (!expr_ptr) {
							NEXT_OPCODE();
						}
					}
					if (!EX(opline)->extended_value && EG(free_op1)) { /* temporary variable */
						zval *new_expr;

						ALLOC_ZVAL(new_expr);
						*new_expr = *expr_ptr;
						expr_ptr = new_expr;
						INIT_PZVAL(expr_ptr);
					} else {
						if (EX(opline)->extended_value) {
							SEPARATE_ZVAL_TO_MAKE_IS_REF(expr_ptr_ptr);
							expr_ptr = *expr_ptr_ptr;
							expr_ptr->refcount++;
						} else if (PZVAL_IS_REF(expr_ptr)) {
							zval *new_expr;

							ALLOC_ZVAL(new_expr);
							*new_expr = *expr_ptr;
							expr_ptr = new_expr;
							zendi_zval_copy_ctor(*expr_ptr);
							INIT_PZVAL(expr_ptr);
						} else {
							expr_ptr->refcount++;
						}
					}
					if (offset) {
						switch(offset->type) {
							case IS_DOUBLE:
								zend_hash_index_update(array_ptr->value.ht, (long) offset->value.dval, &expr_ptr, sizeof(zval *), NULL);
								break;
							case IS_LONG:
								zend_hash_index_update(array_ptr->value.ht, offset->value.lval, &expr_ptr, sizeof(zval *), NULL);
								break;
							case IS_STRING:
								zend_hash_update(array_ptr->value.ht, offset->value.str.val, offset->value.str.len+1, &expr_ptr, sizeof(zval *), NULL);
								break;
							case IS_NULL:
								zend_hash_update(array_ptr->value.ht, "", sizeof(""), &expr_ptr, sizeof(zval *), NULL);
								break;
							default:
								zval_ptr_dtor(&expr_ptr);
								/* do nothing */
								break;
						}
						FREE_OP(EX(Ts), &EX(opline)->op2, EG(free_op2));
					} else {
						zend_hash_next_index_insert(array_ptr->value.ht, &expr_ptr, sizeof(zval *), NULL);
					}
				}
				NEXT_OPCODE();
			case ZEND_CAST: {
					zval *expr = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
					zval *result = &EX(Ts)[EX(opline)->result.u.var].tmp_var;

					*result = *expr;
					if (!EG(free_op1)) {
						zendi_zval_copy_ctor(*result);
					}					
					switch (EX(opline)->extended_value) {
						case IS_NULL:
							convert_to_null(result);
							break;
						case IS_BOOL:
							convert_to_boolean(result);
							break;
						case IS_LONG:
							convert_to_long(result);
							break;
						case IS_DOUBLE:
							convert_to_double(result);
							break;
						case IS_STRING:
							convert_to_string(result);
							break;
						case IS_ARRAY:
							convert_to_array(result);
							break;
						case IS_OBJECT:
							convert_to_object(result);
							break;
					}
				}
				NEXT_OPCODE();
			case ZEND_INCLUDE_OR_EVAL: {
					zend_op_array *new_op_array=NULL;
					zval **original_return_value = EG(return_value_ptr_ptr);
					int return_value_used;
					zval *inc_filename = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
					zval tmp_inc_filename;
					zend_bool failure_retval=0;

					if (inc_filename->type!=IS_STRING) {
						tmp_inc_filename = *inc_filename;
						zval_copy_ctor(&tmp_inc_filename);
						convert_to_string(&tmp_inc_filename);
						inc_filename = &tmp_inc_filename;
					}
					
					return_value_used = RETURN_VALUE_USED(EX(opline));

					switch (EX(opline)->op2.u.constant.value.lval) {
						case ZEND_INCLUDE_ONCE:
						case ZEND_REQUIRE_ONCE: {
								char *opened_path=NULL;
								int dummy = 1;
								zend_file_handle file_handle;

								file_handle.handle.fp = zend_fopen(inc_filename->value.str.val, &opened_path);
								file_handle.type = ZEND_HANDLE_FP;
								file_handle.filename = inc_filename->value.str.val;
								file_handle.opened_path = opened_path;
								file_handle.free_filename = 0;

								if (file_handle.handle.fp) {
									if( !opened_path ) {
										opened_path = file_handle.opened_path = estrndup(inc_filename->value.str.val, inc_filename->value.str.len);
									}	
								
									if (zend_hash_add(&EG(included_files), opened_path, strlen(opened_path)+1, (void *)&dummy, sizeof(int), NULL)==SUCCESS) {
										new_op_array = zend_compile_file(&file_handle, (EX(opline)->op2.u.constant.value.lval==ZEND_INCLUDE_ONCE?ZEND_INCLUDE:ZEND_REQUIRE) TSRMLS_CC);
										zend_destroy_file_handle(&file_handle TSRMLS_CC);
										opened_path = NULL; /* zend_destroy_file_handle() already frees it */
									} else {
										fclose(file_handle.handle.fp);
										failure_retval=1;
									}
								} else {
									if (EX(opline)->op2.u.constant.value.lval==ZEND_INCLUDE_ONCE) {
										zend_message_dispatcher(ZMSG_FAILED_INCLUDE_FOPEN, file_handle.filename);
									} else {
										zend_message_dispatcher(ZMSG_FAILED_REQUIRE_FOPEN, file_handle.filename);
									}
								}
								if (opened_path) {
									efree(opened_path);
								}
								break;
							}
							break;
						case ZEND_INCLUDE:
						case ZEND_REQUIRE:
							new_op_array = compile_filename(EX(opline)->op2.u.constant.value.lval, inc_filename TSRMLS_CC);
							break;
						case ZEND_EVAL: {
								char *eval_desc = zend_make_compiled_string_description("eval()'d code" TSRMLS_CC);

								new_op_array = compile_string(inc_filename, eval_desc TSRMLS_CC);
								efree(eval_desc);
							}
							break;
						EMPTY_SWITCH_DEFAULT_CASE()
					}
					if (inc_filename==&tmp_inc_filename) {
						zval_dtor(&tmp_inc_filename);
					}
					FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));
					EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr = &EX(Ts)[EX(opline)->result.u.var].var.ptr;
					if (new_op_array) {
						zval *saved_object;
						zend_function *saved_function;


						EG(return_value_ptr_ptr) = EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr;
						EG(active_op_array) = new_op_array;
						EX(Ts)[EX(opline)->result.u.var].var.ptr = NULL;

						saved_object = EX(object);
						saved_function = EX(function_state).function;

						EX(function_state).function = (zend_function *) new_op_array;
						EX(object) = NULL;
						
						zend_execute(new_op_array TSRMLS_CC);
						
						EX(function_state).function = saved_function;
						EX(object) = saved_object;
						
						if (!return_value_used) {
							if (EX(Ts)[EX(opline)->result.u.var].var.ptr) {
								zval_ptr_dtor(&EX(Ts)[EX(opline)->result.u.var].var.ptr);
							} 
						} else { /* return value is used */
							if (!EX(Ts)[EX(opline)->result.u.var].var.ptr) { /* there was no return statement */
								ALLOC_ZVAL(EX(Ts)[EX(opline)->result.u.var].var.ptr);
								INIT_PZVAL(EX(Ts)[EX(opline)->result.u.var].var.ptr);
								EX(Ts)[EX(opline)->result.u.var].var.ptr->value.lval = 1;
								EX(Ts)[EX(opline)->result.u.var].var.ptr->type = IS_BOOL;
							}
						}

						EG(opline_ptr) = &EX(opline);
						EG(active_op_array) = op_array;
						EG(function_state_ptr) = &EX(function_state);
						destroy_op_array(new_op_array);
						efree(new_op_array);
					} else {
						if (return_value_used) {
							ALLOC_ZVAL(EX(Ts)[EX(opline)->result.u.var].var.ptr);
							INIT_ZVAL(*EX(Ts)[EX(opline)->result.u.var].var.ptr);
							EX(Ts)[EX(opline)->result.u.var].var.ptr->value.lval = failure_retval;
							EX(Ts)[EX(opline)->result.u.var].var.ptr->type = IS_BOOL;
						}
					}
					EG(return_value_ptr_ptr) = original_return_value;
				}
				NEXT_OPCODE();
			case ZEND_UNSET_VAR:
				{
					zval tmp, *variable = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);

					if (variable->type != IS_STRING) {
						tmp = *variable;
						zval_copy_ctor(&tmp);
						convert_to_string(&tmp);
						variable = &tmp;
					}

					zend_hash_del(EG(active_symbol_table), variable->value.str.val, variable->value.str.len+1);

					if (variable == &tmp) {
						zval_dtor(&tmp);
					}
					FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));
				}
				NEXT_OPCODE();
			case ZEND_UNSET_DIM_OBJ: {
					zval **container = get_zval_ptr_ptr(&EX(opline)->op1, EX(Ts), BP_VAR_R);
					zval *offset = get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R);
					
					if (container) {
						HashTable *ht;

						if((*container)->type == IS_ARRAY) {
							ht = (*container)->value.ht;
						} else {
							ht = NULL;
							if((*container)->type == IS_OBJECT) {
								Z_OBJ_HT_P(*container)->unset_property(*container, offset TSRMLS_CC);
							}
						}
						if (ht)	{
							switch (offset->type) {
								case IS_DOUBLE:
								case IS_RESOURCE:
								case IS_BOOL: 
								case IS_LONG:
									{
										long index;

										if (offset->type == IS_DOUBLE) {
											index = (long) offset->value.lval;
										} else {
											index = offset->value.lval;
										}
					
										zend_hash_index_del(ht, index);
										break;
									}
								case IS_STRING:
									zend_hash_del(ht, offset->value.str.val, offset->value.str.len+1);
									break;
								case IS_NULL:
									zend_hash_del(ht, "", sizeof(""));
									break;
								default: 
									zend_error(E_WARNING, "Illegal offset type in unset");
									break;
							}
						}
					} else {
						/* overloaded element */
					}
					FREE_OP(EX(Ts), &EX(opline)->op2, EG(free_op2));
				}
				NEXT_OPCODE();
			case ZEND_FE_RESET: {
					zval *array_ptr, **array_ptr_ptr;
					HashTable *fe_ht;
					
					if (EX(opline)->extended_value) {
						array_ptr_ptr = get_zval_ptr_ptr(&EX(opline)->op1, EX(Ts), BP_VAR_R);
						if (array_ptr_ptr == NULL) {
							MAKE_STD_ZVAL(array_ptr);
						} else {
							SEPARATE_ZVAL_IF_NOT_REF(array_ptr_ptr);
							array_ptr = *array_ptr_ptr;
							array_ptr->refcount++;
						}
					} else {
						array_ptr = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
						if (EG(free_op1)) { /* IS_TMP_VAR */
							zval *tmp;

							ALLOC_ZVAL(tmp);
							*tmp = *array_ptr;
							INIT_PZVAL(tmp);
							array_ptr = tmp;
						} else {
							array_ptr->refcount++;
						}
					}
					PZVAL_LOCK(array_ptr);
					EX(Ts)[EX(opline)->result.u.var].var.ptr = array_ptr;
					EX(Ts)[EX(opline)->result.u.var].var.ptr_ptr = &EX(Ts)[EX(opline)->result.u.var].var.ptr;	

					if ((fe_ht = HASH_OF(array_ptr)) != NULL) {
						/* probably redundant */
						zend_hash_internal_pointer_reset(fe_ht);
					} else {
						/* JMP to the end of foreach - TBD */
					}
				}
				NEXT_OPCODE();
			case ZEND_FE_FETCH: {
					zval *array = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
					zval *result = &EX(Ts)[EX(opline)->result.u.var].tmp_var;
					zval **value, *key;
					char *str_key;
					ulong int_key;
					HashTable *fe_ht;

					PZVAL_LOCK(array);

					fe_ht = HASH_OF(array);
					if (!fe_ht) {
						zend_error(E_WARNING, "Invalid argument supplied for foreach()");
						EX(opline) = op_array->opcodes+EX(opline)->op2.u.opline_num;
						continue;
					} else if (zend_hash_get_current_data(fe_ht, (void **) &value)==FAILURE) {
						EX(opline) = op_array->opcodes+EX(opline)->op2.u.opline_num;
						continue;
					}
					array_init(result);


					(*value)->refcount++;
					zend_hash_index_update(result->value.ht, 0, value, sizeof(zval *), NULL);

					ALLOC_ZVAL(key);
					INIT_PZVAL(key);
					switch (zend_hash_get_current_key(fe_ht, &str_key, &int_key, 1)) {
						case HASH_KEY_IS_STRING:
							key->value.str.val = str_key;
							key->value.str.len = strlen(str_key);
							key->type = IS_STRING;
							break;
						case HASH_KEY_IS_LONG:
							key->value.lval = int_key;
							key->type = IS_LONG;
							break;
						EMPTY_SWITCH_DEFAULT_CASE()
					}
					zend_hash_index_update(result->value.ht, 1, &key, sizeof(zval *), NULL);
					zend_hash_move_forward(fe_ht);
				}
				NEXT_OPCODE();
			case ZEND_JMP_NO_CTOR: {
					zval *object_zval;
					zend_function *constructor;

					if (EX(opline)->op1.op_type == IS_VAR) {
						PZVAL_LOCK(*EX(Ts)[EX(opline)->op1.u.var].var.ptr_ptr);
					}
					
					object_zval = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
					constructor = Z_OBJ_HT_P(object_zval)->get_constructor(object_zval TSRMLS_CC);

					EX(fbc_constructor) = NULL;
					if (constructor == NULL) {
						EX(opline) = op_array->opcodes + EX(opline)->op2.u.opline_num;
						continue;
					} else {
						EX(fbc_constructor) = constructor;
					}
					
				}
				NEXT_OPCODE();
			case ZEND_ISSET_ISEMPTY_VAR:
				{
					zval tmp, *variable = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
					zval **value;
					zend_bool isset = 1;
					HashTable *target_symbol_table;
		
					target_symbol_table = zend_get_target_symbol_table(EX(opline), EX(Ts), BP_VAR_IS TSRMLS_CC);

					if (variable->type != IS_STRING) {
						tmp = *variable;
						zval_copy_ctor(&tmp);
						convert_to_string(&tmp);
						variable = &tmp;
					}
					
					if (zend_hash_find(target_symbol_table, variable->value.str.val, variable->value.str.len+1, (void **) &value) == FAILURE) {
						isset = 0;
					}
					
					EX(Ts)[EX(opline)->result.u.var].tmp_var.type = IS_BOOL;

					switch (EX(opline)->extended_value) {
						case ZEND_ISSET:
							if (isset && Z_TYPE_PP(value) == IS_NULL) {
								EX(Ts)[EX(opline)->result.u.var].tmp_var.value.lval = 0;
							} else {
								EX(Ts)[EX(opline)->result.u.var].tmp_var.value.lval = isset;
							}
							break;
						case ZEND_ISEMPTY:
							if (!isset || !zend_is_true(*value)) {
								EX(Ts)[EX(opline)->result.u.var].tmp_var.value.lval = 1;
							} else {
								EX(Ts)[EX(opline)->result.u.var].tmp_var.value.lval = 0;
							}
							break;
					}

					if (variable == &tmp) {
						zval_dtor(&tmp);
					}
					FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));
				}
				NEXT_OPCODE();
		case ZEND_ISSET_ISEMPTY_DIM_OBJ:
				{
					zval **container = get_zval_ptr_ptr(&EX(opline)->op1, EX(Ts), BP_VAR_R);
					zval *offset = get_zval_ptr(&EX(opline)->op2, EX(Ts), &EG(free_op2), BP_VAR_R);
					zval **value = NULL;
					int result = 0;

					if (container) {

						if((*container)->type == IS_ARRAY) {
							HashTable *ht;
							int isset = 0;

							ht = (*container)->value.ht;

							switch (offset->type) {
								case IS_DOUBLE:
								case IS_RESOURCE:
								case IS_BOOL: 
								case IS_LONG:
									{
										long index;

										if (offset->type == IS_DOUBLE) {
											index = (long) offset->value.lval;
										} else {
											index = offset->value.lval;
										}
										if (zend_hash_index_find(ht, index, (void **) &value) == SUCCESS) {
											isset = 1;
										}
										break;
									}
								case IS_STRING:
									if (zend_hash_find(ht, offset->value.str.val, offset->value.str.len+1, (void **) &value) == SUCCESS) {
										isset = 1;
									}
									break;
								case IS_NULL:
									if (zend_hash_find(ht, "", sizeof(""), (void **) &value) == SUCCESS) {
										isset = 1;
									}
									break;
								default: 
									zend_error(E_WARNING, "Illegal offset type in unset");
									
									break;
							}
							
							switch (EX(opline)->extended_value) {
								case ZEND_ISSET:
									if (isset && Z_TYPE_PP(value) == IS_NULL) {
										result = 0;
									} else {
										result = isset;
									}
									break;
								case ZEND_ISEMPTY:
									if (!isset || !zend_is_true(*value)) {
										result = 0;
									} else {
										result = 1;
									}
									break;
							}
						} else {
							if((*container)->type == IS_OBJECT) {
								result = Z_OBJ_HT_P(*container)->has_property(*container, offset, (EX(opline)->extended_value == ZEND_ISEMPTY) TSRMLS_CC);
							}
						}
					} else {
						/* string offsets */
					}
					
					EX(Ts)[EX(opline)->result.u.var].tmp_var.type = IS_BOOL;

					switch (EX(opline)->extended_value) {
						case ZEND_ISSET:
							EX(Ts)[EX(opline)->result.u.var].tmp_var.value.lval = result;
							break;
						case ZEND_ISEMPTY:
							EX(Ts)[EX(opline)->result.u.var].tmp_var.value.lval = !result;
							break;
					}

					FREE_OP(EX(Ts), &EX(opline)->op2, EG(free_op2));
				}
				NEXT_OPCODE();
				break;
			case ZEND_EXIT:
				if (EX(opline)->op1.op_type != IS_UNUSED) {
					zval *ptr;

					ptr = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
					if (Z_TYPE_P(ptr) == IS_LONG) {
						EG(exit_status) = Z_LVAL_P(ptr);
					} else {
						zend_print_variable(ptr);
					}
					FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));
				}
				zend_bailout();
				NEXT_OPCODE();
			case ZEND_BEGIN_SILENCE:
				EX(Ts)[EX(opline)->result.u.var].tmp_var.value.lval = EG(error_reporting);
				EX(Ts)[EX(opline)->result.u.var].tmp_var.type = IS_LONG;  /* shouldn't be necessary */
				EG(error_reporting) = 0;
				NEXT_OPCODE();
			case ZEND_END_SILENCE:
				EG(error_reporting) = EX(Ts)[EX(opline)->op1.u.var].tmp_var.value.lval;
				NEXT_OPCODE();
			case ZEND_QM_ASSIGN: {
					zval *value = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);

					EX(Ts)[EX(opline)->result.u.var].tmp_var = *value;
					if (!EG(free_op1)) {
						zval_copy_ctor(&EX(Ts)[EX(opline)->result.u.var].tmp_var);
					}
				}
				NEXT_OPCODE();
			case ZEND_EXT_STMT: 
				if (!EG(no_extensions)) {
					zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_statement_handler, op_array TSRMLS_CC);
				}
				NEXT_OPCODE();
			case ZEND_EXT_FCALL_BEGIN:
				if (!EG(no_extensions)) {
					zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_fcall_begin_handler, op_array TSRMLS_CC);
				}
				NEXT_OPCODE();
			case ZEND_EXT_FCALL_END:
				if (!EG(no_extensions)) {
					zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_fcall_end_handler, op_array TSRMLS_CC);
				}
				NEXT_OPCODE();
			case ZEND_DECLARE_FUNCTION_OR_CLASS:
				do_bind_function_or_class(EX(opline), EG(function_table), EG(class_table), 0);
				NEXT_OPCODE();
			case ZEND_TICKS:
				if (++EG(ticks_count)>=EX(opline)->op1.u.constant.value.lval) {
					EG(ticks_count)=0;
					if (zend_ticks_function) {
						zend_ticks_function(EX(opline)->op1.u.constant.value.lval);
					}
				}
				NEXT_OPCODE();
			case ZEND_IS_CLASS: {
					zval *expr = get_zval_ptr(&EX(opline)->op1, EX(Ts), &EG(free_op1), BP_VAR_R);
					is_class_function(&EX(Ts)[EX(opline)->result.u.var].tmp_var, expr,
										 EX(Ts)[EX(opline)->op2.u.var].EA.class_entry TSRMLS_CC);
					FREE_OP(EX(Ts), &EX(opline)->op1, EG(free_op1));
				}
				NEXT_OPCODE();
			case ZEND_EXT_NOP:
			case ZEND_NOP:
				NEXT_OPCODE();
			EMPTY_SWITCH_DEFAULT_CASE()
		}
	}
	zend_error(E_ERROR, "Arrived at end of main loop which shouldn't happen");
}
