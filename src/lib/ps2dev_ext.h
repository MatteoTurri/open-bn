/*
 * Requests of the PS2 Linux device drivers (/dev/ps2*) that the kit's
 * <linux/ps2/dev.h> does not define.
 */

#ifndef PS2DEV_EXT_H
#define PS2DEV_EXT_H

#include <sys/ioctl.h>
#include <linux/ps2/dev.h>

/* Physical addresses of the pages mapped from the device, one word per page. */
#define PS2IOC_PHYSADDR _IO(PS2IOC_MAGIC, 0x80)

#endif
