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
   | Authors: Stig S�ther Bakken <ssb@fast.no>                            |
   |          Thies C. Arntzen <thies@digicol.de>                         |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef _PHP_XML_H
#define _PHP_XML_H

#ifdef HAVE_LIBEXPAT

extern zend_module_entry xml_module_entry;
#define xml_module_ptr &xml_module_entry

#else

#define xml_module_ptr NULL

#endif

#if defined(HAVE_LIBEXPAT) && defined(PHP_XML_INTERNAL)

#include <xmltok.h>
#include <xmlparse.h>

#ifdef PHP_WIN32
#define PHP_XML_API __declspec(dllexport)
#else
#define PHP_XML_API
#endif


#ifdef XML_UNICODE
#error "UTF-16 Unicode support not implemented!"
#endif

typedef struct {
	XML_Char *default_encoding;
} php_xml_globals;

typedef struct {
	int index;
	int case_folding;
	XML_Parser parser;
	XML_Char *target_encoding;
	char *startElementHandler;
	char *endElementHandler;
	char *characterDataHandler;
	char *processingInstructionHandler;
	char *defaultHandler;
	char *unparsedEntityDeclHandler;
	char *notationDeclHandler;
	char *externalEntityRefHandler;
	char *unknownEncodingHandler;

	zval *object;

	zval *data;
	zval *info;
	int level;
	int toffset;
	int curtag;
	pval **ctag;
	char **ltags;
	int lastwasopen;
	int skipwhite;
	
	XML_Char *baseURI;
} xml_parser;


typedef struct {
	XML_Char *name;
	char (*decoding_function)(unsigned short);
	unsigned short (*encoding_function)(unsigned char);
} xml_encoding;


enum php_xml_option {
    PHP_XML_OPTION_CASE_FOLDING = 1,
    PHP_XML_OPTION_TARGET_ENCODING,
    PHP_XML_OPTION_SKIP_TAGSTART,
    PHP_XML_OPTION_SKIP_WHITE
};

#define RETURN_OUT_OF_MEMORY \
	php_error(E_WARNING, "Out of memory");\
	RETURN_FALSE

/* for xml_parse_into_struct */
	
#define XML_MAXLEVEL 255 /* XXX this should be dynamic */
	
PHP_FUNCTION(xml_parser_create);
PHP_FUNCTION(xml_set_object);
PHP_FUNCTION(xml_set_element_handler);
PHP_FUNCTION(xml_set_character_data_handler);
PHP_FUNCTION(xml_set_processing_instruction_handler);
PHP_FUNCTION(xml_set_default_handler);
PHP_FUNCTION(xml_set_unparsed_entity_decl_handler);
PHP_FUNCTION(xml_set_notation_decl_handler);
PHP_FUNCTION(xml_set_external_entity_ref_handler);
PHP_FUNCTION(xml_parse);
PHP_FUNCTION(xml_get_error_code);
PHP_FUNCTION(xml_error_string);
PHP_FUNCTION(xml_get_current_line_number);
PHP_FUNCTION(xml_get_current_column_number);
PHP_FUNCTION(xml_get_current_byte_index);
PHP_FUNCTION(xml_parser_free);
PHP_FUNCTION(xml_parser_set_option);
PHP_FUNCTION(xml_parser_get_option);
PHP_FUNCTION(utf8_encode);
PHP_FUNCTION(utf8_decode);
PHP_FUNCTION(xml_parse_into_struct);

PHPAPI char *_xml_zval_strdup(zval *val);

#endif /* HAVE_LIBEXPAT */

#define phpext_xml_ptr xml_module_ptr

#ifdef ZTS
#define XMLLS_D php_xml_globals *xml_globals
#define XMLLS_DC , PSLS_D
#define XMLLS_C xml_globals
#define XMLLS_CC , XMLLS_C
#define XML(v) (xml_globals->v)
#define XMLLS_FETCH() php_xml_globals *xml_globals = ts_resource(xml_globals_id)
#else
#define XMLLS_D
#define XMLLS_DC
#define XMLLS_C
#define XMLLS_CC
#define XML(v) (xml_globals.v)
#define XMLLS_FETCH()
#endif

#endif /* _PHP_XML_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
