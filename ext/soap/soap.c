/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2004 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Brad Lafountain <rodif_bl@yahoo.com>                        |
  |          Shane Caraveo <shane@caraveo.com>                           |
  |          Dmitry Stogov <dmitry@zend.com>                             |
  +----------------------------------------------------------------------+
*/
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php_soap.h"
#include "ext/session/php_session.h"

static int le_sdl = 0;
static int le_url = 0;
static int le_service = 0;

typedef struct _soapHeader {
	sdlFunctionPtr                    function;
	zval                              function_name;
	int                               mustUnderstand;
	int                               num_params;
	zval                            **parameters;
	zval                              retval;
	sdlSoapBindingFunctionHeaderPtr   hdr;
	struct _soapHeader               *next;
} soapHeader;

/* Local functions */
static void function_to_string(sdlFunctionPtr function, smart_str *buf);
static void type_to_string(sdlTypePtr type, smart_str *buf, int level);

static void clear_soap_fault(zval *obj TSRMLS_DC);
static void set_soap_fault(zval *obj, char *fault_code, char *fault_string, char *fault_actor, zval *fault_detail TSRMLS_DC);
static void soap_server_fault(char* code, char* string, char *actor, zval* details TSRMLS_DC);

static sdlParamPtr get_param(sdlFunctionPtr function, char *param_name, int index, int);
static sdlFunctionPtr get_function(sdlPtr sdl, const char *function_name);
static sdlFunctionPtr get_doc_function(sdlPtr sdl, xmlNodePtr node);

static sdlFunctionPtr deseralize_function_call(sdlPtr sdl, xmlDocPtr request, char* actor, zval *function_name, int *num_params, zval **parameters[], int *version, soapHeader **headers TSRMLS_DC);
static xmlDocPtr seralize_response_call(sdlFunctionPtr function, char *function_name,char *uri,zval *ret, soapHeader *headers, int version TSRMLS_DC);
static xmlDocPtr seralize_function_call(zval *this_ptr, sdlFunctionPtr function, char *function_name, char *uri, zval **arguments, int arg_count, int version, HashTable *soap_headers TSRMLS_DC);
static xmlNodePtr seralize_parameter(sdlParamPtr param,zval *param_val,int index,char *name, int style, xmlNodePtr parent TSRMLS_DC);
static xmlNodePtr seralize_zval(zval *val, sdlParamPtr param, char *paramName, int style, xmlNodePtr parent TSRMLS_DC);

static void delete_service(void *service);
static void delete_url(void *handle);

#ifndef ZEND_ENGINE_2
static void soap_call_function_handler(INTERNAL_FUNCTION_PARAMETERS, zend_property_reference *property_reference);
#endif

static void soap_error_handler(int error_num, const char *error_filename, const uint error_lineno, const char *format, va_list args);

#define SOAP_SERVER_BEGIN_CODE() \
	zend_bool _old_handler = SOAP_GLOBAL(use_soap_error_handler);\
	int _old_soap_version = SOAP_GLOBAL(soap_version);\
	SOAP_GLOBAL(use_soap_error_handler) = 1;

#define SOAP_SERVER_END_CODE() \
	SOAP_GLOBAL(use_soap_error_handler) = _old_handler;\
	SOAP_GLOBAL(soap_version) = _old_soap_version;

#define HTTP_RAW_POST_DATA "HTTP_RAW_POST_DATA"

#define ZERO_PARAM() \
	if (ZEND_NUM_ARGS() != 0) \
 		WRONG_PARAM_COUNT;

#define FETCH_THIS_SDL(ss) \
	{ \
		zval **__tmp; \
		if(FIND_SDL_PROPERTY(this_ptr,__tmp) != FAILURE) { \
			FETCH_SDL_RES(ss,__tmp); \
		} else { \
			ss = NULL; \
		} \
	}

#define FIND_SDL_PROPERTY(ss,tmp) zend_hash_find(Z_OBJPROP_P(ss), "sdl", sizeof("sdl"), (void **)&tmp)
#define FETCH_SDL_RES(ss,tmp) ss = (sdlPtr) zend_fetch_resource(tmp TSRMLS_CC, -1, "sdl", NULL, 1, le_sdl)

#define FETCH_THIS_SERVICE(ss) \
	{ \
		zval **tmp; \
		if (zend_hash_find(Z_OBJPROP_P(this_ptr),"service", sizeof("service"), (void **)&tmp) != FAILURE) { \
			ss = (soapServicePtr)zend_fetch_resource(tmp TSRMLS_CC, -1, "service", NULL, 1, le_service); \
		} else { \
			ss = NULL; \
		} \
	}

static zend_class_entry* soap_class_entry;
static zend_class_entry* soap_server_class_entry;
static zend_class_entry* soap_fault_class_entry;
zend_class_entry* soap_var_class_entry;
zend_class_entry* soap_param_class_entry;
zend_class_entry* soap_header_class_entry;

ZEND_DECLARE_MODULE_GLOBALS(soap);

static void (*old_error_handler)(int, const char *, const uint, const char*, va_list);

#define PHP_SOAP_SERVER_CLASSNAME "soapserver"
#define PHP_SOAP_CLASSNAME        "soapobject"
#define PHP_SOAP_VAR_CLASSNAME    "soapvar"
#define PHP_SOAP_FAULT_CLASSNAME  "soapfault"
#define PHP_SOAP_PARAM_CLASSNAME  "soapparam"
#define PHP_SOAP_HEADER_CLASSNAME "soapheader"

PHP_MINIT_FUNCTION(soap);
PHP_MSHUTDOWN_FUNCTION(soap);
PHP_MINFO_FUNCTION(soap);

#ifndef ZEND_ENGINE_2
# ifndef PHP_METHOD
#  define PHP_METHOD(classname, name)	ZEND_NAMED_FUNCTION(ZEND_FN(classname##_##name))
#  define PHP_ME(classname, name, arg_info, flags)	ZEND_NAMED_FE(name, ZEND_FN(classname##_##name), arg_info)
# endif

static char *zend_str_tolower_copy(char *dest, const char *source, unsigned int length)
{
	register unsigned char *str = (unsigned char*)source;
	register unsigned char *result = (unsigned char*)dest;
	register unsigned char *end = str + length;

	while (str < end) {
		*result++ = tolower((int)*str++);
	}
	*result = *end;

	return dest;
}
#endif

/*
  Registry Functions
  TODO: this!
*/
PHP_FUNCTION(soap_encode_to_xml);
PHP_FUNCTION(soap_encode_to_zval);
PHP_FUNCTION(use_soap_error_handler);


/* Server Functions */
PHP_METHOD(soapserver,soapserver);
PHP_METHOD(soapserver,setclass);
PHP_METHOD(soapserver,addfunction);
PHP_METHOD(soapserver,getfunctions);
PHP_METHOD(soapserver,handle);
PHP_METHOD(soapserver,setpersistence);
PHP_METHOD(soapserver,bind);
PHP_METHOD(soapserver,fault);
#ifdef HAVE_PHP_DOMXML
PHP_METHOD(soapserver,map);
#endif

/* Client Functions */
PHP_METHOD(soapobject, soapobject);
PHP_METHOD(soapobject, __login);
PHP_METHOD(soapobject, __useproxy);
PHP_METHOD(soapobject, __isfault);
PHP_METHOD(soapobject, __getfault);
PHP_METHOD(soapobject, __call);
PHP_METHOD(soapobject, __getfunctions);
PHP_METHOD(soapobject, __gettypes);
PHP_METHOD(soapobject, __trace);
PHP_METHOD(soapobject, __getlastresponse);
PHP_METHOD(soapobject, __getlastrequest);

/* SoapVar Functions */
PHP_METHOD(soapvar, soapvar);

/* SoapFault Functions */
PHP_METHOD(soapfault, soapfault);

/* SoapParam Functions */
PHP_METHOD(soapparam, soapparam);

/* SoapHeader Functions */
PHP_METHOD(soapheader, soapheader);

static zend_function_entry soap_functions[] = {
#ifdef HAVE_PHP_DOMXML
	PHP_FE(soap_encode_to_xml, NULL)
	PHP_FE(soap_encode_to_zval, NULL)
#endif
	PHP_FE(use_soap_error_handler, NULL)
	{NULL, NULL, NULL}
};

static zend_function_entry soap_fault_functions[] = {
	PHP_ME(soapfault, soapfault, NULL, 0)
	{NULL, NULL, NULL}
};

static zend_function_entry soap_server_functions[] = {
	PHP_ME(soapserver, soapserver, NULL, 0)
	PHP_ME(soapserver, setpersistence, NULL, 0)
	PHP_ME(soapserver, setclass, NULL, 0)
	PHP_ME(soapserver, addfunction, NULL, 0)
	PHP_ME(soapserver, getfunctions, NULL, 0)
	PHP_ME(soapserver, handle, NULL, 0)
	PHP_ME(soapserver, bind, NULL, 0)
	PHP_ME(soapserver, fault, NULL, 0)
#ifdef HAVE_PHP_DOMXML
	PHP_ME(soapserver, map, NULL, 0)
#endif
	{NULL, NULL, NULL}
};

#ifdef ZEND_ENGINE_2
ZEND_BEGIN_ARG_INFO(__call_args, 0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(0)
	ZEND_ARG_PASS_INFO(1)
ZEND_END_ARG_INFO();
#else
unsigned char __call_args[] = { 6, BYREF_NONE, BYREF_NONE, BYREF_NONE, BYREF_NONE, BYREF_NONE, BYREF_FORCE };
#endif

static zend_function_entry soap_client_functions[] = {
	PHP_ME(soapobject, soapobject, NULL, 0)
	PHP_ME(soapobject, __login, NULL, 0)
	PHP_ME(soapobject, __useproxy, NULL, 0)
	PHP_ME(soapobject, __isfault, NULL, 0)
	PHP_ME(soapobject, __getfault, NULL, 0)
	PHP_ME(soapobject, __call, __call_args, 0)
	PHP_ME(soapobject, __trace, NULL, 0)
	PHP_ME(soapobject, __getlastrequest, NULL, 0)
	PHP_ME(soapobject, __getlastresponse, NULL, 0)
	PHP_ME(soapobject, __getfunctions, NULL, 0)
	PHP_ME(soapobject, __gettypes, NULL, 0)
	{NULL, NULL, NULL}
};

static zend_function_entry soap_var_functions[] = {
	PHP_ME(soapvar, soapvar, NULL, 0)
	{NULL, NULL, NULL}
};

static zend_function_entry soap_param_functions[] = {
	PHP_ME(soapparam, soapparam, NULL, 0)
	{NULL, NULL, NULL}
};

static zend_function_entry soap_header_functions[] = {
	PHP_ME(soapheader, soapheader, NULL, 0)
	{NULL, NULL, NULL}
};

zend_module_entry soap_module_entry = {
#ifdef STANDARD_MODULE_HEADER
  STANDARD_MODULE_HEADER,
#endif
  "soap",
  soap_functions,
  PHP_MINIT(soap),
  PHP_MSHUTDOWN(soap),
  NULL,
  NULL,
  PHP_MINFO(soap),
#ifdef STANDARD_MODULE_HEADER
  NO_VERSION_YET,
#endif
  STANDARD_MODULE_PROPERTIES,
};

#ifdef COMPILE_DL_SOAP
ZEND_GET_MODULE(soap)
#endif

static void php_soap_init_globals(zend_soap_globals *soap_globals)
{
	int i;
	long enc;

	soap_globals->sdls = malloc(sizeof(HashTable));
	zend_hash_init(soap_globals->sdls, 0, NULL, delete_sdl, 1);

	soap_globals->defEnc = malloc(sizeof(HashTable));
	zend_hash_init(soap_globals->defEnc, 0, NULL, NULL, 1);

	soap_globals->defEncIndex = malloc(sizeof(HashTable));
	zend_hash_init(soap_globals->defEncIndex, 0, NULL, NULL, 1);

	soap_globals->defEncNs = malloc(sizeof(HashTable));
	zend_hash_init(soap_globals->defEncNs, 0, NULL, NULL, 1);

	soap_globals->defEncPrefix = malloc(sizeof(HashTable));
	zend_hash_init(soap_globals->defEncPrefix, 0, NULL, NULL, 1);

	soap_globals->overrides = NULL;

	i = 0;
	do {
		enc = (long)&defaultEncoding[i];

		/* If has a ns and a str_type then index it */
		if (defaultEncoding[i].details.type_str) {
			if (defaultEncoding[i].details.ns != NULL) {
				char *ns_type;
				ns_type = emalloc(strlen(defaultEncoding[i].details.ns) + strlen(defaultEncoding[i].details.type_str) + 2);
				sprintf(ns_type, "%s:%s", defaultEncoding[i].details.ns, defaultEncoding[i].details.type_str);
				zend_hash_add(soap_globals->defEnc, ns_type, strlen(ns_type) + 1, &enc, sizeof(encodePtr), NULL);
				efree(ns_type);
			} else {
				zend_hash_add(soap_globals->defEnc, defaultEncoding[i].details.type_str, strlen(defaultEncoding[i].details.type_str) + 1, &enc, sizeof(encodePtr), NULL);
			}
		}
		/* Index everything by number */
		if (!zend_hash_index_exists(soap_globals->defEncIndex, defaultEncoding[i].details.type)) {
			zend_hash_index_update(soap_globals->defEncIndex, defaultEncoding[i].details.type, &enc, sizeof(encodePtr), NULL);
		}
		i++;
	} while (defaultEncoding[i].details.type != END_KNOWN_TYPES);

	/* hash by namespace */
	zend_hash_add(soap_globals->defEncNs, XSD_1999_NAMESPACE, sizeof(XSD_1999_NAMESPACE), XSD_NS_PREFIX, sizeof(XSD_NS_PREFIX), NULL);
	zend_hash_add(soap_globals->defEncNs, XSD_NAMESPACE, sizeof(XSD_NAMESPACE), XSD_NS_PREFIX, sizeof(XSD_NS_PREFIX), NULL);
	zend_hash_add(soap_globals->defEncNs, SOAP_1_1_ENC_NAMESPACE, sizeof(SOAP_1_1_ENC_NAMESPACE), SOAP_1_1_ENC_NS_PREFIX, sizeof(SOAP_1_1_ENC_NS_PREFIX), NULL);
	zend_hash_add(soap_globals->defEncNs, SOAP_1_2_ENC_NAMESPACE, sizeof(SOAP_1_2_ENC_NAMESPACE), SOAP_1_2_ENC_NS_PREFIX, sizeof(SOAP_1_2_ENC_NS_PREFIX), NULL);

	/* and by prefix */
	zend_hash_add(soap_globals->defEncPrefix, XSD_NS_PREFIX, sizeof(XSD_NS_PREFIX), XSD_NAMESPACE, sizeof(XSD_NAMESPACE), NULL);
	zend_hash_add(soap_globals->defEncPrefix, SOAP_1_1_ENC_NS_PREFIX, sizeof(SOAP_1_1_ENC_NS_PREFIX), SOAP_1_1_ENC_NAMESPACE, sizeof(SOAP_1_1_ENC_NAMESPACE), NULL);
	zend_hash_add(soap_globals->defEncPrefix, SOAP_1_2_ENC_NS_PREFIX, sizeof(SOAP_1_2_ENC_NS_PREFIX), SOAP_1_2_ENC_NAMESPACE, sizeof(SOAP_1_2_ENC_NAMESPACE), NULL);

	soap_globals->use_soap_error_handler = 0;
	soap_globals->sdl = NULL;
	soap_globals->soap_version = SOAP_1_1;
}

PHP_MSHUTDOWN_FUNCTION(soap)
{
	zend_error_cb = old_error_handler;
	zend_hash_destroy(SOAP_GLOBAL(sdls));
	zend_hash_destroy(SOAP_GLOBAL(defEnc));
	zend_hash_destroy(SOAP_GLOBAL(defEncIndex));
	zend_hash_destroy(SOAP_GLOBAL(defEncNs));
	zend_hash_destroy(SOAP_GLOBAL(defEncPrefix));
	return SUCCESS;
}

PHP_MINIT_FUNCTION(soap)
{
	zend_class_entry ce;

	/* TODO: add ini entry for always use soap errors */
	ZEND_INIT_MODULE_GLOBALS(soap, php_soap_init_globals, NULL);

	/* Enable php stream/wrapper support for libxml */
	xmlRegisterDefaultInputCallbacks();
	xmlRegisterInputCallbacks(php_stream_xmlIO_match_wrapper, php_stream_xmlIO_open_wrapper,
			php_stream_xmlIO_read, php_stream_xmlIO_close);

	/* Register SoapObject class */
	/* BIG NOTE : THIS EMITS AN COMPILATION WARNING UNDER ZE2 - handle_function_call deprecated.
		soap_call_function_handler should be of type struct _zend_function, not (*handle_function_call).
	*/
#ifdef ZEND_ENGINE_2
	{
		zend_internal_function fe;

		fe.type = ZEND_INTERNAL_FUNCTION;
		fe.handler = zif_soapobject___call;
		fe.function_name = NULL;
		fe.scope = NULL;
		fe.fn_flags = 0;
		fe.prototype = NULL;
		fe.num_args = 2;
		fe.arg_info = NULL;
		fe.pass_rest_by_reference = 0;

		INIT_OVERLOADED_CLASS_ENTRY(ce, PHP_SOAP_CLASSNAME, soap_client_functions,
			(zend_function *)&fe, NULL, NULL);
		soap_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
	}
#else
	INIT_OVERLOADED_CLASS_ENTRY(ce, PHP_SOAP_CLASSNAME, soap_client_functions, soap_call_function_handler, NULL, NULL);
	soap_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
#endif

	/* Register SoapVar class */
	INIT_CLASS_ENTRY(ce, PHP_SOAP_VAR_CLASSNAME, soap_var_functions);
	soap_var_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

	/* Register SoapServer class */
	INIT_CLASS_ENTRY(ce, PHP_SOAP_SERVER_CLASSNAME, soap_server_functions);
	soap_server_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

	/* Register SoapFault class */
	INIT_CLASS_ENTRY(ce, PHP_SOAP_FAULT_CLASSNAME, soap_fault_functions);
	soap_fault_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

	/* Register SoapParam class */
	INIT_CLASS_ENTRY(ce, PHP_SOAP_PARAM_CLASSNAME, soap_param_functions);
	soap_param_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

	INIT_CLASS_ENTRY(ce, PHP_SOAP_HEADER_CLASSNAME, soap_header_functions);
	soap_header_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

	le_sdl = register_list_destructors(NULL, NULL);
	le_url = register_list_destructors(delete_url, NULL);
	le_service = register_list_destructors(delete_service, NULL);

	REGISTER_LONG_CONSTANT("SOAP_1_1", SOAP_1_1, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOAP_1_2", SOAP_1_2, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("SOAP_PERSISTENCE_SESSION", SOAP_PERSISTENCE_SESSION, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOAP_PERSISTENCE_REQUEST", SOAP_PERSISTENCE_REQUEST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOAP_FUNCTIONS_ALL", SOAP_FUNCTIONS_ALL, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("SOAP_ENCODED", SOAP_ENCODED, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOAP_LITERAL", SOAP_LITERAL, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("SOAP_RPC", SOAP_RPC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOAP_DOCUMENT", SOAP_DOCUMENT, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("XSD_1999_TIMEINSTANT", XSD_1999_TIMEINSTANT, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("SOAP_ACTOR_NEXT", SOAP_ACTOR_NEXT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOAP_ACTOR_NONE", SOAP_ACTOR_NONE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOAP_ACTOR_UNLIMATERECEIVER", SOAP_ACTOR_UNLIMATERECEIVER, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("UNKNOWN_TYPE", UNKNOWN_TYPE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOAP_ENC_OBJECT", SOAP_ENC_OBJECT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("SOAP_ENC_ARRAY", SOAP_ENC_ARRAY, CONST_CS | CONST_PERSISTENT);

	REGISTER_STRING_CONSTANT("XSD_NAMESPACE", XSD_NAMESPACE, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("XSD_ANYTYPE", XSD_ANYTYPE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_STRING", XSD_STRING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_BOOLEAN", XSD_BOOLEAN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_DECIMAL", XSD_DECIMAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_FLOAT", XSD_FLOAT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_DOUBLE", XSD_DOUBLE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_DURATION", XSD_DURATION, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_DATETIME", XSD_DATETIME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_TIME", XSD_TIME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_DATE", XSD_DATE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_GYEARMONTH", XSD_GYEARMONTH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_GYEAR", XSD_GYEAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_GMONTHDAY", XSD_GMONTHDAY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_GDAY", XSD_GDAY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_GMONTH", XSD_GMONTH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_HEXBINARY", XSD_HEXBINARY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_BASE64BINARY", XSD_BASE64BINARY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_ANYURI", XSD_ANYURI, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_QNAME", XSD_QNAME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_NOTATION", XSD_NOTATION, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_NORMALIZEDSTRING", XSD_NORMALIZEDSTRING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_TOKEN", XSD_TOKEN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_LANGUAGE", XSD_LANGUAGE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_NMTOKEN", XSD_NMTOKEN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_NAME", XSD_NAME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_NCNAME", XSD_NCNAME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_ID", XSD_ID, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_IDREF", XSD_IDREF, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_IDREFS", XSD_IDREFS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_ENTITY", XSD_ENTITY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_ENTITIES", XSD_ENTITIES, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_INTEGER", XSD_INTEGER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_NONPOSITIVEINTEGER", XSD_NONPOSITIVEINTEGER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_NEGATIVEINTEGER", XSD_NEGATIVEINTEGER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_LONG", XSD_LONG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_INT", XSD_INT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_SHORT", XSD_SHORT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_BYTE", XSD_BYTE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_NONNEGATIVEINTEGER", XSD_NONNEGATIVEINTEGER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_UNSIGNEDLONG", XSD_UNSIGNEDLONG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_UNSIGNEDINT", XSD_UNSIGNEDINT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_UNSIGNEDSHORT", XSD_UNSIGNEDSHORT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_UNSIGNEDBYTE", XSD_UNSIGNEDBYTE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("XSD_POSITIVEINTEGER", XSD_POSITIVEINTEGER, CONST_CS | CONST_PERSISTENT);

	old_error_handler = zend_error_cb;
	zend_error_cb = soap_error_handler;

	return SUCCESS;
}

PHP_MINFO_FUNCTION(soap)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Soap Client", "enabled");
	php_info_print_table_row(2, "Soap Server", "enabled");
#if HAVE_PHP_SESSION
	php_info_print_table_row(2, "Soap Serializer", "enabled");
#endif
	php_info_print_table_end();
}

#ifdef HAVE_PHP_DOMXML
PHP_FUNCTION(soap_encode_to_xml)
{
	zval *pzval, *ret;
	encodePtr enc;
	char *name;
	int found, name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &name_len, &pzval) == FAILURE) {
		php_error(E_ERROR, "wrong number of parameters to soap_encode_to_xml");
	}

	enc = get_conversion(Z_TYPE_P(pzval));
	ret = php_domobject_new(seralize_zval(pzval, NULL, name, SOAP_ENCODED), &found, NULL TSRMLS_CC);
	*return_value = *ret;
	zval_copy_ctor(return_value);
	zval_ptr_dtor(&ret);
}

PHP_FUNCTION(soap_encode_to_zval)
{
	zval *dom, **addr, *ret;
	xmlNodePtr node;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &dom) == FAILURE) {
		php_error(E_ERROR, "wrong number of parameters to soap_encode_to_zval");
	}

	if (zend_hash_index_find(Z_OBJPROP_P(dom), 1, (void **)&addr) == FAILURE) {
		php_error(E_ERROR, "Cannot find domaddress to parameter passed to soap_encode_to_zval");
	}

	node = (xmlNodePtr)Z_LVAL_PP(addr);
	ret = master_to_zval(NULL, node);
	*return_value = *ret;
}
#endif

/* SoapParam functions */
PHP_METHOD(soapparam,soapparam)
{
	zval *data;
	char *name;
	int name_length;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zs", &data, &name, &name_length) == FAILURE) {
		php_error(E_ERROR, "Invalid arguments to SoapParam constructor");
	}

#ifndef ZEND_ENGINE_2
	zval_add_ref(&data);
#endif
	add_property_stringl(this_ptr, "param_name", name, name_length, 1);
	add_property_zval(this_ptr, "param_data", data);
}

/* SoapHeader functions */
PHP_METHOD(soapheader,soapheader)
{
	zval *data = NULL, *actor = NULL;
	char *name, *ns;
	int name_len, ns_len;
	zend_bool must_understand = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|zbz", &ns, &ns_len, &name, &name_len, &data, &must_understand, &actor) == FAILURE) {
		php_error(E_ERROR, "Invalid arguments to SoapHeader constructor");
	}

	add_property_stringl(this_ptr, "namespace", ns, ns_len, 1);
	add_property_stringl(this_ptr, "name", name, name_len, 1);
	if (data) {
#ifndef ZEND_ENGINE_2
		zval_add_ref(&data);
#endif
		add_property_zval(this_ptr, "data", data);
	}
	add_property_bool(this_ptr, "mustUnderstand", must_understand);
	if (actor == NULL) {
		add_property_long(this_ptr, "actor", SOAP_ACTOR_NEXT);
	} else if (Z_TYPE_P(actor) == IS_LONG) {
		add_property_long(this_ptr, "actor", Z_LVAL_P(actor));
	} else if (Z_TYPE_P(actor) == IS_STRING) {
		add_property_stringl(this_ptr, "actor", Z_STRVAL_P(actor), Z_STRLEN_P(actor), 1);
	} else {
		php_error(E_ERROR, "Invalid arguments to SoapHeader constructor");
	}
}

/* SoapFault functions */
PHP_METHOD(soapfault,soapfault)
{
	char *fault_string = NULL, *fault_code = NULL, *fault_actor = NULL;
	int fault_string_len, fault_code_len, fault_actor_len;
	zval *details = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|zs",
		&fault_code, &fault_code_len,
		&fault_string, &fault_string_len,
		&details, &fault_actor, &fault_actor_len) == FAILURE) {
		php_error(E_ERROR, "Invalid arguments to SoapFault constructor");
	}

	set_soap_fault(this_ptr, fault_code, fault_string, fault_actor, details TSRMLS_CC);
}

/* SoapVar functions */
PHP_METHOD(soapvar,soapvar)
{
	zval *data, *type;
	char *stype = NULL, *ns = NULL, *name = NULL, *namens = NULL;
	int stype_len, ns_len, name_len, namens_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z!z|ssss", &data, &type, &stype, &stype_len, &ns, &ns_len, &name, &name_len, &namens, &namens_len) == FAILURE) {
		php_error(E_ERROR, "Invalid arguments to SoapVar constructor");
	}

	if (Z_TYPE_P(type) == IS_NULL) {
		add_property_long(this_ptr, "enc_type", UNKNOWN_TYPE);
	} else {
		if (zend_hash_index_exists(SOAP_GLOBAL(defEncIndex), Z_LVAL_P(type))) {
			add_property_long(this_ptr, "enc_type", Z_LVAL_P(type));
		} else {
			php_error(E_ERROR, "Invalid type ID to SoapVar constructor");
		}
	}

	if (data) {
#ifndef ZEND_ENGINE_2
		zval_add_ref(&data);
#endif
		add_property_zval(this_ptr, "enc_value", data);
	}

	if (stype && strlen(stype) > 0) {
		add_property_stringl(this_ptr, "enc_stype", stype, stype_len, 1);
	}
	if (ns && strlen(ns) > 0) {
		add_property_stringl(this_ptr, "enc_ns", ns, ns_len, 1);
	}
	if (name && strlen(name) > 0) {
		add_property_stringl(this_ptr, "enc_name", name, name_len, 1);
	}
	if (namens && strlen(namens) > 0) {
		add_property_stringl(this_ptr, "enc_namens", namens, namens_len, 1);
	}
}

/* SoapServer functions */
PHP_METHOD(soapserver,soapserver)
{
	soapServicePtr service;
	char *uri, *actor = NULL;
	int ret, uri_len, actor_len;
	long version = SOAP_1_1;

	SOAP_SERVER_BEGIN_CODE();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|ls", &uri, &uri_len, &version, &actor, &actor_len) == FAILURE) {
		php_error(E_ERROR, "Invalid arguments to SoapServer constructor");
	}

	service = emalloc(sizeof(soapService));
	memset(service, 0, sizeof(soapService));

	service->version = version;
	service->uri = estrndup(uri, uri_len);
	if (actor) {
		service->actor = estrndup(actor, actor_len);
	}
	service->type = SOAP_FUNCTIONS;
	service->soap_functions.functions_all = FALSE;
	service->soap_functions.ft = emalloc(sizeof(HashTable));
	zend_hash_init(service->soap_functions.ft, 0, NULL, ZVAL_PTR_DTOR, 0);

	ret = zend_list_insert(service, le_service);
	add_property_resource(this_ptr, "service", ret);
	zend_list_addref(ret);

	SOAP_SERVER_END_CODE();
}

#define NULL_OR_STRING(zval) \
	(!zval || Z_TYPE_P(zval) == IS_NULL || Z_TYPE_P(zval) == IS_STRING)

#define IS_VALID_FUNCTION(zval) \
	(zval && Z_TYPE_P(zval) != IS_NULL)

#ifdef HAVE_PHP_DOMXML
PHP_FUNCTION(SoapServer,map)
{
	char *type, *class_name;
	zval *to_xml_before = NULL, *to_xml = NULL, *to_xml_after = NULL,
		*to_zval_before = NULL, *to_zval = NULL, *to_zval_after = NULL;
	int type_len, class_name_len;
	char *ns, *ctype;
	soapServicePtr service;

	SOAP_SERVER_BEGIN_CODE();

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "sz|zzzzz",
		&type, &type_len, &to_xml_before, &to_xml, &to_xml_after, &to_zval_before, &to_zval,
		&to_zval_after) == SUCCESS && NULL_OR_STRING(to_xml_before) && NULL_OR_STRING(to_xml) &&
		NULL_OR_STRING(to_xml_after) && NULL_OR_STRING(to_zval_before) && NULL_OR_STRING(to_zval) &&
		NULL_OR_STRING(to_zval_after)) {

		soapMappingPtr map;
		encodePtr enc, new_enc;
		smart_str resloved_ns = {0};

		FETCH_THIS_SERVICE(service);

		new_enc = malloc(sizeof(encode));
		memset(new_enc, 0, sizeof(encode));

		ctype = strrchr(type, ':');
		if (ctype) {
			smart_str_appendl(&resloved_ns, type, ctype - type);
			smart_str_0(&resloved_ns);
			ctype++;
		} else {
			ns = NULL;
		}

		if (ns) {
			if (zend_hash_find(SOAP_GLOBAL(defEncPrefix),  resloved_ns.c, resloved_ns.len + 1, &ns) == SUCCESS) {
				enc = get_encoder(service->sdl, ns, ctype);
				smart_str_free(&resloved_ns);
				smart_str_appendl(&resloved_ns, ns, strlen(ns));
				smart_str_appendc(&resloved_ns, ':');
				smart_str_appendl(&resloved_ns, ctype, strlen(ctype));
				smart_str_0(&resloved_ns);
				type = resloved_ns.c;
				type_len = resloved_ns.len;
			} else {
				enc = get_encoder_ex(service->sdl, type);
			}
		} else {
			enc = get_encoder_ex(service->sdl, type);
		}

		new_enc->details.type = enc->details.type;
		new_enc->details.ns = strdup(enc->details.ns);
		new_enc->details.type_str = strdup(enc->details.type_str);
		new_enc->details.sdl_type = enc->details.sdl_type;
		new_enc->to_xml = enc->to_xml;
		new_enc->to_zval = enc->to_zval;
		new_enc->to_xml_before = enc->to_xml_before;
		new_enc->to_zval_before = enc->to_zval_before;
		new_enc->to_xml_after = enc->to_xml_after;
		new_enc->to_zval_after = enc->to_zval_after;

		map = emalloc(sizeof(soapMapping));
		memset(map, 0, sizeof(soapMapping));

		map->type = SOAP_MAP_FUNCTION;
		if (IS_VALID_FUNCTION(to_xml_before)) {
			zval_add_ref(&to_xml_before);
			map->map_functions.to_xml_before = to_xml_before;
			new_enc->to_xml_before = to_xml_before_user;
		}
		if (IS_VALID_FUNCTION(to_xml)) {
			zval_add_ref(&to_xml);
			map->map_functions.to_xml = to_xml;
			new_enc->to_xml = to_xml_user;
		}
		if (IS_VALID_FUNCTION(to_xml_after)) {
			zval_add_ref(&to_xml_after);
			map->map_functions.to_xml_after = to_xml_after;
			new_enc->to_xml_after = to_xml_after_user;
		}
		if (IS_VALID_FUNCTION(to_zval_before)) {
			zval_add_ref(&to_zval_before);
			map->map_functions.to_zval_before = to_zval_before;
			new_enc->to_zval_before = to_zval_before_user;
		}
		if (IS_VALID_FUNCTION(to_zval)) {
			zval_add_ref(&to_zval);
			map->map_functions.to_zval = to_zval;
			new_enc->to_zval = to_zval_user;
		}
		if (IS_VALID_FUNCTION(to_zval_after)) {
			zval_add_ref(&to_zval_after);
			map->map_functions.to_zval_after = to_zval_after;
			new_enc->to_zval_after = to_zval_after_user;
		}

		new_enc->details.map = map;

		if (!service->mapping) {
			service->mapping = emalloc(sizeof(HashTable));
			zend_hash_init(service->mapping, 0, NULL, delete_encoder, 0);
		}
		zend_hash_update(service->mapping, type, type_len + 1, &new_enc, sizeof(encodePtr), NULL);
		smart_str_free(&resloved_ns);
	} else if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", &type, &type_len, &class_name, &class_name_len, &type) == SUCCESS) {
	} else {
		php_error(E_ERROR, "Wrong number of parameters to SoapServer->map");
	}
}
#endif

PHP_METHOD(soapserver,bind)
{
	char *wsdl;
	int wsdl_len;
	soapServicePtr service;

	SOAP_SERVER_BEGIN_CODE();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &wsdl, &wsdl_len) == FAILURE) {
		php_error(E_ERROR, "Wrong number of parameters to SoapServer->bind");
	}

	FETCH_THIS_SERVICE(service);
	service->sdl = get_sdl(wsdl);

	SOAP_SERVER_END_CODE();
}

PHP_METHOD(soapserver,setpersistence)
{
	soapServicePtr service;
	int value;

	SOAP_SERVER_BEGIN_CODE();

	FETCH_THIS_SERVICE(service);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &value) != FAILURE) {
		if (service->type == SOAP_CLASS) {
			if (value == SOAP_PERSISTENCE_SESSION ||
				value == SOAP_PERSISTENCE_REQUEST) {
				service->soap_class.persistance = value;
			} else {
				php_error(E_ERROR, "Tried to set persistence with bogus value (%d)", value);
			}
		} else {
			php_error(E_ERROR, "Tried to set persistence when you are using you SOAP SERVER in function mode, no persistence needed");
		}
	}

	SOAP_SERVER_END_CODE();
}

PHP_METHOD(soapserver,setclass)
{
	soapServicePtr service;
	zend_class_entry *ce;
	char *class_name = NULL;
	int found, argc;
	zval ***argv;

	SOAP_SERVER_BEGIN_CODE();

	FETCH_THIS_SERVICE(service);

	argc = ZEND_NUM_ARGS();
	argv = emalloc(argc * sizeof(zval **));

	if (argc < 1 || zend_get_parameters_array_ex(argc, argv) == FAILURE) {
		efree(argv);
		WRONG_PARAM_COUNT;
	}

	if (Z_TYPE_PP(argv[0]) == IS_STRING) {
		class_name = estrdup(Z_STRVAL_PP(argv[0]));

		found = zend_hash_find(EG(class_table), php_strtolower(class_name, Z_STRLEN_PP(argv[0])), Z_STRLEN_PP(argv[0])	 + 1, (void **)&ce);
		efree(class_name);
		if (found != FAILURE) {
			service->type = SOAP_CLASS;
#ifdef ZEND_ENGINE_2
			service->soap_class.ce = *(zend_class_entry**)ce;
#else
			service->soap_class.ce = ce;
#endif
			service->soap_class.persistance = SOAP_PERSISTENCE_REQUEST;
			service->soap_class.argc = argc - 1;
			if (service->soap_class.argc > 0) {
				int i;
				service->soap_class.argv = emalloc(sizeof(zval) * service->soap_class.argc);
				for (i = 0;i < service->soap_class.argc;i++) {
					service->soap_class.argv[i] = *(argv[i + 1]);
					zval_add_ref(&service->soap_class.argv[i]);
				}
			}
		} else {
			php_error(E_ERROR, "Tried to set a non existant class (%s)", Z_STRVAL_PP(argv[0]));
		}
	} else {
		php_error(E_ERROR, "You must pass in a string to setclass");
	}

	efree(argv);

	SOAP_SERVER_END_CODE();
}

PHP_METHOD(soapserver,getfunctions)
{
	soapServicePtr  service;
	HashTable      *ft = NULL;

	SOAP_SERVER_BEGIN_CODE();

	ZERO_PARAM()
	FETCH_THIS_SERVICE(service);

	array_init(return_value);
	if (service->type == SOAP_CLASS) {
		ft = &service->soap_class.ce->function_table;
	} else if (service->soap_functions.functions_all == TRUE) {
		ft = EG(function_table);
	} else if (service->soap_functions.ft != NULL) {
		zval **name;
		HashPosition pos;

		zend_hash_internal_pointer_reset_ex(service->soap_functions.ft, &pos);
		while (zend_hash_get_current_data_ex(service->soap_functions.ft, (void **)&name, &pos) != FAILURE) {
			add_next_index_string(return_value, Z_STRVAL_PP(name), 1);
			zend_hash_move_forward_ex(service->soap_functions.ft, &pos);
		}
	}
	if (ft != NULL) {
		zend_function *f;
		HashPosition pos;
		zend_hash_internal_pointer_reset_ex(ft, &pos);
		while (zend_hash_get_current_data_ex(ft, (void **)&f, &pos) != FAILURE) {
			add_next_index_string(return_value, f->common.function_name, 1);
			zend_hash_move_forward_ex(ft, &pos);
		}
	}

	SOAP_SERVER_END_CODE();
}

PHP_METHOD(soapserver, addfunction)
{
	soapServicePtr service;
	zval *function_name, *function_copy;
	HashPosition pos;

	SOAP_SERVER_BEGIN_CODE();

	FETCH_THIS_SERVICE(service);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &function_name) == FAILURE) {
		php_error(E_ERROR, "Invalid parameters passed to addfunction");
	}

	/* TODO: could use zend_is_callable here */

	if (function_name->type == IS_ARRAY) {
		if (service->type == SOAP_FUNCTIONS) {
			zval **tmp_function, *function_copy;

			if (service->soap_functions.ft == NULL) {
				service->soap_functions.functions_all = FALSE;
				service->soap_functions.ft = emalloc(sizeof(HashTable));
				zend_hash_init(service->soap_functions.ft, 0, NULL, ZVAL_PTR_DTOR, 0);
			}

			zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(function_name), &pos);
			while (zend_hash_get_current_data_ex(Z_ARRVAL_P(function_name), (void **)&tmp_function, &pos) != FAILURE) {
				char *key;
				int   key_len;
				zend_function *f;

				if (Z_TYPE_PP(tmp_function) != IS_STRING) {
					php_error(E_ERROR, "Tried to add a function that isn't a string");
				}

				key_len = Z_STRLEN_PP(tmp_function);
				key = emalloc(key_len + 1);
				zend_str_tolower_copy(key, Z_STRVAL_PP(tmp_function), key_len);

				if (zend_hash_find(EG(function_table), key, key_len+1, (void**)&f) == FAILURE) {
					php_error(E_ERROR, "Tried to add a non existant function '%s'", Z_STRVAL_PP(tmp_function));
				}

				MAKE_STD_ZVAL(function_copy);
				ZVAL_STRING(function_copy, f->common.function_name, 1);
				zend_hash_update(service->soap_functions.ft, key, key_len+1, &function_copy, sizeof(zval *), NULL);

				efree(key);
				zend_hash_move_forward_ex(Z_ARRVAL_P(function_name), &pos);
			}
		}
	} else if (function_name->type == IS_STRING) {
		char *key;
		int   key_len;
		zend_function *f;

		key_len = Z_STRLEN_P(function_name);
		key = emalloc(key_len + 1);
		zend_str_tolower_copy(key, Z_STRVAL_P(function_name), key_len);

		if (zend_hash_find(EG(function_table), key, key_len+1, (void**)&f) == FAILURE) {
			php_error(E_ERROR, "Tried to add a non existant function '%s'", Z_STRVAL_P(function_name));
		}
		if (service->soap_functions.ft == NULL) {
			service->soap_functions.functions_all = FALSE;
			service->soap_functions.ft = emalloc(sizeof(HashTable));
			zend_hash_init(service->soap_functions.ft, 0, NULL, ZVAL_PTR_DTOR, 0);
		}

		MAKE_STD_ZVAL(function_copy);
		ZVAL_STRING(function_copy, f->common.function_name, 1);
		zend_hash_update(service->soap_functions.ft, key, key_len+1, &function_copy, sizeof(zval *), NULL);
		efree(key);
	} else if (function_name->type == IS_LONG) {
		if (Z_LVAL_P(function_name) == SOAP_FUNCTIONS_ALL) {
			if (service->soap_functions.ft != NULL) {
				zend_hash_destroy(service->soap_functions.ft);
				efree(service->soap_functions.ft);
				service->soap_functions.ft = NULL;
			}
			service->soap_functions.functions_all = TRUE;
		} else {
			php_error(E_ERROR, "Invalid value passed to addfunction");
		}
	}

	SOAP_SERVER_END_CODE();
}

PHP_METHOD(soapserver, handle)
{
	int soap_version, old_soap_version;
	sdlPtr old_sdl = NULL;
	soapServicePtr service;
	xmlDocPtr doc_request, doc_return;
	zval function_name, **params, **raw_post, *soap_obj, retval, **server_vars;
	char *fn_name, cont_len[30];
	int num_params = 0, size, i, call_status = 0;
	xmlChar *buf;
	HashTable *function_table;
	soapHeader *soap_headers;

	SOAP_SERVER_BEGIN_CODE();

	FETCH_THIS_SERVICE(service);
	SOAP_GLOBAL(soap_version) = service->version;
	ZERO_PARAM();
	INIT_ZVAL(retval);

	if (zend_hash_find(&EG(symbol_table), "_SERVER", sizeof("_SERVER"), (void **)&server_vars) == SUCCESS) {
		zval **req_method, **query_string;
		if (zend_hash_find(Z_ARRVAL_PP(server_vars), "REQUEST_METHOD", sizeof("REQUEST_METHOD"), (void **)&req_method) == SUCCESS) {
			if (!strcmp(Z_STRVAL_PP(req_method), "GET") && zend_hash_find(Z_ARRVAL_PP(server_vars), "QUERY_STRING", sizeof("QUERY_STRING"), (void **)&query_string) == SUCCESS) {
				if (strstr(Z_STRVAL_PP(query_string), "wsdl") != NULL ||
				   strstr(Z_STRVAL_PP(query_string), "WSDL") != NULL) {
					if (service->sdl) {
/*
					  char *hdr = emalloc(sizeof("Location: ")+strlen(service->sdl->source));
					  strcpy(hdr,"Location: ");
					  strcat(hdr,service->sdl->source);
						sapi_add_header(hdr, sizeof("Location: ")+strlen(service->sdl->source)-1, 1);
						efree(hdr);
*/
						zval readfile, readfile_ret, *param;

						INIT_ZVAL(readfile);
						INIT_ZVAL(readfile_ret);
						MAKE_STD_ZVAL(param);

						sapi_add_header("Content-Type: text/xml; charset=\"utf-8\"", sizeof("Content-Type: text/xml; charset=\"utf-8\""), 1);
						ZVAL_STRING(param, service->sdl->source, 1);
						ZVAL_STRING(&readfile, "readfile", 1);
						if (call_user_function(EG(function_table), NULL, &readfile, &readfile_ret, 1, &param  TSRMLS_CC) == FAILURE) {
							php_error(E_ERROR, "Couldn't find WSDL");
						}

						zval_ptr_dtor(&param);
						zval_dtor(&readfile);
						zval_dtor(&readfile_ret);

						SOAP_SERVER_END_CODE();
						return;
					} else {
						php_error(E_ERROR, "WSDL generation is not supported yet");
/*
						sapi_add_header("Content-Type: text/xml; charset=\"utf-8\"", sizeof("Content-Type: text/xml; charset=\"utf-8\""), 1);
						PUTS("<?xml version=\"1.0\" ?>\n<definitions\n");
						PUTS("    xmlns=\"http://schemas.xmlsoap.org/wsdl/\"\n");
						PUTS("    targetNamespace=\"");
						PUTS(service->uri);
						PUTS("\">\n");
						PUTS("</definitions>");
*/
						SOAP_SERVER_END_CODE();
						return;
					}
				}
			}
		}
	}

/* Turn on output buffering... we don't want people print in their methods
 #if PHP_API_VERSION <= 20010901
	if (php_start_ob_buffer(NULL, 0 TSRMLS_CC) != SUCCESS)
 #else
*/
	if (php_start_ob_buffer(NULL, 0, 0 TSRMLS_CC) != SUCCESS) {
/* #endif */
		php_error(E_ERROR,"ob_start failed");
	}

	if (zend_hash_find(&EG(symbol_table), HTTP_RAW_POST_DATA, sizeof(HTTP_RAW_POST_DATA), (void **) &raw_post)!=FAILURE
		&& ((*raw_post)->type==IS_STRING)) {
		int old_error_reporting = EG(error_reporting);
		sdlFunctionPtr function;

		EG(error_reporting) &= ~(E_WARNING|E_NOTICE|E_USER_WARNING|E_USER_NOTICE);

		doc_request = xmlParseMemory(Z_STRVAL_PP(raw_post),Z_STRLEN_PP(raw_post));
		xmlCleanupParser();

		EG(error_reporting) = old_error_reporting;

		if (doc_request == NULL) {
			php_error(E_ERROR, "Bad Request");
		}
		if (xmlGetIntSubset(doc_request) != NULL) {
			xmlNodePtr env = get_node(doc_request->children,"Envelope");
			if (env && env->ns) {
				if (strcmp(env->ns->href,SOAP_1_1_ENV_NAMESPACE) == 0) {
					SOAP_GLOBAL(soap_version) = SOAP_1_1;
				} else if (strcmp(env->ns->href,SOAP_1_2_ENV_NAMESPACE) == 0) {
					SOAP_GLOBAL(soap_version) = SOAP_1_2;
				}
			}
			php_error(E_ERROR,"DTD are not supported by SOAP");
		}

		old_sdl = SOAP_GLOBAL(sdl);
		SOAP_GLOBAL(sdl) = service->sdl;
		old_soap_version = SOAP_GLOBAL(soap_version);
		function = deseralize_function_call(service->sdl, doc_request, service->actor, &function_name, &num_params, &params, &soap_version, &soap_headers TSRMLS_CC);
		xmlFreeDoc(doc_request);

		if (service->type == SOAP_CLASS) {
			soap_obj = NULL;
#if HAVE_PHP_SESSION
			/* If persistent then set soap_obj from from the previous created session (if available) */
			if (service->soap_class.persistance == SOAP_PERSISTENCE_SESSION) {
				zval **tmp_soap;

				if (PS(session_status) != php_session_active &&
				    PS(session_status) != php_session_disabled) {
					php_session_start(TSRMLS_C);
				}

				/* Find the soap object and assign */
				if (zend_hash_find(Z_ARRVAL_P(PS(http_session_vars)), "_bogus_session_name", sizeof("_bogus_session_name"), (void **) &tmp_soap) == SUCCESS) {
					soap_obj = *tmp_soap;
				}
			}
#endif
			/* If new session or something wierd happned */
			if (soap_obj == NULL) {
				zval *tmp_soap;
				char *class_name;
				int class_name_len;

				MAKE_STD_ZVAL(tmp_soap);
				object_init_ex(tmp_soap, service->soap_class.ce);

				/* Call constructor */
				class_name_len = strlen(service->soap_class.ce->name);
				class_name = emalloc(class_name_len+1);
				memcpy(class_name, service->soap_class.ce->name,class_name_len+1);
				if (zend_hash_exists(&Z_OBJCE_P(tmp_soap)->function_table, php_strtolower(class_name, class_name_len), class_name_len+1)) {
					zval c_ret, constructor;

					INIT_ZVAL(c_ret);
					INIT_ZVAL(constructor);

					ZVAL_STRING(&constructor, service->soap_class.ce->name, 1);
					if (call_user_function(NULL, &tmp_soap, &constructor, &c_ret, service->soap_class.argc, service->soap_class.argv TSRMLS_CC) == FAILURE) {
						php_error(E_ERROR, "Error calling constructor");
					}
					zval_dtor(&constructor);
					zval_dtor(&c_ret);
				}
				efree(class_name);
#if HAVE_PHP_SESSION
				/* If session then update session hash with new object */
				if (service->soap_class.persistance == SOAP_PERSISTENCE_SESSION) {
					zval **tmp_soap_pp;
					if (zend_hash_update(Z_ARRVAL_P(PS(http_session_vars)), "_bogus_session_name", sizeof("_bogus_session_name"), &tmp_soap, sizeof(zval *), (void **)&tmp_soap_pp) == SUCCESS) {
						soap_obj = *tmp_soap_pp;
					}
				} else {
					soap_obj = tmp_soap;
				}
#else
				soap_obj = tmp_soap;
#endif

			}
/* 			function_table = &(soap_obj->value.obj.ce->function_table);*/
 			function_table = &((Z_OBJCE_P(soap_obj))->function_table);
		} else {
			if (service->soap_functions.functions_all == TRUE) {
				function_table = EG(function_table);
			} else {
				function_table = service->soap_functions.ft;
			}
		}

		doc_return = NULL;

		/* Process soap headers */
		if (soap_headers != NULL) {
			soapHeader *header = soap_headers;
			while (header != NULL) {
				soapHeader *h = header;

				header = header->next;
				if (h->mustUnderstand && service->sdl && !h->function && !h->hdr) {
					soap_server_fault("MustUnderstand","Header not understood", NULL, NULL TSRMLS_CC);
				}

				fn_name = estrndup(Z_STRVAL(h->function_name),Z_STRLEN(h->function_name));
				if (zend_hash_exists(function_table, php_strtolower(fn_name, Z_STRLEN(h->function_name)), Z_STRLEN(h->function_name) + 1)) {
 					if (service->type == SOAP_CLASS) {
						call_status = call_user_function(NULL, &soap_obj, &h->function_name, &h->retval, h->num_params, h->parameters TSRMLS_CC);
					} else {
						call_status = call_user_function(EG(function_table), NULL, &h->function_name, &h->retval, h->num_params, h->parameters TSRMLS_CC);
					}
					if (call_status != SUCCESS) {
						php_error(E_ERROR, "Function '%s' call failed", Z_STRVAL(function_name));
					}
				} else if (h->mustUnderstand) {
					soap_server_fault("MustUnderstand","Header not understood", NULL, NULL TSRMLS_CC);
				}
				efree(fn_name);
			}
		}

		fn_name = estrndup(Z_STRVAL(function_name),Z_STRLEN(function_name));
		if (zend_hash_exists(function_table, php_strtolower(fn_name, Z_STRLEN(function_name)), Z_STRLEN(function_name) + 1)) {
 			if (service->type == SOAP_CLASS) {
				call_status = call_user_function(NULL, &soap_obj, &function_name, &retval, num_params, params TSRMLS_CC);
				if (service->soap_class.persistance != SOAP_PERSISTENCE_SESSION) {
					zval_ptr_dtor(&soap_obj);
				}
			} else {
				call_status = call_user_function(EG(function_table), NULL, &function_name, &retval, num_params, params TSRMLS_CC);
			}
		} else {
			php_error(E_ERROR, "Function '%s' doesn't exist", Z_STRVAL(function_name));
		}
		efree(fn_name);

		if (call_status == SUCCESS) {
			char *response_name;

			if (function && function->responseName) {
				response_name = estrdup(function->responseName);
			} else {
				response_name = emalloc(Z_STRLEN(function_name) + strlen("Response") + 1);
				sprintf(response_name,"%sResponse",Z_STRVAL(function_name));
			}
			SOAP_GLOBAL(overrides) = service->mapping;
			doc_return = seralize_response_call(function, response_name, service->uri, &retval, soap_headers, soap_version TSRMLS_CC);
			SOAP_GLOBAL(overrides) = NULL;
			efree(response_name);
		} else {
			php_error(E_ERROR, "Function '%s' call failed", Z_STRVAL(function_name));
		}

		/* Free soap headers */
		while (soap_headers != NULL) {
			soapHeader *h = soap_headers;
			int i;

			soap_headers = soap_headers->next;
			i = h->num_params;
			while (i > 0) {
				zval_ptr_dtor(&h->parameters[--i]);
			}
			efree(h->parameters);
			zval_dtor(&h->function_name);
			zval_dtor(&h->retval);
			efree(h);
		}

		SOAP_GLOBAL(soap_version) = old_soap_version;
		SOAP_GLOBAL(sdl) = old_sdl;

		/* Flush buffer */
		php_end_ob_buffer(0, 0 TSRMLS_CC);

		/* xmlDocDumpMemoryEnc(doc_return, &buf, &size, XML_CHAR_ENCODING_UTF8); */
		xmlDocDumpMemory(doc_return, &buf, &size);

		if (size == 0) {
			php_error(E_ERROR, "Dump memory failed");
		}

		sprintf(cont_len, "Content-Length: %d", size);
		sapi_add_header(cont_len, strlen(cont_len) + 1, 1);
		if (soap_version == SOAP_1_2) {
			sapi_add_header("Content-Type: application/soap+xml; charset=\"utf-8\"", sizeof("Content-Type: application/soap+xml; charset=\"utf-8\""), 1);
		} else {
			sapi_add_header("Content-Type: text/xml; charset=\"utf-8\"", sizeof("Content-Type: text/xml; charset=\"utf-8\""), 1);
		}

		/* Free Memory */
		if (num_params > 0) {
			for (i = 0; i < num_params;i++) {
				zval_ptr_dtor(&params[i]);
			}
			efree(params);
		}

		zval_dtor(&function_name);
		xmlFreeDoc(doc_return);

		php_write(buf, size TSRMLS_CC);
		xmlFree(buf);
	} else {
		if (!zend_ini_long("always_populate_raw_post_data", sizeof("always_populate_raw_post_data"), 0)) {
			php_error(E_ERROR, "PHP-SOAP requires 'always_populate_raw_post_data' to be on please check your php.ini file");
		}

		php_error(E_ERROR, "Can't find HTTP_RAW_POST_DATA");
	}

	zval_dtor(&retval);

	SOAP_SERVER_END_CODE();
}

PHP_METHOD(soapserver, fault)
{
	char *code, *string, *actor=NULL;
	int code_len, string_len, actor_len;
	zval* details = NULL;

	SOAP_SERVER_BEGIN_CODE();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|sz",
	    &code, &code_len, &string, &string_len, &actor, &actor_len, &details) == FAILURE) {
		php_error(E_ERROR, "Invalid parameters passed to soapserver:fault");
	}
	soap_server_fault(code, string, actor, details TSRMLS_CC);
	SOAP_SERVER_END_CODE();
}

static void soap_server_fault(char* code, char* string, char *actor, zval* details TSRMLS_DC)
{
	int soap_version;
	xmlChar *buf, cont_len[30];
	int size;
	zval ret;
	xmlDocPtr doc_return;

	soap_version = SOAP_GLOBAL(soap_version);

	INIT_ZVAL(ret);

	set_soap_fault(&ret, code, string, actor, details TSRMLS_CC);

	doc_return = seralize_response_call(NULL, NULL, NULL, &ret, NULL, soap_version TSRMLS_CC);

	xmlDocDumpMemory(doc_return, &buf, &size);

	/*
	   Want to return HTTP 500 but apache wants to over write
	   our fault code with their own handling... Figure this out later
	*/
	sapi_add_header("HTTP/1.1 500 Internal Service Error", sizeof("HTTP/1.1 500 Internal Service Error"), 1);
	sprintf(cont_len,"Content-Length: %d", size);
	sapi_add_header(cont_len, strlen(cont_len) + 1, 1);
	if (soap_version == SOAP_1_2) {
		sapi_add_header("Content-Type: application/soap+xml; charset=\"utf-8\"", sizeof("Content-Type: application/soap+xml; charset=\"utf-8\""), 1);
	} else {
		sapi_add_header("Content-Type: text/xml; charset=\"utf-8\"", sizeof("Content-Type: text/xml; charset=\"utf-8\""), 1);
	}
	php_write(buf, size TSRMLS_CC);

	xmlFreeDoc(doc_return);
	xmlFree(buf);
	zend_bailout();
}

static void soap_error_handler(int error_num, const char *error_filename, const uint error_lineno, const char *format, va_list args)
{
	TSRMLS_FETCH();

	if (!SOAP_GLOBAL(use_soap_error_handler)) {
		old_error_handler(error_num, error_filename, error_lineno, format, args);
		return;
	}

	/*
	  Trap all errors
	  What do do with these warnings
	    E_WARNING, E_NOTICE, E_CORE_WARNING, E_COMPILE_WARNING, E_USER_WARNING, E_USER_NOTICE
	 */
	if (error_num == E_USER_ERROR || error_num == E_COMPILE_ERROR || error_num == E_CORE_ERROR ||
		error_num == E_ERROR || error_num == E_PARSE) {

		char buffer[1024];
		int buffer_len;
		zval outbuf, outbuflen;

		INIT_ZVAL(outbuf);
		INIT_ZVAL(outbuflen);

		buffer_len = vsnprintf(buffer, sizeof(buffer)-1, format, args);
		buffer[sizeof(buffer)-1]=0;
		if (buffer_len > sizeof(buffer) - 1 || buffer_len < 0) {
			buffer_len = sizeof(buffer) - 1;
		}

		/* Get output buffer and send as fault detials */
		if (php_ob_get_length(&outbuflen TSRMLS_CC) != FAILURE && Z_LVAL(outbuflen) != 0) {
			php_ob_get_buffer(&outbuf TSRMLS_CC);
		}
		php_end_ob_buffer(0, 0 TSRMLS_CC);

		soap_server_fault("Server", buffer, NULL, &outbuf TSRMLS_CC);
	}
}

PHP_FUNCTION(use_soap_error_handler)
{
	zend_bool handler = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &handler) == SUCCESS) {
		SOAP_GLOBAL(use_soap_error_handler) = handler;
	}
}


/* SoapObject functions */
/*
	SoapObject($wsdl, $version=SOAP_1_1)
	SoapObject($location, $uri, $style=SOAP_RPC, $use=SOAP_ENCODED, $version=SOAP_1_1)
*/
PHP_METHOD(soapobject, soapobject)
{
	char *location;
	int location_len;
	zval *arg2 = NULL;
	long use   = SOAP_RPC;
	long style = SOAP_ENCODED;
	long version = SOAP_1_1;
	long old_soap_version	;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|zlll", &location, &location_len, &arg2, &style, &use, &version) == SUCCESS) {
		if (arg2 == NULL || Z_TYPE_P(arg2) == IS_LONG) {
			/* SoapObject($wsdl, $version=SOAP_1_1) */
			sdlPtr sdl;
			int ret;

			if (arg2 != NULL) {
				version = Z_LVAL_P(arg2);
			}
			if (version == SOAP_1_1 || version == SOAP_1_2) {
				add_property_long(this_ptr, "_soap_version", version);
			} else {
				php_error(E_ERROR,"Can't create SoapObject. Wrong 'version' parameter.");
			}
			old_soap_version = SOAP_GLOBAL(soap_version);
			SOAP_GLOBAL(soap_version) = version;

			sdl = get_sdl(location);
			ret = zend_list_insert(sdl, le_sdl);

			add_property_resource(this_ptr, "sdl", ret);
			zend_list_addref(ret);

			SOAP_GLOBAL(soap_version) = old_soap_version;

		} else if (arg2 != NULL && Z_TYPE_P(arg2) == IS_STRING) {
			/* SoapObject($location, $uri, $style=SOAP_RPC, $use=SOAP_ENCODED, $version=SOAP_1_1) */
			add_property_stringl(this_ptr, "location", location, location_len, 1);
			add_property_stringl(this_ptr, "uri", Z_STRVAL_P(arg2), Z_STRLEN_P(arg2), 1);
			if (style == SOAP_RPC || style == SOAP_DOCUMENT) {
				add_property_long(this_ptr, "style", style);
			} else {
				php_error(E_ERROR,"Can't create SoapObject. Wrong 'style' parameter.");
			}
			if (use == SOAP_ENCODED || use == SOAP_LITERAL) {
				add_property_long(this_ptr, "use", use);
			} else {
				php_error(E_ERROR,"Can't create SoapObject. Wrong 'use' parameter.");
			}
			if (version == SOAP_1_1 || version == SOAP_1_2) {
				add_property_long(this_ptr, "_soap_version", version);
			} else {
				php_error(E_ERROR,"Can't create SoapObject. Wrong 'version' parameter.");
			}
		} else {
			php_error(E_ERROR,"Can't create SoapObject. Wrong parameters.");
		}
	}
}

PHP_METHOD(soapobject, __trace)
{
	int level;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &level)) {
		php_error(E_ERROR, "Invalid arguments to SoapObject->__trace");
	}

	add_property_long(this_ptr, "trace", level);
	RETURN_TRUE;
}

static void do_soap_call(zval* thisObj,
                         char* function,
                         int function_len,
                         int arg_count,
                         zval** real_args,
                         zval* return_value,
                         char* soap_action,
                         char* call_uri,
                         HashTable* soap_headers,
                         zval* output_headers
                         TSRMLS_DC)
{
	zval **tmp;
	zval **trace;
 	sdlPtr sdl = NULL;
 	sdlPtr old_sdl = NULL;
 	sdlFunctionPtr fn;
	xmlDocPtr request = NULL;
	char *buffer;
	int len;
	int ret = FALSE;
	int bailout = FALSE;
	int soap_version, old_soap_version;

	if (zend_hash_find(Z_OBJPROP_P(thisObj), "trace", sizeof("trace"), (void **) &trace) == SUCCESS
		&& Z_LVAL_PP(trace) > 0) {
		zend_hash_del(Z_OBJPROP_P(thisObj), "__last_request", sizeof("__last_request"));
		zend_hash_del(Z_OBJPROP_P(thisObj), "__last_response", sizeof("__last_response"));
	}
	if (zend_hash_find(Z_OBJPROP_P(thisObj), "_soap_version", sizeof("_soap_version"), (void **) &tmp) == SUCCESS
		&& Z_LVAL_PP(tmp) == SOAP_1_2) {
		soap_version = SOAP_1_2;
	} else {
		soap_version = SOAP_1_1;
	}

	if (FIND_SDL_PROPERTY(thisObj,tmp) != FAILURE) {
		FETCH_SDL_RES(sdl,tmp);
	}

 	clear_soap_fault(thisObj TSRMLS_CC);

	old_soap_version = SOAP_GLOBAL(soap_version);
zend_try {
	SOAP_GLOBAL(soap_version) = soap_version;
	old_sdl = SOAP_GLOBAL(sdl);
	SOAP_GLOBAL(sdl) = sdl;
 	if (sdl != NULL) {
 		fn = get_function(sdl, function);
 		if (fn != NULL) {
			sdlBindingPtr binding = fn->binding;
 			if (binding->bindingType == BINDING_SOAP) {
 				sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)fn->bindingAttributes;
 				request = seralize_function_call(thisObj, fn, NULL, fnb->input.ns, real_args, arg_count, soap_version, soap_headers TSRMLS_CC);
 				ret = send_http_soap_request(thisObj, request, binding->location, fnb->soapAction, soap_version TSRMLS_CC);
 			}	else {
 				request = seralize_function_call(thisObj, fn, NULL, sdl->target_ns, real_args, arg_count, soap_version, soap_headers TSRMLS_CC);
 				ret = send_http_soap_request(thisObj, request, binding->location, NULL, soap_version TSRMLS_CC);
 			}

 			xmlFreeDoc(request);

 			if (ret) {
				ret = get_http_soap_response(thisObj, &buffer, &len TSRMLS_CC);
	 			if (ret) {
					parse_packet_soap(thisObj, buffer, len, fn, NULL, return_value, output_headers TSRMLS_CC);
					efree(buffer);
				}
			}
 		} else {
 			smart_str error = {0};
 			smart_str_appends(&error,"Function (\"");
 			smart_str_appends(&error,function);
 			smart_str_appends(&error,"\") is not a valid method for this service");
 			smart_str_0(&error);
			add_soap_fault(thisObj, "Client", error.c, NULL, NULL TSRMLS_CC);
			smart_str_free(&error);
		}
	} else {
		zval **uri, **location;
		smart_str action = {0};

		if (zend_hash_find(Z_OBJPROP_P(thisObj), "uri", sizeof("uri"), (void *)&uri) == FAILURE) {
			add_soap_fault(thisObj, "Client", "Error finding \"uri\" property", NULL, NULL TSRMLS_CC);
		} else if (zend_hash_find(Z_OBJPROP_P(thisObj), "location", sizeof("location"),(void **) &location) == FAILURE) {
			add_soap_fault(thisObj, "Client", "Error could not find \"location\" property", NULL, NULL TSRMLS_CC);
		} else {
			if (call_uri == NULL) {
				call_uri = Z_STRVAL_PP(uri);
			}
	 		request = seralize_function_call(thisObj, NULL, function, call_uri, real_args, arg_count, soap_version, soap_headers TSRMLS_CC);

	 		if (soap_action == NULL) {
				smart_str_appends(&action, call_uri);
				smart_str_appendc(&action, '#');
				smart_str_appends(&action, function);
			} else {
				smart_str_appends(&action, soap_action);
			}
			smart_str_0(&action);

			ret = send_http_soap_request(thisObj, request, Z_STRVAL_PP(location), action.c, soap_version TSRMLS_CC);

	 		smart_str_free(&action);
			xmlFreeDoc(request);

			if (ret) {
 				ret = get_http_soap_response(thisObj, &buffer, &len TSRMLS_CC);
 				if (ret) {
					ret = parse_packet_soap(thisObj, buffer, len, NULL, function, return_value, output_headers TSRMLS_CC);
					efree(buffer);
				}
			}
		}
 	}
} zend_catch {
	ret = FALSE;
	bailout = TRUE;
} zend_end_try();
	if (bailout) {
		zend_bailout();
	}
	if (!ret) {
		zval** fault;
		if (zend_hash_find(Z_OBJPROP_P(thisObj), "__soap_fault", sizeof("__soap_fault"), (void **) &fault) == SUCCESS) {
			*return_value = **fault;
			zval_copy_ctor(return_value);
		} else {
			*return_value = *add_soap_fault(thisObj, "Client", "Unknown Error", NULL, NULL TSRMLS_CC);
			zval_copy_ctor(return_value);
		}
	} else {
		zval** fault;
		if (zend_hash_find(Z_OBJPROP_P(thisObj), "__soap_fault", sizeof("__soap_fault"), (void **) &fault) == SUCCESS) {
			*return_value = **fault;
			zval_copy_ctor(return_value);
		}
	}
	SOAP_GLOBAL(soap_version) = old_soap_version;
	SOAP_GLOBAL(sdl) = old_sdl;
}

PHP_METHOD(soapobject, __login)
{
  char *login_name;
  char *login_pass;
  int login_name_len;
  int login_pass_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
	    &login_name, &login_name_len, &login_pass, &login_pass_len) == FAILURE) {
		return;
	}
	add_property_stringl(this_ptr,"_login",login_name,login_name_len, 1);
	add_property_stringl(this_ptr,"_password",login_pass,login_pass_len, 1);
}

PHP_METHOD(soapobject, __useproxy)
{
  char *proxy_host;
  char *proxy_name = NULL;
  char *proxy_pass = NULL;
  int proxy_host_len;
  int proxy_name_len;
  int proxy_pass_len;
  long proxy_port;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl|ss",
	    &proxy_host, &proxy_host_len, &proxy_port, &proxy_name, &proxy_name_len, &proxy_pass, &proxy_pass_len) == FAILURE) {
		return;
	}
	add_property_stringl(this_ptr,"_proxy_host",proxy_host,proxy_host_len, 1);
	add_property_long(this_ptr,"_proxy_port",proxy_port);
	if (proxy_name) {
		add_property_stringl(this_ptr,"_proxy_login",proxy_name,proxy_name_len, 1);
	}
	if (proxy_pass) {
		add_property_stringl(this_ptr,"_proxy_password",proxy_pass,proxy_pass_len, 1);
	}
	zend_hash_del(Z_OBJPROP_P(this_ptr), "httpsocket", sizeof("httpsocket"));
	zend_hash_del(Z_OBJPROP_P(this_ptr), "_use_proxy", sizeof("_use_proxy"));
}

PHP_METHOD(soapobject, __call)
{
	char *function, *soap_action = NULL, *uri = NULL;
	int function_len, soap_action_len, uri_len, i = 0;
	HashTable* soap_headers = NULL;
	zval *headers = NULL;
	zval *output_headers = NULL;
	zval *args;
	zval **real_args;
	zval **param;
	int arg_count;

	HashPosition pos;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa|sszz",
		&function, &function_len, &args, &soap_action, &soap_action_len, &uri, &uri_len, &headers, &output_headers) == FAILURE) {
		php_error(E_ERROR, "Invalid arguments to SoapObject->__call");
	}

	if (headers == NULL || Z_TYPE_P(headers) == IS_NULL) {
	} else if (Z_TYPE_P(headers) == IS_ARRAY) {
		zval** tmp;

		soap_headers = Z_ARRVAL_P(headers);
		zend_hash_internal_pointer_reset(soap_headers);
		while (zend_hash_get_current_data(soap_headers, (void**)&tmp) == SUCCESS) {
			if (Z_TYPE_PP(tmp) != IS_OBJECT ||
			    Z_OBJCE_PP(tmp) != soap_header_class_entry) {
				php_error(E_ERROR, "Invalid arguments to SoapObject->__call");
			}
			zend_hash_move_forward(soap_headers);
		}
	} else if (Z_TYPE_P(headers) == IS_OBJECT &&
	           Z_OBJCE_P(headers) == soap_header_class_entry) {
	  soap_headers = emalloc(sizeof(HashTable));
		zend_hash_init(soap_headers, 0, NULL, NULL/*ZVAL_PTR_DTOR*/, 0);
		zend_hash_next_index_insert(soap_headers, &headers, sizeof(zval*), NULL);
	  headers = NULL;
	} else{
		php_error(E_ERROR, "Invalid arguments to SoapObject->__call");
	}

	arg_count = zend_hash_num_elements(Z_ARRVAL_P(args));

	real_args = emalloc(sizeof(zval *) * arg_count);
	for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(args), &pos);
		zend_hash_get_current_data_ex(Z_ARRVAL_P(args), (void **) &param, &pos) == SUCCESS;
		zend_hash_move_forward_ex(Z_ARRVAL_P(args), &pos)) {
			/*zval_add_ref(param);*/
			real_args[i++] = *param;
	}
	if (output_headers) {
		array_init(output_headers);
	}
	do_soap_call(this_ptr, function, function_len, arg_count, real_args, return_value, soap_action, uri, soap_headers, output_headers TSRMLS_CC);
	efree(real_args);

	if (soap_headers && ! headers) {
		zend_hash_destroy(soap_headers);
		efree(soap_headers);
	}
}

PHP_METHOD(soapobject, __isfault)
{
	if (zend_hash_exists(Z_OBJPROP_P(this_ptr), "__soap_fault", sizeof("__soap_fault"))) {
		RETURN_TRUE
	} else {
		RETURN_FALSE
	}
}

PHP_METHOD(soapobject, __getfault)
{
	zval **tmp;

	if (zend_hash_find(Z_OBJPROP_P(this_ptr), "__soap_fault", sizeof("__soap_fault"), (void **)&tmp) == SUCCESS) {
		*return_value = *(*tmp);
		zval_copy_ctor(return_value);
		return;
	}
	RETURN_NULL();
}

PHP_METHOD(soapobject, __getfunctions)
{
	sdlPtr sdl;
	HashPosition pos;

	FETCH_THIS_SDL(sdl);

	if (sdl) {
		smart_str buf = {0};
		sdlFunctionPtr *function;

		array_init(return_value);
 		zend_hash_internal_pointer_reset_ex(&sdl->functions, &pos);
		while (zend_hash_get_current_data_ex(&sdl->functions, (void **)&function, &pos) != FAILURE) {
			function_to_string((*function), &buf);
			add_next_index_stringl(return_value, buf.c, buf.len, 1);
			smart_str_free(&buf);
			zend_hash_move_forward_ex(&sdl->functions, &pos);
		}
	}
}

PHP_METHOD(soapobject, __gettypes)
{
	sdlPtr sdl;
	HashPosition pos;

	FETCH_THIS_SDL(sdl);

	if (sdl) {
		sdlTypePtr *type;
		smart_str buf = {0};

		array_init(return_value);
		if (sdl->types) {
			zend_hash_internal_pointer_reset_ex(sdl->types, &pos);
			while (zend_hash_get_current_data_ex(sdl->types, (void **)&type, &pos) != FAILURE) {
				type_to_string((*type), &buf, 0);
				add_next_index_stringl(return_value, buf.c, buf.len, 1);
				zend_hash_move_forward_ex(sdl->types, &pos);
				smart_str_free(&buf);
			}
		}
	}
}

PHP_METHOD(soapobject, __getlastrequest)
{
	zval **tmp;

	if (zend_hash_find(Z_OBJPROP_P(this_ptr), "__last_request", sizeof("__last_request"), (void **)&tmp) == SUCCESS) {
		RETURN_STRINGL(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
	}
	RETURN_NULL();
}

PHP_METHOD(soapobject, __getlastresponse)
{
	zval **tmp;

	if (zend_hash_find(Z_OBJPROP_P(this_ptr), "__last_response", sizeof("__last_response"), (void **)&tmp) == SUCCESS) {
		RETURN_STRINGL(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
	}
	RETURN_NULL();
}

#ifndef ZEND_ENGINE_2
static void soap_call_function_handler(INTERNAL_FUNCTION_PARAMETERS, zend_property_reference *property_reference)
{
	pval *object = property_reference->object;
	zend_overloaded_element *function_name = (zend_overloaded_element *)property_reference->elements_list->tail->data;
	char *function = Z_STRVAL(function_name->element);
	zend_function *builtin_function;

	/*
	   Find if the function being called is already defined...
	  ( IMHO: zend should handle this functionality )
	*/
	if (zend_hash_find(&Z_OBJCE_P(this_ptr)->function_table, function, Z_STRLEN(function_name->element) + 1, (void **) &builtin_function) == SUCCESS) {
		builtin_function->internal_function.handler(INTERNAL_FUNCTION_PARAM_PASSTHRU);
	} else {
		int arg_count = ZEND_NUM_ARGS();
		zval **arguments = (zval **) emalloc(sizeof(zval *) * arg_count);

		zend_get_parameters_array(ht, arg_count, arguments);
		do_soap_call(this_ptr, function, Z_STRLEN(function_name->element) + 1, arg_count, arguments, return_value, NULL, NULL, NULL, NULL TSRMLS_CC);
		efree(arguments);
	}
	zval_dtor(&function_name->element);
}
#endif

static void clear_soap_fault(zval *obj TSRMLS_DC)
{
	if (obj != NULL && obj->type == IS_OBJECT) {
		zend_hash_del(Z_OBJPROP_P(obj), "__soap_fault", sizeof("__soap_fault"));
	}
}

zval* add_soap_fault(zval *obj, char *fault_code, char *fault_string, char *fault_actor, zval *fault_detail TSRMLS_DC)
{
	zval *fault;
	MAKE_STD_ZVAL(fault);
	set_soap_fault(fault, fault_code, fault_string, fault_actor, fault_detail TSRMLS_CC);
#ifdef ZEND_ENGINE_2
	fault->refcount--;  /*FIXME*/
#endif
	add_property_zval(obj, "__soap_fault", fault);
	return fault;
}

static void set_soap_fault(zval *obj, char *fault_code, char *fault_string, char *fault_actor, zval *fault_detail TSRMLS_DC)
{
	if (Z_TYPE_P(obj) != IS_OBJECT) {
		object_init_ex(obj, soap_fault_class_entry);
	}
	if (fault_string != NULL) {
		add_property_string(obj, "faultstring", fault_string, 1);
	}
	if (fault_code != NULL) {
		int soap_version = SOAP_GLOBAL(soap_version);
		smart_str code = {0};
		if (soap_version == SOAP_1_1) {
			if (strcmp(fault_code,"Client") == 0 ||
			    strcmp(fault_code,"Server") == 0 ||
			    strcmp(fault_code,"VersionMismatch") == 0 ||
			    strcmp(fault_code,"MustUnderstand") == 0) {
				smart_str_appendl(&code, SOAP_1_1_ENV_NS_PREFIX, sizeof(SOAP_1_1_ENV_NS_PREFIX)-1);
				smart_str_appendc(&code, ':');
			}
			smart_str_appends(&code,fault_code);
		} else if (soap_version == SOAP_1_2) {
			if (strcmp(fault_code,"Client") == 0) {
				smart_str_appendl(&code, SOAP_1_2_ENV_NS_PREFIX, sizeof(SOAP_1_2_ENV_NS_PREFIX)-1);
				smart_str_appendc(&code, ':');
				smart_str_appendl(&code,"Sender",sizeof("Sender")-1);
			} else if (strcmp(fault_code,"Server") == 0) {
				smart_str_appendl(&code, SOAP_1_2_ENV_NS_PREFIX, sizeof(SOAP_1_2_ENV_NS_PREFIX)-1);
				smart_str_appendc(&code, ':');
				smart_str_appendl(&code,"Receiver",sizeof("Receiver")-1);
			} else if (strcmp(fault_code,"VersionMismatch") == 0 ||
			           strcmp(fault_code,"MustUnderstand") == 0 ||
			           strcmp(fault_code,"DataEncodingUnknown") == 0) {
				smart_str_appendl(&code, SOAP_1_2_ENV_NS_PREFIX, sizeof(SOAP_1_2_ENV_NS_PREFIX)-1);
				smart_str_appendc(&code, ':');
				smart_str_appends(&code,fault_code);
			} else {
				smart_str_appends(&code,fault_code);
			}
		}
		smart_str_0(&code);
		add_property_string(obj, "faultcode", code.c, 1);
		smart_str_free(&code);
	}
	if (fault_actor != NULL) {
		add_property_string(obj, "faultactor", fault_actor, 1);
	}
	if (fault_detail != NULL) {
		add_property_zval(obj, "detail", fault_detail);
	}
}

static void deseralize_parameters(xmlNodePtr params, sdlFunctionPtr function, int *num_params, zval ***parameters)
{
	int cur_param = 0,num_of_params = 0;
	zval **tmp_parameters = NULL;

	if (function != NULL) {
		sdlParamPtr *param;
		xmlNodePtr val;
		int	use_names = 0;

		if (function->requestParameters == NULL) {
			return;
		}
		num_of_params = zend_hash_num_elements(function->requestParameters);
		zend_hash_internal_pointer_reset(function->requestParameters);
		while (zend_hash_get_current_data(function->requestParameters, (void **)&param) == SUCCESS) {
			if (get_node(params, (*param)->paramName) != NULL) {
				use_names = 1;
			}
			zend_hash_move_forward(function->requestParameters);
		}
		if (use_names) {
			tmp_parameters = emalloc(num_of_params * sizeof(zval *));
			zend_hash_internal_pointer_reset(function->requestParameters);
			while (zend_hash_get_current_data(function->requestParameters, (void **)&param) == SUCCESS) {
				val = get_node(params, (*param)->paramName);
				if (!val) {
					/* TODO: may be "nil" is not OK? */
					MAKE_STD_ZVAL(tmp_parameters[cur_param]);
					ZVAL_NULL(tmp_parameters[cur_param]);
				} else {
					tmp_parameters[cur_param] = master_to_zval((*param)->encode, val);
				}
				cur_param++;

				zend_hash_move_forward(function->requestParameters);
			}
			(*parameters) = tmp_parameters;
			(*num_params) = num_of_params;
			return;
		}
	}
	if (params) {
		xmlNodePtr trav;

		num_of_params = 0;
		trav = params;
		while (trav != NULL) {
			if (trav->type == XML_ELEMENT_NODE) {
				num_of_params++;
			}
			trav = trav->next;
		}
		if (num_of_params > 0) {
			tmp_parameters = emalloc(num_of_params * sizeof(zval *));

			trav = params;
			while (trav != 0 && cur_param < num_of_params) {
				if (trav->type == XML_ELEMENT_NODE) {
					encodePtr enc;
					sdlParamPtr *param = NULL;
					if (function != NULL &&
					    zend_hash_index_find(function->requestParameters, cur_param, (void **)&param) == FAILURE) {
						php_error(E_ERROR, "Error cannot find parameter");
					}
					if (param == NULL) {
						enc = NULL;
					} else {
						enc = (*param)->encode;
					}
					tmp_parameters[cur_param] = master_to_zval(enc, trav);
					cur_param++;
				}
				trav = trav->next;
			}
		}
	}
	if (num_of_params > cur_param) {
		TSRMLS_FETCH();
		soap_server_fault("Client","Missing parameter", NULL, NULL TSRMLS_CC);
	}
	(*parameters) = tmp_parameters;
	(*num_params) = num_of_params;
}

static sdlFunctionPtr find_function(sdlPtr sdl, xmlNodePtr func, zval* function_name)
{
	sdlFunctionPtr function;

	function = get_function(sdl, func->name);
	if (function && function->binding && function->binding->bindingType == BINDING_SOAP) {
		sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;
		if (fnb->style == SOAP_DOCUMENT) {
			function = NULL;
		}
	}
	if (sdl != NULL && function == NULL) {
		function = get_doc_function(sdl, func);
	}

	INIT_ZVAL(*function_name);
	if (function != NULL) {
		ZVAL_STRING(function_name, (char *)function->functionName, 1);
	} else{
		ZVAL_STRING(function_name, (char *)func->name, 1);
	}

	return function;
}

static sdlFunctionPtr deseralize_function_call(sdlPtr sdl, xmlDocPtr request, char* actor, zval *function_name, int *num_params, zval ***parameters, int *version, soapHeader **headers TSRMLS_DC)
{
	char* envelope_ns = NULL;
	xmlNodePtr trav,env,head,body,func;
	xmlAttrPtr attr;
	sdlFunctionPtr function;

	ZVAL_EMPTY_STRING(function_name);

	/* Get <Envelope> element */
	env = NULL;
	trav = request->children;
	while (trav != NULL) {
		if (trav->type == XML_ELEMENT_NODE) {
			if (env == NULL && node_is_equal_ex(trav,"Envelope",SOAP_1_1_ENV_NAMESPACE)) {
				env = trav;
				*version = SOAP_1_1;
				envelope_ns = SOAP_1_1_ENV_NAMESPACE;
				SOAP_GLOBAL(soap_version) = SOAP_1_1;
			} else if (env == NULL && node_is_equal_ex(trav,"Envelope",SOAP_1_2_ENV_NAMESPACE)) {
				env = trav;
				*version = SOAP_1_2;
				envelope_ns = SOAP_1_2_ENV_NAMESPACE;
				SOAP_GLOBAL(soap_version) = SOAP_1_2;
			} else {
				soap_server_fault("VersionMismatch","Wrong Version", NULL, NULL TSRMLS_CC);
			}
		}
		trav = trav->next;
	}
	if (env == NULL) {
		php_error(E_ERROR,"looks like we got XML without \"Envelope\" element");
	}

	attr = env->properties;
	while (attr != NULL) {
		if (attr->ns == NULL) {
			php_error(E_ERROR,"A SOAP Envelope element cannot have non Namespace qualified attributes");
		} else if (attr_is_equal_ex(attr,"encodingStyle",SOAP_1_2_ENV_NAMESPACE)) {
			if (*version == SOAP_1_2) {
				php_error(E_ERROR,"encodingStyle cannot be specified on the Envelope");
			} else if (strcmp(attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
				php_error(E_ERROR,"Unknown data encoding style");
			}
		}
		attr = attr->next;
	}

	/* Get <Header> element */
	head = NULL;
	trav = env->children;
	while (trav != NULL && trav->type != XML_ELEMENT_NODE) {
		trav = trav->next;
	}
	if (trav != NULL && node_is_equal_ex(trav,"Header",envelope_ns)) {
		head = trav;
		trav = trav->next;
	}

	/* Get <Body> element */
	body = NULL;
	while (trav != NULL && trav->type != XML_ELEMENT_NODE) {
		trav = trav->next;
	}
	if (trav != NULL && node_is_equal_ex(trav,"Body",envelope_ns)) {
		body = trav;
		trav = trav->next;
	}
	while (trav != NULL && trav->type != XML_ELEMENT_NODE) {
		trav = trav->next;
	}
	if (body == NULL) {
		php_error(E_ERROR,"Body must be present in a SOAP envelope");
	}
	attr = body->properties;
	while (attr != NULL) {
		if (attr->ns == NULL) {
			if (*version == SOAP_1_2) {
				php_error(E_ERROR,"A SOAP Body element cannot have non Namespace qualified attributes");
			}
		} else if (attr_is_equal_ex(attr,"encodingStyle",SOAP_1_2_ENV_NAMESPACE)) {
			if (*version == SOAP_1_2) {
				php_error(E_ERROR,"encodingStyle cannot be specified on the Body");
			} else if (strcmp(attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
				php_error(E_ERROR,"Unknown data encoding style");
			}
		}
		attr = attr->next;
	}

	if (trav != NULL && *version == SOAP_1_2) {
		php_error(E_ERROR,"A SOAP 1.2 envelope can contain only Header and Body");
	}

	func = NULL;
	trav = body->children;
	while (trav != NULL) {
		if (trav->type == XML_ELEMENT_NODE) {
			if (func != NULL) {
				php_error(E_ERROR,"looks like we got \"Body\" with several functions call");
			}
			func = trav;
		}
		trav = trav->next;
	}
	if (func == NULL) {
		function = get_doc_function(sdl, NULL);
		if (function != NULL) {
			INIT_ZVAL(*function_name);
			ZVAL_STRING(function_name, (char *)function->functionName, 1);
		} else {
			php_error(E_ERROR,"looks like we got \"Body\" without function call");
		}
	} else {
		if (*version == SOAP_1_1) {
			attr = get_attribute_ex(func->properties,"encodingStyle",SOAP_1_1_ENV_NAMESPACE);
			if (attr && strcmp(attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
				soap_server_fault("Client","Unknown Data Encoding Style", NULL, NULL TSRMLS_CC);
			}
		} else {
			attr = get_attribute_ex(func->properties,"encodingStyle",SOAP_1_2_ENV_NAMESPACE);
			if (attr && strcmp(attr->children->content,SOAP_1_2_ENC_NAMESPACE) != 0) {
				soap_server_fault("DataEncodingUnknown","Unknown Data Encoding Style", NULL, NULL TSRMLS_CC);
			}
		}
		function = find_function(sdl, func, function_name);
		if (sdl != NULL && function == NULL) {
			if (*version == SOAP_1_2) {
				soap_server_fault("rpc:ProcedureNotPresent","Procedure not present", NULL, NULL TSRMLS_CC);
			} else {
				php_error(E_ERROR, "Procedure '%s' not present", func->name);
			}
		}
	}

	*headers = NULL;
	if (head) {
		soapHeader *h, *last = NULL;

		attr = head->properties;
		while (attr != NULL) {
			if (attr->ns == NULL) {
				php_error(E_ERROR,"A SOAP Header element cannot have non Namespace qualified attributes");
			} else if (attr_is_equal_ex(attr,"encodingStyle",SOAP_1_2_ENV_NAMESPACE)) {
				if (*version == SOAP_1_2) {
					php_error(E_ERROR,"encodingStyle cannot be specified on the Header");
				} else if (strcmp(attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
					php_error(E_ERROR,"Unknown data encoding style");
				}
			}
			attr = attr->next;
		}
		trav = head->children;
		while (trav != NULL) {
			if (trav->type == XML_ELEMENT_NODE) {
				xmlNodePtr hdr_func = trav;
				xmlAttrPtr attr;
				int mustUnderstand = 0;

				if (*version == SOAP_1_1) {
					attr = get_attribute_ex(hdr_func->properties,"encodingStyle",SOAP_1_1_ENV_NAMESPACE);
					if (attr && strcmp(attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
						soap_server_fault("Client","Unknown Data Encoding Style", NULL, NULL TSRMLS_CC);
					}
					attr = get_attribute_ex(hdr_func->properties,"actor",envelope_ns);
					if (attr != NULL) {
						if (strcmp(attr->children->content,"http://schemas.xmlsoap.org/soap/actor/next") != 0 &&
						    (actor == NULL || strcmp(attr->children->content,actor) != 0)) {
						  goto ignore_header;
						}
					}
				} else if (*version == SOAP_1_2) {
					attr = get_attribute_ex(hdr_func->properties,"encodingStyle",SOAP_1_2_ENV_NAMESPACE);
					if (attr && strcmp(attr->children->content,SOAP_1_2_ENC_NAMESPACE) != 0) {
						soap_server_fault("DataEncodingUnknown","Unknown Data Encoding Style", NULL, NULL TSRMLS_CC);
					}
					attr = get_attribute_ex(hdr_func->properties,"role",envelope_ns);
					if (attr != NULL) {
						if (strcmp(attr->children->content,"http://www.w3.org/2003/05/soap-envelope/role/ultimateReceiver") != 0 &&
						    strcmp(attr->children->content,"http://www.w3.org/2003/05/soap-envelope/role/next") != 0 &&
						    (actor == NULL || strcmp(attr->children->content,actor) != 0)) {
						  goto ignore_header;
						}
					}
				}
				attr = get_attribute_ex(hdr_func->properties,"mustUnderstand",envelope_ns);
				if (attr) {
					if (strcmp(attr->children->content,"1") == 0 ||
					    strcmp(attr->children->content,"true") == 0) {
						mustUnderstand = 1;
					} else if (strcmp(attr->children->content,"0") == 0 ||
					           strcmp(attr->children->content,"false") == 0) {
						mustUnderstand = 0;
					} else {
						soap_server_fault("Client","mustUnderstand value is not boolean", NULL, NULL TSRMLS_CC);
					}
				}
				h = emalloc(sizeof(soapHeader));
				memset(h, 0, sizeof(soapHeader));
				h->mustUnderstand = mustUnderstand;
				h->function = find_function(sdl, hdr_func, &h->function_name);
				if (!h->function && sdl && function && function->binding && function->binding->bindingType == BINDING_SOAP) {
					sdlSoapBindingFunctionHeaderPtr *hdr;
					sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;
				  if (fnb->input.headers) {
				  	smart_str key = {0};

				  	if (hdr_func->ns) {
				  		smart_str_appends(&key, hdr_func->ns->href);
				  		smart_str_appendc(&key, ':');
				  	}
			  		smart_str_appendl(&key, Z_STRVAL(h->function_name), Z_STRLEN(h->function_name));
			  		smart_str_0(&key);
				    if (zend_hash_find(fnb->input.headers, key.c, key.len+1, (void**)&hdr) == SUCCESS) {
					    h->hdr = *hdr;
					  }
			  		smart_str_free(&key);
				  }
				}
				if (h->hdr) {
					h->num_params = 1;
					h->parameters = emalloc(sizeof(zval*));
					h->parameters[0] = master_to_zval(h->hdr->encode, hdr_func);
				} else {
					if (h->function && h->function->binding && h->function->binding->bindingType == BINDING_SOAP) {
						sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)h->function->bindingAttributes;
						if (fnb->style == SOAP_RPC) {
							hdr_func = hdr_func->children;
						}
					}
					deseralize_parameters(hdr_func, h->function, &h->num_params, &h->parameters);
				}
				INIT_ZVAL(h->retval);
				if (last == NULL) {
					*headers = h;
				} else {
					last->next = h;
				}
				last = h;
			}
ignore_header:
			trav = trav->next;
		}
	}

	if (function && function->binding && function->binding->bindingType == BINDING_SOAP) {
		sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;
		if (fnb->style == SOAP_RPC) {
			func = func->children;
		}
	} else {
		func = func->children;
	}
	deseralize_parameters(func, function, num_params, parameters);
	return function;
}

static int seralize_response_call2(xmlNodePtr body, sdlFunctionPtr function, char *function_name, char *uri, zval *ret, int version, int main TSRMLS_DC)
{
	xmlNodePtr method = NULL, param;
	sdlParamPtr parameter = NULL;
	int param_count;
	int style, use;
	xmlNsPtr ns = NULL;

	if (function != NULL && function->binding->bindingType == BINDING_SOAP) {
		sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;

		style = fnb->style;
		use = fnb->output.use;
		if (style == SOAP_RPC) {
			ns = encode_add_ns(body, fnb->output.ns);
			if (function->responseName) {
				method = xmlNewChild(body, ns, function->responseName, NULL);
			} else {
				method = xmlNewChild(body, ns, function->functionName, NULL);
			}
		}
	} else {
		style = main?SOAP_RPC:SOAP_DOCUMENT;
		use = main?SOAP_ENCODED:SOAP_LITERAL;
		if (style == SOAP_RPC) {
			ns = encode_add_ns(body, uri);
			method = xmlNewChild(body, ns, function_name, NULL);
		}
	}

	if (function != NULL) {
		if (function->responseParameters) {
			param_count = zend_hash_num_elements(function->responseParameters);
		} else {
		  param_count = 0;
		}
	} else {
	  param_count = 1;
	}

	if (param_count == 1) {
		parameter = get_param(function, NULL, 0, TRUE);

		if (style == SOAP_RPC) {
		  xmlNode *rpc_result;
			if (main && version == SOAP_1_2) {
				xmlNs *rpc_ns = xmlNewNs(body, RPC_SOAP12_NAMESPACE, RPC_SOAP12_NS_PREFIX);
				rpc_result = xmlNewChild(method, rpc_ns, "result", NULL);
			}
			param = seralize_parameter(parameter, ret, 0, "return", use, method TSRMLS_CC);
			if (main && version == SOAP_1_2) {
				xmlNodeSetContent(rpc_result,param->name);
			}
		} else {
			param = seralize_parameter(parameter, ret, 0, "return", use, body TSRMLS_CC);
			if (function && function->binding->bindingType == BINDING_SOAP) {
				sdlParamPtr *sparam;

				if (zend_hash_index_find(function->responseParameters, 0, (void **)&sparam) == SUCCESS && (*sparam)->element) {
					ns = encode_add_ns(param, (*sparam)->element->namens);
					xmlNodeSetName(param, (*sparam)->element->name);
					xmlSetNs(param, ns);
				}
			} else if (strcmp(param->name,"return") == 0) {
				ns = encode_add_ns(param, uri);
				xmlNodeSetName(param, function_name);
				xmlSetNs(param, ns);
			}
		}
	} else if (param_count > 1 && Z_TYPE_P(ret) == IS_ARRAY) {
		HashPosition pos;
		zval **data;
		int i = 0;

		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(ret), &pos);
		while (zend_hash_get_current_data_ex(Z_ARRVAL_P(ret), (void **)&data, &pos) != FAILURE) {
			char *param_name;
			int   param_name_len;
			long  param_index;

			zend_hash_get_current_key_ex(Z_ARRVAL_P(ret), &param_name, &param_name_len, &param_index, 0, &pos);
			parameter = get_param(function, param_name, param_index, TRUE);

			if (style == SOAP_RPC) {
				param = seralize_parameter(parameter, *data, i, param_name, use, method TSRMLS_CC);
			} else {
				param = seralize_parameter(parameter, *data, i, param_name, use, body TSRMLS_CC);
				if (function && function->binding->bindingType == BINDING_SOAP) {
					sdlParamPtr *sparam;

					if (zend_hash_index_find(function->responseParameters, i, (void **)&sparam) == SUCCESS) {
						ns = encode_add_ns(param, (*sparam)->encode->details.ns);
						xmlNodeSetName(param, (*sparam)->encode->details.type_str);
						xmlSetNs(param, ns);
					}
				}
			}

			zend_hash_move_forward_ex(Z_ARRVAL_P(ret), &pos);
			i++;
		}
	}
	if (use == SOAP_ENCODED && version == SOAP_1_2 && method != NULL) {
		xmlSetNsProp(method, body->ns, "encodingStyle", SOAP_1_2_ENC_NAMESPACE);
	}
	return use;
}

static xmlDocPtr seralize_response_call(sdlFunctionPtr function, char *function_name, char *uri, zval *ret, soapHeader* headers, int version TSRMLS_DC)
{
	xmlDocPtr doc;
	xmlNodePtr envelope = NULL, body, param;
	xmlNsPtr ns = NULL;
	int use = SOAP_LITERAL;

	encode_reset_ns();

	doc = xmlNewDoc("1.0");
	doc->charset = XML_CHAR_ENCODING_UTF8;
	doc->encoding = xmlStrdup((xmlChar*)"UTF-8");

	if (version == SOAP_1_1) {
		envelope = xmlNewDocNode(doc, NULL, "Envelope", NULL);
		ns = xmlNewNs(envelope, SOAP_1_1_ENV_NAMESPACE, SOAP_1_1_ENV_NS_PREFIX);
		xmlSetNs(envelope,ns);
	} else if (version == SOAP_1_2) {
		envelope = xmlNewDocNode(doc, NULL, "Envelope", NULL);
		ns = xmlNewNs(envelope, SOAP_1_2_ENV_NAMESPACE, SOAP_1_2_ENV_NS_PREFIX);
		xmlSetNs(envelope,ns);
	} else {
	  php_error(E_ERROR, "Unknown SOAP version");
	}
	xmlDocSetRootElement(doc, envelope);

	if (Z_TYPE_P(ret) == IS_OBJECT &&
		Z_OBJCE_P(ret) == soap_fault_class_entry) {
		body = xmlNewChild(envelope, ns, "Body", NULL);
		use = SOAP_LITERAL;
		if (version == SOAP_1_1) {
			HashTable* prop;
			zval **tmp;

			prop = Z_OBJPROP_P(ret);
			param = xmlNewChild(body, ns, "Fault", NULL);
			if (zend_hash_find(prop, "faultcode", sizeof("faultcode"), (void**)&tmp) == SUCCESS) {
				int new_len;
				xmlNodePtr node = xmlNewNode(NULL, "faultcode");
				char *str = php_escape_html_entities(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), &new_len, 0, 0, NULL TSRMLS_CC);
				xmlAddChild(param, node);
				xmlNodeSetContentLen(node, str, new_len);
				efree(str);
			}
			if (zend_hash_find(prop, "faultstring", sizeof("faultstring"), (void**)&tmp) == SUCCESS) {
				int new_len;
				xmlNodePtr node = xmlNewNode(NULL, "faultstring");
				char *str = php_escape_html_entities(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), &new_len, 0, 0, NULL TSRMLS_CC);
				xmlAddChild(param, node);
				xmlNodeSetContentLen(node, str, new_len);
				efree(str);
			}
			if (zend_hash_find(prop, "faultactor", sizeof("faultactor"), (void**)&tmp) == SUCCESS) {
				int new_len;
				xmlNodePtr node = xmlNewNode(NULL, "faultactor");
				char *str = php_escape_html_entities(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), &new_len, 0, 0, NULL TSRMLS_CC);
				xmlAddChild(param, node);
				xmlNodeSetContentLen(node, str, new_len);
				efree(str);
			}
			if (zend_hash_find(prop, "detail", sizeof("detail"), (void**)&tmp) == SUCCESS &&
			    Z_TYPE_PP(tmp) != IS_NULL) {
	  		/*FIXME: use = SOAP_ENCODED;*/
				seralize_zval(*tmp, NULL, "detail", use, param TSRMLS_CC);
			}
		} else {
			HashTable* prop;
			zval **tmp;

			prop = Z_OBJPROP_P(ret);
			param = xmlNewChild(body, ns, "Fault", NULL);
			if (zend_hash_find(prop, "faultcode", sizeof("faultcode"), (void**)&tmp) == SUCCESS) {
				int new_len;
				xmlNodePtr node = xmlNewChild(param, ns, "Code", NULL);
				char *str = php_escape_html_entities(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), &new_len, 0, 0, NULL TSRMLS_CC);
				node = xmlNewChild(node, ns, "Value", NULL);
				xmlNodeSetContentLen(node, str, new_len);
				efree(str);
			}
			if (zend_hash_find(prop, "faultstring", sizeof("faultstring"), (void**)&tmp) == SUCCESS) {
				int new_len;
				xmlNodePtr node = xmlNewChild(param, ns, "Reason", NULL);
				char *str = php_escape_html_entities(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), &new_len, 0, 0, NULL TSRMLS_CC);
				node = xmlNewChild(node, ns, "Text", NULL);
				xmlNodeSetContentLen(node, str, new_len);
				efree(str);
			}
			if (zend_hash_find(prop, "detail", sizeof("detail"), (void**)&tmp) == SUCCESS &&
			    Z_TYPE_PP(tmp) != IS_NULL) {
				seralize_zval(*tmp, NULL, SOAP_1_2_ENV_NS_PREFIX":Detail", use, param TSRMLS_CC);
			}
		}
	} else {

		if (headers) {
			xmlNodePtr head;
			soapHeader *h;

			head = xmlNewChild(envelope, ns, "Header", NULL);
			h = headers;
			while (h != NULL) {
				if (Z_TYPE(h->retval) != IS_NULL) {
					encodePtr hdr_enc = NULL;
					int hdr_use = SOAP_LITERAL;
					zval *hdr_ret = &h->retval;
					char *hdr_ns   = h->hdr?h->hdr->ns:NULL;
					char *hdr_name = Z_STRVAL(h->function_name);


					if (Z_TYPE(h->retval) == IS_OBJECT &&
					    Z_OBJCE(h->retval) == soap_header_class_entry) {
						HashTable* ht = Z_OBJPROP(h->retval);
						zval **tmp;

						if (function && function->binding && function->binding->bindingType == BINDING_SOAP) {
							sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;

							if (fnb->output.headers) {
								sdlSoapBindingFunctionHeaderPtr *hdr;
								smart_str key = {0};

								if (zend_hash_find(ht, "namespace", sizeof("namespace"), (void**)&tmp) == SUCCESS &&
							      Z_TYPE_PP(tmp) == IS_STRING) {
									smart_str_appendl(&key, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
									smart_str_appendc(&key, ':');
									hdr_ns = Z_STRVAL_PP(tmp);
								}
								if (zend_hash_find(ht, "name", sizeof("name"), (void**)&tmp) == SUCCESS &&
								    Z_TYPE_PP(tmp) == IS_STRING) {
									smart_str_appendl(&key, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
									hdr_name = Z_STRVAL_PP(tmp);
								}
								smart_str_0(&key);
								if (zend_hash_find(fnb->output.headers, key.c, key.len+1, (void**)&hdr) == SUCCESS) {
									hdr_enc = (*hdr)->encode;
									hdr_use = (*hdr)->use;
								}
								smart_str_free(&key);
							}
						}
						if (zend_hash_find(ht, "data", sizeof("data"), (void**)&tmp) == SUCCESS) {
							hdr_ret = *tmp;
						} else {
							hdr_ret = NULL;
						}
					}

					if (h->function) {
						if (seralize_response_call2(head, h->function, Z_STRVAL(h->function_name), uri, hdr_ret, version, 0 TSRMLS_CC) == SOAP_ENCODED) {
							use = SOAP_ENCODED;
						}
					} else {
						xmlNodePtr xmlHdr = master_to_xml(hdr_enc, hdr_ret, hdr_use, head);
						if (hdr_name) {
							xmlNodeSetName(xmlHdr,hdr_name);
						}
						if (hdr_ns) {
							xmlNsPtr nsptr = encode_add_ns(xmlHdr,hdr_ns);
							xmlSetNs(xmlHdr, nsptr);
						}
					}
				}
				h = h->next;
			}

			if (head->children == NULL) {
				xmlUnlinkNode(head);
				xmlFreeNode(head);
			}
		}

		body = xmlNewChild(envelope, ns, "Body", NULL);

		if (seralize_response_call2(body, function, function_name, uri, ret, version, 1 TSRMLS_CC) == SOAP_ENCODED) {
			use = SOAP_ENCODED;
		}

	}

	if (use == SOAP_ENCODED) {
		xmlNewNs(envelope, XSD_NAMESPACE, XSD_NS_PREFIX);
		xmlNewNs(envelope, XSI_NAMESPACE, XSI_NS_PREFIX);
		if (version == SOAP_1_1) {
			xmlNewNs(envelope, SOAP_1_1_ENC_NAMESPACE, SOAP_1_1_ENC_NS_PREFIX);
			xmlSetNsProp(envelope, envelope->ns, "encodingStyle", SOAP_1_1_ENC_NAMESPACE);
		} else if (version == SOAP_1_2) {
			xmlNewNs(envelope, SOAP_1_2_ENC_NAMESPACE, SOAP_1_2_ENC_NS_PREFIX);
		}
	}

	return doc;
}

static xmlDocPtr seralize_function_call(zval *this_ptr, sdlFunctionPtr function, char *function_name, char *uri, zval **arguments, int arg_count, int version, HashTable *soap_headers TSRMLS_DC)
{
	xmlDoc *doc;
	xmlNodePtr envelope = NULL, body, method = NULL, head = NULL;
	xmlNsPtr ns = NULL;
	zval **zstyle, **zuse;
	int i, style, use;
	HashTable *hdrs = NULL;

	encode_reset_ns();

	doc = xmlNewDoc("1.0");
	doc->encoding = xmlStrdup((xmlChar*)"UTF-8");
	doc->charset = XML_CHAR_ENCODING_UTF8;
	if (version == SOAP_1_1) {
		envelope = xmlNewDocNode(doc, NULL, "Envelope", NULL);
		ns = xmlNewNs(envelope, SOAP_1_1_ENV_NAMESPACE, SOAP_1_1_ENV_NS_PREFIX);
		xmlSetNs(envelope,ns);
	} else if (version == SOAP_1_2) {
		envelope = xmlNewDocNode(doc, NULL, "Envelope", NULL);
		ns = xmlNewNs(envelope, SOAP_1_2_ENV_NAMESPACE, SOAP_1_2_ENV_NS_PREFIX);
		xmlSetNs(envelope,ns);
	} else {
		php_error(E_ERROR, "SOAP-ERROR: Unknown SOAP version");
	}
	xmlDocSetRootElement(doc, envelope);

	if (soap_headers) {
		head = xmlNewChild(envelope, ns, "Header", NULL);
	}

	body = xmlNewChild(envelope, ns, "Body", NULL);

	if (function && function->binding->bindingType == BINDING_SOAP) {
		sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;

		hdrs = fnb->input.headers;
		style = fnb->style;
		/*FIXME: how to pass method name if style is SOAP_DOCUMENT */
		/*style = SOAP_RPC;*/
		use = fnb->input.use;
		if (style == SOAP_RPC) {
			ns = encode_add_ns(body, fnb->input.ns);
			if (function->requestName) {
				method = xmlNewChild(body, ns, function->requestName, NULL);
			} else {
				method = xmlNewChild(body, ns, function->functionName, NULL);
			}
		}
	} else {
		if (zend_hash_find(Z_OBJPROP_P(this_ptr), "style", sizeof("style"), (void **)&zstyle) == SUCCESS) {
			style = Z_LVAL_PP(zstyle);
		} else {
			style = SOAP_RPC;
		}
		/*FIXME: how to pass method name if style is SOAP_DOCUMENT */
		/*style = SOAP_RPC;*/
		if (style == SOAP_RPC) {
			ns = encode_add_ns(body, uri);
			method = xmlNewChild(body, ns, function_name, NULL);
		}

		if (zend_hash_find(Z_OBJPROP_P(this_ptr), "use", sizeof("use"), (void **)&zuse) == SUCCESS &&
			  Z_LVAL_PP(zuse) == SOAP_LITERAL) {
			use = SOAP_LITERAL;
		} else {
			use = SOAP_ENCODED;
		}
	}

	for (i = 0;i < arg_count;i++) {
		xmlNodePtr param;
		sdlParamPtr parameter = get_param(function, NULL, i, FALSE);

		if (style == SOAP_RPC) {
			param = seralize_parameter(parameter, arguments[i], i, NULL, use, method TSRMLS_CC);
		} else if (style == SOAP_DOCUMENT) {
			param = seralize_parameter(parameter, arguments[i], i, NULL, use, body TSRMLS_CC);
			if (function && function->binding->bindingType == BINDING_SOAP) {
				sdlParamPtr *sparam;

				if (zend_hash_index_find(function->requestParameters, i, (void **)&sparam) == SUCCESS && (*sparam)->element) {
					ns = encode_add_ns(param, (*sparam)->element->namens);
					xmlNodeSetName(param, (*sparam)->element->name);
					xmlSetNs(param, ns);
				}
			}
		}
	}

	if (head) {
		zval** header;

		zend_hash_internal_pointer_reset(soap_headers);
		while (zend_hash_get_current_data(soap_headers,(void**)&header) == SUCCESS) {
		  HashTable *ht = Z_OBJPROP_PP(header);
		  zval **name, **ns, **tmp;

		  if (zend_hash_find(ht, "name", sizeof("name"), (void**)&name) == SUCCESS &&
		      Z_TYPE_PP(name) == IS_STRING &&
					zend_hash_find(ht, "namespace", sizeof("namespace"), (void**)&ns) == SUCCESS &&
		      Z_TYPE_PP(ns) == IS_STRING) {
		  	xmlNodePtr h;
		  	xmlNsPtr nsptr;
		    int hdr_use = SOAP_LITERAL;
		    encodePtr enc = NULL;

		    if (hdrs) {
		    	smart_str key = {0};
		    	sdlSoapBindingFunctionHeaderPtr *hdr;

		    	smart_str_appendl(&key, Z_STRVAL_PP(ns), Z_STRLEN_PP(ns));
		    	smart_str_appendc(&key, ':');
		    	smart_str_appendl(&key, Z_STRVAL_PP(name), Z_STRLEN_PP(name));
		    	smart_str_0(&key);
		    	if (zend_hash_find(hdrs, key.c, key.len+1,(void**)&hdr) == SUCCESS) {
		    		hdr_use = (*hdr)->use;
		    		enc = (*hdr)->encode;
		    		if (hdr_use == SOAP_ENCODED) {
		    			use = SOAP_ENCODED;
		    		}
		    	}
		    	smart_str_free(&key);
		    }

			  if (zend_hash_find(ht, "data", sizeof("data"), (void**)&tmp) == SUCCESS) {
			  	h = master_to_xml(enc, *tmp, hdr_use, head);
			  	xmlNodeSetName(h, Z_STRVAL_PP(name));
			  } else {
			  	h = xmlNewNode(NULL, Z_STRVAL_PP(name));
			  	xmlAddChild(head,h);
			  }
		  	nsptr = encode_add_ns(h,Z_STRVAL_PP(ns));
				xmlSetNs(h, nsptr);

			  if (zend_hash_find(ht, "mustUnderstand", sizeof("mustUnderstand"), (void**)&tmp) == SUCCESS &&
			      Z_TYPE_PP(tmp) == IS_BOOL && Z_LVAL_PP(tmp)) {
			  	if (version == SOAP_1_1) {
			  		xmlSetProp(h, SOAP_1_1_ENV_NS_PREFIX":mustUnderstand","1");
			  	} else {
			  		xmlSetProp(h, SOAP_1_2_ENV_NS_PREFIX":mustUnderstand","true");
			  	}
			  }
			  if (zend_hash_find(ht, "actor", sizeof("actor"), (void**)&tmp) == SUCCESS) {
			  	if (Z_TYPE_PP(tmp) == IS_STRING) {
				  	if (version == SOAP_1_1) {
				  		xmlSetProp(h, SOAP_1_1_ENV_NS_PREFIX":actor",Z_STRVAL_PP(tmp));
				  	} else {
				  		xmlSetProp(h, SOAP_1_2_ENV_NS_PREFIX":role",Z_STRVAL_PP(tmp));
			  		}
			  	} else if (Z_TYPE_PP(tmp) == IS_LONG) {
				  	if (version == SOAP_1_1) {
				  		if (Z_LVAL_PP(tmp) == SOAP_ACTOR_NEXT) {
					  		xmlSetProp(h, SOAP_1_1_ENV_NS_PREFIX":actor",SOAP_1_1_ACTOR_NEXT);
					  	}
				  	} else {
				  		if (Z_LVAL_PP(tmp) == SOAP_ACTOR_NEXT) {
					  		xmlSetProp(h, SOAP_1_2_ENV_NS_PREFIX":role",SOAP_1_2_ACTOR_NEXT);
				  		} else if (Z_LVAL_PP(tmp) == SOAP_ACTOR_NONE) {
					  		xmlSetProp(h, SOAP_1_2_ENV_NS_PREFIX":role",SOAP_1_2_ACTOR_NONE);
				  		} else if (Z_LVAL_PP(tmp) == SOAP_ACTOR_UNLIMATERECEIVER) {
					  		xmlSetProp(h, SOAP_1_2_ENV_NS_PREFIX":role",SOAP_1_2_ACTOR_UNLIMATERECEIVER);
					  	}
			  		}
			  	}
			  }
		  }
			zend_hash_move_forward(soap_headers);
		}
	}

	if (use == SOAP_ENCODED) {
		xmlNewNs(envelope, XSD_NAMESPACE, XSD_NS_PREFIX);
		xmlNewNs(envelope, XSI_NAMESPACE, XSI_NS_PREFIX);
		if (version == SOAP_1_1) {
			xmlNewNs(envelope, SOAP_1_1_ENC_NAMESPACE, SOAP_1_1_ENC_NS_PREFIX);
			xmlSetNsProp(envelope, envelope->ns, "encodingStyle", SOAP_1_1_ENC_NAMESPACE);
		} else if (version == SOAP_1_2) {
			xmlNewNs(envelope, SOAP_1_2_ENC_NAMESPACE, SOAP_1_2_ENC_NS_PREFIX);
			if (method) {
				xmlSetNsProp(method, envelope->ns, "encodingStyle", SOAP_1_2_ENC_NAMESPACE);
			}
		}
	}

	return doc;
}

static xmlNodePtr seralize_parameter(sdlParamPtr param, zval *param_val, int index, char *name, int style, xmlNodePtr parent TSRMLS_DC)
{
	char *paramName;
	xmlNodePtr xmlParam;

	if (Z_TYPE_P(param_val) == IS_OBJECT &&
		Z_OBJCE_P(param_val) == soap_param_class_entry) {
		zval **param_name;
		zval **param_data;

		if (zend_hash_find(Z_OBJPROP_P(param_val), "param_name", sizeof("param_name"), (void **)&param_name) == SUCCESS &&
		   zend_hash_find(Z_OBJPROP_P(param_val), "param_data", sizeof("param_data"), (void **)&param_data) == SUCCESS) {
		  param_val = *param_data;
		  name = Z_STRVAL_PP(param_name);
		}
	}

	if (param != NULL && param->paramName != NULL) {
		paramName = estrdup(param->paramName);
	} else {
		if (name == NULL) {
			paramName = emalloc(10);
			sprintf(paramName,"param%d",index);
		} else {
			paramName = estrdup(name);
		}
	}

	xmlParam = seralize_zval(param_val, param, paramName, style, parent TSRMLS_CC);

	efree(paramName);

	return xmlParam;
}

static xmlNodePtr seralize_zval(zval *val, sdlParamPtr param, char *paramName, int style, xmlNodePtr parent TSRMLS_DC)
{
	xmlNodePtr xmlParam;
	encodePtr enc;

	if (param != NULL) {
		enc = param->encode;
	} else {
		enc = NULL;
	}
	xmlParam = master_to_xml(enc, val, style, parent);
	if (!strcmp(xmlParam->name, "BOGUS")) {
		xmlNodeSetName(xmlParam, paramName);
	}
	return xmlParam;
}

static sdlParamPtr get_param(sdlFunctionPtr function, char *param_name, int index, int response)
{
	sdlParamPtr *tmp;
	HashTable   *ht;

	if (function == NULL) {
		return NULL;
	}

	if (response == FALSE) {
		ht = function->requestParameters;
	} else {
		ht = function->responseParameters;
	}

	if (param_name != NULL) {
		if (zend_hash_find(ht, param_name, strlen(param_name), (void **)&tmp) != FAILURE) {
			return *tmp;
		}
	} else {
		if (zend_hash_index_find(ht, index, (void **)&tmp) != FAILURE) {
			return (*tmp);
		}
	}
	return NULL;
}

static sdlFunctionPtr get_function(sdlPtr sdl, const char *function_name)
{
	sdlFunctionPtr *tmp;

	int len = strlen(function_name);
	char *str = estrndup(function_name,len);
	php_strtolower(str,len);
	if (sdl != NULL) {
		if (zend_hash_find(&sdl->functions, str, len+1, (void **)&tmp) != FAILURE) {
			efree(str);
			return (*tmp);
		} else if (sdl->requests != NULL && zend_hash_find(sdl->requests, str, len+1, (void **)&tmp) != FAILURE) {
			efree(str);
			return (*tmp);
		}
	}
	efree(str);
	return NULL;
}

static sdlFunctionPtr get_doc_function(sdlPtr sdl, xmlNodePtr params)
{
	if (sdl) {
		sdlFunctionPtr *tmp;
		sdlParamPtr    *param;

		zend_hash_internal_pointer_reset(&sdl->functions);
		while (zend_hash_get_current_data(&sdl->functions, (void**)&tmp) == SUCCESS) {
			if ((*tmp)->binding && (*tmp)->binding->bindingType == BINDING_SOAP) {
				sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)(*tmp)->bindingAttributes;
				if (fnb->style == SOAP_DOCUMENT) {
					if (params == NULL) {
						if ((*tmp)->requestParameters == NULL ||
						    zend_hash_num_elements((*tmp)->requestParameters) == 0) {
						  return *tmp;
						}
					} else if ((*tmp)->requestParameters != NULL &&
					           zend_hash_num_elements((*tmp)->requestParameters) > 0) {
						int ok = 1;
						xmlNodePtr node = params;

						zend_hash_internal_pointer_reset((*tmp)->requestParameters);
						while (zend_hash_get_current_data((*tmp)->requestParameters, (void**)&param) == SUCCESS) {
						  if ((*param)->element) {
						  	if (strcmp((*param)->element->name,node->name) != 0) {
							  	ok = 0;
							  	break;
						  	}
						  	if ((*param)->element->namens != NULL && node->ns != NULL) {
						  		if (strcmp((*param)->element->namens,node->ns->href) != 0) {
								  	ok = 0;
								  	break;
						  		}
						  	} else if ((void*)(*param)->element->namens != (void*)node->ns) {
							  	ok = 0;
							  	break;
						  	}
						  } else if (strcmp((*param)->paramName,node->name) != 0) {
						  	ok = 0;
						  	break;
						  }
							zend_hash_move_forward((*tmp)->requestParameters);
/*
							do {
								node = node->next;
							} while (node != NULL && node->type != XML_ELEMENT_NODE);
*/
						}
						if (ok /*&& node == NULL*/) {
							return (*tmp);
						}
					}
				}
			}
			zend_hash_move_forward(&sdl->functions);
		}
	}
	return NULL;
}

static void function_to_string(sdlFunctionPtr function, smart_str *buf)
{
	int i = 0;
	HashPosition pos;

	if (function->responseParameters && function->responseParameters->pListHead) {
		sdlParamPtr *param;
		param = function->responseParameters->pListHead->pData;
		if ((*param)->encode->details.type_str) {
			smart_str_appendl(buf, (*param)->encode->details.type_str, strlen((*param)->encode->details.type_str));
			smart_str_appendc(buf, ' ');
		} else {
			smart_str_appendl(buf, "mixed ", 6);
		}
	} else {
		smart_str_appendl(buf, "void ", 5);
	}

	smart_str_appendl(buf, function->functionName, strlen(function->functionName));

	smart_str_appendc(buf, '(');
	if (function->requestParameters) {
		sdlParamPtr *param;

		i = 0;
		zend_hash_internal_pointer_reset_ex(function->requestParameters, &pos);
		while (zend_hash_get_current_data_ex(function->requestParameters, (void **)&param, &pos) != FAILURE) {
			if ((*param)->encode->details.type_str) {
				smart_str_appendl(buf, (*param)->encode->details.type_str, strlen((*param)->encode->details.type_str));
			} else {
				smart_str_appendl(buf, "mixed ", 6);
			}
			smart_str_appendc(buf, ' ');
			smart_str_appendc(buf, '$');
			smart_str_appendl(buf, (*param)->paramName, strlen((*param)->paramName));
			if (zend_hash_num_elements(function->requestParameters) > i + 1) {
				smart_str_appendl(buf, ", ", 2);
			}
			zend_hash_move_forward_ex(function->requestParameters, &pos);
			i++;
		}
	}
	smart_str_appendc(buf, ')');
	smart_str_0(buf);
}

static void type_to_string(sdlTypePtr type, smart_str *buf, int level)
{
	int i;
	smart_str spaces = {0};
	HashPosition pos;

	for (i = 0;i < level;i++) {
		smart_str_appendc(&spaces, ' ');
	}
	smart_str_appendl(buf, spaces.c, spaces.len);

	if (type->elements) {
		sdlTypePtr *t_type;

		smart_str_appendl(buf, "struct ", 7);
		smart_str_appendl(buf, type->name, strlen(type->name));
		smart_str_appendc(buf, '\n');
		smart_str_appendl(buf, spaces.c, spaces.len);
		smart_str_appendl(buf, "{\n", 2);

		zend_hash_internal_pointer_reset_ex(type->elements, &pos);
		level++;
		while (zend_hash_get_current_data_ex(type->elements, (void **)&t_type, &pos) != FAILURE) {
			type_to_string((*t_type), buf, level);
			zend_hash_move_forward_ex(type->elements, &pos);
		}

		smart_str_appendl(buf, spaces.c, spaces.len);
		smart_str_appendl(buf, "}\n", 2);
	} else {
		smart_str_appendl(buf, type->encode->details.type_str, strlen(type->encode->details.type_str));
		smart_str_appendc(buf, ' ');
		smart_str_appendl(buf, type->name, strlen(type->name));
		smart_str_appendl(buf, ";\n", 2);
	}
	smart_str_free(&spaces);
	smart_str_0(buf);
}

static void delete_url(void *handle)
{
	php_url_free((php_url*)handle);
}

static void delete_service(void *data)
{
	soapServicePtr service = (soapServicePtr)data;

	if (service->soap_functions.ft) {
		zend_hash_destroy(service->soap_functions.ft);
		efree(service->soap_functions.ft);
	}

	if (service->mapping) {
		zend_hash_destroy(service->mapping);
		efree(service->mapping);
	}

	if (service->soap_class.argc) {
		int i;
		for (i = 0; i < service->soap_class.argc;i++) {
			zval_ptr_dtor(&service->soap_class.argv[i]);
		}
		efree(service->soap_class.argv);
	}

	if (service->actor) {
		efree(service->actor);
	}
	efree(service->uri);
	efree(service);
}
