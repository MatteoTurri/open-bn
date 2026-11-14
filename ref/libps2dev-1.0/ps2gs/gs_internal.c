//
// "gs_internal.c"
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

// 
//   NI, fake NI,
//   I(frame)           I(field)
//    -->+--------+      -->>+--------+
//       |   0    |          |        |
//    -->+--------+          |  0/1   |
//       |   1    |          |        |
//    -->+--------+	      -->+--------+
//       |   Z    |	         |        |
//       +--------+	         |   Z    |
//                           |        |
//                           +--------+
// 
//----------------------------------------------------------------------
int __ps2_gs_zbuf_addr(int psm, int w, int h)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	int inter = gp->interlace_mode;
	int ffmode = gp->ff_mode;
	int omode = gp->out_mode;
	int reso = gp->resolution;
	int nw, nh;
	int s;
	
	nw = (w + 63) / 64;
	
	if (psm & 0x2){
		// 16bpp
		nh = (h + 63) / 64;
	} else {
		// 24/32bpp
		nh = (h + 31) / 32;
	}
	
	if ((omode == PS2_GS_NTSC || omode == PS2_GS_PAL || (omode == PS2_GS_DTV && reso == PS2_GS_1080I)) &&
		inter == PS2_GS_INTERLACE && ffmode == PS2_GS_FIELD) {
		s = nw * nh; // single buffer, field mode
	} else {
		s = nw * nh * 2; // double buffer, frame mode
	}
	
	return s;
}

//----------------------------------------------------------------------
int __ps2_gs_zbuf_size(int zpsm, int w, int h)
{
	int nw, nh;
	int s;
	
	nw = (w + 63) / 64;
	
	if (zpsm & 2){
		// 16bpp
		nh = (h + 63) / 64;
	} else {
		// 24/32bpp
		nh = (h + 31) / 32;
	}
	
	s = nw * nh;
	
	return s;
}

//----------------------------------------------------------------------
void __ps2_gs_put_sreg(int reg, __u64 val)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	struct ps2_gssreg r;

#if 0
 // def DEBUG	
	static const char *s_regName[] = {
		"PMODE",	// 0
		"N/A(1)",	// 1
		"SMODE2",	// 2
		"N/A(3)",	// 3
		"N/A(4)",	// 4
		"N/A(5)",	// 5
		"N/A(6)",	// 6
		"DISPFB1",	// 7
		"DISPLAY1",	// 8
		"DISPFB2",	// 9
		"DISPLAY2",	// a
		"EXTBUF",	// b
		"EXTDATA",	// c
		"EXTWRITE",	// 0
		"BGCOLOR",	// e
		"N/A(0f)",
		"N/A(10)", "N/A(11)", "N/A(12)", "N/A(13)", "N/A(14)", "N/A(15)", "N/A(16)", "N/A(17)",
		"N/A(18)", "N/A(19)", "N/A(1a)", "N/A(1b)", "N/A(1c)", "N/A(1d)", "N/A(1e)", "N/A(1f)", 
		"N/A(20)", "N/A(21)", "N/A(22)", "N/A(23)", "N/A(24)", "N/A(25)", "N/A(26)", "N/A(27)",
		"N/A(28)", "N/A(29)", "N/A(2a)", "N/A(2b)", "N/A(2c)", "N/A(2d)", "N/A(2e)", "N/A(2f)", 
		"N/A(30)", "N/A(31)", "N/A(32)", "N/A(33)", "N/A(34)", "N/A(35)", "N/A(36)", "N/A(37)",
		"N/A(38)", "N/A(39)", "N/A(3a)", "N/A(3b)", "N/A(3c)", "N/A(3d)", "N/A(3e)", "N/A(3f)", 
		"CSR",		// 40
		"IMR",		// 41
		"N/A(42)",	// 42
		"N/A(43)",	// 43
		"BUSDIR",	// 44
		"N/A(45)",	// 45
		"N/A(46)",	// 46
		"N/A(47)",	// 47
		"SIGBLID",	// 48
	};
	
#define Hi32(x) (int)(x >> 32)
#define Lo32(x) (int)(x)

	printf("__ps2_gs_put_sreg(), reg= %8s(%#4x), val= %#10x %#10x\n",
		   s_regName[reg], reg, Hi32(val), Lo32(val));
#endif // DEBUG

	r.reg = reg;
	r.val = val;
	ioctl(gp->fd_gs, PS2IOC_SGSSREG, &r);
}

//----------------------------------------------------------------------
__u64 __ps2_gs_get_sreg(int reg)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	struct ps2_gssreg r;
	r.reg = reg;
	ioctl(gp->fd_gs, PS2IOC_GGSSREG, &reg);
	
	return r.val;
}

//----------------------------------------------------------------------
int __ps2_gs_image_size(struct ps2_image *lp)
{
	int bpp, bytes;
	
	switch (lp->psm) {
	case PS2_GS_PSMCT32:
	case PS2_GS_PSMCT24:
	case PS2_GS_PSMT8H:
	case PS2_GS_PSMT4HH:
	case PS2_GS_PSMT4HL:
	case PS2_GS_PSMZ32:
	case PS2_GS_PSMZ24:
		bpp = 32;
		break;
		
	case PS2_GS_PSMCT16:
	case PS2_GS_PSMZ16:
	case PS2_GS_PSMZ16S:
		bpp = 16;
		break;

	case PS2_GS_PSMT8:
		bpp = 8;
		break;
		
	case PS2_GS_PSMT4:
		bpp = 4;
		break;
		
	default:
		fprintf(stderr, "ps2_gs_load_image(), illegal psm= %d\n", lp->psm);
		return -1;
	}
	
	bytes = (lp->w * lp->h * bpp + 7) / 8;
	
	return bytes;
}
