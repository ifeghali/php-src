/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2007 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: David Wang <planetbeing@gmail.com>                          |
   |          Dmitry Stogov <dmitry@zend.com>                             |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "zend.h"
#include "zend_API.h"

#define GC_ROOT_BUFFER_MAX_ENTRIES 10000

#ifdef ZTS
ZEND_API int gc_globals_id;
#else
ZEND_API zend_gc_globals gc_globals;
#endif

/* Forward declarations */
static int children_scan_black(zval **pz TSRMLS_DC);
static int children_mark_grey(zval **pz TSRMLS_DC);
static int children_collect_white(zval **pz TSRMLS_DC);
static int children_scan(zval **pz TSRMLS_DC);

static void root_buffer_dtor(zend_gc_globals *gc_globals TSRMLS_DC)
{
	if (gc_globals->buf) {
		free(gc_globals->buf);
		gc_globals->buf = NULL;
	}	
}

static void gc_globals_ctor_ex(zend_gc_globals *gc_globals TSRMLS_DC)
{
	gc_globals->gc_enabled = 0;

	gc_globals->buf = NULL;

	gc_globals->roots.next = NULL;
	gc_globals->roots.prev = NULL;
	gc_globals->unused = NULL;
	gc_globals->zval_to_free = NULL;

	gc_globals->gc_runs = 0;
	gc_globals->collected = 0;

#if GC_BENCH
	gc_globals->root_buf_length = 0;
	gc_globals->root_buf_peak = 0;
	gc_globals->zval_possible_root = 0;
	gc_globals->zobj_possible_root = 0;
	gc_globals->zval_buffered = 0;
	gc_globals->zobj_buffered = 0;
	gc_globals->zval_remove_from_buffer = 0;
	gc_globals->zobj_remove_from_buffer = 0;
	gc_globals->zval_marked_grey = 0;
	gc_globals->zobj_marked_grey = 0;
#endif
}

ZEND_API void gc_globals_ctor(TSRMLS_D)
{
#ifdef ZTS
	ts_allocate_id(&gc_globals_id, sizeof(zend_gc_globals), (ts_allocate_ctor) gc_globals_ctor_ex, (ts_allocate_dtor) root_buffer_dtor);
#else
	gc_globals_ctor_ex(&gc_globals);
#endif
}

ZEND_API void gc_globals_dtor(TSRMLS_D)
{
#ifndef ZTS
	root_buffer_dtor(&gc_globals TSRMLS_DC);
#endif
}

ZEND_API void gc_reset(TSRMLS_D)
{
	int i;

	GC_G(gc_runs) = 0;
	GC_G(collected) = 0;

#if GC_BENCH
	GC_G(root_buf_length) = 0;
	GC_G(root_buf_peak) = 0;
	GC_G(zval_possible_root) = 0;
	GC_G(zobj_possible_root) = 0;
	GC_G(zval_buffered) = 0;
	GC_G(zobj_buffered) = 0;
	GC_G(zval_remove_from_buffer) = 0;
	GC_G(zobj_remove_from_buffer) = 0;
	GC_G(zval_marked_grey) = 0;
	GC_G(zobj_marked_grey) = 0;
#endif

	if (GC_G(buf) &&
	    (GC_G(roots).next != &GC_G(roots) ||
	     GC_G(roots).prev != &GC_G(roots))) {

		GC_G(roots).next = &GC_G(roots);
		GC_G(roots).prev = &GC_G(roots);

		GC_G(unused) = &GC_G(buf)[0];
		for (i = 0; i < GC_ROOT_BUFFER_MAX_ENTRIES-1; i++) {
			GC_G(buf)[i].prev = &GC_G(buf)[i+1];
		}
		GC_G(buf)[GC_ROOT_BUFFER_MAX_ENTRIES-1].prev = NULL;

		GC_G(zval_to_free) = NULL;
	}
}

ZEND_API void gc_init(TSRMLS_D)
{
	if (GC_G(buf) == NULL && GC_G(gc_enabled)) {
		GC_G(buf) = (gc_root_buffer*) malloc(sizeof(gc_root_buffer) * GC_ROOT_BUFFER_MAX_ENTRIES);
		gc_reset(TSRMLS_C);
	}
}

ZEND_API void gc_zval_possible_root(zval *zv TSRMLS_DC)
{
	if (zv->type == IS_OBJECT) {
		GC_ZOBJ_CHECK_POSSIBLE_ROOT(zv);
		return;
	}

	GC_BENCH_INC(zval_possible_root);

	if (GC_ZVAL_GET_COLOR(zv) != GC_PURPLE) {
		GC_ZVAL_SET_PURPLE(zv);

		if (!GC_ZVAL_ADDRESS(zv)) {
			gc_root_buffer *newRoot = GC_G(unused);

			if (!newRoot) {
				if (!GC_G(gc_enabled)) {
					GC_ZVAL_SET_BLACK(zv);
					return;
				}
				zv->refcount__gc++;
				gc_collect_cycles(TSRMLS_C);
				zv->refcount__gc--;
				GC_ZVAL_SET_PURPLE(zv);
				newRoot = GC_G(unused);
			}

			GC_G(unused) = newRoot->prev;

			newRoot->next = GC_G(roots).next;
			newRoot->prev = &GC_G(roots);
			GC_G(roots).next->prev = newRoot;
			GC_G(roots).next = newRoot;

			GC_ZVAL_SET_ADDRESS(zv, newRoot);

			newRoot->handle = 0;
			newRoot->u.pz = zv;

			GC_BENCH_INC(zval_buffered);
			GC_BENCH_INC(root_buf_length);
			GC_BENCH_PEAK(root_buf_peak, root_buf_length);
		}
	}
}

ZEND_API void gc_zobj_possible_root(zval *zv TSRMLS_DC)
{
	struct _store_object *obj;

	if (UNEXPECTED(Z_OBJ_HT_P(zv)->get_properties == NULL)) {
		return;
	}

	GC_BENCH_INC(zobj_possible_root);

	obj = &EG(objects_store).object_buckets[Z_OBJ_HANDLE_P(zv)].bucket.obj;
	if (GC_GET_COLOR(obj->buffered) != GC_PURPLE) {
		GC_SET_PURPLE(obj->buffered);
		if (!GC_ADDRESS(obj->buffered)) {
			gc_root_buffer *newRoot = GC_G(unused);

			if (!newRoot) {
				if (!GC_G(gc_enabled)) {
					GC_ZVAL_SET_BLACK(zv);
					return;
				}
				zv->refcount__gc++;
				gc_collect_cycles(TSRMLS_C);
				zv->refcount__gc--;
				obj = &EG(objects_store).object_buckets[Z_OBJ_HANDLE_P(zv)].bucket.obj;
				GC_SET_PURPLE(obj->buffered);
				newRoot = GC_G(unused);
			}

			GC_G(unused) = newRoot->prev;

			newRoot->next = GC_G(roots).next;
			newRoot->prev = &GC_G(roots);
			GC_G(roots).next->prev = newRoot;
			GC_G(roots).next = newRoot;

			GC_SET_ADDRESS(obj->buffered, newRoot);

			newRoot->handle = Z_OBJ_HANDLE_P(zv);
			newRoot->u.handlers = Z_OBJ_HT_P(zv);

			GC_BENCH_INC(zobj_buffered);
			GC_BENCH_INC(root_buf_length);
			GC_BENCH_PEAK(root_buf_peak, root_buf_length);
		}
	}
}

static void zobj_scan_black(struct _store_object *obj, zval *pz TSRMLS_DC)
{
	GC_SET_BLACK(obj->buffered);

	if (EXPECTED(Z_OBJ_HANDLER_P(pz, get_properties) != NULL)) {
		zend_hash_apply(Z_OBJPROP_P(pz), (apply_func_t) children_scan_black TSRMLS_CC);
	}
}

static void zval_scan_black(zval *pz TSRMLS_DC)
{
	GC_ZVAL_SET_BLACK(pz);

	if (Z_TYPE_P(pz) == IS_OBJECT) {
		struct _store_object *obj = &EG(objects_store).object_buckets[Z_OBJ_HANDLE_P(pz)].bucket.obj;

		obj->refcount++;
		if (GC_GET_COLOR(obj->buffered) != GC_BLACK) {
			zobj_scan_black(obj, pz TSRMLS_CC);
		}
	} else if (Z_TYPE_P(pz) == IS_ARRAY) {
		if (Z_ARRVAL_P(pz) != &EG(symbol_table)) {
			zend_hash_apply(Z_ARRVAL_P(pz), (apply_func_t) children_scan_black TSRMLS_CC);
		}
	}
}

static int children_scan_black(zval **pz TSRMLS_DC)
{
	(*pz)->refcount__gc++;

	if (GC_ZVAL_GET_COLOR(*pz) != GC_BLACK) {
		zval_scan_black(*pz TSRMLS_CC);
	}

	return 0;
}

static void zobj_mark_grey(struct _store_object *obj, zval *pz TSRMLS_DC)
{
	if (GC_GET_COLOR(obj->buffered) != GC_GREY) {
		GC_BENCH_INC(zobj_marked_grey);
		GC_SET_COLOR(obj->buffered, GC_GREY);
		if (EXPECTED(Z_OBJ_HANDLER_P(pz, get_properties) != NULL)) {
			zend_hash_apply(Z_OBJPROP_P(pz), (apply_func_t) children_mark_grey TSRMLS_CC);
		}
	}
}

static void zval_mark_grey(zval *pz TSRMLS_DC)
{
	if (GC_ZVAL_GET_COLOR(pz) != GC_GREY) {
		GC_BENCH_INC(zval_marked_grey);
		GC_ZVAL_SET_COLOR(pz, GC_GREY);

		if (Z_TYPE_P(pz) == IS_OBJECT) {
			struct _store_object *obj = &EG(objects_store).object_buckets[Z_OBJ_HANDLE_P(pz)].bucket.obj;

			obj->refcount--;
			zobj_mark_grey(obj, pz TSRMLS_CC);
		} else if (Z_TYPE_P(pz) == IS_ARRAY) {
			if (Z_ARRVAL_P(pz) == &EG(symbol_table)) {
				GC_ZVAL_SET_BLACK(pz);
			} else {
				zend_hash_apply(Z_ARRVAL_P(pz), (apply_func_t) children_mark_grey TSRMLS_CC);
			}
		}
	}
}

static int children_mark_grey(zval **pz TSRMLS_DC)
{
	(*pz)->refcount__gc--;
	zval_mark_grey(*pz TSRMLS_CC);
	return 0;
}

static void gc_mark_roots(TSRMLS_D)
{
	gc_root_buffer *current = GC_G(roots).next;

	while (current != &GC_G(roots)) {
		if (current->handle) {
			struct _store_object *obj = &EG(objects_store).object_buckets[current->handle].bucket.obj;

			if (GC_GET_COLOR(obj->buffered) == GC_PURPLE) {
				zval z;

				INIT_PZVAL(&z);
				Z_OBJ_HANDLE(z) = current->handle;
				Z_OBJ_HT(z) = current->u.handlers;
				zobj_mark_grey(obj, &z TSRMLS_CC);
			} else {
				GC_SET_ADDRESS(obj->buffered, NULL);
				GC_REMOVE_FROM_BUFFER(current);
			}
		} else {
			if (GC_ZVAL_GET_COLOR(current->u.pz) == GC_PURPLE) {
				zval_mark_grey(current->u.pz TSRMLS_CC);
			} else {
				GC_ZVAL_SET_ADDRESS(current->u.pz, NULL);
				GC_REMOVE_FROM_BUFFER(current);
			}
		}
		current = current->next;
	}
}

static void zobj_scan(zval *pz TSRMLS_DC)
{
	struct _store_object *obj = &EG(objects_store).object_buckets[Z_OBJ_HANDLE_P(pz)].bucket.obj;

	if (GC_GET_COLOR(obj->buffered) == GC_GREY) {
		if (obj->refcount > 0) {
			zobj_scan_black(obj, pz TSRMLS_CC);
		} else {
			GC_SET_COLOR(obj->buffered, GC_WHITE);
			if (EXPECTED(Z_OBJ_HANDLER_P(pz, get_properties) != NULL)) {
				zend_hash_apply(Z_OBJPROP_P(pz), (apply_func_t) children_scan TSRMLS_CC);
			}
		}
	}
}

static int zval_scan(zval *pz TSRMLS_DC)
{
	if (GC_ZVAL_GET_COLOR(pz) == GC_GREY) {
		if (pz->refcount__gc > 0) {
			zval_scan_black(pz TSRMLS_CC);
		} else {
			GC_ZVAL_SET_COLOR(pz, GC_WHITE);

			if (Z_TYPE_P(pz) == IS_OBJECT) {
				zobj_scan(pz TSRMLS_CC);
			} else if (Z_TYPE_P(pz) == IS_ARRAY) {
				if (Z_ARRVAL_P(pz) == &EG(symbol_table)) {
					GC_ZVAL_SET_BLACK(pz);
				} else {
					zend_hash_apply(Z_ARRVAL_P(pz), (apply_func_t) children_scan TSRMLS_CC);
				}
			}
		}
	}
	return 0;
}

static int children_scan(zval **pz TSRMLS_DC)
{
	zval_scan(*pz TSRMLS_CC);
	return 0;
}

static void gc_scan_roots(TSRMLS_D)
{
	gc_root_buffer *current = GC_G(roots).next;

	while (current != &GC_G(roots)) {
		if (current->handle) {
			zval z;

			INIT_PZVAL(&z);
			Z_OBJ_HANDLE(z) = current->handle;
			Z_OBJ_HT(z) = current->u.handlers;
			zobj_scan(&z TSRMLS_CC);
		} else {
			zval_scan(current->u.pz TSRMLS_CC);
		}
		current = current->next;
	}
}

static void zobj_collect_white(zval *pz TSRMLS_DC)
{
	zend_object_handle handle = Z_OBJ_HANDLE_P(pz);
	struct _store_object *obj = &EG(objects_store).object_buckets[handle].bucket.obj;

	if (obj->buffered == (gc_root_buffer*)GC_WHITE) {
		GC_SET_BLACK(obj->buffered);

		if (EXPECTED(Z_OBJ_HANDLER_P(pz, get_properties) != NULL)) {
			zend_hash_apply(Z_OBJPROP_P(pz), (apply_func_t) children_collect_white TSRMLS_CC);
		}
	}
}

static void zval_collect_white(zval *pz TSRMLS_DC)
{
	if (((zval_gc_info*)(pz))->u.buffered == (gc_root_buffer*)GC_WHITE) {
		GC_ZVAL_SET_BLACK(pz);

		if (Z_TYPE_P(pz) == IS_OBJECT) {
			zobj_collect_white(pz TSRMLS_CC);
		} else {
			if (Z_TYPE_P(pz) == IS_ARRAY) {
				if (Z_ARRVAL_P(pz) == &EG(symbol_table)) {
					return;
				}
				zend_hash_apply(Z_ARRVAL_P(pz), (apply_func_t) children_collect_white TSRMLS_CC);
			}
			/* restore refcount */
			pz->refcount__gc++;
		}

		((zval_gc_info*)pz)->u.next = GC_G(zval_to_free);
		GC_G(zval_to_free) = (zval_gc_info*)pz;
	}
}

static int children_collect_white(zval **pz TSRMLS_DC)
{
	zval_collect_white(*pz TSRMLS_CC);
	return 0;
}

static void gc_collect_roots(TSRMLS_D)
{
	gc_root_buffer *current = GC_G(roots).next;

	while (current != &GC_G(roots)) {
		if (current->handle) {
			struct _store_object *obj = &EG(objects_store).object_buckets[current->handle].bucket.obj;
			zval z;

			GC_SET_ADDRESS(obj->buffered, NULL);
			INIT_PZVAL(&z);
			Z_OBJ_HANDLE(z) = current->handle;
			Z_OBJ_HT(z) = current->u.handlers;
			zobj_collect_white(&z TSRMLS_CC);
		} else {
			GC_ZVAL_SET_ADDRESS(current->u.pz, NULL);
			zval_collect_white(current->u.pz TSRMLS_CC);
		}

		GC_REMOVE_FROM_BUFFER(current);
		current = current->next;
	}
}

ZEND_API int gc_collect_cycles(TSRMLS_D)
{
	int count = 0;

	if (GC_G(roots).next != &GC_G(roots)) {
		zval_gc_info *p, *q;

		GC_G(gc_runs)++;
		GC_G(zval_to_free) = NULL;
		gc_mark_roots(TSRMLS_C);
		gc_scan_roots(TSRMLS_C);
		gc_collect_roots(TSRMLS_C);

		p = GC_G(zval_to_free);
		GC_G(zval_to_free) = NULL;
		while (p) {
			q = p->u.next;
			if (Z_TYPE(p->z) == IS_OBJECT) {
				if (EG(objects_store).object_buckets &&
					EG(objects_store).object_buckets[Z_OBJ_HANDLE(p->z)].valid &&
					EG(objects_store).object_buckets[Z_OBJ_HANDLE(p->z)].bucket.obj.refcount <= 1) {
					if (EXPECTED(Z_OBJ_HANDLER(p->z, get_properties) != NULL)) {
						Z_OBJPROP(p->z)->pDestructor = NULL;
					}
					EG(objects_store).object_buckets[Z_OBJ_HANDLE(p->z)].bucket.obj.refcount = 1;
					zend_objects_store_del_ref_by_handle(Z_OBJ_HANDLE(p->z) TSRMLS_CC);
				}
			} else {
				if (Z_TYPE(p->z) == IS_ARRAY) {
					Z_ARRVAL(p->z)->pDestructor = NULL;
				}
				zval_dtor(&p->z);
			}
			FREE_ZVAL_EX(&p->z);
			p = q;
			count++;
		}
		GC_G(collected) += count;
	}

	return count;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */