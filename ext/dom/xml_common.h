/*
  +----------------------------------------------------------------------+
  | PHP Version 4                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2003 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Christian Stocker <chregu@php.net>                          |
  |          Rob Richards <rrichards@php.net>                            |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_XML_COMMON_H
#define PHP_XML_COMMON_H

typedef struct _dom_ref_obj {
	void *ptr;
	int   refcount;
} dom_ref_obj;

typedef struct _node_ptr {
	xmlNodePtr node;
	int	refcount;
	void *_private;
} node_ptr;

typedef struct _node_object {
	zend_object  std;
	node_ptr *node;
	dom_ref_obj *document;
} node_object;

typedef struct _dom_object {
	zend_object  std;
	node_ptr *ptr;
	dom_ref_obj *document;
	HashTable *prop_handler;
	zend_object_handle handle;
} dom_object;

#ifdef PHP_WIN32
#ifdef PHPAPI
#undef PHPAPI
#endif
#ifdef DOM_EXPORTS
#define PHPAPI __declspec(dllexport)
#else
#define PHPAPI __declspec(dllimport)
#endif /* DOM_EXPORTS */
#endif /* PHP_WIN32 */

#define PHP_DOM_EXPORT(__type) PHPAPI __type

PHP_DOM_EXPORT(zval *) php_dom_create_object(xmlNodePtr obj, int *found, zval *in, zval* return_value, dom_object *domobj TSRMLS_DC);
PHP_DOM_EXPORT(zval *) dom_read_property(zval *object, zval *member TSRMLS_DC);
PHP_DOM_EXPORT(void) dom_write_property(zval *object, zval *member, zval *value TSRMLS_DC);

#define DOM_XMLNS_NAMESPACE \
    (const xmlChar *) "http://www.w3.org/2000/xmlns/"

#define NODE_GET_OBJ(__ptr, __id, __prtype, __intern) { \
	__intern = (node_object *)zend_object_store_get_object(__id TSRMLS_CC); \
	if (__intern->ptr == NULL || !(__ptr = (__prtype)__intern->ptr->node)) { \
  		php_error(E_WARNING, "Couldn't fetch %s", __intern->std.ce->name);\
  		RETURN_NULL();\
  	} \
}

#define DOC_GET_OBJ(__ptr, __id, __prtype, __intern) { \
	__intern = (node_object *)zend_object_store_get_object(__id TSRMLS_CC); \
	if (__intern->document != NULL) { \
		if (!(__ptr = (__prtype)__intern->document->ptr)) { \
  			php_error(E_WARNING, "Couldn't fetch %s", __intern->std.ce->name);\
  			RETURN_NULL();\
  		} \
	} \
}

#define DOM_RET_OBJ(zval, obj, ret, domobject) \
	if (NULL == (zval = php_dom_create_object(obj, ret, zval, return_value, domobject TSRMLS_CC))) { \
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot create required DOM object"); \
		RETURN_FALSE; \
	}

#define DOM_GET_THIS(zval) \
	if (NULL == (zval = getThis())) { \
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Underlying object missing"); \
		RETURN_FALSE; \
	}

#define DOM_GET_THIS_OBJ(__ptr, __id, __prtype, __intern) \
	DOM_GET_THIS(__id); \
	DOM_GET_OBJ(__ptr, __id, __prtype, __intern);

#endif
