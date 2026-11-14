//
// "gs_reset.c"
//

/*
 	Copyright (C) 2001  Sony Computer Entertainment Inc.
 
  This file is subject to the terms and conditions of the GNU Library
  General Public License Version 2. See the file "COPYING.LIB" in the 
  main directory of this archive for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include "gs_internal.h"

//----------------------------------------------------------------------
int ps2_gs_sync_v(int mode)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	struct ps2_gssreg reg;
	
	ioctl(gp->fd_event, PS2IOC_GETEVENT, PS2EV_VBSTART);
	ioctl(gp->fd_event, PS2IOC_WAITEVENT, PS2EV_VBSTART);
	
	if (gp->interlace_mode == PS2_GS_INTERLACE) {
		reg.reg = PS2_GSSREG_CSR;
		if (ioctl(gp->fd_gs, PS2IOC_GGSSREG, &reg) < 0) {
			perror("ps2_gs_sync_v, ioctl");
		}
		
		return ((ps2_gssreg_csr *)&reg.val)->FIELD;
	}
	
	return 1; // odd
}

//----------------------------------------------------------------------
int ps2_gs_set_dbuff(ps2_gs_dbuff *db, int psm, int w, int h, int ztest, int zpsm, int clear)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	int zbp;
	int next2k;
	
	ps2_gs_set_dispenv(&db->disp[0], psm, w);
	ps2_gs_set_dispenv(&db->disp[1], psm, w);
	ps2_gs_set_drawenv(&db->draw0, psm, w, h, ztest, zpsm);
	ps2_gs_set_drawenv(&db->draw1, psm, w, h, ztest, zpsm);
	
	if (clear) {
		ps2_gs_set_clear(&db->clear0, ztest, gp->center_x - (w >> 1), 
						 gp->center_y - (h >> 1), w, h, 0, 0, 0, 0, 0);
		ps2_gs_set_clear(&db->clear1, ztest, gp->center_x - (w >> 1),
						 gp->center_y - (h >> 1), w, h, 0, 0, 0, 0, 0);
	}
	
	PS2_GIFTAG_CLEAR_TAG(&db->giftag0);
	db->giftag0.NLOOP = (sizeof(ps2_gs_drawenv1) + (clear ? sizeof(ps2_gs_clear) : 0)) / 16;
	db->giftag0.EOP = 1;
	db->giftag0.NREG = 1;
	db->giftag0.REGS0 = PS2_GIFTAG_REGS_AD;
	
	PS2_GIFTAG_CLEAR_TAG(&db->giftag1);
	db->giftag1.NLOOP = (sizeof(ps2_gs_drawenv1) + (clear ? sizeof(ps2_gs_clear) : 0)) / 16;
	db->giftag1.EOP = 1;
	db->giftag1.NREG = 1;
	db->giftag1.REGS0 = PS2_GIFTAG_REGS_AD;
	
	zbp = __ps2_gs_zbuf_addr(psm, w, h);
	
	//   NI, fake NI
	//   I(frame)           I(field)
	//    -->+--------+      -->>+--------+
	//       |   0    |          |        |
	//    -->+--------+          |  0/1   |
	//       |   1    |          |        |
	// zbp-->+--------+	   zbp-->+--------+
	//       |   Z    |	         |        |
	//       +--------+	         |   Z    |
	//                           |        |
	//                           +--------+
	// 
	if (gp->interlace_format && gp->ff_mode == PS2_GS_FIELD) {
		// I(field)
		next2k = zbp;
	} else {
		// NI, fake NI, I(frame)
		db->disp[1].dispfb.FBP = zbp >> 1;
		db->draw0.frame1.FBP = zbp >> 1;
		next2k = zbp;
	}
	
	if (ztest) {
		next2k += __ps2_gs_zbuf_size(zpsm, w, h);
	}
	
	return next2k;
}

//----------------------------------------------------------------------
void ps2_gs_set_dbuff_dc(ps2_gs_dbuffdc *db, int psm, int w, int h, int ztest, int zpsm, int clear)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	int zbp;
	
	ps2_gs_set_dispenv(&db->disp[0], psm, w);
	ps2_gs_set_dispenv(&db->disp[1], psm, w);
	ps2_gs_set_drawenv(&db->draw01, psm, w, h, ztest, zpsm);
	ps2_gs_set_drawenv2(&db->draw02, psm, w, h, ztest, zpsm);
	ps2_gs_set_drawenv(&db->draw11, psm, w, h, ztest, zpsm);
	ps2_gs_set_drawenv2(&db->draw12, psm, w, h, ztest, zpsm);
	if (clear) {
		ps2_gs_set_clear(&db->clear0, ztest, gp->center_x - (w >> 1), 
						 gp->center_y - (h >> 1), w, h, 0, 0, 0, 0, 0);
		ps2_gs_set_clear(&db->clear1, ztest, gp->center_x - (w >> 1),
						 gp->center_y - (h >> 1), w, h, 0, 0, 0, 0, 0);
	}
	
	PS2_GIFTAG_CLEAR_TAG(&db->giftag0);
	db->giftag0.NLOOP = (sizeof(ps2_gs_drawenv1) + sizeof(ps2_gs_drawenv2) +
						 (clear ? sizeof(ps2_gs_clear) : 0)) / 16;
	db->giftag0.EOP = 1;
	db->giftag0.NREG = 1;
	db->giftag0.REGS0 = PS2_GIFTAG_REGS_AD;
	
	PS2_GIFTAG_CLEAR_TAG(&db->giftag1);
	db->giftag1.NLOOP = (sizeof(ps2_gs_drawenv1) + sizeof(ps2_gs_drawenv2) +
						 (clear ? sizeof(ps2_gs_clear) : 0)) / 16;
	db->giftag1.EOP = 1;
	db->giftag1.NREG = 1;
	db->giftag1.REGS0 = PS2_GIFTAG_REGS_AD;
	
	zbp = __ps2_gs_zbuf_addr(psm, w, h);
	
	if (((gp->interlace_mode == PS2_GS_INTERLACE) && (gp->ff_mode == PS2_GS_FRAME)) ||
		(gp->interlace_mode == PS2_GS_NOINTERLACE)) {
		int fbp = zbp >> 1;
		db->disp[1].dispfb.FBP = fbp;
		db->draw01.frame1.FBP = fbp;
		db->draw02.frame2.FBP = fbp;
	}
}

//----------------------------------------------------------------------
int ps2_gs_swap_dbuff(ps2_gs_dbuff *db, int id)
{
	int ret = 0;

	ps2_gs_put_dispenv(&db->disp[id & 1]);
	
	if (id & 1) {
		ret = ps2_gs_put_drawenv(&db->giftag1);
	} else {
		ret = ps2_gs_put_drawenv(&db->giftag0);
	}
	
	return ret;
}

//----------------------------------------------------------------------
int ps2_gs_swap_dbuff_dc(ps2_gs_dbuffdc *db, int id)
{
	int ret = 0;
	
	ps2_gs_put_dispenv(&db->disp[id & 1]);
	if (id & 1) {
		ret = ps2_gs_put_drawenv(&db->giftag1);
	} else {
		ret = ps2_gs_put_drawenv(&db->giftag0);
	}
	
	return ret;
}

//----------------------------------------------------------------------
void ps2_gs_set_half_offset(ps2_gs_drawenv1 *draw, int halfoff)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	
	*(__u64 *)&draw->xyoffset1 = 
		PS2_GS_SETREG_XYOFFSET_1(
			((gp->center_x - ((draw->scissor1.SCAX1 + 1) >> 1)) << 4),
			((gp->center_y - ((draw->scissor1.SCAY1 + 1) >> 1)) << 4) + (halfoff ? 8 : 0));
}

//----------------------------------------------------------------------
void ps2_gs_set_half_offset2(ps2_gs_drawenv2 *draw, int  halfoff)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	
	*(__u64 *)&draw->xyoffset2 = 
		PS2_GS_SETREG_XYOFFSET_2(
			((gp->center_x - ((draw->scissor2.SCAX1 + 1) >> 1)) <<4),
			((gp->center_y - ((draw->scissor2.SCAY1 + 1) >> 1)) <<4) + (halfoff ? 8:0));
}
