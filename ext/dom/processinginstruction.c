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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_dom.h"


/*
* class domprocessinginstruction extends domnode 
*
* URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#ID-1004215813
* Since: 
*/

zend_function_entry php_dom_processinginstruction_class_functions[] = {
	PHP_FALIAS("domprocessinginstruction", dom_processinginstruction_processinginstruction, NULL)
	{NULL, NULL, NULL}
};

/* {{{ proto domnode dom_processinginstruction_processinginstruction(string name, [string value]); */
PHP_FUNCTION(dom_processinginstruction_processinginstruction)
{

	zval *id;
	xmlNodePtr nodep = NULL, oldnode = NULL;
	dom_object *intern;
	char *name, *value = NULL;
	int name_len, value_len;

	id = getThis();
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &name, &name_len, &value, &value_len) == FAILURE) {
		return;
	}

	if (name_len == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "PI name is required");
		RETURN_FALSE;
	}

	nodep = xmlNewPI((xmlChar *) name, (xmlChar *) value);

	if (!nodep)
		RETURN_FALSE;

	intern = (dom_object *)zend_object_store_get_object(id TSRMLS_CC);
	if (intern != NULL) {
		oldnode = (xmlNodePtr)intern->ptr;
		if (oldnode != NULL) {
			node_free_resource(oldnode  TSRMLS_CC);
		}
		php_dom_set_object(intern, nodep TSRMLS_CC);
	}
}
/* }}} end dom_processinginstruction_processinginstruction */

/* {{{ proto target	string	
readonly=yes 
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#ID-1478689192
Since: 
*/
int dom_processinginstruction_target_read(dom_object *obj, zval **retval TSRMLS_DC)
{
	xmlNodePtr nodep;

	nodep = obj->ptr;

	ALLOC_ZVAL(*retval);
	ZVAL_STRING(*retval, (char *) (nodep->name), 1);

	return SUCCESS;
}

/* }}} */



/* {{{ proto data	string	
readonly=no 
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#ID-837822393
Since: 
*/
int dom_processinginstruction_data_read(dom_object *obj, zval **retval TSRMLS_DC)
{
	xmlNodePtr nodep;
	xmlChar *content;

	nodep = obj->ptr;

	ALLOC_ZVAL(*retval);

	
	if ((content = xmlNodeGetContent(nodep)) != NULL) {
		ZVAL_STRING(*retval, content, 1);
	} else {
		ZVAL_EMPTY_STRING(*retval);
	}

	xmlFree(content);

	return SUCCESS;
}

int dom_processinginstruction_data_write(dom_object *obj, zval *newval TSRMLS_DC)
{
	xmlNode *nodep;

	nodep = obj->ptr;
	xmlNodeSetContentLen(nodep, Z_STRVAL_P(newval), Z_STRLEN_P(newval) + 1);

	return SUCCESS;
}

/* }}} */


