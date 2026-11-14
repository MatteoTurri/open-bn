/*
 * Chloe library: ChloeStrawberry.
 */

#include "chloe.h"

/* ChloeStrawberry and ChloePizza call each other: neither returns. */
int ChloeStrawberry(ChloeObj *p)
{
	return ChloePizza(p);
}
