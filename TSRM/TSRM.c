/*
   +----------------------------------------------------------------------+
   | Thread Safe Resource Manager                                         |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998, 1999 Zeev Suraski                                |
   +----------------------------------------------------------------------+
   | This source file is subject to the Zend license, that is bundled     |
   | with this package in the file LICENSE.  If you did not receive a     |
   | copy of the Zend license, please mail us at zend@zend.com so we can  |
   | send you a copy immediately.                                         |
   +----------------------------------------------------------------------+
   | Author:  Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


#include "TSRM.h"
#include <stdio.h>
#include <stdlib.h>

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

typedef struct _tsrm_tls_entry tsrm_tls_entry;

struct _tsrm_tls_entry {
	void **storage;
	int count;
	THREAD_T thread_id;
	tsrm_tls_entry *next;
};


typedef struct {
	size_t size;
	void (*ctor)(void *resource);
	void (*dtor)(void *resource);
} tsrm_resource_type;


/* The memory manager table */
static tsrm_tls_entry	**tsrm_tls_table;
static int				tsrm_tls_table_size;
static ts_rsrc_id		id_count;

/* The resource sizes table */
static tsrm_resource_type	*resource_types_table;
static int					resource_types_table_size;


static MUTEX_T tsmm_mutex;	/* thread-safe memory manager mutex */

/* Debug support */
static int tsrm_debug(const char *format, ...);
static int tsrm_debug_status;


/* Startup TSRM (call once for the entire process) */
TSRM_FUNC int tsrm_startup(int expected_threads, int expected_resources, int debug_status)
{
	tsrm_tls_table_size = expected_threads;
	tsrm_tls_table = (tsrm_tls_entry **) calloc(tsrm_tls_table_size, sizeof(tsrm_tls_entry *));
	if (!tsrm_tls_table) {
		return 0;
	}
	id_count=0;

	resource_types_table_size = expected_resources;
	resource_types_table = (tsrm_resource_type *) calloc(resource_types_table_size, sizeof(tsrm_resource_type));
	if (!resource_types_table) {
		free(tsrm_tls_table);
		return 0;
	}

	tsmm_mutex = tsrm_mutex_alloc();

	tsrm_debug_status = debug_status;

	tsrm_debug("Started up TSRM, %d expected threads, %d expected resources\n", expected_threads, expected_resources);
	return 1;
}


/* Shutdown TSRM (call once for the entire process) */
TSRM_FUNC void tsrm_shutdown()
{
	int i;

	if (tsrm_tls_table) {
		for (i=0; i<tsrm_tls_table_size; i++) {
			tsrm_tls_entry *p = tsrm_tls_table[i], *next_p;

			while (p) {
				int j;

				next_p = p->next;
				for (j=0; j<id_count; j++) {
					free(p->storage[j]);
				}
				free(p->storage);
				free(p);
				p = next_p;
			}
		}
		free(tsrm_tls_table);
	}
	if (resource_types_table) {
		free(resource_types_table);
	}
	tsrm_mutex_free(tsmm_mutex);
	tsrm_debug("Shutdown TSRM\n");
}


/* allocates a new thread-safe-resource id */
TSRM_FUNC ts_rsrc_id ts_allocate_id(size_t size, void (*ctor)(void *resource), void (*dtor)(void *resource))
{
	ts_rsrc_id new_id;
	int i;

	tsrm_debug("Obtaining a new resource id, %d bytes\n", size);

	tsrm_mutex_lock(tsmm_mutex);

	/* obtain a resource id */
	new_id = id_count++;
	tsrm_debug("Obtained resource id %d\n", new_id);

	/* store the new resource type in the resource sizes table */
	if (resource_types_table_size < id_count) {
		resource_types_table = (tsrm_resource_type *) realloc(resource_types_table, sizeof(tsrm_resource_type)*id_count);
		if (!resource_types_table) {
			return -1;
		}
		resource_types_table_size = id_count;
	}
	resource_types_table[new_id].size = size;
	resource_types_table[new_id].ctor = ctor;
	resource_types_table[new_id].dtor = dtor;

	/* enlarge the arrays for the already active threads */
	for (i=0; i<tsrm_tls_table_size; i++) {
		tsrm_tls_entry *p = tsrm_tls_table[i];

		while (p) {
			if (p->count < id_count) {
				int j;

				p->storage = (void *) realloc(p->storage, sizeof(void *)*id_count);
				for (j=p->count; j<id_count; j++) {
					p->storage[j] = (void *) malloc(resource_types_table[j].size);
					if (resource_types_table[j].ctor) {
						resource_types_table[j].ctor(p->storage[j]);
					}
				}
				p->count = id_count;
			}
			p = p->next;
		}
	}
	tsrm_mutex_unlock(tsmm_mutex);

	tsrm_debug("Successfully allocated new resource id %d\n", new_id);
	return new_id;
}


static void allocate_new_resource(tsrm_tls_entry **thread_resources_ptr, THREAD_T thread_id)
{
	int i;

	(*thread_resources_ptr) = (tsrm_tls_entry *) malloc(sizeof(tsrm_tls_entry));
	(*thread_resources_ptr)->storage = (void **) malloc(sizeof(void *)*id_count);
	(*thread_resources_ptr)->count = id_count;
	(*thread_resources_ptr)->thread_id = thread_id;
	(*thread_resources_ptr)->next = NULL;
	for (i=0; i<id_count; i++) {
		(*thread_resources_ptr)->storage[i] = (void *) malloc(resource_types_table[i].size);
		if (resource_types_table[i].ctor) {
			resource_types_table[i].ctor((*thread_resources_ptr)->storage[i]);
		}
	}
}


/* fetches the requested resource for the current thread */
void *ts_resource(ts_rsrc_id id)
{
	THREAD_T thread_id = tsrm_thread_id();
	int hash_value;
	tsrm_tls_entry *thread_resources;
	void *resource;

	tsrm_debug("Fetching resource id %d for thread %ld\n", id, (long) thread_id);
	tsrm_mutex_lock(tsmm_mutex);

	hash_value = THREAD_HASH_OF(thread_id, tsrm_tls_table_size);
	thread_resources = tsrm_tls_table[hash_value];

	if (!thread_resources) {
		allocate_new_resource(&tsrm_tls_table[hash_value], thread_id);
		thread_resources = tsrm_tls_table[hash_value];
	} else {
		 do {
			if (thread_resources->thread_id == thread_id) {
				break;
			}
			if (thread_resources->next) {
				thread_resources = thread_resources->next;
			} else {
				allocate_new_resource(&thread_resources->next, thread_id);
				thread_resources = thread_resources->next;
				break;
			}
		 } while (thread_resources);
	}

	resource = thread_resources->storage[id];

	tsrm_mutex_unlock(tsmm_mutex);

	tsrm_debug("Successfully fetched resource id %d for thread id %ld - %x\n", id, (long) thread_id, (long) resource);
	return resource;
}


/* frees all resources allocated for the current thread */
void ts_free_thread()
{
	THREAD_T thread_id = tsrm_thread_id();
	int hash_value;
	tsrm_tls_entry *thread_resources;
	tsrm_tls_entry *last=NULL;

	tsrm_mutex_lock(tsmm_mutex);
	hash_value = THREAD_HASH_OF(thread_id, tsrm_tls_table_size);
	thread_resources = tsrm_tls_table[hash_value];

	while (thread_resources) {
		if (thread_resources->thread_id == thread_id) {
			int i;

			for (i=0; i<thread_resources->count; i++) {
				if (resource_types_table[i].dtor) {
					resource_types_table[i].dtor(thread_resources->storage[i]);
				}
				free(thread_resources->storage[i]);
			}
			free(thread_resources->storage);
			if (last) {
				last->next = thread_resources->next;
			} else {
				tsrm_tls_table[hash_value]=NULL;
			}
			free(thread_resources);
			break;
		}
		if (thread_resources->next) {
			last = thread_resources;
		}
		thread_resources = thread_resources->next;
	}
	tsrm_mutex_unlock(tsmm_mutex);
}


/* deallocates all occurrences of a given id */
void ts_free_id(ts_rsrc_id id)
{
}




/*
 * Utility Functions
 */

/* Obtain the current thread id */
TSRM_FUNC THREAD_T tsrm_thread_id(void)
{
#ifdef WIN32
	return GetCurrentThreadId();
#elif defined(PTHREADS)
	return pthread_self();
#elif defined(NSAPI)
	return systhread_current();
#elif defined(PI3WEB)
	return PIThread_getCurrent();
#endif
}


/* Allocate a mutex */
TSRM_FUNC MUTEX_T tsrm_mutex_alloc( void )
{
    MUTEX_T mutexp;

#ifdef WIN32
    mutexp = CreateMutex(NULL,FALSE,NULL);
#elif defined(PTHREADS)
	mutexp = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutexp,NULL);
#elif defined(NSAPI)
	mutexp = crit_init();
#elif defined(PI3WEB)
	mutexp = PIPlatform_allocLocalMutex();
#endif
#ifdef THR_DEBUG
		printf("Mutex created thread: %d\n",mythreadid());
#endif
    return( mutexp );
}


/* Free a mutex */
TSRM_FUNC void tsrm_mutex_free( MUTEX_T mutexp )
{
    if (mutexp) {
#ifdef WIN32
		CloseHandle(mutexp);
#elif defined(PTHREADS)
		free(mutexp);
#elif defined(NSAPI)
		crit_terminate(mutexp);
#elif defined(PI3WEB)
		PISync_delete(mutexp) 
#endif
    }
#ifdef THR_DEBUG
		printf("Mutex freed thread: %d\n",mythreadid());
#endif
}


/* Lock a mutex */
TSRM_FUNC int tsrm_mutex_lock( MUTEX_T mutexp )
{
	//tsrm_debug("Mutex locked thread: %ld\n",tsrm_thread_id());
#ifdef WIN32
    return WaitForSingleObject(mutexp,1000);
#elif defined(PTHREADS)
	return pthread_mutex_lock(mutexp);
#elif defined(NSAPI)
	return crit_enter(mutexp);
#elif defined(PI3WEB)
	return PISync_lock(mutexp);
#endif
}


/* Unlock a mutex */
TSRM_FUNC int tsrm_mutex_unlock( MUTEX_T mutexp )
{
	//tsrm_debug("Mutex unlocked thread: %ld\n",tsrm_thread_id());
#ifdef WIN32
    return ReleaseMutex(mutexp);
#elif defined(PTHREADS)
	return pthread_mutex_unlock(mutexp);
#elif defined(NSAPI)
	return crit_exit(mutexp);
#elif defined(PI3WEB)
	return PISync_unlock(mutexp);
#endif
}


/*
 * Debug support
 */

static int tsrm_debug(const char *format, ...)
{
	if (tsrm_debug_status) {
		va_list args;
		int size;

		va_start(args, format);
		size = vprintf(format, args);
		va_end(args);
		return size;
	} else {
		return 0;
	}
}


void tsrm_debug_set(int status)
{
	tsrm_debug_status = status;
}
