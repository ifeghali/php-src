/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


#ifndef _OUTPUT_BUFFER
#define _OUTPUT_BUFFER

#include "php.h"

PHPAPI void php_output_startup(void);
PHPAPI int  php_body_write(const char *str, uint str_length);
PHPAPI int  php_header_write(const char *str, uint str_length);
PHPAPI void php_start_ob_buffering(void);
PHPAPI void php_end_ob_buffering(int send_buffer);
PHPAPI int php_ob_get_buffer(pval *p);
PHPAPI void php_start_implicit_flush(void);
PHPAPI void php_end_implicit_flush(void);
PHPAPI char *php_get_output_start_filename(void);
PHPAPI int php_get_output_start_lineno(void);

PHP_FUNCTION(ob_start);
PHP_FUNCTION(ob_end_flush);
PHP_FUNCTION(ob_end_clean);
PHP_FUNCTION(ob_get_contents);
PHP_FUNCTION(ob_implicit_flush);

PHP_GINIT_FUNCTION(output);

#endif /* _OUTPUT_BUFFER */
