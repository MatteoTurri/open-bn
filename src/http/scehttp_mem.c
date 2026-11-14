/*
 * Memory of the HTTP library: the xmalloc family (through replaceable
 * function pointers), the user allocation hooks and the internal page pool.
 */

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "http/scehttp.h"

/* The allocator used by xmalloc and friends (NULL = libc), then the user
 * hooks and the caller address saved by the trampolines. */
void *(*sceHTTPLibMallocFunction)(size_t);
void *(*sceHTTPLibReallocFunction)(void *, size_t);
void (*sceHTTPLibFreeFunction)(void *);
static void *(*g_malloc_hook)(unsigned int);
static void *(*g_realloc_hook)(void *, unsigned int);
static void *(*g_free_hook)(void *);
static void *g_mem_hook_ra;

/* Installed by sceHTTPSet*Function: save the caller address
   (sceHTTPLibGetMemoryFunctionRA) and call the user hook. */
static void *malloc_trampoline(unsigned int size)
{
	g_mem_hook_ra = __builtin_return_address(0);
	return g_malloc_hook(size);
}

static void *realloc_trampoline(void *p, unsigned int size)
{
	g_mem_hook_ra = __builtin_return_address(0);
	return g_realloc_hook(p, size);
}

static void *free_trampoline(void *p)
{
	g_mem_hook_ra = __builtin_return_address(0);
	return g_free_hook(p);
}

/* calloc through the replaceable malloc; fails on overflow of n * size. */
void *xcalloc(size_t n, size_t size)
{
	size_t total = size * n;
	void *p;

	if ((size | n) > 0xffff) {
		if (n == 0)
			return 0;
		if (total / n != size)
			return 0;
	}
	if (sceHTTPLibMallocFunction) {
		p = (*sceHTTPLibMallocFunction)(total);
		if (p)
			memset(p, 0, total);
	} else {
		p = calloc(n, size);
	}
	return p;
}

void *xmalloc(size_t size)
{
	void *p;

	if (sceHTTPLibMallocFunction)
		p = (*sceHTTPLibMallocFunction)(size);
	else
		p = malloc(size);
	return p;
}

void xfree(void *p)
{
	if (sceHTTPLibFreeFunction)
		(*sceHTTPLibFreeFunction)(p);
	else
		free(p);
}

void *xrealloc(void *p, size_t size)
{
	void *q;

	if (sceHTTPLibReallocFunction)
		q = (*sceHTTPLibReallocFunction)(p, size);
	else
		q = realloc(p, size);
	return q;
}

/*
 * Internal pool: an arena of 4 KB pages, with a table of 16-byte page
 * descriptors at its start. Small sizes come from pages split into pieces of
 * 1 << type bytes (16 to 2048), larger ones from runs of whole pages.
 */

/* One descriptor per page. type: 0 = inner page of a block, 1 = first/last
 * page of an allocated block, 2 = first/last page of a free block, 4..11 =
 * page split into pieces of 1 << type bytes. On split pages next is the next
 * page of the same size, prev the offset of the first free piece and count
 * the free pieces. The last page of a free block holds (first word) the
 * index of its first page. */
struct PoolEntry {
	/* 0x00 */ char type;
	char pad_01[3];
	/* 0x04 */ int next;
	/* 0x08 */ int prev;
	/* 0x0C */ int count;  /* pages of the block, or free pieces */
};

/* Head of a free piece in a split page; next/prev are offsets in the page. */
struct PoolChunk {
	/* 0x00 */ int next;
	/* 0x04 */ int prev;
};

static struct PoolEntry *pool_table;
static char *pool_data;
static int pool_npages;
static int pool_free_list[8];
static pthread_mutex_t pool_lock;
static unsigned int pool_avail;
static unsigned int pool_low;      /* lowest pool_avail so far */

/* first free block, -1 = pool not initialized */
static int pool_head = -1;

static int pool_free(void *p);

/* Sets up the pool in the memory at base; returns 0, or -1 if too small. */
static int pool_init(unsigned int base, int size)
{
	char *aligned;
	int pad, npages, i, usable;

	aligned = (char *)((base + 4095) & ~4095);
	pad = aligned - (char *)base;
	npages = (size - pad) >> 12;
	i = (npages * 16 + 4095) >> 12;
	usable = npages - i;
	if (usable <= 0)
		return -1;
	pool_data = aligned + i * 4096;
	pool_npages = usable;
	pool_table = (struct PoolEntry *)aligned;
	memset(aligned, 0, npages * 16);
	pool_head = 0;
	pool_table->type = 2;
	pool_table->count = pool_npages;
	pool_table->next = -1;
	pool_table->prev = -1;
	for (i = 4; i < 12; i++)
		pool_free_list[i - 4] = -1;
	pool_avail = pool_npages << 12;
	pool_low = pool_avail;
	pthread_mutex_init(&pool_lock, 0);
	return 0;
}

static int pool_term(void)
{
	pthread_mutex_destroy(&pool_lock);
	return 0;
}

/* Returns a piece of the smallest fitting size class, or a run of whole
 * pages for sizes over 2048; 0 if none is free. */
static void *pool_malloc(size_t size)
{
	char *ret = 0;
	int *chunk = 0;
	unsigned int i, j;
	int k, shift;

	if (pool_head == -1)
		return 0;
	pthread_mutex_lock(&pool_lock);
	if (size > 2048) {
		k = (size + 4095) >> 12;
		size = k << 12;
		for (i = pool_head; i != -1; i = pool_table[i].next) {
			j = pool_table[i].count;
			if (k < j) {
				int n = i + j - k;
				ret = pool_data + (n << 12);
				pool_table[n].type = 1;
				pool_table[n].count = k;
				pool_table[i].count = j - k;
				pool_table[n - 1].type = 2;
				*(int *)(pool_data + ((n - 1) << 12)) = i;
				break;
			}
			if (j == k) {
				ret = pool_data + (i << 12);
				pool_table[i].type = 1;
				pool_table[i].count = j;
				j = pool_table[i].next;
				if (j != -1)
					pool_table[j].prev = pool_table[i].prev;
				j = pool_table[i].prev;
				if (j == -1)
					pool_head = pool_table[i].next;
				else
					pool_table[j].next = pool_table[i].next;
				break;
			}
		}
		if (i == -1)
			goto fail;
		j = ((unsigned int)(ret - pool_data) >> 12) + k - 1;
		pool_table[j].type = 1;
	} else {
		for (shift = 4; shift < 12 && (1 << shift) < size; shift++)
			;
		size = 1 << shift;
		k = pool_free_list[shift - 4];
		if (k != -1) {
			ret = pool_data + (k << 12) + pool_table[k].prev;
			pool_table[k].prev = *(int *)ret;
			if (*(int *)ret != -1)
				((int *)(pool_data + (k << 12) + *(int *)ret))[1] = -1;
			else
				pool_free_list[shift - 4] = pool_table[k].next;
			pool_table[k].count--;
		} else {
			int n;
			i = pool_head;
			k = pool_table[i].count;
			if (k > 1) {
				pool_table[i].count = k - 1;
				n = i + k;
				pool_table[n - 2].type = 2;
				*(int *)(pool_data + ((n - 2) << 12)) = i;
				i = n - 1;
			} else {
				j = pool_table[i].next;
				if (j != -1)
					pool_table[j].prev = -1;
				pool_head = pool_table[i].next;
			}
			ret = pool_data + (i << 12);
			pool_table[i].type = shift;
			pool_table[i].prev = size;
			pool_table[i].count = (1 << (12 - shift)) - 1;
			pool_free_list[shift - 4] = i;
			pool_table[i].next = -1;
			for (i = size; i < 4096; i += size) {
				chunk = (int *)(ret + i);
				chunk[0] = i + size;
				chunk[1] = i - size;
			}
			chunk[0] = -1;
			((int *)(ret + size))[1] = -1;
		}
	}
	pool_avail -= size;
	if (pool_avail < pool_low)
		pool_low = pool_avail;
	pthread_mutex_unlock(&pool_lock);
	return ret;

fail:
	pthread_mutex_unlock(&pool_lock);
	return 0;
}

/* Grows by allocate-copy-free; a block that is big enough is kept. */
static void *pool_realloc(void *p, size_t size)
{
	unsigned int page;
	size_t old;
	void *q;

	if (p == 0)
		return pool_malloc(size);
	if (size == 0) {
		pool_free(p);
		return 0;
	}
	page = (unsigned int)((char *)p - pool_data) >> 12;
	if (page >= pool_npages)
		return 0;
	if (pool_table[page].type == 2)
		return 0;
	if (pool_table[page].type == 0)
		return 0;
	pthread_mutex_lock(&pool_lock);
	if (pool_table[page].type == 1)
		old = pool_table[page].count << 12;
	else
		old = 1 << pool_table[page].type;
	pthread_mutex_unlock(&pool_lock);
	if (old >= size)
		return p;
	q = pool_malloc(size);
	if (q) {
		memcpy(q, p, old);
		pool_free(p);
	}
	return q;
}

/* Frees a piece or a block, merging free neighbours. */
static int pool_free(void *p)
{
	unsigned int page;
	size_t size = 0;
	char *base;
	int t, old, off, i, next, head, merged, link, cnt;

	page = (unsigned int)((char *)p - pool_data) >> 12;
	if (page >= pool_npages)
		return -1;
	if (pool_table[page].type == 2)
		return -1;
	if (pool_table[page].type == 0)
		return -1;
	pthread_mutex_lock(&pool_lock);
	t = pool_table[page].type;
	if (t != 1) {
		size = 1 << t;
		base = pool_data + (page << 12);
		old = pool_table[page].prev;
		off = (char *)p - base;
		pool_table[page].prev = off;
		pool_table[page].count++;
		if (old == -1) {
			pool_table[page].next = pool_free_list[t - 4];
			pool_free_list[t - 4] = page;
		} else {
			((struct PoolChunk *)(base + old))->prev = off;
		}
		((struct PoolChunk *)(base + off))->next = old;
		((struct PoolChunk *)(base + off))->prev = -1;
		if ((pool_table[page].count << t) == 4096) {
			link = pool_table[page].next;
			if (page == pool_free_list[t - 4]) {
				pool_free_list[t - 4] = link;
			} else {
				i = pool_free_list[t - 4];
				while (i != -1 && pool_table[i].next != page)
					i = pool_table[i].next;
				if (i == -1) {
					pthread_mutex_unlock(&pool_lock);
					return -1;
				}
				pool_table[i].next = link;
			}
			pool_table[page].type = 2;
			pool_table[page].count = 1;
		}
	} else {
		pool_table[page].type = 2;
	}
	if (pool_table[page].type == 2) {
		old = pool_table[page].count;
		if (size == 0)
			size = old << 12;
		merged = 0;
		if (page + old < pool_npages && pool_table[page + old].type == 2) {
			pool_table[page].next = pool_table[page + old].next;
			pool_table[page].prev = pool_table[page + old].prev;
			next = pool_table[page].next;
			off = pool_table[page].prev;
			if (next != -1)
				pool_table[next].prev = page;
			if (off == -1)
				pool_head = page;
			else
				pool_table[off].next = page;
			cnt = pool_table[page + old].count;
			pool_table[page].count += cnt;
			merged = 1;
		}
		if (page != 0 && (pool_table[page - 1].type == 2 || pool_table[page - 1].type == 0)) {
			head = *(int *)(pool_data + ((page - 1) << 12));
			if (merged) {
				next = pool_table[page].next;
				off = pool_table[page].prev;
				if (next != -1)
					pool_table[next].prev = off;
				if (off == -1)
					pool_head = next;
				else
					pool_table[off].next = next;
			}
			old = pool_table[page].count;
			pool_table[head].count += old;
			page = head;
			merged = 1;
		}
		old = pool_table[page].count;
		pool_table[page + old - 1].type = 2;
		*(int *)(pool_data + ((page + old - 1) << 12)) = page;
		if (!merged) {
			pool_table[page].next = pool_head;
			pool_table[page].prev = -1;
			if (pool_head != -1)
				pool_table[pool_head].prev = page;
			pool_head = page;
		}
	}
	pool_avail += size;
	pthread_mutex_unlock(&pool_lock);
	return 0;
}

/* Makes the library allocate from the pool at `pool` (no-op if pool is 0).
 * Returns 0, or -1 if the area is too small. */
int sceHTTPInitMemory(unsigned int pool, unsigned int size)
{
	if (pool == 0)
		return 0;
	if (pool_init(pool, size))
		return -1;
	sceHTTPLibMallocFunction = pool_malloc;
	sceHTTPLibReallocFunction = pool_realloc;
	sceHTTPLibFreeFunction = (void (*)(void *))pool_free;
	return 0;
}

int sceHTTPLibTerminateMemory(void)
{
	int result;

	if (sceHTTPLibMallocFunction != pool_malloc)
		result = 0;
	else
		result = pool_term();
	return result;
}

/* Stores the free bytes of the pool and their lowest value so far.
 * Returns -1 if the pool is not in use. */
int sceHTTPGetMemoryStatus(unsigned int *out_used, unsigned int *out_total)
{
	if (sceHTTPLibMallocFunction != pool_malloc)
		return -1;
	if (out_used == 0 || out_total == 0)
		return -1;
	pthread_mutex_lock(&pool_lock);
	*out_used = pool_avail;
	*out_total = pool_low;
	pthread_mutex_unlock(&pool_lock);
	return 0;
}

void *sceHTTPLibGetMemoryFunctionRA(void)
{
	return g_mem_hook_ra;
}

void sceHTTPFreeMemory(void *p)
{
	xfree(p);
}

void *sceHTTPSetMallocFunction(void *(*fn)(unsigned int))
{
	g_malloc_hook = fn;
	return (void *)(sceHTTPLibMallocFunction = malloc_trampoline);
}

void *sceHTTPSetReallocFunction(void *(*fn)(void *, unsigned int))
{
	g_realloc_hook = fn;
	return (void *)(sceHTTPLibReallocFunction = realloc_trampoline);
}

void *sceHTTPSetFreeFunction(void *(*fn)(void *))
{
	g_free_hook = fn;
	return (void *)(sceHTTPLibFreeFunction = (void (*)(void *))free_trampoline);
}

int sceHTTPSSetDefaultOption(struct sceHTTPClient *conn)
{
	conn->ssl_flags = 29;
	return 0;
}
