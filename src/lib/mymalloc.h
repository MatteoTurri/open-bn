/*
 * myMalloc_*: allocator over one static arena, 16-byte units (mymalloc.c).
 */

#ifndef MYMALLOC_H
#define MYMALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

void myMalloc_InitHeap(void *heap, unsigned size);
void *myMalloc_Malloc(unsigned size);
void myMalloc_Free(void *ptr);
void *_stdMalloc(unsigned size);
void _stdFree(void *ptr);
void *myMalloc_Malloc_(unsigned nbytes);
void myMalloc_Free_(void *ap);

#ifdef __cplusplus
}
#endif

#endif
