/*
 * myMalloc2_*: the same K&R allocator as myMalloc_* (mymalloc.c) with
 * 128-byte units, serialized by a pthread mutex.
 */

#include <pthread.h>
#include "mymalloc2.h"

typedef struct Header2 {
	struct Header2 *ptr;
	unsigned size;
	unsigned char pad[120];
} Header2; /* size = 0x80: the allocation unit */

static Header2 *freep2 = 0;
static void *heap_base2 = 0;
static void *heap_cur2 = 0;  /* unused, but part of the layout */
static unsigned heap_size2 = 0;
static int used2 = 0;
static int peak2 = 0;
static Header2 base2;
static pthread_mutex_t mutex2;

static Header2 *myMalloc2_Grow(unsigned nu);

void myMalloc2_InitHeap(void *heap, unsigned size)
{
	size = (size >> 7) << 7;
	freep2 = 0;
	heap_base2 = heap;
	heap_cur2 = heap;
	heap_size2 = size;
	pthread_mutex_init(&mutex2, 0);
}

void *myMalloc2_Malloc(unsigned size)
{
	void *p;
	unsigned rounded;

	pthread_mutex_lock(&mutex2);
	p = myMalloc2_Malloc_(size + 128);
	if (p == 0)
		goto fail;
	rounded = ((size + 255) >> 7) << 7;
	used2 += rounded;
	*(unsigned *)p = rounded;
	if (peak2 < used2)
		peak2 = used2;
	pthread_mutex_unlock(&mutex2);
	return (char *)p + 128;
fail:
	pthread_mutex_unlock(&mutex2);
	return 0;
}

void myMalloc2_Free(void *ptr)
{
	unsigned rounded;

	if (ptr == 0)
		return;
	ptr = (char *)ptr - 128;
	rounded = *(unsigned *)ptr;
	pthread_mutex_lock(&mutex2);
	used2 -= rounded;
	myMalloc2_Free_(ptr);
	pthread_mutex_unlock(&mutex2);
}

void *_stdMalloc2(unsigned size)
{
	return myMalloc2_Malloc(size);
}

void _stdFree2(void *ptr)
{
	myMalloc2_Free(ptr);
}

void *myMalloc2_Malloc_(unsigned nbytes)
{
	Header2 *p, *prevp;
	unsigned nunits;

	nunits = (nbytes + sizeof(Header2) - 1) / sizeof(Header2) + 1;
	if ((prevp = freep2) == 0) {
		base2.ptr = freep2 = prevp = &base2;
		base2.size = 0;
	}
	for (p = prevp->ptr;; prevp = p, p = p->ptr) {
		if (p->size >= nunits) {
			if (p->size == nunits)
				prevp->ptr = p->ptr;
			else {
				p->size -= nunits;
				p += p->size;
				p->size = nunits;
			}
			freep2 = prevp;
			return (void *)(p + 1);
		}
		if (p == freep2) {
			if ((p = myMalloc2_Grow(nunits)) == 0)
				return 0;
		}
	}
}

void myMalloc2_Free_(void *ap)
{
	Header2 *bp, *p;

	bp = (Header2 *)ap - 1;
	for (p = freep2; !(bp > p && bp < p->ptr); p = p->ptr) {
		if (p == p->ptr)
			break;
		if (p < p->ptr)
			continue;
		if (bp > p || bp < p->ptr)
			break;
	}
	if (bp + bp->size == p->ptr) {
		bp->size += p->ptr->size;
		bp->ptr = p->ptr->ptr;
	} else
		bp->ptr = p->ptr;
	if (p + p->size == bp) {
		p->size += bp->size;
		p->ptr = bp->ptr;
	} else
		p->ptr = bp;
	freep2 = p;
}

/* K&R morecore: hands the whole arena to the free list once; nu is ignored and
 * every later call fails. */
static Header2 *myMalloc2_Grow(unsigned nu)
{
	Header2 *p;
	unsigned units;

	if (heap_size2 == 0 || (p = (Header2 *)heap_base2) == 0)
		return 0;
	units = heap_size2 >> 7;
	p->size = units;
	myMalloc2_Free_(p + 1);
	heap_size2 = 0;
	heap_base2 = 0;
	return freep2;
}
