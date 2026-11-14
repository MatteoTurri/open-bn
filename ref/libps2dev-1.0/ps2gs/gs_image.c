//
// "gs_image.c"
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
int ps2_gs_set_image(struct ps2_image *pImg, int fbp, int fbw, int psm, int x, int y, int w, int h, void *ptr)
{
	pImg->fbp = fbp;
	pImg->fbw = fbw;
	pImg->psm = psm;
	pImg->x = x;
	pImg->y = y;
	pImg->w = w;
	pImg->h = h;
	pImg->ptr = ptr;
	
	return 0;
}

//----------------------------------------------------------------------
int ps2_gs_load_image(struct ps2_image *lp)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	
	if (ioctl(gp->fd_gs, PS2IOC_LOADIMAGE, lp) < 0) {
		perror("ps2_gs_load_image");
		
		return -1;
	}
	
	return __ps2_gs_image_size(lp);
}

//----------------------------------------------------------------------
int ps2_gs_store_image(struct ps2_image *sp)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	
	if (ioctl(gp->fd_gs, PS2IOC_STOREIMAGE, sp) < 0) {
		perror("ps2_gs_store_image");
		return -1;
	}
	
	return __ps2_gs_image_size(sp);
}
