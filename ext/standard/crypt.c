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
   | Authors: Stig Bakken <ssb@gaurdian.no>                               |
   |          Zeev Suraski <zeev@zend.com>                                |
   |          Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   +----------------------------------------------------------------------+
 */
/* $Id$ */
#include <stdlib.h>

#include "php.h"

#if HAVE_CRYPT

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_CRYPT_H
#include <crypt.h>
#endif
#if TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef PHP_WIN32
#include <process.h>
extern char *crypt(char *__key,char *__salt);
#endif

#include "php_crypt.h"

/* 
   The capabilities of the crypt() function is determined by the test programs
   run by configure from aclocal.m4.  They will set PHP_STD_DES_CRYPT,
   PHP_EXT_DES_CRYPT, PHP_MD5_CRYPT and PHP_BLOWFISH_CRYPT as appropriate 
   for the target platform
*/
#if PHP_STD_DES_CRYPT
#define PHP_MAX_SALT_LEN 2
#endif
#if PHP_EXT_DES_CRYPT
#undef PHP_MAX_SALT_LEN
#define PHP_MAX_SALT_LEN 9
#endif
#if PHP_MD5_CRYPT
#undef PHP_MAX_SALT_LEN
#define PHP_MAX_SALT_LEN 12
#endif
#if PHP_BLOWFISH_CRYPT
#undef PHP_MAX_SALT_LEN
#define PHP_MAX_SALT_LEN 17
#endif

 /*
  * If the configure-time checks fail, we provide DES.
  * XXX: This is a hack. Fix the real problem
  */

#ifndef PHP_MAX_SALT_LEN
#define PHP_MAX_SALT_LEN 2
#undef PHP_STD_DES_CRYPT
#define PHP_STD_DES_CRYPT 1
#endif

#if HAVE_LRAND48
#define PHP_CRYPT_RAND lrand48()
#else
#if HAVE_RANDOM
#define PHP_CRYPT_RAND random()
#else
#define PHP_CRYPT_RAND rand()
#endif
#endif

PHP_MINIT_FUNCTION(crypt)
{
#if PHP_STD_DES_CRYPT
    REGISTER_LONG_CONSTANT("CRYPT_SALT_LENGTH", 2, CONST_CS | CONST_PERSISTENT);
#else
#if PHP_MD5_CRYPT
    REGISTER_LONG_CONSTANT("CRYPT_SALT_LENGTH", 12, CONST_CS | CONST_PERSISTENT);
#endif
#endif
    REGISTER_LONG_CONSTANT("CRYPT_STD_DES", PHP_STD_DES_CRYPT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CRYPT_EXT_DES", PHP_EXT_DES_CRYPT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CRYPT_MD5", PHP_MD5_CRYPT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("CRYPT_BLOWFISH", PHP_BLOWFISH_CRYPT, CONST_CS | CONST_PERSISTENT);
    return SUCCESS;
}

static unsigned char itoa64[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static void php_to64(char *s, long v, int n)	{
	while (--n >= 0) {
		*s++ = itoa64[v&0x3f]; 		
		v >>= 6;
	} 
} 
#endif /* HAVE_CRYPT */

/* {{{ proto string crypt(string str [, string salt])
   Encrypt a string */
PHP_FUNCTION(crypt)
{
#if HAVE_CRYPT
	char salt[PHP_MAX_SALT_LEN+1];
	pval **arg1, **arg2;

	salt[0]=salt[PHP_MAX_SALT_LEN]='\0';
	/* This will produce suitable results if people depend on DES-encryption
	   available (passing always 2-character salt). At least for glibc6.1 */
	memset(&salt[1], '$', PHP_MAX_SALT_LEN-1);

	switch (ARG_COUNT(ht)) {
		case 1:
			if (zend_get_parameters_ex(1, &arg1)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &arg1, &arg2)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(arg2);
			memcpy(salt, (*arg2)->value.str.val, MIN(PHP_MAX_SALT_LEN,(*arg2)->value.str.len));
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	convert_to_string_ex(arg1);

	/* The automatic salt generation only covers standard DES and md5-crypt */
	if(!*salt) {
#if HAVE_SRAND48
		srand48((unsigned int) time(0) * getpid());
#else
#if HAVE_SRANDOM
		srandom((unsigned int) time(0) * getpid());
#else
		srand((unsigned int) time(0) * getpid());
#endif
#endif

#if PHP_STD_DES_CRYPT
		php_to64(&salt[0], PHP_CRYPT_RAND, 2);
		salt[2] = '\0';
#else
#if PHP_MD5_CRYPT
		strcpy(salt, "$1$");
		php_to64(&salt[3], PHP_CRYPT_RAND, 4);
		php_to64(&salt[7], PHP_CRYPT_RAND, 4);
		strcpy(&salt[11], "$");
#endif
#endif
	}

	return_value->value.str.val = (char *) crypt((*arg1)->value.str.val, salt);
	return_value->value.str.len = strlen(return_value->value.str.val);
	return_value->type = IS_STRING;
	pval_copy_constructor(return_value);
#else
    PHP_NOT_IN_THIS_BUILD();
#endif /* HAVE_CRYPT */
}
/* }}} */



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
