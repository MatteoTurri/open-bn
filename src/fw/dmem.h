/*
 * DMem: physically mapped memory regions from /dev/ps2mem, for DMA.
 */

#ifndef GUARD_DMEM_H
#define GUARD_DMEM_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Descriptor of a region, stored right after its page-aligned area and
   followed by the physical address of each page. */
struct DMemInfo {
	/* 0x00 */ int fd;
	/* 0x04 */ int page_size;  /* 4096 */
	/* 0x08 */ unsigned int page_mask;
	/* 0x0C */ int page_shift;
	/* 0x10 */ int pagenum;
	/* 0x14 */ unsigned int size;
	/* 0x18 */ unsigned int map_len;
	/* 0x1C */ void *map_addr;
	/* 0x20 */ int virt_base;
	/* 0x24 */ char pad_24[0x28 - 0x24];
	/* 0x28 */ unsigned int *phys_table;
	/* 0x2C */ pthread_mutex_t mutex;
}; /* size = 0x44 */

/* Output of DMemCheckContinuous. */
struct DMemContRange {
	/* 0x00 */ int pad_00;
	/* 0x04 */ unsigned int next_addr;
	/* 0x08 */ int pad_08;
	/* 0x0C */ unsigned int phys_addr;
	/* 0x10 */ unsigned int pages_ok;
	/* 0x14 */ unsigned int pages_short;
	/* 0x18 */ int pad_18;
}; /* size = 0x1C */

int DMemInit(void);
int DMemFinish(void);
void *DMemAlloc(unsigned int size);
int DMemFree(void *addr);
int DMemLock(int addr);
int DMemUnlock(int addr);
unsigned int DMemGetPhysAddr(struct DMemInfo *info, unsigned int addr);
int DMemCheckContinuous(struct DMemInfo *info, struct DMemContRange *out, unsigned int addr, unsigned int num_pages);
struct DMemInfo *DMemFindHitArea(int addr);

#ifdef __cplusplus
}
#endif

#endif /* GUARD_DMEM_H */
