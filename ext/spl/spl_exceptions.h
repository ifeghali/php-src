/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2004 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifndef SPL_EXCEPTIONS_H
#define SPL_EXCEPTIONS_H

#include "php.h"
#include "php_spl.h"

extern zend_class_entry *spl_ce_LogicException;
extern zend_class_entry *spl_ce_BadFunctionCallException;
extern zend_class_entry *spl_ce_BadMethodCallException;
extern zend_class_entry *spl_ce_DomainException;
extern zend_class_entry *spl_ce_InvalidArgumentException;
extern zend_class_entry *spl_ce_LengthException;
extern zend_class_entry *spl_ce_OutOfRangeException;

extern zend_class_entry *spl_ce_RuntimeException;
extern zend_class_entry *spl_ce_OutOfBoundsException;
extern zend_class_entry *spl_ce_OverflowException;
extern zend_class_entry *spl_ce_RangeException;
extern zend_class_entry *spl_ce_UnderflowException;

PHP_MINIT_FUNCTION(spl_exceptions);

#endif /* SPL_EXCEPTIONS_H */

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
