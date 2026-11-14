// 
// "ps2dma.c" -- chain mode DMA emulation library
// 
// 
/*
 	Copyright (C) 2001  Sony Computer Entertainment Inc.
 
  This file is subject to the terms and conditions of the GNU Library
  General Public License Version 2. See the file "COPYING.LIB" in the 
  main directory of this archive for more details.
*/

#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/ps2/dev.h>
#include "ps2vpufile.h"
#include "ps2dma.h"

#define MAXCHAIN	32
#define STACKDEPTH	2

static struct ps2_plist s_plist;
static struct ps2_packet s_chainpkt[MAXCHAIN];
static struct ps2_packet *s_pkt;

//----------------------------------------------------------------------
static int add_chain(int fd, void *ptr, int qwc)
{
    if (ptr == NULL || s_plist.num >= MAXCHAIN) {
		if (ioctl(fd, PS2IOC_SENDL, &s_plist) < 0) {
#ifdef DEBUG
			perror("ps2_dma_start");
#endif
			return -1;
		}
		s_plist.num = 0;
		s_pkt = s_chainpkt;
    }
	
    if (ptr != NULL && qwc != 0) {
		s_pkt->ptr = ptr;
		s_pkt->len = qwc * 16;
		s_pkt++;
		s_plist.num++;
    }
	
	return 0;
}

//----------------------------------------------------------------------
int ps2_dma_wait(int fd, int n)
{
	int ret = ioctl(fd, PS2IOC_SENDQCT, n);
#ifdef DEBUG
	if (ret < 0) {
		perror("ps2_dma_wait");
	}
#endif
	
	return ret;
}

//----------------------------------------------------------------------
int ps2_dma_start_n(int fd, void *ptr, int qwc)
{
    s_plist.packet = s_chainpkt;
    s_plist.num = 0;
    s_pkt = s_chainpkt;
	
	if (add_chain(fd, ptr, qwc) < 0) {
		return -1;
	}
	if (add_chain(fd, NULL, 0) < 0) {
		return -1;
	}
	
	return 0;
}

//----------------------------------------------------------------------
int ps2_dma_send_n(int fd, void *ptr, int qwc)
{
	if (ps2_dma_start_n(fd, ptr, qwc) < 0) {
		return -1;
	}
	
	if (ps2_dma_wait(fd, 1) < 0) {
		return -1;
	}
	
	return 0;
}

//----------------------------------------------------------------------
int ps2_dma_start(int fd, VPUFILE *vfd, ps2_dmatag *tag)
{

#define RELOCATE_VFD_DATA(vfd, p) \
	((vfd) == (void *)0) ?  (void *)(p) : \
		vpuobj_relocate_addr(vfd, 1/*vpu_data*/, (void*)(p) )
/* errno: 
	EOVERFLOW: stack over/under flow
*/

	ps2_dmatag *p;
	ps2_dmatag *stack[STACKDEPTH];
	int sp = 0;
	void *q;
	
	s_plist.packet = s_chainpkt;
	s_plist.num = 0;
	s_pkt = s_chainpkt;

	p = tag;
	
	for (; ;) {
		switch (p->ID) {
		case PS2_DMATAG_CNT:
			if (add_chain(fd, p + 1, p->QWC) < 0) {
				return -1;
			}
			p += p->QWC + 1;
			break;
			
		case PS2_DMATAG_NEXT:
			if (add_chain(fd, p + 1, p->QWC) < 0) {
				return -1;
			}
			q = RELOCATE_VFD_DATA(vfd, p->ADDR);
			if ( q == (void *)-1) {
				return -1;
			}
			p = (ps2_dmatag *)q;
			break;
			
		case PS2_DMATAG_REF:
		case PS2_DMATAG_REFS:
			q = RELOCATE_VFD_DATA(vfd, p->ADDR);
			if ( q == (void *)-1) {
				return -1;
			}
			if (add_chain(fd, q, p->QWC) < 0) {
				return -1;
			}
			p++;
			break;
			
		case PS2_DMATAG_REFE:
			q = RELOCATE_VFD_DATA(vfd, p->ADDR);
			if (add_chain(fd, q, p->QWC) < 0) {
				return -1;
			}
			goto exit;
			
		case PS2_DMATAG_CALL:
			if (add_chain(fd, p + 1, p->QWC) < 0) {
				return -1;
			}
			if (sp < STACKDEPTH) {
				stack[sp++] = p + p->QWC + 1;
			} else {
#ifdef DEBUG
				fprintf(stderr, "ps2_dma_start: stack overflow\n");
#endif
				errno = EOVERFLOW;
				return -1;
			}
			q = RELOCATE_VFD_DATA(vfd, p->ADDR);
			if ( q == (void *)-1) {
				return -1;
			}
			p = (ps2_dmatag *)q;
			break;
			
		case PS2_DMATAG_RET:
			if (add_chain(fd, p + 1, p->QWC) < 0) {
				return -1;
			}
			if (sp > 0) {
				p = stack[--sp];
			} else {
#ifdef DEBUG
				fprintf(stderr, "ps2_dma_start: stack underflow\n");
#endif
				errno = EOVERFLOW;
				return -1;
			}
			break;
			
		case PS2_DMATAG_END:
			if (add_chain(fd, p + 1, p->QWC) < 0) {
				return -1;
			}
			goto exit;
		}
    }
	
exit:
	if (add_chain(fd, NULL, 0) < 0) {
		return -1;
	}
	
	return 0;
}

//----------------------------------------------------------------------
int ps2_dma_send(int fd, VPUFILE *vfd, ps2_dmatag *tag)
{
	if (ps2_dma_start(fd, vfd, tag) < 0) {
		return -1;
	}
	
	if (ps2_dma_wait(fd, 1) < 0) {
		return -1;
	}
	
	return 0;
}
