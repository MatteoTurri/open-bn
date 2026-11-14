/*
 * fw framework event handling over /dev/ps2event: the sync thread, the per-event
 * wait tables and their callbacks.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/ps2/dev.h>
#include "fw_ev.h"
#include "fw_gsdev.h"

/* One entry of an event wait table (11 tables of 4 entries). */
typedef struct {
	/* 0x00 */ int inUse;
	/* 0x04 */ int unk_04;
	/* 0x08 */ int event;        /* event of the table, -1 = free */
	/* 0x0C */ int targetCount;
	/* 0x10 */ int count;
	/* 0x14 */ int totalCount;
	/* 0x18 */ int pid;          /* registering process */
	/* 0x1C */ pthread_t thread; /* registering thread */
	/* 0x20 */ pthread_cond_t cond;
	/* 0x2C */ pthread_mutex_t mutex;
	/* 0x44 */ int callback;     /* function called when count reaches targetCount */
} FwEvEntry; /* size = 0x48 */

/* g_mutex_event/g_cond_event guard the opening of the event fd and the
 * "ready" signal fwEvStartThread waits for; g_fd_event is the /dev/ps2event fd.
 * g_fwEvMaskMutex guards the event mask, the table init and thread cancel. */
int g_fd_event = -1;
static pthread_mutex_t g_fwEvMaskMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_mutex_event;
pthread_cond_t g_cond_event;
static pthread_t g_fwEvThreadId;
static FwEvEntry g_fwEvTable0[4];
static FwEvEntry g_fwEvTable1[4];
static FwEvEntry g_fwEvTable2[4];
static FwEvEntry g_fwEvTable3[4];
static FwEvEntry g_fwEvTable4[4];
static FwEvEntry g_fwEvTable5[4];
static FwEvEntry g_fwEvTable6[4];
static FwEvEntry g_fwEvTable7[4];
static FwEvEntry g_fwEvTable8[4];
static FwEvEntry g_fwEvTable9[4];
static FwEvEntry g_fwEvTable10[4];
static int g_fwEvEnabled;          /* mask of the enabled events */

static FwEvEntry *fwEv_getTableBase(int which);
static int fwEv_enableEvent(int which);
static int fwEv_disableEvent(int which);

static void *fwEv_threadMain(void *arg);

static FwEvEntry *fwEv_getTableBase(int which)
{
	switch (which) {
	case PS2EV_N_VBSTART:
		return g_fwEvTable0;
	case PS2EV_N_VBEND:
		return g_fwEvTable1;
	case PS2EV_N_VIF0:
		return g_fwEvTable2;
	case PS2EV_N_VIF1:
		return g_fwEvTable3;
	case PS2EV_N_VU0:
		return g_fwEvTable4;
	case PS2EV_N_VU1:
		return g_fwEvTable5;
	case PS2EV_N_IPU:
		return g_fwEvTable6;
	case PS2EV_N_SIGNAL:
		return g_fwEvTable7;   /* there is no event 7 */
	case PS2EV_N_FINISH:
		return g_fwEvTable8;
	case PS2EV_N_HSYNC:
		return g_fwEvTable9;
	case PS2EV_N_VSYNC:
		return g_fwEvTable10;
	default:
		return 0;
	}
}

/* Disables all events, then clears the 4 entries of each of the 11 tables
 * (event = -1) and inits their condition variable and mutex. Returns 0. */
static int fwEv_initAllTables(void)
{
	int i, j;

	pthread_mutex_lock(&g_fwEvMaskMutex);
	g_fwEvEnabled = 0;
	pthread_mutex_unlock(&g_fwEvMaskMutex);

	i = 0;
	while (i < 11) {
		FwEvEntry *base;
		int next;

		switch (i) {
		case 0:
			base = g_fwEvTable0;
			break;
		case 1:
			base = g_fwEvTable1;
			break;
		case 2:
			base = g_fwEvTable2;
			break;
		case 3:
			base = g_fwEvTable3;
			break;
		case 4:
			base = g_fwEvTable4;
			break;
		case 5:
			base = g_fwEvTable5;
			break;
		case 6:
			base = g_fwEvTable6;
			break;
		case 7:
			base = g_fwEvTable7;
			break;
		case 8:
			base = g_fwEvTable8;
			break;
		case 9:
			base = g_fwEvTable9;
			break;
		case 10:
			base = g_fwEvTable10;
			break;
		default:
			return -1;
		}
		next = i + 1;
		for (j = 0; j < 4; j++) {
			base[j].inUse = 0;
			base[j].unk_04 = 0;
			base[j].event = -1;
			base[j].targetCount = 0;
			base[j].count = 0;
			base[j].totalCount = 0;
			base[j].pid = 0;
			base[j].thread = 0;
			pthread_cond_init(&base[j].cond, 0);
			pthread_mutex_init(&base[j].mutex, 0);
			base[j].callback = 0;
		}
		i = next;
	}

	return 0;
}

/* Enables event `which` if not already on. Returns 1, or -1 if the ioctl
 * fails. */
static int fwEv_enableEvent(int which)
{
	int result = 1;

	pthread_mutex_lock(&g_fwEvMaskMutex);
	if (((g_fwEvEnabled >> which) & 1) == 0) {
		int mask = g_fwEvEnabled | (1 << which);
		g_fwEvEnabled = mask;
		if (!(ioctl(g_fd_event, PS2IOC_ENABLEEVENT, mask) > -1))
			result = -1;
	}
	pthread_mutex_unlock(&g_fwEvMaskMutex);
	return result;
}

/* Disables event `which` unless an entry of its table is still armed. Returns 0,
 * -1 if an entry is still armed, 1 if the ioctl fails. */
static int fwEv_disableEvent(int which)
{
	FwEvEntry *tab = fwEv_getTableBase(which);
	int result;
	int i;

	pthread_mutex_lock(&g_fwEvMaskMutex);
	result = 0;
	if (((g_fwEvEnabled >> which) & 1) != 0) {
		for (i = 0; i < 4; i++) {
			pthread_mutex_lock(&tab[i].mutex);
			if (tab[i].inUse)
				result = -1;
			pthread_mutex_unlock(&tab[i].mutex);
		}

		if (result == 0) {
			g_fwEvEnabled &= ~(1 << which);
			if (ioctl(g_fd_event, PS2IOC_ENABLEEVENT, g_fwEvEnabled) < 0)
				result = 1;
			else
				result = 0;
		}
	}
	pthread_mutex_unlock(&g_fwEvMaskMutex);
	return result;
}

/* If the current thread already has an entry, updates its callback (when
 * cbHandle != 0); otherwise claims the first free entry for it. */
int fwEvRegistCbfunc(int handle, int cbHandle)
{
	pthread_t self;
	FwEvEntry *tab;
	int found;
	int i;

	self = pthread_self();
	tab = fwEv_getTableBase(handle);

	found = 0;
	for (i = 0; i < 4; i++) {
		pthread_mutex_lock(&tab[i].mutex);
		if (!found && tab[i].inUse != 0 && pthread_equal(tab[i].thread, self)) {
			if (cbHandle != 0)
				tab[i].callback = cbHandle;
			found = 1;
		}
		pthread_mutex_unlock(&tab[i].mutex);
		if (found)
			break;
	}

	if (!found) {
		for (i = 0; i < 4; i++) {
			pthread_mutex_lock(&tab[i].mutex);
			if (tab[i].inUse == 0 && !found) {
				tab[i].inUse = 1;
				tab[i].unk_04 = 0;
				tab[i].targetCount = 1;
				tab[i].count = 0;
				tab[i].totalCount = 0;
				tab[i].event = handle;
				tab[i].pid = getpid();
				tab[i].thread = pthread_self();
				tab[i].callback = cbHandle;
				found = 1;
			}
			pthread_mutex_unlock(&tab[i].mutex);
			if (found)
				break;
		}
	}

	return found ? 0 : -1;
}

/* If the current thread has an entry with a callback, clears the callback;
 * otherwise (cbHandle == 0) frees its entry without a callback. */
int fwEvReleaseCbfunc(int handle, int cbHandle)
{
	pthread_t self;
	FwEvEntry *tab;
	int found;
	int i;

	self = pthread_self();
	tab = fwEv_getTableBase(handle);

	found = 0;
	for (i = 0; i < 4; i++) {
		pthread_mutex_lock(&tab[i].mutex);
		if (!found && tab[i].inUse != 0 && tab[i].callback != 0 && pthread_equal(tab[i].thread, self)) {
			tab[i].callback = 0;
			found = 1;
		}
		pthread_mutex_unlock(&tab[i].mutex);
		if (found)
			break;
	}

	if (!found && cbHandle == 0) {
		for (i = 0; i < 4; i++) {
			pthread_mutex_lock(&tab[i].mutex);
			if (!found && tab[i].inUse != 0 && tab[i].callback == 0 && pthread_equal(tab[i].thread, self)) {
				found = 1;
				tab[i].inUse = 0;
				tab[i].unk_04 = 0;
				tab[i].targetCount = 0;
				tab[i].count = 0;
				tab[i].totalCount = 0;
				tab[i].event = -1;
				tab[i].pid = 0;
				tab[i].thread = 0;
				tab[i].callback = 0;
			}
			pthread_mutex_unlock(&tab[i].mutex);
			if (found)
				break;
		}
	}

	return found ? 0 : -1;
}

/* Registers the current thread on the event table and enables the event; mode must
 * be 0. The third argument is not used. */
int fwEvRegistHandle(int handle, int mode, int unused3)
{
	if (mode)
		return -1;
	fwEvRegistCbfunc(handle, 0);
	fwEv_enableEvent(handle);
	return 0;
}

/* Releases the current thread entry and disables the event if it was the last. */
int fwEvReleaseHandle(int handle)
{
	fwEvReleaseCbfunc(handle, 0);
	fwEv_disableEvent(handle);
	return 0;
}

/* Clears the current count of the first entry of the current thread. */
int fwEvClearCounter(int code)
{
	pthread_t self;
	FwEvEntry *tab;
	int result;
	int i;

	self = pthread_self();
	tab = fwEv_getTableBase(code);

	result = 0;
	i = 0;
	do {
		if (tab[i].inUse != 0 && pthread_equal(tab[i].thread, self)) {
			pthread_mutex_lock(&tab[i].mutex);
			tab[i].count = 0;
			result = 1;
			pthread_mutex_unlock(&tab[i].mutex);
		}
		if (result)
			break;
		i++;
	} while (i < 4);

	return result;
}

/* Clears the current count of every entry in use. */
int fwEvClearCounterAll(int which)
{
	int result;
	FwEvEntry *tab = fwEv_getTableBase(which);
	int i;

	result = 0;
	i = 0;

	do {
		if (tab[i].inUse != 0) {
			pthread_mutex_lock(&tab[i].mutex);
			tab[i].count = 0;
			result = 1;
			pthread_mutex_unlock(&tab[i].mutex);
		}
		i++;
	} while (i < 4);

	return result;
}

/* Sets the target count of the first entry of the current thread. */
int fwEvSetSyncCount(int handle, int val)
{
	pthread_t self;
	FwEvEntry *tab;
	int found;
	int i;

	self = pthread_self();
	tab = fwEv_getTableBase(handle);

	found = 0;
	i = 0;
	do {
		if (tab[i].inUse != 0 && pthread_equal(tab[i].thread, self)) {
			pthread_mutex_lock(&tab[i].mutex);
			tab[i].targetCount = val;
			found = 1;
			pthread_mutex_unlock(&tab[i].mutex);
		}
		if (found)
			break;
		i++;
	} while (i < 4);

	return found;
}

/* Waits on the condition variable of the current thread entry, unless the
 * target count is already reached and val != 0. Returns the current count, then
 * resets it (val == 0) or subtracts the target from it. */
int fwEvSyncWait(int handle, int val)
{
	pthread_t self;
	FwEvEntry *tab;
	int result;
	int i;

	self = pthread_self();
	tab = fwEv_getTableBase(handle);

	result = 0;
	for (i = 0; i < 4; i++) {
		if (tab[i].inUse != 0 && pthread_equal(tab[i].thread, self)) {
			pthread_mutex_lock(&tab[i].mutex);
			if (tab[i].count < tab[i].targetCount || val == 0)
				pthread_cond_wait(&tab[i].cond, &tab[i].mutex);
			result = tab[i].count;
			if (val == 0)
				tab[i].count = 0;
			else
				tab[i].count = result - tab[i].targetCount;
			pthread_mutex_unlock(&tab[i].mutex);
		}
		if (result)
			break;
	}

	return result;
}

/* Blocks the event signal (SIGUSR2) before starting the sync thread. */
static int fwEv_blockEventSignal(void)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGUSR2);
	sigprocmask(SIG_BLOCK, &set, 0);
	return 0;
}

/* Starts the sync thread at a priority halfway between SCHED_FIFO min and max,
 * then waits for its "ready" signal unless the event fd is already open or failed
 * (-30). Returns the event fd. */
int fwEvStartThread(void)
{
	int minPri, maxPri;
	struct sched_param param;
	pthread_t threadId;

	fwInitCallBack_All();
	fwEv_blockEventSignal();

	pthread_mutex_init(&g_mutex_event, 0);
	pthread_cond_init(&g_cond_event, 0);

	if (pthread_create(&g_fwEvThreadId, 0, fwEv_threadMain, 0) < 0)
		return -1;

	minPri = sched_get_priority_min(1);
	maxPri = sched_get_priority_max(1);
	param.sched_priority = (minPri + maxPri) / 2;

	threadId = g_fwEvThreadId;
	if (pthread_setschedparam(threadId, 1, &param) < 0)
		return -2;

	pthread_mutex_lock(&g_mutex_event);
	if (g_fd_event < 0) {
		if (g_fd_event != -30)
			pthread_cond_wait(&g_cond_event, &g_mutex_event);
	}
	pthread_mutex_unlock(&g_mutex_event);
	return g_fd_event;
}

/* Cancels the sync thread and waits for its end. */
int fwEvFinishThread(void)
{
	void *retval;
	pthread_cancel(g_fwEvThreadId);
	pthread_join(g_fwEvThreadId, &retval);
	return 0;
}

/* Dispatches one event: adds the driver event count to each entry; when an entry
 * reaches its target, calls its callback (no arguments) and wakes its waiter.
 * Event 0 (VBSTART) first runs the PreDmaKick callbacks and uses trylock. */
static void fwEv_dispatch(int which)
{
	int count;
	FwEvEntry *tab;
	int i;

	count = ioctl(g_fd_event, PS2IOC_EVENTCOUNT, which);
	tab = fwEv_getTableBase(which);
	if (count == 0)
		return;

	if (which == 0)
		fwExecCallBack_PreDmaKick();

	for (i = 0; i < 4; i++) {
		if (which == 0)
			pthread_mutex_trylock(&tab[i].mutex);
		else
			pthread_mutex_lock(&tab[i].mutex);

		if (tab[i].inUse != 0) {
			tab[i].count += count;
			tab[i].totalCount += count;
			if (tab[i].count >= tab[i].targetCount) {
				if (tab[i].callback != 0)
					((void (*)(void))tab[i].callback)();
				pthread_cond_signal(&tab[i].cond);
			}
		}
		pthread_mutex_unlock(&tab[i].mutex);
	}
}

/* One round of the sync thread: dispatch every enabled event, in order. The
 * mask is reread before each test. */
static void fwEv_dispatchEnabled(void)
{
	if (g_fwEvEnabled & PS2EV_VBSTART)
		fwEv_dispatch(PS2EV_N_VBSTART);
	if (g_fwEvEnabled & PS2EV_VBEND)
		fwEv_dispatch(PS2EV_N_VBEND);
	if (g_fwEvEnabled & PS2EV_VIF0)
		fwEv_dispatch(PS2EV_N_VIF0);
	if (g_fwEvEnabled & PS2EV_VIF1)
		fwEv_dispatch(PS2EV_N_VIF1);
	if (g_fwEvEnabled & PS2EV_VU0)
		fwEv_dispatch(PS2EV_N_VU0);
	if (g_fwEvEnabled & PS2EV_VU1)
		fwEv_dispatch(PS2EV_N_VU1);
	if (g_fwEvEnabled & PS2EV_IPU)
		fwEv_dispatch(PS2EV_N_IPU);
	if (g_fwEvEnabled & PS2EV_SIGNAL)
		fwEv_dispatch(PS2EV_N_SIGNAL);
	if (g_fwEvEnabled & PS2EV_FINISH)
		fwEv_dispatch(PS2EV_N_FINISH);
	if (g_fwEvEnabled & PS2EV_HSYNC)
		fwEv_dispatch(PS2EV_N_HSYNC);
	if (g_fwEvEnabled & PS2EV_VSYNC)
		fwEv_dispatch(PS2EV_N_VSYNC);
}

/* Cancels the thread of every entry in use in all 11 tables, one table at a
 * time under the mask mutex. */
static void fwEv_cancelAllThreads(void)
{
	int i = 0;

	while (i < 11) {
		FwEvEntry *e;
		FwEvEntry *end;
		int next;

		switch (i) {
		case 1:
			e = g_fwEvTable1;
			break;
		case 2:
			e = g_fwEvTable2;
			break;
		case 3:
			e = g_fwEvTable3;
			break;
		case 4:
			e = g_fwEvTable4;
			break;
		case 5:
			e = g_fwEvTable5;
			break;
		case 6:
			e = g_fwEvTable6;
			break;
		case 7:
			e = g_fwEvTable7;
			break;
		case 8:
			e = g_fwEvTable8;
			break;
		case 9:
			e = g_fwEvTable9;
			break;
		case 10:
			e = g_fwEvTable10;
			break;
		case 0:
		default:
			e = g_fwEvTable0;
			break;
		}

		pthread_mutex_lock(&g_fwEvMaskMutex);
		next = i + 1;
		end = e + 4;
		do {
			if (e->inUse != 0)
				pthread_cancel(e->thread);
			e++;
		} while ((int)e < (int)end);
		pthread_mutex_unlock(&g_fwEvMaskMutex);
		i = next;
	}
}

/* Opens /dev/ps2event if needed (-30 when the open fails), under g_mutex_event,
 * and wakes fwEvStartThread. Then arm the event signal (SIGUSR2) and sets
 * FD_CLOEXEC. Returns the fd, or -1 if the ioctl fails. */
static int fwEv_openEventFd(void)
{
	pthread_mutex_lock(&g_mutex_event);
	if (g_fd_event < 0) {
		g_fd_event = open("/dev/ps2event", O_RDWR);
		if (g_fd_event < 0)
			g_fd_event = -30;
	}
	pthread_cond_broadcast(&g_cond_event);
	pthread_mutex_unlock(&g_mutex_event);

	if (ioctl(g_fd_event, PS2IOC_SETSIGNAL, SIGUSR2) < 0)
		return -1;
	fcntl(g_fd_event, F_SETFD, FD_CLOEXEC);
	return g_fd_event;
}

/* Disarms the event signal and all events, then closes the event fd. */
static int fwEv_closeEventFd(void)
{
	if (g_fd_event >= 0) {
		if (ioctl(g_fd_event, PS2IOC_SETSIGNAL, 0) < 0)
			return -1;
		if (ioctl(g_fd_event, PS2IOC_ENABLEEVENT, 0) < 0)
			return -1;
		if (close(g_fd_event) < 0)
			return -1;
	}
	g_fd_event = -1;
	return 0;
}

/* Sync thread: ignores the event signal (SIGUSR2), inits the tables, opens the event fd (closed by a
 * cleanup handler) and waits for signals: SIGUSR2 = event arrived -> dispatch,
 * SIGINT = quit -> cancel the registered threads and close the fd. Signals are ignored
 * until the fd is valid. On error writes -1 to the argument. */
static void *fwEv_threadMain(void *arg)
{
	sigset_t set;
	struct sigaction sa;

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGUSR2, &sa, 0);

	sigemptyset(&set);
	sigaddset(&set, SIGUSR2);
	fwEv_initAllTables();

	pthread_cleanup_push((void (*)(void *))fwEv_closeEventFd, 0);
	if (fwEv_openEventFd() < 0)
		goto error;
	for (;;) {
		int sig;

		sigwait(&set, &sig);
		if (g_fd_event < 0)
			continue;
		if (sig == SIGINT)
			break;
		if (sig == SIGUSR2)
			fwEv_dispatchEnabled();
	}
	fwEv_cancelAllThreads();
	if (fwEv_closeEventFd() < 0)
		goto error;
	pthread_cleanup_pop(1);
	return arg;

error:
	*(int *)arg = -1;
	return arg;
}
