/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Jani Lehtim�ki <jkl@njet.net>                               |
   |          Thies C. Arntzen <thies@thieso.net>                         |
   |          Sascha Schumann <sascha@schumann.cx>                        |
   +----------------------------------------------------------------------+
*/

/* $Id$ */


/* {{{ includes 
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "php.h"
#include "php_string.h"
#include "php_var.h"
#include "basic_functions.h"
#include "php_incomplete_class.h"

#define COMMON ((*struc)->is_ref?"&":"")

/* }}} */
/* {{{ php_var_dump */

static int php_array_element_dump(zval **zv, int num_args, va_list args, zend_hash_key *hash_key)
{
	int level;

	level = va_arg(args, int);

	if (hash_key->nKeyLength==0) { /* numeric key */
		php_printf("%*c[%ld]=>\n", level + 1, ' ', hash_key->h);
	} else { /* string key */
		php_printf("%*c[\"%s\"]=>\n", level + 1, ' ', hash_key->arKey);
	}
	php_var_dump(zv, level + 2);
	return 0;
}

void php_var_dump(zval **struc, int level)
{
	HashTable *myht;

	if (level>1) {
		php_printf("%*c", level-1, ' ');
	}

	switch ((*struc)->type) {
		case IS_BOOL:
			php_printf("%sbool(%s)\n", COMMON, Z_LVAL_PP(struc)?"true":"false");
			break;
		case IS_NULL:
			php_printf("%sNULL\n", COMMON);
			break;
		case IS_LONG:
			php_printf("%sint(%ld)\n", COMMON, Z_LVAL_PP(struc));
			break;
		case IS_DOUBLE: {
				TSRMLS_FETCH();

				php_printf("%sfloat(%.*G)\n", COMMON, (int) EG(precision), Z_DVAL_PP(struc));
			}
			break;
		case IS_STRING:
			php_printf("%sstring(%d) \"", COMMON, Z_STRLEN_PP(struc));
			PHPWRITE(Z_STRVAL_PP(struc), Z_STRLEN_PP(struc));
			PUTS("\"\n");
			break;
		case IS_ARRAY:
			myht = HASH_OF(*struc);
			php_printf("%sarray(%d) {\n", COMMON, zend_hash_num_elements(myht));
			goto head_done;
		case IS_OBJECT:
			myht = HASH_OF(*struc);
			php_printf("%sobject(%s)(%d) {\n", COMMON, (*struc)->value.obj.ce->name, zend_hash_num_elements(myht));
head_done:
			zend_hash_apply_with_arguments(myht, (apply_func_args_t) php_array_element_dump, 1, level);
			if (level>1) {
				php_printf("%*c", level-1, ' ');
			}
			PUTS("}\n");
			break;
		case IS_RESOURCE: {
			char *type_name;
			type_name = zend_rsrc_list_get_rsrc_type(Z_LVAL_PP(struc));
			php_printf("%sresource(%ld) of type (%s)\n", COMMON, Z_LVAL_PP(struc), type_name ? type_name : "Unknown");
			break;
		}
		default:
			php_printf("%sUNKNOWN:0\n",COMMON);
			break;
	}
}

/* }}} */


/* {{{ proto void var_dump(mixed var)
   Dumps a string representation of variable to output */
PHP_FUNCTION(var_dump)
{
	zval ***args;
	int argc;
	int	i;
	
	argc = ZEND_NUM_ARGS();
	
	args = (zval ***)emalloc(argc * sizeof(zval **));
	if (ZEND_NUM_ARGS() == 0 || zend_get_parameters_array_ex(argc, args) == FAILURE) {
		efree(args);
		WRONG_PARAM_COUNT;
	}
	
	for (i=0; i<argc; i++)
		php_var_dump(args[i], 1);
	
	efree(args);
}
/* }}} */


/* {{{ php_var_dump */


#define STR_CAT(P,S,I) {\
	zval *__p = (P);\
	ulong __i = Z_STRLEN_P(__p);\
	Z_STRLEN_P(__p) += (I);\
	if (Z_STRVAL_P(__p)) {\
		Z_STRVAL_P(__p) = (char *)erealloc(Z_STRVAL_P(__p), Z_STRLEN_P(__p) + 1);\
	} else {\
		Z_STRVAL_P(__p) = emalloc(Z_STRLEN_P(__p) + 1);\
		*Z_STRVAL_P(__p) = 0;\
	}\
	strcat(Z_STRVAL_P(__p) + __i, (S));\
}

/* }}} */
/* {{{ php_var_serialize */

static inline int php_add_var_hash(HashTable *var_hash, zval *var, void *var_old)
{
	ulong var_no;
	char id[sizeof(void *)*2+3];

	snprintf(id,sizeof(id)-1, "%p", var);
	id[sizeof(id)-1]='\0';

	if(var_old && zend_hash_find(var_hash, id, strlen(id), var_old) == SUCCESS) {
		if(!var->is_ref) {
			/* we still need to bump up the counter, since non-refs will
			   be counted separately by unserializer */
			var_no = -1;
			zend_hash_next_index_insert(var_hash, &var_no, sizeof(var_no), NULL);
		}
		return FAILURE;
	}
	
	var_no = zend_hash_num_elements(var_hash)+1; /* +1 because otherwise hash will think we are trying to store NULL pointer */
	zend_hash_add(var_hash, id, strlen(id), &var_no, sizeof(var_no), NULL);
	return SUCCESS;
}

PHPAPI void php_var_serialize(zval *buf, zval **struc, HashTable *var_hash)
{
	char s[256];
	ulong slen;
	int i;
	ulong *var_already;
	HashTable *myht;
	TSRMLS_FETCH();

	if(var_hash != NULL && php_add_var_hash(var_hash,*struc,(void *)&var_already) == FAILURE && (*struc)->is_ref) {
		slen = sprintf(s,"R:%ld;",*var_already);
		STR_CAT(buf, s, slen);
		return;
	}

	switch ((*struc)->type) {
		case IS_BOOL:
			slen = sprintf(s, "b:%ld;", Z_LVAL_PP(struc));
			STR_CAT(buf, s, slen);
			return;

		case IS_NULL:
			STR_CAT(buf, "N;", 2);
			return;

		case IS_LONG:
			slen = sprintf(s, "i:%ld;", Z_LVAL_PP(struc));
			STR_CAT(buf, s, slen);
			return;

		case IS_DOUBLE: {
				slen = sprintf(s, "d:%.*G;",(int) EG(precision), Z_DVAL_PP(struc));
				STR_CAT(buf, s, slen);
			}
			return;

		case IS_STRING:{
				char *p;
				
				i = buf->value.str.len;
				slen = sprintf(s, "s:%d:\"", Z_STRLEN_PP(struc));
				STR_CAT(buf, s, slen + Z_STRLEN_PP(struc) + 2);
				p = Z_STRVAL_P(buf) + i + slen;
				if (Z_STRLEN_PP(struc) > 0) {
					memcpy(p, Z_STRVAL_PP(struc), Z_STRLEN_PP(struc));
					p += Z_STRLEN_PP(struc);
				}
				*p++ = '\"';
				*p++ = ';';
				*p = 0;
			}
			return;

		case IS_OBJECT: {
				zval *retval_ptr = NULL;
				zval *fname;
				int res;
				PHP_CLASS_ATTRIBUTES;

				MAKE_STD_ZVAL(fname);
				ZVAL_STRING(fname,"__sleep",1);

				res =  call_user_function_ex(CG(function_table), struc, fname, &retval_ptr, 0, 0, 1, NULL);

				if (res == SUCCESS) {
					if (retval_ptr && HASH_OF(retval_ptr)) {
						int count = zend_hash_num_elements(HASH_OF(retval_ptr));
						
						PHP_SET_CLASS_ATTRIBUTES(*struc);
						slen = sprintf(s, "O:%d:\"%s\":%d:{",name_len,class_name, count);
						PHP_CLEANUP_CLASS_ATTRIBUTES();
						
						STR_CAT(buf, s, slen);
						if (count > 0) {
							char *key;
							zval **d,**name;
							ulong index;
							HashPosition pos;
							
							zend_hash_internal_pointer_reset_ex(HASH_OF(retval_ptr),&pos);
							for (;; zend_hash_move_forward_ex(HASH_OF(retval_ptr),&pos)) {
								if ((i = zend_hash_get_current_key_ex(HASH_OF(retval_ptr), &key, NULL, &index, 0, &pos)) == HASH_KEY_NON_EXISTANT) {
									break;
								}

								zend_hash_get_current_data_ex(HASH_OF(retval_ptr), (void **) (&name), &pos);

								if ((*name)->type != IS_STRING) {
									php_error(E_NOTICE, "__sleep should return an array only containing the names of instance-variables to serialize.");
									continue;
								}

								if (zend_hash_find((*struc)->value.obj.properties,Z_STRVAL_PP(name),Z_STRLEN_PP(name)+1,(void*)&d) == SUCCESS) {
									php_var_serialize(buf, name, NULL);
									php_var_serialize(buf,d,var_hash);	
								}
							}
						}
						STR_CAT(buf, "}", 1);
					}
				} else {
					zval_dtor(fname);
					FREE_ZVAL(fname);

					if (retval_ptr) {
						zval_ptr_dtor(&retval_ptr);
					}
					goto std_array;
				}

				zval_dtor(fname);
				FREE_ZVAL(fname);

				if (retval_ptr) {
					zval_ptr_dtor(&retval_ptr);
				}
				return;	
			}

		case IS_ARRAY:
		  std_array:
			myht = HASH_OF(*struc);
			i = zend_hash_num_elements(myht);
			if ((*struc)->type == IS_ARRAY) {
				slen = sprintf(s, "a:%d:{", i);
			} else {
				PHP_CLASS_ATTRIBUTES;

				PHP_SET_CLASS_ATTRIBUTES(*struc);
				slen = sprintf(s, "O:%d:\"%s\":%d:{",name_len,class_name,i);
				PHP_CLEANUP_CLASS_ATTRIBUTES();
			}
			STR_CAT(buf, s, slen);
			if (i > 0) {
				char *key;
				zval **data,*d;
				ulong index;
				HashPosition pos;
				
				zend_hash_internal_pointer_reset_ex(myht, &pos);
				for (;; zend_hash_move_forward_ex(myht, &pos)) {
					if ((i = zend_hash_get_current_key_ex(myht, &key, NULL, &index, 0, &pos)) == HASH_KEY_NON_EXISTANT) {
						break;
					}
					if (zend_hash_get_current_data_ex(myht, (void **) (&data), &pos) != SUCCESS || !data || data == struc) {
						continue;
					}

					switch (i) {
					  case HASH_KEY_IS_LONG:
							MAKE_STD_ZVAL(d);
							ZVAL_LONG(d,index);
							php_var_serialize(buf, &d, NULL);
							FREE_ZVAL(d);
							break;
						case HASH_KEY_IS_STRING:
							MAKE_STD_ZVAL(d);	
							ZVAL_STRING(d,key,0);
							php_var_serialize(buf, &d, NULL);
							FREE_ZVAL(d);
							break;
					}
					php_var_serialize(buf, data, var_hash);
				}
			}
			STR_CAT(buf, "}", 1);
			return;

		default:
			STR_CAT(buf, "i:0;", 4);
			return;
	} 
}

/* }}} */
/* {{{ php_var_dump */

PHPAPI int php_var_unserialize(zval **rval, const char **p, const char *max, HashTable *var_hash)
{
	const char *q;
	char *str;
	int i;
	char cur;
	ulong id;
	HashTable *myht;
	zval **rval_ref;
	TSRMLS_FETCH();

	if (var_hash && **p != 'R') {  /* references aren't counted by serializer! */
		zend_hash_next_index_insert(var_hash, rval, sizeof(*rval), NULL);
	}

	switch (cur = **p) {
	    case 'R':
			if (*((*p) + 1) != ':') {
				return 0;
			}
			q = *p;
			while (**p && **p != ';') {
				(*p)++;
			}
			if (**p != ';') {
				return 0;
			}
			(*p)++;
			id = atol(q + 2)-1; /* count starts with 1 */
			if (!var_hash) {
				return 0;
			}
			if (zend_hash_index_find(var_hash, id, (void *)&rval_ref) != SUCCESS) {
				return 0;
			}
			zval_ptr_dtor(rval);
			*rval = *rval_ref;
			(*rval)->refcount++;
			(*rval)->is_ref = 1;
			return 1;

		case 'N':
			if (*((*p) + 1) != ';') {
				return 0;
			}
			(*p)++;
			INIT_PZVAL(*rval);
			ZVAL_NULL(*rval);
			(*p)++;
			return 1;

		case 'b': /* bool */
		case 'i':
			if (*((*p) + 1) != ':') {
				return 0;
			}
			q = *p;
			while (**p && **p != ';') {
				(*p)++;
			}
			if (**p != ';') {
				return 0;
			}
			(*p)++;
			INIT_PZVAL(*rval);
			if (cur == 'b') {
				ZVAL_BOOL(*rval,atol(q + 2));
			} else {
				ZVAL_LONG(*rval,atol(q + 2));
			}
			return 1;

		case 'd':
			if (*((*p) + 1) != ':') {
				return 0;
			}
			q = *p;
			while (**p && **p != ';') {
				(*p)++;
			}
			if (**p != ';') {
				return 0;
			}
			(*p)++;
			INIT_PZVAL(*rval);
			ZVAL_DOUBLE(*rval,atof(q + 2));
			return 1;

		case 's':
			if (*((*p) + 1) != ':') {
				return 0;
			}
			(*p) += 2;
			q = *p;
			while (**p && **p != ':') {
				(*p)++;
			}
			if (**p != ':') {
				return 0;
			}
			i = atoi(q);
			if (i < 0 || (*p + 3 + i) > max || *((*p) + 1) != '\"' ||
				*((*p) + 2 + i) != '\"' || *((*p) + 3 + i) != ';') {
				return 0;
			}
			(*p) += 2;

			if (i == 0) {
			  	str = empty_string;
			} else  {
			  	str = estrndup(*p,i);
			}
			(*p) += i + 2;
			INIT_PZVAL(*rval);
			ZVAL_STRINGL(*rval,str,i,0);
			return 1;

		case 'a':
		case 'o':
		case 'O': {
			zend_bool incomplete_class = 0;
			char *class_name = NULL;
			size_t name_len = 0;
			int pi;
			
			INIT_PZVAL(*rval);

			if (cur == 'a') {
				(*rval)->type = IS_ARRAY;
				ALLOC_HASHTABLE((*rval)->value.ht);
				myht = (*rval)->value.ht;
			} else {
				zend_class_entry *ce;

				if (cur == 'O') { /* php4 serialized - we get the class-name */
					if (*((*p) + 1) != ':') {
						return 0;
					}
					(*p) += 2;
					q = *p;
					while (**p && **p != ':') {
						(*p)++;
					}
					if (**p != ':') {
						return 0;
					}
					name_len = i = atoi(q);
					if (i < 0 || (*p + 3 + i) > max || *((*p) + 1) != '\"' ||
						*((*p) + 2 + i) != '\"' || *((*p) + 3 + i) != ':') {
						return 0;
					}
					(*p) += 2;
					class_name = emalloc(i + 1);
					for(pi=0;pi<i;pi++) {
						class_name[pi] = tolower((*p)[pi]);
					}
					class_name[i] = 0;
					(*p) += i;
					
					if (zend_hash_find(EG(class_table), class_name, i+1, (void **) &ce)==FAILURE) {
						incomplete_class = 1;
						ce = PHP_IC_ENTRY_READ;
					}
				} else { /* old php 3.0 data 'o' */
					ce = &zend_standard_class_def;
				}

				object_init_ex(*rval,ce);
				myht = (*rval)->value.obj.properties;

				if (incomplete_class)
					php_store_class_name(*rval, class_name, name_len);

				if (class_name)
					efree(class_name);
			}

			(*p) += 2;
			i = atoi(*p);

			if (cur == 'a') { /* object_init_ex will init the HashTable for objects! */
				zend_hash_init(myht, i + 1, NULL, ZVAL_PTR_DTOR, 0);
			}

			while (**p && **p != ':') {
				(*p)++;
			}
			if (**p != ':' || *((*p) + 1) != '{') {
				return 0;
			}
			for ((*p) += 2; **p && **p != '}' && i > 0; i--) {
				zval *key;
				zval *data;
				
				ALLOC_INIT_ZVAL(key);
				ALLOC_INIT_ZVAL(data);

				if (!php_var_unserialize(&key, p, max, NULL)) {
					zval_dtor(key);
					FREE_ZVAL(key);
					FREE_ZVAL(data);
					return 0;
				}
				if (!php_var_unserialize(&data, p, max, var_hash)) {
					zval_dtor(key);
					FREE_ZVAL(key);
					zval_dtor(data);
					FREE_ZVAL(data);
					return 0;
				}
				switch (key->type) {
					case IS_LONG:
						zend_hash_index_update(myht, Z_LVAL_P(key), &data, sizeof(data), NULL);
						break;
					case IS_STRING:
						zend_hash_update(myht, Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, &data, sizeof(data), NULL);
						break;
				}
				zval_dtor(key);
				FREE_ZVAL(key);
			}

			if ((*rval)->type == IS_OBJECT) {
				zval *retval_ptr = NULL;
				zval *fname;

				MAKE_STD_ZVAL(fname);
				ZVAL_STRING(fname,"__wakeup",1);

				call_user_function_ex(CG(function_table), rval, fname, &retval_ptr, 0, 0, 1, NULL);

				zval_dtor(fname);
				FREE_ZVAL(fname);
				if (retval_ptr)
					zval_ptr_dtor(&retval_ptr);
			}

			return *((*p)++) == '}';
		  }
	}

	return 0;
}

/* }}} */
/* {{{ proto string serialize(mixed variable)
   Returns a string representation of variable (which can later be unserialized) */
PHP_FUNCTION(serialize)
{
	zval **struc;
	php_serialize_data_t var_hash;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &struc) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	return_value->type = IS_STRING;
	return_value->value.str.val = NULL;
	return_value->value.str.len = 0;

	PHP_VAR_SERIALIZE_INIT(var_hash);
	php_var_serialize(return_value, struc, &var_hash);
	PHP_VAR_SERIALIZE_DESTROY(var_hash);
}

/* }}} */
/* {{{ proto mixed unserialize(string variable_representation)
   Takes a string representation of variable and recreates it */


PHP_FUNCTION(unserialize)
{
	zval **buf;
	php_serialize_data_t var_hash;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &buf) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if ((*buf)->type == IS_STRING) {
		const char *p = (*buf)->value.str.val;

		if (Z_STRLEN_PP(buf) == 0) {
			RETURN_FALSE;
		}

		PHP_VAR_UNSERIALIZE_INIT(var_hash);
		if (!php_var_unserialize(&return_value, &p, p + Z_STRLEN_PP(buf),  &var_hash)) {
			PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
			zval_dtor(return_value);
			php_error(E_NOTICE, "unserialize() failed at offset %d of %d bytes",p - Z_STRVAL_PP(buf),Z_STRLEN_PP(buf));
			RETURN_FALSE;
		}
		PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
	} else {
		php_error(E_NOTICE, "argument passed to unserialize() is not an string");
		RETURN_FALSE;
	}
}

/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 tw=78 fdm=marker
 * vim<600: sw=4 ts=4 tw=78
 */
