/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Sascha Schumann <sascha@schumann.cx>                        |
   +----------------------------------------------------------------------+
 */

#include "php.h"

#if HAVE_LIBMHASH

#include "php_mhash.h"

#include "mhash.h"

function_entry mhash_functions[] = {
	PHP_FE(mhash_get_block_size, NULL)
	PHP_FE(mhash_get_hash_name, NULL)
	PHP_FE(mhash_count, NULL)
	PHP_FE(mhash, NULL)
	{0},
};

static PHP_MINIT_FUNCTION(mhash);

zend_module_entry mhash_module_entry = {
	"mhash", 
	mhash_functions,
	PHP_MINIT(mhash), NULL,
	NULL, NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES,
};

#ifdef COMPILE_DL_MHASH
ZEND_GET_MODULE(mhash)
#endif

#define MHASH_FAILED_MSG "mhash initialization failed"

static PHP_MINIT_FUNCTION(mhash)
{
	int i;
	char *name;
	char buf[128];

	for(i = 0; i <= mhash_count(); i++) {
		name = mhash_get_hash_name(i);
		if(name) {
			snprintf(buf, 127, "MHASH_%s", name);
			zend_register_long_constant(buf, strlen(buf) + 1, i, CONST_PERSISTENT, module_number ELS_CC);
			free(name);
		}
	}
	
	return SUCCESS;
}

/* {{{ proto int mhash_count()
   get the number of available hashes */
PHP_FUNCTION(mhash_count)
{
	RETURN_LONG(mhash_count());
}
/* }}} */

/* {{{ proto int mhash_get_block_size(int hash)
   get the block size of hash */
PHP_FUNCTION(mhash_get_block_size)
{
	pval **hash;

	if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &hash) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(hash);

	RETURN_LONG(mhash_get_block_size((*hash)->value.lval));
}
/* }}} */

/* {{{ proto string mhash_get_hash_name(int hash)
   get the name of hash */
PHP_FUNCTION(mhash_get_hash_name)
{
	pval **hash;
	char *name;

	if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &hash) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(hash);

	name = mhash_get_hash_name((*hash)->value.lval);
	if(name) {
		RETVAL_STRING(name, 1);
		free(name);
	} else {
		RETVAL_FALSE;
	}
}
/* }}} */

/* {{{ proto string mhash(int hash, string data)
   hash data with hash */
PHP_FUNCTION(mhash)
{
	pval **hash, **data;
	MHASH td;
	int bsize;
	unsigned char *hash_data;

	if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &hash, &data) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(hash);
	convert_to_string_ex(data);

	bsize = mhash_get_block_size((*hash)->value.lval);
	td = mhash_init((*hash)->value.lval);
	if(td == MHASH_FAILED) {
		php_error(E_WARNING, MHASH_FAILED_MSG);
		RETURN_FALSE;
	}

	mhash(td, (*data)->value.str.val, (*data)->value.str.len);

	hash_data = (unsigned char *) mhash_end(td);
	
	if (hash_data) {
		RETVAL_STRINGL(hash_data, bsize, 1);
	
		free(hash_data);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

#endif
