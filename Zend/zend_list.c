/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2000 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 0.92 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available at through the world-wide-web at                           |
   | http://www.zend.com/license/0_92.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


/* resource lists */

#include "zend.h"
#include "zend_list.h"
#include "zend_API.h"
#include "zend_globals.h"

ZEND_API int le_index_ptr;

/* true global */
static HashTable list_destructors;


static inline int zend_list_do_insert(HashTable *list, void *ptr, int type, zend_bool valid)
{
	int index;
	zend_rsrc_list_entry le;

	index = zend_hash_next_free_element(list);

	if (index==0) index++;

	le.ptr=ptr;
	le.type=type;
	le.refcount=1;
	le.valid = valid;
	zend_hash_index_update(list, index, (void *) &le, sizeof(zend_rsrc_list_entry), NULL);
	return index;
}

static inline int zend_list_do_delete(HashTable *list,int id)
{
	zend_rsrc_list_entry *le;
	ELS_FETCH();
	
	if (zend_hash_index_find(&EG(regular_list), id, (void **) &le)==SUCCESS) {
/*		printf("del(%d): %d->%d\n", id, le->refcount, le->refcount-1); */
		if (--le->refcount<=0) {
			return zend_hash_index_del(&EG(regular_list), id);
		} else {
			return SUCCESS;
		}
	} else {
		return FAILURE;
	}
}


static inline void *zend_list_do_find(HashTable *list,int id, int *type)
{
	zend_rsrc_list_entry *le;

	if (zend_hash_index_find(list, id, (void **) &le)==SUCCESS) {
		*type = le->type;
		return le->ptr;
	} else {
		*type = -1;
		return NULL;
	}
}


ZEND_API int zend_list_insert(void *ptr, int type)
{
	ELS_FETCH();

	return zend_list_do_insert(&EG(regular_list), ptr, type, 1);
}


ZEND_API int zend_plist_insert(void *ptr, int type)
{
	ELS_FETCH();

	return zend_list_do_insert(&EG(persistent_list), ptr, type, 1);
}


ZEND_API int zend_list_addref(int id)
{
	zend_rsrc_list_entry *le;
	ELS_FETCH();
	
	if (zend_hash_index_find(&EG(regular_list), id, (void **) &le)==SUCCESS) {
/*		printf("add(%d): %d->%d\n", id, le->refcount, le->refcount+1); */
		le->refcount++;
		return SUCCESS;
	} else {
		return FAILURE;
	}
}


ZEND_API int zend_list_delete(int id)
{
	ELS_FETCH();

	return zend_list_do_delete(&EG(regular_list), id);
}


ZEND_API int zend_plist_delete(int id)
{
	ELS_FETCH();

	return zend_list_do_delete(&EG(persistent_list), id);
}


ZEND_API int zend_list_convert_to_number(int id)
{
	zend_rsrc_list_entry *le;
	ELS_FETCH();
	
	if (zend_hash_index_find(&EG(regular_list), id, (void **) &le)==SUCCESS
		&& le->valid) {
		return id;
	}
	return 0;
}



ZEND_API void *zend_list_find(int id, int *type)
{
	ELS_FETCH();

	return zend_list_do_find(&EG(regular_list), id, type);
}


ZEND_API void *zend_plist_find(int id, int *type)
{
	ELS_FETCH();

	return zend_list_do_find(&EG(persistent_list), id, type);
}


ZEND_API int zend_register_resource(zval *rsrc_result, void *rsrc_pointer, int rsrc_type)
{
	int rsrc_id;

	rsrc_id = zend_list_insert(rsrc_pointer, rsrc_type);
	
	if (rsrc_result) {
		rsrc_result->value.lval = rsrc_id;
		rsrc_result->type = IS_RESOURCE;
	}

	return rsrc_id;
}


ZEND_API int zend_register_false_resource(zval *rsrc_result, void *rsrc_pointer, int rsrc_type)
{
	int rsrc_id;
	ELS_FETCH();

	rsrc_id = zend_list_do_insert(&EG(regular_list), rsrc_pointer, rsrc_type, 0);
	
	if (rsrc_result) {
		rsrc_result->value.lval = rsrc_id;
		rsrc_result->type = IS_RESOURCE;
	}

	return rsrc_id;
}


ZEND_API void *zend_fetch_resource(zval **passed_id, int default_id, char *resource_type_name, int *found_resource_type, int num_resource_types, ...)
{
	int id;
	int actual_resource_type;
	void *resource;
	va_list resource_types;
	int i;

	if (default_id==-1) { /* use id */
		if (!passed_id) {
			if (resource_type_name) {
				zend_error(E_WARNING, "No %s resource supplied", resource_type_name);
			}
			return NULL;
		} else if ((*passed_id)->type != IS_RESOURCE) {
			if (resource_type_name) {
				zend_error(E_WARNING, "Supplied argument is not a valid %s resource", resource_type_name);
			}
			return NULL;
		}
		id = (*passed_id)->value.lval;
	} else {
		id = default_id;
	}

	resource = zend_list_find(id, &actual_resource_type);
	if (!resource) {
		if (resource_type_name)
			zend_error(E_WARNING, "%d is not a valid %s resource", id, resource_type_name);
		return NULL;
	}

	va_start(resource_types, num_resource_types);
	for (i=0; i<num_resource_types; i++) {
		if (actual_resource_type == va_arg(resource_types, int)) {
			va_end(resource_types);
			if (found_resource_type) {
				*found_resource_type = actual_resource_type;
			}
			return resource;
		}
	}
	va_end(resource_types);

	if (resource_type_name)
		zend_error(E_WARNING, "Supplied resource is not a valid %s resource", resource_type_name);

	return NULL;
}


void list_entry_destructor(void *ptr)
{
	zend_rsrc_list_entry *le = (zend_rsrc_list_entry *) ptr;
	zend_rsrc_list_dtors_entry *ld;
	
	if (zend_hash_index_find(&list_destructors, le->type,(void **) &ld)==SUCCESS) {
		switch (ld->type) {
			case ZEND_RESOURCE_LIST_TYPE_STD:
				if (ld->list_dtor) {
					(ld->list_dtor)(le->ptr);
				}
				break;
			case ZEND_RESOURCE_LIST_TYPE_EX:
				if (ld->list_dtor_ex) {
					ld->list_dtor_ex(le);
				}
				break;
			EMPTY_SWITCH_DEFAULT_CASE()
		}
	} else {
		zend_error(E_WARNING,"Unknown list entry type in request shutdown (%d)",le->type);
	}
}


void plist_entry_destructor(void *ptr)
{
	zend_rsrc_list_entry *le = (zend_rsrc_list_entry *) ptr;
	zend_rsrc_list_dtors_entry *ld;

	if (zend_hash_index_find(&list_destructors, le->type,(void **) &ld)==SUCCESS) {
		if (ld->plist_dtor) {
			(ld->plist_dtor)(le->ptr);
		}
	} else {
		zend_error(E_WARNING,"Unknown persistent list entry type in module shutdown (%d)",le->type);
	}
}


int zend_init_rsrc_list(ELS_D)
{
	return zend_hash_init(&EG(regular_list), 0, NULL, list_entry_destructor, 0);
}


int zend_init_rsrc_plist(ELS_D)
{
	return zend_hash_init(&EG(persistent_list), 0, NULL, plist_entry_destructor, 1);
}


void zend_destroy_rsrc_list(ELS_D)
{
	zend_hash_graceful_destroy(&EG(regular_list));
}


void zend_destroy_rsrc_plist(ELS_D)
{
	zend_hash_graceful_destroy(&EG(persistent_list));
}


static int clean_module_resource(zend_rsrc_list_entry *le, int *resource_id)
{
	if (le->type == *resource_id) {
		return 1;
	} else {
		return 0;
	}
}


static int zend_clean_module_rsrc_dtors_cb(zend_rsrc_list_dtors_entry *ld, int *module_number)
{
	if (ld->module_number == *module_number) {
		ELS_FETCH();

		zend_hash_apply_with_argument(&EG(regular_list), (int (*)(void *,void *)) clean_module_resource, (void *) &(ld->resource_id));
		zend_hash_apply_with_argument(&EG(persistent_list), (int (*)(void *,void *)) clean_module_resource, (void *) &(ld->resource_id));
		return 1;
	} else {
		return 0;
	}
}


void zend_clean_module_rsrc_dtors(int module_number)
{
	zend_hash_apply_with_argument(&list_destructors, (int (*)(void *,void *)) zend_clean_module_rsrc_dtors_cb, (void *) &module_number);
}


ZEND_API int zend_register_list_destructors(void (*ld)(void *), void (*pld)(void *), int module_number)
{
	zend_rsrc_list_dtors_entry lde;
	
#if 0
	printf("Registering destructors %d for module %d\n", list_destructors.nNextFreeElement, module_number);
#endif
	
	lde.list_dtor=(void (*)(void *)) ld;
	lde.plist_dtor=(void (*)(void *)) pld;
	lde.list_dtor_ex = lde.plist_dtor_ex = NULL;
	lde.module_number = module_number;
	lde.resource_id = list_destructors.nNextFreeElement;
	lde.type = ZEND_RESOURCE_LIST_TYPE_STD;
	
	if (zend_hash_next_index_insert(&list_destructors, (void *) &lde, sizeof(zend_rsrc_list_dtors_entry), NULL)==FAILURE) {
		return FAILURE;
	}
	return list_destructors.nNextFreeElement-1;
}


ZEND_API int zend_register_list_destructors_ex(rsrc_dtor_func_t ld, rsrc_dtor_func_t pld, int module_number)
{
	zend_rsrc_list_dtors_entry lde;
	
#if 0
	printf("Registering destructors %d for module %d\n", list_destructors.nNextFreeElement, module_number);
#endif

	lde.list_dtor = NULL;
	lde.plist_dtor = NULL;
	lde.list_dtor_ex = ld;
	lde.plist_dtor_ex = pld;
	lde.module_number = module_number;
	lde.resource_id = list_destructors.nNextFreeElement;
	lde.type = ZEND_RESOURCE_LIST_TYPE_EX;
	
	if (zend_hash_next_index_insert(&list_destructors,(void *) &lde, sizeof(zend_rsrc_list_dtors_entry),NULL)==FAILURE) {
		return FAILURE;
	}
	return list_destructors.nNextFreeElement-1;
}


int zend_init_rsrc_list_dtors()
{
	return zend_hash_init(&list_destructors, 50, NULL, NULL, 1);
}


void zend_destroy_rsrc_list_dtors()
{
	zend_hash_destroy(&list_destructors);
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
