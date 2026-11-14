/*
 * Semaphore parameters of the sceSema API (scesema.c).
 */

#ifndef SCESEMA_H
#define SCESEMA_H

/* Parameters of sce_sema_createsema (only initCount is used) and status
   filled by ReferSemaStatus (only currentCount). */
typedef struct {
	/* 0x00 */ int currentCount;
	/* 0x04 */ int maxCount;
	/* 0x08 */ int initCount; /* initial count */
	/* 0x0C */ int numWaitThreads;
	/* 0x10 */ unsigned int attr;
	/* 0x14 */ unsigned int option;
} SemaParam; /* size = 0x18 */

#ifdef __cplusplus
extern "C" {
#endif

/* A semaphore handle is the address of a sem_t, -1 when creation failed. */
int sce_sema_createsema(SemaParam *p);
int sce_sema_deletesema(int sema);
int sce_sema_signalsema(int sema);
int sce_sema_waitsema(int sema);
int sce_sema_pollsema(int sema);
int ReferSemaStatus(int sema, SemaParam *st);

#ifdef __cplusplus
}
#endif

#endif
