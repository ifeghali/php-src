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
   | Authors: Christian Stocker <chregu@php.net>                          |
   |          Rob Richards <rrichards@php.net>                            |
   +----------------------------------------------------------------------+
*/

/* $Id: cdatasection.c,v 1.1 2003/06/05 17:06:52 rrichards Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_dom.h"


/*
* class domcdatasection extends domtext 
*
* URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#ID-667469212
* Since: 
*/

zend_function_entry php_dom_cdatasection_class_functions[] = {
	PHP_FALIAS("domcdatasection", dom_cdatasection_cdatasection, NULL)
	{NULL, NULL, NULL}
};

/* {{{ proto domnode dom_cdatasection_cdatasection([string value]); */
PHP_FUNCTION(dom_cdatasection_cdatasection)
{

	zval *id;
	xmlNodePtr nodep = NULL, oldnode = NULL;
	dom_object *intern;
	char *value = NULL;
	int value_len;

	id = getThis();
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &value, &value_len) == FAILURE) {
		return;
	}


	nodep = xmlNewCDataBlock(NULL, (xmlChar *) value, value_len);

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
/* }}} end dom_cdatasection_cdatasection */
