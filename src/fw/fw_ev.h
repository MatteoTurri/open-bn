/*
 * fwEv*: event handling and the sync thread of the fw framework (fw_ev.c).
 */

#ifndef FW_EV_H
#define FW_EV_H

/* The handles are the event numbers PS2EV_N_* of the kit. */
#include <linux/ps2/dev.h>

#ifdef __cplusplus
extern "C" {
#endif

int fwEvRegistCbfunc(int handle, int cbHandle);
int fwEvReleaseCbfunc(int handle, int cbHandle);
int fwEvRegistHandle(int handle, int mode, int unused3);
int fwEvReleaseHandle(int handle);
int fwEvClearCounter(int code);
int fwEvClearCounterAll(int which);
int fwEvSetSyncCount(int handle, int val);
int fwEvSyncWait(int handle, int val);
int fwEvStartThread(void);
int fwEvFinishThread(void);

#ifdef __cplusplus
}
#endif

#endif
