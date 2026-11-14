/*
 * Chloe library: ChloeMandarin, a probe for the /dev/akmem device.
 */

#include "chloe.h"

/* The device name is decoded in place: the tail of "/dev/ps2vpu1" becomes
   "akmem" for the probe and is put back afterwards. */
static char chloe_mandarin_buf[] = "/dev/ps2vpu1";

/* Probes "/dev/akmem" through p->lstatFn and fails with -19 (ENODEV) if the
   call fails or word 36 of the result is not 266. */
int ChloeMandarin(ChloeObj *p)
{
	char sysinfo[144];
	int r;

	p->hash += 0x1221123;

	chloe_mandarin_buf[5] = 'a';
	chloe_mandarin_buf[7] = 109;
	chloe_mandarin_buf[9] = 109;
	chloe_mandarin_buf[10] = 0;
	chloe_mandarin_buf[6] ^= 0x18;
	chloe_mandarin_buf[8] += -17;

	r = p->lstatFn(chloe_mandarin_buf, sysinfo);

	chloe_mandarin_buf[5] = 'p';
	chloe_mandarin_buf[6] = 's';
	chloe_mandarin_buf[7] = '2';
	chloe_mandarin_buf[8] = 'v';
	chloe_mandarin_buf[9] = 'p';
	chloe_mandarin_buf[10] = 'u';

	if (r < 0) {
		p->pageSize = 0;
		return -19;
	}
	if (*(int *)(sysinfo + 36) != 266) {
		p->pageSize = 0;
		return -19;
	}
	return 0;
}
