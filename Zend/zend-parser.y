%{
/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998, 1999 Andi Gutmans, Zeev Suraski                  |
   +----------------------------------------------------------------------+
   | This source file is subject to version 0.91 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available at through the world-wide-web at                           |
   | http://www.zend.com/license/0_91.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* 
 * LALR shift/reduce conflicts and how they are resolved:
 *
 * - 2 shift/reduce conflicts due to the dangeling elseif/else ambiguity.  Solved by shift.
 * - 1 shift/reduce conflict due to arrays within encapsulated strings. Solved by shift. 
 * - 1 shift/reduce conflict due to objects within encapsulated strings.  Solved by shift.
 * 
 */

#include "zend_compile.h"
#include "zend.h"
#include "zend_list.h"
#include "zend_globals.h"
#include "zend_API.h"
#include "zend_variables.h"
#include "zend_operators.h"


#define YYERROR_VERBOSE

%}

%pure_parser
%expect 4

%left T_INCLUDE T_EVAL
%left ','
%left T_LOGICAL_OR
%left T_LOGICAL_XOR
%left T_LOGICAL_AND
%right T_PRINT
%left '=' T_PLUS_EQUAL T_MINUS_EQUAL T_MUL_EQUAL T_DIV_EQUAL T_CONCAT_EQUAL T_MOD_EQUAL T_AND_EQUAL T_OR_EQUAL XT_OR_EQUAL T_SL_EQUAL T_SR_EQUAL
%left '?' ':'
%left T_BOOLEAN_OR
%left T_BOOLEAN_AND
%left '|'
%left '^'
%left '&'
%nonassoc T_IS_EQUAL T_IS_NOT_EQUAL
%nonassoc '<' T_IS_SMALLER_OR_EQUAL '>' T_IS_GREATER_OR_EQUAL
%left T_SL T_SR
%left '+' '-' '.'
%left '*' '/' '%'
%right '!' '~' T_INC T_DEC T_INT_CAST T_DOUBLE_CAST T_STRING_CAST T_ARRAY_CAST T_OBJECT_CAST T_BOOL_CAST '@'
%right '['
%nonassoc T_NEW
%token T_EXIT
%token T_IF
%left T_ELSEIF
%left T_ELSE
%left T_ENDIF
%token T_LNUMBER
%token T_DNUMBER
%token T_STRING
%token T_STRING_VARNAME
%token T_VARIABLE
%token T_NUM_STRING
%token T_INLINE_HTML
%token T_CHARACTER
%token T_BAD_CHARACTER
%token T_ENCAPSED_AND_WHITESPACE
%token T_CONSTANT_ENCAPSED_STRING
%token T_ECHO
%token T_DO
%token T_WHILE
%token T_ENDWHILE
%token T_FOR
%token T_ENDFOR
%token T_FOREACH
%token T_ENDFOREACH
%token T_AS
%token T_SWITCH
%token T_ENDSWITCH
%token T_CASE
%token T_DEFAULT
%token T_BREAK
%token T_CONTINUE
%token T_OLD_FUNCTION
%token T_FUNCTION
%token T_CONST
%token T_RETURN
%token T_REQUIRE
%token T_GLOBAL
%token T_STATIC
%token T_VAR
%token T_UNSET
%token T_ISSET
%token T_EMPTY
%token T_CLASS
%token T_EXTENDS
%token T_OBJECT_OPERATOR
%token T_DOUBLE_ARROW
%token T_LIST
%token T_ARRAY
%token T_LINE
%token T_FILE
%token T_COMMENT
%token T_ML_COMMENT
%token T_OPEN_TAG
%token T_OPEN_TAG_WITH_ECHO
%token T_CLOSE_TAG
%token T_WHITESPACE
%token T_HEREDOC
%token T_DOLLAR_OPEN_CURLY_BRACES
%token T_CURLY_OPEN
%token T_PAAMAYIM_NEKUDOTAYIM
%token T_PHP_TRACK_VARS

%% /* Rules */

top_statement_list:	
		top_statement_list  { do_extended_info(CLS_C); } top_statement { ELS_FETCH(); HANDLE_INTERACTIVE(); }
	|	/* empty */
;


top_statement:
		statement
	|	declaration_statement	{ do_early_binding(CLS_C); }
;


inner_statement_list:
		inner_statement_list  { do_extended_info(CLS_C); } inner_statement { ELS_FETCH(); HANDLE_INTERACTIVE(); }
	|	/* empty */
;


inner_statement:
		statement
	|	declaration_statement
;


statement:
		'{' inner_statement_list '}'
	|	T_IF '(' expr ')' { do_if_cond(&$3, &$4 CLS_CC); } statement { do_if_after_statement(&$4, 1 CLS_CC); } elseif_list else_single { do_if_end(CLS_C); }
	|	T_IF '(' expr ')' ':' { do_if_cond(&$3, &$4 CLS_CC); } inner_statement_list { do_if_after_statement(&$4, 1 CLS_CC); } new_elseif_list new_else_single T_ENDIF ';' { do_if_end(CLS_C); }
	|	T_WHILE '(' { $1.u.opline_num = get_next_op_number(CG(active_op_array));  } expr  ')' { do_while_cond(&$4, &$5 CLS_CC); } while_statement { do_while_end(&$1, &$5 CLS_CC); }
	|	T_DO { $1.u.opline_num = get_next_op_number(CG(active_op_array));  do_do_while_begin(CLS_C); } statement T_WHILE '(' expr ')' ';' { do_do_while_end(&$1, &$6 CLS_CC); }
	|	T_FOR 
			'('
				for_expr
			';' { do_free(&$3 CLS_CC); $4.u.opline_num = get_next_op_number(CG(active_op_array)); }
				for_expr
			';' { do_for_cond(&$6, &$7 CLS_CC); }
				for_expr
			')' { do_free(&$9 CLS_CC); do_for_before_statement(&$4, &$7 CLS_CC); }
			for_statement { do_for_end(&$7 CLS_CC); }
	|	T_SWITCH '(' expr ')' { do_switch_cond(&$3 CLS_CC); } switch_case_list { do_switch_end(&$6 CLS_CC); }
	|	T_BREAK ';'		 	{ do_brk_cont(ZEND_BRK, NULL CLS_CC); }
	|	T_BREAK expr ';'		{ do_brk_cont(ZEND_BRK, &$2 CLS_CC); }
	|	T_CONTINUE ';'		{ do_brk_cont(ZEND_CONT, NULL CLS_CC); }
	|	T_CONTINUE expr ';'	{ do_brk_cont(ZEND_CONT, &$2 CLS_CC); }
	|	T_RETURN ';'			{ do_return(NULL CLS_CC); }
	|	T_RETURN expr ';'		{ do_return(&$2 CLS_CC); }
	|	T_GLOBAL global_var_list
	|	T_STATIC static_var_list
	|	T_ECHO echo_expr_list ';'
	|	T_INLINE_HTML			{ do_echo(&$1 CLS_CC); }
	|	expr ';'			{ do_free(&$1 CLS_CC); }
	|	T_REQUIRE expr ';'			{ if ($2.op_type==IS_CONST && $2.u.constant.type==IS_STRING) { require_filename($2.u.constant.value.str.val CLS_CC); zval_dtor(&$2.u.constant); } else { do_include_or_eval(ZEND_INCLUDE, &$$, &$2 CLS_CC); } }
	|	T_UNSET '(' r_cvar ')' ';' { do_unset(&$3 CLS_CC); }
	|	T_FOREACH '(' expr T_AS { do_foreach_begin(&$1, &$3, &$2, &$4 CLS_CC); } w_cvar foreach_optional_arg ')' { do_foreach_cont(&$6, &$7, &$4 CLS_CC); } foreach_statement { do_foreach_end(&$1, &$2 CLS_CC); }
	|	';'		/* empty statement */
;


declaration_statement:
		T_FUNCTION { $1.u.opline_num = CG(zend_lineno); } T_STRING { do_begin_function_declaration(&$1, &$3, 0 CLS_CC); }
			'(' parameter_list ')' '{' inner_statement_list '}' { do_end_function_declaration(&$1 CLS_CC); }
	|	T_OLD_FUNCTION { $1.u.opline_num = CG(zend_lineno); } T_STRING  { do_begin_function_declaration(&$1, &$3, 0 CLS_CC); }
			parameter_list '(' inner_statement_list ')' ';' { do_end_function_declaration(&$1 CLS_CC); }
	|	T_CLASS T_STRING { do_begin_class_declaration(&$2, NULL CLS_CC); } '{' class_statement_list '}' { do_end_class_declaration(CLS_C); }
	|	T_CLASS T_STRING T_EXTENDS T_STRING { do_begin_class_declaration(&$2, &$4 CLS_CC); } '{' class_statement_list '}' { do_end_class_declaration(CLS_C); }
;


foreach_optional_arg:
		/* empty */		{ $$.op_type = IS_UNUSED; }
	|	T_DOUBLE_ARROW w_cvar		{ $$ = $2; }
;


for_statement:
		statement
	|	':' inner_statement_list T_ENDFOR ';'
;


foreach_statement:
		statement
	|	':' inner_statement_list T_ENDFOREACH ';'
;


switch_case_list:
		'{' case_list '}'					{ $$ = $2; }
	|	'{' ';' case_list '}'				{ $$ = $3; }
	|	':' case_list T_ENDSWITCH ';'			{ $$ = $2; }
	|	':' ';' case_list T_ENDSWITCH ';'		{ $$ = $3; }
;


case_list:
		/* empty */	{ $$.u.opline_num = -1; }
	|	case_list T_CASE expr case_separator { do_case_before_statement(&$1, &$2, &$3 CLS_CC); } inner_statement_list { do_case_after_statement(&$$, &$2 CLS_CC); }
	|	case_list T_DEFAULT case_separator { do_default_before_statement(&$1, &$2 CLS_CC); } inner_statement_list { do_case_after_statement(&$$, &$2 CLS_CC); }
;


case_separator:
		':'
	|	';'
;


while_statement:
		statement
	|	':' inner_statement_list T_ENDWHILE ';'
;



elseif_list:
		/* empty */
	|	elseif_list T_ELSEIF '(' expr ')' { do_if_cond(&$4, &$5 CLS_CC); } statement { do_if_after_statement(&$5, 0 CLS_CC); }
;


new_elseif_list:
		/* empty */
	|	new_elseif_list T_ELSEIF '(' expr ')' ':' { do_if_cond(&$4, &$5 CLS_CC); } inner_statement_list { do_if_after_statement(&$5, 0 CLS_CC); }
;


else_single:
		/* empty */
	|	T_ELSE statement
;


new_else_single:
		/* empty */
	|	T_ELSE ':' inner_statement_list
;




parameter_list: 
		non_empty_parameter_list
	|	/* empty */
;


non_empty_parameter_list:
		T_VARIABLE						{ znode tmp;  fetch_simple_variable(&tmp, &$1, 0 CLS_CC); $$.op_type = IS_CONST; $$.u.constant.value.lval=1; $$.u.constant.type=IS_LONG; INIT_PZVAL(&$$.u.constant); do_receive_arg(ZEND_RECV, &tmp, &$$, NULL, BYREF_NONE CLS_CC); }
	|	'&' T_VARIABLE					{ znode tmp;  fetch_simple_variable(&tmp, &$2, 0 CLS_CC); $$.op_type = IS_CONST; $$.u.constant.value.lval=1; $$.u.constant.type=IS_LONG; INIT_PZVAL(&$$.u.constant); do_receive_arg(ZEND_RECV, &tmp, &$$, NULL, BYREF_FORCE CLS_CC); }
	|	T_CONST T_VARIABLE 			{ znode tmp;  fetch_simple_variable(&tmp, &$2, 0 CLS_CC); $$.op_type = IS_CONST; $$.u.constant.value.lval=1; $$.u.constant.type=IS_LONG; INIT_PZVAL(&$$.u.constant); do_receive_arg(ZEND_RECV, &tmp, &$$, NULL, BYREF_NONE CLS_CC); }
	|	T_VARIABLE '=' static_scalar		{ znode tmp;  fetch_simple_variable(&tmp, &$1, 0 CLS_CC); $$.op_type = IS_CONST; $$.u.constant.value.lval=1; $$.u.constant.type=IS_LONG; INIT_PZVAL(&$$.u.constant); do_receive_arg(ZEND_RECV_INIT, &tmp, &$$, &$3, BYREF_NONE CLS_CC); }
	|	T_VARIABLE '=' T_UNSET 		{ znode tmp;  fetch_simple_variable(&tmp, &$1, 0 CLS_CC); $$.op_type = IS_CONST; $$.u.constant.value.lval=1; $$.u.constant.type=IS_LONG; INIT_PZVAL(&$$.u.constant); do_receive_arg(ZEND_RECV_INIT, &tmp, &$$, NULL, BYREF_NONE CLS_CC); }
	|	non_empty_parameter_list ',' T_VARIABLE 						{ znode tmp;  fetch_simple_variable(&tmp, &$3, 0 CLS_CC); $$=$1; $$.u.constant.value.lval++; do_receive_arg(ZEND_RECV, &tmp, &$$, NULL, BYREF_NONE CLS_CC); }
	|	non_empty_parameter_list ',' '&' T_VARIABLE					{ znode tmp;  fetch_simple_variable(&tmp, &$4, 0 CLS_CC); $$=$1; $$.u.constant.value.lval++; do_receive_arg(ZEND_RECV, &tmp, &$$, NULL, BYREF_FORCE CLS_CC); }
	|	non_empty_parameter_list ',' T_CONST T_VARIABLE			{ znode tmp;  fetch_simple_variable(&tmp, &$4, 0 CLS_CC); $$=$1; $$.u.constant.value.lval++; do_receive_arg(ZEND_RECV, &tmp, &$$, NULL, BYREF_NONE CLS_CC); }
	|	non_empty_parameter_list ',' T_VARIABLE '=' static_scalar 	{ znode tmp;  fetch_simple_variable(&tmp, &$3, 0 CLS_CC); $$=$1; $$.u.constant.value.lval++; do_receive_arg(ZEND_RECV_INIT, &tmp, &$$, &$5, BYREF_NONE CLS_CC); }
	|	non_empty_parameter_list ',' T_VARIABLE '=' T_UNSET	 	{ znode tmp;  fetch_simple_variable(&tmp, &$3, 0 CLS_CC); $$=$1; $$.u.constant.value.lval++; do_receive_arg(ZEND_RECV_INIT, &tmp, &$$, NULL, BYREF_NONE CLS_CC); }
;


function_call_parameter_list:
		non_empty_function_call_parameter_list		{ $$ = $1; }
	|	/* empty */									{ $$.u.constant.value.lval = 0; }
;


non_empty_function_call_parameter_list:
		expr_without_variable	{	$$.u.constant.value.lval = 1;  do_pass_param(&$1, ZEND_SEND_VAL, $$.u.constant.value.lval CLS_CC); }
	|	cvar					{	$$.u.constant.value.lval = 1;  do_pass_param(&$1, ZEND_SEND_VAR, $$.u.constant.value.lval CLS_CC); }
	|	'&' w_cvar 				{	$$.u.constant.value.lval = 1;  do_pass_param(&$2, ZEND_SEND_REF, $$.u.constant.value.lval CLS_CC); }
	|	non_empty_function_call_parameter_list ',' expr_without_variable	{	$$.u.constant.value.lval=$1.u.constant.value.lval+1;  do_pass_param(&$3, ZEND_SEND_VAL, $$.u.constant.value.lval CLS_CC); }
	|	non_empty_function_call_parameter_list ',' cvar 					{	$$.u.constant.value.lval=$1.u.constant.value.lval+1;  do_pass_param(&$3, ZEND_SEND_VAR, $$.u.constant.value.lval CLS_CC); }
	|	non_empty_function_call_parameter_list ',' '&' w_cvar				{	$$.u.constant.value.lval=$1.u.constant.value.lval+1;  do_pass_param(&$4, ZEND_SEND_REF, $$.u.constant.value.lval CLS_CC); }
;

global_var_list:
		global_var_list ',' global_var { do_fetch_global_or_static_variable(&$3, NULL, ZEND_FETCH_GLOBAL CLS_CC); }
	|	global_var { do_fetch_global_or_static_variable(&$1, NULL, ZEND_FETCH_GLOBAL CLS_CC); }
;


global_var:
		T_VARIABLE			{ $$ = $1; }
	|	'$' r_cvar			{ $$ = $2; }
	|	'$' '{' expr '}'	{ $$ = $3; }
;


static_var_list:
		static_var_list ',' T_VARIABLE { do_fetch_global_or_static_variable(&$3, NULL, ZEND_FETCH_STATIC CLS_CC); }
	|	static_var_list ',' T_VARIABLE '=' static_scalar { do_fetch_global_or_static_variable(&$3, &$5, ZEND_FETCH_STATIC CLS_CC); }
	|	T_VARIABLE  { do_fetch_global_or_static_variable(&$1, NULL, ZEND_FETCH_STATIC CLS_CC); }
	|	T_VARIABLE '=' static_scalar { do_fetch_global_or_static_variable(&$1, &$3, ZEND_FETCH_STATIC CLS_CC); }

;


class_statement_list:
		class_statement_list class_statement
	|	/* empty */
;


class_statement:
		T_VAR class_variable_decleration ';'
	|	T_FUNCTION { $1.u.opline_num = CG(zend_lineno); } T_STRING { do_begin_function_declaration(&$1, &$3, 1 CLS_CC); } '(' 
			parameter_list ')' '{' inner_statement_list '}' { do_end_function_declaration(&$1 CLS_CC); }
	|	T_OLD_FUNCTION { $1.u.opline_num = CG(zend_lineno); } T_STRING { do_begin_function_declaration(&$1, &$3, 1 CLS_CC); }
			parameter_list '(' inner_statement_list ')' ';' { do_end_function_declaration(&$1 CLS_CC); }

;


class_variable_decleration:
		class_variable_decleration ',' T_VARIABLE { do_declare_property(&$3, NULL CLS_CC); }
	|	class_variable_decleration ',' T_VARIABLE '=' static_scalar  { do_declare_property(&$3, &$5 CLS_CC); }
	|	T_VARIABLE	{ do_declare_property(&$1, NULL CLS_CC); }
	|	T_VARIABLE '=' static_scalar { do_declare_property(&$1, &$3 CLS_CC); }
;

	
echo_expr_list:	
		/* empty */
	|	echo_expr_list ',' expr { do_echo(&$3 CLS_CC); }
	|	expr					{ do_echo(&$1 CLS_CC); }
;


for_expr:
		/* empty */			{ $$.op_type = IS_CONST;  $$.u.constant.type = IS_BOOL;  $$.u.constant.value.lval = 1; }
	|	for_expr ',' { do_free(&$1 CLS_CC); } expr	{ $$ = $4; }
	|	expr				{ $$ = $1; }
;


expr_without_variable:	
		T_LIST '(' { do_list_init(CLS_C); } assignment_list ')' '=' expr { do_list_end(&$$, &$7 CLS_CC); }
	|	w_cvar '=' expr		{ do_assign(&$$, &$1, &$3 CLS_CC); }
	|	w_cvar '=' '&' w_cvar	{ do_assign_ref(&$$, &$1, &$4 CLS_CC); }
	|	T_NEW class_name { do_extended_fcall_begin(CLS_C); do_begin_new_object(&$1, &$2 CLS_CC); } ctor_arguments { do_end_new_object(&$$, &$2, &$1, &$4 CLS_CC); do_extended_fcall_end(CLS_C);}
	|	rw_cvar T_PLUS_EQUAL expr 	{ do_binary_assign_op(ZEND_ASSIGN_ADD, &$$, &$1, &$3 CLS_CC); }
	|	rw_cvar T_MINUS_EQUAL expr	{ do_binary_assign_op(ZEND_ASSIGN_SUB, &$$, &$1, &$3 CLS_CC); }
	|	rw_cvar T_MUL_EQUAL expr		{ do_binary_assign_op(ZEND_ASSIGN_MUL, &$$, &$1, &$3 CLS_CC); }
	|	rw_cvar T_DIV_EQUAL expr		{ do_binary_assign_op(ZEND_ASSIGN_DIV, &$$, &$1, &$3 CLS_CC); }
	|	rw_cvar T_CONCAT_EQUAL expr	{ do_binary_assign_op(ZEND_ASSIGN_CONCAT, &$$, &$1, &$3 CLS_CC); }
	|	rw_cvar T_MOD_EQUAL expr		{ do_binary_assign_op(ZEND_ASSIGN_MOD, &$$, &$1, &$3 CLS_CC); }
	|	rw_cvar T_AND_EQUAL expr		{ do_binary_assign_op(ZEND_ASSIGN_BW_AND, &$$, &$1, &$3 CLS_CC); }
	|	rw_cvar T_OR_EQUAL expr 		{ do_binary_assign_op(ZEND_ASSIGN_BW_OR, &$$, &$1, &$3 CLS_CC); }
	|	rw_cvar XT_OR_EQUAL expr 		{ do_binary_assign_op(ZEND_ASSIGN_BW_XOR, &$$, &$1, &$3 CLS_CC); }
	|	rw_cvar T_SL_EQUAL expr	{ do_binary_assign_op(ZEND_ASSIGN_SL, &$$, &$1, &$3 CLS_CC); } 
	|	rw_cvar T_SR_EQUAL expr	{ do_binary_assign_op(ZEND_ASSIGN_SR, &$$, &$1, &$3 CLS_CC); } 
	|	rw_cvar T_INC { do_post_incdec(&$$, &$1, ZEND_POST_INC CLS_CC); }
	|	T_INC rw_cvar { do_pre_incdec(&$$, &$2, ZEND_PRE_INC CLS_CC); }
	|	rw_cvar T_DEC { do_post_incdec(&$$, &$1, ZEND_POST_DEC CLS_CC); }
	|	T_DEC rw_cvar { do_pre_incdec(&$$, &$2, ZEND_PRE_DEC CLS_CC); }
	|	expr T_BOOLEAN_OR { do_boolean_or_begin(&$1, &$2 CLS_CC); } expr { do_boolean_or_end(&$$, &$1, &$4, &$2 CLS_CC); }
	|	expr T_BOOLEAN_AND { do_boolean_and_begin(&$1, &$2 CLS_CC); } expr { do_boolean_and_end(&$$, &$1, &$4, &$2 CLS_CC); }  
	|	expr T_LOGICAL_OR { do_boolean_or_begin(&$1, &$2 CLS_CC); } expr { do_boolean_or_end(&$$, &$1, &$4, &$2 CLS_CC); }
	|	expr T_LOGICAL_AND { do_boolean_and_begin(&$1, &$2 CLS_CC); } expr { do_boolean_and_end(&$$, &$1, &$4, &$2 CLS_CC); }
	|	expr T_LOGICAL_XOR expr { do_binary_op(ZEND_BOOL_XOR, &$$, &$1, &$3 CLS_CC); }
	|	expr '|' expr	{ do_binary_op(ZEND_BW_OR, &$$, &$1, &$3 CLS_CC); }
	|	expr '&' expr	{ do_binary_op(ZEND_BW_AND, &$$, &$1, &$3 CLS_CC); }
	|	expr '^' expr	{ do_binary_op(ZEND_BW_XOR, &$$, &$1, &$3 CLS_CC); }
	|	expr '.' expr 	{ do_binary_op(ZEND_CONCAT,&$$,&$1,&$3 CLS_CC); }
	|	expr '+' expr 	{ do_binary_op(ZEND_ADD,&$$,&$1,&$3 CLS_CC); }
	|	expr '-' expr 	{ do_binary_op(ZEND_SUB,&$$,&$1,&$3 CLS_CC); }
	|	expr '*' expr	{ do_binary_op(ZEND_MUL,&$$,&$1,&$3 CLS_CC); }
	|	expr '/' expr	{ do_binary_op(ZEND_DIV,&$$,&$1,&$3 CLS_CC); }
	|	expr '%' expr 	{ do_binary_op(ZEND_MOD,&$$,&$1,&$3 CLS_CC); }
	| 	expr T_SL expr { do_binary_op(ZEND_SL, &$$, &$1, &$3 CLS_CC); }
	|	expr T_SR expr { do_binary_op(ZEND_SR, &$$, &$1, &$3 CLS_CC); }
	|	'+' expr { $1.u.constant.value.lval=0; $1.u.constant.type=IS_LONG; $1.op_type = IS_CONST; INIT_PZVAL(&$1.u.constant); do_binary_op(ZEND_ADD, &$$, &$1, &$2 CLS_CC); }
	|	'-' expr { $1.u.constant.value.lval=0; $1.u.constant.type=IS_LONG; $1.op_type = IS_CONST; INIT_PZVAL(&$1.u.constant); do_binary_op(ZEND_SUB, &$$, &$1, &$2 CLS_CC); }
	|	'!' expr { do_unary_op(ZEND_BOOL_NOT, &$$, &$2 CLS_CC); }
	|	'~' expr { do_unary_op(ZEND_BW_NOT, &$$, &$2 CLS_CC); }
	|	expr T_IS_EQUAL expr				{ do_binary_op(ZEND_IS_EQUAL, &$$, &$1, &$3 CLS_CC); }
	|	expr T_IS_NOT_EQUAL expr 			{ do_binary_op(ZEND_IS_NOT_EQUAL, &$$, &$1, &$3 CLS_CC); }
	|	expr '<' expr 					{ do_binary_op(ZEND_IS_SMALLER, &$$, &$1, &$3 CLS_CC); }
	|	expr T_IS_SMALLER_OR_EQUAL expr 	{ do_binary_op(ZEND_IS_SMALLER_OR_EQUAL, &$$, &$1, &$3 CLS_CC); }
	|	expr '>' expr 					{ do_binary_op(ZEND_IS_SMALLER, &$$, &$3, &$1 CLS_CC); }
	|	expr T_IS_GREATER_OR_EQUAL expr 	{ do_binary_op(ZEND_IS_SMALLER_OR_EQUAL, &$$, &$3, &$1 CLS_CC); }
	|	'(' expr ')' 	{ $$ = $2; }
	|	expr '?' { do_begin_qm_op(&$1, &$2 CLS_CC); }
		expr ':' { do_qm_true(&$4, &$2, &$5 CLS_CC); }
		expr	 { do_qm_false(&$$, &$7, &$2, &$5 CLS_CC); }
	|	T_STRING	'(' { do_extended_fcall_begin(CLS_C); do_begin_function_call(&$1 CLS_CC); }
				function_call_parameter_list
				')' { do_end_function_call(&$1, &$$, &$4, 0 CLS_CC); do_extended_fcall_end(CLS_C); }
	|	r_cvar '(' { do_extended_fcall_begin(CLS_C); do_begin_dynamic_function_call(&$1 CLS_CC); } 
				function_call_parameter_list 
				')' { do_end_function_call(&$1, &$$, &$4, 0 CLS_CC); do_extended_fcall_end(CLS_C);}
	|	T_STRING T_PAAMAYIM_NEKUDOTAYIM T_STRING '(' { do_extended_fcall_begin(CLS_C); do_begin_class_member_function_call(&$1, &$3 CLS_CC); } 
											function_call_parameter_list 
											')' { do_end_function_call(&$3, &$$, &$6, 1 CLS_CC); do_extended_fcall_end(CLS_C);}
	|	internal_functions_in_yacc { $$ = $1; }
	|	T_INT_CAST expr 		{ do_cast(&$$, &$2, IS_LONG CLS_CC); }
	|	T_DOUBLE_CAST expr 	{ do_cast(&$$, &$2, IS_DOUBLE CLS_CC); }
	|	T_STRING_CAST expr	{ do_cast(&$$, &$2, IS_STRING CLS_CC); } 
	|	T_ARRAY_CAST expr 	{ do_cast(&$$, &$2, IS_ARRAY CLS_CC); }
	|	T_OBJECT_CAST expr 	{ do_cast(&$$, &$2, IS_OBJECT CLS_CC); }
	|	T_BOOL_CAST expr	{ do_cast(&$$, &$2, IS_BOOL CLS_CC); }
	|	T_EXIT exit_expr	{ do_exit(&$$, &$2 CLS_CC); }
	|	'@' { do_begin_silence(&$1 CLS_CC); } expr { do_end_silence(&$1 CLS_CC); $$ = $3; }
	|	scalar				{ $$ = $1; }
	|	T_ARRAY '(' array_pair_list ')' { $$ = $3; }
	|	'`' encaps_list '`'		{ do_shell_exec(&$$, &$2 CLS_CC); }
	|	T_PRINT expr  { do_print(&$$, &$2 CLS_CC); }
;


exit_expr:
		/* empty */		{ $$.op_type = IS_UNUSED; }	
	|	'(' ')'			{ $$.op_type = IS_UNUSED; }	
	|	'(' expr ')'	{ $$ = $2; }
;


ctor_arguments:
		/* empty */									{ $$.u.constant.value.lval=0; }
	|	'(' function_call_parameter_list ')'		{ $$ = $2; }
;


class_name:
		T_STRING	{ $$ = $1; }
	|	r_cvar	{ $$ = $1; }
;



common_scalar:
		T_LNUMBER 					{ $$=$1; }
	|	T_DNUMBER 					{ $$=$1; }
	|	T_CONSTANT_ENCAPSED_STRING	{ $$ = $1; }
	|	T_LINE 					{ $$ = $1; }
	|	T_FILE 					{ $$ = $1; }
;


static_scalar: /* compile-time evaluated scalars */
		common_scalar		{ $$ = $1; }
	|	T_STRING 				{ do_fetch_constant(&$$, &$1, ZEND_CT CLS_CC); }
	|	'+' static_scalar	{ $$ = $1; }
	|	'-' static_scalar	{ zval minus_one;  minus_one.type = IS_LONG; minus_one.value.lval = -1;  mul_function(&$2.u.constant, &$2.u.constant, &minus_one);  $$ = $2; }
	|	T_ARRAY '(' static_array_pair_list ')' { $$ = $3; }
;


scalar:
		T_STRING 					{ do_fetch_constant(&$$, &$1, ZEND_RT CLS_CC); }
	|	T_STRING_VARNAME			{ $$ = $1; }
	|	common_scalar			{ $$ = $1; }
	|	'"' encaps_list '"' 	{ $$ = $2; }
	|	'\'' encaps_list '\''	{ $$ = $2; }
	|	T_HEREDOC encaps_list T_HEREDOC { $$ = $2; do_end_heredoc(CLS_C); }
;


static_array_pair_list:
		/* empty */ 						{ $$.op_type = IS_CONST; INIT_PZVAL(&$$.u.constant); array_init(&$$.u.constant); }
	|	non_empty_static_array_pair_list	{ $$ = $1; }
;


non_empty_static_array_pair_list:
		non_empty_static_array_pair_list ',' static_scalar T_DOUBLE_ARROW static_scalar	{ do_add_static_array_element(&$$, &$3, &$5); }
	|	non_empty_static_array_pair_list ',' static_scalar		{ do_add_static_array_element(&$$, NULL, &$3); }
	|	static_scalar T_DOUBLE_ARROW static_scalar	{ $$.op_type = IS_CONST; INIT_PZVAL(&$$.u.constant); array_init(&$$.u.constant); do_add_static_array_element(&$$, &$1, &$3); }
	|	static_scalar 									{ $$.op_type = IS_CONST; INIT_PZVAL(&$$.u.constant); array_init(&$$.u.constant); do_add_static_array_element(&$$, NULL, &$1); }
;

expr:
		r_cvar					{ $$ = $1; }
	|	expr_without_variable	{ $$ = $1; }
;



r_cvar:
	cvar { do_end_variable_parse(BP_VAR_R CLS_CC); $$ = $1; }
;


w_cvar:
	cvar { do_end_variable_parse(BP_VAR_W CLS_CC); $$ = $1; }
;


rw_cvar:
	cvar { do_end_variable_parse(BP_VAR_RW CLS_CC); $$ = $1; }
;


cvar:
		cvar_without_objects { $$ = $1; }
	|	cvar_without_objects T_OBJECT_OPERATOR { do_push_object(&$1 CLS_CC); } ref_list { $$ = $4; }
;


cvar_without_objects:
		reference_variable { $$ = $1; }
	|	simple_indirect_reference reference_variable { do_indirect_references(&$$, &$1, &$2 CLS_CC); }
;


reference_variable:
		dim_list ']' { $$ = $1; }
	|	compound_variable		{ do_fetch_globals(&$1 CLS_CC); do_begin_variable_parse(CLS_C); fetch_simple_variable(&$$, &$1, 1 CLS_CC); }
;
	

compound_variable:
		T_VARIABLE			{ $$ = $1; }
	|	'$' '{' expr '}'	{ $$ = $3; }
;


dim_list:
		dim_list ']' '[' dim_offset	{ fetch_array_dim(&$$, &$1, &$4 CLS_CC); }
	|	compound_variable  { do_fetch_globals(&$1 CLS_CC); do_begin_variable_parse(CLS_C); } '[' dim_offset		{ fetch_array_begin(&$$, &$1, &$4 CLS_CC); }
;


dim_offset:
		/* empty */		{ $$.op_type = IS_UNUSED; }
	|	expr			{ $$ = $1; }
;


ref_list:
		object_property  { $$ = $1; }
	|	ref_list T_OBJECT_OPERATOR { do_push_object(&$1 CLS_CC); } object_property { $$ = $4; }
;

object_property:
		scalar_object_property		{ znode tmp_znode;  do_pop_object(&tmp_znode CLS_CC);  do_fetch_property(&$$, &tmp_znode, &$1 CLS_CC); }
	|	object_dim_list ']' { $$ = $1; }
;

scalar_object_property:
		T_STRING			{ $$ = $1; }
	|	'{' expr '}'	{ $$ = $2; }
	|	cvar_without_objects { do_end_variable_parse(BP_VAR_R CLS_CC); $$ = $1; }
;


object_dim_list:
		object_dim_list ']' '[' dim_offset { fetch_array_dim(&$$, &$1, &$4 CLS_CC); }
	|	T_STRING { znode tmp_znode, res;  do_pop_object(&tmp_znode CLS_CC);  do_fetch_property(&res, &tmp_znode, &$1 CLS_CC);  $1 = res; } '[' dim_offset { fetch_array_dim(&$$, &$1, &$4 CLS_CC); }
;


simple_indirect_reference:
		'$' { $$.u.constant.value.lval = 1; }
	|	simple_indirect_reference '$' { $$.u.constant.value.lval++; }
;

assignment_list:
		assignment_list ',' assignment_list_element
	|	assignment_list_element
;


assignment_list_element:
		w_cvar								{ do_add_list_element(&$1 CLS_CC); }
	|	T_LIST '(' { do_new_list_begin(CLS_C); } assignment_list ')'	{ do_new_list_end(CLS_C); }
	|	/* empty */							{ do_add_list_element(NULL CLS_CC); }
;


array_pair_list:
		/* empty */ 				{ do_init_array(&$$, NULL, NULL CLS_CC); }
	|	non_empty_array_pair_list	{ $$ = $1; }
;

non_empty_array_pair_list:
		non_empty_array_pair_list ',' expr T_DOUBLE_ARROW expr	{ do_add_array_element(&$$, &$5, &$3 CLS_CC); }
	|	non_empty_array_pair_list ',' expr		{ do_add_array_element(&$$, &$3, NULL CLS_CC); }
	|	expr T_DOUBLE_ARROW expr	{ do_init_array(&$$, &$3, &$1 CLS_CC); }
	|	expr 						{ do_init_array(&$$, &$1, NULL CLS_CC); }
;


encaps_list:
		encaps_list encaps_var { do_end_variable_parse(BP_VAR_R CLS_CC);  do_add_variable(&$$, &$1, &$2 CLS_CC); }
	|	encaps_list T_STRING						{ do_add_string(&$$, &$1, &$2 CLS_CC); }
	|	encaps_list T_NUM_STRING					{ do_add_string(&$$, &$1, &$2 CLS_CC); }
	|	encaps_list T_ENCAPSED_AND_WHITESPACE		{ do_add_string(&$$, &$1, &$2 CLS_CC); }
	|	encaps_list T_CHARACTER 					{ do_add_char(&$$, &$1, &$2 CLS_CC); }
	|	encaps_list T_BAD_CHARACTER				{ do_add_string(&$$, &$1, &$2 CLS_CC); }
	|	encaps_list '['		{ $2.u.constant.value.chval = '['; do_add_char(&$$, &$1, &$2 CLS_CC); }
	|	encaps_list ']'		{ $2.u.constant.value.chval = ']'; do_add_char(&$$, &$1, &$2 CLS_CC); }
	|	encaps_list '{'		{ $2.u.constant.value.chval = '{'; do_add_char(&$$, &$1, &$2 CLS_CC); }
	|	encaps_list '}'		{ $2.u.constant.value.chval = '}'; do_add_char(&$$, &$1, &$2 CLS_CC); }
	|	encaps_list T_OBJECT_OPERATOR  { znode tmp;  $2.u.constant.value.chval = '-';  do_add_char(&tmp, &$1, &$2 CLS_CC);  $2.u.constant.value.chval = '>'; do_add_char(&$$, &tmp, &$2 CLS_CC); }
	|	/* empty */			{ do_init_string(&$$ CLS_CC); }

;



encaps_var:
		T_VARIABLE { do_fetch_globals(&$1 CLS_CC); do_begin_variable_parse(CLS_C); fetch_simple_variable(&$$, &$1, 1 CLS_CC); }
	|	T_VARIABLE '[' { do_begin_variable_parse(CLS_C); } encaps_var_offset ']'	{ do_fetch_globals(&$1 CLS_CC);  fetch_array_begin(&$$, &$1, &$4 CLS_CC); }
	|	T_VARIABLE T_OBJECT_OPERATOR T_STRING { do_begin_variable_parse(CLS_C); fetch_simple_variable(&$2, &$1, 1 CLS_CC); do_fetch_property(&$$, &$2, &$3 CLS_CC); }
	|	T_DOLLAR_OPEN_CURLY_BRACES expr '}' { do_begin_variable_parse(CLS_C);  fetch_simple_variable(&$$, &$2, 1 CLS_CC); }
	|	T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '[' expr ']' '}' { do_begin_variable_parse(CLS_C);  fetch_array_begin(&$$, &$2, &$4 CLS_CC); }
	|	T_CURLY_OPEN cvar '}' { $$ = $2; }
;


encaps_var_offset:
		T_STRING	{ $$ = $1; }
	|	T_NUM_STRING	{ $$ = $1; }
	|	T_VARIABLE { fetch_simple_variable(&$$, &$1, 1 CLS_CC); }
;


internal_functions_in_yacc:
		T_ISSET '(' cvar ')'	{ do_isset_or_isempty(ZEND_ISSET, &$$, &$3 CLS_CC); }
	|	T_EMPTY '(' cvar ')'	{ do_isset_or_isempty(ZEND_ISEMPTY, &$$, &$3 CLS_CC); }
	|	T_INCLUDE expr 		{ do_include_or_eval(ZEND_INCLUDE, &$$, &$2 CLS_CC); }
	|	T_EVAL '(' expr ')' 	{ do_include_or_eval(ZEND_EVAL, &$$, &$3 CLS_CC); }
;


%%

