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
   | Authors: Uwe Steinmann <Uwe.Steinmann@fernuni-hagen.de>              |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

/* FdfTk lib 2.0 is a Complete C/C++ FDF Toolkit available from
   http://beta1.adobe.com/ada/acrosdk/forms.html. */

/* Note that there is no code from the FdfTk lib in this file */

#if !PHP_31 && defined(THREAD_SAFE)
#undef THREAD_SAFE
#endif

#include "php.h"
#include "ext/standard/head.h"
#include <math.h>
#include "php_fdf.h"

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if WIN32|WINNT
# include <io.h>
# include <fcntl.h>
#endif

#if HAVE_FDFLIB

#ifdef THREAD_SAFE
DWORD FDFlibTls;
static int numthreads=0;

typedef struct fdflib_global_struct{
	int le_fdf;
} fdflib_global_struct;

# define FDF_GLOBAL(a) fdflib_globals->a
# define FDF_TLS_VARS fdflib_global_struct *fdflib_globals=TlsGetValue(FDFlibTls)

#else
#  define FDF_GLOBAL(a) a
#  define FDF_TLS_VARS
int le_fdf_info;
int le_fdf;
#endif

function_entry fdf_functions[] = {
	PHP_FE(fdf_open,								NULL)
	PHP_FE(fdf_create,								NULL)
	PHP_FE(fdf_close,								NULL)
	PHP_FE(fdf_save,								NULL)
	PHP_FE(fdf_get_value,							NULL)
	PHP_FE(fdf_set_value,							NULL)
	PHP_FE(fdf_next_field_name,						NULL)
	PHP_FE(fdf_set_ap,								NULL)
	PHP_FE(fdf_set_status,							NULL)
	PHP_FE(fdf_get_status,							NULL)
	PHP_FE(fdf_set_file,							NULL)
	PHP_FE(fdf_get_file,							NULL)
	PHP_FE(fdf_add_template,							NULL)
	{NULL, NULL, NULL}
};

zend_module_entry fdf_module_entry = {
	"fdf", fdf_functions, PHP_MINIT(fdf), PHP_MSHUTDOWN(fdf), NULL, NULL,
	PHP_MINFO(fdf), STANDARD_MODULE_PROPERTIES
};

#if COMPILE_DL
#include "dl/phpdl.h"
DLEXPORT zend_module_entry *get_module(void) { return &fdf_module_entry; }
#endif

static void phpi_FDFClose(FDFDoc fdf) {
	(void)FDFClose(fdf);
}

PHP_MINIT_FUNCTION(fdf)
{
	FDFErc err;
	FDF_GLOBAL(le_fdf) = register_list_destructors(phpi_FDFClose, NULL);
	err = FDFInitialize();
	if(err == FDFErcOK)
		return SUCCESS;
	return FAILURE;
}

PHP_MINFO_FUNCTION(fdf)
{
	/* need to use a PHPAPI function here because it is external module in windows */
	php_printf("FdfTk Version %s", FDFGetVersion());
}

PHP_MSHUTDOWN_FUNCTION(fdf)
{
	FDFErc err;
	err = FDFFinalize();
	if(err == FDFErcOK)
		return SUCCESS;
	return FAILURE;
}

/* {{{ proto int fdf_open(string filename)
   Opens a new fdf document */
PHP_FUNCTION(fdf_open) {
	pval **file;
	int id, type;
	FDFDoc fdf;
	FDFErc err;
	FDF_TLS_VARS;


	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &file) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(file);

	err = FDFOpen((*file)->value.str.val, 0, &fdf);
	if(err != FDFErcOK)
		printf("Aiii, error\n");
	if(!fdf)
		RETURN_FALSE;

	id = zend_list_insert(fdf,FDF_GLOBAL(le_fdf));
	RETURN_LONG(id);
} /* }}} */

/* {{{ proto void fdf_close(int fdfdoc)
   Closes the fdf document */
PHP_FUNCTION(fdf_close) {
	pval **arg1;
	int id, type;
	FDFDoc fdf;
	FDF_TLS_VARS;

	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(arg1);
	id=(*arg1)->value.lval;
	fdf = zend_list_find(id,&type);
	if(!fdf || type!=FDF_GLOBAL(le_fdf)) {
		php_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}

/*	FDFClose(fdf); */
	zend_list_delete(id);

	RETURN_TRUE;
} /* }}} */

/* {{{ proto void fdf_create(void)
   Creates a new fdf document */
PHP_FUNCTION(fdf_create) {
	int id, type;
	FDFDoc fdf;
	FDFErc err;
	FDF_TLS_VARS;

	err = FDFCreate(&fdf);
	if(err != FDFErcOK)
		printf("Aiii, error\n");
	if(!fdf)
		RETURN_FALSE;

	id = zend_list_insert(fdf,FDF_GLOBAL(le_fdf));
	RETURN_LONG(id);
}
/* }}} */

/* {{{ proto void fdf_get_value(int fdfdoc, string fieldname)
   Gets the value of a field as string */
PHP_FUNCTION(fdf_get_value) {
	pval **arg1, **arg2;
	int id, type;
	ASInt32 nr;
	char *buffer;
	FDFDoc fdf;
	FDFErc err;
	FDF_TLS_VARS;

	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(arg1);
	convert_to_string_ex(arg2);
	id=(*arg1)->value.lval;
	fdf = zend_list_find(id,&type);
	if(!fdf || type!=FDF_GLOBAL(le_fdf)) {
		php_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}

	err = FDFGetValue(fdf, (*arg2)->value.str.val, NULL, 0, &nr);
	if(err != FDFErcOK)
		printf("Aiii, error\n");
  /* In the inofficial version of FdfTK 4.0 (as FDFGetVersion says. The
     library has a name with version 3.0, don't know what adobe has in
     mind) the number of bytes of the value doesn't include the trailing
     '\0'. This was not the case in 2.0
  */
	if(strcmp(FDFGetVersion(), "2.0"))
		nr++;
	buffer = emalloc(nr);
	err = FDFGetValue(fdf, (*arg2)->value.str.val, buffer, nr, &nr);
	if(err != FDFErcOK)
		printf("Aiii, error\n");

	RETURN_STRING(buffer, 0);
}
/* }}} */

/* {{{ proto void fdf_set_value(int fdfdoc, string fieldname, string value, int isName)
   Sets the value of a field */
PHP_FUNCTION(fdf_set_value) {
	pval **arg1, **arg2, **arg3, **arg4;
	int id, type;
	FDFDoc fdf;
	FDFErc err;
	FDF_TLS_VARS;

	if (ARG_COUNT(ht) != 4 || zend_get_parameters_ex(4, &arg1, &arg2,&arg3, &arg4) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(arg1);
	convert_to_string_ex(arg2);
	convert_to_string_ex(arg3);
	convert_to_long_ex(arg4);
	id=(*arg1)->value.lval;
	fdf = zend_list_find(id,&type);
	if(!fdf || type!=FDF_GLOBAL(le_fdf)) {
		php_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}

	err = FDFSetValue(fdf, (*arg2)->value.str.val,(*arg3)->value.str.val, (ASBool) (*arg4)->value.lval);
	if(err != FDFErcOK)
		printf("Aiii, error\n");

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void fdf_next_field_name(int fdfdoc [, string fieldname])
   Gets the name of the next field name or the first field name */
PHP_FUNCTION(fdf_next_field_name) {
	pval **argv[2];
	int id, type, argc;
	ASInt32 nr;
	char *buffer, *fieldname;
	FDFDoc fdf;
	FDFErc err;
	FDF_TLS_VARS;

	argc = ARG_COUNT(ht);
	if((argc > 2) || (argc < 1))
		WRONG_PARAM_COUNT;

	if (zend_get_parameters_array_ex(argc, argv) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(argv[0]);
	if(argc == 2) {
		convert_to_string_ex(argv[1]);
		fieldname = (*argv[1])->value.str.val;
	} else {
		fieldname = NULL;
	}
	id=(*argv[0])->value.lval;
	fdf = zend_list_find(id,&type);
	if(!fdf || type!=FDF_GLOBAL(le_fdf)) {
		php_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}

	err = FDFNextFieldName(fdf, fieldname, NULL, 0, &nr);
	if(err != FDFErcOK)
		printf("Aiii, error\n");
	if(nr == 0)
		RETURN_STRING(empty_string, 1);
		
	buffer = emalloc(nr);
	err = FDFNextFieldName(fdf, fieldname, buffer, nr, &nr);
	if(err != FDFErcOK)
		printf("Aiii, error\n");

	RETURN_STRING(buffer, 0);
}
/* }}} */

/* {{{ proto void fdf_set_ap(int fdfdoc, string fieldname, int face, string filename, int pagenr)
   Sets the value of a field */
PHP_FUNCTION(fdf_set_ap) {
	pval **arg1, **arg2, **arg3, **arg4, **arg5;
	int id, type;
	FDFDoc fdf;
	FDFErc err;
	FDFAppFace face;
	FDF_TLS_VARS;

	if (ARG_COUNT(ht) != 5 || zend_get_parameters_ex(5, &arg1, &arg2,&arg3, &arg4, &arg5) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(arg1);
	convert_to_string_ex(arg2);
	convert_to_long_ex(arg3);
	convert_to_string_ex(arg4);
	convert_to_long_ex(arg5);
	id=(*arg1)->value.lval;
	fdf = zend_list_find(id,&type);
	if(!fdf || type!=FDF_GLOBAL(le_fdf)) {
		php_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}

	switch((*arg3)->value.lval) {
		case 1:
			face = FDFNormalAP;
			break;
		case 2:
			face = FDFRolloverAP;
			break;
		case 3:
			face = FDFDownAP;
			break;
		default:
			face = FDFNormalAP;
	}

	err = FDFSetAP(fdf, (*arg2)->value.str.val, face, NULL,
(*arg4)->value.str.val, (ASInt32) (*arg5)->value.lval);
	if(err != FDFErcOK)
		printf("Aiii, error\n");

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void fdf_set_status(int fdfdoc, string status)
   Sets the value in the /Status key. */
PHP_FUNCTION(fdf_set_status) {
	pval **arg1, **arg2;
	int id, type;
	ASInt32 nr;
	FDFDoc fdf;
	FDFErc err;
	FDF_TLS_VARS;

	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(arg1);
	convert_to_string_ex(arg2);
	id=(*arg1)->value.lval;
	fdf = zend_list_find(id,&type);
	if(!fdf || type!=FDF_GLOBAL(le_fdf)) {
		php_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}

	err = FDFSetStatus(fdf, (*arg2)->value.str.val);
	if(err != FDFErcOK)
		printf("Aiii, error\n");

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void fdf_get_status(int fdfdoc)
   Gets the value in the /Status key. */
PHP_FUNCTION(fdf_get_status) {
	pval **arg1;
	int id, type;
	ASInt32 nr;
	char *buf;
	FDFDoc fdf;
	FDFErc err;
	FDF_TLS_VARS;

	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(arg1);
	id=(*arg1)->value.lval;
	fdf = zend_list_find(id,&type);
	if(!fdf || type!=FDF_GLOBAL(le_fdf)) {
		php_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}

	err = FDFGetStatus(fdf, NULL, 0,  &nr);
	if(err != FDFErcOK)
		printf("Aiii, error\n");
	if(nr == 0)
		RETURN_STRING(empty_string, 1);
	buf = emalloc(nr);
	err = FDFGetStatus(fdf, buf, nr,  &nr);
	if(err != FDFErcOK)
		printf("Aiii, error\n");

	RETURN_STRING(buf, 0);
}
/* }}} */

/* {{{ proto void fdf_set_file(int fdfdoc, string filename)
   Sets the value of the FDF's /F key */
PHP_FUNCTION(fdf_set_file) {
	pval **arg1, **arg2;
	int id, type;
	FDFDoc fdf;
	FDFErc err;
	FDF_TLS_VARS;

	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(arg1);
	convert_to_string_ex(arg2);
	id=(*arg1)->value.lval;
	fdf = zend_list_find(id,&type);
	if(!fdf || type!=FDF_GLOBAL(le_fdf)) {
		php_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}

	err = FDFSetFile(fdf, (*arg2)->value.str.val);
	if(err != FDFErcOK)
		printf("Aiii, error\n");

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void fdf_get_file(int fdfdoc)
   Gets the value in the /F key. */
PHP_FUNCTION(fdf_get_file) {
	pval **arg1;
	int id, type;
	ASInt32 nr;
	char *buf;
	FDFDoc fdf;
	FDFErc err;
	FDF_TLS_VARS;

	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(arg1);
	id=(*arg1)->value.lval;
	fdf = zend_list_find(id,&type);
	if(!fdf || type!=FDF_GLOBAL(le_fdf)) {
		php_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}

	err = FDFGetFile(fdf, NULL, 0,  &nr);
	if(err != FDFErcOK)
		printf("Aiii, error\n");
	if(nr == 0)
		RETURN_STRING(empty_string, 1);
	buf = emalloc(nr);
	err = FDFGetFile(fdf, buf, nr,  &nr);
	if(err != FDFErcOK)
		printf("Aiii, error\n");

	RETURN_STRING(buf, 0);
}
/* }}} */

/* {{{ proto void fdf_save(int fdfdoc, string filename)
   Writes out an FDF file. */
PHP_FUNCTION(fdf_save) {
	pval **arg1, **arg2;
	int id, type;
	FDFDoc fdf;
	FDFErc err;
	FDF_TLS_VARS;

	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(arg1);
	convert_to_string_ex(arg2);
	id=(*arg1)->value.lval;
	fdf = zend_list_find(id,&type);
	if(!fdf || type!=FDF_GLOBAL(le_fdf)) {
		php_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}

	err = FDFSave(fdf, (*arg2)->value.str.val);
	if(err != FDFErcOK)
		printf("Aiii, error\n");

	RETURN_TRUE;
} /* }}} */

/* {{{ proto void fdf_add_template(int fdfdoc, int newpage, string filename, string template, int rename)
   Adds a template to the FDF*/
PHP_FUNCTION(fdf_add_template) {
	pval **arg1, **arg2, **arg3, **arg4, **arg5;
	int id, type;
	FDFDoc fdf;
	FDFErc err;
	pdfFileSpecRec filespec;
	FDF_TLS_VARS;

	if (ARG_COUNT(ht) != 5 || zend_get_parameters_ex(5, &arg1, &arg2,&arg3, &arg4, &arg5) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(arg1);
	convert_to_long_ex(arg2);
	convert_to_string_ex(arg3);
	convert_to_string_ex(arg4);
	convert_to_long_ex(arg5);
	id=(*arg1)->value.lval;
	fdf = zend_list_find(id,&type);
	if(!fdf || type!=FDF_GLOBAL(le_fdf)) {
		php_error(E_WARNING,"Unable to find file identifier %d",id);
		RETURN_FALSE;
	}

	filespec.FS = NULL;
	filespec.F = (*arg3)->value.str.val;
	filespec.Mac = NULL;
	filespec.DOS = NULL;
	filespec.Unix = NULL;
	filespec.ID[0] = NULL;
	filespec.ID[1] = NULL;
	filespec.bVolatile = false;
	err = FDFAddTemplate(fdf, (*arg2)->value.lval, &filespec,(*arg4)->value.str.val, (*arg5)->value.lval);
	if(err != FDFErcOK)
		printf("Aiii, error\n");

	RETURN_TRUE;
}
/* }}} */

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
