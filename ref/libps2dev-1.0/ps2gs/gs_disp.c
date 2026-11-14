//
// "gs_disp.c"
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
void ps2_gs_set_dispenv(ps2_gs_dispenv *disp, int psm, int w)
{
	*(__u64 *)&disp->dispfb = 0;
	disp->dispfb.FBP = 0;
	disp->dispfb.FBW = (w + 63) / 64;
	disp->dispfb.PSM = psm;
	disp->dispfb.DBX = 0;
	disp->dispfb.DBY = 0;
	
	*(__u64 *)&disp->bgcolor = 0;
}

//----------------------------------------------------------------------
void ps2_gs_put_dispenv(ps2_gs_dispenv *disp)
{
	__ps2_gs_put_sreg(PS2_GSSREG_DISPFB2, *(__u64 *)&disp->dispfb);
	__ps2_gs_put_sreg(PS2_GSSREG_BGCOLOR, *(__u64 *)&disp->bgcolor);
}

