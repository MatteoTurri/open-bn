//
// "gs_tex.c"
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
int ps2_gs_set_texenv(ps2_gs_texenv *tp, int flush,
					  struct ps2_image *image,
					  int w2, int h2,
					  int tfx, int cbp, int cpsm, int cld, int filter)
{
	int tbp0 = image->fbp;
	int tbw = image->fbw;
	int psm = image->psm;
	
	if (flush) {
		tp->texflushaddr = PS2_GS_TEXFLUSH;
	} else {
		tp->texflushaddr = PS2_GS_NOP;
	}
	*(__u64 *)&tp->texflush = 0;
	
	tp->tex01addr = PS2_GS_TEX0_1;
	*(__u64 *)&tp->tex01 =
		PS2_GS_SETREG_TEX0(tbp0, tbw, psm, w2, h2, PS2_GS_TEX_TCC_RGBA, tfx,
						   cbp, cpsm, PS2_GS_TEX_CSM_CSM1, 0, cld); 
	
	tp->tex11addr = PS2_GS_TEX1_1;
	*(__u64 *)&tp->tex11 =
		PS2_GS_SETREG_TEX1(PS2_GS_TEX1_LCM_CALC, 0, filter & 1, filter,
						   PS2_GS_TEX1_MTBA_NOAUTO, 0, 0);
	
	tp->clamp1addr = PS2_GS_CLAMP_1;
	*(__u64 *)&tp->clamp1 =
		PS2_GS_SETREG_CLAMP(PS2_GS_CLAMP_CLAMP, PS2_GS_CLAMP_CLAMP, 0, 0, 0, 0);
	
	return sizeof(ps2_gs_texenv) / 16;
}

//----------------------------------------------------------------------
int ps2_gs_set_texenv2(ps2_gs_texenv2 *tp, int flush,
					   struct ps2_image *image,
					   int w2, int h2,
					   int tfx, int cbp, int cpsm,
					   int cld, int filter)
{
	int tbp0 = image->fbp;
	int tbw = image->fbw;
	int psm = image->psm;
	
	if (flush) {
		tp->texflushaddr = PS2_GS_TEXFLUSH;
	} else {
		tp->texflushaddr = PS2_GS_NOP;
	}
	*(__u64 *)&tp->texflush = 0;
	tp->tex02addr = PS2_GS_TEX0_2;
	*(__u64 *)&tp->tex02 =
		PS2_GS_SETREG_TEX0(tbp0, tbw, psm, w2, h2, PS2_GS_TEX_TCC_RGBA, tfx,
						   cbp, cpsm, PS2_GS_TEX_CSM_CSM1, 0, cld); 
	tp->tex12addr = PS2_GS_TEX1_2;
	*(__u64 *)&tp->tex12 =
		PS2_GS_SETREG_TEX1(PS2_GS_TEX1_LCM_CALC, 0, filter & 1, filter,
						   PS2_GS_TEX1_MTBA_NOAUTO, 0, 0);
	tp->clamp2addr = PS2_GS_CLAMP_2;
	*(__u64 *)&tp->clamp2 =
		PS2_GS_SETREG_CLAMP(PS2_GS_CLAMP_CLAMP, PS2_GS_CLAMP_CLAMP, 0, 0, 0, 0);
	
	return sizeof(ps2_gs_texenv2) / 16;
}
