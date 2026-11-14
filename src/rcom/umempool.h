/*
 * U_MemPool*: an allocation pool of RCOM that tracks its buffers so that they
 * can be freed (or zeroed and freed) all at once.
 */

#ifndef RCOM_UMEMPOOL_H
#define RCOM_UMEMPOOL_H

struct MemPool;

int U_MemPoolCreate(struct MemPool **out);
int U_MemPoolDestroy(struct MemPool **inout);
int U_MemPoolZeroAndDestroy(struct MemPool **inout);
void U_MemPoolReset(struct MemPool *pool, int zero);
int U_MemPoolMalloc(struct MemPool *pool, unsigned int size, void **out_buf);
int U_MemPoolMallocAndZero(struct MemPool *pool, unsigned int size, void **out_buf);
int U_MemPoolMallocAndCopy(struct MemPool *pool, unsigned int size, const void *src, void **out_buf);
int U_MemPoolAdoptBuffer(struct MemPool *pool, unsigned int size, void *buf);
int U_MemPoolRealloc(struct MemPool *pool, unsigned int new_size, void **inout_buf);
int U_MemPoolZeroAndFree(struct MemPool *pool, void **inout_buf);
int U_MemPoolFree(struct MemPool *pool, void **inout_buf);

#endif
