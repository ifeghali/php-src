/* 
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Zeev Suraski <zeev@zend.com>                                |
   |          Thies C. Arntzen <thies@thieso.net>                         |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php.h"
#include "ext/standard/head.h"
#include "ext/session/php_session.h"
#include "ext/standard/basic_functions.h"
#include "SAPI.h"

/* output functions */
static int php_ub_body_write(const char *str, uint str_length);
static int php_ub_body_write_no_header(const char *str, uint str_length);
static int php_b_body_write(const char *str, uint str_length);

static void php_ob_init(uint initial_size, uint block_size, zval *output_handler, uint chunk_size);
static void php_ob_append(const char *text, uint text_length);
#if 0
static void php_ob_prepend(const char *text, uint text_length);
#endif


#ifdef ZTS
int output_globals_id;
#else
php_output_globals output_globals;
#endif

static int php_default_output_func(const char *str, uint str_len)
{
	fwrite(str, 1, str_len, stderr);
	return str_len;
}


static void php_output_init_globals(php_output_globals *output_globals_p TSRMLS_DC)
{
 	OG(php_body_write) = php_default_output_func;
	OG(php_header_write) = php_default_output_func;
	OG(implicit_flush) = 0;
	OG(output_start_filename) = NULL;
	OG(output_start_lineno) = 0;
}


/* Start output layer */
PHPAPI void php_output_startup(void)
{
#ifdef ZTS
	ts_allocate_id(&output_globals_id, sizeof(php_output_globals), (ts_allocate_ctor) php_output_init_globals, NULL);
#else 
	php_output_init_globals(&output_globals TSRMLS_CC);
#endif
}


PHPAPI void php_output_activate(void)
{
	TSRMLS_FETCH();

	OG(php_body_write) = php_ub_body_write;
	OG(php_header_write) = sapi_module.ub_write;
	OG(ob_nesting_level) = 0;
	OG(ob_lock) = 0;
	OG(disable_output) = 0;
}


PHPAPI void php_output_set_status(zend_bool status)
{
	TSRMLS_FETCH();

	OG(disable_output) = !status;
}


void php_output_register_constants()
{
	TSRMLS_FETCH();

	REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_START", PHP_OUTPUT_HANDLER_START, CONST_CS | CONST_PERSISTENT);
	REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_CONT", PHP_OUTPUT_HANDLER_CONT, CONST_CS | CONST_PERSISTENT);
	REGISTER_MAIN_LONG_CONSTANT("PHP_OUTPUT_HANDLER_END", PHP_OUTPUT_HANDLER_END, CONST_CS | CONST_PERSISTENT);
}


PHPAPI int php_body_write(const char *str, uint str_length)
{
	TSRMLS_FETCH();
	return OG(php_body_write)(str, str_length);	
}

PHPAPI int php_header_write(const char *str, uint str_length)
{
	TSRMLS_FETCH();
	if (OG(disable_output)) {
		return 0;
	} else {
		return OG(php_header_write)(str, str_length);
	}
}

/* {{{ php_start_ob_buffer
 * Start output buffering */
PHPAPI int php_start_ob_buffer(zval *output_handler, uint chunk_size)
{
	TSRMLS_FETCH();

	if (OG(ob_lock)) {
		return FAILURE;
	}
	if (chunk_size) {
		php_ob_init((chunk_size*3/2), chunk_size/2, output_handler, chunk_size);
	} else {
		php_ob_init(40*1024, 10*1024, output_handler, chunk_size);
	}
	OG(php_body_write) = php_b_body_write;
	return SUCCESS;
}
/* }}} */

/* {{{ php_end_ob_buffer
 * End output buffering (one level) */
PHPAPI void php_end_ob_buffer(zend_bool send_buffer, zend_bool just_flush)
{
	char *final_buffer=NULL;
	int final_buffer_length=0;
	zval *alternate_buffer=NULL;
	char *to_be_destroyed_buffer;
	char *to_be_destroyed_handled_output[2] = { 0, 0 };
	int status;
	TSRMLS_FETCH();

	if (OG(ob_nesting_level)==0) {
		return;
	}
	status = 0;
	if (!OG(active_ob_buffer).status & PHP_OUTPUT_HANDLER_START) {
		/* our first call */
		status |= PHP_OUTPUT_HANDLER_START;
	}
	if (just_flush) {
		status |= PHP_OUTPUT_HANDLER_CONT;
	} else {
		status |= PHP_OUTPUT_HANDLER_END;
	}

	if (OG(active_ob_buffer).internal_output_handler) {
		final_buffer = OG(active_ob_buffer).internal_output_handler_buffer;
		final_buffer_length = OG(active_ob_buffer).internal_output_handler_buffer_size;
		OG(active_ob_buffer).internal_output_handler(OG(active_ob_buffer).buffer, OG(active_ob_buffer).text_length, &final_buffer, &final_buffer_length, status);
	} else if (OG(active_ob_buffer).output_handler) {
		zval **params[2];
		zval *orig_buffer;
		zval *z_status;
		TSRMLS_FETCH();

		ALLOC_INIT_ZVAL(orig_buffer);
		ZVAL_STRINGL(orig_buffer,OG(active_ob_buffer).buffer,OG(active_ob_buffer).text_length,0);
		orig_buffer->refcount=2;	/* don't let call_user_function() destroy our buffer */
		orig_buffer->is_ref=1;

		ALLOC_INIT_ZVAL(z_status);
		ZVAL_LONG(z_status,status);

		params[0] = &orig_buffer;
		params[1] = &z_status;
		OG(ob_lock) = 1;
		if (call_user_function_ex(CG(function_table), NULL, OG(active_ob_buffer).output_handler, &alternate_buffer, 2, params, 1, NULL TSRMLS_CC)==SUCCESS) {
			convert_to_string_ex(&alternate_buffer);
			final_buffer = alternate_buffer->value.str.val;
			final_buffer_length = alternate_buffer->value.str.len;
		}
		OG(ob_lock) = 0;
		zval_ptr_dtor(&OG(active_ob_buffer).output_handler);
		if (orig_buffer->refcount==2) { /* free the zval */
			FREE_ZVAL(orig_buffer);
		} else {
			orig_buffer->refcount-=2;
		}
		zval_ptr_dtor(&z_status);
	}

	if (!final_buffer) {
		final_buffer = OG(active_ob_buffer).buffer;
		final_buffer_length = OG(active_ob_buffer).text_length;
	}

	if (OG(ob_nesting_level)==1) { /* end buffering */
		if (SG(headers_sent) && !SG(request_info).headers_only) {
			OG(php_body_write) = php_ub_body_write_no_header;
		} else {
			OG(php_body_write) = php_ub_body_write;
		}
	}

	to_be_destroyed_buffer = OG(active_ob_buffer).buffer;
	if (OG(active_ob_buffer).internal_output_handler
		&& (final_buffer != OG(active_ob_buffer).internal_output_handler_buffer)) {
		to_be_destroyed_handled_output[0] = final_buffer;
	}

	if (!just_flush) {
		if (OG(active_ob_buffer).internal_output_handler) {
			to_be_destroyed_handled_output[1] = OG(active_ob_buffer).internal_output_handler_buffer;
		}
		if (OG(ob_nesting_level)>1) { /* restore previous buffer */
			php_ob_buffer *ob_buffer_p;

			zend_stack_top(&OG(ob_buffers), (void **) &ob_buffer_p);
			OG(active_ob_buffer) = *ob_buffer_p;
			zend_stack_del_top(&OG(ob_buffers));
			if (OG(ob_nesting_level)==2) { /* destroy the stack */
				zend_stack_destroy(&OG(ob_buffers));
			}
		}
		OG(ob_nesting_level)--;
	}

	if (send_buffer) {
		OG(php_body_write)(final_buffer, final_buffer_length);
	}

	if (alternate_buffer) {
		zval_ptr_dtor(&alternate_buffer);
	}

	if (!just_flush) {
		efree(to_be_destroyed_buffer);
	} else {
		OG(active_ob_buffer).text_length = 0;
		OG(active_ob_buffer).status |= PHP_OUTPUT_HANDLER_START;
		OG(php_body_write) = php_b_body_write;
	}
	if (to_be_destroyed_handled_output[0]) {
		efree(to_be_destroyed_handled_output[0]);
	}
	if (to_be_destroyed_handled_output[1]) {
		efree(to_be_destroyed_handled_output[1]);
	}
}
/* }}} */

/* {{{ php_end_ob_buffers
 * End output buffering (all buffers) */
PHPAPI void php_end_ob_buffers(zend_bool send_buffer)
{
	TSRMLS_FETCH();

	while (OG(ob_nesting_level)!=0) {
		php_end_ob_buffer(send_buffer, 0);
	}

	if (!OG(disable_output) && send_buffer && BG(use_trans_sid)) {
		session_adapt_flush(OG(php_header_write));
	}
}
/* }}} */

/* {{{ php_start_implicit_flush
 */
PHPAPI void php_start_implicit_flush()
{
	TSRMLS_FETCH();

	php_end_ob_buffer(1, 0);		/* Switch out of output buffering if we're in it */
	OG(implicit_flush)=1;
}
/* }}} */

/* {{{ php_end_implicit_flush
 */
PHPAPI void php_end_implicit_flush()
{
	TSRMLS_FETCH();

	OG(implicit_flush)=0;
}
/* }}} */

/* {{{ php_ob_set_internal_handler
 */
PHPAPI void php_ob_set_internal_handler(php_output_handler_func_t internal_output_handler, uint buffer_size)
{
	TSRMLS_FETCH();

	if (OG(ob_nesting_level)==0) {
		return;
	}

	OG(active_ob_buffer).internal_output_handler = internal_output_handler;
	OG(active_ob_buffer).internal_output_handler_buffer = (char *) emalloc(buffer_size);
	OG(active_ob_buffer).internal_output_handler_buffer_size = buffer_size;
}
/* }}} */

/*
 * Output buffering - implementation
 */

/* {{{ php_ob_allocate
 */
static inline void php_ob_allocate(void)
{
	TSRMLS_FETCH();

	if (OG(active_ob_buffer).size<OG(active_ob_buffer).text_length) {
		while (OG(active_ob_buffer).size <= OG(active_ob_buffer).text_length) {
			OG(active_ob_buffer).size+=OG(active_ob_buffer).block_size;
		}
			
		OG(active_ob_buffer).buffer = (char *) erealloc(OG(active_ob_buffer).buffer, OG(active_ob_buffer).size+1);
	}
}
/* }}} */

/* {{{ php_ob_init
 */
static void php_ob_init(uint initial_size, uint block_size, zval *output_handler, uint chunk_size)
{
	TSRMLS_FETCH();

	if (OG(ob_nesting_level)>0) {
		if (OG(ob_nesting_level)==1) { /* initialize stack */
			zend_stack_init(&OG(ob_buffers));
		}
		zend_stack_push(&OG(ob_buffers), &OG(active_ob_buffer), sizeof(php_ob_buffer));
	}
	OG(ob_nesting_level)++;
	OG(active_ob_buffer).block_size = block_size;
	OG(active_ob_buffer).size = initial_size;
	OG(active_ob_buffer).buffer = (char *) emalloc(initial_size+1);
	OG(active_ob_buffer).text_length = 0;
	OG(active_ob_buffer).output_handler = output_handler;
	OG(active_ob_buffer).chunk_size = chunk_size;
	OG(active_ob_buffer).status = 0;
	OG(active_ob_buffer).internal_output_handler = NULL;
}
/* }}} */

/* {{{ php_ob_append
 */
static void php_ob_append(const char *text, uint text_length)
{
	char *target;
	int original_ob_text_length;
	TSRMLS_FETCH();

	original_ob_text_length=OG(active_ob_buffer).text_length;
	OG(active_ob_buffer).text_length = OG(active_ob_buffer).text_length + text_length;

	php_ob_allocate();
	target = OG(active_ob_buffer).buffer+original_ob_text_length;
	memcpy(target, text, text_length);
	target[text_length]=0;

	if (OG(active_ob_buffer).chunk_size
		&& OG(active_ob_buffer).text_length >= OG(active_ob_buffer).chunk_size) {
		zval *output_handler = OG(active_ob_buffer).output_handler;

		if (output_handler) {
			output_handler->refcount++;
		}
		php_end_ob_buffer(1, 1);
		return;
	}
}
/* }}} */

#if 0
static void php_ob_prepend(const char *text, uint text_length)
{
	char *p, *start;
	TSRMLS_FETCH();

	OG(active_ob_buffer).text_length += text_length;
	php_ob_allocate();

	/* php_ob_allocate() may change OG(ob_buffer), so we can't initialize p&start earlier */
	p = OG(ob_buffer)+OG(ob_text_length);
	start = OG(ob_buffer);

	while (--p>=start) {
		p[text_length] = *p;
	}
	memcpy(OG(ob_buffer), text, text_length);
	OG(ob_buffer)[OG(active_ob_buffer).text_length]=0;
}
#endif


/* {{{ php_ob_get_buffer
 * Return the current output buffer */
int php_ob_get_buffer(pval *p)
{
	TSRMLS_FETCH();

	if (OG(ob_nesting_level)==0) {
		return FAILURE;
	}
	ZVAL_STRINGL(p,OG(active_ob_buffer).buffer,OG(active_ob_buffer).text_length,1);
	return SUCCESS;
}
/* }}} */

/* {{{ php_ob_get_length
 * Return the size of the current output buffer */
int php_ob_get_length(pval *p)
{
	TSRMLS_FETCH();

	if (OG(ob_nesting_level) == 0) {
		return FAILURE;
	}
	ZVAL_LONG(p,OG(active_ob_buffer).text_length);
	return SUCCESS;
}
/* }}} */

/*
 * Wrapper functions - implementation
 */


/* buffered output function */
static int php_b_body_write(const char *str, uint str_length)
{
	php_ob_append(str, str_length);
	return str_length;
}

/* {{{ php_ub_body_write_no_header
 */
static int php_ub_body_write_no_header(const char *str, uint str_length)
{
	char *newstr = NULL;
	size_t new_length=0;
	int result;
	TSRMLS_FETCH();

	if (OG(disable_output)) {
		return 0;
	}
	if (BG(use_trans_sid)) {
		session_adapt_uris(str, str_length, &newstr, &new_length);
	}
		
	if (newstr) {
		str = newstr;
		str_length = new_length;
	}

	result = OG(php_header_write)(str, str_length);

	if (OG(implicit_flush)) {
		sapi_flush();
	}

	return result;
}
/* }}} */

/* {{{ php_ub_body_write
 */
static int php_ub_body_write(const char *str, uint str_length)
{
	int result = 0;
	TSRMLS_FETCH();

	if (SG(request_info).headers_only) {
		php_header();
		zend_bailout();
	}
	if (php_header()) {
		if (zend_is_compiling(TSRMLS_C)) {
			OG(output_start_filename) = zend_get_compiled_filename(TSRMLS_C);
			OG(output_start_lineno) = zend_get_compiled_lineno(TSRMLS_C);
		} else if (zend_is_executing(TSRMLS_C)) {
			OG(output_start_filename) = zend_get_executed_filename(TSRMLS_C);
			OG(output_start_lineno) = zend_get_executed_lineno(TSRMLS_C);
		}

		OG(php_body_write) = php_ub_body_write_no_header;
		result = php_ub_body_write_no_header(str, str_length);
	}

	return result;
}
/* }}} */

/*
 * HEAD support
 */

/* {{{ proto void ob_start([ string user_function [, int chunk_size]])
   Turn on Output Buffering (specifying an optional output handler). */
PHP_FUNCTION(ob_start)
{
	zval *output_handler;
	uint chunk_size=0;

	switch (ZEND_NUM_ARGS()) {
		case 0:
			output_handler = NULL;
			break;
		case 1: {
				zval **output_handler_p;

				if (zend_get_parameters_ex(1, &output_handler_p)==FAILURE) {
					RETURN_FALSE;
				}
				SEPARATE_ZVAL(output_handler_p);
				output_handler = *output_handler_p;
				output_handler->refcount++;
			}
			break;
		case 2: {
				zval **output_handler_p, **chunk_size_p;

				if (zend_get_parameters_ex(2, &output_handler_p, &chunk_size_p)==FAILURE) {
					RETURN_FALSE;
				}
				SEPARATE_ZVAL(output_handler_p);
				output_handler = *output_handler_p;
				output_handler->refcount++;
				convert_to_long_ex(chunk_size_p);
				chunk_size = (uint) Z_LVAL_PP(chunk_size_p);
			}
			break;
		default:
			ZEND_WRONG_PARAM_COUNT();
			break;
	}
	if (php_start_ob_buffer(output_handler, chunk_size)==FAILURE) {
		TSRMLS_FETCH();

		if (SG(headers_sent) && !SG(request_info).headers_only) {
			OG(php_body_write) = php_ub_body_write_no_header;
		} else {
			OG(php_body_write) = php_ub_body_write;
		}
		OG(ob_nesting_level) = 0;
		php_error(E_ERROR, "Cannot use output buffering in output buffering display handlers");
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void ob_end_flush(void)
   Flush (send) the output buffer, and turn off output buffering */
PHP_FUNCTION(ob_end_flush)
{
	php_end_ob_buffer(1, 0);
}
/* }}} */

/* {{{ proto void ob_end_clean(void)
   Clean (erase) the output buffer, and turn off output buffering */
PHP_FUNCTION(ob_end_clean)
{
	php_end_ob_buffer(0, 0);
}
/* }}} */

/* {{{ proto string ob_get_contents(void)
   Return the contents of the output buffer */
PHP_FUNCTION(ob_get_contents)
{
	if (php_ob_get_buffer(return_value)==FAILURE) {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string ob_get_length(void)
   Return the length of the output buffer */
PHP_FUNCTION(ob_get_length)
{
	if (php_ob_get_length(return_value)==FAILURE) {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto void ob_implicit_flush([int flag])
   Turn implicit flush on/off and is equivalent to calling flush() after every output call */
PHP_FUNCTION(ob_implicit_flush)
{
	zval **zv_flag;
	int flag;

	switch(ZEND_NUM_ARGS()) {
		case 0:
			flag = 1;
			break;
		case 1:
			if (zend_get_parameters_ex(1, &zv_flag)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(zv_flag);
			flag = (*zv_flag)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	if (flag) {
		php_start_implicit_flush();
	} else {
		php_end_implicit_flush();
	}
}
/* }}} */

PHPAPI char *php_get_output_start_filename()
{
	TSRMLS_FETCH();

	return OG(output_start_filename);
}


PHPAPI int php_get_output_start_lineno()
{
	TSRMLS_FETCH();

	return OG(output_start_lineno);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 tw=78 fdm=marker
 * vim<600: sw=4 ts=4 tw=78
 */
