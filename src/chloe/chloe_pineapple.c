/*
 * Chloe library: ChloePineapple.
 */

#include "chloe.h"

int ChloePineapple(ChloeObj *p)
{
	p->hash ^= 0x11111111;
	return 0;
}
