/*
 * thPosixThread: thread start/stop, the thread lists and the idle-queue
 * scheduler (checkThread).
 */

#include "thposixthread.h"
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t thPosixThread::thread_list_mutex;
pthread_mutex_t thPosixThread::thread_idle_list_mutex;
pthread_mutex_t thPosixThread::thread_add_list_mutex;
list<thPosixThread *> thPosixThread::thread_add_list = list<thPosixThread *>();
list<thPosixThread *> thPosixThread::thread_list = list<thPosixThread *>();
thThreadQueue thPosixThread::thread_idle_queue[6];
int thPosixThread::thread_idle_queue_policy[6] = { 2, 2, 2 }; /* see checkThread() */

/* Reversed comparison: makes thThreadQueue a min-heap on key. */
int operator<(const thThreadQueueItem &a, const thThreadQueueItem &b)
{
	return a.key > b.key;
}

void thPosixThread::setCancelState(cancel_state_t state, cancel_state_t *old)
{
	int oldraw;
	pthread_setcancelstate(state, &oldraw);
	if (old)
		*old = (cancel_state_t)(oldraw != 0);
}

void thPosixThread::setCancelType(cancel_type_t type, cancel_type_t *old)
{
	int oldraw;
	pthread_setcanceltype(type, &oldraw);
	if (old)
		*old = (cancel_type_t)(oldraw != 0);
}

int thPosixThread::start()
{
	m_pendingEvent = 0;
	m_status = ST_RUNNING;
	if (m_tid) {
		pthread_join(m_tid, 0);
		m_tid = 0;
	}
	if (m_abortFlag) {
		m_status = ST_CALL_FINISHED;
		return 0;
	}
	if (m_started)
		return -1;
	m_started = 1;
	int r = pthread_create(&m_tid, 0, execute, this);
	if (r != 0) {
		printf("ERROR : couldnt create thread. - %d\n", r);
		m_started = 0;
	}
	return r;
}

/* pthread entry point: enables deferred cancellation, sets the scheduling
   priority and nice value from m_policy, then runs threadProc() unless the
   thread was aborted. */
void *thPosixThread::execute(void *arg)
{
	thPosixThread *self = (thPosixThread *)arg;
	int result = 0;
	struct sched_param param;

	setCancelState(CANCEL_ENABLE, 0);
	setCancelType(CANCEL_DEFERRED, 0);
	param.sched_priority = 0;
	int pmax = sched_get_priority_max(SCHED_OTHER);
	int pmin = sched_get_priority_min(SCHED_OTHER);
	int prio = (pmax + pmin) / 2;
	int niceval;

	switch (self->m_policy) {
	case ST_IDLE:
		prio += 1;
		if (prio >= pmax)
			prio = pmax;
		niceval = -5;
		break;
	default:
	case ST_RUNNING:
		niceval = 0;
		break;
	case ST_CALL_FINISHED:
		prio -= 1;
		if (prio <= pmin)
			prio = pmin;
		niceval = 5;
		break;
	case ST_END:
		prio -= 2;
		if (prio <= pmin)
			prio = pmin;
		niceval = 10;
		break;
	case ST_RELEASE:
		prio -= 3;
		if (prio <= pmin)
			prio = pmin;
		niceval = 15;
		break;
	}
	param.sched_priority = prio;

	pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
	nice(niceval);

	self->m_status = ST_RUNNING;
	if (!self->m_abortFlag)
		result = self->threadProc();
	self->m_status = ST_CALL_FINISHED;
	self->m_started = 0;
	return (void *)result;
}

/* Returns the result of threadFinished(); on 1 the status moves to ST_END, or
   to ST_RELEASE when release() was called. An aborted thread skips the check
   and returns 1. */
int thPosixThread::finished()
{
	if (m_abortFlag) {
		if (m_releaseFlag == 1)
			m_status = ST_RELEASE;
		else
			m_status = ST_END;
		return 1;
	}

	int result = threadFinished();
	if (result == 0)
		goto out;
	if (result != 1)
		return result;

	if (IsCallFinished()) {
		if (m_releaseFlag == result)
			m_status = ST_RELEASE;
		else
			m_status = ST_END;
	} else {
		m_status = ST_END;
	}

out:
	return result;
}

void thPosixThread::event_handler()
{
	if (m_abortFlag) {
		m_pendingEvent = 0;
	} else {
		if (threadEvent())
			m_pendingEvent = 0;
	}
}

void thPosixThread::abort()
{
	abortProc();
	m_abortFlag = 1;
	if (IsEnd())
		m_status = ST_RELEASE;
}

void thPosixThread::release()
{
	releaseProc();
	m_releaseFlag = 1;
	if (IsEnd())
		m_status = ST_RELEASE;
}

void thPosixThread::end_release()
{
	releaseProc();
	m_releaseFlag = 1;
	if (IsEnd())
		m_status = ST_RELEASE;
	if (IsIdle())
		m_status = ST_RELEASE;
}

/* Waits until no event is pending; returns 0 if the thread is aborted. */
int thPosixThread::waitEventProc()
{
	if (m_abortFlag)
		return 0;
	if (m_pendingEvent) {
		do {
			if (m_abortFlag)
				return 0;
			sleep(0);
		} while (m_pendingEvent);
	}
	return 1;
}

int thPosixThread::IsIdle()
{
	return m_status == ST_IDLE;
}
int thPosixThread::IsRunning()
{
	return m_status == ST_RUNNING;
}
int thPosixThread::IsCallFinished()
{
	return m_status == ST_CALL_FINISHED;
}
int thPosixThread::IsEnd()
{
	return m_status == ST_END;
}
int thPosixThread::IsRelease()
{
	return m_status == ST_RELEASE;
}

void thPosixThread::addThreadList()
{
	pthread_mutex_lock(&thread_add_list_mutex);
	thread_add_list.push_back(this);
	pthread_mutex_unlock(&thread_add_list_mutex);
}

void thPosixThread::delThreadList()
{
	pthread_mutex_lock(&thread_list_mutex);
	thread_list.remove(this);
	pthread_mutex_unlock(&thread_list_mutex);
}

/* Queues `this` on idle queue a with key b (at least 1). */
int thPosixThread::addThreadIdleList(int a, int b)
{
	pthread_mutex_lock(&thread_idle_list_mutex);
	thread_idle_queue[a].push(thThreadQueueItem(this, (b > 0) ? b : 1));
	return pthread_mutex_unlock(&thread_idle_list_mutex);
}

/* Aborts and releases every thread on idle queue a, emptying it. */
void thPosixThread::releaseAllThreadIdleList(int a)
{
	pthread_mutex_lock(&thread_idle_list_mutex);
	if (!thread_idle_queue[a].empty()) {
		do {
			thThreadQueue &q = thread_idle_queue[a];
			q.top().thread->abort();
			q.top().thread->release();
			q.pop();
		} while (!thread_idle_queue[a].empty());
	}
	pthread_mutex_unlock(&thread_idle_list_mutex);
}

thThreadQueue *thPosixThread::getThreadIdleList(int a)
{
	return &thread_idle_queue[a];
}

void thPosixThread::setThreadIdleListPolicy(int a, status_type_t policy)
{
	pthread_mutex_lock(&thread_idle_list_mutex);
	thread_idle_queue_policy[a] = policy;
	pthread_mutex_unlock(&thread_idle_list_mutex);
}

int thPosixThread::getThreadIdleListPolicy(int a)
{
	return thread_idle_queue_policy[a];
}

/* checkThreadStart() takes the thread-list and idle-list mutexes without
   blocking (16 = EBUSY) and returns 1 holding both; checkThreadFinish()
   releases them. */
int thPosixThread::checkThreadStart()
{
	if (pthread_mutex_trylock(&thread_list_mutex) == 16)
		return 0;
	if (pthread_mutex_trylock(&thread_idle_list_mutex) == 16) {
		pthread_mutex_unlock(&thread_list_mutex);
		return 0;
	}
	return 1;
}

int thPosixThread::checkThreadFinish()
{
	pthread_mutex_unlock(&thread_list_mutex);
	return pthread_mutex_unlock(&thread_idle_list_mutex);
}

/*
 * One scheduler step, skipped while the lists are busy:
 *  1) moves the newly added threads to thread_list;
 *  2) on each idle queue, starts the top thread if it is ST_IDLE (re-queued
 *     with key 0), or pops it once it reaches the state the queue policy
 *     waits for;
 *  3) calls finished() on ST_CALL_FINISHED threads and event_handler() on
 *     running threads with a pending event; joins and deletes ST_RELEASE ones.
 */
void thPosixThread::checkThread()
{
	list<thPosixThread *>::iterator it, cur;

	if (!checkThreadStart())
		return;

	if (pthread_mutex_trylock(&thread_add_list_mutex) == 16) {
		checkThreadFinish();
		return;
	}

	for (it = thread_add_list.begin(); it != thread_add_list.end(); it++)
		thread_list.push_back(*it);
	thread_add_list.clear();
	pthread_mutex_unlock(&thread_add_list_mutex);

	for (int i = 0; i < 6; i++) {
		if (getThreadIdleList(i)->empty())
			continue;
		thThreadQueueItem item = getThreadIdleList(i)->top();
		if (item.thread->IsIdle()) {
			getThreadIdleList(i)->pop();
			item.key = 0;
			getThreadIdleList(i)->push(item);
			item.thread->start();
			if (item.thread->m_abortFlag)
				i--;
		} else {
			/* Policy 3 accepts ST_END or ST_RELEASE, policy 4 only ST_RELEASE, any
			   other value ST_CALL_FINISHED as well. */
			int policy = getThreadIdleListPolicy(i);
			if (policy == 3)
				goto chkEnd;
			if (policy < 4)
				goto chkFinished;
			if (policy == 4)
				goto chkRelease;
chkFinished:
			if (item.thread->IsCallFinished())
				goto dopop;
chkEnd:
			if (item.thread->IsEnd())
				goto dopop;
chkRelease:
			if (!item.thread->IsRelease())
				continue;
dopop:
			getThreadIdleList(i)->pop();
			i--;
		}
	}

	pthread_mutex_unlock(&thread_idle_list_mutex);

	{
		list<thPosixThread *> del;

		del.clear();
		for (it = thread_list.begin(); it != thread_list.end();) {
			cur = it++;
			if ((*cur)->IsCallFinished()) {
				(*cur)->finished();
			} else if ((*cur)->IsRunning()) {
				if ((*cur)->m_pendingEvent)
					(*cur)->event_handler();
			} else if ((*cur)->IsRelease()) {
				if ((*cur)->m_tid)
					pthread_join((*cur)->m_tid, 0);
				del.push_back(*cur);
			}
		}
		pthread_mutex_unlock(&thread_list_mutex);
		for (it = del.begin(); it != del.end(); it++)
			delete *it;
		del.clear();
	}
}

/* Aborts and releases every thread, then waits until checkThread() has
   deleted them all. */
void thPosixThread::AllDelete()
{
	checkThread();
	if (thread_list.size() == 0)
		return;
	list<thPosixThread *>::iterator it;
	for (it = thread_list.begin(); it != thread_list.end(); it++) {
		(*it)->m_abortFlag = 1;
		(*it)->end_release();
	}
	usleep(100000);
	while (thread_list.size() != 0) {
		checkThread();
		sleep(0);
	}
}
