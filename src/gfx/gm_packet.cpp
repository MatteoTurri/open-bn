/*
 * gm_packet: DMA packet buffers and heap of the renderer on top of the VPU1
 * device (/dev/ps2mem), and the gm_window GS render target.
 */

#pragma implementation
#pragma implementation "gm_window_inline.h"
#include "gm_packet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/ps2/dev.h>
#include <linux/ps2/ee.h>
#include "lib/ps2dev_ext.h"
#include "ps2vpu.h"
#include "fw/fw.h"
#include "fw/fw_gsenv.h"
#include "fw/fw_pk.h"
#include "gm_window.h"
#include <iostream.h>
#include "gm_window_inline.h"

int gm_packet::init(int dbufSize, int heapSize)
{
	term();

	if (dbufSize == 0)
		dbufSize = 0x80000;
	if (heapSize == 0)
		heapSize = 0x10000;
	fDbufSize = dbufSize;
	fHeapSize = heapSize;

	if (fChannel < 0) {
		fChannel = open("/dev/ps2mem", O_RDWR);
		if (fChannel < 0)
			return 0;
	}

	fcntl(fChannel, F_SETFD, FD_CLOEXEC);

	fVpu = g_vpu1;
	if (fVpu == 0)
		return 0;

	int pool = ps2_vpu_fd(fVpu);

	void *p = mmap(0, 0x8000, PROT_READ | PROT_WRITE, MAP_SHARED, pool, 0);
	if (p == (void *)-1)
		p = 0;
	fExtraBuf = (char *)p;

	int total = fDbufSize * 2 + fHeapSize;
	p = mmap(0, total, PROT_READ | PROT_WRITE, MAP_SHARED, fChannel, 0);
	if (p == (void *)-1)
		return 0;

	int npages = (total + 4095) / 4096;

	fMemBase = (char *)p;
	fHeapMem = fMemBase + fDbufSize * 2;

	fPhysTable = new int[npages];
	ioctl(fChannel, PS2IOC_PHYSADDR, fPhysTable);

	init_heap();

	fParity = 0;
	unk_38 = 0;
	fPutStart = 0;
	fPutEnd = 0;
	fFlags = 1;
	fCursor = fMemBase;
	fTagCursor = fMemBase + fDbufSize - 4096;
	fChainStart = fTagCursor;
	return 1;
}

void gm_packet::flip()
{
	if (!(fFlags & 1))
		return;
	flush_packet();
	if (fFlags & 2) {
		int *p;
		int n;
		end_list(p, n);
	}

	char *cur = fTagCursor;
	if (cur != fChainStart) {
		*(unsigned *)cur = PS2_DMATAG_RET << 28;
		*(unsigned *)(cur + 4) = 0;
		cur += 16;
		fwPkAddDmaChain_withCall_physAddr(get_phys_addr((int *)fChainStart));
		if (((unsigned)(long)cur & 0xfff) == 0)
			cur -= 0x2000;
		fTagCursor = cur;
		fChainStart = cur;
	}

	char *base = fMemBase;
	if (fParity & 1)
		base += fDbufSize;
	char *lim = (char *)((unsigned)(long)fTagCursor & ~0xfff);
	fLastDbufUsed = fCursor - base;
	fLastDbufCap = base + fDbufSize - lim;
	fParity++;
	char *next = fMemBase;
	next += (fParity & 1) * fDbufSize;
	fCursor = next;
	fTagCursor = next + fDbufSize - 4096;
	fChainStart = fTagCursor;

	cleanup_heap();
}

/* Frees the pending blocks older than fCleanupDelay generations (an inline
   copy of free_heap); the younger ones stay pending. */
void gm_packet::cleanup_heap()
{
	if (!(fFlags & 1))
		return;
	if (fPendingList == 0)
		return;

	gm_heap_block *blk = fPendingList;
	gm_heap_block *prevKept = 0;

	for (;;) {
		int age = fParity - blk->gen;
		gm_heap_block *next = blk->pend;

		if (age >= fCleanupDelay) {
			char *up = (char *)blk + 16;
			if (!(fFlags & 1))
				goto skip;
			if (up == 0)
				goto skip;
			{
				gm_heap_block *node = (gm_heap_block *)(up - 16);
				gm_heap_block *prev = fHeapFreeList;
				gm_heap_block *cur = prev->next;

				while (cur != 0 && !(node < cur)) {
					prev = cur;
					cur = cur->next;
				}

				if ((char *)node + node->size * 16 == (char *)cur && cur != 0) {
					node->size += cur->size;
					node->next = cur->next;
				} else {
					node->next = cur;
				}

				if ((char *)prev + prev->size * 16 == (char *)node) {
					if (prev != fHeapFreeList) {
						prev->size += node->size;
						prev->next = node->next;
					} else {
						prev->next = node;
					}
				} else {
					prev->next = node;
				}
			}
		} else {
			if (prevKept != 0)
				prevKept->pend = blk;
			else
				fPendingList = blk;
			prevKept = blk;
		}

skip:
		blk = next;
		if (blk == 0)
			break;
	}

	if (prevKept != 0)
		prevKept->pend = 0;
	else
		fPendingList = 0;
}

void gm_packet::flush_packet()
{
	if (!(fFlags & 1))
		return;
	char *putStart = (char *)fPutStart;
	if (putStart == 0)
		return;

	int wasListing = fFlags & 2;
	fPutStart = 0;
	char *putEnd = (char *)fPutEnd;
	fPutEnd = 0;
	char *cur = fTagCursor;
	int remaining = (int)(putEnd - putStart);

	if (!wasListing) {
		if (remaining > 0) {
			do {
				if (((unsigned)(long)(cur + 16) & 0xfff) == 0) {
					char *wrapped = cur - 8176;
					*(unsigned *)cur = PS2_DMATAG_NEXT << 28;
					int r = (int)get_phys_addr((int *)wrapped);
					*(unsigned *)(cur + 4) = r;
					cur = wrapped;
				}
				int chunkBytes = 4096 - ((unsigned)(long)putStart & 0xfff);
				if (remaining < chunkBytes)
					chunkBytes = remaining;
				remaining -= chunkBytes;
				int qwords = chunkBytes / 16;
				*(unsigned *)cur = (PS2_DMATAG_REF << 28) | qwords;
				int phys = (int)get_phys_addr((int *)putStart);
				*(unsigned *)(cur + 4) = phys;
				cur += 16;
				putStart += chunkBytes;
			} while (remaining > 0);
		}
	} else {
		if (((unsigned)(long)cur & 0xfff) == 0)
			cur = cur - 8192;
		int chunk = remaining / 16;
		*(unsigned *)cur = (PS2_DMATAG_REF << 28) | chunk;
		*(char **)(cur + 4) = putStart;
		cur += 16;
	}

	fTagCursor = cur;
}

inline void *gm_alloc_heap(gm_packet *pk, int size)
{
	if (!(pk->fFlags & 1))
		return 0;
	if (size == 0)
		return 0;

	gm_heap_block *prev = pk->fHeapFreeList;
	gm_heap_block *blk;

	size = (size + 15) / 16 + 1;

	blk = prev->next;
	while (blk != 0) {
		gm_heap_block *next = blk->next;

		if (blk->size >= size) {
			if (blk->size == size) {
				prev->next = next;
			} else {
				gm_heap_block *rest = (gm_heap_block *)((char *)blk + size * 16);
				prev->next = rest;
				rest->next = next;
				rest->size = blk->size - size;
				blk->size = size;
			}
			blk->gen = pk->fParity;
			return (char *)blk + 16;
		}
		prev = blk;
		blk = next;
	}
	return 0;
}

/* Closes the display list opened by begin_list(): allocates in the heap the
   DMA tag buffer plus a copy of the segment [fListDataStart, fCursor), rebuilds the
   chain with rebuild_list_dmatag and copies the segment. */
int gm_packet::end_list(int *&p, int &n)
{
	if ((fFlags & 3) != 3) {
		p = 0;
		n = 0;
		return 0;
	}

	flush_packet();
	fFlags &= ~2;
	p = 0;
	n = 0;

	if (fTagCursor != fListTagStart) {
		int count = count_list_dmatag();
		int extra = (count + 254) / 255 + 1;
		int tagBytes = (count + extra) * 16;
		int used = fCursor - fListDataStart;
		int size = tagBytes + used;
		n = size;
		p = (int *)gm_alloc_heap(this, size);
		if (p == 0) {
			fCursor = fListDataStart;
			fTagCursor = fListTagStart;
			return 0;
		}
		int *tail = (int *)((char *)p + tagBytes);
		rebuild_list_dmatag(p, tail);
		memcpy(tail, fListDataStart, used);
	}

	fCursor = fListDataStart;
	fTagCursor = fListTagStart;
	return 1;
}

inline void gm_print_phys_table(gm_packet *pk)
{
	int total = pk->fDbufSize * 2 + pk->fHeapSize;
	int npages = (total + 4095) / 4096;
	for (int i = 0; i < npages; i++) {
		printf("%08x\n", pk->fPhysTable[i]);
	}
	fflush(stdout);
}

inline void gm_print_heap(gm_packet *pk)
{
	printf("%p - %p ...\n", pk->fHeapMem, pk->fHeapMem + pk->fHeapSize);

	gm_heap_block *minNode = pk->fHeapFreeList;
	char *heapMem = pk->fHeapMem;
	gm_heap_block *cur = minNode;
	gm_heap_block *first = minNode;
	do {
		if ((unsigned long)cur < (unsigned long)minNode)
			minNode = cur;
		cur = cur->next;
	} while (cur != first);

	cur = minNode;
	do {
		if (cur == pk->fHeapFreeList)
			printf("* ");
		else
			printf("  ");

		int size = cur->size;
		int startIdx = (int)(((char *)cur - heapMem) >> 4);
		int endIdx = startIdx + size;
		printf("%3d - %3d ( %3d QW )\n", startIdx, endIdx, size);

		cur = cur->next;
	} while (cur != minNode);
}

/* Walks the DMA chain from fChainStart and prints each tag (ID = word0 >> 28, values
   above 7 print as "----"), with the tag address translated to physical and
   the ADDR field to virtual. */
void gm_packet::print_chain()
{
	static char *tagName[] = { "REFE", "CNT ", "NEXT", "REF ", "REFS", "CALL", "RET ", "END ", "----" };
	int *p = (int *)fChainStart;
	int *ret = 0;

	while (p) {
		int qwc = p[0];
		int addr = p[1];

		char *virt;
		int total = fDbufSize * 2 + fHeapSize;
		int npages = (total + 4095) / 4096;
		for (int i = 0; i < npages; i++) {
			int off = addr - fPhysTable[i];
			if ((unsigned)off < 4096) {
				virt = fMemBase + i * 4096 + off;
				goto found;
			}
		}
		virt = 0;
found:

		int id = qwc >> 28;
		qwc = (qwc & 0xffff) << 4;
		if ((unsigned)id > 7)
			id = 8;

		int *cur = p;
		switch (id) {
		case 1:
			p += qwc / 4 + 4;
			break;
		case 2:
			p = (int *)virt;
			qwc = 1;
			break;
		case 3:
		case 4:
			p += 4;
			break;
		case 5:
			ret = p + 4;
			p = (int *)virt;
			break;
		case 6:
			p = ret;
			break;
		case 0:     /* REFE */
		case 7:     /* END */
		default:
			p = 0;
			break;
		}

		if (qwc == 0)
			virt = fMemBase;

		int off = (char *)cur - fMemBase;
		printf("%08x : %s %08x ( %08x - %08x bytes )\n", fPhysTable[off / 4096] + (off & 0xfff), tagName[id],
		    virt - fMemBase, addr, qwc);
	}
}

gm_packet::gm_packet()
{
	fChannel = -1;
	unk_04 = 0;
	fVpu = 0;
	fExtraBuf = 0;
	fFlags = 0;
	fDbufSize = 0;
	fHeapSize = 0;
	fCleanupDelay = 4;
	fPhysTable = 0;
	fMemBase = 0;
	fHeapMem = 0;
	fLastDbufUsed = 0;
	fLastDbufCap = 0;
}

gm_packet::~gm_packet()
{
	term();
}

void gm_packet::term()
{
	if (!(fFlags & 1))
		return;

	if (fChannel >= 0) {
		if (fMemBase != 0) {
			int sz = fDbufSize * 2 + fHeapSize;
			munmap(fMemBase, sz);
			fMemBase = 0;
			fHeapMem = 0;
		}
		close(fChannel);
		fChannel = -1;
	}

	if (fExtraBuf != 0) {
		munmap(fExtraBuf, 0x8000);
		fExtraBuf = 0;
	}

	fVpu = 0;
	unk_04 = 0;
	if (fPhysTable != 0) {
		delete[] fPhysTable;
		fPhysTable = 0;
	}

	fFlags &= ~1;
}

void gm_packet::send()
{
	if (!(fFlags & 1))
		return;
	flush_packet();
	if (fFlags & 2) {
		int *pa;
		int na;
		end_list(pa, na);
	}

	char *cur = fTagCursor;
	if (cur != fChainStart) {
		*(unsigned *)cur = PS2_DMATAG_RET << 28;
		*(unsigned *)(cur + 4) = 0;
		cur += 16;
		int r = (int)get_phys_addr((int *)fChainStart);
		fwPkAddDmaChain_withCall_physAddr(r);
		unsigned rem = (unsigned)(long)cur & 0xfff;
		char *newcur = cur - 0x2000;
		if (rem == 0)
			cur = newcur;
		fTagCursor = cur;
		fChainStart = cur;
	}
}

void gm_packet::wait()
{
	int fd = ps2_vpu_fd(fVpu);
	ioctl(fd, PS2IOC_SENDQCT, 1);
}

void gm_packet::init_heap()
{
	int n = fHeapSize / 16 - 1;
	gm_heap_block *base = (gm_heap_block *)fHeapMem;
	gm_heap_block *first = base + 1;

	base->next = first;
	base->size = 1;
	first->next = 0;
	first->size = n;

	fHeapFreeList = base;
	fPendingList = 0;
}

void *gm_packet::alloc_heap(int size)
{
	if (!(fFlags & 1))
		return 0;
	if (size == 0)
		return 0;

	gm_heap_block *prev = fHeapFreeList;
	gm_heap_block *blk;

	size = (size + 15) / 16 + 1;

	blk = prev->next;
	while (blk != 0) {
		gm_heap_block *next = blk->next;

		if (blk->size >= size) {
			if (blk->size == size) {
				prev->next = next;
			} else {
				gm_heap_block *rest = (gm_heap_block *)((char *)blk + size * 16);
				prev->next = rest;
				rest->next = next;
				rest->size = blk->size - size;
				blk->size = size;
			}
			blk->gen = fParity;
			return (char *)blk + 16;
		}
		prev = blk;
		blk = next;
	}
	return 0;
}

void gm_packet::free_heap(void *p)
{
	if (!(fFlags & 1))
		return;
	if (!p)
		return;

	gm_heap_block *node = (gm_heap_block *)((char *)p - 16);
	gm_heap_block *prev = fHeapFreeList;
	gm_heap_block *cur = prev->next;

	while (cur != 0 && !(node < (void *)cur)) {
		prev = cur;
		cur = cur->next;
	}

	if ((char *)node + node->size * 16 == (char *)cur && cur != 0) {
		node->size += cur->size;
		node->next = cur->next;
	} else {
		node->next = cur;
	}

	if ((char *)prev + prev->size * 16 == (char *)node) {
		if (prev != fHeapFreeList) {
			prev->size += node->size;
			prev->next = node->next;
		} else {
			prev->next = node;
		}
	} else {
		prev->next = node;
	}
}

void gm_packet::free_heap2(void *p)
{
	if (!(fFlags & 1))
		return;
	if (!p)
		return;

	gm_heap_block *blk = (gm_heap_block *)((char *)p - 16);
	blk->pend = fPendingList;
	fPendingList = blk;
}

int gm_packet::max_heap_space()
{
	if (!(fFlags & 1))
		return 0;

	gm_heap_block *cur = fHeapFreeList->next;
	int best = 0;
	while (cur != 0) {
		if (best < cur->size)
			best = cur->size;
		cur = cur->next;
	}
	return best * 16 - 16;
}

void *gm_packet::alloc_packet(int size)
{
	if (!(fFlags & 1))
		return 0;
	char *p = fCursor;
	fCursor = p + size;
	return p;
}

void gm_packet::put_packet(int *p, int size)
{
	if (!(fFlags & 1))
		return;
	int *end = (int *)((char *)p + size);
	if (p == fPutEnd) {
		fPutEnd = end;
	} else {
		flush_packet();
		fPutStart = p;
		fPutEnd = end;
	}
}

void gm_packet::begin_list()
{
	if (!(fFlags & 1))
		return;
	if (fFlags & 2)
		return;
	flush_packet();
	fFlags |= 2;
	fListDataStart = fCursor;
	fListTagStart = fTagCursor;
}

void gm_packet::put_list(int *p)
{
	if (!(fFlags & 1))
		return;
	if (fFlags & 2)
		return;
	if (p == 0)
		return;

	flush_packet();
	char *cur = fTagCursor;
	if (((unsigned)(long)(cur + 16) & 0xfff) != 0) {
		/* no wrap */
	} else {
		char *wrapped = cur - 8176;
		*(unsigned *)cur = PS2_DMATAG_NEXT << 28;
		int r = (int)get_phys_addr((int *)wrapped);
		*(unsigned *)(cur + 4) = r;
		cur = wrapped;
	}
	*(unsigned *)cur = PS2_DMATAG_CALL << 28;
	int phys = (int)get_phys_addr((int *)p);
	*(unsigned *)(cur + 4) = phys;
	char *newcur = cur + 16;
	fTagCursor = newcur;
	p[-1] = fParity;
}

int gm_packet::count_list_dmatag()
{
	int *p = (int *)fListTagStart;
	int count = 0;
	while (p != (int *)fTagCursor) {
		unsigned off = (unsigned)(long)p & 0xfff;
		if (off == 0)
			p = (int *)((char *)p - 8192);
		int tag = *p;
		int bytes = (tag & 0x00ffffff) << 4;
		p = (int *)((char *)p + 16);
		int next = count + 1;
		int extra = (bytes + 4095) / 4095;
		count = next + extra;
	}
	return count;
}

/* Rebuilds in `list` the chain of DMA "ref" tags of the list [fListTagStart, fTagCursor),
   relocating by src - fListDataStart the addresses that still point into the ring
   (below fHeapMem) and splitting each tag at 4096-byte physical pages (a REF
   continuation NEXT tag when the destination crosses a page). */
void gm_packet::rebuild_list_dmatag(int *list, int *src)
{
	int *dst = list;
	char *tag = fListTagStart;
	int len0 = (char *)src - fListDataStart;

	if (tag != fTagCursor) {
		do {
			if (((unsigned)(long)tag & 0xfff) == 0)
				tag -= 8192;

			int addr = *(int *)(tag + 4);
			unsigned word0 = *(unsigned *)tag;
			tag += 16;

			int bytes = (int)((word0 & 0x00ffffff) << 4);
			if ((unsigned)(long)addr < (unsigned)(long)fHeapMem)
				addr += len0;

			while (bytes > 0) {
				if (((unsigned)(long)(dst + 4) & 0xfff) == 0) {
					dst[0] = PS2_DMATAG_NEXT << 28;
					dst[1] = get_phys_addr(dst + 4);
					dst += 4;
				}

				int chunk = 4096 - (addr & 0xfff);
				if (bytes < chunk)
					chunk = bytes;
				bytes -= chunk;
				dst[0] = (PS2_DMATAG_REF << 28) | (chunk / 16);
				dst[1] = get_phys_addr((int *)addr);
				dst += 4;
				addr += chunk;
			}
		} while (tag != fTagCursor);
	}

	*(unsigned *)dst = PS2_DMATAG_RET << 28;
	*(unsigned *)(dst + 1) = 0;
}

unsigned gm_packet::get_phys_addr(int *p)
{
	int off = (char *)p - fMemBase;
	int page = off / 4096;
	int within = off & 0xfff;
	return fPhysTable[page] + within;
}

void *gm_packet::get_virt_addr(int phys)
{
	int total = fDbufSize * 2 + fHeapSize;
	int npages = (total + 4095) / 4096;
	for (int i = 0; i < npages; i++) {
		int off = phys - fPhysTable[i];
		if ((unsigned)off < 4096) {
			return fMemBase + i * 4096 + off;
		}
	}
	return 0;
}

void gm_packet::get_heap_usage(int *total, int *used, int *maxfree)
{
	if (total)
		*total = fHeapSize;

	if (used || maxfree) {
		int sum = 0;
		gm_heap_block *cur = fHeapFreeList;
		gm_heap_block *first = cur;
		int best = 0;
		do {
			int sz = cur->size;
			cur = cur->next;
			int bytes = sz * 16 - 16;
			sum += bytes;
			if (best < bytes)
				best = bytes;
		} while (cur != first);

		if (used)
			*used = sum;
		if (maxfree)
			*maxfree = best;
	}
}

void gm_packet::get_dbuf_usage(int *size, int *used, int *cap)
{
	if (size)
		*size = fDbufSize;
	if (used)
		*used = fLastDbufUsed;
	if (cap)
		*cap = fLastDbufCap;
}

void gm_packet::render_heap(char *buf, int n)
{
	for (int i = 0; i < n; i++) {
		buf[i] = 1;
	}

	int total = fHeapSize / 16;
	char *heapBase = fHeapMem;
	if (total == 0)
		return;

	gm_heap_block *blk = fHeapFreeList;
	if (blk == 0)
		return;

	do {
		int idx = (int)((char *)blk - heapBase) / 16;
		int last = idx + blk->size;
		int start = idx * n / total;
		int end = last * n / total;
		for (int j = start; j < end; j++)
			buf[j] = 0;
		blk = blk->next;
	} while (blk != 0);
}

void gm_packet::render_dbuf(char *buf, int n)
{
	int usedCol = fLastDbufUsed * n / fDbufSize;
	int capCol = fLastDbufCap * n / fDbufSize;
	int freeStart = n - capCol;
	for (int i = 0; i < n; i++) {
		if (i < usedCol || i >= freeStart)
			buf[i] = 1;
		else
			buf[i] = 0;
	}
}

void gm_packet::print_phys_table()
{
	int total = fDbufSize * 2 + fHeapSize;
	int npages = (total + 4095) / 4096;
	for (int i = 0; i < npages; i++) {
		printf("%08x\n", fPhysTable[i]);
	}
	fflush(stdout);
}

void gm_packet::print_heap()
{
	printf("%p - %p ...\n", fHeapMem, fHeapMem + fHeapSize);

	gm_heap_block *minNode = fHeapFreeList;
	char *heapMem = fHeapMem;
	gm_heap_block *cur = minNode;
	gm_heap_block *first = minNode;
	do {
		if ((unsigned long)cur < (unsigned long)minNode)
			minNode = cur;
		cur = cur->next;
	} while (cur != first);

	cur = minNode;
	do {
		if (cur == fHeapFreeList)
			printf("* ");
		else
			printf("  ");

		int size = cur->size;
		int startIdx = (int)(((char *)cur - heapMem) >> 4);
		int endIdx = startIdx + size;
		printf("%3d - %3d ( %3d QW )\n", startIdx, endIdx, size);

		cur = cur->next;
	} while (cur != minNode);
}

void gm_packet::dump_vumem(int start, int count)
{
	printf("dump vumem 0x%04x\n", start);

	if (start < 0)
		start = 0;
	if (start > 1024)
		start = 1024;
	if (count < 0)
		count = 0;
	int maxn = 1024 - start;
	if (count > maxn)
		count = maxn;

	int *p = (int *)(fExtraBuf + 16384 + start * 16);
	float *fp = (float *)p;
	for (int i = 0; i < count; i++) {
		printf("%08x %08x %08x %08x : ", p[0], p[1], p[2], p[3]);
		printf("%.6f %.6f %.6f %.6f\n", fp[0], fp[1], fp[2], fp[3]);
		fp += 4;
		p += 4;
	}
}

void gm_packet::dump_micro(int start, int count)
{
	if (start < 0)
		start = 0;
	if (start > 2048)
		start = 2048;
	if (count < 0)
		count = 0;
	int maxn = 2048 - start;
	if (count > maxn)
		count = maxn;

	int *p = (int *)(fExtraBuf + start * 8);
	for (int i = 0; i < count; i++) {
		printf("%08x %08x\n", p[0], p[1]);
		p += 2;
	}
}

void gm_packet::clear_vumem(int val, int flag)
{
	if (flag == 0) {
		int *p = (int *)(fExtraBuf + 16384);
		for (int i = 0; i < 1024; i++) {
			p[0] = val;
			p[1] = val;
			p[2] = val;
			p[3] = val;
			p += 4;
		}
	} else {
		srand(val);
		int *p = (int *)(fExtraBuf + 16384);
		for (int i = 0; i < 1024; i++) {
			p[0] = rand();
			p[1] = rand();
			p[2] = rand();
			p[3] = rand();
			p += 4;
		}
	}
}

void gm_packet::clear_micro(int val, int flag)
{
	if (flag == 0) {
		int *p = (int *)fExtraBuf;
		for (int i = 0; i < 2048; i++) {
			p[0] = val;
			p[1] = val;
			p += 2;
		}
	} else {
		int *p = (int *)fExtraBuf;
		for (int i = 0; i < 2048; i++) {
			p[0] = rand();
			p[1] = rand();
			p += 2;
		}
	}
}

#include "vec.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */
