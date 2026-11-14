/*
 * myMalloc_*: the K&R malloc/free (circular free list, 16-byte units) over one
 * static arena set by myMalloc_InitHeap, without locking. The public
 * Malloc/Free add a 16-byte header holding the rounded size, for used/peak.
 */

#include "mymalloc.h"

typedef struct Header1 {
	struct Header1 *ptr;
	unsigned size;
	unsigned char pad[8];
} Header1; /* size = 0x10: the allocation unit */

static Header1 *freep1 = 0;
static void *heap_base1 = 0;
static void *heap_cur1 = 0;  /* unused, but part of the layout */
static unsigned heap_size1 = 0;
static int used1 = 0;
static int peak1 = 0;
static Header1 base1;

static Header1 *myMalloc_Grow(unsigned nu);

void myMalloc_InitHeap(void *heap, unsigned size)
{
	size = (size >> 4) << 4;
	freep1 = 0;
	heap_base1 = heap;
	heap_cur1 = heap;
	heap_size1 = size;
}

void *myMalloc_Malloc(unsigned size)
{
	void *p;
	unsigned rounded;

	p = myMalloc_Malloc_(size + 16);
	if (p == 0)
		goto fail;
	rounded = ((size + 31) >> 4) << 4;
	used1 += rounded;
	*(unsigned *)p = rounded;
	if (peak1 < used1)
		peak1 = used1;
	return (char *)p + 16;
fail:
	return 0;
}

void myMalloc_Free(void *ptr)
{
	unsigned rounded;

	if (ptr == 0)
		return;
	ptr = (char *)ptr - 16;
	rounded = *(unsigned *)ptr;
	used1 -= rounded;
	myMalloc_Free_(ptr);
}

void *_stdMalloc(unsigned size)
{
	return myMalloc_Malloc(size);
}

void _stdFree(void *ptr)
{
	myMalloc_Free(ptr);
}

void *myMalloc_Malloc_(unsigned nbytes)
{
	Header1 *p, *prevp;
	unsigned nunits;

	nunits = (nbytes + sizeof(Header1) - 1) / sizeof(Header1) + 1;
	if ((prevp = freep1) == 0) {
		base1.ptr = freep1 = prevp = &base1;
		base1.size = 0;
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
			freep1 = prevp;
			return (void *)(p + 1);
		}
		if (p == freep1) {
			if ((p = myMalloc_Grow(nunits)) == 0)
				return 0;
		}
	}
}

void myMalloc_Free_(void *ap)
{
	Header1 *bp, *p;

	bp = (Header1 *)ap - 1;
	for (p = freep1; !(bp > p && bp < p->ptr); p = p->ptr) {
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
	freep1 = p;
}

/* K&R morecore: hands the whole arena to the free list once; nu is ignored and
 * every later call fails. */
static Header1 *myMalloc_Grow(unsigned nu)
{
	Header1 *p;
	unsigned units;

	if (heap_size1 == 0 || (p = (Header1 *)heap_base1) == 0)
		return 0;
	units = heap_size1 >> 4;
	p->size = units;
	myMalloc_Free_(p + 1);
	heap_size1 = 0;
	heap_base1 = 0;
	return freep1;
}
