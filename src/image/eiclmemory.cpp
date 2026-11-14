/*
 * EiclZeroMemory: memset to zero.
 */

#include <string.h>
#include "eiclmemory.h"

void *EiclZeroMemory(void *p, int n)
{
	return memset(p, 0, n);
}
