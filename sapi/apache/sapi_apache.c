/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999 The PHP Group                         |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Rasmus Lerdorf <rasmus@php.net>                             |
   | (with helpful hints from Dean Gaudet <dgaudet@arctic.org>            |
   | PHP 4.0 patches by:                                                  |
   | Zeev Suraski <zeev@zend.com>                                         |
   | Stig Bakken <ssb@fast.no>                                            |
   +----------------------------------------------------------------------+
 */
/* $Id$ */

#include "php.h"

#include "httpd.h"
#include "http_config.h"
#if MODULE_MAGIC_NUMBER > 19980712
# include "ap_compat.h"
#else
# if MODULE_MAGIC_NUMBER > 19980324
#  include "compat.h"
# endif
#endif
#include "http_core.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_request.h"
#include "http_log.h"

#include "zend.h"
#include "php_ini.h"
#include "php_globals.h"
#include "SAPI.h"
#include "main.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_highlight.h"
#include "zend_indent.h"
#include "ext/standard/php3_standard.h"
#include "util_script.h"
#include "php_version.h"
/*#include "mod_php4.h"*/

PHPAPI int apache_php_module_main(request_rec *r, int fd, int display_source_mode SLS_DC)
{
	zend_file_handle file_handle;
#ifdef ZTS
	zend_compiler_globals cg;
	zend_executor_globals eg;
	php_core_globals pcg;
	zend_compiler_globals *compiler_globals=&cg;
	zend_executor_globals *executor_globals=&eg;
	php_core_globals *core_globals=&pcg;
#endif
	SLS_FETCH();

	if (php_request_startup(CLS_C ELS_CC PLS_CC SLS_CC) == FAILURE) {
		return FAILURE;
	}
	file_handle.type = ZEND_HANDLE_FD;
	file_handle.handle.fd = fd;
	file_handle.filename = SG(request_info).path_translated;
	file_handle.free_filename = 0;

	if (display_source_mode) {
		zend_syntax_highlighter_ini syntax_highlighter_ini;

		if (open_file_for_scanning(&file_handle CLS_CC)==SUCCESS) {
			php_get_highlight_struct(&syntax_highlighter_ini);
			zend_highlight(&syntax_highlighter_ini);
			fclose(file_handle.handle.fp);
			return OK;
		} else {
			return NOT_FOUND;
		}
	} else {
		(void) php_execute_script(&file_handle CLS_CC ELS_CC);
	}
	
	php3_header();			/* Make sure headers have been sent */
	php_end_ob_buffering(1);
	return (OK);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
