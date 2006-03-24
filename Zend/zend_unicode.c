/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2006 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at                              |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andrei Zmievski <andrei@php.net>                            |
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_globals.h"
#include "zend_operators.h"
#include "zend_exceptions.h"
#include "zend_API.h"
#include "zend_unicode.h"
#include <unicode/unorm.h>

#ifdef ZTS
ZEND_API ts_rsrc_id unicode_globals_id;
#else
ZEND_API zend_unicode_globals unicode_globals;
#endif

ZEND_API zend_class_entry *unicodeConversionException;

/* {{{ zend_set_converter_error_mode */
void zend_set_converter_error_mode(UConverter *conv, uint8_t error_mode)
{
	UErrorCode status = U_ZERO_ERROR;

	switch (error_mode) {
		case ZEND_CONV_ERROR_STOP:
			ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_STOP, NULL, NULL, NULL, &status);
			break;

		case ZEND_CONV_ERROR_SKIP:
			ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_SKIP, UCNV_SKIP_STOP_ON_ILLEGAL, NULL, NULL, &status);
			break;

		case ZEND_CONV_ERROR_SUBST:
			ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_SUBSTITUTE, UCNV_SUB_STOP_ON_ILLEGAL, NULL, NULL, &status);
			break;

		case ZEND_CONV_ERROR_ESCAPE_UNICODE:
			ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_UNICODE, NULL, NULL, &status);
			break;

		case ZEND_CONV_ERROR_ESCAPE_ICU:
			ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_ICU, NULL, NULL, &status);
			break;

		case ZEND_CONV_ERROR_ESCAPE_JAVA:
			ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_JAVA, NULL, NULL, &status);
			break;

		case ZEND_CONV_ERROR_ESCAPE_XML_DEC:
			ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_DEC, NULL, NULL, &status);
			break;

		case ZEND_CONV_ERROR_ESCAPE_XML_HEX:
			ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_HEX, NULL, NULL, &status);
			break;

		default:
			assert(0);
			break;
	}
}
/* }}} */

/* {{{ zend_set_converter_subst_char */
void zend_set_converter_subst_char(UConverter *conv, UChar *subst_char, int8_t subst_char_len)
{
	char dest[8];
	int8_t dest_len = 8;
	UErrorCode status = U_ZERO_ERROR;
	UErrorCode temp = U_ZERO_ERROR;
	const void *old_context;
	UConverterFromUCallback old_cb;

	if (!subst_char_len)
		return;

	ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_STOP, NULL, &old_cb, &old_context, &temp);
	dest_len = ucnv_fromUChars(conv, dest, dest_len, subst_char, subst_char_len, &status);
	ucnv_setFromUCallBack(conv, old_cb, old_context, NULL, NULL, &temp);
	if (U_FAILURE(status)) {
		zend_error(E_WARNING, "Could not set substitution character for the converter");
		return;
	}
	ucnv_setSubstChars(conv, dest, dest_len, &status);
	if (status == U_ILLEGAL_ARGUMENT_ERROR) {
		zend_error(E_WARNING, "Substitution character byte sequence is too short or long for this converter");
		return;
	}
}
/* }}} */

/* {{{ zend_set_converter_encoding */
int zend_set_converter_encoding(UConverter **converter, const char *encoding)
{
	UErrorCode status = U_ZERO_ERROR;
	UConverter *new_converter = NULL;

	if (!converter) {
		return FAILURE;
	}

	/*
	 * The specified encoding might be the same as converter's existing one,
	 * which results in a no-op.
	 */
	if (*converter && encoding && encoding[0]) {
		const char *current = ucnv_getName(*converter, &status);
		status = U_ZERO_ERROR; /* reset error */
		if (!ucnv_compareNames(current, encoding)) {
			return SUCCESS;
		}
	}

	/*
	 * If encoding is NULL, ucnv_open() will return a converter based on
	 * the default platform encoding as determined by ucnv_getDefaultName().
	 */
	new_converter = ucnv_open(encoding, &status);
	if (U_FAILURE(status)) {
		return FAILURE;
	}

	if (*converter) {
		ucnv_close(*converter);
	}
	*converter = new_converter;

	return SUCCESS;
}
/* }}} */

/* {{{ zend_copy_converter */
int zend_copy_converter(UConverter **target, UConverter *source)
{
	UErrorCode status = U_ZERO_ERROR;
	const char *encoding;

	assert(source != NULL);

	encoding = ucnv_getName(source, &status);
	if (U_FAILURE(status)) {
		return FAILURE;
	}

	return zend_set_converter_encoding(target, encoding);
}
/* }}} */

/* {{{ zend_convert_to_unicode */
ZEND_API void zend_convert_to_unicode(UConverter *conv, UChar **target, int *target_len, const char *source, int source_len, UErrorCode *status)
{
	UChar *buffer = NULL;
	UChar *output;
	int32_t buffer_len = 0;
	int32_t converted = 0;
	const char *input = source;
	UConverterType conv_type;

	if (U_FAILURE(*status)) {
		return;
	}

	ucnv_resetToUnicode(conv);
	conv_type = ucnv_getType(conv);

	switch (conv_type) {
		case UCNV_SBCS:
		case UCNV_LATIN_1:
		case UCNV_US_ASCII:
			/*
			 * For single-byte charsets, 1 input byte = 1 output UChar
			 */
			buffer_len = source_len;
			break;

		default:
			/*
			 * Initial estimate: 1.25 UChar's for every 2 source bytes + 2 (past a
			 * certain limit (2)). The rationale behind this is that (atleast
			 * in the case of GB2312) it is possible that there are single byte
			 * characters in the input string. By using an GD2312 text as
			 * example it seemed that a value of 1.25 allowed for as little
			 * re-allocations as possible without over estimating the buffer
			 * too much. In case there is a lot of single-byte characters
			 * around a single multi-byte character this estimation is too low,
			 * and then the re-allocation routines in the loop below kick in.
			 * There we multiply by 1.33 and add 1 so that it's quite efficient
			 * for smaller input strings without causing too many iterations of
			 * this loop.
			 */
			buffer_len = (source_len > 2) ? ((source_len >> 1) + (source_len >> 3) + 2) : source_len;
			break;
	}

	while (1) {
		buffer = eurealloc(buffer, buffer_len + 1);
		output = buffer + converted;
		ucnv_toUnicode(conv, &output, buffer + buffer_len, &input, source + source_len, NULL, TRUE, status);
		converted = (int32_t) (output - buffer);
		if (*status == U_BUFFER_OVERFLOW_ERROR) {
			buffer_len = (buffer_len * 1.33) + 1;
			*status = U_ZERO_ERROR;
		} else {
			break;
		}
	}

	/*
	 * We return the buffer in case of failure anyway. The caller may want to
	 * use partially converted string for something.
	 */

	buffer[converted] = 0;
	*target = buffer;
	*target_len = converted;
}
/* }}} */

/* {{{ zend_convert_from_unicode */
ZEND_API int zend_convert_from_unicode(UConverter *conv, char **target, int *target_len, const UChar *source, int source_len, UErrorCode *status)
{
	char *buffer = NULL;
	char *output;
	int32_t buffer_len = 0;
	int32_t converted = 0;
	const UChar *input = source;

	if (U_FAILURE(*status)) {
		return 0;
	}

	ucnv_resetFromUnicode(conv);

	buffer_len = ucnv_getMaxCharSize(conv) * source_len;

	while (1) {
		buffer = erealloc(buffer, buffer_len + 1);
		output = buffer + converted;
		ucnv_fromUnicode(conv, &output, buffer + buffer_len, &input, source + source_len, NULL, TRUE, status);
		converted = (int32_t) (output - buffer);
		if (*status == U_BUFFER_OVERFLOW_ERROR) {
			buffer_len += 64;
			*status = U_ZERO_ERROR;
		} else {
			break;
		}
	}

	/*
	 * We return the buffer in case of failure anyway. The caller may want to
	 * use partially converted string for something.
	 */

	buffer[converted] = 0; /* NULL-terminate the output string */
	*target = buffer;
	*target_len = converted;

	return input - source;
}
/* }}} */

/* {{{ zend_convert_encodings */
ZEND_API void zend_convert_encodings(UConverter *target_conv, UConverter *source_conv,
		char **target, int *target_len,
		const char *source, int source_len, UErrorCode *status)
{
	char *buffer = NULL;
	char *output;
	const char *input = source;
	int32_t allocated = 0;
	int32_t converted = 0;
	int8_t null_size;
	UChar pivot_buf[1024], *pivot, *pivot2;

	if (U_FAILURE(*status)) {
		return;
	}

	null_size = ucnv_getMinCharSize(target_conv);
	allocated = source_len + null_size;

	ucnv_resetToUnicode(source_conv);
	ucnv_resetFromUnicode(target_conv);
	pivot = pivot2 = pivot_buf;

	while (1) {
		buffer = (char *) erealloc(buffer, allocated);
		output = buffer + converted;
		ucnv_convertEx(target_conv, source_conv, &output, buffer + allocated - null_size,
				&input, source + source_len, pivot_buf, &pivot, &pivot2, pivot_buf + 1024, FALSE, TRUE, status);
		converted = (int32_t) (output - buffer);
		if (*status == U_BUFFER_OVERFLOW_ERROR) {
			allocated += 1024;
			*status = U_ZERO_ERROR;
		} else {
			break;
		}
	}

	memset(buffer + converted, 0, null_size); /* NULL-terminate the output string */
	*target = buffer;
	*target_len = converted;

	/* Report the conversion error */
	if (U_FAILURE(*status)) {
		zend_error(E_NOTICE, "Error converting from codepage string to Unicode: %s", u_errorName(*status));
	}
}
/* }}} */

/* {{{ zend_raise_conversion_error_ex */
ZEND_API void zend_raise_conversion_error_ex(char *message, UConverter *conv, int error_char_offset, int use_exception TSRMLS_DC)
{
	UChar err_char[U16_MAX_LENGTH];
	int8_t err_char_len = sizeof(err_char);
	UChar32 codepoint;
	const char *conv_name;
	UErrorCode status = U_ZERO_ERROR;
	char *reason_fmt = "%s (converter %s failed on character {U+%04X} at offset %d)";
	char *no_reason_fmt = "%s";
	char *message_fmt;

	if (!message)
		return;

	ucnv_getInvalidUChars(conv, err_char, &err_char_len, &status);
	conv_name = ucnv_getName(conv, &status);
	/*
	 * UTODO
	 * use some other standard than MIME? or fallback onto IANA? or use
	 * internal converter name? ponder
	 */
	conv_name = ucnv_getStandardName(conv_name, "MIME", &status);
	codepoint = (err_char_len < 2) ? err_char[0] : U16_GET_SUPPLEMENTARY(err_char[0], err_char[1]);
	;

	message_fmt = conv ? reason_fmt : no_reason_fmt;

	if (use_exception) {
		zend_throw_exception_ex(unicodeConversionException, 0 TSRMLS_CC, message_fmt, message, conv_name?conv_name:"<unknown>", codepoint, error_char_offset);
	} else {
		zend_error(E_WARNING, message_fmt, message, conv_name?conv_name:"", codepoint, error_char_offset);
	}
}
/* }}} */

/* {{{ zval_unicode_to_string */
ZEND_API int zval_unicode_to_string(zval *string, UConverter *conv TSRMLS_DC)
{
	UErrorCode status = U_ZERO_ERROR;
	int retval = TRUE;
	char *s = NULL;
	int s_len;
	int num_conv;

	UChar *u = Z_USTRVAL_P(string);
	int u_len = Z_USTRLEN_P(string);

	num_conv = zend_convert_from_unicode(conv, &s, &s_len, u, u_len, &status);

	if (U_FAILURE(status)) {
		int32_t offset = u_countChar32(u, num_conv)-1;

		if (s) {
			efree(s);
		}
		zend_raise_conversion_error_ex("Could not convert Unicode string to binary string", conv, offset, (UG(from_u_error_mode) & ZEND_CONV_ERROR_EXCEPTION) TSRMLS_CC);
		retval = FAILURE;
	}

	ZVAL_STRINGL(string, s, s_len, 0);

	efree((UChar*)u);
	return retval;
}
/* }}} */

/* {{{ zval_string_to_unicode_ex */
ZEND_API int zval_string_to_unicode_ex(zval *string, UConverter *conv)
{
	UErrorCode status = U_ZERO_ERROR;
	int retval = TRUE;
	UChar *u = NULL;
	int u_len;

	char *s = Z_STRVAL_P(string);
	int s_len = Z_STRLEN_P(string);

	Z_TYPE_P(string) = IS_UNICODE;
	zend_convert_to_unicode(conv, &u, &u_len, s, s_len, &status);
	ZVAL_UNICODEL(string, u, u_len, 0);

	if (U_FAILURE(status)) {
		retval = FALSE;
	}

	efree(s);
	return retval;
}
/* }}} */

/* {{{ zval_string_to_unicode */
ZEND_API int zval_string_to_unicode(zval *string TSRMLS_DC)
{
	return zval_string_to_unicode_ex(string, ZEND_U_CONVERTER(UG(runtime_encoding_conv)));
}
/* }}} */

/* {{{ zend_cmp_unicode_and_string */
ZEND_API int zend_cmp_unicode_and_string(UChar *ustr, char* str, uint len)
{
	UErrorCode status = U_ZERO_ERROR;
	UChar *u = NULL;
	int u_len;
	int retval = TRUE;
	TSRMLS_FETCH();

	zend_convert_to_unicode(ZEND_U_CONVERTER(UG(runtime_encoding_conv)), &u, &u_len, str, len, &status);
	if (U_FAILURE(status)) {
		efree(u);
		return FALSE;
	}
	retval = u_memcmp(ustr, u, u_len);
	efree(u);
	return retval;
}
/* }}} */

/* {{{ zend_cmp_unicode_and_literal */
/*
 * Compare a Unicode string and an ASCII literal. Because ASCII maps nicely onto Unicode
 * range U+0000 .. U+007F, we can simply casst ASCII chars to Unicode values and avoid
 * memory allocation.
 */
ZEND_API int zend_cmp_unicode_and_literal(UChar *ustr, int ulen, char *str, int slen)
{
	int result;
	uint len = MIN(ulen, slen);

	while (len--) {
		result = (int)(uint16_t)*ustr - (int)(uint16_t)*str;
		if (result != 0)
			return result;
		ustr++;
		str++;
	}

	return ulen - slen;
}
/* }}} */

/* {{{ zend_is_valid_identifier */
ZEND_API int zend_is_valid_identifier(UChar *ident, int len)
{
	UChar32 codepoint;
	int32_t i;
	int32_t ident_len = len;
	UProperty id_prop = UCHAR_XID_START;

	for (i = 0; i < ident_len; ) {
		U16_NEXT(ident, i, ident_len, codepoint);
		if (!u_hasBinaryProperty(codepoint, id_prop) &&
				codepoint != 0x5f) { /* special case for starting '_' */
			return 0;
		}
		id_prop = UCHAR_XID_CONTINUE;
	}

	return 1;
}
/* }}} */

/* {{{ zend_normalize_string */
static inline void zend_normalize_string(UChar **dest, int32_t *dest_len, UChar *src, int src_len, UErrorCode *status)
{
	UChar *buffer = NULL;
	int32_t buffer_len;

	buffer_len = src_len;
	while (1) {
		*status = U_ZERO_ERROR;
		buffer = eurealloc(buffer, buffer_len+1);
		buffer_len = unorm_normalize(src, src_len, UNORM_NFKC, 0, buffer, buffer_len, status);
		if (*status != U_BUFFER_OVERFLOW_ERROR) {
			break;
		}
	}
	if (U_SUCCESS(*status)) {
		buffer[buffer_len] = 0;
		*dest = buffer;
		*dest_len = buffer_len;
	} else {
		efree(buffer);
	}
}
/* }}} */

/* {{{ zend_case_fold_string */
ZEND_API void zend_case_fold_string(UChar **dest, int *dest_len, UChar *src, int src_len, uint32_t options, UErrorCode *status)
{
	UChar *buffer = NULL;
	int32_t buffer_len;

	buffer_len = src_len;
	while (1) {
		*status = U_ZERO_ERROR;
		buffer = eurealloc(buffer, buffer_len+1);
		buffer_len = u_strFoldCase(buffer, buffer_len, src, src_len, options, status);
		if (*status != U_BUFFER_OVERFLOW_ERROR) {
			break;
		}
	}
	if (U_SUCCESS(*status)) {
		buffer[buffer_len] = 0;
		*dest = buffer;
		*dest_len = buffer_len;
	} else {
		efree(buffer);
	}
}
/* }}} */

/* {{{ zend_normalize_identifier */
ZEND_API int zend_normalize_identifier(UChar **dest, int *dest_len, UChar *ident, int ident_len, zend_bool fold_case)
{
	UChar *buffer = NULL;
	UChar *orig_ident = ident;
	int32_t buffer_len;
	UErrorCode status = U_ZERO_ERROR;

	if (unorm_quickCheck(ident, ident_len, UNORM_NFKC, &status) != UNORM_YES) {
		zend_normalize_string(&buffer, &buffer_len, ident, ident_len, &status);
		if (U_FAILURE(status)) {
			return 0;
		}
		ident = buffer;
		ident_len = buffer_len;
	}

	if (fold_case) {
		zend_case_fold_string(&buffer, &buffer_len, ident, ident_len, U_FOLD_CASE_DEFAULT, &status);
		if (ident != orig_ident) {
			efree(ident);
		}
		if (U_FAILURE(status)) {
			return 0;
		}
		ident = buffer;
		ident_len = buffer_len;

		if (unorm_quickCheck(ident, ident_len, UNORM_NFKC, &status) != UNORM_YES) {
			zend_normalize_string(&buffer, &buffer_len, ident, ident_len, &status);
			if (ident != orig_ident) {
				efree(ident);
			}
			if (U_FAILURE(status)) {
				return 0;
			}
			ident = buffer;
			ident_len = buffer_len;
		}
	}

	*dest = ident;
	*dest_len = ident_len;
	return 1;
}
/* }}} */

/* {{{ zend_register_unicode_exceptions */
void zend_register_unicode_exceptions(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "UnicodeConversionException", NULL);
	unicodeConversionException = zend_register_internal_class_ex(&ce, zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim: noet sw=4 ts=4 fdm=marker
 */
