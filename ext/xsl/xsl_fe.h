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
  | Author: Christian Stocker <chregu@php.net>                           |
  +----------------------------------------------------------------------+
*/

/* $Id: header,v 1.12 2003/02/20 00:19:04 sniper Exp $ */

#ifndef XSL_FE_H
#define XSL_FE_H

extern zend_function_entry php_xsl_xsltprocessor_class_functions[];
zend_class_entry *xsl_xsltprocessor_class_entry;

PHP_FUNCTION(xsl_xsltprocessor_import_stylesheet);
PHP_FUNCTION(xsl_xsltprocessor_transform_to_doc);
PHP_FUNCTION(xsl_xsltprocessor_transform_to_uri);
PHP_FUNCTION(xsl_xsltprocessor_transform_to_xml);
PHP_FUNCTION(xsl_xsltprocessor_set_parameter);
PHP_FUNCTION(xsl_xsltprocessor_get_parameter);
PHP_FUNCTION(xsl_xsltprocessor_remove_parameter);

#endif
