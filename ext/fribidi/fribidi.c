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
   | Authors: Onn Ben-Zvi <onnb@mercury.co.il>                            |
   |          Tal Peer <tal@php.net>
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_fribidi.h"

#if HAVE_FRIBIDI

#include "ext/standard/info.h"
#include <fribidi/fribidi.h>

function_entry fribidi_functions[] = {
	PHP_FE(fribidi_log2vis,	NULL)		
	{NULL, NULL, NULL}
};

zend_module_entry fribidi_module_entry = {
	STANDARD_MODULE_HEADER,
	"fribidi",
	fribidi_functions,
	PHP_MINIT(fribidi),
	PHP_MSHUTDOWN(fribidi),
	NULL,
	NULL,
	PHP_MINFO(fribidi),
	"0.1",
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_FRIBIDI
ZEND_GET_MODULE(fribidi)
#endif

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(fribidi)
{
	/* Charsets */
	REGISTER_LONG_CONSTANT("FRIBIDI_CHARSET_UTF8", FRIBIDI_CHARSET_UTF8, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FRIBIDI_CHARSET_8859_6", FRIBIDI_CHARSET_ISO8859_6, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FRIBIDI_CHARSET_8859_8", FRIBIDI_CHARSET_ISO8859_8, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FRIBIDI_CHARSET_CP1255", FRIBIDI_CHARSET_CP1255, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FRIBIDI_CHARSET_CP1256", FRIBIDI_CHARSET_CP1256, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FRIBIDI_CHARSET_ISIRI_3342", FRIBIDI_CHARSET_ISIRI_3342, CONST_CS | CONST_PERSISTENT);
	
	/* Directions */
	REGISTER_LONG_CONSTANT("FRIBIDI_AUTO", FRIBIDI_TYPE_ON, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FRIBIDI_LTR", FRIBIDI_TYPE_LTR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FRIBIDI_RTL", FRIBIDI_TYPE_RTL, CONST_CS | CONST_PERSISTENT);
	
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(fribidi)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(fribidi)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "FriBidi support", "enabled");
	php_info_print_table_header(2, "FriBidi version", FRIBIDI_VERSION);
	php_info_print_table_end();
}
/* }}} */

/*
+ -----------------------------------------------------------+
| Name: fribidi_log2vis                                      |
| Purpose: convert a logical string to a visual one          |
| Input: 1) The logical string.                              |
|        2) Base direction -                                 |
|             Possible values:                               |
|             a) FRIBIDI_LTR  - left to right.               |
|             b) FRIBIDI_RTL  - right to left.               |
|             c) FRIBIDI_AUTO - autodetected by the BiDi     |
|                               BiDi algorithm.              |
|        3) Character code being used -                      |
|             Possible values (i.e., charsets supported)     |
|              FRIBIDI_CHARSET_UTF8                          |
|              FRIBIDI_CHARSET_8859_6                        |
|              FRIBIDI_CHARSET_8859_8                        |
|              FRIBIDI_CHARSET_CP1255                        |
|              FRIBIDI_CHARSET_CP1256                        |
|              FRIBIDI_CHARSET_ISIRI_3342                    |
|                                                            |
| Output: on success: The visual string.                     |
|         on failure: FALSE                                  |
+------------------------------------------------------------+
*/           

/* {{{ proto string fribidi_log2vis(string v_str, long direction, long charset)
   Convert a logical string to a visual one */
PHP_FUNCTION(fribidi_log2vis)
{
	zval **direction;
	long charset;
	FriBidiChar *u_logical_str, *u_visual_str;  /* unicode strings .... */
	char *v_str, *in_string, *out_string;
	int len, alloc_len;
	FriBidiCharType base_dir;
	FriBidiStrIndex *position_L_to_V_list;
	FriBidiStrIndex *position_V_to_L_list;
	FriBidiLevel    *embedding_level_list;
				   
	/* parse parameters from input */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "szl", &v_str, &len, &direction, &charset) == FAILURE) {
		return;
	}

	if (Z_TYPE_PP(direction) == IS_LONG) {
		convert_to_long_ex(direction);
		base_dir = Z_LVAL_PP(direction);
	} else if (Z_TYPE_PP(direction) == IS_STRING) {
		convert_to_string_ex(direction);
		if ((Z_STRVAL_PP(direction))[0] == 'R') {
			base_dir = FRIBIDI_TYPE_RTL;
		} else if (Z_STRVAL_PP(direction)[0] == 'L') {
			base_dir = FRIBIDI_TYPE_LTR;
		} else {
			base_dir = FRIBIDI_TYPE_ON;
		}
			
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "The use of strings to mark the base direction is deprecated, please use the FRIBIDI_LTR, FRIBIDI_RTL and FRIBIDI_AUTO constants");
	}

	/* allocate space and prepare all local variables */
	in_string = estrndup(v_str, len);
	alloc_len = len+1;

	u_logical_str = (FriBidiChar*) emalloc(sizeof(FriBidiChar)*alloc_len);
	u_visual_str = (FriBidiChar*) emalloc(sizeof(FriBidiChar)*alloc_len);
	
	position_L_to_V_list =  (FriBidiStrIndex *) emalloc(sizeof(FriBidiStrIndex)*alloc_len);
	position_V_to_L_list =  (FriBidiStrIndex *) emalloc(sizeof(FriBidiStrIndex)*alloc_len);
	embedding_level_list = (FriBidiLevel *) emalloc(sizeof(FriBidiLevel)*alloc_len);

	if(in_string[len-1] == '\n') {
		in_string[len-1] = '\0';
	}

	switch(charset) {
		case FRIBIDI_CHARSET_UTF8:
		case FRIBIDI_CHARSET_ISO8859_6:
		case FRIBIDI_CHARSET_ISO8859_8:
		case FRIBIDI_CHARSET_CP1255:
		case FRIBIDI_CHARSET_CP1256:
		case FRIBIDI_CHARSET_ISIRI_3342:
			len = fribidi_charset_to_unicode(charset, in_string, len, u_logical_str);
			break;
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown charset");
			efree(u_logical_str);
			efree(u_visual_str);
			efree(position_L_to_V_list);
			efree(position_V_to_L_list);
			efree(embedding_level_list);
			efree(in_string);
			RETURN_FALSE;
	}
	
	/* visualize the logical.... */
	
	out_string = (char *) emalloc(sizeof(char)*alloc_len);

	fribidi_log2vis(u_logical_str, len, &base_dir, u_visual_str, position_L_to_V_list, position_V_to_L_list, embedding_level_list);
	
	/* convert back to original char set */
	switch(charset) {
		case FRIBIDI_CHARSET_UTF8:
		case FRIBIDI_CHARSET_ISO8859_6:
		case FRIBIDI_CHARSET_ISO8859_8:
		case FRIBIDI_CHARSET_CP1255:
		case FRIBIDI_CHARSET_CP1256:
		case FRIBIDI_CHARSET_ISIRI_3342:
			fribidi_unicode_to_charset(charset, u_visual_str, len, out_string);
			break;									
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown charset");
			efree(u_logical_str);
			efree(u_visual_str);
			efree(position_L_to_V_list);
			efree(position_V_to_L_list);
			efree(embedding_level_list);
			efree(out_string);
			efree(in_string);
			RETURN_FALSE;
	}

	efree(u_logical_str);
	efree(u_visual_str);
	efree(position_L_to_V_list);
	efree(position_V_to_L_list);
	efree(embedding_level_list);
	efree(in_string);

	RETVAL_STRING(out_string, 1);
	efree(out_string);
}
/* }}} */

#endif	/* HAVE_FRIBIDI */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
