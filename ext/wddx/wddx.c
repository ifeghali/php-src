/*
   +----------------------------------------------------------------------+
   | PHP HTML Embedded Scripting Language Version 3.0                     |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-1999 PHP Development Team (See Credits file)      |
   +----------------------------------------------------------------------+
   | This program is free software; you can redistribute it and/or modify |
   | it under the terms of one of the following licenses:                 |
   |                                                                      |
   |  A) the GNU General Public License as published by the Free Software |
   |     Foundation; either version 2 of the License, or (at your option) |
   |     any later version.                                               |
   |                                                                      |
   |  B) the PHP License as published by the PHP Development Team and     |
   |     included in the distribution in the file: LICENSE                |
   |                                                                      |
   | This program is distributed in the hope that it will be useful,      |
   | but WITHOUT ANY WARRANTY; without even the implied warranty of       |
   | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        |
   | GNU General Public License for more details.                         |
   |                                                                      |
   | You should have received a copy of both licenses referred to here.   |
   | If you did not, or have any questions about PHP licensing, please    |
   | contact core@php.net.                                                |
   +----------------------------------------------------------------------+
   | Authors: Andrei Zmievski <andrei@ispi.net>                          |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "php.h"
#include "php_wddx.h"

#if HAVE_WDDX
#include "dlist.h"

#include "php_wddx_api.h"

#define WDDX_BUF_LEN			256
#define PHP_CLASS_NAME_VAR		"php_class_name"

#define EL_ARRAY				"array"
#define EL_BOOLEAN				"boolean"
#define EL_CHAR					"char"
#define EL_CHAR_CODE			"code"
#define EL_NULL					"null"
#define EL_NUMBER				"number"
#define	EL_PACKET				"wddxPacket"
#define	EL_STRING				"string"
#define EL_STRUCT				"struct"
#define EL_VALUE				"value"
#define EL_VAR					"var"
#define EL_VAR_NAME				"name"
#define EL_VERSION				"version"

#define php_wddx_deserialize(a,b) \
	php_wddx_deserialize_ex((a)->value.str.val, (a)->value.str.len, (b))

#define SET_STACK_VARNAME							\
		if (stack->varname) {						\
			ent.varname = estrdup(stack->varname);	\
			efree(stack->varname);					\
			stack->varname = NULL;					\
		} else										\
			ent.varname = NULL;						\
			
static int le_wddx;

struct _wddx_packet {
	DLIST *packet_head;
	int packet_length;
};

typedef struct {
	zval *data;
	enum {
		ST_ARRAY,
		ST_BOOLEAN,
		ST_NULL,
		ST_NUMBER,
		ST_STRING,
		ST_STRUCT
	} type;
	char *varname;
} st_entry;

typedef struct {
	int top, max;
	char *varname;
	void **elements;
} wddx_stack;


/* {{{ function prototypes */
static void php_wddx_process_data(void *user_data, const char *s, int len);
/* }}} */


/* {{{ module definition structures */

function_entry wddx_functions[] = {
	PHP_FE(wddx_serialize_value, NULL)
	PHP_FE(wddx_serialize_vars, NULL)
	PHP_FE(wddx_packet_start, NULL)
	PHP_FE(wddx_packet_end, NULL)
	PHP_FE(wddx_add_vars, NULL)
	PHP_FE(wddx_deserialize, NULL)
	{NULL, NULL, NULL}
};

zend_module_entry wddx_module_entry = {
	"WDDX", wddx_functions, php_minit_wddx, NULL, NULL, NULL, NULL, STANDARD_MODULE_PROPERTIES
};

/* }}} */

	
/* {{{ int wddx_stack_init(wddx_stack *stack) */
static int wddx_stack_init(wddx_stack *stack)
{
	stack->top = 0;
	stack->elements = (void **) emalloc(sizeof(void **) * STACK_BLOCK_SIZE);
	if (!stack->elements) {
		return FAILURE;
	} else {
		stack->max = STACK_BLOCK_SIZE;
		stack->varname = NULL;
		return SUCCESS;
	}
}
/* }}} */


/* {{{ int wddx_stack_push(wddx_stack *stack, void *element, int size) */
static int wddx_stack_push(wddx_stack *stack, void *element, int size)
{
	if (stack->top >= stack->max) {		/* we need to allocate more memory */
		stack->elements = (void **) erealloc(stack->elements,
				   (sizeof(void **) * (stack->max += STACK_BLOCK_SIZE)));
		if (!stack->elements) {
			return FAILURE;
		}
	}
	stack->elements[stack->top] = (void *) emalloc(size);
	memcpy(stack->elements[stack->top], element, size);
	return stack->top++;
}
/* }}} */


/* {{{ int wddx_stack_top(wddx_stack *stack, void **element) */
static int wddx_stack_top(wddx_stack *stack, void **element)
{
	if (stack->top > 0) {
		*element = stack->elements[stack->top - 1];
		return SUCCESS;
	} else {
		*element = NULL;
		return FAILURE;
	}
}
/* }}} */


/* {{{ int wddx_stack_is_empty(wddx_stack *stack) */
static int wddx_stack_is_empty(wddx_stack *stack)
{
	if (stack->top == 0) {
		return 1;
	} else {
		return 0;
	}
}
/* }}} */


/* {{{ int wddx_stack_destroy(wddx_stack *stack) */
static int wddx_stack_destroy(wddx_stack *stack)
{
	register int i;

	if (stack->elements) {
		for (i = 0; i < stack->top; i++) {
			if (((st_entry *)stack->elements[i])->data)
			{
				zval_dtor(((st_entry *)stack->elements[i])->data);
				efree(((st_entry *)stack->elements[i])->data);
			}
			efree(stack->elements[i]);
		}		
		efree(stack->elements);
	}
	return SUCCESS;
}
/* }}} */


/* {{{ _php_free_packet_chunk */
static void _php_free_packet_chunk(char **chunk_ptr)
{
	if ((*chunk_ptr))
		efree((*chunk_ptr));
}
/* }}} */


/* {{{ php_wddx_destructor */
void php_wddx_destructor(wddx_packet *packet)
{
	dlst_kill(packet->packet_head, (void (*)(void *))_php_free_packet_chunk);
	efree(packet);
}
/* }}} */


/* {{{ php_minit_wddx */
int php_minit_wddx(INIT_FUNC_ARGS)
{
	le_wddx = register_list_destructors(php_wddx_destructor, NULL);
	
	return SUCCESS;
}
/* }}} */


/* {{{ php_wddx_add_chunk */
void php_wddx_add_chunk(wddx_packet *packet, char *str)
{
	char **chunk_ptr;
	
	chunk_ptr = (char**)dlst_newnode(sizeof(char *));
	(*chunk_ptr) = estrdup(str);
	dlst_insertafter(packet->packet_head, chunk_ptr, PHP_DLST_TAIL(packet->packet_head));
	packet->packet_length += strlen(str);
}
/* }}} */


/* {{{ php_wddx_gather */
char* php_wddx_gather(wddx_packet *packet)
{
	char **chunk;
	char *buf;
	
	buf = (char *)emalloc(packet->packet_length+1);	
	buf[0] = '\0';
	for(chunk=dlst_first(packet->packet_head);
		chunk!=NULL;
		chunk = dlst_next(chunk)) {
		strcat(buf, *chunk);
	}
	
	return buf;
}
/* }}} */


/* {{{ void php_wddx_packet_start */
void php_wddx_packet_start(wddx_packet *packet, char *comment)
{
	char tmp_buf[WDDX_BUF_LEN];
	
	php_wddx_add_chunk(packet, WDDX_PACKET_S);
	if (comment)
	{
		sprintf(tmp_buf, WDDX_HEADER_COMMENT, comment);
		php_wddx_add_chunk(packet, tmp_buf);
	}
	else
		php_wddx_add_chunk(packet, WDDX_HEADER);
	php_wddx_add_chunk(packet, WDDX_DATA_S);
}
/* }}} */


/* {{{ int php_wddx_packet_end */
void php_wddx_packet_end(wddx_packet *packet)
{
	php_wddx_add_chunk(packet, WDDX_DATA_E);
	php_wddx_add_chunk(packet, WDDX_PACKET_E);	
}
/* }}} */


/* {{{ void php_wddx_serialize_var(wddx_packet *packet, zval *var) */
static void php_wddx_serialize_string(wddx_packet *packet, zval *var)
{
	char *buf,
		 *c,
		 control_buf[WDDX_BUF_LEN];
	int i;

	php_wddx_add_chunk(packet, WDDX_STRING_S);

	if (var->value.str.len > 0) {
		i = 0;
		buf = (char *)emalloc(var->value.str.len);
		for(c=var->value.str.val; *c!='\0'; c++)
		{
			if (iscntrl((int)*c))
			{
				if (*buf)
				{
					buf[i] = '\0';
					php_wddx_add_chunk(packet, buf);
					i = 0;
					buf[i] = '\0';
				}
				sprintf(control_buf, WDDX_CHAR, *c);
				php_wddx_add_chunk(packet, control_buf);
			}
			else
				buf[i++] = *c;
		}
		buf[i] = '\0';
		if (*buf)
			php_wddx_add_chunk(packet, buf);				
		efree(buf);
	}
	
	php_wddx_add_chunk(packet, WDDX_STRING_E);
}
/* }}} */


/* {{{ void php_wddx_serialize_number(wddx_packet *packet, zval *var) */
static void php_wddx_serialize_number(wddx_packet *packet, zval *var)
{
	char tmp_buf[WDDX_BUF_LEN];
	zval tmp;
	
	tmp = *var;
	zval_copy_ctor(&tmp);
	convert_to_string(&tmp);
	sprintf(tmp_buf, WDDX_NUMBER, tmp.value.str.val);
	zval_dtor(&tmp);

	php_wddx_add_chunk(packet, tmp_buf);	
}
/* }}} */


/* {{{ void php_wddx_serialize_boolean(wddx_packet *packet, zval *var) */
static void php_wddx_serialize_boolean(wddx_packet *packet, zval *var)
{
	char tmp_buf[WDDX_BUF_LEN];

	sprintf(tmp_buf, WDDX_BOOLEAN, var->value.lval ? "true" : "false");
	php_wddx_add_chunk(packet, tmp_buf);
}
/* }}} */

/* {{{ void php_wddx_serialize_unset(wddx_packet *packet, zval *var) */
static void php_wddx_serialize_unset(wddx_packet *packet)
{
	php_wddx_add_chunk(packet, WDDX_NULL);
}
/* }}} */

/* {{{ void php_wddx_serialize_hash(wddx_packet *packet, zval *var) */
static void php_wddx_serialize_hash(wddx_packet *packet, zval *var)
{
	zval **ent;
	char *key;
	int hash_type, ent_type;
	ulong idx;
	HashTable *target_hash;
	char tmp_buf[WDDX_BUF_LEN];

	target_hash = HASH_OF(var);
	
	zend_hash_internal_pointer_reset(target_hash);

	hash_type = zend_hash_get_current_key(target_hash, &key, &idx);	

	if (hash_type == HASH_KEY_IS_STRING) {
		php_wddx_add_chunk(packet, WDDX_STRUCT_S);
		efree(key);
	} else {
		sprintf(tmp_buf, WDDX_ARRAY_S, zend_hash_num_elements(target_hash));
		php_wddx_add_chunk(packet, tmp_buf);
	}

	/* If variable is an object, serialize its classname */
	if (var->type == IS_OBJECT)
	{
		sprintf(tmp_buf, WDDX_VAR_S, PHP_CLASS_NAME_VAR);
		php_wddx_add_chunk(packet, tmp_buf);
		php_wddx_add_chunk(packet, WDDX_STRING_S);
		php_wddx_add_chunk(packet, var->value.obj.ce->name);
		php_wddx_add_chunk(packet, WDDX_STRING_E);
		php_wddx_add_chunk(packet, WDDX_VAR_E);
	}
			
	while(zend_hash_get_current_data(target_hash, (void**)&ent) == SUCCESS) {
		if (hash_type == HASH_KEY_IS_STRING) {
			ent_type = zend_hash_get_current_key(target_hash, &key, &idx);

			if (ent_type == HASH_KEY_IS_STRING) {
				php_wddx_serialize_var(packet, *ent, key);
				efree(key);
			} else {
				sprintf(tmp_buf, "%ld", idx);
				php_wddx_serialize_var(packet, *ent, tmp_buf);
			}
		} else
			php_wddx_serialize_var(packet, *ent, NULL);

		zend_hash_move_forward(target_hash);
	}
	
	if (hash_type == HASH_KEY_IS_STRING)
		php_wddx_add_chunk(packet, WDDX_STRUCT_E);
	else
		php_wddx_add_chunk(packet, WDDX_ARRAY_E);
}
/* }}} */


/* {{{ void php_wddx_serialize_var(wddx_packet *packet, zval *var, char *name) */
void php_wddx_serialize_var(wddx_packet *packet, zval *var, char *name)
{
	char tmp_buf[WDDX_BUF_LEN];
	
	if (name) {
		sprintf(tmp_buf, WDDX_VAR_S, name);
		php_wddx_add_chunk(packet, tmp_buf);
	}
	
	switch(var->type) {
		case IS_STRING:
			php_wddx_serialize_string(packet, var);
			break;
			
		case IS_LONG:
		case IS_DOUBLE:
			php_wddx_serialize_number(packet, var);
			break;

		case IS_BOOL:
			php_wddx_serialize_boolean(packet, var);
			break;

		case IS_UNSET:
			php_wddx_serialize_unset(packet);
			break;
		
		case IS_ARRAY:
		case IS_OBJECT:
			php_wddx_serialize_hash(packet, var);
			break;
	}
	
	if (name) {
		php_wddx_add_chunk(packet, WDDX_VAR_E);
	}
}
/* }}} */


/* {{{ void php_wddx_add_var(wddx_packet *packet, zval *name_var) */
static void php_wddx_add_var(wddx_packet *packet, zval *name_var)
{
	zval **val;
	HashTable *target_hash;
	ELS_FETCH();
	
	if (name_var->type == IS_STRING)
	{
		if (zend_hash_find(EG(active_symbol_table), name_var->value.str.val,
							name_var->value.str.len+1, (void**)&val) != FAILURE) {
			php_wddx_serialize_var(packet, *val, name_var->value.str.val);
		}		
	}
	else if (name_var->type == IS_ARRAY || name_var->type == IS_OBJECT)
	{
		target_hash = HASH_OF(name_var);
		
		zend_hash_internal_pointer_reset(target_hash);

		while(zend_hash_get_current_data(target_hash, (void**)&val) == SUCCESS) {
			php_wddx_add_var(packet, *val);
				
			zend_hash_move_forward(target_hash);
		}
	}
}
/* }}} */


/* {{{ void php_wddx_push_element(void *user_data, const char *name, const char **atts) */
static void php_wddx_push_element(void *user_data, const char *name, const char **atts)
{
	st_entry ent;
	wddx_stack *stack = (wddx_stack *)user_data;
	
	if (!strcmp(name, EL_PACKET)) {
		int i;
		
		for (i=0; atts[i]; i++) {
			if (!strcmp(atts[i], EL_VERSION)) {
			}
		}
	} else if (!strcmp(name, EL_STRING)) {
		ent.type = ST_STRING;
		SET_STACK_VARNAME;
		
		ALLOC_ZVAL(ent.data);
		ent.data->value.str.val = NULL;
		ent.data->value.str.len = 0;
		INIT_PZVAL(ent.data);
		wddx_stack_push((wddx_stack *)stack, &ent, sizeof(st_entry));
	} else if (!strcmp(name, EL_CHAR)) {
		int i;
		char tmp_buf[2];
		
		for (i=0; atts[i]; i++) {
			if (!strcmp(atts[i], EL_CHAR_CODE) && atts[i+1]) {
				sprintf(tmp_buf, "%c", (char)strtol(atts[i+1], NULL, 16));
				php_wddx_process_data(user_data, tmp_buf, strlen(tmp_buf));
			}
		}
	} else if (!strcmp(name, EL_NUMBER)) {
		ent.type = ST_NUMBER;
		SET_STACK_VARNAME;
		
		ALLOC_ZVAL(ent.data);
		INIT_PZVAL(ent.data);
		wddx_stack_push((wddx_stack *)stack, &ent, sizeof(st_entry));
	} else if (!strcmp(name, EL_BOOLEAN)) {
		int i;

		for (i=0; atts[i]; i++) {
			if (!strcmp(atts[i], EL_VALUE) && atts[i+1]) {
				ent.type = ST_BOOLEAN;
				SET_STACK_VARNAME;

				ALLOC_ZVAL(ent.data);
				INIT_PZVAL(ent.data);
				wddx_stack_push((wddx_stack *)stack, &ent, sizeof(st_entry));
				php_wddx_process_data(user_data, atts[i+1], strlen(atts[i+1]));
			}
		}
	} else if (!strcmp(name, EL_NULL)) {
		ent.type = ST_NULL;
		SET_STACK_VARNAME;

		ALLOC_ZVAL(ent.data);
		INIT_PZVAL(ent.data);
		ent.data->type = IS_UNSET;
		
		wddx_stack_push((wddx_stack *)stack, &ent, sizeof(st_entry));
	} else if (!strcmp(name, EL_ARRAY)) {
		ent.type = ST_ARRAY;
		SET_STACK_VARNAME;
		
		ALLOC_ZVAL(ent.data);
		array_init(ent.data);
		INIT_PZVAL(ent.data);
		wddx_stack_push((wddx_stack *)stack, &ent, sizeof(st_entry));
	} else if (!strcmp(name, EL_STRUCT)) {
		ent.type = ST_STRUCT;
		SET_STACK_VARNAME;
		
		ALLOC_ZVAL(ent.data);
		array_init(ent.data);
		INIT_PZVAL(ent.data);
		wddx_stack_push((wddx_stack *)stack, &ent, sizeof(st_entry));
	} else if (!strcmp(name, EL_VAR)) {
		int i;
		
		for (i=0; atts[i]; i++) {
			if (!strcmp(atts[i], EL_VAR_NAME) && atts[i+1]) {
				stack->varname = estrdup(atts[i+1]);
			}
		}
	}
}
/* }}} */


/* {{{ void php_wddx_pop_element(void *user_data, const char *name) */
static void php_wddx_pop_element(void *user_data, const char *name)
{
	st_entry 			*ent1, *ent2;
	wddx_stack 			*stack = (wddx_stack *)user_data;
	HashTable 			*target_hash;
	zend_class_entry 	*ce;
	zval				*obj;
	zval				*tmp;
	ELS_FETCH();
	
	if (!strcmp(name, EL_STRING) || !strcmp(name, EL_NUMBER) ||
		!strcmp(name, EL_BOOLEAN) || !strcmp(name, EL_NULL) ||
	  	!strcmp(name, EL_ARRAY) || !strcmp(name, EL_STRUCT)) {
		if (stack->top > 1) {
			wddx_stack_top(stack, (void**)&ent1);
			stack->top--;
			wddx_stack_top(stack, (void**)&ent2);
			if (ent2->data->type == IS_ARRAY || ent2->data->type == IS_OBJECT) {
				target_hash = HASH_OF(ent2->data);
				
				if (ent1->varname) {
					if (!strcmp(ent1->varname, PHP_CLASS_NAME_VAR) &&
						ent1->data->type == IS_STRING &&
						ent1->data->value.str.len)
					{
						if (zend_hash_find(EG(class_table), ent1->data->value.str.val,
										   ent1->data->value.str.len+1, (void **) &ce)==FAILURE) {
							php_error(E_NOTICE, "Deserializing non-existant class: %s! No methods will be available!",
									  ent1->data->value.str.val);
							ce = &zend_standard_class_def;
						}

						/* Initialize target object */
						MAKE_STD_ZVAL(obj);
						INIT_PZVAL(obj);
						object_init_ex(obj, ce);
						
						/* Merge current hashtable with object's default properties */
						zend_hash_merge(obj->value.obj.properties,
										ent2->data->value.ht,
										(void (*)(void *)) zval_add_ref,
										(void *) &tmp, sizeof(zval *), 0);

						/* Clean up old array entry */
						zval_dtor(ent2->data);
						efree(ent2->data);
						
						/* Set stack entry to point to the newly created object */
						ent2->data = obj;
						
						/* Clean up class name var entry */
						zval_dtor(ent1->data);
						efree(ent1->data);
					}
					else
						zend_hash_update(target_hash,
										 ent1->varname, strlen(ent1->varname)+1,
										 &ent1->data, sizeof(zval *), NULL);
					efree(ent1->varname);
				} else	{
					zend_hash_next_index_insert(target_hash,
												&ent1->data,
												sizeof(zval *), NULL);
				}
			}
			efree(ent1);
		}
	}
	else if (!strcmp(name, EL_VAR) && stack->varname)
		efree(stack->varname);
}
/* }}} */


/* {{{ void php_wddx_process_data(void *user_data, const char *s, int len) */
static void php_wddx_process_data(void *user_data, const char *s, int len)
{
	st_entry *ent;
	wddx_stack *stack = (wddx_stack *)user_data;

	if (!wddx_stack_is_empty(stack)) {
		wddx_stack_top(stack, (void**)&ent);
		switch (ent->type) {
			case ST_STRING:
				ent->data->type = IS_STRING;
				if (ent->data->value.str.len == 0) {
					ent->data->value.str.val = estrndup(s, len);
					ent->data->value.str.len = len;
				} else {
					ent->data->value.str.val = erealloc(ent->data->value.str.val,
							ent->data->value.str.len + len + 1);
					strncpy(ent->data->value.str.val+ent->data->value.str.len, s, len);
					ent->data->value.str.len += len;
					ent->data->value.str.val[ent->data->value.str.len] = '\0';
				}
				break;

			case ST_NUMBER:
				ent->data->type = IS_STRING;
				ent->data->value.str.len = len;
				ent->data->value.str.val = estrndup(s, len);
				convert_scalar_to_number(ent->data);
				break;

			case ST_BOOLEAN:
				ent->data->type = IS_BOOL;
				if (!strcmp(s, "true"))
					ent->data->value.lval = 1;
				else if (!strcmp(s, "false"))
					ent->data->value.lval = 0;
				else {
					stack->top--;
					zval_dtor(ent->data);
					efree(ent->data);
					if (ent->varname)
						efree(ent->varname);
					efree(ent);
				}
				break;

			default:
				break;
		}
	}
}
/* }}} */


/* {{{ void php_wddx_deserialize_ex(char *value, int vallen, zval *return_value) */
void php_wddx_deserialize_ex(char *value, int vallen, zval *return_value)
{
	wddx_stack stack;
	XML_Parser parser;
	st_entry *ent;
	
	wddx_stack_init(&stack);
	parser = XML_ParserCreate(NULL);

	XML_SetUserData(parser, &stack);
	XML_SetElementHandler(parser, php_wddx_push_element, php_wddx_pop_element);
	XML_SetCharacterDataHandler(parser, php_wddx_process_data);
	
	XML_Parse(parser, value, vallen, 1);
	
	XML_ParserFree(parser);

	if (!wddx_stack_is_empty(&stack)) {
		wddx_stack_top(&stack, (void**)&ent);
		*return_value = *(ent->data);
		zval_copy_ctor(return_value);
	}
		
	wddx_stack_destroy(&stack);
}
/* }}} */


/* {{{ proto string wddx_serialize_value(mixed var [, string comment ])
   Creates a new packet and serializes the given value */
PHP_FUNCTION(wddx_serialize_value)
{
	int argc;
	zval **var,
		 **comment;
	wddx_packet *packet;
	char *buf;
	
	argc = ARG_COUNT(ht);
	if(argc < 1 || argc > 2 || zend_get_parameters_ex(argc, &var, &comment) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	packet = php_wddx_constructor();
	if (!packet) {
		RETURN_FALSE;
	}

	if (argc == 2)
	{
		convert_to_string_ex(comment);
		php_wddx_packet_start(packet, (*comment)->value.str.val);
	}
	else
		php_wddx_packet_start(packet, NULL);

	php_wddx_serialize_var(packet, (*var), NULL);
	php_wddx_packet_end(packet);
	buf = php_wddx_gather(packet);
	php_wddx_destructor(packet);
					
	RETURN_STRING(buf, 0);
}
/* }}} */


/* {{{ proto string wddx_serialize_vars(. . .)
   Creates a new packet and serializes given variables into a struct */
PHP_FUNCTION(wddx_serialize_vars)
{
	int argc, i;
	wddx_packet *packet;
	zval ***args;
	char *buf;
		
	argc = ARG_COUNT(ht);
	/* Allocate arguments array and get the arguments, checking for errors. */
	args = (zval ***)emalloc(argc * sizeof(zval **));
	if (zend_get_parameters_array_ex(argc, args) == FAILURE) {
		efree(args);
		WRONG_PARAM_COUNT;
	}
		
	packet = php_wddx_constructor();
	if (!packet) {
		RETURN_FALSE;
	}

	php_wddx_packet_start(packet, NULL);
	php_wddx_add_chunk(packet, WDDX_STRUCT_S);
	
	for (i=0; i<argc; i++) {
		if ((*args[i])->type != IS_ARRAY && (*args[i])->type != IS_OBJECT)
			convert_to_string_ex(args[i]);
		php_wddx_add_var(packet, *args[i]);
	}	
	
	php_wddx_add_chunk(packet, WDDX_STRUCT_E);
	php_wddx_packet_end(packet);
	buf = php_wddx_gather(packet);
	php_wddx_destructor(packet);

	efree(args);
						
	RETURN_STRING(buf, 0);	
}
/* }}} */

wddx_packet *php_wddx_constructor(void)
{
	wddx_packet *packet;

	packet = emalloc(sizeof(wddx_packet));
	if(!packet) return NULL;

	packet->packet_head = dlst_init();
	packet->packet_length = 0;

	return packet;
}

/* {{{ proto int wddx_packet_start([ string comment ])
   Starts a WDDX packet with optional comment and returns the packet id */
PHP_FUNCTION(wddx_packet_start)
{
	int argc;
	zval **comment;
	wddx_packet *packet;

	comment = NULL;
	argc = ARG_COUNT(ht);

	if (argc > 1 || (argc == 1 && zend_get_parameters_ex(1, &comment)==FAILURE)) {
		WRONG_PARAM_COUNT;
	}

	packet = php_wddx_constructor();
	if (!packet) {
		RETURN_FALSE;
	}
	
	if (argc == 1) {
		convert_to_string_ex(comment);
		php_wddx_packet_start(packet, (*comment)->value.str.val);
	}
	else
		php_wddx_packet_start(packet, NULL);
	
	php_wddx_add_chunk(packet, WDDX_STRUCT_S);

	ZEND_REGISTER_RESOURCE(return_value, packet, le_wddx);
}
/* }}} */


/* {{{ proto string wddx_packet_end(int packet_id)
   Ends specified WDDX packet and returns the string containing the packet */
PHP_FUNCTION(wddx_packet_end)
{
	zval **packet_id;
	char *buf;
	wddx_packet *packet = NULL;
	
	if (ARG_COUNT(ht)!=1 || zend_get_parameters_ex(1, &packet_id)==FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(packet, wddx_packet *, packet_id, -1, "WDDX packet ID", le_wddx);
			
	php_wddx_add_chunk(packet, WDDX_STRUCT_E);	
	
	php_wddx_packet_end(packet);

	buf = php_wddx_gather(packet);
	
	zend_list_delete((*packet_id)->value.lval);
	
	RETURN_STRING(buf, 0);
}
/* }}} */


/* {{{ proto int wddx_add_vars(int packet_id, . . .)
   Serializes given variables and adds them to packet given by packet_id */
PHP_FUNCTION(wddx_add_vars)
{
	int argc, i;
	zval ***args;
	zval **packet_id;
	wddx_packet *packet = NULL;
	
	argc = ARG_COUNT(ht);
	if (argc < 2) {
		WRONG_PARAM_COUNT;
	}
	
	/* Allocate arguments array and get the arguments, checking for errors. */
	args = (zval ***)emalloc(argc * sizeof(zval **));
	if (zend_get_parameters_array_ex(argc, args) == FAILURE) {
		efree(args);
		WRONG_PARAM_COUNT;
	}
	
	packet_id = args[0];

	packet = (wddx_packet *)zend_fetch_resource(packet_id, -1, "WDDX packet ID", NULL, 1, le_wddx);
	if (!packet)
	{
		efree(args);
		RETURN_FALSE;
	}
		
	for (i=1; i<argc; i++) {
		if ((*args[i])->type != IS_ARRAY && (*args[i])->type != IS_OBJECT)
			convert_to_string_ex(args[i]);
		php_wddx_add_var(packet, (*args[i]));
	}

	efree(args);	
	RETURN_TRUE;
}
/* }}} */


/* {{{  proto mixed wddx_deserialize(string packet) 
   Deserializes given packet and returns a PHP value */
PHP_FUNCTION(wddx_deserialize)
{
	zval **packet;
	
	if (ARG_COUNT(ht)!=1 || zend_get_parameters_ex(1, &packet) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(packet);
	if ((*packet)->value.str.len == 0)
		return;
		
	php_wddx_deserialize(*packet, return_value);
}
/* }}} */


#endif /* HAVE_LIBEXPAT */
