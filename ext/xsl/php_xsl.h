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
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_XSL_H
#define PHP_XSL_H

extern zend_module_entry xsl_module_entry;
#define phpext_xsl_ptr &xsl_module_entry

#ifdef PHP_WIN32
#define PHP_XSL_API __declspec(dllexport)
#else
#define PHP_XSL_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include <libxslt/xsltconfig.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>
#include <libxslt/transform.h>

#include "../dom/xml_common.h"
#include "xsl_fe.h"

typedef struct _xsl_object {
	zend_object  std;
	void *ptr;
	dom_ref_obj *document;
	HashTable *prop_handler;
	zend_object_handle handle;
    HashTable *parameter;
} xsl_object;

void php_xsl_set_object(zval *wrapper, void *obj TSRMLS_DC);
void xsl_objects_dtor(void *object, zend_object_handle handle TSRMLS_DC);
zval *php_xsl_create_object(xsltStylesheetPtr obj, int *found, zval *wrapper_in, zval *return_value  TSRMLS_DC);

#define REGISTER_XSL_CLASS(ce, name, parent_ce, funcs, entry) \
INIT_CLASS_ENTRY(ce, name, funcs); \
ce.create_object = xsl_objects_new; \
entry = zend_register_internal_class_ex(&ce, parent_ce, NULL TSRMLS_CC);

#define XSL_DOMOBJ_NEW(zval, obj, ret) \
	if (NULL == (zval = php_xsl_create_object(obj, ret, zval, return_value TSRMLS_CC))) { \
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot create required DOM object"); \
		RETURN_FALSE; \
	}

    
    
PHP_MINIT_FUNCTION(xsl);
PHP_MSHUTDOWN_FUNCTION(xsl);
PHP_RINIT_FUNCTION(xsl);
PHP_RSHUTDOWN_FUNCTION(xsl);
PHP_MINFO_FUNCTION(xsl);


/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(xsl)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(xsl)
*/

/* In every utility function you add that needs to use variables 
   in php_xsl_globals, call TSRM_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as XSL_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define XSL_G(v) TSRMG(xsl_globals_id, zend_xsl_globals *, v)
#else
#define XSL_G(v) (xsl_globals.v)
#endif

#endif	/* PHP_XSL_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
