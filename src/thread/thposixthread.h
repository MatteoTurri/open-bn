/*
 * thPosixThread: base class of the program's worker threads, with the static
 * thread lists and the six idle queues scheduled by checkThread().
 */

#ifndef THPOSIXTHREAD_H
#define THPOSIXTHREAD_H

#include <pthread.h>
#include <list>
#include <vector>
#include <queue>

class thPosixThread;

class thPosixThread;

struct thThreadQueueItem {
	/* 0x00 */ thPosixThread *thread;
	/* 0x04 */ unsigned int key;     /* sort key, compared unsigned */

	thThreadQueueItem(thPosixThread *t, unsigned int k) : thread(t), key(k) {}
	~thThreadQueueItem() {}
};

int operator<(const thThreadQueueItem &a, const thThreadQueueItem &b);

typedef priority_queue<thThreadQueueItem, vector<thThreadQueueItem>, less<thThreadQueueItem> > thThreadQueue;

/*
 * Worker thread. A subclass supplies threadProc() (the body) and
 * threadFinished() (the completion check); checkThread() moves each thread
 * through ST_IDLE -> ST_RUNNING -> ST_CALL_FINISHED -> ST_END / ST_RELEASE and
 * joins and deletes the ST_RELEASE ones. The vtable pointer follows the fields.
 */
class thPosixThread {
public:
	enum cancel_state_t {
		CANCEL_ENABLE = 0,
		CANCEL_DISABLE = 1
	};
	enum cancel_type_t {
		CANCEL_DEFERRED = 0,
		CANCEL_ASYNCHRONOUS = 1
	};
	enum status_type_t {
		ST_IDLE = 0,
		ST_RUNNING = 1,
		ST_CALL_FINISHED = 2,
		ST_END = 3,
		ST_RELEASE = 4
	};

	static void setCancelState(cancel_state_t state, cancel_state_t *old);
	static void setCancelType(cancel_type_t type, cancel_type_t *old);

	thPosixThread();

	virtual int threadProc() = 0;      /* thread body */
	virtual ~thPosixThread() { delThreadList(); }
	virtual int threadFinished() = 0;  /* returns 1 when the work is done */
	virtual void abortProc() {}
	virtual void releaseProc() {}
	virtual int threadEvent() { return 1; }

	int start();
	static void *execute(void *arg);
	int finished();
	void event_handler();
	void abort();
	void release();
	void end_release();
	int waitEventProc();

	int IsIdle();
	int IsRunning();
	int IsCallFinished();
	int IsEnd();
	int IsRelease();

	/* Thread lists: addThreadList()/delThreadList() act on `this`; the idle-queue
	   functions take a queue index 0..5. */
	void addThreadList();
	void delThreadList();
	int addThreadIdleList(int a, int b);
	static void releaseAllThreadIdleList(int a);
	static thThreadQueue *getThreadIdleList(int a);
	static void setThreadIdleListPolicy(int a, status_type_t policy);
	static int getThreadIdleListPolicy(int a);
	static int checkThreadStart();
	static int checkThreadFinish();
	static void checkThread();
	static void AllDelete();

	/* Public: threads waiting for this one read m_tid directly. */
	/* 0x00 */ int m_started;      /* 1 from start() until execute() returns */
	/* 0x04 */ unsigned long m_tid;          /* pthread handle */
	/* 0x08 */ int m_abortFlag;
	/* 0x0C */ int m_releaseFlag;
	/* 0x10 */ status_type_t m_status;
	/* 0x14 */ status_type_t m_policy;       /* start priority 0 (highest) .. 4 (lowest) */
	/* 0x18 */ int m_pendingEvent; /* an event waits for event_handler() */

private:
	static list<thPosixThread *> thread_add_list;
	static list<thPosixThread *> thread_list;
	static thThreadQueue thread_idle_queue[6];
	static int thread_idle_queue_policy[6];
	static pthread_mutex_t thread_list_mutex;
	static pthread_mutex_t thread_idle_list_mutex;
	static pthread_mutex_t thread_add_list_mutex;
};  /* size = 0x20 */

inline thPosixThread::thPosixThread()
{
	m_tid = 0;
	m_started = 0;
	m_status = ST_IDLE;
	m_abortFlag = 0;
	m_releaseFlag = 0;
	m_pendingEvent = 0;
	m_policy = ST_CALL_FINISHED; /* policy 2: priority one step below normal */
	addThreadList();
}

#endif
