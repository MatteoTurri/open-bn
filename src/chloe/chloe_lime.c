/*
 * Chloe library: ChloeLime (MIPS_TASKFLAGS get and clear).
 */

#include <stdio.h>
#include "chloe.h"

/* sysmips(2061) is MIPS_TASKFLAGS: reads the flags into sysmipsArg, then sets them to 0. */
int ChloeLime(ChloeObj *p)
{
	int r;
	p->hash = p->hash >> 8;
	r = sysmips(2061, 0, &p->sysmipsArg);
	if (r < 0)
		printf("MIPS_TASKFLAGS: get error\n");
	r = sysmips(2061, 1, 0);
	if (r < 0)
		printf("MIPS_TASKFLAGS: set error(%d)\n", r);
	return 0;
}
