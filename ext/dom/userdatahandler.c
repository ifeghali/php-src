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

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_dom.h"


/*
* class domuserdatahandler 
*
* URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#UserDataHandler
* Since: DOM Level 3
*/

zend_function_entry php_dom_userdatahandler_class_functions[] = {
	PHP_FALIAS(handle, dom_userdatahandler_handle, NULL)
	{NULL, NULL, NULL}
};

/* {{{ attribute protos, not implemented yet */


/* {{{ proto dom_void dom_userdatahandler_handle(unsigned short operation, string key, domobject data, node src, node dst);
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#ID-handleUserDataEvent
Since: 
*/
PHP_FUNCTION(dom_userdatahandler_handle)
{
 DOM_NOT_IMPLEMENTED();
}
/* }}} end dom_userdatahandler_handle */
