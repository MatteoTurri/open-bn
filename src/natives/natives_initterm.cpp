/*
 * Small/amx natives "topInit*": runs the startup init (jobInit2) on a thJob
 * while the navigation menu stays active; topInitSync polls for its end.
 */

#include "amx.h"
#include "natives/smallext.h"
#include "script/scenewindow.h"
#include "script/script.h"
#include "app/strutil.h"
#include "app/command.h"
#include "app/systemoptions.h"
#include "thread/thposixthread.h"
#include "thread/thjob.h"

#include "ui/navimenu.h"

#include "app/jobinit2.h"

static int s_topInitDone;

/* Finish check of the init job (thJob::threadFinished): flags the init as done. */
static bool topInitPoll(thJob *job, void *arg)
{
	s_topInitDone = 1;
	return true;
}

static cell AMX_NATIVE_CALL n_topInitStart(AMX *amx, cell *params)
{
	thJob *job;

	NaviMenu::Enable(true);
	s_topInitDone = 0;
	job = new thJob(0, jobInit2, topInitPoll, 0);
	job->start();
	job->release();
	return 0;
}

static cell AMX_NATIVE_CALL n_topInitSync(AMX *amx, cell *params)
{
	return s_topInitDone;
}

AMX_NATIVE_INFO smallext_initterm_p[] = {
	{ "topInitStart", n_topInitStart },
	{ "topInitSync", n_topInitSync },
	{ 0, 0 },
};
