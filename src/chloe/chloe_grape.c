/*
 * Chloe library: ChloeGrape.
 */

#include <unistd.h>
#include "chloe.h"

int ChloeGrape(ChloeObj *p)
{
	return geteuid();
}
