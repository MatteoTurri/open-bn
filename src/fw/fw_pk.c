/*
 * DMA chain of GS packets of the fw framework (fwPk*, fwHig*), built in DMem
 * memory and double-buffered.
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/ps2/gs.h>
#include <linux/ps2/dev.h>
#include <linux/ps2/ee.h>

#include "ps2gs.h"
#include "ps2vpu.h"
#include "fw.h"
#include "fw_ev.h"
#include "fw_gsdev.h"
#include "fw_pk.h"
#include "dmem.h"

struct fwQword {
	unsigned int w0;
	unsigned int w1;
	unsigned int w2;
	unsigned int w3;
};


/* DMA packer mode, also read by fwHigIsAfterKick. */
int DMA_postkick;

static pthread_mutex_t g_fwPkMutex;
static struct DMemInfo *g_fwPkDMemInfo;

/* The two halves of the chain buffer, indexed by 1 - g_fwPkSomeCounter. */
struct fwPkChainBuf {
	/* 0x00 */ int cur;  /* cleared on every flip */
	/* 0x04 */ void *base;
};
static struct DMemContRange g_fwPkCont;
static int g_fwPkPageShift;                       /* page_shift */
static void *g_fwPkChainStart;
/* Current write pointer of the chain; a "ref" patches the tag just emitted,
 * in the previous qword (offset -8). */
static int *g_fwPkChainWp;
static int g_fwPkHalfSize;
static int g_fwPkChainMask;                          /* page_size - 16 */
static void *g_fwPkChainEnd;
static int g_fwPkQwords;
static struct fwPkChainBuf *g_fwPkCurBuf;
static struct fwPkChainBuf g_fwPkChainBuf[2];
static void *g_fwPkDrawStart;
static void *g_fwPkDrawWp;
static int g_fwPkChainCount;

static int g_fwPkReadyFlag = 0;
static int g_fwPkSomeCounter = 0;

/* Header qword queued by fwPkAddDmaChain_withCont/withRef: a zero DMA tag and
 * two VIF codes, the last (DIRECT) takes the length in qwords. */
typedef struct {
	unsigned int tag[2];
	unsigned int vif0;
	unsigned int direct;
} fwPkTagHeader;
static fwPkTagHeader g_fwPkContHeader = { { 0 }, PS2_VIF_SET_CODE(0, 0, PS2_VIF_FLUSH, 0),
	PS2_VIF_SET_CODE(0, 0, PS2_VIF_DIRECT, 0) };
static fwPkTagHeader g_fwPkRefHeader = { { 0 }, 0, 0 };

static int fwPk_checkLen(unsigned int len);
static int fwPk_kickImpl(int fd, void *handleUnused);
static int fwPk_addChain(int mode, unsigned int phys, unsigned int count, unsigned int *vif);
static int fwPk_setupChainEnd(void *end);

/* Builds in place the GIF packet that writes 0 to the GS FINISH register: an A+D
 * GIFtag (NLOOP 1, EOP 1, NREG 1, REGS0 AD) and the data/address pair
 * (0, PS2_GS_FINISH). */
void _ps2_gs_set_finish(ps2_gs_finish *p)
{
	PS2_GIFTAG_CLEAR_TAG(&p->giftag);
	p->giftag.NLOOP = 1;
	p->giftag.EOP = 1;
	p->giftag.NREG = 1;
	p->giftag.REGS0 = PS2_GIFTAG_REGS_AD;
	p->finish.pad00 = 0;
	p->finishaddr = PS2_GS_FINISH;
}

/* Mode of the DMA packer (DMA_postkick); returns the new mode. */
int fwPkSetBufMode(int mode)
{
	DMA_postkick = mode;
	return mode;
}

int fwPkCheckBufMode(void)
{
	return DMA_postkick;
}

int fwHigIsAfterKick(void)
{
	return DMA_postkick;
}

/* Ready flag, read under the packer mutex. */
int fwPkCheckReady(void)
{
	int result;
	pthread_mutex_lock(&g_fwPkMutex);
	result = g_fwPkReadyFlag;
	pthread_mutex_unlock(&g_fwPkMutex);
	return result;
}

/* Waits for the end of the DMA (event 9) between the PreGsSync and PostGsSync
 * callbacks, then runs a pending framebuffer dump. */
int fwPkWaitDma(void)
{
	int result;
	fwExecCallBack_PreGsSync();
	result = fwEvSyncWait(PS2EV_N_FINISH, 1);
	fwExecCallBack_PostGsSync();
	_fwGsStoreImage_cb();
	return result;
}

/* Makes buffer `flag` the draw kick base, applies its half-pixel offset and kick
 * the closed half of the chain buffer on the VPU1; -1 if flag < 0. */
int fwPkKickChainBuf(void *handle, int flag)
{
	int kickBase;
	int fd;

	if (flag >= 0) {
		fwGsSetKickBaseDrawBuf(flag);
		kickBase = fwGsGetKickBaseDrawBuf();
		fwGsSetHalfOffset(kickBase, 0, 0, 0);

		fd = ps2_vpu_fd(g_vpu1);
		return fwPk_kickImpl(fd, handle);
	}
	return -1;
}

int fwHigAddDmaPacketUsingCall(void *pkt, void *addr, int qwc, void (*call)(void))
{}

int fwHigAddDmaPacketUsingRef(void *pkt, void *addr, int qwc)
{}

/* Queues the header qword (tag + VIF DIRECT with the length) and `size` qwords
 * of data as a CNT block. */
int fwPkAddDmaChain_withCont(void *handle, int size)
{
	int used = (int)g_fwPkChainWp - (int)g_fwPkChainStart;
	int avail = g_fwPkHalfSize - used;
	int need = (size + 6) * 16;
	if (need >= avail) {
		printf("warning fwpkt: buffer full!!\n");
		return -1;
	}
	g_fwPkContHeader.direct = PS2_VIF_SET_CODE((unsigned int)size, 0, PS2_VIF_DIRECT, 0);
	fwPkAddDmaChain_withCont_setVif(&g_fwPkContHeader, 1, 0, 0);
	fwPkAddDmaChain_withCont_setVif(handle, size, 0, 0);
	return 0;
}

/* Queues `nqw` qwords of data as a CNT block, splits by a NEXT tag whenever the
 * write pointer reaches the end of a page, and closes the header with the two
 * VIF words passed by the caller. */
int fwPkAddDmaChain_withCont_setVif(void *data, int nqw, unsigned int vif1, unsigned int vif2)
{
	int *hdr;
	unsigned int phys;
	int n;
	struct fwQword *src = (struct fwQword *)data;

	if (g_fwPkHalfSize - ((int)g_fwPkChainWp - (int)g_fwPkChainStart) <= (nqw + 3) * 16) {
		printf("warning fwpkt: buffer full!!\n");
		return -1;
	}
	if (((int)g_fwPkChainWp & g_fwPkChainMask) == g_fwPkChainMask) {
		g_fwPkChainWp[0] = PS2_DMATAG_NEXT << 28;
		phys = DMemGetPhysAddr(g_fwPkDMemInfo, (unsigned int)(g_fwPkChainWp + 4));
		g_fwPkChainWp[1] = (int)(phys & 0x7fffffff);
		g_fwPkChainWp[2] = 0;
		g_fwPkChainWp[3] = 0;
		g_fwPkChainWp = g_fwPkChainWp + 4;
	}
	hdr = g_fwPkChainWp;
	g_fwPkChainWp = hdr + 4;
	while (nqw != 0) {
		int *dst;

		if (((int)g_fwPkChainWp & g_fwPkChainMask) == g_fwPkChainMask) {
			hdr[0] = ((((int)g_fwPkChainWp - (int)hdr) >> 4) - 1) | (PS2_DMATAG_CNT << 28);
			hdr[1] = 0;
			hdr[2] = 0;
			hdr[3] = 0;
			n = ((int)g_fwPkChainWp - (int)hdr) >> 4;
			if (n != 1)
				hdr[3] = PS2_VIF_SET_CODE(n - 1, 0, PS2_VIF_DIRECT, 0);
			g_fwPkChainWp[0] = PS2_DMATAG_NEXT << 28;
			phys = DMemGetPhysAddr(g_fwPkDMemInfo, (unsigned int)(g_fwPkChainWp + 4));
			g_fwPkChainWp[1] = (int)(phys & 0x7fffffff);
			g_fwPkChainWp[2] = 0;
			g_fwPkChainWp[3] = 0;
			g_fwPkChainWp = g_fwPkChainWp + 4;
			hdr = g_fwPkChainWp;
			g_fwPkChainWp = g_fwPkChainWp + 4;
		} else {
			dst = g_fwPkChainWp;
			dst[3] = src->w3;
			dst[2] = src->w2;
			dst[1] = src->w1;
			dst[0] = src->w0;
			nqw--;
			src++;
			g_fwPkChainWp = g_fwPkChainWp + 4;
		}
	}
	hdr[0] = ((((int)g_fwPkChainWp - (int)hdr) >> 4) - 1) | (PS2_DMATAG_CNT << 28);
	hdr[1] = 0;
	hdr[2] = (int)vif1;
	hdr[3] = (int)vif2;
	return 0;
}

/* Queues a DMA CALL tag (id 5) to the physical address of `addr` in its DMem
 * region. */
int fwPkAddDmaChain_withCall(void *addr)
{
	struct DMemInfo *info = DMemFindHitArea((int)addr);
	unsigned int phys;
	if (!info)
		return -1;
	if (fwPk_checkLen(1) < 0)
		return -1;
	phys = DMemGetPhysAddr(info, (unsigned int)addr);
	fwPk_addChain(PS2_DMATAG_CALL, phys, 0, 0);
	return 0;
}

/* As withCall, but `phys` is already a physical address. */
int fwPkAddDmaChain_withCall_physAddr(unsigned int phys)
{
	if (fwPk_checkLen(1537) < 0)
		return -1;
	fwPk_addChain(PS2_DMATAG_CALL, phys, 0, 0);
	return 0;
}

/* Queues the header qword as a CNT block, then the data by reference (REF). */
int fwPkAddDmaChain_withRef(void *handle, int size)
{
	g_fwPkRefHeader.direct = PS2_VIF_SET_CODE((unsigned int)size, 0, PS2_VIF_DIRECT, 0);
	if (fwPk_checkLen(size + 1536) < 0)
		return -1;
	fwPkAddDmaChain_withCont_setVif(&g_fwPkRefHeader, 1, 0, 0);
	fwPkAddDmaChain_withRef_setVif(handle, size, 0, 0);
	return 0;
}

/* Queues addr..len by reference (REF tag, id 3), one tag for each physically
 * contiguous run; `ref`, if non-zero, goes in word -2 of the first tag. */
int fwPkAddDmaChain_withRef_setVif(void *data, unsigned int len, int ref, unsigned int vif)
{
	struct DMemInfo *info;
	struct DMemContRange cont;
	unsigned int vifLocal = vif;
	unsigned int addr = (unsigned int)data;
	unsigned int phys;
	int r;

	info = DMemFindHitArea((int)addr);
	if (info == 0 || fwPk_checkLen(len) < 0)
		return -1;
	phys = DMemGetPhysAddr(info, addr);
	cont.pages_ok = 0;
	cont.pages_short = len;
	for (;;) {
		DMemCheckContinuous(info, &cont, addr, cont.pages_short);
		r = fwPk_addChain(3, phys, cont.pages_ok, &vifLocal);
		if (ref != 0) {
			g_fwPkChainWp[-2] = ref;
			ref = 0;
		}
		if (r < 0)
			break;
		addr = cont.next_addr;
		if (cont.pages_short == 0)
			break;
		phys = cont.phys_addr;
	}
	return 0;
}

/* Room in the chain buffer for `len` qwords? Pages needed (rounded up, plus 3
 * tag qwords) against what is left in the current half: 0 if they fit, else
 * a warning and -1. */
static int fwPk_checkLen(unsigned int len)
{
	if (((((int)(len << 4) >> g_fwPkPageShift) + 3) << 4)
	    >= g_fwPkHalfSize - ((char *)g_fwPkChainWp - (char *)g_fwPkChainStart)) {
		printf("warning fwpkt: buffer full!!\n");
		return -1;
	}
	return 0;
}

/* Under the packer mutex takes the ready state, clears the event 9 counters and
 * passes the base of the half just closed to the VPU driver (ioctl 0x8008ee81,
 * not in the kit's dev.h; it takes a ps2_packet with length 0).
 * Returns the ready state, or -1 if the ioctl fails. */
static int fwPk_kickImpl(int fd, void *handleUnused)
{
	int oldFlag, counter, idx, result;
	struct ps2_packet pkt;

	pthread_mutex_lock(&g_fwPkMutex);
	oldFlag = g_fwPkReadyFlag;
	counter = g_fwPkSomeCounter;
	g_fwPkReadyFlag = 0;
	idx = 1 - counter;

	fwEvClearCounterAll(PS2EV_N_FINISH);

	pkt.len = 0;
	pkt.ptr = g_fwPkChainBuf[idx].base;
	result = ioctl(fd, 0x8008ee81U, &pkt);

	if (!(result > -1))
		oldFlag = -1;

	pthread_mutex_unlock(&g_fwPkMutex);
	return oldFlag;
}

/* Emits one DMA chain qword. At the end of a page close it with a NEXT tag
 * to the physical address of the next page, then writes the tag (mode is the
 * DMA tag id: CALL, otherwise REF) and, when `vif` is set, the VIF code from
 * *vif: DIRECT/DIRECTHL -> DIRECT of count qwords, UNPACK -> the
 * code with the count in bits 16..23 and *vif advanced by count, any other cmd
 * -> the code as is. */
static int fwPk_addChain(int mode, unsigned int phys, unsigned int count, unsigned int *vif)
{
	unsigned int nextPhys;

	if (((int)g_fwPkChainWp & g_fwPkChainMask) == g_fwPkChainMask) {
		g_fwPkChainWp[0] = PS2_DMATAG_NEXT << 28;
		nextPhys = DMemGetPhysAddr(g_fwPkDMemInfo, (unsigned int)(g_fwPkChainWp + 4));
		g_fwPkChainWp[1] = (int)(nextPhys & 0x7fffffff);
		g_fwPkChainWp[2] = 0;
		g_fwPkChainWp[3] = 0;
		g_fwPkChainWp = g_fwPkChainWp + 4;
	}

	switch (mode) {
	case PS2_DMATAG_CALL:
		g_fwPkChainWp[0] = (int)(count | (PS2_DMATAG_CALL << 28));
		g_fwPkChainWp[1] = (int)phys;
		g_fwPkChainWp[2] = 0;
		g_fwPkChainWp[3] = 0;
		break;

	case PS2_DMATAG_REF:
	default:
		g_fwPkChainWp[0] = (int)(count | (PS2_DMATAG_REF << 28));
		g_fwPkChainWp[1] = (int)(phys & 0x7fffffff);
		g_fwPkChainWp[2] = 0;
		g_fwPkChainWp[3] = 0;
		break;
	}

	if (vif != 0) {
		int cmd = (int)*vif >> 24;

		switch (cmd) {
		case PS2_VIF_DIRECT:
		case PS2_VIF_DIRECTHL:
			g_fwPkChainWp[3] = (int)PS2_VIF_SET_CODE(count, 0, PS2_VIF_DIRECT, 0);
			break;

		case PS2_VIF_UNPACK:
			g_fwPkChainWp[3] = (int)((*vif & 0xff00ffff) | (count << 16));
			*vif = *vif + count;
			break;

		default:
			g_fwPkChainWp[3] = (int)*vif;
			break;
		}
	}

	g_fwPkChainWp = g_fwPkChainWp + 4;
	return 0;
}

/* Puts the FINISH packet at the end of the chain buffer (+16) and closes the
 * chain with a VIF DIRECT of the 2 qwords of the packet in the previous qword. */
static int fwPk_setupChainEnd(void *end)
{
	int *p = (int *)end;

	_ps2_gs_set_finish((ps2_gs_finish *)(p + 4));
	p[2] = 0;
	p[1] = 0;
	p[0] = 0;
	p[3] = PS2_VIF_SET_CODE(2, 0, PS2_VIF_DIRECT, 0);
	return 0;
}

/* Splits the DMem area `base` into two 16-byte aligned halves (double-buffered
 * DMA chain), keeping 48 bytes of tail per half, registers the DMem region and
 * arm the first buffer. */
int fwPkInitChainBuf(void *base, int size, int mode)
{
	struct DMemInfo *info;
	struct fwPkChainBuf *e;
	int half;
	char *buf1;
	char *end;
	int unit;

	half = size / 2 - 48;
	info = DMemFindHitArea((int)base);
	fwPkSetBufMode(mode);
	buf1 = (char *)((((int)base + half) + 15) & -16);
	end = (char *)((((int)buf1 + half) + 15) & -16);
	g_fwPkQwords = (int)((unsigned int)half >> 3);
	g_fwPkHalfSize = (int)(((unsigned int)half >> 4) << 4);
	g_fwPkChainMask = info->page_size - 16;
	g_fwPkDMemInfo = info;
	g_fwPkPageShift = info->page_shift;
	g_fwPkSomeCounter = 0;
	g_fwPkChainBuf[0].base = base;
	g_fwPkChainBuf[1].base = buf1;
	g_fwPkChainEnd = end;
	fwPk_setupChainEnd(end);
	unit = 16;
	g_fwPkChainCount = 0;
	e = &g_fwPkChainBuf[g_fwPkSomeCounter];
	g_fwPkCurBuf = e;
	g_fwPkDrawStart = e->base;
	e->cur = 0;
	g_fwPkDrawWp = e->base;
	DMemCheckContinuous(g_fwPkDMemInfo, &g_fwPkCont, g_fwPkDMemInfo->virt_base, size / unit);
	g_fwPkChainStart = g_fwPkChainBuf[g_fwPkSomeCounter].base;
	g_fwPkChainWp = (int *)g_fwPkChainStart;
	return 0;
}

/* Clears the chain buffer state. */
void fwPkReleaseChainBuf(void)
{
	g_fwPkSomeCounter = 0;
	g_fwPkDMemInfo = 0;
	g_fwPkChainWp = 0;
	g_fwPkChainBuf[0].base = 0;
	g_fwPkChainBuf[1].base = 0;
	g_fwPkCurBuf = 0;
	/* NOTE: bug: writes through g_fwPkCurBuf right after clearing it. */
	g_fwPkCurBuf->base = 0;
	g_fwPkDrawStart = 0;
	g_fwPkDrawWp = 0;
	g_fwPkCurBuf->cur = 0;
	g_fwPkChainCount = 0;
}

/* Closes the chain of the current buffer (a next page tag if the write pointer
 * is on the last qword of the page, then an END tag) and flips the
 * active buffer under the mutex. */
int fwPkFlipChainBuf(void)
{
	unsigned int phys;
	struct DMemInfo *info;
	struct fwPkChainBuf *e;
	int idx;

	fwPkAddDmaChain_withRef_setVif(g_fwPkChainEnd, 3, 0, 0);
	if (((int)g_fwPkChainWp & g_fwPkChainMask) == g_fwPkChainMask) {
		g_fwPkChainWp[0] = PS2_DMATAG_NEXT << 28;
		phys = DMemGetPhysAddr(g_fwPkDMemInfo, (unsigned int)(g_fwPkChainWp + 4));
		g_fwPkChainWp[1] = (int)(phys & 0x7fffffff);
		g_fwPkChainWp[2] = 0;
		g_fwPkChainWp[3] = 0;
		g_fwPkChainWp = g_fwPkChainWp + 4;
	}
	g_fwPkChainWp[0] = PS2_DMATAG_END << 28;
	g_fwPkChainWp[1] = 0;
	g_fwPkChainWp[2] = 0;
	g_fwPkChainWp[3] = 0;
	g_fwPkChainWp = g_fwPkChainWp + 4;
	pthread_mutex_lock(&g_fwPkMutex);
	g_fwPkReadyFlag = 1;
	g_fwPkChainCount = 0;
	info = g_fwPkDMemInfo;
	idx = 1 - g_fwPkSomeCounter;
	g_fwPkSomeCounter = idx;
	e = &g_fwPkChainBuf[idx];
	g_fwPkCurBuf = e;
	g_fwPkDrawStart = e->base;
	e->cur = 0;
	g_fwPkDrawWp = e->base;
	DMemCheckContinuous(info, &g_fwPkCont, info->virt_base, info->size >> 4);
	g_fwPkChainStart = g_fwPkChainBuf[g_fwPkSomeCounter].base;
	g_fwPkChainWp = (int *)g_fwPkChainStart;
	pthread_mutex_unlock(&g_fwPkMutex);
	return 0;
}
