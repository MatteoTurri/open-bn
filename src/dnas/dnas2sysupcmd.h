/*
 * The command list of a DNAS2 system update, built by DNAS2SysupManager and
 * walked by DNAS2Sysupdater.
 */

#ifndef DNAS2SYSUPCMD_H
#define DNAS2SYSUPCMD_H

/*
 * Node of the command list. Tag 1001 starts an RPM section (data points to
 * the number of nodes in it), 2001 is an RPM package (data points to an
 * RPMData), 3001 ends the section; 1002-1004 are the kernel, MBR and PFS
 * steps, 1005 a marker.
 */
struct sysupCommandList {
	/* 0x00 */ unsigned int tag;
	/* 0x04 */ void *data;
	/* 0x08 */ sysupCommandList *next;
}; /* size = 0x0C */

#endif
