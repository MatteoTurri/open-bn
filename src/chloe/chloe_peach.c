/*
 * Chloe library: property setters.
 */

#include "chloe.h"

int ChloePeach(ChloeObj *p, int propA, int propB)
{
	p->propA = propA;
	p->propB = propB;
	p->hash += 0x8CD2434;
	return 0;
}

int ChloeCocoa(ChloeObj *p, int propC, int propD)
{
	p->propC = propC;
	p->propD = propD;
	p->hash <<= 3;
	return 0;
}
