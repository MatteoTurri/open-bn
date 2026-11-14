/*
 * U_MemPool*: RCOM allocation pool that tracks its buffers so that they can
 * be freed (or zeroed and freed) all at once. Allocation failures return 600.
 */

#include "../../ref/rcom-1.1.0/bsafe.h"
#include "umempool.h"

struct MemPoolEntry {
	/* 0x00 */ void *data;
	/* 0x04 */ unsigned int size;
	/* 0x08 */ void (*free_cb)(void *);  /* if set, called instead of T_free (adopted object) */
};

struct MemPool {
	/* 0x00 */ int count;  /* entries in use */
	/* 0x04 */ int capacity;  /* entries allocated */
	/* 0x08 */ struct MemPoolEntry *entries;
};

static struct MemPoolEntry *mempool_find_entry(struct MemPool *, void *);
static int mempool_adopt_impl(struct MemPool *, unsigned int, void *, void (*)(void *));

int U_MemPoolCreate(struct MemPool **out)
{
	struct MemPool *pool = T_malloc(12);

	*out = pool;
	if (pool == 0)
		return 600;
	T_memset(pool, 0, 12);
	return 0;
}

int U_MemPoolDestroy(struct MemPool **inout)
{
	U_MemPoolReset(*inout, 0);
	T_free((*inout)->entries);
	T_free(*inout);
	*inout = 0;
	return 0;
}

int U_MemPoolZeroAndDestroy(struct MemPool **inout)
{
	U_MemPoolReset(*inout, 1);
	T_free((*inout)->entries);
	T_free(*inout);
	*inout = 0;
	return 0;
}

/* Frees every tracked buffer (zeroed first if `zero`) or hands it to its free_cb; empties the pool. */
void U_MemPoolReset(struct MemPool *pool, int zero)
{
	unsigned int count = pool->count;
	struct MemPoolEntry *e = pool->entries;

	while (count-- > 0) {
		if (e->data != 0) {
			if (e->free_cb != 0) {
				e->free_cb(e->data);
				e->data = 0;
			} else {
				if (zero != 0)
					T_memset(e->data, 0, e->size);
				T_free(e->data);
				e->data = 0;
			}
		}
		e++;
	}
	pool->count = 0;
}

int U_MemPoolMalloc(struct MemPool *pool, unsigned int size, void **out_buf)
{
	void *buf = T_malloc(size);
	int result;

	*out_buf = buf;
	if (buf == 0)
		return 600;
	result = U_MemPoolAdoptBuffer(pool, size, buf);
	if (result != 0) {
		T_free(*out_buf);
	}
	return result;
}

int U_MemPoolMallocAndZero(struct MemPool *pool, unsigned int size, void **out_buf)
{
	int result = U_MemPoolMalloc(pool, size, out_buf);

	if (result != 0)
		return result;
	T_memset(*out_buf, 0, size);
	return 0;
}

int U_MemPoolMallocAndCopy(struct MemPool *pool, unsigned int size, const void *src, void **out_buf)
{
	int result = U_MemPoolMalloc(pool, size, out_buf);

	if (result != 0)
		return result;
	T_memcpy(*out_buf, src, size);
	return 0;
}

int U_MemPoolAdoptBuffer(struct MemPool *pool, unsigned int size, void *buf)
{
	return mempool_adopt_impl(pool, size, buf, 0);
}

/* Appends an entry, growing the array by 10 slots when it is full. */
static int mempool_adopt_impl(struct MemPool *pool, unsigned int size, void *data, void (*free_cb)(void *))
{
	int n;

	if ((unsigned int)(pool->count + 1) > (unsigned int)pool->capacity) {
		int new_cap = pool->count + 10;
		struct MemPoolEntry *new_arr = T_malloc(new_cap * sizeof(struct MemPoolEntry));

		if (new_arr == 0)
			return 600;
		T_memcpy(new_arr, pool->entries, pool->count * sizeof(struct MemPoolEntry));
		T_free(pool->entries);
		pool->entries = new_arr;
		pool->capacity = new_cap;
	}
	pool->entries[pool->count].data = data;
	pool->entries[pool->count].size = size;
	n = pool->count;
	pool->entries[n].free_cb = free_cb;
	pool->count = n + 1;
	return 0;
}

/*
 * Moves a tracked buffer to a new block of `new_size` bytes and zeroes the
 * old one; adopted objects cannot be resized.
 */
int U_MemPoolRealloc(struct MemPool *pool, unsigned int new_size, void **inout_buf)
{
	struct MemPoolEntry *e;
	void *new_buf;
	unsigned int copy_size;

	if (*inout_buf == 0)
		return U_MemPoolMalloc(pool, new_size, inout_buf);

	e = mempool_find_entry(pool, *inout_buf);
	if (e == 0)
		return 600;
	if (e->free_cb != 0)
		return 600;

	new_buf = T_malloc(new_size);
	if (new_buf == 0)
		return 600;

	copy_size = (new_size < e->size) ? new_size : e->size;
	T_memcpy(new_buf, *inout_buf, copy_size);
	T_memset(*inout_buf, 0, e->size);
	T_free(*inout_buf);
	*inout_buf = new_buf;
	e->data = new_buf;
	e->size = new_size;
	return 0;
}

int U_MemPoolZeroAndFree(struct MemPool *pool, void **inout_buf)
{
	struct MemPoolEntry *e = mempool_find_entry(pool, *inout_buf);

	if (e == 0)
		return 600;
	if (e->free_cb != 0) {
		e->free_cb(e->data);
	} else {
		T_memset(*inout_buf, 0, e->size);
		T_free(*inout_buf);
	}
	e->data = 0;
	*inout_buf = 0;
	return 0;
}

int U_MemPoolFree(struct MemPool *pool, void **inout_buf)
{
	struct MemPoolEntry *e = mempool_find_entry(pool, *inout_buf);

	if (e == 0)
		return 600;
	if (e->free_cb != 0) {
		e->free_cb(e->data);
	} else {
		T_free(*inout_buf);
	}
	e->data = 0;
	*inout_buf = 0;
	return 0;
}

/* Entry holding `data`, or 0. */
static struct MemPoolEntry *mempool_find_entry(struct MemPool *pool, void *data)
{
	unsigned int i;
	struct MemPoolEntry *e;

	if (data == 0)
		return 0;
	i = pool->count;
	e = pool->entries;
	while (i-- > 0) {
		if (e->data == data)
			return e;
		e++;
	}
	return 0;
}

/* Tracks `obj`, to be released by `free_cb` instead of T_free. */
int U_MemPoolAdoptObject(struct MemPool *pool, void *obj, void (*free_cb)(void *))
{
	return mempool_adopt_impl(pool, 0, obj, free_cb);
}
