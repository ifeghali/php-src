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


#ifndef _ZEND_STACK_H
#define _ZEND_STACK_H

typedef struct _zend_stack {
	int top, max;
	void **elements;
} zend_stack;


#define STACK_BLOCK_SIZE 64

ZEND_API int zend_stack_init(zend_stack *stack);
ZEND_API int zend_stack_push(zend_stack *stack, void *element, int size);
ZEND_API int zend_stack_top(zend_stack *stack, void **element);
ZEND_API int zend_stack_del_top(zend_stack *stack);
ZEND_API int zend_stack_int_top(zend_stack *stack);
ZEND_API int zend_stack_is_empty(zend_stack *stack);
ZEND_API int zend_stack_destroy(zend_stack *stack);
ZEND_API void **zend_stack_base(zend_stack *stack);
ZEND_API int zend_stack_count(zend_stack *stack);
ZEND_API void zend_stack_apply(zend_stack *stack, void (*apply_function)(void *element), int type);
ZEND_API void zend_stack_apply_with_argument(zend_stack *stack, void (*apply_function)(void *element, void *arg), int type, void *arg);

#define ZEND_STACK_APPLY_TOPDOWN	1
#define ZEND_STACK_APPLY_BOTTOMUP	2

#endif /* _ZEND_STACK_H */
