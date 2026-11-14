/*
 * Chloe library: ChloePie and the magic check ChloeTart.
 */

#include "chloe.h"

int ChloePie(ChloeObj *p)
{
	p->hash += 384;
	return 0;
}

/* Checks the hash against the expected magic.
   NOTE: bug: no return value on the success path. */
int ChloeTart(ChloeObj *p)
{
	if (p->hash != 0x1688c1c)
		return -12;
}
