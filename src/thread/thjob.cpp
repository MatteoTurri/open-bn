/*
 * thJob: thread running a job given as callbacks (run in the thread, poll for
 * completion, done on destruction).
 */

#include "thjob.h"

thJob::thJob(void *arg, void (*run)(thJob *), bool (*poll)(thJob *, void *), void (*done)(thJob *))
        : m_arg(arg), m_run(run), m_poll(poll), m_done(done)
{
	unsigned int i;

	for (i = 0; i < 4; i++)
		m_int[i] = 0;
	for (i = 0; i < 4; i++)
		m_ptr[i] = 0;
}

/* Calls the completion callback. */
thJob::~thJob()
{
	if (m_done)
		m_done(this);
}

int thJob::threadProc()
{
	if (m_run)
		m_run(this);
	return 0;
}

/* Finished when aborted, without a poll callback, or when m_poll returns nonzero. */
int thJob::threadFinished()
{
	if (m_abortFlag)
		return 1;
	if (!m_poll)
		return 1;
	return m_poll(this, m_arg) != 0;
}

void thJob::abortProc()
{}

void thJob::releaseProc()
{}
