/* 
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_01.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
 */


/* $Id$ */


#include "php.h"
#include "main.h"
#include "modules.h"
#include "internal_functions_registry.h"
#include "zend_compile.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "ext/bcmath/php_bcmath.h"
#include "ext/db/php_db.h"
#include "ext/gd/php_gd.h"
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
#include "ext/standard/php_syslog.h"
#include "ext/standard/php_standard.h"
#include "ext/standard/php_lcg.h"
#include "ext/standard/php_output.h"
#include "ext/standard/php_array.h"
#include "ext/standard/php_assert.h"
#include "ext/com/php_COM.h"
#include "ext/standard/reg.h"
#include "ext/pcre/php_pcre.h"
#include "ext/odbc/php_odbc.h"
#include "ext/session/php_session.h"
#include "ext/xml/php_xml.h"
#include "ext/mysql/php_mysql.h"

/* SNMP has to be moved to ext */
/* #include "dl/snmp/php_snmp.h" */

zend_module_entry *php_builtin_extensions[] = {
#if WITH_BCMATH
	phpext_bcmath_ptr,
#endif
	phpext_standard_ptr,
	COM_module_ptr,
	phpext_pcre_ptr,
	phpext_odbc_ptr,
	phpext_session_ptr,
	phpext_xml_ptr,
	phpext_mysql_ptr
};

#define EXTCOUNT (sizeof(php_builtin_extensions)/sizeof(zend_module_entry *))

	
int php_startup_internal_extensions(void)
{
	return php_startup_extensions(php_builtin_extensions, EXTCOUNT);
}

int php_global_startup_internal_extensions(void)
{
	return php_global_startup_extensions(php_builtin_extensions, EXTCOUNT);
}

int php_global_shutdown_internal_extensions(void)
{
	return php_global_shutdown_extensions(php_builtin_extensions, EXTCOUNT);
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
