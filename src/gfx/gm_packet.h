/*
 * gm_packet: the DMA packet double buffer and the qword heap of the GS/VU1
 * renderer. A heap block has a 16-byte header before the returned pointer.
 */

#ifndef GM_PACKET_H
#define GM_PACKET_H

#pragma interface

struct gm_heap_block {
	/* 0x00 */ gm_heap_block *next;  /* main free list, ascending addresses */
	/* 0x04 */ int size;  /* payload size in qwords */
	/* 0x08 */ gm_heap_block *pend;  /* link of the pending list */
	/* 0x0C */ int gen;  /* generation (fParity) at allocation */
}; /* size = 0x10 */

class gm_graph;
struct ps2_vpu_struct;

class gm_packet {
	friend class gm_graph;
	friend void *gm_alloc_heap(gm_packet *, int);
	friend void gm_print_phys_table(gm_packet *);
	friend void gm_print_heap(gm_packet *);

public:
	gm_packet();
	~gm_packet();

	int init(int dbufSize, int heapSize);
	void wait();
	void term();
	void send();
	void flip();

	void init_heap();
	void cleanup_heap();
	void *alloc_heap(int size);
	void free_heap(void *p);
	void free_heap2(void *p);
	int max_heap_space();

	void *alloc_packet(int size);
	void put_packet(int *p, int size);
	void begin_list();

	unsigned get_phys_addr(int *p);
	void *get_virt_addr(int phys);
	void get_heap_usage(int *total, int *used, int *maxfree);
	void get_dbuf_usage(int *size, int *used, int *cap);

	void put_list(int *p);
	int count_list_dmatag();
	void flush_packet();

	int end_list(int *&p, int &n);
	void rebuild_list_dmatag(int *list, int *src);
	void print_chain();

	void render_dbuf(char *buf, int n);
	void render_heap(char *buf, int n);
	void print_phys_table();
	void print_heap();
	void dump_vumem(int start, int count);
	void dump_micro(int start, int count);
	void clear_vumem(int val, int flag);
	void clear_micro(int val, int flag);

private:
	/* 0x00 */ int fChannel;  /* DMA channel, -1 = not open */
	/* 0x04 */ int unk_04;
	/* 0x08 */ ps2_vpu_struct *fVpu;  /* VPU1 handle (g_vpu1) */
	/* 0x0C */ char *fExtraBuf;  /* extra buffer, freed with a fixed size of 0x8000 in term() */
	/* 0x10 */ unsigned fFlags;  /* bit 0 = active, bit 1 = option */
	/* 0x14 */ int fDbufSize;  /* size of one half of the double buffer */
	/* 0x18 */ int fHeapSize;  /* total heap size */
	/* 0x1C */ int fCleanupDelay;  /* generations before a pending block is freed (4) */
	/* 0x20 */ int *fPhysTable;  /* page to physical address table */
	/* 0x24 */ char *fMemBase;  /* double buffer (2 * fDbufSize), then the heap */
	/* 0x28 */ char *fHeapMem;  /* heap base, inside fMemBase */
	/* 0x2C */ gm_heap_block *fHeapFreeList;  /* free list sentinel */
	/* 0x30 */ gm_heap_block *fPendingList;  /* blocks waiting to be freed (free_heap2) */
	/* 0x34 */ char *fChainStart;  /* first DMA tag not yet added to the chain */
	/* 0x38 */ int unk_38;
	/* 0x3C */ int fParity;  /* frame counter (flip) */
	/* 0x40 */ char *fTagCursor;  /* DMA tag write cursor, at the end of the double buffer half */
	/* 0x44 */ char *fCursor;  /* bump allocation cursor */
	/* 0x48 */ char *fListTagStart;  /* tag cursor at begin_list */
	/* 0x4C */ char *fListDataStart;  /* data cursor at begin_list */
	/* 0x50 */ void *fPutStart;  /* start of the contiguous put_packet data not yet flushed, or 0 */
	/* 0x54 */ void *fPutEnd;  /* end of that data */
	/* 0x58 */ int fLastDbufUsed;
	/* 0x5C */ int fLastDbufCap;
}; /* size = 0x60 */

#endif
