/*
 * sceBNSysup*: the C interface of the DNAS2 system update, on a single
 * DNAS2SysupManager instance.
 */

#include "scebnsysup.h"

#include "dnas2sysupmanager.h"

// Init creates the manager, mounts the contents partition and loads the
// update definition; Start and GetStatus do not check that Init succeeded.
static DNAS2SysupManager *g_theManager = 0;

extern "C" int sceBNSysupInit(int, int)
{
	if (g_theManager)
		return -1;

	g_theManager = new DNAS2SysupManager;

	if (g_theManager->MountContentsPart() < 0)
		return -2;

	if (g_theManager->LoadSysupDef() < 0)
		return -3;
	return 0;
}

extern "C" int sceBNSysupStart(int, int)
{
	if (g_theManager->StartSysupThread() < 0)
		return -1;
	return 0;
}

extern "C" int sceBNSysupExit(int, int)
{
	if (!g_theManager)
		return -3;

	if (g_theManager->UnMountContentsPart() < 0)
		return -2;

	delete g_theManager;
	g_theManager = 0;
	return 0;
}

extern "C" int sceBNSysupGetStatus(sceBNSysupStatus *out)
{
	return g_theManager->GetStatus(out);
}
