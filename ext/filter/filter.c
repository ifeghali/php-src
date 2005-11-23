/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2005 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Rasmus Lerdorf <rasmus@php.net>                             |
  |          Derick Rethans <derick@php.net>                             |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_filter.h"
#include "Zend/zend_API.h"

ZEND_DECLARE_MODULE_GLOBALS(filter)

#include "filter_private.h"

typedef struct filter_list_entry {
	char  *name;
	int    id;
	void (*function)(PHP_INPUT_FILTER_PARAM_DECL);
} filter_list_entry;

filter_list_entry filter_list[] = {
	{ "int",             FL_INT,           php_filter_int             },
	{ "boolean",         FL_BOOLEAN,       php_filter_boolean         },
	{ "float",           FL_FLOAT,         php_filter_float           },

	{ "validate_regexp", FL_REGEXP,        php_filter_validate_regexp },
	{ "validate_url",    FL_URL,           php_filter_validate_url    },
	{ "validate_email",  FL_EMAIL,         php_filter_validate_email  },
	{ "validate_ip",     FL_IP,            php_filter_validate_ip     },

	{ "string",          FS_STRING,        php_filter_string          },
	{ "stripped",        FS_STRING,        php_filter_string          },
	{ "encoded",         FS_ENCODED,       php_filter_encoded         },
	{ "special_chars",   FS_SPECIAL_CHARS, php_filter_special_chars   },
	{ "unsafe_raw",      FS_UNSAFE_RAW,    php_filter_unsafe_raw      },
	{ "email",           FS_EMAIL,         php_filter_email           },
	{ "url",             FS_URL,           php_filter_url             },
	{ "number_int",      FS_NUMBER_INT,    php_filter_number_int      },
	{ "number_float",    FS_NUMBER_FLOAT,  php_filter_number_float    },
	{ "magic_quotes",    FS_MAGIC_QUOTES,  php_filter_magic_quotes    },

	{ "callback",        FC_CALLBACK,      php_filter_callback        },
};
	
#ifndef PARSE_ENV
#define PARSE_ENV 4
#endif

#ifndef PARSE_SERVER
#define PARSE_SERVER 5
#endif

#ifndef PARSE_SESSION
#define PARSE_SESSION 6
#endif

static unsigned int php_sapi_filter(int arg, char *var, char **val, unsigned int val_len, unsigned int *new_val_len TSRMLS_DC);

/* {{{ filter_functions[]
 */
function_entry filter_functions[] = {
	PHP_FE(input_get, NULL)
	PHP_FE(input_filters_list, NULL)
	PHP_FE(input_has_variable, NULL)
	PHP_FE(input_name_to_filter, NULL)
	PHP_FE(filter_data, NULL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ filter_module_entry
 */
zend_module_entry filter_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"filter",
	filter_functions,
	PHP_MINIT(filter),
	PHP_MSHUTDOWN(filter),
	NULL,
	PHP_RSHUTDOWN(filter),
	PHP_MINFO(filter),
	"0.9.3",
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_FILTER
ZEND_GET_MODULE(filter)
#endif

/* {{{ UpdateDefaultFilter
 */
static PHP_INI_MH(UpdateDefaultFilter)
{
	int i, size = sizeof(filter_list) / sizeof(filter_list_entry);

	for (i = 0; i < size; ++i) {
		if ((strcasecmp(new_value, filter_list[i].name) == 0)) {
			IF_G(default_filter) = filter_list[i].id;
			return SUCCESS;
		}
	}
	/* Fallback to "string" filter */
	IF_G(default_filter) = FS_DEFAULT;
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_INI
 */
static PHP_INI_MH(OnUpdateFlags)
{
	if (!new_value) {
		IF_G(default_filter_flags) = FILTER_FLAG_NO_ENCODE_QUOTES;
	} else {
		IF_G(default_filter_flags) = atoi(new_value);
	}
	return SUCCESS;
}

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("filter.default",   "string", PHP_INI_ALL, UpdateDefaultFilter, default_filter, zend_filter_globals, filter_globals)
	PHP_INI_ENTRY("filter.default_flags", NULL,     PHP_INI_ALL, OnUpdateFlags)
PHP_INI_END()
/* }}} */

/* {{{ php_filter_init_globals
*/
static void php_filter_init_globals(zend_filter_globals *filter_globals)
{
	filter_globals->post_array = NULL;
	filter_globals->get_array = NULL;
	filter_globals->cookie_array = NULL;
	filter_globals->env_array = NULL;
	filter_globals->server_array = NULL;
	filter_globals->session_array = NULL;
	filter_globals->default_filter = FS_DEFAULT;
}
/* }}} */

#define PARSE_REQUEST 99

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(filter)
{
	ZEND_INIT_MODULE_GLOBALS(filter, php_filter_init_globals, NULL);

	REGISTER_INI_ENTRIES();

	REGISTER_LONG_CONSTANT("INPUT_POST", PARSE_POST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INPUT_GET", PARSE_GET, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INPUT_COOKIE", PARSE_COOKIE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INPUT_ENV", PARSE_ENV, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INPUT_SERVER", PARSE_SERVER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("INPUT_SESSION", PARSE_SESSION, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FILTER_FLAG_NONE", FILTER_FLAG_NONE, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FL_INT", FL_INT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FL_BOOLEAN", FL_BOOLEAN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FL_FLOAT", FL_FLOAT, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FL_REGEXP", FL_REGEXP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FL_URL", FL_URL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FL_EMAIL", FL_EMAIL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FL_IP", FL_IP, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FS_DEFAULT", FS_DEFAULT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FS_STRING", FS_STRING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FS_STRIPPED", FS_STRING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FS_ENCODED", FS_ENCODED, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FS_SPECIAL_CHARS", FS_SPECIAL_CHARS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FS_UNSAFE_RAW", FS_UNSAFE_RAW, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FS_EMAIL", FS_EMAIL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FS_URL", FS_URL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FS_NUMBER_INT", FS_NUMBER_INT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FS_NUMBER_FLOAT", FS_NUMBER_FLOAT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FS_MAGIC_QUOTES", FS_MAGIC_QUOTES, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FC_CALLBACK", FC_CALLBACK, CONST_CS | CONST_PERSISTENT);
	
	REGISTER_LONG_CONSTANT("FILTER_FLAG_ALLOW_OCTAL", FILTER_FLAG_ALLOW_OCTAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_ALLOW_HEX", FILTER_FLAG_ALLOW_HEX, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FILTER_FLAG_STRIP_LOW", FILTER_FLAG_STRIP_LOW, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_STRIP_HIGH", FILTER_FLAG_STRIP_HIGH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_ENCODE_LOW", FILTER_FLAG_ENCODE_LOW, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_ENCODE_HIGH", FILTER_FLAG_ENCODE_HIGH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_ENCODE_AMP", FILTER_FLAG_ENCODE_AMP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_NO_ENCODE_QUOTES", FILTER_FLAG_NO_ENCODE_QUOTES, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_EMPTY_STRING_NULL", FILTER_FLAG_EMPTY_STRING_NULL, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FILTER_FLAG_ALLOW_FRACTION", FILTER_FLAG_ALLOW_FRACTION, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_ALLOW_THOUSAND", FILTER_FLAG_ALLOW_THOUSAND, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_ALLOW_SCIENTIFIC", FILTER_FLAG_ALLOW_SCIENTIFIC, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FILTER_FLAG_SCHEME_REQUIRED", FILTER_FLAG_SCHEME_REQUIRED, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_HOST_REQUIRED", FILTER_FLAG_HOST_REQUIRED, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_PATH_REQUIRED", FILTER_FLAG_PATH_REQUIRED, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_QUERY_REQUIRED", FILTER_FLAG_QUERY_REQUIRED, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FILTER_FLAG_IPV4", FILTER_FLAG_IPV4, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_IPV6", FILTER_FLAG_IPV6, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_NO_RES_RANGE", FILTER_FLAG_NO_RES_RANGE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FILTER_FLAG_NO_PRIV_RANGE", FILTER_FLAG_NO_PRIV_RANGE, CONST_CS | CONST_PERSISTENT);

	sapi_register_input_filter(php_sapi_filter);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(filter)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
#define VAR_ARRAY_COPY_DTOR(a)   \
	if (IF_G(a)) {               \
		zval_ptr_dtor(&IF_G(a)); \
		IF_G(a) = NULL;          \
	}
PHP_RSHUTDOWN_FUNCTION(filter)
{
	VAR_ARRAY_COPY_DTOR(get_array)
	VAR_ARRAY_COPY_DTOR(post_array)
	VAR_ARRAY_COPY_DTOR(cookie_array)
	VAR_ARRAY_COPY_DTOR(server_array)
	VAR_ARRAY_COPY_DTOR(env_array)
	VAR_ARRAY_COPY_DTOR(session_array)
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(filter)
{
	php_info_print_table_start();
	php_info_print_table_row( 2, "Input Validation and Filtering", "enabled" );
	php_info_print_table_row( 2, "Revision", "$Revision$");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

static filter_list_entry php_find_filter(long id)
{
	int i, size = sizeof(filter_list) / sizeof(filter_list_entry);

	for (i = 0; i < size; ++i) {
		if (filter_list[i].id == id) {
			return filter_list[i];
		}
	}
	/* Fallback to "string" filter */
	for (i = 0; i < size; ++i) {
		if (filter_list[i].id == FS_DEFAULT) {
			return filter_list[i];
		}
	}
	/* To shut up GCC */
	return filter_list[0];
}

static void php_zval_filter(zval *value, long filter, long flags, zval *options, char* charset TSRMLS_DC)
{
	filter_list_entry  filter_func;
	
	filter_func = php_find_filter(filter);

	if (!filter_func.id) {
		/* Find default filter */
		filter_func = php_find_filter(FS_DEFAULT);
	}

	/* Here be strings */
	convert_to_string(value);

	filter_func.function(value, flags, options, charset TSRMLS_CC);
}

/* {{{ php_sapi_filter(int arg, char *var, char **val, unsigned int val_len, unsigned *new_val_len)
 */
static unsigned int php_sapi_filter(int arg, char *var, char **val, unsigned int val_len, unsigned int *new_val_len TSRMLS_DC)
{
	zval  new_var, raw_var;
	zval *array_ptr = NULL, *orig_array_ptr = NULL;
	char *orig_var;
	int retval = 0;

	assert(*val != NULL);

#define PARSE_CASE(s,a,t)                    \
		case s:                              \
			if (!IF_G(a)) {                  \
				ALLOC_ZVAL(array_ptr);       \
				array_init(array_ptr);       \
				INIT_PZVAL(array_ptr);       \
				IF_G(a) = array_ptr;         \
			} else {                         \
				array_ptr = IF_G(a);         \
			}                                \
			orig_array_ptr = PG(http_globals)[t]; \
			break;

	switch (arg) {
		PARSE_CASE(PARSE_POST,    post_array,    TRACK_VARS_POST)
		PARSE_CASE(PARSE_GET,     get_array,     TRACK_VARS_GET)
		PARSE_CASE(PARSE_COOKIE,  cookie_array,  TRACK_VARS_COOKIE)
		PARSE_CASE(PARSE_SERVER,  server_array,  TRACK_VARS_SERVER)
		PARSE_CASE(PARSE_ENV,     env_array,     TRACK_VARS_ENV)

		case PARSE_STRING: /* PARSE_STRING is used by parse_str() function */
			retval = 1;
			break;
	}

	if (array_ptr) {
		/* Make a copy of the variable name, as php_register_variable_ex seems to
		 * modify it */
		orig_var = estrdup(var);

		/* Store the RAW variable internally */
		/* FIXME: Should not use php_register_variable_ex as that also registers
		 * globals when register_globals is turned on */
		Z_STRLEN(raw_var) = val_len;
		Z_STRVAL(raw_var) = estrndup(*val, val_len + 1);
		Z_TYPE(raw_var) = IS_STRING;

		php_register_variable_ex(var, &raw_var, array_ptr TSRMLS_CC);
	}

	/* Register mangled variable */
	/* FIXME: Should not use php_register_variable_ex as that also registers
	 * globals when register_globals is turned on */
	Z_STRLEN(new_var) = val_len;
	Z_STRVAL(new_var) = estrndup(*val, val_len + 1);
	Z_TYPE(new_var) = IS_STRING;

	if (val_len) {
		if (! (IF_G(default_filter) == FS_UNSAFE_RAW)) {
			php_zval_filter(&new_var, IF_G(default_filter), IF_G(default_filter_flags), NULL, NULL/*charset*/ TSRMLS_CC);
		}
	}

	if (orig_array_ptr) {
		php_register_variable_ex(orig_var, &new_var, orig_array_ptr TSRMLS_CC);
		efree(orig_var);
	}

	if (retval) {
		if (new_val_len) {
			*new_val_len = Z_STRLEN(new_var);
		}
		efree(*val);
		if (Z_STRLEN(new_var)) {
			*val = estrndup(Z_STRVAL(new_var), Z_STRLEN(new_var) + 1);
			zval_dtor(&new_var);
		} else {
			*val = estrdup("");
		}
	}

	return retval;
}
/* }}} */

/* {{{ static void php_zval_filter_recursive(zval *array, long filter, long flags, char *charset TSRMLS_DC)
 */
static void php_zval_filter_recursive(zval *value, long filter, long flags, zval *options, char *charset TSRMLS_DC)
{
	zval **element;
	HashPosition pos;

	if (Z_TYPE_P(value) == IS_ARRAY) {
		for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(value), &pos);
			 zend_hash_get_current_data_ex(Z_ARRVAL_P(value), (void **) &element, &pos) == SUCCESS;
			 zend_hash_move_forward_ex(Z_ARRVAL_P(value), &pos)) {
				php_zval_filter_recursive(*element, filter, flags, options, charset TSRMLS_CC);
		}
	} else {
		php_zval_filter(value, filter, flags, options, charset TSRMLS_CC);
	}
}
/* }}} */

#define FIND_SOURCE(a,t)                              \
		array_ptr = IF_G(a);                          \
		break;

/* {{{ proto mixed input_has_variable(constant type, string variable_name)
 */
PHP_FUNCTION(input_has_variable)
{
	long        arg;
	char       *var;
	int         var_len;
	zval      **tmp;
	zval       *array_ptr = NULL;
	HashTable  *hash_ptr;
	int         found = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &arg, &var, &var_len) == FAILURE) {
		return;
	}

	switch (arg) {
		case PARSE_GET:     FIND_SOURCE(get_array,     TRACK_VARS_GET)
		case PARSE_POST:    FIND_SOURCE(post_array,    TRACK_VARS_POST)
		case PARSE_COOKIE:  FIND_SOURCE(cookie_array,  TRACK_VARS_COOKIE)
		case PARSE_SERVER:  FIND_SOURCE(server_array,  TRACK_VARS_SERVER)
		case PARSE_ENV:     FIND_SOURCE(env_array,     TRACK_VARS_ENV)
	}

	if (!array_ptr) {
		RETURN_FALSE;
	}

	if (!found) {
		hash_ptr = HASH_OF(array_ptr);

		if (hash_ptr && zend_hash_find(hash_ptr, var, var_len + 1, (void **)&tmp) == SUCCESS) {
			RETURN_TRUE;
		}
	}

	RETURN_FALSE;
}
/* }}} */

/* {{{ proto mixed input_get(constant type, string variable_name [, int filter [, mixed flags [, string charset]]])
 */
PHP_FUNCTION(input_get)
{
	long        arg, filter = FS_DEFAULT;
	char       *var, *charset = NULL;
	int         var_len, charset_len;
	zval       *flags = NULL;
	zval      **tmp;
	zval       *array_ptr = NULL, *array_ptr2 = NULL, *array_ptr3 = NULL;
	HashTable  *hash_ptr;
	int         found = 0;
	int         filter_flags = 0;
	zval       *options = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls|lzs", &arg, &var, &var_len, &filter, &flags, &charset, &charset_len) == FAILURE) {
		return;
	}

	if (flags) {
		switch (Z_TYPE_P(flags)) {
			case IS_ARRAY:
				options = flags;
				break;

			case IS_STRING:
			case IS_BOOL:
			case IS_LONG:
				convert_to_long(flags);
				filter_flags = Z_LVAL_P(flags);
				options = NULL;
				break;
		}
	}

	switch(arg) {
		case PARSE_GET:     FIND_SOURCE(get_array,     TRACK_VARS_GET)
		case PARSE_POST:    FIND_SOURCE(post_array,    TRACK_VARS_POST)
		case PARSE_COOKIE:  FIND_SOURCE(cookie_array,  TRACK_VARS_COOKIE)
		case PARSE_SERVER:  FIND_SOURCE(server_array,  TRACK_VARS_SERVER)
		case PARSE_ENV:     FIND_SOURCE(env_array,     TRACK_VARS_ENV)

		case PARSE_SESSION:
			/* FIXME: Implement session source */
			break;

		case PARSE_REQUEST:
			if (PG(variables_order)) {
				zval **a_ptr = &array_ptr;
				char *p, *variables_order = PG(variables_order);

				for (p = variables_order; p && *p; p++) {
					switch (*p) {
						case 'p':
						case 'P':
							if (IF_G(default_filter) != FS_UNSAFE_RAW) {
								*a_ptr = IF_G(post_array);
							} else {
								*a_ptr = PG(http_globals)[TRACK_VARS_POST];
							}
							break;
						case 'g':
						case 'G':
							if (IF_G(default_filter) != FS_UNSAFE_RAW) {
								*a_ptr = IF_G(get_array);
							} else {
								*a_ptr = PG(http_globals)[TRACK_VARS_GET];
							}
							break;
						case 'c':
						case 'C':
							if (IF_G(default_filter) != FS_UNSAFE_RAW) {
								*a_ptr = IF_G(cookie_array);
							} else {
								*a_ptr = PG(http_globals)[TRACK_VARS_COOKIE];
							}
							break;
					}
					if (array_ptr && !array_ptr2) {
						a_ptr = &array_ptr2;
						continue;
					}
					if (array_ptr2 && !array_ptr3) { 
						a_ptr = &array_ptr3;
					}
				}
			} else {
				FIND_SOURCE(get_array, TRACK_VARS_GET)
			}

	}

	if (!array_ptr) {
		RETURN_FALSE;
	}

	if (array_ptr3) {
		hash_ptr = HASH_OF(array_ptr3);
		if (hash_ptr && zend_hash_find(hash_ptr, var, var_len + 1, (void **)&tmp) == SUCCESS) {
			*return_value = **tmp;
			found = 1;
		} 
	}

	if (array_ptr2 && !found) {
		hash_ptr = HASH_OF(array_ptr2);
		if (hash_ptr && zend_hash_find(hash_ptr, var, var_len + 1, (void **)&tmp) == SUCCESS) {
			*return_value = **tmp;
			found = 1;
		}
	}

	if (!found) {
		hash_ptr = HASH_OF(array_ptr);

		if (hash_ptr && zend_hash_find(hash_ptr, var, var_len + 1, (void **)&tmp) == SUCCESS) {
			*return_value = **tmp;
			found = 1;
		}
	}

	if (found) {
		zval_copy_ctor(return_value);  /* Watch out for empty strings */
		php_zval_filter_recursive(return_value, filter, filter_flags, options, charset TSRMLS_CC);
	} else {
		RETVAL_FALSE;
	}
}
/* }}} */

/* {{{ proto input_filters_list()
 * Returns a list of all supported filters */
PHP_FUNCTION(input_filters_list)
{
	int i, size = sizeof(filter_list) / sizeof(filter_list_entry);

	array_init(return_value);
	for (i = 0; i < size; ++i) {
		add_next_index_string(return_value, filter_list[i].name, 1);
	}
}
/* }}} */

/* {{{ proto input_name_to_filter(string filtername)
 * Returns the filter ID belonging to a named filter */
PHP_FUNCTION(input_name_to_filter)
{
	int i, filter_len;
	int size = sizeof(filter_list) / sizeof(filter_list_entry);
	char *filter;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filter, &filter_len) == FAILURE) {
		return;
	}

	for (i = 0; i < size; ++i) {
		if (strcmp(filter_list[i].name, filter) == 0) {
			RETURN_LONG(filter_list[i].id);
		}
	}
	RETURN_NULL();
}
/* }}} */

/* {{{ proto filter_data(mixed variable, int filter [, mixed filter_options [, string charset ]])
 */
PHP_FUNCTION(filter_data)
{
	long        filter = FS_DEFAULT;
	char       *charset = NULL;
	int         charset_len;
	zval       *var, *flags = NULL;
	int         filter_flags = 0;
	zval       *options = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z/l|zs", &var, &filter, &flags, &charset, &charset_len) == FAILURE) {
		return;
	}

	if (flags) {
		switch (Z_TYPE_P(flags)) {
			case IS_ARRAY:
				options = flags;
				break;

			case IS_STRING:
			case IS_BOOL:
			case IS_LONG:
				convert_to_long(flags);
				filter_flags = Z_LVAL_P(flags);
				options = NULL;
				break;
		}
	}

	php_zval_filter_recursive(var, filter, filter_flags, options, charset TSRMLS_CC);
	RETURN_ZVAL(var, 1, 0);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
