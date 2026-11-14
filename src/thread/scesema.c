/*
 * sceSema API of the PS2 kernel on top of the POSIX semaphores of PS2 Linux:
 * a semaphore handle is the address of a malloc'ed sem_t.
 */

#include <stdlib.h>
#include <semaphore.h>
#include "scesema.h"

/* Allocates and initializes a semaphore; returns -1 on failure. */
int sce_sema_createsema(SemaParam *p)
{
	sem_t *sem;
	sem_t *sem2;
	int r;

	sem = malloc(16);
	sem2 = sem;
	if (sem) {
		r = sem_init(sem, 0, p->initCount);
		if (r >= 0)
			return (int)sem;
	}
	if (sem2)  /* redundant as written */
		free(sem2);
	return -1;
}

/* Frees the semaphore and returns the handle just freed. */
int sce_sema_deletesema(int sema)
{
	free((void *)sema);
	return sema;
}

int sce_sema_signalsema(int sema)
{
	sem_post((sem_t *)sema);
	return 0;
}

int sce_sema_waitsema(int sema)
{
	sem_wait((sem_t *)sema);
	return 0;
}

int sce_sema_pollsema(int sema)
{
	return sem_trywait((sem_t *)sema);
}

/* Stores the current count in st->currentCount; always returns 0. */
int ReferSemaStatus(int sema, SemaParam *st)
{
	int val;

	sem_getvalue((sem_t *)sema, &val);
	st->currentCount = val;
	return 0;
}
