/*
 * NaviMenu: the navigation menu window (load, open, enable and lock).
 */

#include "script/scenewindow.h"

#include "app/systemparam.h"
#include "navimenu.h"
#include <string>
#include <list>
#include <pthread.h>
#include "app/buffer.h"
#include "app/strutil.h"
#include "xml/xml_filedescriptor.h"
#include "thread/thposixthread.h"
#include "ime/textim.h"
#include "script/thxmlloader.h"

/* Topmost modal scene window, or 0 when there is none. */
static inline SceneWindow *ModalWindow()
{
	int n = SceneWindow::modal_window_list.end() - SceneWindow::modal_window_list.begin();
	if (n != 0)
		return *(SceneWindow **)((char *)SceneWindow::modal_window_list.begin()
		    + ((n - 1) << 2));  /* redundant as written */
	else
		return 0;
}

/* Window description loaded from navimenu.xml; 0 until Load() has run. */
static SceneWindow::CreateArg *g_window;

/* Loader callback: keeps a heap copy of the window description. */
static void navimenuLoaded(void *arg, int result, SceneWindow::CreateArg *ca)
{
	if (ca != 0)
		g_window = new SceneWindow::CreateArg(*ca);
}

namespace NaviMenu {

/* g_loaded: 1 once Load() has run; g_lockCount: Lock() nesting depth. */
static int g_enabled;
static int g_loaded;
static int g_lockCount;

void Init()
{
	g_loaded = 0;
	g_lockCount = 0;
	g_enabled = 0;
}

void Load()
{
	thXmlLoader *ldr = new thXmlLoader("file:/opt0/bn/script/system/navimenu.xml", "", "", SceneWindow::MODE_MODAL,
	    THXML_NO_MODAL_COUNT, 0, navimenuLoaded, Buffer());

	ldr->start();
	pthread_join(ldr->m_tid, 0);
	ldr->release();
	g_loaded = 1;
}

void Exit()
{
	delete g_window;
	g_window = 0;
}

/* Opens the menu window unless something blocks it (not loaded, disabled,
   locked, a modal window or the text IM open), then broadcasts MSG_NAVIMENU_OPENED. */
int Open()
{
	if (!g_loaded)
		return 0;
	if (!g_window)
		return 0;
	if (!g_enabled)
		return 0;
	if (!SystemParam::enable_navimenu)
		return 0;
	if (g_lockCount > 0)
		return 0;
	if (ModalWindow())
		return 0;
	if (thXmlLoader::for_modal_window_count)
		return 0;
	if (TextIM::IsOpened())
		return 0;

	CreateSceneWindow(*g_window);
	SceneWindow::BroadcastMessage(SceneWindow::MSG_NAVIMENU_OPENED, 0, 0, 0, 0);
	return 1;
}

void Enable(bool enable)
{
	g_enabled = enable;
}

void Lock(bool lock)
{
	if (lock)
		g_lockCount++;
	else
		g_lockCount--;
}

}
