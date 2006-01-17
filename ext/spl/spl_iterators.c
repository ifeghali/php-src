/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2006 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"

#include "php_spl.h"
#include "spl_functions.h"
#include "spl_engine.h"
#include "spl_iterators.h"
#include "spl_directory.h"
#include "spl_array.h"
#include "spl_exceptions.h"

PHPAPI zend_class_entry *spl_ce_RecursiveIterator;
PHPAPI zend_class_entry *spl_ce_RecursiveIteratorIterator;
PHPAPI zend_class_entry *spl_ce_FilterIterator;
PHPAPI zend_class_entry *spl_ce_RecursiveFilterIterator;
PHPAPI zend_class_entry *spl_ce_ParentIterator;
PHPAPI zend_class_entry *spl_ce_SeekableIterator;
PHPAPI zend_class_entry *spl_ce_LimitIterator;
PHPAPI zend_class_entry *spl_ce_CachingIterator;
PHPAPI zend_class_entry *spl_ce_RecursiveCachingIterator;
PHPAPI zend_class_entry *spl_ce_OuterIterator;
PHPAPI zend_class_entry *spl_ce_IteratorIterator;
PHPAPI zend_class_entry *spl_ce_NoRewindIterator;
PHPAPI zend_class_entry *spl_ce_InfiniteIterator;
PHPAPI zend_class_entry *spl_ce_EmptyIterator;
PHPAPI zend_class_entry *spl_ce_AppendIterator;
PHPAPI zend_class_entry *spl_ce_RegExIterator;
PHPAPI zend_class_entry *spl_ce_RecursiveRegExIterator;

zend_function_entry spl_funcs_RecursiveIterator[] = {
	SPL_ABSTRACT_ME(RecursiveIterator, hasChildren,  NULL)
	SPL_ABSTRACT_ME(RecursiveIterator, getChildren,  NULL)
	{NULL, NULL, NULL}
};

typedef enum {
	RIT_LEAVES_ONLY = 0,
	RIT_SELF_FIRST  = 1,
	RIT_CHILD_FIRST = 2
} RecursiveIteratorMode;

#define RIT_CATCH_GET_CHILD CIT_CATCH_GET_CHILD

typedef enum {
	RS_NEXT  = 0,
	RS_TEST  = 1,
	RS_SELF  = 2,
	RS_CHILD = 3,
	RS_START = 4
} RecursiveIteratorState;

typedef struct _spl_sub_iterator {
	zend_object_iterator    *iterator;
	zval                    *zobject;
	zend_class_entry        *ce;
	RecursiveIteratorState  state;
} spl_sub_iterator;

typedef struct _spl_recursive_it_object {
	zend_object              std;
	spl_sub_iterator         *iterators;
	int                      level;
	RecursiveIteratorMode    mode;
	int                      flags;
	int                      max_depth;
	zend_bool                in_iteration;
	zend_function            *beginIteration;
	zend_function            *endIteration;
	zend_function            *callHasChildren;
	zend_function            *callGetChildren;
	zend_function            *beginChildren;
	zend_function            *endChildren;
	zend_function            *nextElement;
	zend_class_entry         *ce;
} spl_recursive_it_object;

typedef struct _spl_recursive_it_iterator {
	zend_object_iterator   intern;
	zval                   *zobject;
} spl_recursive_it_iterator;

static zend_object_handlers spl_handlers_rec_it_it;
static zend_object_handlers spl_handlers_dual_it;

static void spl_recursive_it_dtor(zend_object_iterator *_iter TSRMLS_DC)
{
	spl_recursive_it_iterator *iter   = (spl_recursive_it_iterator*)_iter;
	spl_recursive_it_object   *object = (spl_recursive_it_object*)_iter->data;
	zend_object_iterator      *sub_iter;

	while (object->level) {
		sub_iter = object->iterators[object->level].iterator;
		sub_iter->funcs->dtor(sub_iter TSRMLS_CC);
		zval_ptr_dtor(&object->iterators[object->level--].zobject);
	}
	object->iterators = erealloc(object->iterators, sizeof(spl_sub_iterator));
	object->level = 0;

	zval_ptr_dtor(&iter->zobject);	
	efree(iter);
}
	
static int spl_recursive_it_valid_ex(spl_recursive_it_object *object, zval *zthis TSRMLS_DC)
{
	zend_object_iterator      *sub_iter;
	int                       level = object->level;
	
	while (level >=0) {
		sub_iter = object->iterators[level].iterator;
		if (sub_iter->funcs->valid(sub_iter TSRMLS_CC) == SUCCESS) {
			return SUCCESS;
		}
		level--;
	}
	if (object->endIteration && object->in_iteration) {
		zend_call_method_with_0_params(&zthis, object->ce, &object->endIteration, "endIteration", NULL);
	}
	object->in_iteration = 0;
	return FAILURE;
}

static int spl_recursive_it_valid(zend_object_iterator *iter TSRMLS_DC)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)iter->data;
	
	return spl_recursive_it_valid_ex(object, ((spl_recursive_it_iterator*)iter)->zobject TSRMLS_CC);
}

static void spl_recursive_it_get_current_data(zend_object_iterator *iter, zval ***data TSRMLS_DC)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)iter->data;
	zend_object_iterator      *sub_iter = object->iterators[object->level].iterator;
	
	sub_iter->funcs->get_current_data(sub_iter, data TSRMLS_CC);
}

static int spl_recursive_it_get_current_key(zend_object_iterator *iter, char **str_key, uint *str_key_len, ulong *int_key TSRMLS_DC)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)iter->data;
	zend_object_iterator      *sub_iter = object->iterators[object->level].iterator;

	if (sub_iter->funcs->get_current_key) {
		return sub_iter->funcs->get_current_key(sub_iter, str_key, str_key_len, int_key TSRMLS_CC);
	} else {
		*int_key = iter->index;
		return HASH_KEY_IS_LONG;
	}
}

static void spl_recursive_it_move_forward_ex(spl_recursive_it_object *object, zval *zthis TSRMLS_DC)
{
	zend_object_iterator      *iterator;
	zval                      *zobject;
	zend_class_entry          *ce;
	zval                      *retval, *child;
	zend_object_iterator      *sub_iter;
	int                       has_children;

	while (!EG(exception)) {
next_step:
		iterator = object->iterators[object->level].iterator;
		switch (object->iterators[object->level].state) {
			case RS_NEXT:
				iterator->funcs->move_forward(iterator TSRMLS_CC);
			case RS_START:
				if (iterator->funcs->valid(iterator TSRMLS_CC) == FAILURE) {
					break;
				}
				object->iterators[object->level].state = RS_TEST;					
				/* break; */
			case RS_TEST:
				ce = object->iterators[object->level].ce;
				zobject = object->iterators[object->level].zobject;
				if (object->callHasChildren) {
					zend_call_method_with_0_params(&zthis, object->ce, &object->callHasChildren, "callHasChildren", &retval);
				} else {
					zend_call_method_with_0_params(&zobject, ce, NULL, "haschildren", &retval);
				}
				if (retval) {
					has_children = zend_is_true(retval);
					zval_ptr_dtor(&retval);
					if (has_children) {
						if (object->max_depth == -1 || object->max_depth > object->level) {
							switch (object->mode) {
							case RIT_LEAVES_ONLY:
							case RIT_CHILD_FIRST:
								object->iterators[object->level].state = RS_CHILD;
								goto next_step;
							case RIT_SELF_FIRST:
								object->iterators[object->level].state = RS_SELF;
								goto next_step;
							}
						} else {
							/* do not recurse into */
							if (object->mode == RIT_LEAVES_ONLY) {
								/* this is not a leave, so skip it */
								object->iterators[object->level].state = RS_NEXT;
								goto next_step;
							}
						}
					}
				}
				if (object->nextElement) {
					zend_call_method_with_0_params(&zthis, object->ce, &object->nextElement, "nextelement", NULL);
				}
				object->iterators[object->level].state = RS_NEXT;
				return /* self */;
			case RS_SELF:
				if (object->nextElement && (object->mode == RIT_SELF_FIRST || object->mode == RIT_CHILD_FIRST)) {
					zend_call_method_with_0_params(&zthis, object->ce, &object->nextElement, "nextelement", NULL);
				}
				if (object->mode == RIT_SELF_FIRST) {
					object->iterators[object->level].state = RS_CHILD;
				} else {
					object->iterators[object->level].state = RS_NEXT;
				}
				return /* self */;
			case RS_CHILD:
				ce = object->iterators[object->level].ce;
				zobject = object->iterators[object->level].zobject;
				if (object->callGetChildren) {
					zend_call_method_with_0_params(&zthis, object->ce, &object->callGetChildren, "callGetChildren", &child);
				} else {
					zend_call_method_with_0_params(&zobject, ce, NULL, "getchildren", &child);
				}

				if (EG(exception)) {
					if (!(object->flags & RIT_CATCH_GET_CHILD)) {
						return;
					} else {
						zend_clear_exception(TSRMLS_C);
						if (child) {
							zval_ptr_dtor(&child);
						}
						object->iterators[object->level].state = RS_NEXT;
						goto next_step;
					}
				}

				ce = child && Z_TYPE_P(child) == IS_OBJECT ? Z_OBJCE_P(child) : NULL;
				if (!ce || !instanceof_function(ce, U_CLASS_ENTRY(spl_ce_RecursiveIterator) TSRMLS_CC)) {
					if (child) {
						zval_ptr_dtor(&child);
					}
					zend_throw_exception(U_CLASS_ENTRY(spl_ce_UnexpectedValueException), "Objects returned by RecursiveIterator::getChildren() must implement RecursiveIterator", 0 TSRMLS_CC);
					return;
				}
				if (object->mode == RIT_CHILD_FIRST) {
					object->iterators[object->level].state = RS_SELF;
				} else {
					object->iterators[object->level].state = RS_NEXT;
				}
				object->iterators = erealloc(object->iterators, sizeof(spl_sub_iterator) * (++object->level+1));
				sub_iter = ce->get_iterator(ce, child TSRMLS_CC);
				object->iterators[object->level].iterator = sub_iter;
				object->iterators[object->level].zobject = child;
				object->iterators[object->level].ce = ce;
				object->iterators[object->level].state = RS_START;
				if (sub_iter->funcs->rewind) {
					sub_iter->funcs->rewind(sub_iter TSRMLS_CC);
				}
				if (object->beginChildren) {
					zend_call_method_with_0_params(&zthis, object->ce, &object->beginChildren, "beginchildren", NULL);
				}
				goto next_step;
		}
		/* no more elements */
		if (object->level > 0) {
			if (object->endChildren) {
				zend_call_method_with_0_params(&zthis, object->ce, &object->endChildren, "endchildren", NULL);
			}
			iterator->funcs->dtor(iterator TSRMLS_CC);
			zval_ptr_dtor(&object->iterators[object->level].zobject);
			object->level--;
		} else {
			return; /* done completeley */
		}
	}
}

static void spl_recursive_it_rewind_ex(spl_recursive_it_object *object, zval *zthis TSRMLS_DC)
{
	zend_object_iterator      *sub_iter;

	while (object->level) {
		sub_iter = object->iterators[object->level].iterator;
		sub_iter->funcs->dtor(sub_iter TSRMLS_CC);
		zval_ptr_dtor(&object->iterators[object->level--].zobject);
		if (!object->endChildren || object->endChildren->common.scope != U_CLASS_ENTRY(spl_ce_RecursiveIteratorIterator)) {
			zend_call_method_with_0_params(&zthis, object->ce, &object->endChildren, "endchildren", NULL);
		}
	}
	object->iterators = erealloc(object->iterators, sizeof(spl_sub_iterator));
	object->iterators[0].state = RS_START;
	sub_iter = object->iterators[0].iterator;
	if (sub_iter->funcs->rewind) {
		sub_iter->funcs->rewind(sub_iter TSRMLS_CC);
	}
	if (object->beginIteration && !object->in_iteration) {
		zend_call_method_with_0_params(&zthis, object->ce, &object->beginIteration, "beginIteration", NULL);
	}
	object->in_iteration = 1;
	spl_recursive_it_move_forward_ex(object, zthis TSRMLS_CC);
}

static void spl_recursive_it_move_forward(zend_object_iterator *iter TSRMLS_DC)
{
	spl_recursive_it_move_forward_ex((spl_recursive_it_object*)iter->data, ((spl_recursive_it_iterator*)iter)->zobject TSRMLS_CC);
}

static void spl_recursive_it_rewind(zend_object_iterator *iter TSRMLS_DC)
{
	spl_recursive_it_rewind_ex((spl_recursive_it_object*)iter->data, ((spl_recursive_it_iterator*)iter)->zobject TSRMLS_CC);
}

static zend_object_iterator *spl_recursive_it_get_iterator(zend_class_entry *ce, zval *zobject TSRMLS_DC)
{
	spl_recursive_it_iterator *iterator = emalloc(sizeof(spl_recursive_it_iterator));
	spl_recursive_it_object   *object   = (spl_recursive_it_object*)zend_object_store_get_object(zobject TSRMLS_CC);

	zobject->refcount++;
	iterator->intern.data = (void*)object;
	iterator->intern.funcs = ce->iterator_funcs.funcs;
	iterator->zobject = zobject;
	return (zend_object_iterator*)iterator;
}

zend_object_iterator_funcs spl_recursive_it_iterator_funcs = {
	spl_recursive_it_dtor,
	spl_recursive_it_valid,
	spl_recursive_it_get_current_data,
	spl_recursive_it_get_current_key,
	spl_recursive_it_move_forward,
	spl_recursive_it_rewind
};

/* {{{ proto RecursiveIteratorIterator::__construct(RecursiveIterator|IteratorAggregate it [, int mode = RIT_LEAVES_ONLY [, int flags = 0]]) throws InvalidArgumentException
   Creates a RecursiveIteratorIterator from a RecursiveIterator. */
SPL_METHOD(RecursiveIteratorIterator, __construct)
{
	zval                      *object = getThis();
	spl_recursive_it_object   *intern;
	zval                      *iterator;
	zend_class_entry          *ce_iterator;
	long                       mode = RIT_LEAVES_ONLY, flags = 0;
	int                        inc_refcount = 1;

	php_set_error_handling(EH_THROW, U_CLASS_ENTRY(spl_ce_InvalidArgumentException) TSRMLS_CC);

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "o|ll", &iterator, &mode, &flags) == SUCCESS) {
		if (instanceof_function(Z_OBJCE_P(iterator), U_CLASS_ENTRY(zend_ce_aggregate) TSRMLS_CC)) {
			zval *aggregate = iterator;
			zend_call_method_with_0_params(&aggregate, Z_OBJCE_P(aggregate), &Z_OBJCE_P(aggregate)->iterator_funcs.zf_new_iterator, "getiterator", &iterator);
			inc_refcount = 0;
		}
	} else {
		iterator = NULL;
	}
	if (!iterator || !instanceof_function(Z_OBJCE_P(iterator), U_CLASS_ENTRY(spl_ce_RecursiveIterator) TSRMLS_CC)) {
		if (iterator && !inc_refcount) {
			zval_ptr_dtor(&iterator);
		}
		php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
		zend_throw_exception(U_CLASS_ENTRY(spl_ce_InvalidArgumentException), "An instance of RecursiveIterator or IteratorAggregate creating it is required", 0 TSRMLS_CC);
		return;
	}

	intern = (spl_recursive_it_object*)zend_object_store_get_object(object TSRMLS_CC);
	intern->iterators = emalloc(sizeof(spl_sub_iterator));
	intern->level = 0;
	intern->mode = mode;
	intern->flags = flags;
	intern->max_depth = -1;
	intern->in_iteration = 0;
	intern->ce = Z_OBJCE_P(object);

	zend_hash_find(&intern->ce->function_table, "beginiteration", sizeof("beginiteration"), (void **) &intern->beginIteration);
	if (intern->beginIteration->common.scope == U_CLASS_ENTRY(spl_ce_RecursiveIteratorIterator)) {
		intern->beginIteration = NULL;
	}
	zend_hash_find(&intern->ce->function_table, "enditeration", sizeof("enditeration"), (void **) &intern->endIteration);
	if (intern->endIteration->common.scope == U_CLASS_ENTRY(spl_ce_RecursiveIteratorIterator)) {
		intern->endIteration = NULL;
	}
	zend_hash_find(&intern->ce->function_table, "callhaschildren", sizeof("callHasChildren"), (void **) &intern->callHasChildren);
	if (intern->callHasChildren->common.scope == U_CLASS_ENTRY(spl_ce_RecursiveIteratorIterator)) {
		intern->callHasChildren = NULL;
	}
	zend_hash_find(&intern->ce->function_table, "callgetchildren", sizeof("callGetChildren"), (void **) &intern->callGetChildren);
	if (intern->callGetChildren->common.scope == U_CLASS_ENTRY(spl_ce_RecursiveIteratorIterator)) {
		intern->callGetChildren = NULL;
	}
	zend_hash_find(&intern->ce->function_table, "beginchildren", sizeof("beginchildren"), (void **) &intern->beginChildren);
	if (intern->beginChildren->common.scope == U_CLASS_ENTRY(spl_ce_RecursiveIteratorIterator)) {
		intern->beginChildren = NULL;
	}
	zend_hash_find(&intern->ce->function_table, "endchildren", sizeof("endchildren"), (void **) &intern->endChildren);
	if (intern->endChildren->common.scope == U_CLASS_ENTRY(spl_ce_RecursiveIteratorIterator)) {
		intern->endChildren = NULL;
	}
	zend_hash_find(&intern->ce->function_table, "nextelement", sizeof("nextElement"), (void **) &intern->nextElement);
	if (intern->nextElement->common.scope == U_CLASS_ENTRY(spl_ce_RecursiveIteratorIterator)) {
		intern->nextElement = NULL;
	}
	ce_iterator = Z_OBJCE_P(iterator); /* respect inheritance, don't use spl_ce_RecursiveIterator */
	intern->iterators[0].iterator = ce_iterator->get_iterator(ce_iterator, iterator TSRMLS_CC);
	if (inc_refcount) {
		iterator->refcount++;
	}
	intern->iterators[0].zobject = iterator;
	intern->iterators[0].ce = ce_iterator;
	intern->iterators[0].state = RS_START;

	php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
} /* }}} */

/* {{{ proto void RecursiveIteratorIterator::rewind()
   Rewind the iterator to the first element of the top level inner iterator. */
SPL_METHOD(RecursiveIteratorIterator, rewind)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	spl_recursive_it_rewind_ex(object, getThis() TSRMLS_CC);
} /* }}} */

/* {{{ proto bolean RecursiveIteratorIterator::valid()
   Check whether the current position is valid */
SPL_METHOD(RecursiveIteratorIterator, valid)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_BOOL(spl_recursive_it_valid_ex(object, getThis() TSRMLS_CC) == SUCCESS);
} /* }}} */

/* {{{ proto mixed RecursiveIteratorIterator::key()
   Access the current key */
SPL_METHOD(RecursiveIteratorIterator, key)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	zend_object_iterator      *iterator = object->iterators[object->level].iterator;

	if (iterator->funcs->get_current_key) {
		char *str_key;
		uint str_key_len;
		ulong int_key;
		
		switch (iterator->funcs->get_current_key(iterator, &str_key, &str_key_len, &int_key TSRMLS_CC)) {
			case HASH_KEY_IS_LONG:
				RETURN_LONG(int_key);
				break;
			case HASH_KEY_IS_STRING:
				RETURN_STRINGL(str_key, str_key_len-1, 0);
				break;
			case HASH_KEY_IS_UNICODE:
				RETURN_UNICODEL((void*)str_key, str_key_len-1, 0);
				break;
			default:
				RETURN_NULL();
		}
	} else {
		RETURN_NULL();
	}
} /* }}} */

/* {{{ proto mixed RecursiveIteratorIterator::current()
   Access the current element value */
SPL_METHOD(RecursiveIteratorIterator, current)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	zend_object_iterator      *iterator = object->iterators[object->level].iterator;
	zval                      **data;

	iterator->funcs->get_current_data(iterator, &data TSRMLS_CC);
	RETURN_ZVAL(*data, 1, 0);
} /* }}} */

/* {{{ proto void RecursiveIteratorIterator::next()
   Move forward to the next element */
SPL_METHOD(RecursiveIteratorIterator, next)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	spl_recursive_it_move_forward_ex(object, getThis() TSRMLS_CC);
} /* }}} */

/* {{{ proto int RecursiveIteratorIterator::getDepth()
   Get the current depth of the recursive iteration */
SPL_METHOD(RecursiveIteratorIterator, getDepth)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	RETURN_LONG(object->level);
} /* }}} */

/* {{{ proto RecursiveIterator RecursiveIteratorIterator::getSubIterator([int level])
   The current active sub iterator or the iterator at specified level */
SPL_METHOD(RecursiveIteratorIterator, getSubIterator)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	long  level = object->level;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &level) == FAILURE) {
		return;
	}
	if (level < 0 || level > object->level) {
		RETURN_NULL();
	}
	RETURN_ZVAL(object->iterators[level].zobject, 1, 0);
} /* }}} */

/* {{{ proto RecursiveIterator RecursiveIteratorIterator::getInnerIterator()
   The current active sub iterator */
SPL_METHOD(RecursiveIteratorIterator, getInnerIterator)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	long  level = object->level;
	
	RETURN_ZVAL(object->iterators[level].zobject, 1, 0);
} /* }}} */

/* {{{ proto RecursiveIterator RecursiveIteratorIterator::beginIteration()
   Called when iteration begins (after first rewind() call) */
SPL_METHOD(RecursiveIteratorIterator, beginIteration)
{
	/* nothing to do */
} /* }}} */

/* {{{ proto RecursiveIterator RecursiveIteratorIterator::endIteration()
   Called when iteration ends (when valid() first returns false */
SPL_METHOD(RecursiveIteratorIterator, endIteration)
{
	/* nothing to do */
} /* }}} */

/* {{{ proto bool RecursiveIteratorIterator::callHasChildren()
   Called for each element to test whether it has children */
SPL_METHOD(RecursiveIteratorIterator, callHasChildren)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	zend_class_entry *ce = object->iterators[object->level].ce;
	zval *retval, *zobject;

	zobject = object->iterators[object->level].zobject;
	if (!zobject) {
		RETURN_FALSE;
	} else {
		zend_call_method_with_0_params(&zobject, ce, NULL, "haschildren", &retval);
		RETURN_ZVAL(retval, 0, 1);
	}
} /* }}} */

/* {{{ proto RecursiveIterator RecursiveIteratorIterator::callGetChildren()
   Return children of current element */
SPL_METHOD(RecursiveIteratorIterator, callGetChildren)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	zend_class_entry *ce = object->iterators[object->level].ce;
	zval *retval, *zobject;

	zobject = object->iterators[object->level].zobject;
	if (!zobject) {
		return;
	} else {
		zend_call_method_with_0_params(&zobject, ce, NULL, "getchildren", &retval);
		if (retval) {
			RETURN_ZVAL(retval, 0, 1);
		}
	}
} /* }}} */

/* {{{ proto RecursiveIterator RecursiveIteratorIterator::beginChildren()
   Called when recursing one level down */
SPL_METHOD(RecursiveIteratorIterator, beginChildren)
{
	/* nothing to do */
} /* }}} */

/* {{{ proto RecursiveIterator RecursiveIteratorIterator::endChildren()
   Called when end recursing one level */
SPL_METHOD(RecursiveIteratorIterator, endChildren)
{
	/* nothing to do */
} /* }}} */

/* {{{ proto RecursiveIterator RecursiveIteratorIterator::nextElement()
   Called when the next element is available */
SPL_METHOD(RecursiveIteratorIterator, nextElement)
{
	/* nothing to do */
} /* }}} */

/* {{{ proto RecursiveIterator RecursiveIteratorIterator::setMaxDepth([$max_depth = -1])
   Set the maximum allowed depth (or any depth if pmax_depth = -1] */
SPL_METHOD(RecursiveIteratorIterator, setMaxDepth)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	long  max_depth = -1;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &max_depth) == FAILURE) {
		return;
	}
	if (max_depth < -1) {
		zend_throw_exception(U_CLASS_ENTRY(spl_ce_OutOfRangeException), "Parameter max_depth must be >= -1", 0 TSRMLS_CC);
		return;
	}
	object->max_depth = max_depth;
} /* }}} */

/* {{{ proto RecursiveIterator RecursiveIteratorIterator::getMaxDepth()
   Return the maximum accepted depth or false if any depth is allowed */
SPL_METHOD(RecursiveIteratorIterator, getMaxDepth)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (object->max_depth == -1) {
		RETURN_FALSE;
	} else {
		RETURN_LONG(object->max_depth);
	}
} /* }}} */

static union _zend_function *spl_recursive_it_get_method(zval **object_ptr, char *method, int method_len TSRMLS_DC)
{
	union _zend_function    *function_handler;
	spl_recursive_it_object *object = (spl_recursive_it_object*)zend_object_store_get_object(*object_ptr TSRMLS_CC);
	long                     level = object->level;
	zval                    *zobj = object->iterators[level].zobject;
	
	function_handler = std_object_handlers.get_method(object_ptr, method, method_len TSRMLS_CC);
	if (!function_handler) {
		if (zend_hash_find(&Z_OBJCE_P(zobj)->function_table, method, method_len+1, (void **) &function_handler) == FAILURE) {
			if (Z_OBJ_HT_P(zobj)->get_method) {
				*object_ptr = zobj;
				function_handler = Z_OBJ_HT_P(*object_ptr)->get_method(object_ptr, method, method_len TSRMLS_CC);
			}
		}
	}
	return function_handler;
}

/* {{{ spl_RecursiveIteratorIterator_dtor */
static void spl_RecursiveIteratorIterator_free_storage(void *_object TSRMLS_DC)
{
	spl_recursive_it_object   *object = (spl_recursive_it_object *)_object;
	zend_object_iterator      *sub_iter;

	if (object->iterators) {
		while (object->level >= 0) {
			sub_iter = object->iterators[object->level].iterator;
			sub_iter->funcs->dtor(sub_iter TSRMLS_CC);
			zval_ptr_dtor(&object->iterators[object->level--].zobject);
		}
		efree(object->iterators);
		object->iterators = NULL;
	}

	zend_hash_destroy(object->std.properties);
	FREE_HASHTABLE(object->std.properties);

	efree(object);
}
/* }}} */

/* {{{ spl_RecursiveIteratorIterator_new */
static zend_object_value spl_RecursiveIteratorIterator_new(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	spl_recursive_it_object *intern;
	zval *tmp;

	intern = emalloc(sizeof(spl_recursive_it_object));
	memset(intern, 0, sizeof(spl_recursive_it_object));
	intern->std.ce = class_type;

	ALLOC_HASHTABLE(intern->std.properties);
	zend_hash_init(intern->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
	zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t) spl_RecursiveIteratorIterator_free_storage, NULL TSRMLS_CC);
	retval.handlers = &spl_handlers_rec_it_it;
	return retval;
}
/* }}} */

static
ZEND_BEGIN_ARG_INFO(arginfo_recursive_it___construct, 0) 
	ZEND_ARG_OBJ_INFO(0, iterator, Traversable, 0)
	ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO();

static
ZEND_BEGIN_ARG_INFO(arginfo_recursive_it_getSubIterator, 0) 
	ZEND_ARG_INFO(0, level)
ZEND_END_ARG_INFO();

static
ZEND_BEGIN_ARG_INFO(arginfo_recursive_it_setMaxDepth, 0) 
	ZEND_ARG_INFO(0, max_depth)
ZEND_END_ARG_INFO();

static zend_function_entry spl_funcs_RecursiveIteratorIterator[] = {
	SPL_ME(RecursiveIteratorIterator, __construct,       arginfo_recursive_it___construct,    ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, rewind,            NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, valid,             NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, key,               NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, current,           NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, next,              NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, getDepth,          NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, getSubIterator,    arginfo_recursive_it_getSubIterator, ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, getInnerIterator,  NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, beginIteration,    NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, endIteration,      NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, callHasChildren,   NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, callGetChildren,   NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, beginChildren,     NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, endChildren,       NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, nextElement,       NULL,                                ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, setMaxDepth,       arginfo_recursive_it_setMaxDepth,    ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveIteratorIterator, getMaxDepth,       NULL,                                ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

#if MBO_0
static int spl_dual_it_gets_implemented(zend_class_entry *interface, zend_class_entry *class_type TSRMLS_DC)
{
	class_type->iterator_funcs.zf_valid = NULL;
	class_type->iterator_funcs.zf_current = NULL;
	class_type->iterator_funcs.zf_key = NULL;
	class_type->iterator_funcs.zf_next = NULL;
	class_type->iterator_funcs.zf_rewind = NULL;
	if (!class_type->iterator_funcs.funcs) {
		class_type->iterator_funcs.funcs = &zend_interface_iterator_funcs_iterator;
	}

	return SUCCESS;
}
#endif

static union _zend_function *spl_dual_it_get_method(zval **object_ptr, char *method, int method_len TSRMLS_DC)
{
	union _zend_function *function_handler;
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(*object_ptr TSRMLS_CC);

	function_handler = std_object_handlers.get_method(object_ptr, method, method_len TSRMLS_CC);
	if (!function_handler) {
		if (zend_hash_find(&intern->inner.ce->function_table, method, method_len+1, (void **) &function_handler) == FAILURE) {
			if (Z_OBJ_HT_P(intern->inner.zobject)->get_method) {
				*object_ptr = intern->inner.zobject;
				function_handler = Z_OBJ_HT_P(*object_ptr)->get_method(object_ptr, method, method_len TSRMLS_CC);
			}
		}
	}
	return function_handler;
}

#if MBO_0
int spl_dual_it_call_method(char *method, INTERNAL_FUNCTION_PARAMETERS)
{
	zval ***func_params, func;
	zval *retval_ptr;
	int arg_count;
	int current = 0;
	int success;
	void **p;
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	ZVAL_TEXT(&func, method, 0);
	if (!zend_is_callable(&func, 0, &method)) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Method %v::%R() does not exist", intern->inner.ce->name, Z_TYPE(method), Z_UNIVAL(method));
		return FAILURE;
	}

	p = EG(argument_stack).top_element-2;
	arg_count = (ulong) *p;

	func_params = safe_emalloc(sizeof(zval **), arg_count, 0);

	current = 0;
	while (arg_count-- > 0) {
		func_params[current] = (zval **) p - (arg_count-current);
		current++;
	}

	if (call_user_function_ex(EG(function_table), NULL, &func, &retval_ptr, arg_count, func_params, 0, NULL TSRMLS_CC) == SUCCESS && retval_ptr) {
		RETURN_ZVAL(retval_ptr, 0, 1);
		
		success = SUCCESS;
	} else {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Unable to call %s::%s()", intern->inner.ce->name, method);
		success = FAILURE;
	}

	efree(func_params); 
	return success;
}
#endif

static inline int spl_dual_it_fetch(spl_dual_it_object *intern, int check_more TSRMLS_DC);

static inline spl_dual_it_object* spl_dual_it_construct(INTERNAL_FUNCTION_PARAMETERS, zend_class_entry *ce_inner, dual_it_type dit_type)
{
	zval                 *zobject, *retval;
	spl_dual_it_object   *intern;
	zend_class_entry     *ce;
	int                   inc_refcount = 1;


	php_set_error_handling(EH_THROW, U_CLASS_ENTRY(spl_ce_InvalidArgumentException) TSRMLS_CC);

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	intern->dit_type = dit_type;
	switch (dit_type) {
		case DIT_LimitIterator: {
			intern->u.limit.offset = 0; /* start at beginning */
			intern->u.limit.count = -1; /* get all */
			if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|ll", &zobject, ce_inner, &intern->u.limit.offset, &intern->u.limit.count) == FAILURE) {
				php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
				return NULL;
			}
			if (intern->u.limit.offset < 0) {
				php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
				zend_throw_exception(U_CLASS_ENTRY(spl_ce_OutOfRangeException), "Parameter offset must be > 0", 0 TSRMLS_CC);
				return NULL;
			}
			if (intern->u.limit.count < 0 && intern->u.limit.count != -1) {
				php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
				zend_throw_exception(U_CLASS_ENTRY(spl_ce_OutOfRangeException), "Parameter count must either be -1 or a value greater than or equal 0", 0 TSRMLS_CC);
				return NULL;
			}
			break;
		}
		case DIT_CachingIterator:
		case DIT_RecursiveCachingIterator: {
			long flags = CIT_CALL_TOSTRING;
			if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|l", &zobject, ce_inner, &flags) == FAILURE) {
				php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
				return NULL;
			}
			if (((flags & CIT_CALL_TOSTRING) && (flags & (CIT_TOSTRING_USE_KEY|CIT_TOSTRING_USE_CURRENT)))
			|| ((flags & (CIT_TOSTRING_USE_KEY|CIT_TOSTRING_USE_CURRENT)) == (CIT_TOSTRING_USE_KEY|CIT_TOSTRING_USE_CURRENT))) {
				php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
				zend_throw_exception(U_CLASS_ENTRY(spl_ce_InvalidArgumentException), "Flags must contain only one of CIT_CALL_TOSTRING, CIT_TOSTRING_USE_KEY, CIT_TOSTRING_USE_CURRENT", 0 TSRMLS_CC);
				return NULL;
			}
			intern->u.caching.flags |= flags & CIT_PUBLIC;
			MAKE_STD_ZVAL(intern->u.caching.zcache);
			array_init(intern->u.caching.zcache);
			break;
		}
		case DIT_IteratorIterator: {
			zend_class_entry **pce_cast;
			char * class_name;
			int class_name_len;

			if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|s", &zobject, ce_inner, &class_name, &class_name_len) == FAILURE) {
				php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
				return NULL;
			}
			ce = Z_OBJCE_P(zobject);
			if (!instanceof_function(ce, U_CLASS_ENTRY(zend_ce_iterator) TSRMLS_CC)) {
				if (ZEND_NUM_ARGS() > 1) {
					if (zend_lookup_class(class_name, class_name_len, &pce_cast TSRMLS_CC) == FAILURE 
					|| !instanceof_function(ce, *pce_cast TSRMLS_CC)
					|| !(*pce_cast)->get_iterator
					) {
						zend_throw_exception(U_CLASS_ENTRY(spl_ce_LogicException), "Class to downcast to not found or not base class or does not implement Traversable", 0 TSRMLS_CC);
						php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
						return NULL;
					}
					ce = *pce_cast;
				}
				if (instanceof_function(ce, U_CLASS_ENTRY(zend_ce_aggregate) TSRMLS_CC)) {
					zend_call_method_with_0_params(&zobject, ce, &ce->iterator_funcs.zf_new_iterator, "getiterator", &retval);
					if (!retval || Z_TYPE_P(retval) != IS_OBJECT || !instanceof_function(Z_OBJCE_P(retval), U_CLASS_ENTRY(zend_ce_traversable) TSRMLS_CC)) {
						zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_LogicException), 0 TSRMLS_CC, "%s::getIterator() must return an object that implememnts Traversable", ce->name);
						php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
						return NULL;
					}
					zobject = retval;
					ce = Z_OBJCE_P(zobject);
					inc_refcount = 0;
				}
			}
			break;
		}
		case DIT_AppendIterator:
			spl_instantiate(U_CLASS_ENTRY(spl_ce_ArrayIterator), &intern->u.append.zarrayit, 1 TSRMLS_CC);
			zend_call_method_with_0_params(&intern->u.append.zarrayit, U_CLASS_ENTRY(spl_ce_ArrayIterator), &U_CLASS_ENTRY(spl_ce_ArrayIterator)->constructor, "__construct", NULL);
			intern->u.append.iterator = U_CLASS_ENTRY(spl_ce_ArrayIterator)->get_iterator(U_CLASS_ENTRY(spl_ce_ArrayIterator), intern->u.append.zarrayit TSRMLS_CC);
			php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
			return intern;
#if HAVE_PCRE || HAVE_BUNDLED_PCRE
		case DIT_RegExIterator:
		case DIT_RecursiveRegExIterator: {
			char *regex;
			int len, poptions, coptions;
			pcre_extra *extra = NULL;

			intern->u.regex.flags = 0;
			if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os|l", &zobject, ce_inner, &regex, &len, &intern->u.regex.flags) == FAILURE) {
				php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
				return NULL;
			}
			intern->u.regex.pce = pcre_get_compiled_regex_cache(regex, len, &extra, &poptions, &coptions TSRMLS_CC);
			intern->u.regex.pce->refcount++;
			break;;
		}
#endif
		default:
			if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &zobject, ce_inner) == FAILURE) {
				php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
				return NULL;
			}
			break;
	}

	php_set_error_handling(EH_THROW, zend_exception_get_default(TSRMLS_C) TSRMLS_CC);

	if (inc_refcount) {
		zobject->refcount++;
	}
	intern->inner.zobject = zobject;
	intern->inner.ce = dit_type == DIT_IteratorIterator ? ce : Z_OBJCE_P(zobject);
	intern->inner.object = zend_object_store_get_object(zobject TSRMLS_CC);
	intern->inner.iterator = intern->inner.ce->get_iterator(intern->inner.ce, zobject TSRMLS_CC);

	php_set_error_handling(EH_NORMAL, NULL TSRMLS_CC);
	return intern;
}

/* {{{ proto FilterIterator::__construct(Iterator it) 
   Create an Iterator from another iterator */
SPL_METHOD(dual_it, __construct)
{
	spl_dual_it_construct(INTERNAL_FUNCTION_PARAM_PASSTHRU, U_CLASS_ENTRY(zend_ce_iterator), DIT_Default);
} /* }}} */

/* {{{ proto Iterator FilterIterator::getInnerIterator() 
       proto Iterator CachingIterator::getInnerIterator()
       proto Iterator LimitIterator::getInnerIterator()
       proto Iterator ParentIterator::getInnerIterator()
   Get the inner iterator */
SPL_METHOD(dual_it, getInnerIterator)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (intern->inner.zobject) {
		RETVAL_ZVAL(intern->inner.zobject, 1, 0);
	} else {
		RETURN_NULL();
	}
} /* }}} */

static inline void spl_dual_it_require(spl_dual_it_object *intern TSRMLS_DC)
{
	if (!intern->inner.iterator) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "The inner constructor wasn't initialized with an iterator instance");
	}
}

static inline void spl_dual_it_free(spl_dual_it_object *intern TSRMLS_DC)
{
	if (intern->inner.iterator && intern->inner.iterator->funcs->invalidate_current) {
		intern->inner.iterator->funcs->invalidate_current(intern->inner.iterator TSRMLS_CC);
	}
	if (intern->current.data) {
		zval_ptr_dtor(&intern->current.data);
		intern->current.data = NULL;
	}
	if (intern->current.str_key) {
		efree(intern->current.str_key);
		intern->current.str_key = NULL;
	}
	if (intern->dit_type == DIT_CachingIterator || intern->dit_type == DIT_RecursiveCachingIterator) {
		if (intern->u.caching.zstr) {
			zval_ptr_dtor(&intern->u.caching.zstr);
			intern->u.caching.zstr = NULL;
		}
		if (intern->u.caching.zchildren) {
			zval_ptr_dtor(&intern->u.caching.zchildren);
			intern->u.caching.zchildren = NULL;
		}
	}
}

static inline void spl_dual_it_rewind(spl_dual_it_object *intern TSRMLS_DC)
{
	spl_dual_it_free(intern TSRMLS_CC);
	intern->current.pos = 0;
	if (intern->inner.iterator->funcs->rewind) {
		intern->inner.iterator->funcs->rewind(intern->inner.iterator TSRMLS_CC);
	}
}

static inline int spl_dual_it_valid(spl_dual_it_object *intern TSRMLS_DC)
{
	/* FAILURE / SUCCESS */
	return intern->inner.iterator->funcs->valid(intern->inner.iterator TSRMLS_CC);
}

static inline int spl_dual_it_fetch(spl_dual_it_object *intern, int check_more TSRMLS_DC)
{
	zval **data;

	spl_dual_it_free(intern TSRMLS_CC);
	if (!check_more || spl_dual_it_valid(intern TSRMLS_CC) == SUCCESS) {
		intern->inner.iterator->funcs->get_current_data(intern->inner.iterator, &data TSRMLS_CC);
		intern->current.data = *data;
		intern->current.data->refcount++;
		if (intern->inner.iterator->funcs->get_current_key) {
			intern->current.key_type = intern->inner.iterator->funcs->get_current_key(intern->inner.iterator, &intern->current.str_key, &intern->current.str_key_len, &intern->current.int_key TSRMLS_CC);
		} else {
			intern->current.key_type = HASH_KEY_IS_LONG;
			intern->current.int_key = intern->current.pos;
		}
		return SUCCESS;
	}
	return FAILURE;
}

static inline void spl_dual_it_next(spl_dual_it_object *intern, int do_free TSRMLS_DC)
{
	if (do_free) {
		spl_dual_it_free(intern TSRMLS_CC);
	} else {
		spl_dual_it_require(intern TSRMLS_CC);
	}
	intern->inner.iterator->funcs->move_forward(intern->inner.iterator TSRMLS_CC);
	intern->current.pos++;
}

/* {{{ proto void ParentIterator::rewind()
       proto void IteratorIterator::rewind()
   Rewind the iterator
   */
SPL_METHOD(dual_it, rewind)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	spl_dual_it_rewind(intern TSRMLS_CC);
	spl_dual_it_fetch(intern, 1 TSRMLS_CC);
} /* }}} */

/* {{{ proto boolean FilterIterator::valid()
       proto boolean ParentIterator::valid()
       proto boolean IteratorIterator::valid()
       proto boolean NoRewindIterator::valid()
   Check whether the current element is valid */
SPL_METHOD(dual_it, valid)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_BOOL(intern->current.data);
} /* }}} */

/* {{{ proto mixed FilterIterator::key()
       proto mixed CachingIterator::key()
       proto mixed LimitIterator::key()
       proto mixed ParentIterator::key()
       proto mixed IteratorIterator::key()
       proto mixed NoRewindIterator::key()
       proto mixed AppendIterator::key()
   Get the current key */
SPL_METHOD(dual_it, key)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (intern->current.data) {
		if (intern->current.key_type == HASH_KEY_IS_STRING) {
			RETURN_STRINGL(intern->current.str_key, intern->current.str_key_len-1, 1);
		} else if (intern->current.key_type == HASH_KEY_IS_UNICODE) {
			RETURN_UNICODEL((UChar *)intern->current.str_key, intern->current.str_key_len-1, 1);
		} else {
			RETURN_LONG(intern->current.int_key);
		}
	}
	RETURN_NULL();
} /* }}} */

/* {{{ proto mixed FilterIterator::current()
       proto mixed CachingIterator::current()
       proto mixed LimitIterator::current()
       proto mixed ParentIterator::current()
       proto mixed IteratorIterator::current()
       proto mixed NoRewindIterator::current()
       proto mixed AppendIterator::current()
   Get the current element value */
SPL_METHOD(dual_it, current)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (intern->current.data) {
		RETVAL_ZVAL(intern->current.data, 1, 0);
	} else {
		RETURN_NULL();
	}
} /* }}} */

/* {{{ proto void ParentIterator::next()
       proto void IteratorIterator::next()
       proto void NoRewindIterator::next()
   Move the iterator forward */
SPL_METHOD(dual_it, next)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	spl_dual_it_next(intern, 1 TSRMLS_CC);
	spl_dual_it_fetch(intern, 1 TSRMLS_CC);
} /* }}} */

static inline void spl_filter_it_fetch(zval *zthis, spl_dual_it_object *intern TSRMLS_DC)
{
	zval *retval;

	while (spl_dual_it_fetch(intern, 1 TSRMLS_CC) == SUCCESS) {
		zend_call_method_with_0_params(&zthis, intern->std.ce, NULL, "accept", &retval);
		if (retval) {
			if (zend_is_true(retval)) {
				zval_ptr_dtor(&retval);
				return;
			}
			zval_ptr_dtor(&retval);
		}

		intern->inner.iterator->funcs->move_forward(intern->inner.iterator TSRMLS_CC);
	}
	spl_dual_it_free(intern TSRMLS_CC);
}

static inline void spl_filter_it_rewind(zval *zthis, spl_dual_it_object *intern TSRMLS_DC)
{
	spl_dual_it_rewind(intern TSRMLS_CC);
	spl_filter_it_fetch(zthis, intern TSRMLS_CC);
}

static inline void spl_filter_it_next(zval *zthis, spl_dual_it_object *intern TSRMLS_DC)
{
	spl_dual_it_next(intern, 1 TSRMLS_CC);
	spl_filter_it_fetch(zthis, intern TSRMLS_CC);
}

/* {{{ proto void FilterIterator::rewind()
   Rewind the iterator */
SPL_METHOD(FilterIterator, rewind)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	spl_filter_it_rewind(getThis(), intern TSRMLS_CC);
} /* }}} */

/* {{{ proto void FilterIterator::next()
   Move the iterator forward */
SPL_METHOD(FilterIterator, next)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	spl_filter_it_next(getThis(), intern TSRMLS_CC);
} /* }}} */

/* {{{ proto RecursiveFilterIterator::__construct(RecursiveIterator it)
   Create a RecursiveFilterIterator from a RecursiveIterator */
SPL_METHOD(RecursiveFilterIterator, __construct)
{
	spl_dual_it_construct(INTERNAL_FUNCTION_PARAM_PASSTHRU, U_CLASS_ENTRY(spl_ce_RecursiveIterator), DIT_Default);
} /* }}} */

/* {{{ proto boolean RecursiveFilterIterator::hasChildren()
   Check whether the inner iterator's current element has children */
SPL_METHOD(RecursiveFilterIterator, hasChildren)
{
	spl_dual_it_object   *intern;
	zval                 *retval;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	zend_call_method_with_0_params(&intern->inner.zobject, intern->inner.ce, NULL, "haschildren", &retval);
	RETURN_ZVAL(retval, 0, 1);
} /* }}} */

/* {{{ proto RecursiveFilterIterator RecursiveFilterIterator::getChildren()
   Return the inner iterator's children contained in a RecursiveFilterIterator */
SPL_METHOD(RecursiveFilterIterator, getChildren)
{
	spl_dual_it_object   *intern;
	zval                 *retval;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	zend_call_method_with_0_params(&intern->inner.zobject, intern->inner.ce, NULL, "getchildren", &retval);
	spl_instantiate_arg_ex1(Z_OBJCE_P(getThis()), &return_value, 0, retval TSRMLS_CC);
	zval_ptr_dtor(&retval);
} /* }}} */

/* {{{ proto ParentIterator::__construct(RecursiveIterator it)
   Create a ParentIterator from a RecursiveIterator */
SPL_METHOD(ParentIterator, __construct)
{
	spl_dual_it_construct(INTERNAL_FUNCTION_PARAM_PASSTHRU, U_CLASS_ENTRY(spl_ce_RecursiveIterator), DIT_Default);
} /* }}} */

/* {{{ proto boolean ParentIterator::hasChildren()
   Check whether the inner iterator's current element has children */
SPL_METHOD(ParentIterator, hasChildren)
{
	spl_dual_it_object   *intern;
	zval                 *retval;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	zend_call_method_with_0_params(&intern->inner.zobject, intern->inner.ce, NULL, "haschildren", &retval);
	RETURN_ZVAL(retval, 0, 1);
} /* }}} */

/* {{{ proto ParentIterator ParentIterator::getChildren()
   Return the inner iterator's children contained in a ParentIterator */
SPL_METHOD(ParentIterator, getChildren)
{
	spl_dual_it_object   *intern;
	zval                 *retval;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	zend_call_method_with_0_params(&intern->inner.zobject, intern->inner.ce, NULL, "getchildren", &retval);
	spl_instantiate_arg_ex1(Z_OBJCE_P(getThis()), &return_value, 0, retval TSRMLS_CC);
	zval_ptr_dtor(&retval);
} /* }}} */

#if HAVE_PCRE || HAVE_BUNDLED_PCRE
/* {{{ proto RegExIterator::__construct(Iterator it, string $regex [, int $flags]) 
   Create an RegExIterator from another iterator and a regular expression */
SPL_METHOD(RegExIterator, __construct)
{
	spl_dual_it_construct(INTERNAL_FUNCTION_PARAM_PASSTHRU, U_CLASS_ENTRY(zend_ce_iterator), DIT_RegExIterator);
} /* }}} */

/* {{{ proto bool RegExIterator::accept()
   Match (string)current() against regular expression */
SPL_METHOD(RegExIterator, accept)
{
	spl_dual_it_object   *intern;
	int count;
	char *subject, tmp[32];
	int subject_len, use_copy = 0;
	zval subject_copy;
	pcre_extra *extra;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (intern->u.regex.flags & REGIT_USE_KEY) {
		if (intern->current.key_type == HASH_KEY_IS_LONG) {
			subject_len = snprintf(tmp, sizeof(tmp), "%ld", intern->current.int_key);
			subject = &tmp[0];
		} else {
			subject_len = intern->current.str_key_len;
			subject = intern->current.str_key;
		}
	} else {
		zend_make_printable_zval(intern->current.data, &subject_copy, &use_copy);
		if (use_copy) {
			subject = Z_STRVAL(subject_copy);
			subject_len = Z_STRLEN(subject_copy);
		} else {
			subject = Z_STRVAL_P(intern->current.data);
			subject_len = Z_STRLEN_P(intern->current.data);
		}
	}

	extra = intern->u.regex.pce->extra;
	count = pcre_exec(intern->u.regex.pce->re, extra, subject, subject_len, 0, 0, NULL, 0);

	if (use_copy) {
		zval_dtor(&subject_copy);
	}

	RETURN_BOOL(count >= 0);
} /* }}} */

/* {{{ proto RecursiveRegExIterator::__construct(RecursiveIterator it, string $regex [, int $flags]) 
   Create an RecursiveRegExIterator from another recursive iterator and a regular expression */
SPL_METHOD(RecursiveRegExIterator, __construct)
{
	spl_dual_it_construct(INTERNAL_FUNCTION_PARAM_PASSTHRU, U_CLASS_ENTRY(spl_ce_RecursiveIterator), DIT_RecursiveRegExIterator);
} /* }}} */
#endif

/* {{{ spl_dual_it_free_storage */
static inline void spl_dual_it_free_storage(void *_object TSRMLS_DC)
{
	spl_dual_it_object        *object = (spl_dual_it_object *)_object;

	spl_dual_it_free(object TSRMLS_CC);

	if (object->inner.iterator) {
		object->inner.iterator->funcs->dtor(object->inner.iterator TSRMLS_CC);
	}

	if (object->inner.zobject) {
		zval_ptr_dtor(&object->inner.zobject);
	}
	
	if (object->dit_type == DIT_AppendIterator) {
		object->u.append.iterator->funcs->dtor(object->u.append.iterator TSRMLS_CC);
		zval_ptr_dtor(&object->u.append.zarrayit);
	}

	if (object->dit_type == DIT_CachingIterator || object->dit_type == DIT_RecursiveCachingIterator) {
		if (object->u.caching.zcache) {
			zval_ptr_dtor(&object->u.caching.zcache);
			object->u.caching.zcache = NULL;
		}
	}

#if HAVE_PCRE || HAVE_BUNDLED_PCRE
	if (object->dit_type == DIT_RegExIterator || object->dit_type == DIT_RecursiveRegExIterator) {
		if (object->u.regex.pce) {
			object->u.regex.pce->refcount--;
		}
	}
#endif

	zend_hash_destroy(object->std.properties);
	FREE_HASHTABLE(object->std.properties);

	efree(object);
}
/* }}} */

/* {{{ spl_dual_it_new */
static zend_object_value spl_dual_it_new(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	spl_dual_it_object *intern;
	zval *tmp;

	intern = emalloc(sizeof(spl_dual_it_object));
	memset(intern, 0, sizeof(spl_dual_it_object));
	intern->std.ce = class_type;

	ALLOC_HASHTABLE(intern->std.properties);
	zend_hash_init(intern->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
	zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t) spl_dual_it_free_storage, NULL TSRMLS_CC);
	retval.handlers = &spl_handlers_dual_it;
	return retval;
}
/* }}} */

static
ZEND_BEGIN_ARG_INFO(arginfo_filter_it___construct, 0) 
	ZEND_ARG_OBJ_INFO(0, iterator, Iterator, 0)
ZEND_END_ARG_INFO();

static zend_function_entry spl_funcs_FilterIterator[] = {
	SPL_ME(dual_it,         __construct,      arginfo_filter_it___construct, ZEND_ACC_PUBLIC)
	SPL_ME(FilterIterator,  rewind,           NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,         valid,            NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,         key,              NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,         current,          NULL, ZEND_ACC_PUBLIC)
	SPL_ME(FilterIterator,  next,             NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,         getInnerIterator, NULL, ZEND_ACC_PUBLIC)
	SPL_ABSTRACT_ME(FilterIterator, accept, NULL)
	{NULL, NULL, NULL}
};

static
ZEND_BEGIN_ARG_INFO(arginfo_parent_it___construct, 0) 
	ZEND_ARG_OBJ_INFO(0, iterator, RecursiveIterator, 0)
ZEND_END_ARG_INFO();

static zend_function_entry spl_funcs_RecursiveFilterIterator[] = {
	SPL_ME(RecursiveFilterIterator,  __construct,      arginfo_parent_it___construct, ZEND_ACC_PUBLIC)
	SPL_MA(ParentIterator,  accept,  RecursiveFilterIterator, hasChildren, NULL, ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveFilterIterator,  hasChildren,      NULL, ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveFilterIterator,  getChildren,      NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

static zend_function_entry spl_funcs_ParentIterator[] = {
	SPL_ME(ParentIterator,  __construct,      arginfo_parent_it___construct, ZEND_ACC_PUBLIC)
	SPL_MA(ParentIterator,  accept,           ParentIterator, hasChildren, NULL, ZEND_ACC_PUBLIC)
	SPL_ME(ParentIterator,  hasChildren,      NULL, ZEND_ACC_PUBLIC)
	SPL_ME(ParentIterator,  getChildren,      NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,         getInnerIterator, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

#if HAVE_PCRE || HAVE_BUNDLED_PCRE
static
ZEND_BEGIN_ARG_INFO(arginfo_regex_it___construct, 0) 
	ZEND_ARG_OBJ_INFO(0, iterator, Iterator, 0)
	ZEND_ARG_INFO(0, regex)
ZEND_END_ARG_INFO();

static zend_function_entry spl_funcs_RegExIterator[] = {
	SPL_ME(RegExIterator,   __construct,      arginfo_regex_it___construct, ZEND_ACC_PUBLIC)
	SPL_ME(RegExIterator,   accept,           NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

static zend_function_entry spl_funcs_RecursiveRegExIterator[] = {
	SPL_ME(RecursiveRegExIterator, __construct,      arginfo_regex_it___construct, ZEND_ACC_PUBLIC)
	SPL_ME(ParentIterator,         hasChildren,      NULL, ZEND_ACC_PUBLIC)
	SPL_ME(ParentIterator,         getChildren,      NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
#endif

static inline int spl_limit_it_valid(spl_dual_it_object *intern TSRMLS_DC)
{
	/* FAILURE / SUCCESS */
	if (intern->u.limit.count != -1 && intern->current.pos >= intern->u.limit.offset + intern->u.limit.count) {
		return FAILURE;
	} else {
		return spl_dual_it_valid(intern TSRMLS_CC);
	}
}

static inline void spl_limit_it_seek(spl_dual_it_object *intern, long pos TSRMLS_DC)
{
	zval  *zpos;

	spl_dual_it_free(intern TSRMLS_CC);
	if (pos < intern->u.limit.offset) {
		zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_OutOfBoundsException), 0 TSRMLS_CC, "Cannot seek to %ld which is below the offset %ld", pos, intern->u.limit.offset);
		return;
	}
	if (pos > intern->u.limit.offset + intern->u.limit.count && intern->u.limit.count != -1) {
		zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_OutOfBoundsException), 0 TSRMLS_CC, "Cannot seek to %ld which is behind offest %ld plus count %ld", pos, intern->u.limit.offset, intern->u.limit.count);
		return;
	}
	if (instanceof_function(intern->inner.ce, U_CLASS_ENTRY(spl_ce_SeekableIterator) TSRMLS_CC)) {
		MAKE_STD_ZVAL(zpos);
		ZVAL_LONG(zpos, pos);
		spl_dual_it_free(intern TSRMLS_CC);
		zend_call_method_with_1_params(&intern->inner.zobject, intern->inner.ce, NULL, "seek", NULL, zpos);
		zval_ptr_dtor(&zpos);
		intern->current.pos = pos;
		if (spl_limit_it_valid(intern TSRMLS_CC) == SUCCESS) {
			spl_dual_it_fetch(intern, 0 TSRMLS_CC);
		}
	} else {
		/* emulate the forward seek, by next() calls */
		/* a back ward seek is done by a previous rewind() */
		if (pos < intern->current.pos) {
			spl_dual_it_rewind(intern TSRMLS_CC);
		}
		while (pos > intern->current.pos && spl_dual_it_valid(intern TSRMLS_CC) == SUCCESS) {
			spl_dual_it_next(intern, 1 TSRMLS_CC);
		}
		if (spl_dual_it_valid(intern TSRMLS_CC) == SUCCESS) {
			spl_dual_it_fetch(intern, 1 TSRMLS_CC);
		}
	}
}

/* {{{ proto LimitIterator::__construct(Iterator it [, int offset, int count])
   Construct a LimitIterator from an Iterator with a given starting offset and optionally a maximum count */
SPL_METHOD(LimitIterator, __construct)
{
	spl_dual_it_construct(INTERNAL_FUNCTION_PARAM_PASSTHRU, U_CLASS_ENTRY(zend_ce_iterator), DIT_LimitIterator);
} /* }}} */

/* {{{ proto void LimitIterator::rewind() 
   Rewind the iterator to the specified starting offset */
SPL_METHOD(LimitIterator, rewind)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	spl_dual_it_rewind(intern TSRMLS_CC);
	spl_limit_it_seek(intern, intern->u.limit.offset TSRMLS_CC);
} /* }}} */

/* {{{ proto boolean LimitIterator::valid()
   Check whether the current element is valid */
SPL_METHOD(LimitIterator, valid)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

/*	RETURN_BOOL(spl_limit_it_valid(intern TSRMLS_CC) == SUCCESS);*/
	RETURN_BOOL((intern->u.limit.count == -1 || intern->current.pos < intern->u.limit.offset + intern->u.limit.count) && intern->current.data);
} /* }}} */

/* {{{ proto void LimitIterator::next()
   Move the iterator forward */
SPL_METHOD(LimitIterator, next)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	spl_dual_it_next(intern, 1 TSRMLS_CC);
	if (intern->u.limit.count == -1 || intern->current.pos < intern->u.limit.offset + intern->u.limit.count) {
		spl_dual_it_fetch(intern, 1 TSRMLS_CC);
	}
} /* }}} */

/* {{{ proto void LimitIterator::seek(int position)
   Seek to the given position */
SPL_METHOD(LimitIterator, seek)
{
	spl_dual_it_object   *intern;
	long                 pos;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &pos) == FAILURE) {
		return;
	}

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	spl_limit_it_seek(intern, pos TSRMLS_CC);
	RETURN_LONG(intern->current.pos);
} /* }}} */

/* {{{ proto int LimitIterator::getPosition()
   Return the current position */
SPL_METHOD(LimitIterator, getPosition)
{
	spl_dual_it_object   *intern;
	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_LONG(intern->current.pos);
} /* }}} */

static
ZEND_BEGIN_ARG_INFO(arginfo_seekable_it_seek, 0) 
	ZEND_ARG_INFO(0, position)
ZEND_END_ARG_INFO();

static zend_function_entry spl_funcs_SeekableIterator[] = {
	SPL_ABSTRACT_ME(SeekableIterator, seek, arginfo_seekable_it_seek)
	{NULL, NULL, NULL}
};

static
ZEND_BEGIN_ARG_INFO(arginfo_limit_it___construct, 0) 
	ZEND_ARG_OBJ_INFO(0, iterator, Iterator, 0)
	ZEND_ARG_INFO(0, offset)
	ZEND_ARG_INFO(0, count)
ZEND_END_ARG_INFO();

static
ZEND_BEGIN_ARG_INFO(arginfo_limit_it_seek, 0) 
	ZEND_ARG_INFO(0, position)
ZEND_END_ARG_INFO();

static zend_function_entry spl_funcs_LimitIterator[] = {
	SPL_ME(LimitIterator,   __construct,      arginfo_limit_it___construct, ZEND_ACC_PUBLIC)
	SPL_ME(LimitIterator,   rewind,           NULL, ZEND_ACC_PUBLIC)
	SPL_ME(LimitIterator,   valid,            NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,         key,              NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,         current,          NULL, ZEND_ACC_PUBLIC)
	SPL_ME(LimitIterator,   next,             NULL, ZEND_ACC_PUBLIC)
	SPL_ME(LimitIterator,   seek,             arginfo_limit_it_seek, ZEND_ACC_PUBLIC)
	SPL_ME(LimitIterator,   getPosition,      NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,         getInnerIterator, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

static inline int spl_caching_it_valid(spl_dual_it_object *intern TSRMLS_DC)
{
	return intern->u.caching.flags & CIT_VALID ? SUCCESS : FAILURE;
}

static inline int spl_caching_it_has_next(spl_dual_it_object *intern TSRMLS_DC)
{
	return spl_dual_it_valid(intern TSRMLS_CC);
}

static inline void spl_caching_it_next(spl_dual_it_object *intern TSRMLS_DC)
{
	if (spl_dual_it_fetch(intern, 1 TSRMLS_CC) == SUCCESS) {
		intern->u.caching.flags |= CIT_VALID;
		/* Full cache ? */
		if (intern->u.caching.flags & CIT_FULL_CACHE) {
			zval *zcacheval;
			
			MAKE_STD_ZVAL(zcacheval);
			ZVAL_ZVAL(zcacheval, intern->current.data, 1, 0);
			if (intern->current.key_type == HASH_KEY_IS_LONG) {
				add_index_zval(intern->u.caching.zcache, intern->current.int_key, zcacheval);
			} else {
				zend_u_symtable_update(HASH_OF(intern->u.caching.zcache), intern->current.key_type, intern->current.str_key, intern->current.str_key_len, &zcacheval, sizeof(void*), NULL);
			}
		}
		/* Recursion ? */
		if (intern->dit_type == DIT_RecursiveCachingIterator) {
			zval *retval, *zchildren, zflags;
			zend_call_method_with_0_params(&intern->inner.zobject, intern->inner.ce, NULL, "haschildren", &retval);
			if (zend_is_true(retval)) {
				zend_call_method_with_0_params(&intern->inner.zobject, intern->inner.ce, NULL, "getchildren", &zchildren);
				if (EG(exception) && intern->u.caching.flags & CIT_CATCH_GET_CHILD) {
					zend_clear_exception(TSRMLS_C);
					if (zchildren) {
						zval_ptr_dtor(&zchildren);
					}
				} else {
					INIT_PZVAL(&zflags);
					ZVAL_LONG(&zflags, intern->u.caching.flags & CIT_PUBLIC);
					spl_instantiate_arg_ex2(U_CLASS_ENTRY(spl_ce_RecursiveCachingIterator), &intern->u.caching.zchildren, 1, zchildren, &zflags TSRMLS_CC);
					zval_ptr_dtor(&zchildren);
				}
			}
			zval_ptr_dtor(&retval);		
		}
		if (intern->u.caching.flags & CIT_CALL_TOSTRING) {
			int  use_copy;
			zval expr_copy;
			ALLOC_ZVAL(intern->u.caching.zstr);
			*intern->u.caching.zstr = *intern->current.data;
			if (UG(unicode)) {
				zend_make_unicode_zval(intern->u.caching.zstr, &expr_copy, &use_copy);
			} else {
				zend_make_printable_zval(intern->u.caching.zstr, &expr_copy, &use_copy);
			}
			if (use_copy) {
				*intern->u.caching.zstr = expr_copy;
				INIT_PZVAL(intern->u.caching.zstr);
				zval_copy_ctor(intern->u.caching.zstr);
				zval_dtor(&expr_copy);
			} else {
				INIT_PZVAL(intern->u.caching.zstr);
				zval_copy_ctor(intern->u.caching.zstr);
			}
		}
		spl_dual_it_next(intern, 0 TSRMLS_CC);	
	} else {
		intern->u.caching.flags &= ~CIT_VALID;
	}
}

static inline void spl_caching_it_rewind(spl_dual_it_object *intern TSRMLS_DC)
{
	spl_dual_it_rewind(intern TSRMLS_CC);
	zend_hash_clean(HASH_OF(intern->u.caching.zcache));
	spl_caching_it_next(intern TSRMLS_CC);
}

/* {{{ proto CachingIterator::__construct(Iterator it [, flags = CIT_CALL_TOSTRING])
   Construct a CachingIterator from an Iterator */
SPL_METHOD(CachingIterator, __construct)
{
	spl_dual_it_construct(INTERNAL_FUNCTION_PARAM_PASSTHRU, U_CLASS_ENTRY(zend_ce_iterator), DIT_CachingIterator);
} /* }}} */

/* {{{ proto void CachingIterator::rewind()
   Rewind the iterator */
SPL_METHOD(CachingIterator, rewind)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	spl_caching_it_rewind(intern TSRMLS_CC);
} /* }}} */

/* {{{ proto boolean CachingIterator::valid()
   Check whether the current element is valid */
SPL_METHOD(CachingIterator, valid)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_BOOL(spl_caching_it_valid(intern TSRMLS_CC) == SUCCESS);
} /* }}} */

/* {{{ proto void CachingIterator::next()
   Move the iterator forward */
SPL_METHOD(CachingIterator, next)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	spl_caching_it_next(intern TSRMLS_CC);
} /* }}} */

/* {{{ proto boolean CachingIterator::hasNext()
   Check whether the inner iterator has a valid next element */
SPL_METHOD(CachingIterator, hasNext)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_BOOL(spl_caching_it_has_next(intern TSRMLS_CC) == SUCCESS);
} /* }}} */

/* {{{ proto string CachingIterator::__toString()
   Return the string representation of the current element */
SPL_METHOD(CachingIterator, __toString)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!(intern->u.caching.flags & (CIT_CALL_TOSTRING|CIT_TOSTRING_USE_KEY|CIT_TOSTRING_USE_CURRENT)))	{
		zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_BadMethodCallException), 0 TSRMLS_CC, "%v does not fetch string value (see CachingIterator::__construct)", Z_OBJCE_P(getThis())->name);
	}
	if (intern->u.caching.flags & CIT_TOSTRING_USE_KEY) {
		if (intern->current.key_type == HASH_KEY_IS_STRING) {
			RETURN_STRINGL(intern->current.str_key, intern->current.str_key_len, 1);
		} else if (intern->current.key_type == HASH_KEY_IS_UNICODE) {
			RETURN_UNICODEL((void*)intern->current.str_key, intern->current.str_key_len, 1);
		} else {
			RETVAL_LONG(intern->current.int_key);
			convert_to_string(return_value);
			return;
		}
	} else if (intern->u.caching.flags & CIT_TOSTRING_USE_CURRENT) {
		RETVAL_ZVAL(intern->current.data, 1, 0);
		
		return;
	}
	if (intern->u.caching.zstr) {
		*return_value = *intern->u.caching.zstr;
		zval_copy_ctor(return_value);
		convert_to_text(return_value);
		INIT_PZVAL(return_value);
	} else {
		RETURN_NULL();
	}
} /* }}} */

/* {{{ proto void CachingIterator::offsetSet(mixed index, mixed newval)
   Set given index in cache */
SPL_METHOD(CachingIterator, offsetSet)
{
	spl_dual_it_object   *intern;
	void *arKey;
	uint nKeyLength;
	zend_uchar type;
	zval *value;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!(intern->u.caching.flags & CIT_FULL_CACHE))	{
		zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_BadMethodCallException), 0 TSRMLS_CC, "%v does not use a full cache (see CachingIterator::__construct)", Z_OBJCE_P(getThis())->name);
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Tz", &arKey, &nKeyLength, &type, &value) == FAILURE) {
		return;
	}

	value->refcount++;
	zend_u_symtable_update(HASH_OF(intern->u.caching.zcache), type, arKey, nKeyLength+1, &value, sizeof(value), NULL);
}
/* }}} */

/* {{{ proto string CachingIterator::offsetGet(mixed index)
   Return the internal cache if used */
SPL_METHOD(CachingIterator, offsetGet)
{
	spl_dual_it_object   *intern;
	void *arKey;
	uint nKeyLength;
	zend_uchar type;
	zval **value;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!(intern->u.caching.flags & CIT_FULL_CACHE))	{
		zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_BadMethodCallException), 0 TSRMLS_CC, "%v does not use a full cache (see CachingIterator::__construct)", Z_OBJCE_P(getThis())->name);
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "T", &arKey, &nKeyLength, &type) == FAILURE) {
		return;
	}

	if (zend_u_symtable_find(HASH_OF(intern->u.caching.zcache), type, arKey, nKeyLength+1, (void**)&value) == FAILURE) {
		zend_error(E_NOTICE, "Undefined index:  %R", type, arKey);
		return;
	}
	
	RETURN_ZVAL(*value, 1, 0);
}
/* }}} */

/* {{{ proto void CachingIterator::offsetUnset(mixed index)
   Unset given index in cache */
SPL_METHOD(CachingIterator, offsetUnset)
{
	spl_dual_it_object   *intern;
	void *arKey;
	uint nKeyLength;
	zend_uchar type;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!(intern->u.caching.flags & CIT_FULL_CACHE))	{
		zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_BadMethodCallException), 0 TSRMLS_CC, "%v does not use a full cache (see CachingIterator::__construct)", Z_OBJCE_P(getThis())->name);
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "T", &arKey, &nKeyLength, &type) == FAILURE) {
		return;
	}

	zend_u_symtable_del(HASH_OF(intern->u.caching.zcache), type, arKey, nKeyLength+1);
}
/* }}} */

/* {{{ proto bool CachingIterator::offsetExists(mixed index)
   Return whether the requested index exists */
SPL_METHOD(CachingIterator, offsetExists)
{
	spl_dual_it_object   *intern;
	void *arKey;
	uint nKeyLength;
	zend_uchar type;
	
	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!(intern->u.caching.flags & CIT_FULL_CACHE))	{
		zend_throw_exception_ex(U_CLASS_ENTRY(spl_ce_BadMethodCallException), 0 TSRMLS_CC, "%v does not use a full cache (see CachingIterator::__construct)", Z_OBJCE_P(getThis())->name);
	}
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "T", &arKey, &nKeyLength, &type) == FAILURE) {
		return;
	}

	RETURN_BOOL(zend_u_symtable_exists(HASH_OF(intern->u.caching.zcache), type, arKey, nKeyLength+1));
}
/* }}} */

/* {{{ proto int CachingIterator::getFlags()
   Return the internal flags */
SPL_METHOD(CachingIterator, getFlags)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_LONG(intern->u.caching.flags);
}
/* }}} */

/* {{{ proto void CachingIterator::setFlags()
   Set the internal flags */
SPL_METHOD(CachingIterator, setFlags)
{
	spl_dual_it_object   *intern;
	long flags;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", flags) == FAILURE) {
		return;
	}

	if (((flags & CIT_CALL_TOSTRING) && (flags & (CIT_TOSTRING_USE_KEY|CIT_TOSTRING_USE_CURRENT)))
	|| ((flags & (CIT_TOSTRING_USE_KEY|CIT_TOSTRING_USE_CURRENT)) == (CIT_TOSTRING_USE_KEY|CIT_TOSTRING_USE_CURRENT))) {
		zend_throw_exception(spl_ce_InvalidArgumentException , "Flags must contain only one of CIT_CALL_TOSTRING, CIT_TOSTRING_USE_KEY, CIT_TOSTRING_USE_CURRENT", 0 TSRMLS_CC);
		return;
	}
	if ((intern->u.caching.flags & CIT_CALL_TOSTRING) != 0 && (flags & ~CIT_CALL_TOSTRING) == 0) {
		zend_throw_exception(U_CLASS_ENTRY(spl_ce_InvalidArgumentException), "Unsetting flag CALL_TO_STRING is not possible", 0 TSRMLS_CC);
		return;
	}
	if ((flags && CIT_FULL_CACHE) != 0 && (intern->u.caching.flags & CIT_FULL_CACHE) == 0) {
		/* clear on (re)enable */
		zend_hash_clean(HASH_OF(intern->u.caching.zcache));
	}
	intern->u.caching.flags = (intern->u.caching.flags & ~CIT_PUBLIC) | (flags & CIT_PUBLIC);
}
/* }}} */

static
ZEND_BEGIN_ARG_INFO(arginfo_caching_it___construct, 0) 
	ZEND_ARG_OBJ_INFO(0, iterator, Iterator, 0)
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO();

static
ZEND_BEGIN_ARG_INFO(arginfo_caching_it_setFlags, 0) 
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO();

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_caching_it_offsetGet, 0, 0, 1)
	ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO();

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_caching_it_offsetSet, 0, 0, 2)
	ZEND_ARG_INFO(0, index)
	ZEND_ARG_INFO(0, newval)
ZEND_END_ARG_INFO();

static zend_function_entry spl_funcs_CachingIterator[] = {
	SPL_ME(CachingIterator, __construct,      arginfo_caching_it___construct, ZEND_ACC_PUBLIC)
	SPL_ME(CachingIterator, rewind,           NULL, ZEND_ACC_PUBLIC)
	SPL_ME(CachingIterator, valid,            NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,         key,              NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,         current,          NULL, ZEND_ACC_PUBLIC)
	SPL_ME(CachingIterator, next,             NULL, ZEND_ACC_PUBLIC)
	SPL_ME(CachingIterator, hasNext,          NULL, ZEND_ACC_PUBLIC)
	SPL_ME(CachingIterator, __toString,       NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,         getInnerIterator, NULL, ZEND_ACC_PUBLIC)
	SPL_ME(CachingIterator, getFlags,         NULL, ZEND_ACC_PUBLIC)
	SPL_ME(CachingIterator, setFlags,         arginfo_caching_it_setFlags,    ZEND_ACC_PUBLIC)
	SPL_ME(CachingIterator, offsetGet,        arginfo_caching_it_offsetGet,   ZEND_ACC_PUBLIC)
	SPL_ME(CachingIterator, offsetSet,        arginfo_caching_it_offsetSet,   ZEND_ACC_PUBLIC)
	SPL_ME(CachingIterator, offsetUnset,      arginfo_caching_it_offsetGet,   ZEND_ACC_PUBLIC)
	SPL_ME(CachingIterator, offsetExists,     arginfo_caching_it_offsetGet,   ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

/* {{{ proto RecursiveCachingIterator::__construct(RecursiveIterator it [, flags = CIT_CALL_TOSTRING])
   Create an iterator from a RecursiveIterator */
SPL_METHOD(RecursiveCachingIterator, __construct)
{
	spl_dual_it_construct(INTERNAL_FUNCTION_PARAM_PASSTHRU, U_CLASS_ENTRY(spl_ce_RecursiveIterator), DIT_RecursiveCachingIterator);
} /* }}} */

/* {{{ proto bolean RecursiveCachingIterator::hasChildren()
   Check whether the current element of the inner iterator has children */
SPL_METHOD(RecursiveCachingIterator, hasChildren)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_BOOL(intern->u.caching.zchildren);
} /* }}} */

/* {{{ proto RecursiveCachingIterator RecursiveCachingIterator::getChildren()
  Return the inner iterator's children as a RecursiveCachingIterator */
SPL_METHOD(RecursiveCachingIterator, getChildren)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (intern->u.caching.zchildren) {
		RETURN_ZVAL(intern->u.caching.zchildren, 1, 0);
	} else {
		RETURN_NULL();
	}
} /* }}} */

static
ZEND_BEGIN_ARG_INFO_EX(arginfo_caching_rec_it___construct, 0, ZEND_RETURN_VALUE, 2) 
	ZEND_ARG_OBJ_INFO(0, iterator, Iterator, 0)
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO();

static zend_function_entry spl_funcs_RecursiveCachingIterator[] = {
	SPL_ME(RecursiveCachingIterator, __construct,   arginfo_caching_rec_it___construct, ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveCachingIterator, hasChildren,   NULL, ZEND_ACC_PUBLIC)
	SPL_ME(RecursiveCachingIterator, getChildren,   NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

/* {{{ proto IteratorIterator::__construct(Traversable it)
   Create an iterator from anything that is traversable */
SPL_METHOD(IteratorIterator, __construct)
{
	spl_dual_it_construct(INTERNAL_FUNCTION_PARAM_PASSTHRU, U_CLASS_ENTRY(zend_ce_traversable), DIT_IteratorIterator);
} /* }}} */

static
ZEND_BEGIN_ARG_INFO(arginfo_iterator_it___construct, 0) 
	ZEND_ARG_OBJ_INFO(0, iterator, Traversable, 0)
ZEND_END_ARG_INFO();

static zend_function_entry spl_funcs_IteratorIterator[] = {
	SPL_ME(IteratorIterator, __construct,      arginfo_iterator_it___construct, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,          rewind,           NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,          valid,            NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,          key,              NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,          current,          NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,          next,             NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,          getInnerIterator, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

/* {{{ proto NoRewindIterator::__construct(Iterator it)
   Create an iterator from another iterator */
SPL_METHOD(NoRewindIterator, __construct)
{
	spl_dual_it_construct(INTERNAL_FUNCTION_PARAM_PASSTHRU, U_CLASS_ENTRY(zend_ce_iterator), DIT_NoRewindIterator);
} /* }}} */

/* {{{ proto void NoRewindIterator::rewind()
   Prevent a call to inner iterators rewind() */
SPL_METHOD(NoRewindIterator, rewind)
{
	/* nothing to do */
} /* }}} */

/* {{{ proto void NoRewindIterator::valid()
   Return inner iterators valid() */
SPL_METHOD(NoRewindIterator, valid)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	RETURN_BOOL(intern->inner.iterator->funcs->valid(intern->inner.iterator TSRMLS_CC) == SUCCESS);
} /* }}} */

/* {{{ proto mixed NoRewindIterator::key()
   Return inner iterators key() */
SPL_METHOD(NoRewindIterator, key)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (intern->inner.iterator->funcs->get_current_key) {
		char *str_key;
		uint str_key_len;
		ulong int_key;
		switch (intern->inner.iterator->funcs->get_current_key(intern->inner.iterator, &str_key, &str_key_len, &int_key TSRMLS_CC)) {
			case HASH_KEY_IS_LONG:
				RETURN_LONG(int_key);
				break;	
			case HASH_KEY_IS_STRING:
				RETURN_STRINGL(str_key, str_key_len-1, 0);
				break;
			case HASH_KEY_IS_UNICODE:
				RETURN_UNICODEL((void*)str_key, str_key_len-1, 0);
				break;
			default:
				RETURN_NULL();
		}
	} else {
		RETURN_NULL();
	}
} /* }}} */

/* {{{ proto mixed NoRewindIterator::current()
   Return inner iterators current() */
SPL_METHOD(NoRewindIterator, current)
{
	spl_dual_it_object   *intern;
	zval **data;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	intern->inner.iterator->funcs->get_current_data(intern->inner.iterator, &data TSRMLS_CC);
	RETURN_ZVAL(*data, 1, 0);
} /* }}} */

/* {{{ proto void NoRewindIterator::next()
   Return inner iterators next() */
SPL_METHOD(NoRewindIterator, next)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	intern->inner.iterator->funcs->move_forward(intern->inner.iterator TSRMLS_CC);
} /* }}} */

static
ZEND_BEGIN_ARG_INFO(arginfo_norewind_it___construct, 0) 
	ZEND_ARG_OBJ_INFO(0, iterator, Iterator, 0)
ZEND_END_ARG_INFO();

static zend_function_entry spl_funcs_NoRewindIterator[] = {
	SPL_ME(NoRewindIterator, __construct,      arginfo_norewind_it___construct, ZEND_ACC_PUBLIC)
	SPL_ME(NoRewindIterator, rewind,           NULL, ZEND_ACC_PUBLIC)
	SPL_ME(NoRewindIterator, valid,            NULL, ZEND_ACC_PUBLIC)
	SPL_ME(NoRewindIterator, key,              NULL, ZEND_ACC_PUBLIC)
	SPL_ME(NoRewindIterator, current,          NULL, ZEND_ACC_PUBLIC)
	SPL_ME(NoRewindIterator, next,             NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,          getInnerIterator, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

/* {{{ proto InfiniteIterator::__construct(Iterator it)
   Create an iterator from another iterator */
SPL_METHOD(InfiniteIterator, __construct)
{
	spl_dual_it_construct(INTERNAL_FUNCTION_PARAM_PASSTHRU, U_CLASS_ENTRY(zend_ce_iterator), DIT_InfiniteIterator);
} /* }}} */

/* {{{ proto InfiniteIterator::next()
   Prevent a call to inner iterators rewind() (internally the current data will be fetched if valid()) */
SPL_METHOD(InfiniteIterator, next)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	spl_dual_it_next(intern, 1 TSRMLS_CC);
	if (spl_dual_it_valid(intern TSRMLS_CC) == SUCCESS) {
		spl_dual_it_fetch(intern, 0 TSRMLS_CC);
	} else {
		spl_dual_it_rewind(intern TSRMLS_CC);
		if (spl_dual_it_valid(intern TSRMLS_CC) == SUCCESS) {
			spl_dual_it_fetch(intern, 0 TSRMLS_CC);
		}
	}
} /* }}} */

static zend_function_entry spl_funcs_InfiniteIterator[] = {
	SPL_ME(InfiniteIterator, __construct,      arginfo_norewind_it___construct, ZEND_ACC_PUBLIC)
	SPL_ME(InfiniteIterator, next,             NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

/* {{{ proto EmptyIterator::rewind()
   Does nothing  */
SPL_METHOD(EmptyIterator, rewind)
{
} /* }}} */

/* {{{ proto EmptyIterator::valid()
   Return false */
SPL_METHOD(EmptyIterator, valid)
{
	RETURN_FALSE;
} /* }}} */

/* {{{ proto EmptyIterator::key()
   Throws exception */
SPL_METHOD(EmptyIterator, key)
{
	zend_throw_exception(U_CLASS_ENTRY(spl_ce_BadMethodCallException), "Accessing the key of an EmptyIterator", 0 TSRMLS_CC);
} /* }}} */

/* {{{ proto EmptyIterator::current()
   Throws exception */
SPL_METHOD(EmptyIterator, current)
{
	zend_throw_exception(U_CLASS_ENTRY(spl_ce_BadMethodCallException), "Accessing the value of an EmptyIterator", 0 TSRMLS_CC);
} /* }}} */

/* {{{ proto EmptyIterator::next()
   Does nothing */
SPL_METHOD(EmptyIterator, next)
{
} /* }}} */

static zend_function_entry spl_funcs_EmptyIterator[] = {
	SPL_ME(EmptyIterator, rewind,           NULL, ZEND_ACC_PUBLIC)
	SPL_ME(EmptyIterator, valid,            NULL, ZEND_ACC_PUBLIC)
	SPL_ME(EmptyIterator, key,              NULL, ZEND_ACC_PUBLIC)
	SPL_ME(EmptyIterator, current,          NULL, ZEND_ACC_PUBLIC)
	SPL_ME(EmptyIterator, next,             NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

int spl_append_it_next_iterator(spl_dual_it_object *intern TSRMLS_DC) /* {{{*/
{
	spl_dual_it_free(intern TSRMLS_CC);

	if (intern->inner.zobject) {
		zval_ptr_dtor(&intern->inner.zobject);
		intern->inner.zobject = NULL;
		intern->inner.ce = NULL;
		intern->inner.object = NULL;
		if (intern->inner.iterator) {
			intern->inner.iterator->funcs->dtor(intern->inner.iterator TSRMLS_CC);
			intern->inner.iterator = NULL;
		}
	}
	if (intern->u.append.iterator->funcs->valid(intern->u.append.iterator TSRMLS_CC) == SUCCESS) {
		zval **it;

		intern->u.append.iterator->funcs->get_current_data(intern->u.append.iterator, &it TSRMLS_CC);
		(*it)->refcount++;
		intern->inner.zobject = *it;
		intern->inner.ce = Z_OBJCE_PP(it);
		intern->inner.object = zend_object_store_get_object(*it TSRMLS_CC);
		intern->inner.iterator = intern->inner.ce->get_iterator(intern->inner.ce, *it TSRMLS_CC);
		spl_dual_it_rewind(intern TSRMLS_CC);
		return SUCCESS;
	} else {
		return FAILURE;
	}
} /* }}} */

void spl_append_it_fetch(spl_dual_it_object *intern TSRMLS_DC) /* {{{*/
{
	while (spl_dual_it_valid(intern TSRMLS_CC) != SUCCESS) {
		intern->u.append.iterator->funcs->move_forward(intern->u.append.iterator TSRMLS_CC);
		if (spl_append_it_next_iterator(intern TSRMLS_CC) != SUCCESS) {
			return;
		}
	}
	spl_dual_it_fetch(intern, 0 TSRMLS_CC);
} /* }}} */

void spl_append_it_next(spl_dual_it_object *intern TSRMLS_DC) /* {{{ */
{
	if (spl_dual_it_valid(intern TSRMLS_CC) == SUCCESS) {
		spl_dual_it_next(intern, 1 TSRMLS_CC);
	}
	spl_append_it_fetch(intern TSRMLS_CC);
} /* }}} */

/* {{{ proto AppendIterator::__construct()
   Create an AppendIterator */
SPL_METHOD(AppendIterator, __construct)
{
	spl_dual_it_construct(INTERNAL_FUNCTION_PARAM_PASSTHRU, U_CLASS_ENTRY(zend_ce_iterator), DIT_AppendIterator);
} /* }}} */

/* {{{ proto void AppendIterator::append(Iterator it)
   Append an iterator */
SPL_METHOD(AppendIterator, append)
{
	spl_dual_it_object   *intern;
	zval *it;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &it, U_CLASS_ENTRY(zend_ce_iterator)) == FAILURE) {
		return;
	}
	spl_array_iterator_append(intern->u.append.zarrayit, it TSRMLS_CC);

	if (!intern->inner.iterator || spl_dual_it_valid(intern TSRMLS_CC) != SUCCESS) {
		if (intern->u.append.iterator->funcs->valid(intern->u.append.iterator TSRMLS_CC) != SUCCESS) {
			intern->u.append.iterator->funcs->rewind(intern->u.append.iterator TSRMLS_CC);
		}
		do {
			spl_append_it_next_iterator(intern TSRMLS_CC);
		} while (intern->inner.zobject != it);
		spl_append_it_fetch(intern TSRMLS_CC);
	}
} /* }}} */

/* {{{ proto void AppendIterator::rewind()
   Rewind to the first iterator and rewind the first iterator, too */
SPL_METHOD(AppendIterator, rewind)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	intern->u.append.iterator->funcs->rewind(intern->u.append.iterator TSRMLS_CC);
	if (spl_append_it_next_iterator(intern TSRMLS_CC) == SUCCESS) {
		spl_append_it_fetch(intern TSRMLS_CC);
	}
} /* }}} */

/* {{{ proto boolean AppendIterator::valid()
   Check if the current state is valid */
SPL_METHOD(AppendIterator, valid)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_BOOL(intern->current.data);
} /* }}} */

/* {{{ proto AppendIterator::next()
   Forward to next element */
SPL_METHOD(AppendIterator, next)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	spl_append_it_next(intern TSRMLS_CC);
} /* }}} */

/* {{{ proto AppendIterator::getIteratorIndex()
   Get index of iterator */
SPL_METHOD(AppendIterator, getIteratorIndex)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	spl_array_iterator_key(intern->u.append.zarrayit, return_value TSRMLS_CC);
} /* }}} */

/* {{{ proto AppendIterator::getArrayIterator()
   Get access to inner ArrayIterator */
SPL_METHOD(AppendIterator, getArrayIterator)
{
	spl_dual_it_object   *intern;

	intern = (spl_dual_it_object*)zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_ZVAL(intern->u.append.zarrayit, 1, 0);
} /* }}} */

static
ZEND_BEGIN_ARG_INFO(arginfo_append_it_append, 0) 
	ZEND_ARG_OBJ_INFO(0, iterator, Iterator, 0)
ZEND_END_ARG_INFO();

static zend_function_entry spl_funcs_AppendIterator[] = {
	SPL_ME(AppendIterator, __construct,      NULL, ZEND_ACC_PUBLIC)
	SPL_ME(AppendIterator, append,           arginfo_append_it_append, ZEND_ACC_PUBLIC)
	SPL_ME(AppendIterator, rewind,           NULL, ZEND_ACC_PUBLIC)
	SPL_ME(AppendIterator, valid,            NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,        key,              NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,        current,          NULL, ZEND_ACC_PUBLIC)
	SPL_ME(AppendIterator, next,             NULL, ZEND_ACC_PUBLIC)
	SPL_ME(dual_it,        getInnerIterator, NULL, ZEND_ACC_PUBLIC)
	SPL_ME(AppendIterator, getIteratorIndex, NULL, ZEND_ACC_PUBLIC)
	SPL_ME(AppendIterator, getArrayIterator, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

/* {{{ array iterator_to_array(Traversable it) 
   Copy the iterator into an array */
PHP_FUNCTION(iterator_to_array)
{
	zval                   *obj, **data;
	zend_object_iterator   *iter;
	char                   *str_key;
	uint                    str_key_len;
	ulong                   int_key;
	int                     key_type;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &obj, U_CLASS_ENTRY(zend_ce_traversable)) == FAILURE) {
		RETURN_FALSE;
	}
	
	array_init(return_value);
	
	iter = Z_OBJCE_P(obj)->get_iterator(Z_OBJCE_P(obj), obj TSRMLS_CC);

	if (iter->funcs->rewind) {
		iter->funcs->rewind(iter TSRMLS_CC);
	}
	while (iter->funcs->valid(iter TSRMLS_CC) == SUCCESS) {
		iter->funcs->get_current_data(iter, &data TSRMLS_CC);
		(*data)->refcount++;
		if (iter->funcs->get_current_key) {
			key_type = iter->funcs->get_current_key(iter, &str_key, &str_key_len, &int_key TSRMLS_CC);
			switch(key_type) {
				case HASH_KEY_IS_STRING:
					add_assoc_zval_ex(return_value, str_key, str_key_len, *data);
					efree(str_key);
					break;
				case HASH_KEY_IS_UNICODE:
					add_u_assoc_zval_ex(return_value, IS_UNICODE, str_key, str_key_len, *data);
					efree(str_key);
					break;
				case HASH_KEY_IS_LONG:
					add_index_zval(return_value, int_key, *data);
					break;
			}
		} else {
			add_next_index_zval(return_value, *data);
		}
		iter->funcs->move_forward(iter TSRMLS_CC);
	}
	iter->funcs->dtor(iter TSRMLS_CC);
}
/* }}} */

/* {{{ int iterator_count(Traversable it) 
   Count the elements in an iterator */
PHP_FUNCTION(iterator_count)
{
	zval                   *obj;
	zend_object_iterator   *iter;
	long                    count = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &obj, U_CLASS_ENTRY(zend_ce_traversable)) == FAILURE) {
		RETURN_FALSE;
	}
	
	iter = Z_OBJCE_P(obj)->get_iterator(Z_OBJCE_P(obj), obj TSRMLS_CC);

	if (iter->funcs->rewind) {
		iter->funcs->rewind(iter TSRMLS_CC);
	}
	while (iter->funcs->valid(iter TSRMLS_CC) == SUCCESS) {
		count++;
		iter->funcs->move_forward(iter TSRMLS_CC);
	}
	iter->funcs->dtor(iter TSRMLS_CC);
	
	RETURN_LONG(count);
}
/* }}} */

static zend_function_entry spl_funcs_OuterIterator[] = {
	SPL_ABSTRACT_ME(OuterIterator, getInnerIterator,   NULL)
	{NULL, NULL, NULL}
};

/* {{{ PHP_MINIT_FUNCTION(spl_iterators)
 */
PHP_MINIT_FUNCTION(spl_iterators)
{
	REGISTER_SPL_INTERFACE(RecursiveIterator);
	REGISTER_SPL_ITERATOR(RecursiveIterator);

	REGISTER_SPL_STD_CLASS_EX(RecursiveIteratorIterator, spl_RecursiveIteratorIterator_new, spl_funcs_RecursiveIteratorIterator);
	REGISTER_SPL_ITERATOR(RecursiveIteratorIterator);

	memcpy(&spl_handlers_rec_it_it, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	spl_handlers_rec_it_it.get_method = spl_recursive_it_get_method;
	spl_handlers_rec_it_it.clone_obj = NULL;

	memcpy(&spl_handlers_dual_it, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	spl_handlers_dual_it.get_method = spl_dual_it_get_method;
	/*spl_handlers_dual_it.call_method = spl_dual_it_call_method;*/
	spl_handlers_dual_it.clone_obj = NULL;
	
	spl_ce_RecursiveIteratorIterator->get_iterator = spl_recursive_it_get_iterator;
	spl_ce_RecursiveIteratorIterator->iterator_funcs.funcs = &spl_recursive_it_iterator_funcs;

	REGISTER_SPL_CLASS_CONST_LONG(RecursiveIteratorIterator, "LEAVES_ONLY",     RIT_LEAVES_ONLY);
	REGISTER_SPL_CLASS_CONST_LONG(RecursiveIteratorIterator, "SELF_FIRST",      RIT_SELF_FIRST);
	REGISTER_SPL_CLASS_CONST_LONG(RecursiveIteratorIterator, "CHILD_FIRST",     RIT_CHILD_FIRST);
	REGISTER_SPL_CLASS_CONST_LONG(RecursiveIteratorIterator, "CATCH_GET_CHILD", RIT_CATCH_GET_CHILD);

	REGISTER_SPL_STD_CLASS_EX(FilterIterator, spl_dual_it_new, spl_funcs_FilterIterator);
	REGISTER_SPL_ITERATOR(FilterIterator);
	spl_ce_FilterIterator->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	REGISTER_SPL_SUB_CLASS_EX(RecursiveFilterIterator, FilterIterator, spl_dual_it_new, spl_funcs_RecursiveFilterIterator);
	REGISTER_SPL_IMPLEMENTS(RecursiveFilterIterator, RecursiveIterator);

	REGISTER_SPL_SUB_CLASS_EX(ParentIterator, RecursiveFilterIterator, spl_dual_it_new, spl_funcs_ParentIterator);

	REGISTER_SPL_INTERFACE(SeekableIterator);
	REGISTER_SPL_ITERATOR(SeekableIterator);

	REGISTER_SPL_STD_CLASS_EX(LimitIterator, spl_dual_it_new, spl_funcs_LimitIterator);
	REGISTER_SPL_ITERATOR(LimitIterator);

	REGISTER_SPL_STD_CLASS_EX(CachingIterator, spl_dual_it_new, spl_funcs_CachingIterator);
	REGISTER_SPL_ITERATOR(CachingIterator);
	REGISTER_SPL_IMPLEMENTS(CachingIterator, ArrayAccess);

	REGISTER_SPL_CLASS_CONST_LONG(CachingIterator, "CALL_TOSTRING",        CIT_CALL_TOSTRING); 
	REGISTER_SPL_CLASS_CONST_LONG(CachingIterator, "CATCH_GET_CHILD",      CIT_CATCH_GET_CHILD); 
	REGISTER_SPL_CLASS_CONST_LONG(CachingIterator, "TOSTRING_USE_KEY",     CIT_TOSTRING_USE_KEY);
	REGISTER_SPL_CLASS_CONST_LONG(CachingIterator, "TOSTRING_USE_CURRENT", CIT_TOSTRING_USE_CURRENT);
	REGISTER_SPL_CLASS_CONST_LONG(CachingIterator, "FULL_CACHE",           CIT_FULL_CACHE); 

	REGISTER_SPL_SUB_CLASS_EX(RecursiveCachingIterator, CachingIterator, spl_dual_it_new, spl_funcs_RecursiveCachingIterator);
	REGISTER_SPL_IMPLEMENTS(RecursiveCachingIterator, RecursiveIterator);
	
	REGISTER_SPL_STD_CLASS_EX(IteratorIterator, spl_dual_it_new, spl_funcs_IteratorIterator);
	REGISTER_SPL_ITERATOR(IteratorIterator);

	REGISTER_SPL_STD_CLASS_EX(NoRewindIterator, spl_dual_it_new, spl_funcs_NoRewindIterator);
	REGISTER_SPL_ITERATOR(NoRewindIterator);

	REGISTER_SPL_INTERFACE(OuterIterator);
	REGISTER_SPL_ITERATOR(OuterIterator);

	REGISTER_SPL_STD_CLASS_EX(AppendIterator, spl_dual_it_new, spl_funcs_AppendIterator);

	REGISTER_SPL_IMPLEMENTS(RecursiveIteratorIterator, OuterIterator);
	REGISTER_SPL_IMPLEMENTS(CachingIterator, OuterIterator);
	REGISTER_SPL_IMPLEMENTS(FilterIterator, OuterIterator);
	REGISTER_SPL_IMPLEMENTS(LimitIterator, OuterIterator);
	REGISTER_SPL_IMPLEMENTS(IteratorIterator, OuterIterator);
	REGISTER_SPL_IMPLEMENTS(NoRewindIterator, OuterIterator);
	REGISTER_SPL_IMPLEMENTS(AppendIterator, OuterIterator);

	REGISTER_SPL_SUB_CLASS_EX(InfiniteIterator, IteratorIterator, spl_dual_it_new, spl_funcs_InfiniteIterator);
#if HAVE_PCRE || HAVE_BUNDLED_PCRE
	REGISTER_SPL_SUB_CLASS_EX(RegExIterator, FilterIterator, spl_dual_it_new, spl_funcs_RegExIterator);
	REGISTER_SPL_CLASS_CONST_LONG(RegExIterator, "USE_KEY", REGIT_USE_KEY);
	REGISTER_SPL_SUB_CLASS_EX(RecursiveRegExIterator, RegExIterator, spl_dual_it_new, spl_funcs_RecursiveRegExIterator);
	REGISTER_SPL_IMPLEMENTS(RecursiveRegExIterator, RecursiveIterator);
#else
	spl_ce_RegExIterator = NULL;
	spl_ce_RecursiveRegExIterator = NULL;
#endif

	REGISTER_SPL_STD_CLASS_EX(EmptyIterator, NULL, spl_funcs_EmptyIterator);
	REGISTER_SPL_ITERATOR(EmptyIterator);

	return SUCCESS;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
