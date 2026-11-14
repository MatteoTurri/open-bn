/*
 * DMem: "direct memory" regions mapped from /dev/ps2mem (mmap + a physical
 * address table per page), kept in an array sorted by start address.
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "lib/ps2dev_ext.h"
#include "dmem.h"

struct DMemRegionEntry {
	/* 0x00 */ int start;
	/* 0x04 */ int end;
	/* 0x08 */ struct DMemInfo *info;
	/* 0x0C */ int pad_0C;
}; /* size = 0x10 */

static pthread_mutex_t g_dmem_mutex;
static int g_dmem_region_count;
static int g_dmem_last_hit_index;
static struct DMemRegionEntry g_dmem_regions[256];
static int g_dmem_page_size;
static int g_dmem_page_mask;
static int g_dmem_page_shift;

/* The page shift is computed as the popcount of the page mask. */
int DMemInit(void)
{
	int n;

	g_dmem_page_size = 4096;
	g_dmem_page_mask = 4095;
	g_dmem_page_shift = 0;
	n = 4095;
	do {
		g_dmem_page_shift = g_dmem_page_shift + (n & 1);
		n = n >> 1;
	} while (n != 0);
	g_dmem_region_count = 0;
	g_dmem_last_hit_index = 0;
	memset(g_dmem_regions, 0, sizeof(g_dmem_regions));
	return 0;
}

int DMemFinish(void)
{
	int i = 0;
	struct DMemRegionEntry *e;

	if (g_dmem_region_count > 0) {
		e = g_dmem_regions;
		do {
			close(e->info->fd);
			munmap(e->info->map_addr, e->info->map_len);
			i++;
			e++;
		} while (i < g_dmem_region_count);
	}
	return 0;
}

/* Maps a region of `size` bytes from /dev/ps2mem, followed by its DMemInfo
 * descriptor and its physical address table (filled by the driver), and inserts
 * it in the sorted region array. Returns the page-aligned virtual address. */
void *DMemAlloc(unsigned int size)
{
	struct DMemRegionEntry e;
	struct DMemInfo *mgr;
	unsigned int aligned;
	void *mem;
	int fd;
	int pagenum;
	int real_size;
	unsigned int t;
	int i;
	int n;

	if (g_dmem_region_count >= 256)
		return 0;
	pthread_mutex_lock(&g_dmem_mutex);
	fd = open("/dev/ps2mem", O_RDWR);
	if (fd < 0) {
		printf("Error A\n", fd);
		/* NOTE: bug: returns with g_dmem_mutex still locked. */
		return 0;
	}
	printf("ps2mem fd %d\n", fd);
	fcntl(fd, F_SETFD, FD_CLOEXEC);
	pagenum = ((int)size >> g_dmem_page_shift) + 1;
	printf("pagenum %d\n", pagenum);
	real_size = size + pagenum * 4 + g_dmem_page_size + 68;
	i = 0;
	printf("real_size 0x%08x byte\n", real_size);
	mem = mmap(0, real_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memset(mem, 0, real_size);
	printf("MMAP: 0x%8.8x\n", mem);
	t = (unsigned int)mem - 1;
	aligned = (t + g_dmem_page_size) & -g_dmem_page_size;
	printf("MAlign: 0x%8.8x\n", aligned);
	mgr = (struct DMemInfo *)(aligned + size);
	printf("MemMgr: 0x%8.8x\n", mgr);
	mgr->fd = fd;
	mgr->page_size = g_dmem_page_size;
	mgr->page_mask = g_dmem_page_mask;
	mgr->page_shift = g_dmem_page_shift;
	mgr->pagenum = pagenum;
	mgr->size = size;
	mgr->map_len = real_size;
	mgr->map_addr = mem;
	mgr->virt_base = aligned;
	mgr->phys_table = (unsigned int *)(mgr + 1);
	printf("Table: 0x%8.8x\n", mgr->phys_table);
	ioctl(mgr->fd, PS2IOC_PHYSADDR, mgr->phys_table);
	printf("get addr table\n");
	e.start = aligned;
	e.end = (int)mgr;
	e.info = mgr;
	if (g_dmem_region_count != 0) {
		n = g_dmem_region_count;
		for (i = n; i > 0; i--)
			if (g_dmem_regions[i - 1].start < e.start)
				break;
		if (i < n)
			memmove(&g_dmem_regions[i + 1], &g_dmem_regions[i], (n - i) * sizeof(g_dmem_regions[0]));
	}
	g_dmem_regions[i] = e;
	g_dmem_region_count = g_dmem_region_count + 1;
	printf("dmem num %d, cache_id %d, regist_id %d\n", g_dmem_region_count, g_dmem_last_hit_index, i);
	pthread_mutex_unlock(&g_dmem_mutex);
	return (void *)mgr->virt_base;
}

/* Releases the region covering `addr`: removes it from the array, adjust the hit
 * cache, closes the fd and unmap. All under the global DMem mutex. */
int DMemFree(void *addr)
{
	int ret = 0;
	struct DMemInfo *info;
	int count;
	int idx;

	pthread_mutex_lock(&g_dmem_mutex);
	info = DMemFindHitArea((int)addr);
	if (info == 0) {
		ret = -1;
	} else {
		count = g_dmem_region_count;
		idx = g_dmem_last_hit_index;
		if (idx < count - 1)
			memmove(&g_dmem_regions[idx], &g_dmem_regions[idx + 1],
			    (count - idx - 1) * sizeof(g_dmem_regions[0]));
		if (g_dmem_last_hit_index != idx)
			g_dmem_last_hit_index = idx;
		else if (idx != 0)
			g_dmem_last_hit_index = idx - 1;
		g_dmem_region_count = g_dmem_region_count - 1;
		printf("DMemFree: 0x%8.8x, dmemnum%d, cache%d\n", info->map_addr, g_dmem_region_count,
		    g_dmem_last_hit_index);
		close(info->fd);
		munmap(info->map_addr, info->map_len);
	}
	pthread_mutex_unlock(&g_dmem_mutex);
	return ret;
}

int DMemLock(int addr)
{
	struct DMemInfo *info = DMemFindHitArea(addr);

	if (info == 0)
		return -1;
	pthread_mutex_lock(&info->mutex);
	return 0;
}

int DMemUnlock(int addr)
{
	struct DMemInfo *info = DMemFindHitArea(addr);

	if (info == 0)
		return -1;
	pthread_mutex_unlock(&info->mutex);
	return 0;
}

unsigned int DMemGetPhysAddr(struct DMemInfo *info, unsigned int addr)
{
	int off = (int)addr - info->virt_base;
	unsigned int page_base = info->phys_table[off >> info->page_shift];

	return page_base + ((unsigned int)off & info->page_mask);
}

/* How many 16-byte units are contiguous from `addr` to the end of its page.
 * Returns 1 if num_pages fit; else 0 with the address and physical address of
 * the next page, the units available and those still missing. */
int DMemCheckContinuous(struct DMemInfo *info, struct DMemContRange *out, unsigned int addr, unsigned int num_pages)
{
	unsigned int off = addr & info->page_mask;
	int avail = (info->page_size - (int)off) >> 4;

	if ((int)num_pages <= avail) {
		out->next_addr = 0;
		out->phys_addr = 0;
		out->pages_ok = num_pages;
		out->pages_short = 0;
		return 1;
	}
	out->next_addr = addr + avail * 16;
	out->phys_addr = DMemGetPhysAddr(info, out->next_addr);
	out->pages_ok = avail;
	out->pages_short = num_pages - avail;
	return 0;
}

/* Binary search of the region covering `addr`, with a one-entry cache of the
 * last hit. */
struct DMemInfo *DMemFindHitArea(int addr)
{
	struct DMemRegionEntry *cached = &g_dmem_regions[g_dmem_last_hit_index];
	struct DMemInfo *result = 0;
	int lo, hi;

	if (addr >= cached->start && addr < cached->end) {
		result = cached->info;
		goto out;
	}
	lo = 0;
	hi = g_dmem_region_count - 1;
	while (lo <= hi) {
		int mid = (lo + hi) >> 1;
		struct DMemRegionEntry *e = &g_dmem_regions[mid];

		if (addr < e->start) {
			hi = mid - 1;
		} else if (addr < e->end) {
			g_dmem_last_hit_index = mid;
			result = e->info;
			goto out;
		} else {
			lo = mid + 1;
		}
	}
out:
	return result;
}
