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
   | Author: Wez Furlong <wez@thebrainroom.com>                           |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifndef PHP_COM_DOTNET_INTERNAL_H
#define PHP_COM_DOTNET_INTERNAL_H

#define _WIN32_DCOM
#define COBJMACROS
#include <ocidl.h>
#include "oleauto.h"
#include "win32/winutil.h"

/* brain-death in winutil.h defines the macro to hide the useful function... */
#undef php_win_err

typedef struct _php_com_dotnet_object {
	VARIANT v;

	ITypeInfo *typeinfo;
	int code_page;
	unsigned int have_default_bind:1;

	zend_class_entry *ce;
	DISPID default_bind; /* default property for array accesses */
} php_com_dotnet_object;

static inline int php_com_is_valid_object(zval *zv TSRMLS_DC)
{
	zend_class_entry *ce = Z_OBJCE_P(zv);
	return strcmp("com", ce->name) == 0 ||
		strcmp("dotnet", ce->name) == 0 ||
		strcmp("variant", ce->name) == 0;
}

#define CDNO_FETCH(zv)			(php_com_dotnet_object*)zend_object_store_get_object(zv TSRMLS_CC)
#define CDNO_FETCH_VERIFY(obj, zv)	do { \
	if (!php_com_is_valid_object(zv TSRMLS_CC)) { \
		php_com_throw_exception("expected a variant object" TSRMLS_CC); \
		return; \
	} \
	obj = (php_com_dotnet_object*)zend_object_store_get_object(zv TSRMLS_CC); \
} while(0)

/* com_extension.c */
TsHashTable php_com_typelibraries;
zend_class_entry *php_com_variant_class_entry;

/* com_handlers.c */
zend_object_value php_com_object_new(zend_class_entry *ce TSRMLS_DC);
void php_com_object_clone(void *object, void **clone_ptr TSRMLS_DC);
void php_com_object_dtor(void *object, zend_object_handle handle TSRMLS_DC);
zend_object_handlers php_com_object_handlers;

/* com_olechar.c */
PHPAPI char *php_com_olestring_to_string(OLECHAR *olestring,
		uint *string_len, int codepage TSRMLS_DC);
PHPAPI OLECHAR *php_com_string_to_olestring(char *string,
		uint string_len, int codepage TSRMLS_DC);


/* com_com.c */
PHP_FUNCTION(com_create_instance);
HRESULT php_com_invoke_helper(php_com_dotnet_object *obj, DISPID id_member,
		WORD flags, DISPPARAMS *disp_params, VARIANT *v TSRMLS_DC);
HRESULT php_com_get_id_of_name(php_com_dotnet_object *obj, char *name,
		int namelen, DISPID *dispid TSRMLS_DC);
int php_com_do_invoke_by_id(php_com_dotnet_object *obj, DISPID dispid,
		WORD flags,	VARIANT *v, int nargs, zval **args TSRMLS_DC);
int php_com_do_invoke(php_com_dotnet_object *obj, char *name, int namelen,
		WORD flags,	VARIANT *v, int nargs, zval **args TSRMLS_DC);

/* com_variant.c */
PHP_FUNCTION(com_variant_create_instance);
PHP_FUNCTION(variant_set);
PHP_FUNCTION(variant_add);
PHP_FUNCTION(variant_cat);
PHP_FUNCTION(variant_sub);
PHP_FUNCTION(variant_mul);
PHP_FUNCTION(variant_and);
PHP_FUNCTION(variant_div);
PHP_FUNCTION(variant_eqv);
PHP_FUNCTION(variant_idiv);
PHP_FUNCTION(variant_imp);
PHP_FUNCTION(variant_mod);
PHP_FUNCTION(variant_or);
PHP_FUNCTION(variant_pow);
PHP_FUNCTION(variant_xor);
PHP_FUNCTION(variant_abs);
PHP_FUNCTION(variant_fix);
PHP_FUNCTION(variant_int);
PHP_FUNCTION(variant_neg);
PHP_FUNCTION(variant_not);
PHP_FUNCTION(variant_round);
PHP_FUNCTION(variant_cmp);
PHP_FUNCTION(variant_date_to_timestamp);
PHP_FUNCTION(variant_date_from_timestamp);
PHP_FUNCTION(variant_get_type);
PHP_FUNCTION(variant_set_type);
PHP_FUNCTION(variant_cast);

PHPAPI void php_com_variant_from_zval_with_type(VARIANT *v, zval *z, VARTYPE type, int codepage TSRMLS_DC);
PHPAPI void php_com_variant_from_zval(VARIANT *v, zval *z, int codepage TSRMLS_DC);
PHPAPI int php_com_zval_from_variant(zval *z, VARIANT *v, int codepage TSRMLS_DC);

/* com_dotnet.c */
PHP_FUNCTION(com_dotnet_create_instance);
void php_com_dotnet_rshutdown(TSRMLS_D);

/* com_misc.c */
zval *php_com_throw_exception(char *message TSRMLS_DC);
PHPAPI void php_com_wrap_dispatch(zval *z, IDispatch *disp,
		int codepage TSRMLS_DC);
PHPAPI void php_com_wrap_variant(zval *z, VARIANT *v,
		int codepage TSRMLS_DC);

/* com_typeinfo.c */
PHPAPI ITypeLib *php_com_load_typelib_via_cache(char *search_string,
		int mode, int codepage, int *cached TSRMLS_DC);
PHPAPI ITypeLib *php_com_load_typelib(char *search_string, int mode,
		int codepage TSRMLS_DC);
PHPAPI int php_com_import_typelib(ITypeLib *TL, int mode,
		int codepage TSRMLS_DC);
void php_com_typelibrary_dtor(void *pDest);


#endif
