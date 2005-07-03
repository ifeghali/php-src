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
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef DATETIME_H
#define DATETIME_H

PHP_FUNCTION(time);
PHP_FUNCTION(idate);
PHP_FUNCTION(localtime);
PHP_FUNCTION(getdate);
PHP_FUNCTION(checkdate);
#if HAVE_STRPTIME
PHP_FUNCTION(strptime);
#endif 

PHPAPI int php_idate(char format, int timestamp, int gm);
PHPAPI char *php_std_date(time_t t TSRMLS_DC);

#endif /* DATETIME_H */
