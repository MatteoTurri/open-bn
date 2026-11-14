/*
 * thJob: thread running a job given as callbacks (run in the thread, poll for
 * completion, done on destruction), with free argument slots for the caller.
 */

#ifndef THJOB_H
#define THJOB_H

#include "thposixthread.h"

#include <string>

class thJob : public thPosixThread {
public:
	thJob(void *arg, void (*run)(thJob *), bool (*poll)(thJob *, void *), void (*done)(thJob *));

	virtual int threadProc();
	virtual ~thJob();
	virtual int threadFinished();
	virtual void abortProc();
	virtual void releaseProc();

	/* 0x20 */ void *m_arg;  /* argument of m_poll */
	/* 0x24 */ void (*m_run)(thJob *);  /* job body, run by the thread */
	/* 0x28 */ bool (*m_poll)(thJob *, void *);  /* completion test */
	/* 0x2C */ void (*m_done)(thJob *);  /* called by the destructor */
	/* 0x30 */ string m_name;
	/* 0x34 */ string m_str[4];  /* job arguments and results, meaning set by the caller */
	/* 0x44 */ int m_int[4];
	/* 0x54 */ void *m_ptr[4];
};  /* size = 0x64 */

#endif
