/*
  vpudev.c: sample code for vpu deivces
 
 	Copyright (C) 2000  Sony Computer Entertainment Inc.
 
  This file is subject to the terms and conditions of the GNU Library
  General Public License Version 2. See the file "COPYING.LIB" in the 
  main directory of this archive for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/ps2/dev.h>
#include <errno.h>
#include "ps2vpu.h"

#define CP2_STATUS	"$16"
#define CP2_MAC		"$17"
#define CP2_CLIPPING	"$18"

#define CP2_R		"$20"
#define CP2_I		"$21"
#define CP2_Q		"$22"

#define CP2_TCP		"$26"
#define CP2_CMSAR0	"$27"
#define CP2_FBRST	"$28"
#define 	RS0	0x2		// VPU0 reset
#define 	DE0	0x4		// VPU0 D enable
#define 	TE0	0x8		// VPU0 T enable
#define 	RS1	0x200		// VPU1 reset
#define 	DE1	0x400		// VPU1 D enable
#define 	TE1	0x800		// VPU1 T enable

#define CP2_VPU_STAT	"$29"
#define 	VBS0		0x1	// VPU0 busy
#define 	VBS1		0x100	// VPU1 busy

#define CP2_CMSAR1	"$31"

//----------------------------------------------------------------------
struct ps2_vpu_struct {
	int fd;
	int vpu;
	unsigned int mapsize;
	void *text;
	unsigned int text_size;
	void *data;
	unsigned int data_size;
};

static ps2_vpu s_vpu[2] = {{fd:-1,}, {fd:-1}};

//----------------------------------------------------------------------
void ps2_vpu_reset(ps2_vpu *dev)
{
	static  u_int init_vif_regs[8] __attribute__((aligned (16))) = {
		0x01000404, /*  STCYCL	: WL:0x04 CL:0x04 */
		0x20000000, /*  STMASK	:                 */
		0x00000000, /*  DATA	: MASK:0x00000000 */
		0x05000000, /*  STMOD	: MODE:0x0        */
		0x04000000, /*  ITOP	: ADDR:0[10bit]   */
		0x00000000, /*  NOP                       */
		0x00000000, /*  NOP                       */
		0x00000000, /*  NOP                       */
	};
	struct ps2_vifreg vifreg;
	struct ps2_packet packet;

	// VIFn_MARK=0x00000000;	/* Clear VIFn Write Mask */
	vifreg.val = 0;
	vifreg.reg = PS2_VIFREG_MARK;
	ioctl(dev->fd, PS2IOC_SVIFREG, &vifreg);

	// VIFn_ERR=0x00000000;	/* Clear VIFn Error Mask */
	vifreg.val = 0;
	vifreg.reg = PS2_VIFREG_ERR;
	ioctl(dev->fd, PS2IOC_SVIFREG, &vifreg);

	// VIFn_FBRST = 0x00000001; /* Reset VIFn */
	vifreg.val = 1;
	vifreg.reg = PS2_VIFREG_FBRST;
	ioctl(dev->fd, PS2IOC_SVIFREG, &vifreg);

	// Reset VUn
	__asm__ __volatile__(
		"cfc2.	$8," CP2_FBRST ";"	// read from FBRST
		"ctc2.	%0," CP2_FBRST ";"	// write to FBRST
		"sync.p;"
		"or	$8, %1;"
		"ctc2.	$8," CP2_FBRST ";"	// write to FBRST
		"sync.p;"
	: 
	: "r" (dev->vpu?RS1:RS0), "r" (dev->vpu?(TE1|DE1):(TE0|DE0))
	: "$8"
	);

	// Init. VIFn
	packet.ptr = init_vif_regs;
	packet.len = sizeof(init_vif_regs);
	ioctl(dev->fd, PS2IOC_SEND, &packet);
}

//----------------------------------------------------------------------
void ps2_vpu_start(ps2_vpu *dev, unsigned int start)
{
	start &= 0xffff;
	
	if (dev->vpu) 
		__asm__ __volatile__(
			"ctc2	%0," CP2_CMSAR1 ";"	// write to CMSAR1
			"sync.p;"
			"vnop;"
			"nop;"
		:
		: "r" (start)
		);
	else
		__asm__ __volatile__(
			"ctc2	%0," CP2_CMSAR0 ";"	// write to CMSAR0
			"sync.p;"
			"vnop;"
			"vcallmsr vi27;"
			"nop;"
		:
		: "r" (start)
		);

}

//----------------------------------------------------------------------
int ps2_vpu_busy(ps2_vpu *dev)
{
	register int reg=0;

	__asm__ __volatile__(
		"cfc2	%0," CP2_VPU_STAT";"	// read from vpu-STAT
		"and	%0, %1;"
		"nop;"
	: "=r" (reg)
	: "r" (dev->vpu?VBS1:VBS0)
	);
	return reg;
}

//----------------------------------------------------------------------
ps2_vpu *ps2_vpu_open(int vpu)
{
	int i;
	int fd;
	static char err[100];
	struct ps2_vpuinfo vpuinfo;
	char *dev_name;
	void *ptr;
	int mapsize;
	ps2_vpu *pvpu;

	pvpu = vpu ? &s_vpu[1] : &s_vpu[0];
	dev_name = vpu ? PS2_DEV_VPU1 : PS2_DEV_VPU0;

	if (pvpu->fd != -1) {
		errno = EBUSY;
		return (void *)-1;
	}

	fd = open(dev_name, O_RDWR);
	if (fd<0){
#ifdef DEBUG
		sprintf(err, "%s:open", dev_name);
		perror(err);
#endif
		return (void *)-1;
	}

	i = ioctl(fd, PS2IOC_VPUINFO, &vpuinfo);
	if (i<0) {
		sprintf(err, "%s:PS2IOC_VPUINFO", dev_name);
		perror(err);
		close(fd);
		return (void *)-1;
	}
	mapsize = vpuinfo.umemsize+vpuinfo.vumemsize;

	ptr = mmap(NULL, mapsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == (void*) -1 ) {
#ifdef DEBUG
		sprintf(err, "%s:mmap", dev_name);
		perror(err);
#endif
		close(fd);
		return (void *)-1;
	}

	pvpu->vpu = vpu;
	pvpu->fd = fd;
	pvpu->text = ptr;
	pvpu->text_size = vpuinfo.umemsize;
	pvpu->data = ptr+vpuinfo.umemsize;
	pvpu->data_size = vpuinfo.vumemsize;
	pvpu->mapsize = mapsize;

	return pvpu;

}

//----------------------------------------------------------------------
int ps2_vpu_text_size(ps2_vpu *pvpu)
{
	return pvpu->text_size;
}

//----------------------------------------------------------------------
void *ps2_vpu_text(ps2_vpu *pvpu)
{
	return pvpu->text;
}

//----------------------------------------------------------------------
int ps2_vpu_data_size(ps2_vpu *pvpu)
{
	return pvpu->data_size;
}

//----------------------------------------------------------------------
void *ps2_vpu_data(ps2_vpu *pvpu)
{
	return pvpu->data;
}

//----------------------------------------------------------------------
int ps2_vpu_close(ps2_vpu *pvpu)
{
	if (munmap(pvpu->text, pvpu->mapsize) < 0) {
#ifdef DEBUG
	  perror("ps2_vpu_close, munmap");
#endif
	  return -1;
	}
	
	if (close(pvpu->fd) < 0) {
#ifdef DEBUG
	  perror("ps2_vpu_close, close");
#endif
	  return -1;
	}
	
	pvpu->fd = -1;
	
	return 0;
}

//----------------------------------------------------------------------
int ps2_vpu_fd(ps2_vpu *pvpu)
{
	return pvpu->fd;
}
