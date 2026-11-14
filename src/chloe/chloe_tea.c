/*
 * Chloe library: ChloeTea and ChloeCoffee.
 */

#include "chloe.h"

int ChloeTea(ChloeObj *p)
{
	p->hash += 52;
	return 52;
}

int ChloeCoffee(ChloeObj *p)
{
	return ChloeMelon(p);
}
