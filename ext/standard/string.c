/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   |          Stig S�ther Bakken <ssb@fast.no>                            |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

/* Synced with php 3.0 revision 1.193 1999-06-16 [ssb] */

#include <stdio.h>
#include "php.h"
#include "reg.h"
#include "php_string.h"
#include "php_variables.h"
#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif
#include "zend_execute.h"
#include "php_globals.h"
#include "basic_functions.h"

int php_tag_find(char *tag, int len, char *set);

/* this is read-only, so it's ok */
static char hexconvtab[] = "0123456789abcdef";

static char *php_bin2hex(const unsigned char *old, const size_t oldlen, size_t *newlen)
{
	unsigned char *new = NULL;
	size_t i, j;

	new = (char *) emalloc(oldlen * 2 * sizeof(char));
	if(!new) {
		return new;
	}
	
	for(i = j = 0; i < oldlen; i++) {
		new[j++] = hexconvtab[old[i] >> 4];
		new[j++] = hexconvtab[old[i] & 15];
	}

	if(newlen) *newlen = oldlen * 2 * sizeof(char);

	return new;
}

/* proto bin2hex(string data)
   converts the binary representation of data to hex */
PHP_FUNCTION(bin2hex)
{
	zval **data;
	char *new;
	size_t newlen;

	if(ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &data) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(data);

	new = php_bin2hex((*data)->value.str.val, (*data)->value.str.len, &newlen);
	
	if(!new) {
		RETURN_FALSE;
	}

	RETURN_STRINGL(new, newlen, 0);
}


/* {{{ proto int strspn(string str, string mask)
   Find length of initial segment consisting entirely of characters found in mask */
PHP_FUNCTION(strspn)
{
	zval **s1,**s2;
	
	if (ARG_COUNT(ht)!=2 || zend_get_parameters_ex(2, &s1, &s2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(s1);
	convert_to_string_ex(s2);
	RETURN_LONG(php_strspn((*s1)->value.str.val, (*s2)->value.str.val,
                           (*s1)->value.str.val + (*s1)->value.str.len,
						   (*s2)->value.str.val + (*s2)->value.str.len));
}
/* }}} */

/* {{{ proto int strcspn(string str, string mask)
   Find length of initial segment consisting entirely of characters not found in mask */
PHP_FUNCTION(strcspn)
{
	zval **s1,**s2;
	
	if (ARG_COUNT(ht)!=2 || zend_get_parameters_ex(2, &s1, &s2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(s1);
	convert_to_string_ex(s2);
	RETURN_LONG(php_strcspn((*s1)->value.str.val, (*s2)->value.str.val,
				            (*s1)->value.str.val + (*s1)->value.str.len,
							(*s2)->value.str.val + (*s2)->value.str.len));
}
/* }}} */

PHPAPI void php_trim(zval *str, zval * return_value, int mode)
/* mode 1 : trim left
   mode 2 : trim right
   mode 3 : trim left and right
*/
{
	register int i;
	int len = str->value.str.len;
	int trimmed = 0;
	char *c = str->value.str.val;

	if (mode & 1) {
		for (i = 0; i < len; i++) {
			if (c[i] == ' ' || c[i] == '\n' || c[i] == '\r' ||
				c[i] == '\t' || c[i] == '\v') {
				trimmed++;
			} else {
				break;
			}
		}
		len -= trimmed;
		c += trimmed;
	}
	if (mode & 2) {
		for (i = len - 1; i >= 0; i--) {
			if (c[i] == ' ' || c[i] == '\n' || c[i] == '\r' ||
				c[i] == '\t' || c[i] == '\v') {
				len--;
			} else {
				break;
			}
		}
	}
	RETVAL_STRINGL(c, len, 1);
}

/* {{{ proto string rtrim(string str)
   An alias for chop */
/* }}} */

/* {{{ proto string chop(string str)
   Remove trailing whitespace */
PHP_FUNCTION(chop)
{
	zval **str;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	if ((*str)->type == IS_STRING) {
		php_trim(*str, return_value, 2);
		return;
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto string trim(string str)
   Strip whitespace from the beginning and end of a string */
PHP_FUNCTION(trim)
{
	zval **str;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	if ((*str)->type == IS_STRING) {
		php_trim(*str, return_value, 3);
		return;
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto string ltrim(string str)
   Strip whitespace from the beginning of a string */
PHP_FUNCTION(ltrim)
{
	zval **str;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);
	if ((*str)->type == IS_STRING) {
		php_trim(*str, return_value, 1);
		return;
	}
	RETURN_FALSE;
}
/* }}} */

PHPAPI void php_explode(zval *delim, zval *str, zval *return_value) 
{
	char *p1, *p2, *endp;
	int i = 0;

	endp = str->value.str.val + str->value.str.len;

	p1 = str->value.str.val;
	p2 = php_memnstr(str->value.str.val, delim->value.str.val, delim->value.str.len, endp);

	if (p2 == NULL) {
		add_index_stringl(return_value, i++, p1, str->value.str.len, 1);
	} else {
		do {
			add_index_stringl(return_value, i++, p1, p2-p1, 1);
			p1 = p2 + delim->value.str.len;
		} while ((p2 = php_memnstr(p1, delim->value.str.val, delim->value.str.len, endp)) != NULL);

		if (p1 <= endp) {
			add_index_stringl(return_value, i++, p1, endp-p1, 1);
		}
	}
}

/* {{{ proto array explode(string separator, string str)
   Split a string on string separator and return array of components */
PHP_FUNCTION(explode)
{
	zval **str, **delim;

	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &delim, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(str);
	convert_to_string_ex(delim);

	if (! (*delim)->value.str.len) {
		php_error(E_WARNING,"Empty delimiter");
		RETURN_FALSE;
	}

	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}

	php_explode(*delim, *str, return_value);
}
/* }}} */


/* {{{ proto string join(array src, string glue)
   An alias for implode */
/* }}} */
PHPAPI void php_implode(zval *delim, zval *arr, zval *return_value) 
{
	zval **tmp;
	int len = 0, count = 0, target = 0;

	/* convert everything to strings, and calculate length */
	zend_hash_internal_pointer_reset(arr->value.ht);
	while (zend_hash_get_current_data(arr->value.ht, (void **) &tmp) == SUCCESS) {
		convert_to_string_ex(tmp);
		len += (*tmp)->value.str.len;
		if (count>0) {
			len += delim->value.str.len;
		}
		count++;
		zend_hash_move_forward(arr->value.ht);
	}

	/* do it */
	return_value->value.str.val = (char *) emalloc(len + 1);
	return_value->value.str.val[0] = '\0';
	return_value->value.str.val[len] = '\0';
	zend_hash_internal_pointer_reset(arr->value.ht);
	while (zend_hash_get_current_data(arr->value.ht, (void **) &tmp) == SUCCESS) {
		count--;
		memcpy(return_value->value.str.val + target, (*tmp)->value.str.val,
               (*tmp)->value.str.len);
		target += (*tmp)->value.str.len; 
		if (count > 0) {
			memcpy(return_value->value.str.val + target, delim->value.str.val,
                   delim->value.str.len);
			target += delim->value.str.len;
		}
		zend_hash_move_forward(arr->value.ht);
	}
	return_value->type = IS_STRING;
	return_value->value.str.len = len;
}


/* {{{ proto string implode(array src, string glue)
   Join array elements placing glue string between items and return one string */
PHP_FUNCTION(implode)
{
	zval **arg1, **arg2, *delim, *arr;
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if ((*arg1)->type == IS_ARRAY && (*arg2)->type == IS_STRING) {
		SEPARATE_ZVAL(arg1);
		arr = *arg1;
		delim = *arg2;
	} else if ((*arg2)->type == IS_ARRAY) {
		SEPARATE_ZVAL(arg2)
		arr = *arg2;
		convert_to_string_ex(arg1);
		delim = *arg1;
	} else {
		php_error(E_WARNING, "Bad arguments to %s()",
				   get_active_function_name());
		return;
	}
	php_implode(delim, arr, return_value);
}
/* }}} */


/* {{{ proto string strtok([string str,] string token)
   Tokenize a string */
PHP_FUNCTION(strtok)
{
	zval **str, **tok;
	char *token = NULL, *tokp=NULL;
	char *first = NULL;
	int argc;
	BLS_FETCH();
	
	argc = ARG_COUNT(ht);

	if ((argc == 1 && zend_get_parameters_ex(1, &tok) == FAILURE) ||
		(argc == 2 && zend_get_parameters_ex(2, &str, &tok) == FAILURE) ||
		argc < 1 || argc > 2) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(tok);
	tokp = token = (*tok)->value.str.val;

	if (argc == 2) {
		convert_to_string_ex(str);

		STR_FREE(BG(strtok_string));
		BG(strtok_string) = estrndup((*str)->value.str.val,(*str)->value.str.len);
		BG(strtok_pos1) = BG(strtok_string);
		BG(strtok_pos2) = NULL;
	}
	if (BG(strtok_pos1) && *BG(strtok_pos1)) {
		for ( /* NOP */ ; token && *token; token++) {
			BG(strtok_pos2) = strchr(BG(strtok_pos1), (int) *token);
			if (!first || (BG(strtok_pos2) && BG(strtok_pos2) < first)) {
				first = BG(strtok_pos2);
			}
		}						/* NB: token is unusable now */

		BG(strtok_pos2) = first;
		if (BG(strtok_pos2)) {
			*BG(strtok_pos2) = '\0';
		}
		RETVAL_STRING(BG(strtok_pos1),1);
#if 0
		/* skip 'token' white space for next call to strtok */
		while (BG(strtok_pos2) && 
			strchr(tokp, *(BG(strtok_pos2)+1))) {
			BG(strtok_pos2)++;
		}
#endif
		if (BG(strtok_pos2))
			BG(strtok_pos1) = BG(strtok_pos2) + 1;
		else
			BG(strtok_pos1) = NULL;
	} else {
		RETVAL_FALSE;
	}
}
/* }}} */

PHPAPI char *php_strtoupper(char *s, size_t len)
{
	char *c;
	int ch;
	size_t i;

	c = s;
	for (i=0; i<len; i++) {
		ch = toupper((unsigned char)*c);
		*c++ = ch;
	}
	return (s);
}

/* {{{ proto string strtoupper(string str)
   Make a string uppercase */
PHP_FUNCTION(strtoupper)
{
	zval **arg;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg)) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);

	*return_value = **arg;
	zval_copy_ctor(return_value);
	php_strtoupper(return_value->value.str.val, return_value->value.str.len);
}
/* }}} */


PHPAPI char *php_strtolower(char *s, size_t len)
{
	register int ch;
	char *c;
	size_t i;

	c = s;
	for (i=0; i<len; i++) {
		ch = tolower((unsigned char)*c);
		*c++ = ch;
	}
	return (s);
}

/* {{{ proto string strtolower(string str)
   Make a string lowercase */
PHP_FUNCTION(strtolower)
{
	zval **str;
	char *ret;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &str)) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	*return_value = **str;
	zval_copy_ctor(return_value);
	ret = php_strtolower(return_value->value.str.val, return_value->value.str.len);
}
/* }}} */

/* {{{ proto string basename(string path)
   Return the filename component of the path */
PHP_FUNCTION(basename)
{
	zval **str;
	char *ret, *c;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &str)) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);
	ret = estrdup((*str)->value.str.val);
	c = ret + (*str)->value.str.len -1;	
	while (*c == '/'
#ifdef MSVC5
		   || *c == '\\'
#endif
		)
		c--;
	*(c + 1) = '\0';	
	if ((c = strrchr(ret, '/'))
#ifdef MSVC5
		|| (c = strrchr(ret, '\\'))
#endif
		) {
		RETVAL_STRING(c + 1,1);
	} else {
		RETVAL_STRING((*str)->value.str.val,1);
	}
	efree(ret);
}
/* }}} */

PHPAPI void php_dirname(char *str, int len) {
	register char *c;

	c = str + len - 1;
	while (*c == '/'
#ifdef MSVC5
		   || *c == '\\'
#endif
		)
		c--; /* strip trailing slashes */
	*(c + 1) = '\0';
	if ((c = strrchr(str, '/'))
#ifdef MSVC5
		|| (c = strrchr(str, '\\'))
#endif
		)
		*c='\0';
}

/* {{{ proto string dirname(string path)
   Return the directory name component of the path */
PHP_FUNCTION(dirname)
{
	zval **str;
	char *ret;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &str)) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);
	ret = estrdup((*str)->value.str.val);
	php_dirname(ret,(*str)->value.str.len);
	RETVAL_STRING(ret,1);
	efree(ret);
}
/* }}} */


/* case insensitve strstr */
PHPAPI char *php_stristr(unsigned char *s, unsigned char *t,
                         size_t s_len, size_t t_len)
{
	php_strtolower(s, s_len);
	php_strtolower(t, t_len);
	return php_memnstr(s, t, t_len, s + s_len);
}

PHPAPI size_t php_strspn(char *s1, char *s2, char *s1_end, char *s2_end)
{
	register const char *p = s1, *spanp;
	register char c = *p;

cont:
	for (spanp = s2; p != s1_end && spanp != s2_end;)
		if (*spanp++ == c) {
			c = *(++p);
			goto cont;
		}
	return (p - s1);
}

PHPAPI size_t php_strcspn(char *s1, char *s2, char *s1_end, char *s2_end)
{
	register const char *p, *spanp;
	register char c = *s1;

	for (p = s1;;) {
		spanp = s2;
		do {
			if (*spanp == c || p == s1_end)
				return (p - s1);
		} while (spanp++ < s2_end);
		c = *(++p);
	}
	/* NOTREACHED */
}

/* {{{ proto string stristr(string haystack, string needle)
   Find first occurrence of a string within another, case insensitive */
PHP_FUNCTION(stristr)
{
	zval **haystack, **needle;
	char *found = NULL;
	char needle_char[2];
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &haystack, &needle) ==
		FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(haystack);

	if ((*needle)->type == IS_STRING) {
		if ((*needle)->value.str.len==0) {
			php_error(E_WARNING,"Empty delimiter");
			RETURN_FALSE;
		}
		found = php_stristr((*haystack)->value.str.val, (*needle)->value.str.val,
							(*haystack)->value.str.len, (*needle)->value.str.len);
	} else {
		convert_to_long_ex(needle);
		needle_char[0] = tolower((char) (*needle)->value.lval);
		needle_char[1] = '\0';
		found = php_stristr((*haystack)->value.str.val, needle_char,
				            (*haystack)->value.str.len, 1);
	}

	if (found) {
		RETVAL_STRING(found, 1);
	} else {
		RETVAL_FALSE;
	}
}
/* }}} */

/* {{{ proto string strstr(string haystack, string needle)
   Find first occurrence of a string within another */
PHP_FUNCTION(strstr)
{
	zval **haystack, **needle;
	char *haystack_end;
	char *found = NULL;
	char needle_char[2];
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &haystack, &needle) ==
		FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(haystack);
	haystack_end = (*haystack)->value.str.val + (*haystack)->value.str.len;

	if ((*needle)->type == IS_STRING) {
		if ((*needle)->value.str.len==0) {
			php_error(E_WARNING,"Empty delimiter");
			RETURN_FALSE;
		}
		found = php_memnstr((*haystack)->value.str.val, (*needle)->value.str.val,
                            (*needle)->value.str.len, haystack_end);
	} else {
		convert_to_long_ex(needle);
		needle_char[0] = (char) (*needle)->value.lval;
		needle_char[1] = '\0';
		found = php_memnstr((*haystack)->value.str.val, needle_char, 1, haystack_end);
	}


	if (found) {
		RETVAL_STRING(found, 1);
	} else {
		RETVAL_FALSE;
	}
}
/* }}} */

/* {{{ proto string strchr(string haystack, string needle)
   An alias for strstr */
/* }}} */

/* {{{ proto int strpos(string haystack, string needle [, int offset])
   Find position of first occurrence of a string within another */
PHP_FUNCTION(strpos)
{
	zval **haystack, **needle, **OFFSET;
	int offset = 0;
	char *found = NULL;
	char *endp;
	char *startp;
	
	switch(ARG_COUNT(ht)) {
	case 2:
		if (zend_get_parameters_ex(2, &haystack, &needle) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 3:
		if (zend_get_parameters_ex(3, &haystack, &needle, &OFFSET) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long_ex(OFFSET);
		offset = (*OFFSET)->value.lval;
		if (offset < 0) {
			php_error(E_WARNING,"offset not contained in string");
			RETURN_FALSE;
		}	
		break;
	default:
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(haystack);

	if (offset > (*haystack)->value.str.len) {
		php_error(E_WARNING,"offset not contained in string");
		RETURN_FALSE;
	}

	startp = (*haystack)->value.str.val;
	startp+= offset;

	endp = (*haystack)->value.str.val;
	endp+= (*haystack)->value.str.len;

	if ((*needle)->type == IS_STRING) {
		if ((*needle)->value.str.len==0) {
			php_error(E_WARNING,"Empty delimiter");
			RETURN_FALSE;
		}
		found = php_memnstr(startp, (*needle)->value.str.val, (*needle)->value.str.len, endp);
	} else {
		char buf;

		convert_to_long_ex(needle);
		buf = (char) (*needle)->value.lval;

		found = php_memnstr(startp, &buf, 1, endp);
	}

	if (found) {
		RETVAL_LONG(found - (*haystack)->value.str.val);
	} else {
		RETVAL_FALSE;
	}
}
/* }}} */

/* {{{ proto int strrpos(string haystack, string needle)
   Find the last occurrence of a character in a string within another */
PHP_FUNCTION(strrpos)
{
	zval **haystack, **needle;
	char *found = NULL;
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &haystack, &needle) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(haystack);

	if ((*needle)->type == IS_STRING) {
		found = strrchr((*haystack)->value.str.val, *(*needle)->value.str.val);
	} else {
		convert_to_long_ex(needle);
		found = strrchr((*haystack)->value.str.val, (char) (*needle)->value.lval);
	}

	if (found) {
		RETVAL_LONG((*haystack)->value.str.len - strlen(found));
	} else {
		RETVAL_FALSE;
	}
}
/* }}} */

/* {{{ proto string strrchr(string haystack, string needle)
   Find the last occurrence of a character in a string within another */
PHP_FUNCTION(strrchr)
{
	zval **haystack, **needle;
	char *found = NULL;
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &haystack, &needle) ==
		FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(haystack);

	if ((*needle)->type == IS_STRING) {
		found = strrchr((*haystack)->value.str.val, *(*needle)->value.str.val);
	} else {

		convert_to_long_ex(needle);
		found = strrchr((*haystack)->value.str.val, (*needle)->value.lval);
	}


	if (found) {
		RETVAL_STRING(found,1);
	} else {
		RETVAL_FALSE;
	}
}
/* }}} */

static char *php_chunk_split(char *src, int srclen, char *end, int endlen,
							 int chunklen, int *destlen)
{
	char *dest;
	char *p, *q;
	int chunks; /* complete chunks! */
	int restlen;

	chunks = srclen / chunklen;
	restlen = srclen - chunks * chunklen; /* srclen % chunklen */

	dest = emalloc((srclen + (chunks + 1) * endlen + 1) * sizeof(char));

	for(p = src, q = dest; p < (src + srclen - chunklen + 1); ) {
		memcpy(q, p, chunklen);
		q += chunklen;
		memcpy(q, end, endlen);
		q += endlen;
		p += chunklen;
	}

	if(restlen) {
		memcpy(q, p, restlen);
		q += restlen;
		memcpy(q, end, endlen);
		q += endlen;
	}

	*q = '\0';
	if (destlen) {
		*destlen = q - dest;
	}

	return(dest);
}

/* {{{ proto string chunk_split(string str [, int chunklen [, string ending]])
   Return split line */
PHP_FUNCTION(chunk_split) 
{
	zval **p_str, **p_chunklen, **p_ending;
	int argc;
	char *result;
	char *end    = "\r\n";
	int endlen   = 2;
	int chunklen = 76;
	int result_len;

	argc = ARG_COUNT(ht);

	if (argc < 1 || argc > 3 ||
		zend_get_parameters_ex(argc, &p_str, &p_chunklen, &p_ending) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	switch(argc) {
		case 3:
			convert_to_string_ex(p_ending);
			end    = (*p_ending)->value.str.val;
			endlen = (*p_ending)->value.str.len;
		case 2:
			convert_to_long_ex(p_chunklen);
			chunklen = (*p_chunklen)->value.lval;
		case 1:
			convert_to_string_ex(p_str);
	}
			
	if(chunklen == 0) {
		php_error(E_WARNING, "chunk length is 0");
		RETURN_FALSE;
	}
	
	result = php_chunk_split((*p_str)->value.str.val, (*p_str)->value.str.len,
						     end, endlen, chunklen, &result_len);
	
	if(result) {
		RETVAL_STRINGL(result, result_len, 0);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string substr(string str, int start [, int length])
   Return part of a string */
PHP_FUNCTION(substr)
{
	zval **string, **from, **len;
	int argc, l;
	int f;
	
	argc = ARG_COUNT(ht);

	if ((argc == 2 && zend_get_parameters_ex(2, &string, &from) == FAILURE) ||
		(argc == 3 && zend_get_parameters_ex(3, &string, &from, &len) == FAILURE) ||
		argc < 2 || argc > 3) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(string);
	convert_to_long_ex(from);
	f = (*from)->value.lval;

	if (argc == 2) {
		l = (*string)->value.str.len;
	} else {
		convert_to_long_ex(len);
		l = (*len)->value.lval;
	}

	/* if "from" position is negative, count start position from the end
	 * of the string
	 */
	if (f < 0) {
		f = (*string)->value.str.len + f;
		if (f < 0) {
			f = 0;
		}
	}

	/* if "length" position is negative, set it to the length
	 * needed to stop that many chars from the end of the string
	 */
	if (l < 0) {
		l = ((*string)->value.str.len - f) + l;
		if (l < 0) {
			l = 0;
		}
	}

	if (f >= (int)(*string)->value.str.len) {
		RETURN_FALSE;
	}

	if((f+l) > (int)(*string)->value.str.len) {
		l = (int)(*string)->value.str.len - f;
	}

	RETVAL_STRINGL((*string)->value.str.val + f, l, 1);
}
/* }}} */


/* {{{ proto string substr_replace(string str, string repl, int start [, int length])
   Replace part of a string with another string */
PHP_FUNCTION(substr_replace)
{
    zval**      string;
    zval**      from;
    zval**      len;
    zval**      repl;
    char*       result;
	int         result_len;
    int         argc;
    int         l;
    int         f;
	
	argc = ARG_COUNT(ht);

	if ((argc == 3 && zend_get_parameters_ex(3, &string, &repl, &from) == FAILURE) ||
		(argc == 4 && zend_get_parameters_ex(4, &string, &repl, &from, &len) == FAILURE) ||
		argc < 3 || argc > 4) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_string_ex(string);
	convert_to_string_ex(repl);
	convert_to_long_ex(from);
	f = (*from)->value.lval;

	if (argc == 3) {
		l = (*string)->value.str.len;
	} else {
		convert_to_long_ex(len);
		l = (*len)->value.lval;
	}

	/* if "from" position is negative, count start position from the end
	 * of the string
	 */
	if (f < 0) {
		f = (*string)->value.str.len + f;
		if (f < 0) {
			f = 0;
		}
	}

	/* if "length" position is negative, set it to the length
	 * needed to stop that many chars from the end of the string
	 */
	if (l < 0) {
		l = ((*string)->value.str.len - f) + l;
		if (l < 0) {
			l = 0;
		}
	}

	if (f >= (int)(*string)->value.str.len) {
		RETURN_STRINGL((*string)->value.str.val, (*string)->value.str.len, 1);
	}

	if((f+l) > (int)(*string)->value.str.len) {
		l = (int)(*string)->value.str.len - f;
	}

	result_len = (*string)->value.str.len - l + (*repl)->value.str.len;
	result = (char *)ecalloc(result_len + 1, sizeof(char *));

	memcpy(result, (*string)->value.str.val, f);
	memcpy(&result[f], (*repl)->value.str.val, (*repl)->value.str.len);
	memcpy(&result[f + (*repl)->value.str.len], (*string)->value.str.val + f + l,
          (*string)->value.str.len - f - l);

	RETVAL_STRINGL(result, result_len, 0);
}
/* }}} */


/* {{{ proto string quotemeta(string str)
   Quote meta characters */
PHP_FUNCTION(quotemeta)
{
	zval **arg;
	char *str, *old;
	char *old_end;
	char *p, *q;
	char c;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);

	old = (*arg)->value.str.val;
	old_end = (*arg)->value.str.val + (*arg)->value.str.len;

	if (old == old_end) {
		RETURN_FALSE;
	}
	
	str = emalloc(2 * (*arg)->value.str.len + 1);
	
	for(p = old, q = str; p != old_end; p++) {
		c = *p;
		switch(c) {
			case '.':
			case '\\':
			case '+':
			case '*':
			case '?':
			case '[':
			case '^':
			case ']':
			case '$':
			case '(':
			case ')':
				*q++ = '\\';
				/* break is missing _intentionally_ */
			default:
				*q++ = c;
		}
	}
	*q = 0;
	RETVAL_STRINGL(erealloc(str, q - str + 1), q - str, 0);
}
/* }}} */

/* {{{ proto int ord(string character)
   Return ASCII value of character */
PHP_FUNCTION(ord)
{
	zval **str;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);
	RETVAL_LONG((unsigned char)(*str)->value.str.val[0]);
}
/* }}} */

/* {{{ proto string chr(int ascii)
   Convert ASCII code to a character */
PHP_FUNCTION(chr)
{
	zval **num;
	char temp[2];
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &num) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(num);
	temp[0] = (char) (*num)->value.lval;
	temp[1] = '\0';
	RETVAL_STRINGL(temp, 1,1);
}
/* }}} */

/* {{{ proto string ucfirst(string str)
   Make a string's first character uppercase */
PHP_FUNCTION(ucfirst)
{
	zval **arg;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);

	if (!*(*arg)->value.str.val) {
		RETURN_FALSE;
	}

	*return_value=**arg;
	zval_copy_ctor(return_value);
	*return_value->value.str.val = toupper((unsigned char)*return_value->value.str.val);
}
/* }}} */

/* {{{ proto string ucwords(string str)
   Uppercase the first character of every word in a string */
PHP_FUNCTION(ucwords)
{
	zval **str;
	char *r;
	char *r_end;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	if (!(*str)->value.str.len) {
		RETURN_FALSE;
	}
	*return_value=**str;
	zval_copy_ctor(return_value);
	*return_value->value.str.val = toupper((unsigned char)*return_value->value.str.val);

	r=return_value->value.str.val;
	r_end = r + (*str)->value.str.len;
	while((r=php_memnstr(r, " ", 1, r_end)) != NULL){
		if(r < r_end){
			r++;
			*r=toupper((unsigned char)*r);
		} else {
			break;
		}
	}
}
/* }}} */

PHPAPI char *php_strtr(char *string, int len, char *str_from,
					   char *str_to, int trlen)
{
	int i;
	unsigned char xlat[256];

	if ((trlen < 1) || (len < 1)) {
		return string;
	}

	for (i = 0; i < 256; xlat[i] = i, i++);

	for (i = 0; i < trlen; i++) {
		xlat[(unsigned char) str_from[i]] = str_to[i];
	}

	for (i = 0; i < len; i++) {
		string[i] = xlat[(unsigned char) string[i]];
	}

	return string;
}

static void php_strtr_array(zval *return_value,char *str,int slen,HashTable *hash)
{
	zval *entry;
	char *string_key;
	zval **trans;
	zval ctmp;
	ulong num_key;
	int minlen = 128*1024;
	int maxlen = 0, pos, len, newpos, newlen, found;
	char *newstr, *key;
	
	zend_hash_internal_pointer_reset(hash);
	while (zend_hash_get_current_data(hash, (void **)&entry) == SUCCESS) {
		switch (zend_hash_get_current_key(hash, &string_key, &num_key)) {
		case HASH_KEY_IS_STRING:
			len = strlen(string_key);
			if (len > maxlen) maxlen = len;
			if (len < minlen) minlen = len;
			efree(string_key);
			break; 
			
		case HASH_KEY_IS_LONG:
			ctmp.type = IS_LONG;
			ctmp.value.lval = num_key;
			
			convert_to_string(&ctmp);
			len = ctmp.value.str.len;
			zval_dtor(&ctmp);

			if (len > maxlen) maxlen = len;
			if (len < minlen) minlen = len;
			break;
		}
		zend_hash_move_forward(hash);
	}
	
	key = emalloc(maxlen+1);
	newstr = emalloc(8192);
	newlen = 8192;
	newpos = pos = 0;

	while (pos < slen) {
		if ((pos + maxlen) > slen) {
			maxlen = slen - pos;
		}
			
		found = 0;
		memcpy(key,str+pos,maxlen);

		for (len = maxlen; len >= minlen; len--) {
			key[ len ]=0;
			
			if (zend_hash_find(hash,key,len+1,(void**)&trans) == SUCCESS) {
				char *tval;
				int tlen;
				zval tmp;

				if ((*trans)->type != IS_STRING) {
					tmp = **trans;
					zval_copy_ctor(&tmp);
					convert_to_string(&tmp);
					tval = tmp.value.str.val;
					tlen = tmp.value.str.len;
				} else {
					tval = (*trans)->value.str.val;
					tlen = (*trans)->value.str.len;
				}

				if ((newpos + tlen + 1) > newlen) {
					newlen = newpos + tlen + 1 + 8192;
					newstr = erealloc(newstr,newlen);
				}
				
				memcpy(newstr+newpos,tval,tlen);
				newpos += tlen;
				pos += len;
				found = 1;

				if ((*trans)->type != IS_STRING) {
					zval_dtor(&tmp);
				}
				break;
			} 
		}

		if (! found) {
			if ((newpos + 1) > newlen) {
				newlen = newpos + 1 + 8192;
				newstr = erealloc(newstr,newlen);
			}
			
			newstr[ newpos++ ] = str[ pos++ ];
		}
	}

	efree(key);
	newstr[ newpos ] = 0;
	RETURN_STRINGL(newstr,newpos,0);
}

/* {{{ proto string strtr(string str, string from, string to)
   Translate characters in str using given translation tables */
PHP_FUNCTION(strtr)
{								/* strtr(STRING,FROM,TO) */
	zval **str, **from, **to;
	int ac = ARG_COUNT(ht);

	if (ac < 2 || ac > 3 || zend_get_parameters_ex(ac, &str, &from, &to) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	if (ac == 2 && (*from)->type != IS_ARRAY) {
		php_error(E_WARNING,"arg2 must be passed an array");
		RETURN_FALSE;
	}

	convert_to_string_ex(str);

	if (ac == 2) {
		php_strtr_array(return_value,(*str)->value.str.val,(*str)->value.str.len,HASH_OF(*from));
	} else {
		convert_to_string_ex(from);
		convert_to_string_ex(to);

		*return_value=**str;
		zval_copy_ctor(return_value);
		
		php_strtr(return_value->value.str.val,
				  return_value->value.str.len,
				  (*from)->value.str.val,
				  (*to)->value.str.val,
				  MIN((*from)->value.str.len,(*to)->value.str.len));
	}
}
/* }}} */


/* {{{ proto string strrev(string str)
   Reverse a string */
PHP_FUNCTION(strrev)
{
	zval **str;
	int i,len;
	char c;
	
	if (ARG_COUNT(ht)!=1 || zend_get_parameters_ex(1, &str)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_string_ex(str);
	
	*return_value = **str;
	zval_copy_ctor(return_value);

	len = return_value->value.str.len;
	
	for (i=0; i<len-1-i; i++) {
		c=return_value->value.str.val[i];
		return_value->value.str.val[i] = return_value->value.str.val[len-1-i];
		return_value->value.str.val[len-1-i]=c;
	}
}
/* }}} */

static void php_similar_str(const char *txt1, int len1, const char *txt2, 
							int len2, int *pos1, int *pos2, int *max)
{
	char *p, *q;
	char *end1 = (char *) txt1 + len1;
	char *end2 = (char *) txt2 + len2;
	int l;
	
	*max = 0;
	for (p = (char *) txt1; p < end1; p++) {
		for (q = (char *) txt2; q < end2; q++) {
			for (l = 0; (p + l < end1) && (q + l < end2) && (p[l] == q[l]); 
				 l++);
			if (l > *max) {
				*max = l;
				*pos1 = p - txt1;
				*pos2 = q - txt2;
			}
		}
	}
}

static int php_similar_char(const char *txt1, int len1, 
							const char *txt2, int len2)
{
	int sum;
	int pos1, pos2, max;

	php_similar_str(txt1, len1, txt2, len2, &pos1, &pos2, &max);
	if ((sum = max)) {
		if (pos1 && pos2)
			sum += php_similar_char(txt1, pos1, txt2, pos2);
		if ((pos1 + max < len1) && (pos2 + max < len2))
			sum += php_similar_char(txt1 + pos1 + max, len1 - pos1 - max, 
									txt2 + pos2 + max, len2 - pos2 - max);
	}
	return sum;
}

/* {{{ proto int similar_text(string str1, string str2 [, double percent])
   Calculates the similarity between two strings */
PHP_FUNCTION(similar_text)
{
	zval **t1, **t2, **percent;
	int ac = ARG_COUNT(ht);
	int sim;
	
	if (ac < 2 || ac > 3 ||
		zend_get_parameters_ex(ac, &t1, &t2, &percent) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_string_ex(t1);
	convert_to_string_ex(t2);
	if (ac > 2) {
		convert_to_double_ex(percent);
	}
	
	if (((*t1)->value.str.len + (*t2)->value.str.len) == 0) {
		if(ac > 2) {
			(*percent)->value.dval = 0;
		}
		RETURN_LONG(0);
	}
	
	sim = php_similar_char((*t1)->value.str.val, (*t1)->value.str.len, 
							 (*t2)->value.str.val, (*t2)->value.str.len);
	
	if (ac > 2) {
		(*percent)->value.dval = sim * 200.0 / ((*t1)->value.str.len + (*t2)->value.str.len);
	}
	
	RETURN_LONG(sim);
}
/* }}} */
	

/* be careful, this edits the string in-place */
PHPAPI void php_stripslashes(char *string, int *len)
{
	char *s, *t;
	int l;
	char escape_char='\\';
	PLS_FETCH();

	if (PG(magic_quotes_sybase)) {
		escape_char='\'';
	}

	if (len != NULL) {
		l = *len;
	} else {
		l = strlen(string);
	}
	s = string;
	t = string;
	while (l > 0) {
		if (*t == escape_char) {
			t++;				/* skip the slash */
			if (len != NULL)
				(*len)--;
			l--;
			if (l > 0) {
				if(*t=='0') {
					*s++='\0';
					t++;
				} else {
					*s++ = *t++;	/* preserve the next character */
				}
				l--;
			}
		} else {
			if (s != t)
				*s++ = *t++;
			else {
				s++;
				t++;
			}
			l--;
		}
	}
	if (s != t) {
		*s = '\0';
	}
}

/* {{{ proto string addcslashes(string str, string charlist)
   Escape all chars mentioned in charlist with backslash. It creates
   octal representations if asked to backslash characters with 8th bit set
   or with ASCII<32 (except '\n', '\r', '\t' etc...) */
PHP_FUNCTION(addcslashes)
{
	zval **str, **what;

	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &str, &what) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);
	convert_to_string_ex(what);
	return_value->value.str.val = php_addcslashes((*str)->value.str.val,(*str)->value.str.len,&return_value->value.str.len,0,(*what)->value.str.val,(*what)->value.str.len);
	return_value->type = IS_STRING;
}
/* }}} */

/* {{{ proto string addslashes(string str)
   Escape single quote, double quotes and backslash characters in a string with backslashes */
PHP_FUNCTION(addslashes)
{
	zval **str;

	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);
	return_value->value.str.val = php_addslashes((*str)->value.str.val,(*str)->value.str.len,&return_value->value.str.len,0);
	return_value->type = IS_STRING;
}
/* }}} */

/* {{{ proto string stripcslashes(string str)
   Strip backslashes from a string. Uses C-style conventions*/
PHP_FUNCTION(stripcslashes)
{
	zval **str;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	*return_value = **str;
	zval_copy_ctor(return_value);
	php_stripcslashes(return_value->value.str.val,&return_value->value.str.len);
}
/* }}} */

/* {{{ proto string stripslashes(string str)
   Strip backslashes from a string */
PHP_FUNCTION(stripslashes)
{
	zval **str;
	
	if (ARG_COUNT(ht) != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	*return_value = **str;
	zval_copy_ctor(return_value);
	php_stripslashes(return_value->value.str.val,&return_value->value.str.len);
}
/* }}} */

#ifndef HAVE_STRERROR
#if !APACHE
char *strerror(int errnum) 
{
	extern int sys_nerr;
	extern char *sys_errlist[];
	BLS_FETCH();

	if ((unsigned int)errnum < sys_nerr) return(sys_errlist[errnum]);
	(void)sprintf(BG(str_ebuf), "Unknown error: %d", errnum);
	return(BG(str_ebuf));
}
#endif
#endif

PHPAPI void php_stripcslashes(char *str, int *len)
{
	char *source,*target,*end;
	int  nlen = *len, i;
	char numtmp[4];

	for (source=str,end=str+nlen,target=str; source<end; source++) {
		if (*source == '\\' && source+1<end) {
			source++;
			switch (*source) {
				case 'n': *target++='\n'; nlen--; break;
				case 'r': *target++='\r'; nlen--; break;
				case 'a': *target++='\a'; nlen--; break;
				case 't': *target++='\t'; nlen--; break;
				case 'v': *target++='\v'; nlen--; break;
				case 'b': *target++='\b'; nlen--; break;
				case 'f': *target++='\f'; nlen--; break;
				case '\\': *target++='\\'; nlen--; break;
				case 'x': if (source+1<end && isxdigit((int)(*(source+1)))) {
						numtmp[0] = *++source;
						if (source+1<end && isxdigit((int)(*(source+1)))) {
							numtmp[1] = *++source;
							numtmp[2] = '\0';
							nlen-=3;
						} else {
							numtmp[1] = '\0';
							nlen-=2;
						}
						*target++=(char)strtol(numtmp, NULL, 16);
						break;
					}
					/* break is left intentionally */
				default: i=0; 
					while (source<end && *source>='0' && *source<='7' && i<3) {
						numtmp[i++] = *source++;
					}
					if (i) {
						numtmp[i]='\0';
						*target++=(char)strtol(numtmp, NULL, 8);
						nlen-=i;
						source--;
					} else {
						*target++='\\';
						*target++=*source;
					}
			}
		} else {
			*target++=*source;
		}
	}
	*target='\0';

	*len = nlen;
}
			

PHPAPI char *php_addcslashes(char *str, int length, int *new_length, int should_free, char *what, int wlength)
{
	char flags[256];
	char *new_str = emalloc((length?length:(length=strlen(str)))*4+1); 
	char *source,*target;
	char *end;
	char c;
	int  newlen;

	if (!wlength) {
		wlength = strlen(what);
	}

	if (!length) {
		length = strlen(str);
	}

	memset(flags, '\0', sizeof(flags));
	for (source=what,end=source+wlength; (c=*source) || source<end; source++) {
		if (source+3<end && *(source+1) == '.' && *(source+2) == '.' && (unsigned char)*(source+3)>=(unsigned char)c) {
			memset(flags+c, 1, (unsigned char)*(source+3)-(unsigned char)c+1);
			source+=3;
		} else
			flags[(unsigned char)c]=1;
	}

	for (source=str,end=source+length,target=new_str; (c=*source) || source<end; source++) {
		if (flags[(unsigned char)c]) {
			if ((unsigned char)c<32 || (unsigned char)c>126) {
				*target++ = '\\';
				switch (c) {
					case '\n': *target++ = 'n'; break;
					case '\t': *target++ = 't'; break;
					case '\r': *target++ = 'r'; break;
					case '\a': *target++ = 'a'; break;
					case '\v': *target++ = 'v'; break;
					case '\b': *target++ = 'b'; break;
					case '\f': *target++ = 'f'; break;
					default: target += sprintf(target, "%03o", (unsigned char)c);
				}
				continue;
			} 
			*target++ = '\\';
		}
		*target++ = c;
	}
	*target = 0;
	newlen = target-new_str;
	if (target-new_str<length*4) {
		new_str = erealloc(new_str, newlen+1);
	}
	if (new_length) {
		*new_length = newlen;
	}
	if (should_free) {
		STR_FREE(str);
	}
	return new_str;
}

PHPAPI char *php_addslashes(char *str, int length, int *new_length, int should_free)
{
	/* maximum string length, worst case situation */
	char *new_str;
	char *source,*target;
	char *end;
	char c;
	PLS_FETCH();
 	
	if (!str) {
		*new_length = 0;
		return str;
	}
	new_str = (char *) emalloc((length?length:(length=strlen(str)))*2+1);
	for (source=str,end=source+length,target=new_str; (c = *source) || source<end; source++) {
		switch(c) {
			case '\0':
				*target++ = '\\';
				*target++ = '0';
				break;
			case '\'':
				if (PG(magic_quotes_sybase)) {
					*target++ = '\'';
					*target++ = '\'';
					break;
				}
				/* break is missing *intentionally* */
			case '\"':
			case '\\':
				if (!PG(magic_quotes_sybase)) {
					*target++ = '\\';
				}
				/* break is missing *intentionally* */
			default:
				*target++ = c;
				break;
		}
	}
	*target = 0;
	if (new_length) {
		*new_length = target - new_str;
	}
	if (should_free) {
		STR_FREE(str);
	}
	return new_str;
}


#define _HEB_BLOCK_TYPE_ENG 1
#define _HEB_BLOCK_TYPE_HEB 2
#define isheb(c) (((((unsigned char) c)>=224) && (((unsigned char) c)<=250)) ? 1 : 0)
#define _isblank(c) (((((unsigned char) c)==' ' || ((unsigned char) c)=='\t')) ? 1 : 0)
#define _isnewline(c) (((((unsigned char) c)=='\n' || ((unsigned char) c)=='\r')) ? 1 : 0)

PHPAPI void php_char_to_str(char *str,uint len,char from,char *to,int to_len,zval *result)
{
	int char_count=0;
	char *source,*target,*tmp,*source_end=str+len, *tmp_end=NULL;
	
	for (source=str; source<source_end; source++) {
		if (*source==from) {
			char_count++;
		}
	}

	result->type = IS_STRING;
		
	if (char_count==0) {
		result->value.str.val = estrndup(str,len);
		result->value.str.len = len;
		return;
	}
	
	result->value.str.len = len+char_count*(to_len-1);
	result->value.str.val = target = (char *) emalloc(result->value.str.len+1);
	
	for (source=str; source<source_end; source++) {
		if (*source==from) {
			for (tmp=to,tmp_end=tmp+to_len; tmp<tmp_end; tmp++) {
				*target = *tmp;
				target++;
			}
		} else {
			*target = *source;
			target++;
		}
	}
	*target = 0;
}


PHPAPI inline char *
php_memnstr(char *haystack, char *needle, int needle_len, char *end)
{
	char *p = haystack;
	char *s = NULL;

	for(; p <= end - needle_len && 
			(s = memchr(p, *needle, end - p - needle_len + 1)); p = s + 1) {
		if(memcmp(s, needle, needle_len) == 0)
			return s;
	}
	return NULL;
}

PHPAPI char *php_str_to_str(char *haystack, int length, 
	char *needle, int needle_len, char *str, int str_len, int *_new_length)
{
	char *p, *q;
	char *r, *s;
	char *end = haystack + length;
	char *new;
	char *off;
	
	new = emalloc(length);
	/* we jump through haystack searching for the needle. hurray! */
	for(p = haystack, q = new;
			(r = php_memnstr(p, needle, needle_len, end));) {
	/* this ain't optimal. you could call it `efficient memory usage' */
		off = erealloc(new, (q - new) + (r - p) + (str_len) + 1);
		if(off != new) {
			if(!off) {
				goto finish;
			}
			q += off - new;
			new = off;
		}
		memcpy(q, p, r - p);
		q += r - p;
		memcpy(q, str, str_len);
		q += str_len;
		p = r + needle_len;
	}
	
	/* if there is a rest, copy it */
	if((end - p) > 0) {
		s = (q) + (end - p);
		off = erealloc(new, s - new + 1);
		if(off != new) {
			if(!off) {
				goto finish;
			}
			q += off - new;
			new = off;
			s = q + (end - p);
		}
		memcpy(q, p, end - p);
		q = s;
	}
finish:
	*q = '\0';
	if(_new_length) *_new_length = q - new;
	return new;
}


/* {{{ proto string str_replace(string needle, string str, string haystack)
   Replace all occurrences of needle in haystack with str */
PHP_FUNCTION(str_replace)
{
	zval **haystack, **needle, **str;
	char *new;
	int len = 0;

	if(ARG_COUNT(ht) != 3 ||
			zend_get_parameters_ex(3, &needle, &str, &haystack) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(haystack);
	convert_to_string_ex(needle);
	convert_to_string_ex(str);

	if((*haystack)->value.str.len == 0) {
		RETURN_STRING(empty_string,1);
	}

	if((*needle)->value.str.len == 1) {
		php_char_to_str((*haystack)->value.str.val,
						(*haystack)->value.str.len,
						(*needle)->value.str.val[0],
						(*str)->value.str.val,
						(*str)->value.str.len,
						return_value);
		return;
	}

	if((*needle)->value.str.len == 0) {
		php_error(E_WARNING, "The length of the needle must not be 0");
		RETURN_FALSE;
	}

	new = php_str_to_str((*haystack)->value.str.val, (*haystack)->value.str.len,
						 (*needle)->value.str.val, (*needle)->value.str.len,
						 (*str)->value.str.val, (*str)->value.str.len, &len);
	RETURN_STRINGL(new, len, 0);
}
/* }}} */

/* Converts Logical Hebrew text (Hebrew Windows style) to Visual text
 * Cheers/complaints/flames - Zeev Suraski <zeev@php.net>
 */
static void php_hebrev(INTERNAL_FUNCTION_PARAMETERS,int convert_newlines)
{
	zval **str,**max_chars_per_line;
	char *heb_str,*tmp,*target,*opposite_target,*broken_str;
	int block_start, block_end, block_type, block_length, i;
	int block_ended;
	long max_chars=0;
	int begin,end,char_count,orig_begin;

	
	switch(ARG_COUNT(ht)) {
		case 1:
			if (zend_get_parameters_ex(1, &str)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &str, &max_chars_per_line)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(max_chars_per_line);
			max_chars = (*max_chars_per_line)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	convert_to_string_ex(str);
	
	if ((*str)->value.str.len==0) {
		RETURN_FALSE;
	}

	tmp = (*str)->value.str.val;
	block_start=block_end=0;
	block_ended=0;

	heb_str = (char *) emalloc((*str)->value.str.len+1);
	target = heb_str+(*str)->value.str.len;
	opposite_target = heb_str;
	*target = 0;
	target--;

	block_length=0;

	if (isheb(*tmp)) {
		block_type = _HEB_BLOCK_TYPE_HEB;
	} else {
		block_type = _HEB_BLOCK_TYPE_ENG;
	}
	
	do {
		if (block_type==_HEB_BLOCK_TYPE_HEB) {
			while((isheb((int)*(tmp+1)) || _isblank((int)*(tmp+1)) || ispunct((int)*(tmp+1)) || (int)*(tmp+1)=='\n' ) && block_end<(*str)->value.str.len-1) {
				tmp++;
				block_end++;
				block_length++;
			}
			for (i=block_start; i<=block_end; i++) {
				*target = (*str)->value.str.val[i];
				switch (*target) {
					case '(':
						*target = ')';
						break;
					case ')':
						*target = '(';
						break;
					default:
						break;
				}
				target--;
			}
			block_type = _HEB_BLOCK_TYPE_ENG;
		} else {
			while(!isheb(*(tmp+1)) && (int)*(tmp+1)!='\n' && block_end<(*str)->value.str.len-1) {
				tmp++;
				block_end++;
				block_length++;
			}
			while ((_isblank((int)*tmp) || ispunct((int)*tmp)) && *tmp!='/' && *tmp!='-' && block_end>block_start) {
				tmp--;
				block_end--;
			}
			for (i=block_end; i>=block_start; i--) {
				*target = (*str)->value.str.val[i];
				target--;
			}
			block_type = _HEB_BLOCK_TYPE_HEB;
		}
		block_start=block_end+1;
	} while(block_end<(*str)->value.str.len-1);


	broken_str = (char *) emalloc((*str)->value.str.len+1);
	begin=end=(*str)->value.str.len-1;
	target = broken_str;
		
	while (1) {
		char_count=0;
		while ((!max_chars || char_count<max_chars) && begin>0) {
			char_count++;
			begin--;
			if (begin<=0 || _isnewline(heb_str[begin])) {
				while(begin>0 && _isnewline(heb_str[begin-1])) {
					begin--;
					char_count++;
				}
				break;
			}
		}
		if (char_count==max_chars) { /* try to avoid breaking words */
			int new_char_count=char_count, new_begin=begin;
			
			while (new_char_count>0) {
				if (_isblank(heb_str[new_begin]) || _isnewline(heb_str[new_begin])) {
					break;
				}
				new_begin++;
				new_char_count--;
			}
			if (new_char_count>0) {
				char_count=new_char_count;
				begin=new_begin;
			}
		}
		orig_begin=begin;
		
		if (_isblank(heb_str[begin])) {
			heb_str[begin]='\n';
		}
		while (begin<=end && _isnewline(heb_str[begin])) { /* skip leading newlines */
			begin++;
		}
		for (i=begin; i<=end; i++) { /* copy content */
			*target = heb_str[i];
			target++;
		}
		for (i=orig_begin; i<=end && _isnewline(heb_str[i]); i++) {
			*target = heb_str[i];
			target++;
		}
		begin=orig_begin;

		if (begin<=0) {
			*target = 0;
			break;
		}
		begin--;
		end=begin;
	}
	efree(heb_str);

	if (convert_newlines) {
		php_char_to_str(broken_str,(*str)->value.str.len,'\n',"<br>\n",5,return_value);
		efree(broken_str);
	} else {
		return_value->value.str.val = broken_str;
		return_value->value.str.len = (*str)->value.str.len;
		return_value->type = IS_STRING;
	}
}


/* {{{ proto string hebrev(string str [, int max_chars_per_line])
   Convert logical Hebrew text to visual text */
PHP_FUNCTION(hebrev)
{
	php_hebrev(INTERNAL_FUNCTION_PARAM_PASSTHRU,0);
}
/* }}} */

/* {{{ proto string hebrevc(string str [, int max_chars_per_line])
   Convert logical Hebrew text to visual text with newline conversion */
PHP_FUNCTION(hebrevc)
{
	php_hebrev(INTERNAL_FUNCTION_PARAM_PASSTHRU,1);
}
/* }}} */

/* {{{ proto string nl2br(string str)
   Converts newlines to HTML line breaks */
PHP_FUNCTION(nl2br)
{
	zval **str;
	
	if (ARG_COUNT(ht)!=1 || zend_get_parameters_ex(1, &str)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_string_ex(str);
	
	php_char_to_str((*str)->value.str.val,(*str)->value.str.len,'\n',"<br>\n",5,return_value);
}
/* }}} */

/* {{{ proto string strip_tags(string str [, string allowable_tags])
   Strips HTML and PHP tags from a string */
PHP_FUNCTION(strip_tags)
{
	char *buf;
	zval **str, **allow=NULL;

	switch(ARG_COUNT(ht)) {
		case 1:
			if(zend_get_parameters_ex(1, &str)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if(zend_get_parameters_ex(2, &str, &allow)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(allow);
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	convert_to_string_ex(str);
	buf = estrdup((*str)->value.str.val);
	php_strip_tags(buf, (*str)->value.str.len, 0, allow?(*allow)->value.str.val:NULL);
	RETURN_STRING(buf, 0);
}
/* }}} */

/* {{{ proto string setlocale(string category, string locale)
   Set locale information */
PHP_FUNCTION(setlocale)
{
	zval **pcategory, **plocale;
	zval *category, *locale;
	int cat;
	char *loc, *retval;
	BLS_FETCH();

	if (ARG_COUNT(ht)!=2 || zend_get_parameters_ex(2, &pcategory, &plocale)==FAILURE)
		WRONG_PARAM_COUNT;
#ifdef HAVE_SETLOCALE
	convert_to_string_ex(pcategory);
	convert_to_string_ex(plocale);
	category = *pcategory;
	locale = *plocale;
	if (!strcasecmp ("LC_ALL", category->value.str.val))
		cat = LC_ALL;
	else if (!strcasecmp ("LC_COLLATE", category->value.str.val))
		cat = LC_COLLATE;
	else if (!strcasecmp ("LC_CTYPE", category->value.str.val))
		cat = LC_CTYPE;
	else if (!strcasecmp ("LC_MONETARY", category->value.str.val))
		cat = LC_MONETARY;
	else if (!strcasecmp ("LC_NUMERIC", category->value.str.val))
		cat = LC_NUMERIC;
	else if (!strcasecmp ("LC_TIME", category->value.str.val))
		cat = LC_TIME;
	else {
		php_error(E_WARNING,"Invalid locale category name %s, must be one of LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY, LC_NUMERIC or LC_TIME", category->value.str.val);
		RETURN_FALSE;
	}
	if (!strcmp ("0", locale->value.str.val))
		loc = NULL;
	else
		loc = locale->value.str.val;
	retval = setlocale (cat, loc);
	if (retval) {
		/* Remember if locale was changed */
		if (loc) {
			STR_FREE(BG(locale_string));
			BG(locale_string) = estrdup(retval);
		}

		RETVAL_STRING(retval,1);
		return;
	}
#endif
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto void parse_str(string encoded_string)
   Parses GET/POST/COOKIE data and sets global variables. */
PHP_FUNCTION(parse_str)
{
	zval **arg;
	char *res = NULL;
	ELS_FETCH();
	PLS_FETCH();
	SLS_FETCH();

	if (zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(arg);
	if ((*arg)->value.str.val && *(*arg)->value.str.val) {
		res = estrndup((*arg)->value.str.val,(*arg)->value.str.len);
	}
	php_treat_data(PARSE_STRING, res ELS_CC PLS_CC SLS_CC);
}
/* }}} */

#define PHP_TAG_BUF_SIZE 1023

/* Check if tag is in a set of tags 
 *
 * states:
 * 
 * 0 start tag
 * 1 first non-whitespace char seen
 */
int php_tag_find(char *tag, int len, char *set) {
	char c, *n, *t;
	int i=0, state=0, done=0;
	char *norm = emalloc(len);

	n = norm;
	t = tag;
	c = tolower(*t);
	/* 
	   normalize the tag removing leading and trailing whitespace
	   and turn any <a whatever...> into just <a> and any </tag>
	   into <tag>
	*/
	while(i<len && !done) {
		switch(c) {
		case '<':
			*(n++) = c;
			break;
		case '>':
			done =1;
			break;
		default:
			if(!isspace((int)c)) {
				if(state==0) {
					state=1;
					if(c!='/') *(n++) = c;
				} else {
					*(n++) = c;
				}
			} else {
				if(state==1) done=1;
			}
			break;
		}
		c = tolower(*(++t));
	}  
	*(n++) = '>';
	*n = '\0'; 
	if(strstr(set,norm)) done=1;
	else done=0;
	efree(norm);
	return done;
}

/* A simple little state-machine to strip out html and php tags 
	
	State 0 is the output state, State 1 means we are inside a
	normal html tag and state 2 means we are inside a php tag.

	The state variable is passed in to allow a function like fgetss
	to maintain state across calls to the function.

	lc holds the last significant character read and br is a bracket
	counter.

	When an allow string is passed in we keep track of the string
	in state 1 and when the tag is closed check it against the
	allow string to see if we should allow it.
*/
PHPAPI void php_strip_tags(char *rbuf, int len, int state, char *allow) {
	char *tbuf, *buf, *p, *tp, *rp, c, lc;
	int br, i=0;

	buf = estrdup(rbuf);
	c = *buf;
	lc = '\0';
	p = buf;
	rp = rbuf;
	br = 0;
	if(allow) {
		php_strtolower(allow, len);
		tbuf = emalloc(PHP_TAG_BUF_SIZE+1);
		tp = tbuf;
	} else {
		tbuf = tp = NULL;
	}

	while(i<len) {
		switch (c) {
			case '<':
				if (state == 0) {
					lc = '<';
					state = 1;
					if(allow) {
						*(tp++) = '<';
					}
				}
				break;

			case '(':
				if (state == 2) {
					if (lc != '\"') {
						lc = '(';
						br++;
					}
				} else if (state == 0) {
					*(rp++) = c;
				}
				break;	

			case ')':
				if (state == 2) {
					if (lc != '\"') {
						lc = ')';
						br--;
					}
				} else if (state == 0) {
					*(rp++) = c;
				}
				break;	

			case '>':
				if (state == 1) {
					lc = '>';
					state = 0;
					if(allow) {
						*(tp++) = '>';
						*tp='\0';
						if(php_tag_find(tbuf, tp-tbuf, allow)) {
							memcpy(rp,tbuf,tp-tbuf);
							rp += tp-tbuf;
						}
						tp = tbuf;
					}
				} else if (state == 2) {
					if (!br && lc != '\"' && *(p-1)=='?') {
						state = 0;
					}
				}
				break;

			case '\"':
				if (state == 2) {
					if (lc == '\"') {
						lc = '\0';
					} else if (lc != '\\') {
						lc = '\"';
					}
				} else if (state == 0) {
					*(rp++) = c;
				} else if (allow && state == 1) {
					*(tp++) = c;
				}
				break;

			case '?':
				if (state==1 && *(p-1)=='<') {
					br=0;
					state=2;
					break;
				}
				/* fall-through */

			default:
				if (state == 0) {
					*(rp++) = c;
				} else if(allow && state == 1) {
					*(tp++) = c;
					if( (tp-tbuf)>=PHP_TAG_BUF_SIZE ) { /* no buffer overflows */
						tp = tbuf;
					}
				}
				break;
		}
		c = *(++p);
		i++;
	}	
	*rp = '\0';
	efree(buf);
	if(allow) efree(tbuf);
}

/* {{{ proto string str_repeat(string input, int mult)
   Returns the input string repeat mult times */
PHP_FUNCTION(str_repeat)
{
	zval		**input_str;		/* Input string */
	zval		**mult;				/* Multiplier */
	char		 *result;			/* Resulting string */
	int			  result_len;		/* Length of the resulting string */
	int			  i;
	
	if (ARG_COUNT(ht) != 2 || zend_get_parameters_ex(2, &input_str, &mult) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	/* Make sure we're dealing with proper types */
	convert_to_string_ex(input_str);
	convert_to_long_ex(mult);
	
	if ((*mult)->value.lval < 1) {
		php_error(E_WARNING, "Second argument to %s() has to be greater than 0",
				  get_active_function_name());
		return;
	}

	/* Don't waste our time if it's empty */
	if ((*input_str)->value.str.len == 0)
		RETURN_STRINGL(empty_string, 0, 1);
	
	/* Initialize the result string */	
	result_len = (*input_str)->value.str.len * (*mult)->value.lval;
	result = (char *)emalloc(result_len + 1);
	
	/* Copy the input string into the result as many times as necessary */
	for (i=0; i<(*mult)->value.lval; i++) {
		memcpy(result + (*input_str)->value.str.len * i,
			   (*input_str)->value.str.val,
			   (*input_str)->value.str.len);
	}
	result[result_len] = '\0';
	
	RETURN_STRINGL(result, result_len, 0);
}
/* }}} */

/* {{{ proto mixed count_chars(string input[, int mode])
   Returns info about what characters are used in input */
PHP_FUNCTION(count_chars)
{
	zval **input, **mode;
	int chars[256];
	int ac=ARG_COUNT(ht);
	int mymode=0;
	unsigned char *buf;
	int len, inx;
	char retstr[256];
	int retlen=0;

	if (ac < 1 || ac > 2 || zend_get_parameters_ex(ac, &input, &mode) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_string_ex(input);

	if (ac == 2) {
		convert_to_long_ex(mode);
		mymode = (*mode)->value.lval;
		
		if (mymode < 0 || mymode > 4) {
			php_error(E_WARNING, "unknown mode");
			RETURN_FALSE;
		}
	}
	
	len = (*input)->value.str.len;
	buf = (unsigned char *) (*input)->value.str.val;
	memset((void*) chars,0,sizeof(chars));

	while (len > 0) {
		chars[*buf]++;
		buf++;
		len--;
	}

	if (mymode < 3) {
		array_init(return_value);
	}

	for (inx=0; inx < 255; inx++) {
		switch (mymode) {
 		case 0:
			add_index_long(return_value,inx,chars[inx]);
			break;
 		case 1:
			if (chars[inx] != 0) {
				add_index_long(return_value,inx,chars[inx]);
			}
			break;
  		case 2:
			if (chars[inx] == 0) {
				add_index_long(return_value,inx,chars[inx]);
			}
			break;
  		case 3:
			if (chars[inx] != 0) {
				retstr[retlen++] = inx;
			}
			break;
  		case 4:
			if (chars[inx] == 0) {
				retstr[retlen++] = inx;
			}
			break;
		}
	}
	
	if (mymode >= 3 && mymode <= 4) {
		RETURN_STRINGL(retstr,retlen,1);
	}
}
/* }}} */
