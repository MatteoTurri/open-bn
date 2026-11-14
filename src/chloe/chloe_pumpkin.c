/*
 * Chloe library: thunks.
 */

#include <unistd.h>
#include "chloe.h"

int ChloePumpkin(ChloeObj *p)
{
	getuid();
	ChloeGrape(p);
	return 0;
}

int ChloeCrumble(ChloeObj *p)
{
	ChloeMilk(p);
	return 0;
}

int ChloeTapioca(ChloeObj *p)
{
	return ChloeSherbet(p);
}
