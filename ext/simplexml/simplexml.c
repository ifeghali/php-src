/*
  +----------------------------------------------------------------------+
  | PHP Version 4                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2003 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.02 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available at through the world-wide-web at                           |
  | http://www.php.net/license/2_02.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sterling Hughes <sterling@php.net>                           |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_simplexml.h"

zend_class_entry *sxe_class_entry;

#define SKIP_TEXT(__p) \
	if ((__p)->type == XML_TEXT_NODE) { \
		goto next_iter; \
	}

static php_sxe_object *php_sxe_object_new(TSRMLS_D);
static zend_object_value php_sxe_register_object(php_sxe_object * TSRMLS_DC);

/* {{{ php_sxe_fetch_object()
 */
static inline php_sxe_object *
php_sxe_fetch_object(zval *object TSRMLS_DC)
{
	return (php_sxe_object *) zend_object_store_get_object(object TSRMLS_CC);
}
/* }}} */

/* {{{ _node_as_zval()
 */
static void
_node_as_zval(php_sxe_object *sxe, xmlNodePtr node, zval *value TSRMLS_DC)
{
	php_sxe_object *subnode;

	subnode = php_sxe_object_new(TSRMLS_C);
	subnode->document = sxe->document;
	subnode->document->refcount++;
	subnode->nsmap = sxe->nsmap;
	subnode->node = node;

	value->type = IS_OBJECT;
	value->value.obj = php_sxe_register_object(subnode TSRMLS_CC);
}
/* }}} */

#define APPEND_PREV_ELEMENT(__c, __v) \
	if ((__c) == 1) { \
		array_init(return_value); \
		add_next_index_zval(return_value, __v); \
	}

#define APPEND_CUR_ELEMENT(__c, __v) \
	if (++(__c) > 1) { \
		add_next_index_zval(return_value, __v); \
	}

#define GET_NODE(__s, __n) (__n) = (__s)->node ? (__s)->node : xmlDocGetRootElement((xmlDocPtr) (__s)->document->ptr)


/* {{{ match_ns()
 */
static inline int 
match_ns(php_sxe_object *sxe, xmlNodePtr node, xmlChar *name)
{
	if (!xmlStrcmp(node->ns->prefix, name) || !xmlStrcmp((xmlChar *) xmlHashLookup(sxe->nsmap, node->ns->href), name)) {
		return 1;
	}

	return 0;
}
/* }}} */
	
/* {{{ sxe_property_read()
 */
static zval *
sxe_property_read(zval *object, zval *member TSRMLS_DC)
{
	zval           *return_value;
	zval           *value;
	php_sxe_object *sxe;
	char           *name;
	char           *contents;
	char           *mapname = NULL;
	xmlNodePtr      node;
	xmlAttrPtr      attr;
	int             counter = 0;

	MAKE_STD_ZVAL(return_value);
	ZVAL_NULL(return_value);

	name = Z_STRVAL_P(member);

	sxe = php_sxe_fetch_object(object TSRMLS_CC);

	GET_NODE(sxe, node);

	attr = node->properties;
	while (attr) {
		if (!xmlStrcmp(attr->name, name)) {
			APPEND_PREV_ELEMENT(counter, value);
			
			MAKE_STD_ZVAL(value);
			contents = xmlNodeListGetString((xmlDocPtr) sxe->document->ptr, attr->children, 1);
			ZVAL_STRING(value, contents, 0);

			APPEND_CUR_ELEMENT(counter, value);
		}
		attr = attr->next;
	}

	if (!sxe->node) {
		sxe->node = node;
	}
	node = node->children;

	while (node) {
		SKIP_TEXT(node);
		
		if (node->ns) {
			if (node->parent->ns) {
				if (!xmlStrcmp(node->ns->href, node->parent->ns->href)) {
					goto next_iter;
				}
			}
			
			if (match_ns(sxe, node, name)) {
				MAKE_STD_ZVAL(value);
				_node_as_zval(sxe, node->parent, value);
				APPEND_CUR_ELEMENT(counter, value);
				goto next_iter;
			}
		}

		if (!xmlStrcmp(node->name, name)) {
			APPEND_PREV_ELEMENT(counter, value);

			MAKE_STD_ZVAL(value);
			_node_as_zval(sxe, node, value TSRMLS_CC);

			APPEND_CUR_ELEMENT(counter, value);
		}

next_iter:
		node = node->next;
	}

	/* Only one value found */
	if (counter == 1) {
		return_value = value;
	}

	return return_value;
}
/* }}} */

/* {{{ change_node_zval()
 */
static void
change_node_zval(xmlNodePtr node, zval *value)
{
	switch (Z_TYPE_P(value)) {
		case IS_LONG:
		case IS_BOOL:
		case IS_DOUBLE:
		case IS_NULL:
			convert_to_string(value);
		case IS_STRING:
			xmlNodeSetContentLen(node, Z_STRVAL_P(value), Z_STRLEN_P(value));
			break;
		default:
			php_error(E_WARNING, "It is not yet possible to assign complex types to attributes");
			break;
	}
}
/* }}} */


/* {{{ sxe_property_write()
 */
static void
sxe_property_write(zval *object, zval *member, zval *value TSRMLS_DC)
{
	php_sxe_object *sxe;
	char           *name;
	xmlNodePtr      node;
	xmlNodePtr      newnode;
	xmlAttrPtr      attr;
	int             counter = 0;
	int             is_attr = 0;

	name = Z_STRVAL_P(member);
	sxe = php_sxe_fetch_object(object TSRMLS_CC);

	GET_NODE(sxe, node);

	attr = node->properties;
	while (attr) {
		if (!xmlStrcmp(attr->name, name)) {
			is_attr = 1;
			++counter;
			break;
		}

		attr = attr->next;
	}

	node = node->children;
	while (node) {
		SKIP_TEXT(node);
		if (!xmlStrcmp(node->name, name)) {
			newnode = node;
			++counter;
		}

next_iter:
		node = node->next;
	}

	if (counter == 1) {
		if (is_attr) {
			change_node_zval(attr->children, value);
		} else {
			change_node_zval(newnode->children, value);
		}
	} else if (counter > 1) {
		php_error(E_WARNING, "Cannot assign to an array of nodes (duplicate subnodes or attr detected)\n");
	}
		
}
/* }}} */

/* {{{ sxe_property_get_ptr()
 */
static zval **
sxe_property_get_ptr(zval *object, zval *member TSRMLS_DC)
{
	zval **property_ptr;
	zval  *property;

	property_ptr = emalloc(sizeof(zval **));

	property = sxe_property_read(object, member TSRMLS_CC);
	zval_add_ref(&property);

	*property_ptr = property;
	
	return property_ptr;
}
/* }}} */

/* {{{ sxe_property_exists()
 */
static int
sxe_property_exists(zval *object, zval *member, int check_empty TSRMLS_DC)
{
	php_sxe_object *sxe;
	char           *name;
	xmlNodePtr      node;
	xmlAttrPtr      attr;
	
	sxe = php_sxe_fetch_object(object TSRMLS_CC);
	name = Z_STRVAL_P(member);

	GET_NODE(sxe, node);

	attr = node->properties;
	while (attr) {
		if (!xmlStrcmp(attr->name, name)) {
			return 1;
		}

		attr = attr->next;
	}

	node = node->children;
	while (node) {
		SKIP_TEXT(node);

		if (!xmlStrcmp(node->name, name)) {
			return 1;
		}

next_iter:
		node = node->next;
	}

	return 0;
}
/* }}} */

/* {{{ sxe_property_delete()
 */
static void
sxe_property_delete(zval *object, zval *member TSRMLS_DC)
{
	php_sxe_object *sxe;
	xmlNodePtr      node;
	xmlNodePtr      nnext;
	xmlAttrPtr      attr;
	xmlAttrPtr      anext;

	sxe = php_sxe_fetch_object(object TSRMLS_CC);

	GET_NODE(sxe, node);

	attr = node->properties;
	while (attr) {
		anext = attr->next;
		if (!xmlStrcmp(attr->name, Z_STRVAL_P(member))) {
			// free
		}
		attr = anext;
	}
	
	node = node->children;
	while (node) {
		nnext = node->next;

		SKIP_TEXT(node);
		
		if (!xmlStrcmp(node->name, Z_STRVAL_P(member))) {
			xmlUnlinkNode(node);
			xmlFreeNode(node);
		}

next_iter:
		node = nnext;
	}
}
/* }}} */

/* {{{ _get_base_node_value()
 */
static void
_get_base_node_value(xmlNodePtr node, zval **value TSRMLS_CC)
{
	php_sxe_object *subnode;
	char           *contents;

	MAKE_STD_ZVAL(*value);
	
	if (node->children && node->children->type == XML_TEXT_NODE && !xmlIsBlankNode(node->children)) {
		contents = xmlNodeListGetString(node->doc, node->children, 1);
		if (contents) {
			ZVAL_STRING(*value, contents, 1);
		}
	} else {
		subnode = php_sxe_object_new(TSRMLS_C);
		subnode->document = emalloc(sizeof(simplexml_ref_obj));
		subnode->document->refcount = 2;
		subnode->document->ptr = node->doc;
		subnode->node = node;

		(*value)->type = IS_OBJECT;
		(*value)->value.obj = php_sxe_register_object(subnode TSRMLS_CC);
		zval_add_ref(value);
	}
}
/* }}} */

/* {{{ sxe_properties_get()
 */
static HashTable *
sxe_properties_get(zval *object TSRMLS_DC)
{
	zval           **data_ptr;
	zval            *value;
	zval            *newptr;
	HashTable       *rv;
	php_sxe_object  *sxe;
	char            *name;
	xmlNodePtr       node;
	ulong            h;
	int              namelen;

	ALLOC_HASHTABLE(rv);
	zend_hash_init(rv, 0, NULL, ZVAL_PTR_DTOR, 0);
	
	sxe = php_sxe_fetch_object(object TSRMLS_CC);

	GET_NODE(sxe, node);
	node = node->children;

	while (node) {
		SKIP_TEXT(node);

		_get_base_node_value(node, &value TSRMLS_CC);
		
		name = (char *) node->name;
		namelen = xmlStrlen(node->name) + 1;

		h = zend_hash_func(name, namelen);
		if (zend_hash_quick_find(rv, name, namelen, h, (void **) &data_ptr) == SUCCESS) {
			if (Z_TYPE_PP(data_ptr) == IS_ARRAY) {
				zend_hash_next_index_insert(Z_ARRVAL_PP(data_ptr), &value, sizeof(zval *), NULL);
			} else {
				MAKE_STD_ZVAL(newptr);
				array_init(newptr);

				zend_hash_next_index_insert(Z_ARRVAL_P(newptr), data_ptr, sizeof(zval *), NULL);
				zend_hash_next_index_insert(Z_ARRVAL_P(newptr), &value, sizeof(zval *), NULL);

				zend_hash_quick_update(rv, name, namelen, h, &newptr, sizeof(zval *), NULL);
			}
		} else {
			zend_hash_quick_update(rv, name, namelen, h, &value, sizeof(zval *), NULL);
		}

next_iter:
		node = node->next;
	}

	return rv;
}
/* }}} */

/* {{{ sxe_objects_compare()
 */
static int
sxe_objects_compare(zval *object1, zval *object2 TSRMLS_DC)
{
	php_sxe_object *sxe1;
	php_sxe_object *sxe2;

	sxe1 = php_sxe_fetch_object(object1 TSRMLS_CC);
	sxe2 = php_sxe_fetch_object(object2 TSRMLS_CC);

	if (sxe1->node == NULL) {
		if (sxe2->node) {
			return 1;
		} else if (sxe1->document->ptr == sxe2->document->ptr) {
			return 0;
		}
	} else {
		return !(sxe1->node == sxe2->node);
	}
}
/* }}} */

/* {{{ sxe_constructor_get()
 */
static union _zend_function *
sxe_constructor_get(zval *object TSRMLS_DC)
{
	return NULL;
}
/* }}} */

/* {{{ sxe_method_get()
 */
static union _zend_function *
sxe_method_get(zval *object, char *name, int len TSRMLS_DC)
{
	zend_internal_function *f;

	f = emalloc(sizeof(zend_internal_function));
	f->type = ZEND_OVERLOADED_FUNCTION;
	f->arg_types = NULL;
	f->scope = sxe_class_entry;
	f->fn_flags = 0;
	f->function_name = estrndup(name, len);

	return (union _zend_function *) f;
}
/* }}} */

/* {{{ simplexml_ce_xpath_search()
 */
static void 
simplexml_ce_xpath_search(INTERNAL_FUNCTION_PARAMETERS)
{
	php_sxe_object    *sxe;
	zval              *value;
	char              *query;
	int                query_len;
	int                i;
	xmlNodeSetPtr      result;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &query, &query_len) == FAILURE) {
		return;
	}

	sxe = php_sxe_fetch_object(getThis() TSRMLS_CC);
	if (!sxe->xpath) {
		sxe->xpath = xmlXPathNewContext((xmlDocPtr) sxe->document->ptr);
	}
	if (!sxe->node) {
		sxe->node = xmlDocGetRootElement((xmlDocPtr) sxe->document->ptr);
	}
	sxe->xpath->node = sxe->node;

	result = xmlXPathEval(query, sxe->xpath)->nodesetval;
	if (!result) {
		RETURN_FALSE;
	}

	array_init(return_value);

	for (i = 0; i < result->nodeNr; ++i) {
		MAKE_STD_ZVAL(value);
		/**
		 * Detect the case where the last selector is text(), simplexml
		 * always accesses the text() child by default, therefore we assign
		 * to the parent node.
		 */
		if (result->nodeTab[i]->type == XML_TEXT_NODE) {
			_node_as_zval(sxe, result->nodeTab[i]->parent, value);
		} else {
			_node_as_zval(sxe, result->nodeTab[i], value);
		}
		add_next_index_zval(return_value, value);
	}
	
}
/* }}} */

#define SCHEMA_FILE 0
#define SCHEMA_BLOB 1
#define SCHEMA_OBJECT 2

/* {{{ simplexml_ce_schema_validate_file()
 */
static void
simplexml_ce_schema_validate(INTERNAL_FUNCTION_PARAMETERS, int type)
{
	php_sxe_object         *sxe;
	zval                   *source;
	xmlSchemaParserCtxtPtr  parser;
	xmlSchemaPtr            sptr;
	xmlSchemaValidCtxtPtr   vptr;
	int                     is_valid;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &source) == FAILURE) {
		return;
	}

	sxe = php_sxe_fetch_object(getThis() TSRMLS_CC);

	switch (type) {
		case SCHEMA_FILE:
			convert_to_string_ex(&source);
			parser = xmlSchemaNewParserCtxt(Z_STRVAL_P(source));
			sptr = xmlSchemaParse(parser);
			xmlSchemaFreeParserCtxt(parser);
			break;
		case SCHEMA_BLOB:
			convert_to_string_ex(&source);
			parser = xmlSchemaNewMemParserCtxt(Z_STRVAL_P(source), Z_STRLEN_P(source));
			sptr = xmlSchemaParse(parser);
			xmlSchemaFreeParserCtxt(parser);
			break;
	}

	vptr = xmlSchemaNewValidCtxt(sptr);
	is_valid = xmlSchemaValidateDoc(vptr, (xmlDocPtr) sxe->document->ptr);
	xmlSchemaFree(sptr);
	xmlSchemaFreeValidCtxt(vptr);

	if (is_valid) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ simplexml_ce_register_ns()
 */
static void
simplexml_ce_register_ns(INTERNAL_FUNCTION_PARAMETERS)
{
	php_sxe_object *sxe;
	char *nsname;
	char *nsvalue;
	int   nsname_len;
	int   nsvalue_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &nsname, &nsname_len, &nsvalue, &nsvalue_len) == FAILURE) {
		return;
	}

	sxe = php_sxe_fetch_object(getThis() TSRMLS_CC);

	xmlHashAddEntry(sxe->nsmap, nsvalue, nsname);
}
/* }}} */


/* {{{ sxe_call_method()
 */
static int
sxe_call_method(char *method, INTERNAL_FUNCTION_PARAMETERS)
{
	if (!strcmp(method, "xsearch")) {
		simplexml_ce_xpath_search(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	} else if (!strcmp(method, "validate_schema_file")) {
		simplexml_ce_schema_validate(INTERNAL_FUNCTION_PARAM_PASSTHRU, SCHEMA_FILE);	
	} else if (!strcmp(method, "validate_schema_buffer")) {
		simplexml_ce_schema_validate(INTERNAL_FUNCTION_PARAM_PASSTHRU, SCHEMA_BLOB);
	} else if (!strcmp(method, "register_ns")) {
		simplexml_ce_register_ns(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	} else {
		return 0;
	}

	return 1;
}
/* }}} */

/* {{{ sxe_class_entry_get()
 */
static zend_class_entry *
sxe_class_entry_get(zval *object TSRMLS_DC)
{
	return sxe_class_entry;
}
/* }}} */

/* {{{ sxe_class_name_get()
 */
static int
sxe_class_name_get(zval *object, char **class_name, zend_uint *class_name_len, int parent TSRMLS_DC)
{
	*class_name = estrdup("simplexml_element");
	*class_name_len = sizeof("simplexml_element");

	return 0;
}
/* }}} */

/* {{{ cast_object()
 */
static inline void
cast_object(zval *object, int type, char *contents TSRMLS_DC)
{
	if (contents) {
		int len = strlen(contents);
		ZVAL_STRINGL(object, contents, len, 1);
	} else {
		ZVAL_NULL(object);
	}

	switch (type) {
		case IS_STRING:
			return;
		case IS_BOOL:
			convert_to_boolean(object);
			break;
		case IS_LONG:
			convert_to_long(object);
			break;
		case IS_DOUBLE:
			convert_to_double(object);
			break;
	}
}
/* }}} */

/* {{{ sxe_object_cast()
 */
static void
sxe_object_cast(zval *readobj, zval *writeobj, int type, int should_free TSRMLS_DC)
{
	php_sxe_object *sxe;
	char           *contents;

    sxe = php_sxe_fetch_object(readobj TSRMLS_CC);
	
	if (should_free) {
		zval_dtor(writeobj);
	}

	if (sxe->node) {
		contents = xmlNodeListGetString((xmlDocPtr) sxe->document->ptr, sxe->node->children, 1);
		if (!xmlIsBlankNode(sxe->node->children) && contents) {
			cast_object(writeobj, type, NULL TSRMLS_CC);
		}
	}

	cast_object(writeobj, type, contents TSRMLS_CC);
}
/* }}} */

/* {{{ sxe_object_set()
 */
static void
sxe_object_set(zval **property, zval *value TSRMLS_DC)
{
	/* XXX: TODO
	 * This call is not yet implemented in the engine
	 * so leave it blank for now.
	 */
}
/* }}} */

/* {{{ sxe_object_get()
 */
static zval *
sxe_object_get(zval *property TSRMLS_DC)
{
	/* XXX: TODO
	 * This call is not yet implemented in the engine
	 * so leave it blank for now.
	 */
	return NULL;
}


static zend_object_handlers sxe_object_handlers[] = {
	ZEND_OBJECTS_STORE_HANDLERS,
	sxe_property_read,
	sxe_property_write,
	sxe_property_get_ptr,
	sxe_property_get_ptr,
	sxe_object_get,
	sxe_object_set,
	sxe_property_exists,
	sxe_property_delete,
	sxe_properties_get,
	sxe_method_get,
	sxe_call_method,
	sxe_constructor_get,
	sxe_class_entry_get,
	sxe_class_name_get,
	sxe_objects_compare,
	sxe_object_cast
};

/* {{{ sxe_object_clone()
 */
static void
sxe_object_clone(void *object, void **clone_ptr TSRMLS_DC)
{
	php_sxe_object *sxe = (php_sxe_object *) object;
	php_sxe_object *clone;

	clone = php_sxe_object_new(TSRMLS_C);

	clone->document = emalloc(sizeof(simplexml_ref_obj));
	clone->document->refcount = 1;
	clone->document->ptr = xmlCopyDoc((xmlDocPtr) sxe->document->ptr, 1);

	*clone_ptr = (void *) clone;
}
/* }}} */

/* {{{ _free_ns_entry()
 */
static void
_free_ns_entry(void *p, xmlChar *data)
{
	xmlFree(p);
}
/* }}} */

/* {{{ sxe_object_dtor()
 */
static void
sxe_object_dtor(void *object, zend_object_handle handle TSRMLS_DC)
{
	php_sxe_object *sxe;

	sxe = (php_sxe_object *) object;

	if (--sxe->document->refcount > 0) {
		return;
	}
	
	xmlFreeDoc(sxe->document->ptr);
	FREE_HASHTABLE(sxe->zo.properties);

	if (sxe->xpath) {
		xmlXPathFreeContext(sxe->xpath);
	}

	xmlHashFree(sxe->nsmap, _free_ns_entry);
	zend_objects_destroy_object(object, handle TSRMLS_CC);
}
/* }}} */

/* {{{ php_sxe_object_new() 
 */
static php_sxe_object *
php_sxe_object_new(TSRMLS_D)
{
	php_sxe_object *intern;

	intern = ecalloc(1, sizeof(php_sxe_object));
	intern->zo.ce = sxe_class_entry;
	intern->zo.in_get = 0;
	intern->zo.in_set = 0;

	ALLOC_HASHTABLE(intern->zo.properties);
	zend_hash_init(intern->zo.properties, 0, NULL, ZVAL_PTR_DTOR, 0);

	return intern;
}
/* }}} */

/* {{{ php_sxe_register_object
 */
static zend_object_value
php_sxe_register_object(php_sxe_object *intern TSRMLS_DC)
{
	zend_object_value rv;

	rv.handle = zend_objects_store_put(intern, sxe_object_dtor, sxe_object_clone TSRMLS_CC);
	rv.handlers = (zend_object_handlers *) &sxe_object_handlers;

	return rv;
}
/* }}} */

/* {{{ sxe_object_new()
 */
static zend_object_value
sxe_object_new(zend_class_entry *ce TSRMLS_DC)
{
	php_sxe_object    *intern;

	intern = php_sxe_object_new(TSRMLS_C);
	return php_sxe_register_object(intern TSRMLS_CC);
}
/* }}} */

/* {{{ proto simplemxml_element simplexml_load_file(string filename)
   Load a filename and return a simplexml_element object to allow for processing */
PHP_FUNCTION(simplexml_load_file)
{
	php_sxe_object *sxe;
	char           *filename;
	int             filename_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
		return;
	}

	sxe = php_sxe_object_new(TSRMLS_C);
	sxe->document = emalloc(sizeof(simplexml_ref_obj));
	sxe->document->refcount = 1;
	sxe->document->ptr = (void *) xmlParseFile(filename);
	if (sxe->document->ptr == NULL) {
		efree(sxe->document);
		RETURN_FALSE;
	}
	sxe->nsmap = xmlHashCreate(10);
	

	return_value->type = IS_OBJECT;
	return_value->value.obj = php_sxe_register_object(sxe TSRMLS_CC);
}
/* }}} */

/* {{{ proto simplemxml_element simplexml_load_string(string data)
   Load a string and return a simplexml_element object to allow for processing */
PHP_FUNCTION(simplexml_load_string)
{
	php_sxe_object *sxe;
	char           *data;
	int             data_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) == FAILURE) {
		return;
	}

	sxe = php_sxe_object_new(TSRMLS_C);
	sxe->document = emalloc(sizeof(simplexml_ref_obj));
	sxe->document->refcount = 1;
	sxe->document->ptr = xmlParseMemory(data, data_len);
	if (sxe->document->ptr == NULL) {
		efree(sxe->document);
		RETURN_FALSE;
	}
	sxe->nsmap = xmlHashCreate(10);
		
	return_value->type = IS_OBJECT;
	return_value->value.obj = php_sxe_register_object(sxe TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool simplexml_save_document_file(string filename, simplexml_element node)
   Save a XML document to a file from a SimpleXML node */
PHP_FUNCTION(simplexml_save_document_file)
{
	php_sxe_object *sxe;
	zval           *element;
	char           *filename;
	int             filename_len;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &filename, &filename_len, &element) == FAILURE) {
		return;
	}

	sxe = php_sxe_fetch_object(element TSRMLS_CC);

	xmlSaveFile(filename, (xmlDocPtr) sxe->document->ptr);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool simplexml_save_document_string(string &var, simplexml_element node)
   Save a document to a variable from a SimpleXML node */
PHP_FUNCTION(simplexml_save_document_string)
{
	php_sxe_object *sxe;
	zval           *data;
	zval           *element;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &data, &element) == FAILURE) {
		return;
	}

	sxe = php_sxe_fetch_object(element TSRMLS_CC);
	xmlDocDumpMemory((xmlDocPtr) sxe->document->ptr, (xmlChar **) &Z_STRVAL_P(data), &Z_STRLEN_P(data));
	Z_TYPE_P(data) = IS_STRING;
	zval_add_ref(&data);

	RETURN_TRUE;
}
/* }}} */

/* this is lame, first_arg_force_ref (and others) doesn't 
   work through dll linkage on windows.  no other extension 
   outside basic_functions uses first_arg_force_ref.  */
unsigned char fix_first_arg_force_ref[] = { 1, BYREF_FORCE };

function_entry simplexml_functions[] = {
	PHP_FE(simplexml_load_file, NULL)
	PHP_FE(simplexml_load_string, NULL)
	PHP_FE(simplexml_save_document_file, NULL)
	PHP_FE(simplexml_save_document_string, fix_first_arg_force_ref)
	{NULL, NULL, NULL}
};

zend_module_entry simplexml_module_entry = {
	STANDARD_MODULE_HEADER,
	"simplexml",
	simplexml_functions,
	PHP_MINIT(simplexml),
	PHP_MSHUTDOWN(simplexml),
	PHP_RINIT(simplexml),	
	PHP_RSHUTDOWN(simplexml),
	PHP_MINFO(simplexml),
	"0.1",
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SIMPLEXML
ZEND_GET_MODULE(simplexml)
#endif
	
/* {{{ PHP_MINIT_FUNCTION(simplexml)
 */
PHP_MINIT_FUNCTION(simplexml)
{
	zend_class_entry sxe;

	INIT_CLASS_ENTRY(sxe, "simplexml_element", NULL);
	sxe.create_object = sxe_object_new;
	sxe_class_entry = zend_register_internal_class(&sxe TSRMLS_CC);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION(simplexml)
 */
PHP_MSHUTDOWN_FUNCTION(simplexml)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION(simplexml)
 */
PHP_RINIT_FUNCTION(simplexml)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION(simplexml)
 */
PHP_RSHUTDOWN_FUNCTION(simplexml)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION(simplexml)
 */
PHP_MINFO_FUNCTION(simplexml)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Simplexml support", "enabled");
	php_info_print_table_row(2, "Revision", "$Revision$");
	php_info_print_table_end();

}
/* }}} */

/**
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: t
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
