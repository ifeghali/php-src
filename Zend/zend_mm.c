/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2002 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

#include <stdlib.h>
#include "zend_mm.h"

#if WIN32|WINNT
# ifndef inline
#  define inline __inline
# endif
#endif

#define ZEND_MM_FREE_BLOCK	0
#define ZEND_MM_USED_BLOCK	1

#ifndef MAX
#define MAX(a, b) (((a)>(b))?(a):(b))
#endif

/* Platform alignment test */
typedef union _mm_align_test {
	void *ptr;
	double dbl;
	long lng;
} mm_align_test;

#if (defined (__GNUC__) && __GNUC__ >= 2)
#define ZEND_MM_ALIGNMENT (__alignof__ (mm_align_test))
#else
#define ZEND_MM_ALIGNMENT (sizeof(mm_align_test))
#endif

#define ZEND_MM_ALIGNMENT_MASK ~(ZEND_MM_ALIGNMENT-1)

/* Aligned header size */
#define ZEND_MM_ALIGNED_SIZE(size) ((size + ZEND_MM_ALIGNMENT - 1) & ZEND_MM_ALIGNMENT_MASK)
#define ZEND_MM_ALIGNED_HEADER_SIZE	ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_block))
#define ZEND_MM_ALIGNED_FREE_HEADER_SIZE ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_free_block))
#define ZEND_MM_ALIGNED_SEGMENT_SIZE ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_segment))


/* Memory calculations */
#define ZEND_MM_BLOCK_AT(blk, offset)		((zend_mm_block *) (((char *) (blk))+(offset)))
#define ZEND_MM_DATA_OF(p)				((void *) (((char *) (p))+ZEND_MM_ALIGNED_HEADER_SIZE))
#define ZEND_MM_HEADER_OF(blk)			ZEND_MM_BLOCK_AT(blk, -(int)ZEND_MM_ALIGNED_HEADER_SIZE)

/* Debug output */
#define ZEND_MM_DEBUG(stmt)


static inline void zend_mm_add_to_free_list(zend_mm_heap *heap, zend_mm_free_block *mm_block)
{
	mm_block->next_free_block = heap->free_list;
	mm_block->prev_free_block = NULL;
	heap->free_list = mm_block;

	if (mm_block->next_free_block) {
		mm_block->next_free_block->prev_free_block = mm_block;
	}
}


static inline void zend_mm_remove_from_free_list(zend_mm_heap *heap, zend_mm_free_block *mm_block)
{
	if (mm_block->prev_free_block) {
		mm_block->prev_free_block->next_free_block = mm_block->next_free_block;
	} else {
		heap->free_list = mm_block->next_free_block;
	}

	if (mm_block->next_free_block) {
		mm_block->next_free_block->prev_free_block = mm_block->prev_free_block;
	}
}

static inline void zend_mm_create_new_free_block(zend_mm_heap *heap, zend_mm_block *mm_block, size_t true_size)
{
	int remaining_size;
	zend_mm_free_block *new_free_block;

	/* calculate sizes */
	remaining_size = mm_block->size - true_size;

	if (remaining_size < ZEND_MM_ALIGNED_FREE_HEADER_SIZE) {
		/* keep best_fit->size as is, it'll include this extra space */
		return;
	}

	/* prepare new free block */
	mm_block->size = true_size;
	new_free_block = (zend_mm_free_block *) ZEND_MM_BLOCK_AT(mm_block, mm_block->size);

	new_free_block->type = ZEND_MM_FREE_BLOCK;
	new_free_block->size = remaining_size;
	new_free_block->prev_size = true_size;

	/* update the next block's prev_size */
	ZEND_MM_BLOCK_AT(new_free_block, new_free_block->size)->prev_size = new_free_block->size;

	/* add the new free block to the free list */
	zend_mm_add_to_free_list(heap, new_free_block);
	return;
}

zend_bool zend_mm_add_memory_block(zend_mm_heap *heap, size_t block_size)
{
	/* We need to make sure that block_size is big enough for the minimum segment size */
	zend_mm_free_block *mm_block;
	zend_mm_block *guard_block;
	zend_mm_segment *segment;

	/* align block size downwards */
	block_size = block_size & ZEND_MM_ALIGNMENT_MASK;

	segment = (zend_mm_segment *) malloc(block_size);
	if (!segment) {
		return 1;
	}
/*	fprintf(stderr, "Allocating segment %X\n", segment); */
	segment->next_segment = heap->segments_list;
	heap->segments_list = segment;

	block_size -= ZEND_MM_ALIGNED_SEGMENT_SIZE;
	mm_block = (zend_mm_free_block *) ((char *) segment + ZEND_MM_ALIGNED_SEGMENT_SIZE);
	
	mm_block->size = block_size - ZEND_MM_ALIGNED_FREE_HEADER_SIZE; /* keep one guard block in the end */
	mm_block->type = ZEND_MM_FREE_BLOCK;
	mm_block->prev_size = 0; /* Size is always at least ZEND_MM_ALIGNED_HEADER_SIZE big (>0) so 0 is OK */
	
	/* setup guard block */
	guard_block = ZEND_MM_BLOCK_AT(mm_block, mm_block->size);
	guard_block->type = ZEND_MM_USED_BLOCK;
	guard_block->size = ZEND_MM_ALIGNED_FREE_HEADER_SIZE;
	guard_block->prev_size = mm_block->size;
	ZEND_MM_DEBUG(("Setup guard block at 0x%0.8X\n", guard_block));

	zend_mm_add_to_free_list(heap, mm_block);

	return 0;
}

/* Notes:
 * - This function may alter the block_sizes values to match platform alignment
 * - This function does *not* perform sanity checks on the arguments
 */
zend_bool zend_mm_startup(zend_mm_heap *heap, size_t block_size)
{
	heap->block_size = block_size;
	heap->free_list = NULL;
	heap->segments_list = NULL;
	return zend_mm_add_memory_block(heap, block_size);
}


void zend_mm_shutdown(zend_mm_heap *heap)
{
	zend_mm_segment *segment = heap->segments_list;
	zend_mm_segment *prev;

	while (segment) {
		prev = segment;
		segment = segment->next_segment;
/*		fprintf(stderr, "Freeing segment %X\n", prev);*/
		free(prev);
	}
	heap->segments_list = NULL;
}

void *zend_mm_alloc(zend_mm_heap *heap, size_t size)
{
	size_t true_size;
	zend_mm_free_block *p, *best_fit=NULL;

	/* The max() can probably be optimized with an if() which checks more specific cases */
	true_size = MAX(ZEND_MM_ALIGNED_SIZE(size)+ZEND_MM_ALIGNED_HEADER_SIZE, ZEND_MM_ALIGNED_FREE_HEADER_SIZE);
	for (p = heap->free_list; p; p = p->next_free_block) {
		if (p->size == true_size) {
			best_fit = p;
			break;
		}
		if ((p->size > true_size) && (!best_fit || (best_fit->size > p->size))) {	/* better fit */
			best_fit = p;
		}
	}
	if (!best_fit) {
		if (true_size > (heap->block_size - ZEND_MM_ALIGNED_SEGMENT_SIZE - ZEND_MM_ALIGNED_FREE_HEADER_SIZE)) {
			/* Make sure we add a memory block which is big enough */
			zend_mm_add_memory_block(heap, true_size + ZEND_MM_ALIGNED_SEGMENT_SIZE + ZEND_MM_ALIGNED_FREE_HEADER_SIZE);
		} else {
			zend_mm_add_memory_block(heap, heap->block_size);
		}
		return zend_mm_alloc(heap, size);
	}

	/* mark as used */
	best_fit->type = ZEND_MM_USED_BLOCK;

	/* remove from free list */
	zend_mm_remove_from_free_list(heap, best_fit);

	zend_mm_create_new_free_block(heap, (zend_mm_block *) best_fit, true_size);

	return ZEND_MM_DATA_OF(best_fit);
}


void zend_mm_free(zend_mm_heap *heap, void *p)
{
	zend_mm_block *mm_block = ZEND_MM_HEADER_OF(p);
	zend_mm_block *prev_block, *next_block;
	int in_free_list=0;
   
	if (mm_block->type!=ZEND_MM_USED_BLOCK) {
		/* error */
		return;
	}
   
	next_block = ZEND_MM_BLOCK_AT(mm_block, mm_block->size);

    /* merge with previous block if empty */
	if (mm_block->prev_size != 0
		&& (prev_block=ZEND_MM_BLOCK_AT(mm_block, -(int)mm_block->prev_size))->type == ZEND_MM_FREE_BLOCK) {
		prev_block->size += mm_block->size;
		mm_block = prev_block;
		next_block->prev_size = mm_block->size;
		in_free_list = 1;	/* linked two ways */
	}

	/* merge with the next block if empty */
	if (next_block->type == ZEND_MM_FREE_BLOCK) {
		mm_block->size += next_block->size;
		zend_mm_remove_from_free_list(heap, (zend_mm_free_block *) next_block);
		next_block = ZEND_MM_BLOCK_AT(mm_block, mm_block->size);	/* recalculate */
		next_block->prev_size = mm_block->size;
	}

	mm_block->type = ZEND_MM_FREE_BLOCK;
	if (!in_free_list) {
		zend_mm_add_to_free_list(heap, (zend_mm_free_block *) mm_block);
	}
}

void *zend_mm_realloc(zend_mm_heap *heap, void *p, size_t size)
{
	zend_mm_block *mm_block = ZEND_MM_HEADER_OF(p);
	zend_mm_block *next_block;
	size_t true_size = MAX(ZEND_MM_ALIGNED_SIZE(size)+ZEND_MM_ALIGNED_HEADER_SIZE, ZEND_MM_ALIGNED_FREE_HEADER_SIZE);

	if (true_size <= mm_block->size) {
		zend_mm_create_new_free_block(heap, mm_block, true_size);
		/* We don't yet merge this free block with the following one */

		return p;
	}

	next_block = ZEND_MM_BLOCK_AT(mm_block, mm_block->size);

	if (next_block->type != ZEND_MM_FREE_BLOCK || (mm_block->size + next_block->size < true_size)) {
		void *ptr;
	
		ptr = zend_mm_alloc(heap, size);
		memcpy(ptr, p, mm_block->size - ZEND_MM_ALIGNED_HEADER_SIZE);
		zend_mm_free(heap, p);
		return ptr;
	}

	zend_mm_remove_from_free_list(heap, (zend_mm_free_block *) next_block);
	mm_block->size += next_block->size;
	/* update the next block's prev_size */
	ZEND_MM_BLOCK_AT(mm_block, mm_block->size)->prev_size = mm_block->size;
	
	zend_mm_create_new_free_block(heap, mm_block, true_size);
	/* We don't yet merge this free block with the following one */

	return p;
}
