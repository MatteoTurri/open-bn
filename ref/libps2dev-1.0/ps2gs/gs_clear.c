//
// "gs_clear.c"
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
int ps2_gs_set_clear(ps2_gs_clear *cp, int ztest, int x, int y,
					 int w, int h, int r, int g, int b, int a, __u32 z)
{
	int zt;
	
	cp->testaaddr = PS2_GS_TEST_1;
	*(__u64 *)&cp->testa = PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, 1, 1);
	
	cp->primaddr = PS2_GS_PRIM;
	PS2_GS_CLEAR_GSREG(&cp->prim);
	cp->prim.PRIM = PS2_GS_PRIM_PRIM_SPRITE;
	
	cp->rgbaqaddr = PS2_GS_RGBAQ;
	*(__u64 *)&cp->rgbaq = PS2_GS_SETREG_RGBAQ(r & 0xff, g & 0xff, b & 0xff, a & 0xff, 0x3f800000);
	
	cp->xyz2aaddr = PS2_GS_XYZ2;
	*(__u64 *)&cp->xyz2a = PS2_GS_SETREG_XYZ(x << 4, y << 4, z);
	
	cp->xyz2baddr = PS2_GS_XYZ2;
	*(__u64 *)&cp->xyz2b = PS2_GS_SETREG_XYZ((x + w) << 4, (y + h) << 4, z);
	
	cp->testbaddr = PS2_GS_TEST_1;
	zt = ztest ? (ztest & 3) : PS2_GS_TEST_ZTST_ALWAYS;
	*(__u64 *)&cp->testb =
		PS2_GS_SETREG_TEST(PS2_GS_TEST_ATE_OFF,
						   PS2_GS_TEST_ATST_NEVER,
						   0,
						   PS2_GS_TEST_AFAIL_KEEP,
						   PS2_GS_TEST_DATE_OFF,
						   PS2_GS_TEST_DATM_PASS0,
						   PS2_GS_TEST_ZTE_ON,
						   zt);
	
	return sizeof(ps2_gs_clear) / 16;
}

//----------------------------------------------------------------------
int ps2_gs_set_clear2(ps2_gs_clear *cp, int ztest, int x, int y,
					  int w, int h, int r, int g, int b, int a, __u32 z)
{
	int zt;
	
	cp->testaaddr = PS2_GS_TEST_2;
	*(__u64 *)&cp->testa = PS2_GS_SETREG_TEST(0, 0, 0, 0, 0, 0, 1, 1);
	
	cp->primaddr = PS2_GS_PRIM;
	PS2_GS_CLEAR_GSREG(&cp->prim);
	cp->prim.PRIM = PS2_GS_PRIM_PRIM_SPRITE;
	cp->prim.CTXT = PS2_GS_PRIM_CTXT_CONTEXT2;
	
	cp->rgbaqaddr = PS2_GS_RGBAQ;
	*(__u64 *)&cp->rgbaq = PS2_GS_SETREG_RGBAQ(r, g, b, a, 0x3f800000);
	
	cp->xyz2aaddr = PS2_GS_XYZ2;
	*(__u64 *)&cp->xyz2a = PS2_GS_SETREG_XYZ(x << 4, y << 4, z);
	
	cp->xyz2baddr = PS2_GS_XYZ2;
	*(__u64 *)&cp->xyz2b = PS2_GS_SETREG_XYZ((x + w) << 4, (y + h) << 4, z);
	
	cp->testbaddr = PS2_GS_TEST_2;
	zt = ztest ? (ztest & 3) : PS2_GS_TEST_ZTST_ALWAYS;
	*(__u64 *)&cp->testb =
		PS2_GS_SETREG_TEST(PS2_GS_TEST_ATE_OFF,
						   PS2_GS_TEST_ATST_NEVER,
						   0,
						   PS2_GS_TEST_AFAIL_KEEP,
						   PS2_GS_TEST_DATE_OFF,
						   PS2_GS_TEST_DATM_PASS0,
						   PS2_GS_TEST_ZTE_ON,
						   zt);
	
	return sizeof(ps2_gs_clear) / 16;
}
