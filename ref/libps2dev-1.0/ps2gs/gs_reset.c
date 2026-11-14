//
// "gs_reset.c"
//

/*
 	Copyright (C) 2001  Sony Computer Entertainment Inc.
 
  This file is subject to the terms and conditions of the GNU Library
  General Public License Version 2. See the file "COPYING.LIB" in the 
  main directory of this archive for more details.
*/

#include "gs_internal.h"

//----------------------------------------------------------------------
int __ps2_gs_reset_check_param(int mode, int inter, int omode, int ffmode, int resolution, int refreshRate)
{
	if (mode == 1) {
		return 0;
	}
	
	if (mode != 0) {
		return -1;
	}
	
	// check omode/resolution/refreshRate
	switch (omode) {
	case PS2_GS_VESA:
		switch (resolution) {
		case PS2_GS_640x480:
		case PS2_GS_800x600:
		case PS2_GS_1024x768:
		case PS2_GS_1280x1024:
			switch (refreshRate) {
			case PS2_GS_60Hz:
			case PS2_GS_75Hz:
				break;
				
			default:
				return -1;
			}
			break;
					
		default:
			return -1;
		}
		break;
		
	case PS2_GS_DTV:
		switch (resolution) {
		case PS2_GS_480P:
		case PS2_GS_1080I:
		case PS2_GS_720P:
			break;
					
		default:
			return -1;
		}
		break;
				
	case PS2_GS_NTSC:
	case PS2_GS_PAL:
		break;
		
	default:
		return -1;
		
	} // switch (omode)
	
	/* Version linked into bn: the second switch covers only NTSC(2) and
	 * PAL(3), NOT DTV(1) -- the original skips with `slti a2,2`
	 * (omode<2 -> end), while the libps2dev 1.0 source, with DTV, gives
	 * `blez a2` (omode<1). */
	// check inter/ffmode
	switch (omode) {
	case PS2_GS_NTSC:
	case PS2_GS_PAL:
		switch (inter) {
		case PS2_GS_INTERLACE:
			switch (ffmode) {
			case PS2_GS_FIELD:
			case PS2_GS_FRAME:
				break;
					
			default:
				return -1;
			}
			break;
			
		case PS2_GS_NOINTERLACE:
			break;
			
		default:
			return -1;
		}
	}
	
	return 0;
}

//----------------------------------------------------------------------
int ps2_gs_start_display(int start)
{
	struct ps2_pmode pmode;
	int ret;
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	
	ret = ioctl(gp->fd_gs, PS2IOC_GPMODE, &pmode);
	if (ret < 0) {
		return -1;
	}
	
	pmode.sw = 2;
	
	return ioctl(gp->fd_gs, PS2IOC_SPMODE, &pmode);
}

//----------------------------------------------------------------------
int ps2_gs_reset(int mode, int inter, int omode, int ffmode, int resolution, int refreshRate)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	
	if (__ps2_gs_reset_check_param(mode, inter, omode, ffmode, resolution, refreshRate) < 0) {
		return -1;
	}
	
	switch (mode) {
	case 1:
		// stop/flush GS
		return ioctl(gp->fd_gs, PS2IOC_GSRESET, PS2_GSRESET_GS);
		
	case 0:
	{
		int ret;
		__u64 csr;
		
		//struct ps2_screeninfo sinfo;
		struct ps2_crtmode crtmode;
		struct ps2_display display2;
		struct ps2_pmode pmode;
		int width, height;
		int interlaceFormat;
		
		// full reset GS
		ret = ioctl(gp->fd_gs, PS2IOC_GSRESET, PS2_GSRESET_FULL);
		if (ret < 0) {
			perror("ps2_gs_reset, ioctl PS2_GSRESET_FULL");
			return -1;
		}
		
		// set pmode, display off
		ret = ioctl(gp->fd_gs, PS2IOC_GPMODE, &pmode);
		pmode.sw = 0; // ch1: off, ch2: off
		ret = ioctl(gp->fd_gs, PS2IOC_SPMODE, &pmode);
		
		/* Version linked into bn: DTV 1080I does NOT count as
		 * interlaced (`addiu -2` + `sltiu 2` and nothing else), unlike
		 * the libps2dev 1.0 source. */
		interlaceFormat = (omode == PS2_GS_NTSC || omode == PS2_GS_PAL);

		// get GS version
		csr = __ps2_gs_get_sreg(PS2_GSSREG_CSR);
		gp->version = (csr >> 16) & 0xff;
		
		gp->interlace_mode = inter;
		gp->out_mode = omode;
		gp->ff_mode = ffmode;
		gp->resolution = resolution;
		gp->refresh_rate = refreshRate;
		gp->interlace_format = interlaceFormat;
		width = 640;
		height = 480;
		
		switch (omode) {
		case PS2_GS_NTSC:
		case PS2_GS_PAL:
			width = 640;
			height = (omode == PS2_GS_NTSC) ? 448 : 512;
			break;
			
		case PS2_GS_DTV:
			switch (resolution) {
			case PS2_GS_480P: width = 720; height = 480; break;
			case PS2_GS_1080I: width = 1920; height = 1080; break;
			/* version linked into bn: 720P is 1280x720, not 1024x720 */
			case PS2_GS_720P: width = 1280; height = 720; break;
			}
			break;
			
		case PS2_GS_VESA:
			switch (resolution) {
			case PS2_GS_640x480: width = 640; height = 480; break;
			case PS2_GS_800x600: width = 800; height = 600; break;
			case PS2_GS_1024x768: width = 1024; height = 768; break;
			case PS2_GS_1280x1024: width = 1280; height = 1024; break;
			}
		}
		
		if (interlaceFormat && (inter == PS2_GS_NOINTERLACE || ffmode == PS2_GS_FRAME)) {
			// fake NI or I(frame)
			height /= 2;
		}
		
		ps2_gs_set_center(2048, 2048);
		__ps2_gs_set_screen_size(width, height);
		
		// set screen info
#if 0
		sinfo.mode = gp->out_mode;
		sinfo.fbp = 0;
		sinfo.psm = PS2_GS_PSMCT32;
		sinfo.w = gp->width;
		sinfo.h = gp->height;
		if (interlaceFormat && (inter == PS2_GS_INTERLACE && ffmode == PS2_GS_FRAME)) {
			sinfo.h *= 2;
		}
#endif
		crtmode.mode = gp->out_mode;
		
		switch (gp->out_mode) {
		case PS2_GS_NTSC:
		case PS2_GS_PAL:
			crtmode.res = gp->interlace_mode | gp->ff_mode;
			break;

		/* version linked into bn: DTV has a branch of its own,
		 * res = resolution (three distinct bodies in the switch tree) */
		case PS2_GS_DTV:
			crtmode.res = gp->resolution;
			break;
			
		case PS2_GS_VESA:
			crtmode.res = gp->resolution | gp->refresh_rate;
			break;
			
		default:
			return -1;
		}
		
		//ret = ioctl(gp->fd_gs, PS2IOC_SSCREENINFO, &sinfo);
		ret = ioctl(gp->fd_gs, PS2IOC_SCRTMODE, &crtmode);
		if (ret < 0) {
			perror("ps2_gs_reset, PS2IOC_SSCREENINFO");
			return -1;
		}
		
		// display
		display2.ch = 1; // use region 2
		display2.w = gp->width;
		display2.h = gp->height;
		display2.dx = 0;
		display2.dy = 0;
		if (interlaceFormat && (inter == PS2_GS_INTERLACE && ffmode == PS2_GS_FRAME)) {
			display2.h *= 2;
		}
		
		ret = ioctl(gp->fd_gs, PS2IOC_SDISPLAY, &display2);
		if (ret < 0) {
			perror("ps2_gs_reset, PS2IOC_SDISPLAY");
			return -1;
		}

#if 0		
		// clear buffers...
		{
			static ps2_gs_dbuff db __attribute__((aligned(16)));
			static ps2_gs_finish finish __attribute__((aligned(16)));
			
			ps2_gs_set_dbuff(&db, PS2_GS_PSMCT32, width, height, 0, PS2_GS_PSMZ24, 1);
			ps2_gs_put_drawenv(&db.giftag0);
			ps2_gs_put_drawenv(&db.giftag1);
			
			ps2_gs_set_finish(&finish);
			ps2_gs_wait_finish(&finish);
			
			ps2_gs_put_dispenv(&db.disp[0]);
		}
		
		// set pmode, start display
		ret = ioctl(gp->fd_gs, PS2IOC_GPMODE, &pmode);
		pmode.sw = 2; // ch1: off, ch2: on
		ret = ioctl(gp->fd_gs, PS2IOC_SPMODE, &pmode);
#endif

		break; // case 0;
	}
		
	default:
		return -1;
		
	} // switch (mode)
	
	return 0;
}
