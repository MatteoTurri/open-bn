//
// "gs_gparam.c"
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

static ps2_gs_gparam __ps2_gs_gparam = { -1 };

//----------------------------------------------------------------------
static void __ps2_gs_calc_offset(void)
{
	ps2_gs_gparam *gp = &__ps2_gs_gparam;
	
	gp->offset_x = gp->center_x - gp->width / 2;
	gp->offset_y = gp->center_y - gp->height / 2;
}

//----------------------------------------------------------------------
ps2_gs_gparam *ps2_gs_get_gparam(void)
{
	return &__ps2_gs_gparam;
}

//----------------------------------------------------------------------
void __ps2_gs_set_screen_size(int w, int h)
{
	ps2_gs_gparam *gp = &__ps2_gs_gparam;
	
	gp->width = w;
	gp->height = h;
	
#define PS2_GS_NTSC_PIXEL_RATIO 0.47f
	gp->pixel_ratio = (gp->out_mode == PS2_GS_VESA) ? 1.0f : (PS2_GS_NTSC_PIXEL_RATIO / 0.5f);
	if (gp->interlace_format && (gp->interlace_mode == PS2_GS_NOINTERLACE || gp->ff_mode == PS2_GS_FRAME)) {
		// NI or I(frame)
		gp->pixel_ratio /= 2;
	}
	
	__ps2_gs_calc_offset();
}

//----------------------------------------------------------------------
void ps2_gs_set_center(int cx, int cy)
{
	ps2_gs_gparam *gp = &__ps2_gs_gparam;
	
	gp->center_x = cx;
	gp->center_y = cy;
	
	__ps2_gs_calc_offset();
}
