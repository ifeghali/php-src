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
   | Authors: Andi Gutmans <andi@zend.com>                                |
   +----------------------------------------------------------------------+
 */

#include "php.h"

#if WITH_BCMATH

#include "number.h"
#include "php_bcmath.h"

function_entry bcmath_functions[] = {
	PHP_FE(bcadd,									NULL)
	PHP_FE(bcsub,									NULL)
	PHP_FE(bcmul,									NULL)
	PHP_FE(bcdiv,									NULL)
	PHP_FE(bcmod,									NULL)
	PHP_FE(bcpow,									NULL)
	PHP_FE(bcsqrt,									NULL)
	PHP_FE(bcscale,									NULL)
	PHP_FE(bccomp,									NULL)
	{NULL, NULL, NULL}
};

zend_module_entry bcmath_module_entry = {
	"bcmath", bcmath_functions, NULL, NULL, PHP_RINIT(bcmath), PHP_RSHUTDOWN(bcmath), NULL, STANDARD_MODULE_PROPERTIES
};

#if defined(COMPILE_DL) || defined(COMPILE_DL_BCMATH)
ZEND_GET_MODULE(bcmath)
#endif

#ifndef THREAD_SAFE
static long bc_precision;
#endif

PHP_RINIT_FUNCTION(bcmath)
{
	init_numbers();
	if (cfg_get_long("bcmath.scale",&bc_precision)==FAILURE) {
		bc_precision=0;
	}
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(bcmath)
{
	destruct_numbers();
	return SUCCESS;
}

/* {{{ proto string bcadd(string left_operand, string right_operand [, int scale])
   Returns the sum of two arbitrary precision numbers */
PHP_FUNCTION(bcadd)
{
	pval **left, **right,**scale_param;
	bc_num first, second, result;
	int scale=bc_precision;

	switch (ARG_COUNT(ht)) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 3:
				if (zend_get_parameters_ex(3, &left,&right, &scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	convert_to_string_ex(right);
	init_num(&first);
	init_num(&second);
	init_num(&result);
	str2num(&first,(*left)->value.str.val,scale);
	str2num(&second,(*right)->value.str.val,scale);
	bc_add (first,second,&result, scale);
	return_value->value.str.val = num2str(result);
	return_value->value.str.len = strlen(return_value->value.str.val);
	return_value->type = IS_STRING;
	free_num(&first);
	free_num(&second);
	free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bcsub(string left_operand, string right_operand [, int scale])
   Returns the difference between two arbitrary precision numbers */
PHP_FUNCTION(bcsub)
{
	pval **left, **right,**scale_param;
	bc_num first, second, result;
	int scale=bc_precision;

	switch (ARG_COUNT(ht)) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 3:
				if (zend_get_parameters_ex(3, &left,&right, &scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	convert_to_string_ex(right);
	init_num(&first);
	init_num(&second);
	init_num(&result);
	str2num(&first,(*left)->value.str.val,scale);
	str2num(&second,(*right)->value.str.val,scale);
	bc_sub (first,second,&result, scale);
	return_value->value.str.val = num2str(result);
	return_value->value.str.len = strlen(return_value->value.str.val);
	return_value->type = IS_STRING;
	free_num(&first);
	free_num(&second);
	free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bcmul(string left_operand, string right_operand [, int scale])
   Returns the multiplication of two arbitrary precision numbers */
PHP_FUNCTION(bcmul)
{
	pval **left, **right,**scale_param;
	bc_num first, second, result;
	int scale=bc_precision;

	switch (ARG_COUNT(ht)) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 3:
				if (zend_get_parameters_ex(3, &left,&right, &scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	convert_to_string_ex(right);
	init_num(&first);
	init_num(&second);
	init_num(&result);
	str2num(&first,(*left)->value.str.val,scale);
	str2num(&second,(*right)->value.str.val,scale);
	bc_multiply (first,second,&result, scale);
	return_value->value.str.val = num2str(result);
	return_value->value.str.len = strlen(return_value->value.str.val);
	return_value->type = IS_STRING;
	free_num(&first);
	free_num(&second);
	free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bcdiv(string left_operand, string right_operand [, int scale])
   Returns the quotient of two arbitrary precision numbers (division) */
PHP_FUNCTION(bcdiv)
{
	pval **left, **right,**scale_param;
	bc_num first, second, result;
	int scale=bc_precision;

	switch (ARG_COUNT(ht)) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 3:
				if (zend_get_parameters_ex(3, &left,&right, &scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	convert_to_string_ex(right);
	init_num(&first);
	init_num(&second);
	init_num(&result);
	str2num(&first,(*left)->value.str.val,scale);
	str2num(&second,(*right)->value.str.val,scale);
	switch (bc_divide (first,second,&result, scale)) {
		case 0: /* OK */
			return_value->value.str.val = num2str(result);
			return_value->value.str.len = strlen(return_value->value.str.val);
			return_value->type = IS_STRING;
			break;
		case -1: /* division by zero */
			php_error(E_WARNING,"Division by zero");
			break;
	}
	free_num(&first);
	free_num(&second);
	free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bcmod(string left_operand, string right_operand)
   Returns the modulus of the two arbitrary precision operands */
PHP_FUNCTION(bcmod)
{
	pval **left, **right;
	bc_num first, second, result;

	switch (ARG_COUNT(ht)) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	convert_to_string_ex(right);
	init_num(&first);
	init_num(&second);
	init_num(&result);
	str2num(&first,(*left)->value.str.val,0);
	str2num(&second,(*right)->value.str.val,0);
	switch (bc_modulo(first,second,&result, 0)) {
		case 0:
			return_value->value.str.val = num2str(result);
			return_value->value.str.len = strlen(return_value->value.str.val);
			return_value->type = IS_STRING;
			break;
		case -1:
			php_error(E_WARNING,"Division by zero");
			break;
	}
	free_num(&first);
	free_num(&second);
	free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bcpow(string x, string y [, int scale])
   Returns the value of an arbitrary precision number raised to the power of another */
PHP_FUNCTION(bcpow)
{
	pval **left, **right,**scale_param;
	bc_num first, second, result;
	int scale=bc_precision;

	switch (ARG_COUNT(ht)) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 3:
				if (zend_get_parameters_ex(3, &left,&right, &scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	convert_to_string_ex(right);
	init_num(&first);
	init_num(&second);
	init_num(&result);
	str2num(&first,(*left)->value.str.val,scale);
	str2num(&second,(*right)->value.str.val,scale);
	bc_raise (first,second,&result, scale); 
	return_value->value.str.val = num2str(result);
	return_value->value.str.len = strlen(return_value->value.str.val);
	return_value->type = IS_STRING;
	free_num(&first);
	free_num(&second);
	free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bcsqrt(string operand [, int scale])
   Returns the square root of an arbitray precision number */
PHP_FUNCTION(bcsqrt)
{
	pval **left,**scale_param;
	bc_num result;
	int scale=bc_precision;

	switch (ARG_COUNT(ht)) {
		case 1:
				if (zend_get_parameters_ex(1, &left)== FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 2:
				if (zend_get_parameters_ex(2, &left,&scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}
	convert_to_string_ex(left);
	init_num(&result);
	str2num(&result,(*left)->value.str.val,scale);
	if (bc_sqrt (&result, scale) != 0) {
		return_value->value.str.val = num2str(result);
		return_value->value.str.len = strlen(return_value->value.str.val);
		return_value->type = IS_STRING;
	} else {
		php_error(E_WARNING,"Square root of negative number");
	}
	free_num(&result);
	return;
}
/* }}} */

/* {{{ proto string bccomp(string left_operand, string right_operand [, int scale])
   Compares two arbitrary precision numbers */
PHP_FUNCTION(bccomp)
{
	pval **left, **right, **scale_param;
	bc_num first, second;
	int scale=bc_precision;

	switch (ARG_COUNT(ht)) {
		case 2:
				if (zend_get_parameters_ex(2, &left,&right) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				break;
		case 3:
				if (zend_get_parameters_ex(3, &left,&right, &scale_param) == FAILURE) {
		        	WRONG_PARAM_COUNT;
    			}
				convert_to_long_ex(scale_param);
				scale = (int) (*scale_param)->value.lval;
				break;
		default:
				WRONG_PARAM_COUNT;
				break;
	}

	convert_to_string_ex(left);
	convert_to_string_ex(right);
	init_num(&first);
	init_num(&second);

	str2num(&first,(*left)->value.str.val,scale);
	str2num(&second,(*right)->value.str.val,scale);
	return_value->value.lval = bc_compare(first,second);
	return_value->type = IS_LONG;

	free_num(&first);
	free_num(&second);
	return;
}
/* }}} */

/* {{{ proto string bcscale(int scale)
   Sets default scale parameter for all bc math functions */
PHP_FUNCTION(bcscale)
{
	pval **new_scale;
	
	if (ARG_COUNT(ht)!=1 || zend_get_parameters_ex(1,&new_scale)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_long_ex(new_scale);
	bc_precision = (*new_scale)->value.lval;
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
