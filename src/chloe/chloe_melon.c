/*
 * Chloe library: ChloeMelon.
 */

#include <crypt.h>
#include "chloe.h"

int ChloeMelon(ChloeObj *p)
{
	p->cryptResult = (int)crypt((const char *)p->propC, (const char *)p->propD);
	return 0;
}
