/*
 * myMalloc2_*: allocator over one static arena, 128-byte units, thread safe (mymalloc2.c).
 */

#ifndef MYMALLOC2_H
#define MYMALLOC2_H

#ifdef __cplusplus
extern "C" {
#endif

void myMalloc2_InitHeap(void *heap, unsigned size);
void *myMalloc2_Malloc(unsigned size);
void myMalloc2_Free(void *ptr);
void *_stdMalloc2(unsigned size);
void _stdFree2(void *ptr);
void *myMalloc2_Malloc_(unsigned nbytes);
void myMalloc2_Free_(void *ap);

#ifdef __cplusplus
}
#endif

#endif
