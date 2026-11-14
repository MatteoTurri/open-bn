//
// "gs_open.c"
//

/*
 	Copyright (C) 2001  Sony Computer Entertainment Inc.
 
  This file is subject to the terms and conditions of the GNU Library
  General Public License Version 2. See the file "COPYING.LIB" in the 
  main directory of this archive for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/fcntl.h>
#include "gs_internal.h"

//----------------------------------------------------------------------
int ps2_gs_open(int fd_event)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	int ret;
	
	gp->fd_gs = open(PS2_DEV_GS, O_RDWR);
	if (gp->fd_gs < 0) {
		perror("ps2_gs_open, gs");
		return -1;
	}

	ioctl(gp->fd_gs, PS2IOC_GSCREENINFO, &gp->oldinfo);
	
	if (fd_event >= 0) {
		gp->fd_event = fd_event;
		gp->fd_event_open = 0;
	} else {
		gp->fd_event = open(PS2_DEV_EVENT, O_RDWR);
		gp->fd_event_open = 1;
		if (gp->fd_event < 0) {
			perror("ps2_gs_open, event");
			return -1;
		}
	}
	
	ret = ioctl(gp->fd_event, PS2IOC_ENABLEEVENT, PS2EV_VBSTART | PS2EV_FINISH);
	if (ret < 0) {
		perror("ps2_gs_open, ENABLEEVENT");
		return -1;
	}
	
	return gp->fd_gs;
}

//----------------------------------------------------------------------
int ps2_gs_close(void)
{
	ps2_gs_gparam *gp = ps2_gs_get_gparam();
	
	if (gp->fd_gs < 0) {
		return -1;
	}
	
	if (gp->fd_event_open) {
		if (close(gp->fd_event) < 0) {
			return -1;
		}
		gp->fd_event = -1;
	}

	ioctl(gp->fd_gs, PS2IOC_SSCREENINFO, &gp->oldinfo);
	
	if (close(gp->fd_gs) < 0) {
		return -1;
	}
	
	gp->fd_gs = -1;
	
	return 0;
}
