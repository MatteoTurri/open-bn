/*
 * MsgBox: the script-driven message box (load, open, poll for the answer).
 */

#include <string>
#include <list>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "script/scenewindow_createarg.h"

#include "app/buffer.h"

#include "xml/xml_filedescriptor.h"
#include "thread/thposixthread.h"
#include "app/variables.h"

#include "script/thxmlloader.h"
#include "msgbox.h"

/* Window description loaded from msgbox.xml; 0 until Load() has run. */
static SceneWindow::CreateArg *g_window;

/* Loader callback: keeps a heap copy of the window description. */
static void msgboxLoaded(void *arg, int result, SceneWindow::CreateArg *ca)
{
	if (ca != 0)
		g_window = new SceneWindow::CreateArg(*ca);
}

namespace MsgBox {

/* g_state: 1 once Load() has run. g_activeId: -1 when idle, 0 while a box is
   open, then the answer read back from the "msgbox-return" variable. */
static int g_state;
static int g_activeId;

void Init()
{
	g_state = 0;
	g_activeId = -1;
}

void Load()
{
	thXmlLoader *ldr = new thXmlLoader("file:/opt0/bn/script/system/msgbox.xml", "", "", SceneWindow::MODE_MODAL,
	    THXML_NO_MODAL_COUNT, 0, msgboxLoaded, Buffer());

	ldr->start();
	pthread_join(ldr->m_tid, 0);
	ldr->release();
	g_state = 1;
}

void Exit()
{
	delete g_window;
	g_window = 0;
}

/* Window callback: stores the first value of the script variable
   "msgbox-return" in g_activeId, or -1 when it is empty. */
static void msgboxCallback()
{
	list<string> values;
	Variables::GetVariable("msgbox-return", values);
	if (values.size() != 0)
		g_activeId = atoi(values.front().c_str());
	else
		g_activeId = -1;
}

/* Publishes the msgbox-* script variables and opens the window loaded by
   Load(). Returns 0 when nothing is loaded or a box is already open. */
int Open(int type, const char *msg, const char *yes, const char *no, int flag)
{
	char buf[32];

	if (!g_state)
		return 0;
	if (!g_window)
		return 0;
	if (!g_activeId)
		return 0;

	sprintf(buf, "%d", type);
	Variables::SetVariable("msgbox-type", buf);
	Variables::SetVariable("msgbox-msg", msg);
	Variables::SetVariable("msgbox-yes", yes);
	Variables::SetVariable("msgbox-no", no);
	sprintf(buf, "%d", flag);
	Variables::SetVariable("msgbox-flag", buf);
	Variables::SetVariable("msgbox-return", "-1");

	SceneWindow *w = CreateSceneWindow(*g_window);
	if (w)
		w->SetCallback(SceneWindow::CALLBACK_ON_DESTROY, 0x43424D, msgboxCallback);

	g_activeId = 0;
	return 1;
}

int Sync()
{
	int r = g_activeId;
	if (r) {
		g_activeId = -1;
		return r;
	}
	return 0;
}

}

inline SceneWindow::CreateArg::CreateArg(const CreateArg &o)
        : mode(o.mode), psml_version(o.psml_version), screen_w(o.screen_w), screen_h(o.screen_h), screen_d(o.screen_d),
          fade_time(o.fade_time), uri(o.uri), dir(o.dir), name(o.name), script(o.script), key(o.key), doc(o.doc),
          file_image_cache(o.file_image_cache), file_uri_cache(o.file_uri_cache)
{}
