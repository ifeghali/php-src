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
   | Author: Kirill Maximov (kir@rus.net)                                 |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef QUOT_PRINT_H
#define QUOT_PRINT_H

PHPAPI unsigned char *php_quot_print_decode(const unsigned char *str, size_t length, size_t *ret_length, int replace_us_by_ws);

PHP_FUNCTION(quoted_printable_decode);

#endif /* QUOT_PRINT_H */
