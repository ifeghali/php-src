/* 
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2002 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
 */


/* $Id$ */

/* {{{ includes
 */
#include "php.h"
#include "php_main.h"
#include "zend_modules.h"
#include "internal_functions_registry.h"
#include "zend_compile.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#if WITH_BCMATH
#include "ext/bcmath/php_bcmath.h"
#endif
#include "ext/standard/dl.h"
#include "ext/standard/file.h"
#include "ext/standard/fsock.h"
#include "ext/standard/head.h"
#include "ext/standard/pack.h"
#include "ext/standard/php_browscap.h"
#include "ext/standard/php_crypt.h"
#include "ext/standard/php_dir.h"
#include "ext/standard/php_filestat.h"
#include "ext/standard/php_mail.h"
#include "ext/standard/php_ext_syslog.h"
#include "ext/standard/php_standard.h"
#include "ext/standard/php_lcg.h"
#include "ext/standard/php_array.h"
#include "ext/standard/php_assert.h"
#include "ext/calendar/php_calendar.h"
#include "ext/com/php_COM.h"
#if HAVE_FTP
#include "ext/ftp/php_ftp.h"
#endif
#include "ext/standard/reg.h"
#include "ext/pcre/php_pcre.h"
#if HAVE_UODBC
#include "ext/odbc/php_odbc.h"
#endif
#include "ext/session/php_session.h"
#if HAVE_LIBEXPAT
#include "ext/xml/php_xml.h"
#endif
#if HAVE_LIBEXPAT && HAVE_WDDX
#include "ext/wddx/php_wddx.h"
#endif
#if HAVE_MYSQL
#include "ext/mysql/php_mysql.h"
#endif
#include "ext/mbstring/mbstring.h"
#if HAVE_OVERLOAD
#include "ext/overload/php_overload.h"
#endif
#include "ext/tokenizer/php_tokenizer.h"
/* }}} */

/* {{{ php_builtin_extensions[]
 */
zend_module_entry *php_builtin_extensions[] = {
	phpext_standard_ptr,
#if WITH_BCMATH
	phpext_bcmath_ptr,
#endif
	phpext_calendar_ptr,
	phpext_com_ptr,
#if HAVE_FTP
	phpext_ftp_ptr,
#endif
#if defined(MBSTR_ENC_TRANS)
	phpext_mbstring_ptr,
#endif
#if HAVE_MYSQL
	phpext_mysql_ptr,
#endif
#if HAVE_UODBC
	phpext_odbc_ptr,
#endif
#if HAVE_OVERLOAD
  phpext_overload_ptr,
#endif
	phpext_tokenizer_ptr,
	phpext_pcre_ptr,
#if HAVE_LIBEXPAT
	phpext_xml_ptr,
#endif
#if HAVE_LIBEXPAT && HAVE_WDDX
	phpext_wddx_ptr,
#endif
	phpext_session_ptr
};
/* }}} */

#define EXTCOUNT (sizeof(php_builtin_extensions)/sizeof(zend_module_entry *))

	
int php_startup_internal_extensions(void)
{
	return php_startup_extensions(php_builtin_extensions, EXTCOUNT);
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
