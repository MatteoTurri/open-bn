/*
 * sceBNSysup*: the C interface of the DNAS2 system update.
 */

#ifndef SCEBNSYSUP_H
#define SCEBNSYSUP_H

struct sceBNSysupStatus {
	/* 0x00 */ unsigned int state;   /* 1 idle, 2 running, 3 done, 4 failed */
	/* 0x04 */ unsigned int result;  /* 2 ok, 3 ok with marker; error code if failed */
	/* 0x08 */ unsigned int total_steps;
	/* 0x0C */ unsigned int step;    /* steps completed */
	/* 0x10 */ char reserved[144 - 16];
}; /* size = 0x90 */

#ifdef __cplusplus
extern "C" {
#endif

/* The two arguments of Init, Start and Exit are not used. */
int sceBNSysupInit(int, int);
int sceBNSysupStart(int, int);
int sceBNSysupExit(int, int);
int sceBNSysupGetStatus(struct sceBNSysupStatus *out);

#ifdef __cplusplus
}
#endif

#endif
