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
  | Author: Sterling Hughes <sterling@php.net>                           |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_SIMPLEXML_H
#define PHP_SIMPLEXML_H

extern zend_module_entry simplexml_module_entry;
#define phpext_simplexml_ptr &simplexml_module_entry

#ifdef PHP_WIN32
#define PHP_SIMPLEXML_API __declspec(dllexport)
#else
#define PHP_SIMPLEXML_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxml/uri.h>
#include <libxml/xmlerror.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xpointer.h>

PHP_MINIT_FUNCTION(simplexml);
PHP_MSHUTDOWN_FUNCTION(simplexml);
PHP_RINIT_FUNCTION(simplexml);
PHP_RSHUTDOWN_FUNCTION(simplexml);
PHP_MINFO_FUNCTION(simplexml);

typedef struct {
	zend_object zo;
	xmlDocPtr document;
	xmlNodePtr node;
} php_sxe_object;


#ifdef ZTS
#define SIMPLEXML_G(v) TSRMG(simplexml_globals_id, zend_simplexml_globals *, v)
#else
#define SIMPLEXML_G(v) (simplexml_globals.v)
#endif

#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
