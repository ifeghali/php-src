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


#ifndef _T_GLOBALS_H
#define _T_GLOBALS_H


#include <setjmp.h>

#include "zend_globals_macros.h"

#include "zend_stack.h"
#include "zend_ptr_stack.h"
#include "zend_hash.h"
#include "zend_llist.h"

/* Define ZTS if you want a thread-safe Zend */
/*#undef ZTS*/

#ifdef ZTS
#include "../TSRM/TSRM.h"

#ifdef __cplusplus
class ZendFlexLexer;
#endif

BEGIN_EXTERN_C()
ZEND_API extern int compiler_globals_id;
ZEND_API extern int executor_globals_id;
extern int alloc_globals_id;
END_EXTERN_C()

#endif

#define SYMTABLE_CACHE_SIZE 32


#include "zend_compile.h"
#include "zend_execute.h"

/* excpt.h on Digital Unix 4.0 defines function_table */
#undef function_table
	
struct _zend_compiler_globals {
	zend_stack bp_stack;
	zend_stack switch_cond_stack;
	zend_stack foreach_copy_stack;
	zend_stack object_stack;

	zend_class_entry class_entry, *active_class_entry;

	/* variables for list() compilation */
	zend_llist list_llist;
	zend_llist dimension_llist;

	zend_stack function_call_stack;

	char *compiled_filename;

	int zend_lineno;
	int comment_start_line;
	char *heredoc;
	int heredoc_len;

	zend_op_array *active_op_array;

	HashTable *function_table;	/* function symbol table */
	HashTable *class_table;		/* class table */

	HashTable used_files;		/* files already included using 'use' */

	zend_llist filenames_list;

	zend_bool short_tags;
	zend_bool asp_tags;
	zend_bool allow_call_time_pass_reference;

	/* For extensions support */
	zend_bool extended_info;	/* generate extension information for debugger/profiler */
	zend_bool handle_op_arrays;	/* run op_arrays through op_array handlers */

	zend_bool unclean_shutdown;

	zend_llist open_files;
#ifdef ZTS
#ifdef __cplusplus
	ZendFlexLexer *ZFL;
#else
	void *ZFL;
#endif
#endif
};


struct _zend_executor_globals {
	zval *return_value;
	zval **return_value_ptr_ptr;

	zval uninitialized_zval;
	zval *uninitialized_zval_ptr;

	zval error_zval;
	zval *error_zval_ptr;

	zend_function_state *function_state_ptr;
	zend_ptr_stack arg_types_stack;
	zend_stack overloaded_objects_stack;
	zval global_return_value;

	/* symbol table cache */
	HashTable *symtable_cache[SYMTABLE_CACHE_SIZE];
	HashTable **symtable_cache_limit;
	HashTable **symtable_cache_ptr;

	zend_op **opline_ptr;

	HashTable *active_symbol_table;
	HashTable symbol_table;		/* main symbol table */

	HashTable imported_files;	/* files already included using 'import' */

	jmp_buf bailout;

	int error_reporting;

	zend_op_array *active_op_array;
	zend_op_array *main_op_array;

	HashTable *function_table;	/* function symbol table */
	HashTable *class_table;		/* class table */
	HashTable *zend_constants;	/* constants table */

	long precision;

	/* for extended information support */
	zend_bool no_extensions;

	HashTable regular_list;
	HashTable persistent_list;

	zend_ptr_stack argument_stack;
	int free_op1, free_op2;
	int (*unary_op)(zval *result, zval *op1);
	int (*binary_op)(zval *result, zval *op1, zval *op2);

	zval *garbage[4];
	int garbage_ptr;
	zend_bool suspend_garbage;

	void *reserved[ZEND_MAX_RESERVED_RESOURCES];
#if SUPPORT_INTERACTIVE
	int interactive;
#endif
};



struct _zend_alloc_globals {
	mem_header *head;		/* standard list */
	mem_header *phead;		/* persistent list */
	void *cache[MAX_CACHED_MEMORY][MAX_CACHED_ENTRIES];
	unsigned char cache_count[MAX_CACHED_MEMORY];

#if MEMORY_LIMIT
	unsigned int memory_limit;
	unsigned int allocated_memory;
	unsigned char memory_exhausted;
#endif
};


#endif /* _T_GLOBALS_H */
