/*
 * Chloe library: ChloeScone and ChloePekoe (reboot).
 */

#include <unistd.h>
#include "chloe.h"

/* Clears the first four table slots through the object's own memsetFn. */
int ChloeScone(ChloeObj *p)
{
	p->memsetFn(&p->openFn, 0, 16);
	return 0;
}

/* Same command bn_do_reboot ends with (bnexec.cpp). */
int ChloePekoe(ChloeObj *p)
{
	return execl("/sbin/reboot", "/sbin/reboot", 0);
}
