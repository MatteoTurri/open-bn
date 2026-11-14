//
// "gs_draw.c"
//

/*
 	Copyright (C) 2001  Sony Computer Entertainment Inc.
 
  This file is subject to the terms and conditions of the GNU Library
  General Public License Version 2. See the file "COPYING.LIB" in the 
  main directory of this archive for more details.
*/


#include <stdio.h>
#include <stdlib.h>
#include <linux/ps2/dev.h>
#include "ps2dma.h"
#include "gs_internal.h"

//----------------------------------------------------------------------
int ps2_gs_set_drawenv(ps2_gs_drawenv1 *draw, int psm, int w, int h, int ztest, int zpsm)
{
	int ztop = __ps2_gs_zbuf_addr(psm, w, h);
	
	draw->frame1addr = PS2_GS_FRAME_1;
	*(__u64 *)&draw->frame1 = PS2_GS_SETREG_FRAME(0, ((w + 63) >> 6) & 0x3f, psm & 0xf, 0);
	
	draw->zbuf1addr =PS2_GS_ZBUF_1;
	if (ztest == 0) {
		*(__u64 *)&draw->zbuf1 =
			PS2_GS_SETREG_ZBUF(ztop, zpsm & 0xf, PS2_GS_ZBUF_ZMSK_MASK);
	} else {
		*(__u64 *)&draw->zbuf1 =
			PS2_GS_SETREG_ZBUF(ztop, zpsm & 0xf, PS2_GS_ZBUF_ZMSK_NOMASK);
	}
	
	draw->xyoffset1addr = PS2_GS_XYOFFSET_1;
	*(__u64 *)&draw->xyoffset1 =
		PS2_GS_SETREG_XYOFFSET_1(((2048 - (w >> 1)) << 4), ((2048 - (h >> 1)) << 4));
	
	draw->scissor1addr = PS2_GS_SCISSOR_1;
	*(__u64 *)&draw->scissor1 = PS2_GS_SETREG_SCISSOR(0, w - 1, 0, h - 1);
	
	draw->prmodecontaddr = PS2_GS_PRMODECONT;
	draw->prmodecont.AC = PS2_GS_PRMODECONT_REFPRIM;

	draw->colclampaddr = PS2_GS_COLCLAMP;
	draw->colclamp.CLAMP = PS2_GS_COLCLAMP_CLAMP;
	
	draw->dtheaddr = PS2_GS_DTHE;
	if (psm & 0x2) {
		draw->dthe.DTHE = PS2_GS_DTHE_ON;
	} else {
		draw->dthe.DTHE = PS2_GS_DTHE_OFF;
	}
	
	draw->test1addr = PS2_GS_TEST_1;
	if (ztest) {
		*(__u64 *)&draw->test1 = 
			PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, 1, ztest & 0x3);
	} else {
		*(__u64 *)&draw->test1 = 
			PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, 1, PS2_GS_TEST_ZTST_ALWAYS);
	}
	
	return sizeof(ps2_gs_drawenv1) / 16;
}

//----------------------------------------------------------------------
int ps2_gs_set_drawenv2(ps2_gs_drawenv2 *draw, int psm, int w, int h, int ztest, int zpsm)
{
	int ztop = __ps2_gs_zbuf_addr(psm, w, h);
	
	draw->frame2addr = PS2_GS_FRAME_2;
	*(__u64 *)&draw->frame2 = PS2_GS_SETREG_FRAME(0, (w >> 6) & 0x3f, psm & 0xf, 0);
	draw->zbuf2addr = PS2_GS_ZBUF_2;
	if (ztest == 0) {
		*(__u64 *)&draw->zbuf2 =
			PS2_GS_SETREG_ZBUF(ztop, zpsm & 0xf, PS2_GS_ZBUF_ZMSK_MASK);
	} else {
		*(__u64 *)&draw->zbuf2 =
			PS2_GS_SETREG_ZBUF(ztop, zpsm & 0xf, PS2_GS_ZBUF_ZMSK_NOMASK);
	}
	
	draw->xyoffset2addr = PS2_GS_XYOFFSET_2;
	*(__u64 *)&draw->xyoffset2 =
		PS2_GS_SETREG_XYOFFSET_2(((2048 - (w >> 1)) << 4), ((2048 - (h >> 1)) << 4));
	
	draw->scissor2addr = PS2_GS_SCISSOR_2;
	*(__u64 *)&draw->scissor2 = PS2_GS_SETREG_SCISSOR(0, w - 1, 0, h - 1);
	
	draw->prmodecontaddr = PS2_GS_PRMODECONT;
	draw->prmodecont.AC = PS2_GS_PRMODECONT_REFPRIM;
	
	draw->colclampaddr = PS2_GS_COLCLAMP;
	draw->colclamp.CLAMP = PS2_GS_COLCLAMP_CLAMP;

	draw->dtheaddr = PS2_GS_DTHE;
	if (psm & 0x2) {
		draw->dthe.DTHE = PS2_GS_DTHE_ON;
	} else {
		draw->dthe.DTHE = PS2_GS_DTHE_OFF;
	}

	draw->test2addr = PS2_GS_TEST_2;
	if (ztest) {
		*(__u64 *)&draw->test2 =
			PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, 1, ztest & 0x3);
	} else {
		*(__u64 *)&draw->test2 =
			PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, 1, PS2_GS_TEST_ZTST_ALWAYS);
	}
	
	return sizeof(ps2_gs_drawenv2) / 16;
}

//----------------------------------------------------------------------
int ps2_gs_put_drawenv(ps2_giftag *p)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	int n;
	
	n = p->NLOOP * (p->NREG ? p->NREG : 16) + 1;
	
	if (ps2_dma_start_n(gp->fd_gs, p, n) < 0) {
		perror("ps2_gs_put_drawenv");
		return -1;
	}
	
	return 0;
}

//----------------------------------------------------------------------
void ps2_gs_set_finish(ps2_gs_finish *p)
{
	PS2_GIFTAG_CLEAR_TAG(&p->giftag);
	p->giftag.NLOOP = 1;
	p->giftag.EOP = 1;
	p->giftag.NREG = 1;
	p->giftag.REGS0 = PS2_GIFTAG_REGS_AD;
	
	p->finish.pad00 = 0;
	p->finishaddr = PS2_GS_FINISH;
}

//----------------------------------------------------------------------
void ps2_gs_wait_finish(ps2_gs_finish *p)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	
	ps2_gs_put_drawenv(&p->giftag);
	ps2_dma_wait(gp->fd_gs, 1);
	
	ioctl(gp->fd_event, PS2IOC_WAITEVENT, PS2EV_FINISH);
}
