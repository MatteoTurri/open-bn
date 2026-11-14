/*
 * Chloe library: ChloeCrisp.
 */

#include "chloe.h"

/* Records the ChloeKnife..ChloeFork blob size, then installs the device B table. */
int ChloeCrisp(ChloeObj *p)
{
	p->blobDistance = (int)ChloeFork - (int)ChloeKnife;
	return ChloeYogurt(p);
}
