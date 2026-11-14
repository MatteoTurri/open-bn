/*
 * Texture manager DMA and GIF tags: DMA ref/ret chains for image uploads and
 * the GIF packets that flush the texture cache and start an image load.
 */

#include <stddef.h>
#include <linux/ps2/ee.h>

#include "fw/dmem.h"
#include "fw/fw_pk.h"
#include "tm_int.h"

/* One A+D register write: data, then the GS register address. */
struct tmGsAD {
	unsigned long long data;
	unsigned long long addr;
};

/* GIF packet with A+D register writes, the first one included: TEXFLUSH alone,
 * or the head of an image load, BITBLTBUF of NLOOP=4 writes
 * (_tmSetTag_LoadImage_tail adds the rest). */
struct TmGifPacket {
	/* 0x00 */ unsigned int tag_lo;  /* GIFtag low word: NLOOP, EOP */
	/* 0x04 */ unsigned int tag_hi;  /* GIFtag high word: NREG=1 */
	/* 0x08 */ unsigned int regs;  /* REGS list (0xE = A+D) */
	/* 0x0C */ unsigned int pad;
	/* 0x10 */ struct tmGsAD ad;
}; /* size = 0x20 */

static struct TmGifPacket g_texFlushTag;
static struct TmGifPacket g_loadImageHead;

/* Writes a DMA "ret" tag (end of chain) and returns the address after it. */
void *_tmSetTag_DmaRet(void *tag)
{
	unsigned int *t = (unsigned int *)tag;

	t[0] = PS2_DMATAG_RET << 28;
	t[1] = 0;
	return (char *)tag + 16;
}

/*
 * Writes DMA "ref" tags for len quadwords at addr, split so that no chunk
 * crosses a 4 KB page. A tag falling at the end of a page of the tag buffer
 * becomes a "next" tag to the physical address of the following one. Returns
 * the end of the chain.
 */
void *_tmSetTag_DmaRef(void *dst, int addr, int len)
{
	struct DMemInfo *info;
	int chunk;

	info = DMemFindHitArea((int)dst);
	chunk = 256 - ((addr & 0xfff) >> 4);
	if (len < chunk)
		chunk = len;

	if (len != 0) {
		do {
			void *next = (char *)dst + 16;

			if (((unsigned int)next & 0xfff) != 0) {
				unsigned int phys;

				*(unsigned int *)dst = chunk | (PS2_DMATAG_REF << 28);
				phys = DMemGetPhysAddr(info, addr);
				len -= chunk;
				addr += chunk << 4;
				chunk = (len < 257) ? len : 256;
				((unsigned int *)dst)[1] = phys;
			} else {
				*(unsigned int *)dst = PS2_DMATAG_NEXT << 28;
				((unsigned int *)dst)[1] = DMemGetPhysAddr(info, (unsigned int)next);
			}
			dst = next;
		} while (len != 0);
	}
	return dst;
}

static __inline__ void _tmTexFlushSetData(unsigned long long v)
{
	g_texFlushTag.ad.data = v;
}

static __inline__ void _tmTexFlushSetPad(unsigned int v)
{
	g_texFlushTag.pad = v;
}

/* Queues a GIF packet that writes TEXFLUSH (flushes the GS texture cache). */
void _tmAddTag_TexFlush(void)
{
	_tmTexFlushSetData(0);
	g_texFlushTag.tag_lo = 0x8001;
	g_texFlushTag.tag_hi = 0x10000000;
	g_texFlushTag.regs = 14;
	_tmTexFlushSetPad(0);
	g_texFlushTag.ad.addr = 0x3f;
	fwPkAddDmaChain_withCont(&g_texFlushTag, 2);
}

static __inline__ void _tmSetPad(unsigned int v)
{
	g_loadImageHead.pad = v;
}

static __inline__ void _tmSetData(unsigned long long v)
{
	g_loadImageHead.ad.data = v;
}

void _tmAddTag_LoadImage_head(short dbp, short dbw, short dpsm)
{
	g_loadImageHead.tag_lo = 4;
	g_loadImageHead.tag_hi = 0x10000000;
	g_loadImageHead.regs = 14;
	_tmSetPad(0);
	g_loadImageHead.ad.addr = 0x50;
	_tmSetData(
	    ((unsigned long long)dbp << 32) | ((unsigned long long)dbw << 48) | ((unsigned long long)dpsm << 56));
	fwPkAddDmaChain_withCont(&g_loadImageHead, 2);
}

/*
 * Tail of an image load: TRXPOS, TRXREG and TRXDIR as A+D writes, then the
 * IMAGE-mode GIFtag of the pixel data.
 */
void _tmSetTag_LoadImage_tail(void *dst, int w, int h, int dbw)
{
	unsigned int *p = (unsigned int *)dst;
	struct tmGsAD *ad;

	p[0] = 0;
	p[1] = 0;
	p[2] = 0;
	p[3] = 0x51000004 + dbw;
	dst = (char *)dst + 16;
	ad = (struct tmGsAD *)dst;

	ad[0].data = 0;   /* TRXPOS */
	ad[0].addr = 81;
	ad[1].data = (unsigned long long)(short)w | ((unsigned long long)(short)h << 32); /* TRXREG */
	ad[1].addr = 82;
	ad[2].data = 0;   /* TRXDIR */
	ad[2].addr = 83;

	p = (unsigned int *)dst; /* IMAGE-mode GIFtag, NLOOP=dbw, EOP */
	p[12] = 0x8000 + dbw;
	p[13] = 0x08000000;
	p[14] = 0;
	p[15] = 0;
}

/*
 * Builds the DMA chain of an image upload. For each mipmap level: ref tags for
 * the 5-quadword GS header at addr and for count quadwords of data (at len, or
 * right after the header when len is 0), then a ret tag. tags[] receives the
 * physical address of each level's chain.
 */
void *_tmSetTag_Image(unsigned int *tags, void *dst, int addr, int len, int count, int levels)
{
	struct DMemInfo *info;

	info = DMemFindHitArea((int)dst);
	levels--;
	if (levels != -1) {
		do {
			*tags = DMemGetPhysAddr(info, (unsigned int)dst);
			tags++;
			if (len != 0) {
				dst = _tmSetTag_DmaRef(dst, addr, 5);
				dst = _tmSetTag_DmaRef(dst, len, count);
				len += count << 4;
			} else {
				dst = _tmSetTag_DmaRef(dst, addr, count + 5);
				addr += count << 4;
			}
			dst = _tmSetTag_DmaRet(dst);
			addr += 80;
			count >>= 2;
			levels--;
		} while (levels != -1);
	}
	return dst;
}
