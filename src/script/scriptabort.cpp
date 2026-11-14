/*
 * Error handling: Error::Raise records the most important pending error and
 * Error::Run shows the PSML panel of that error; plus the script loaders used.
 * Script__Abort* are in script_abort_entry.cpp.
 */

#include <string>
#include "script/scriptabort.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "app/buffer.h"
#include "xml/xml_filedescriptor.h"
#include "thread/thposixthread.h"
#include "scenewindow.h"
#include "ime/textim.h"
#include "thxmlloader.h"
#include "app/systemparam.h"
#include "natives/hisextension.h"

/* Pending error: code, two arguments, priority and state (0 none, 1 to show,
   2 shown, 3 closed by the user). */
static int g_errCode;
static int g_errArg1;
static int g_errArg2;
static int g_errPrio;
static int g_errFlag;

/* Priority of each error code. */
static const int g_errPrioTable[5] = { 9, 5, 5, 5, 0 };

void Error::Init()
{
	g_errFlag = 0;
	g_errCode = 0;
	g_errArg1 = 0;
}

void Error::Exit()
{}

typedef void (*LoadedCallback)(void *, int, SceneWindow::CreateArg *);

/* Starts a thread that loads the PSML script url with parameters param; cb gets the window to create. */
static void scriptLoad(const string &url, const string &param, LoadedCallback cb)
{
	thXmlLoader *ldr = new thXmlLoader(url, "", param, SceneWindow::MODE_MAIN, 0, 0, cb, Buffer());

	ldr->addThreadIdleList(2, 1);
	ldr->release();
}

/* Like scriptLoad, as a modal window; if g_errArg1 is set, first fades out the
   background and main windows and drops the modal ones. */
static void scriptLoadFade(const string &url, const string &param, LoadedCallback cb)
{
	if (g_errArg1) {
		SceneWindow::StartFadeoutBGWindow(500);
		SceneWindow::StartFadeoutMainWindow(500);
		SceneWindow::RemoveModal();
	}
	thXmlLoader *ldr = new thXmlLoader(url, "", param, SceneWindow::MODE_MODAL, 0, 0, cb, Buffer());

	ldr->addThreadIdleList(2, 1);
	ldr->release();
}

/* Close callback of the error window. */
static void scriptErrorCallback()
{
	g_errFlag = 3;
}

/* Creates the loaded error window and registers its close callback. */
static void scriptOpenErrorWindow(void *, int, SceneWindow::CreateArg *arg)
{
	if (arg == 0)
		return;
	SceneWindow *w = CreateSceneWindow(*arg);
	if (w == 0)
		return;
	w->SetCallback(SceneWindow::CALLBACK_ON_DESTROY, 0x435245, scriptErrorCallback);
}

#include "fs/thmediachecker.h"

static inline string err_param_str(const char *s)
{
	return s;
}
static inline void err_push(string url, const char *param)
{
	scriptLoadFade(url, err_param_str(param), scriptOpenErrorWindow);
}

static inline void err_jump(string url, const char *param)
{
	scriptLoad(url, string(param), 0);
}

/* Shows the pending error: the fatal panel for code 0, the error panel with
   type and errno for codes 1-3. Once the user has closed it (state 3), goes to
   the top menu or to the system update as g_errArg1 says. Raises error 0 if an
   illegal disc is detected. */
void Error::Run()
{
	if (SystemParam::alert_illegal_disc) {
		if (thMediaChecker::getCDVDType() == 255)
			Error::Raise(0, 0, 0);
	}
	switch (g_errFlag) {
	case 0:
		break;
	case 1:
		switch (g_errCode) {
		case 0:
			TextIM::Close();
			err_push("file:/opt0/bn/script/system/fatal.xml", "");
			g_errFlag = 2;
			return;
		case 1 ... 3:
			break;
		default:
			return;
		}
		char buf[256];
		sprintf(buf, "type=%d;errno=%d", g_errCode, g_errArg2);
		err_push("/opt0/bn/script/system/error.xml", buf);
		g_errFlag = 2;
		break;
	case 2:
		break;
	case 3:
		switch (g_errArg1) {
		case 0:
			g_errFlag = 0;
			break;
		case 1:
			if (HisExtension_Jump(-1) == 0)
				err_jump("file:/opt0/bn/script/top.xml", "menu_id=GROUP-TOPMENU");
			g_errFlag = 0;
			break;
		case 2:
			err_jump("file:/opt0/bn/script/top.xml", "menu_id=GROUP-TOPMENU");
			g_errFlag = 0;
			break;
		case 3:
			err_jump("file:/opt0/bn/script/utility/sysupdate.xml", "");
			g_errFlag = 0;
			break;
		}
		break;
	}
}

/* Records an error unless one of equal or higher priority is pending. */
void Error::Raise(int code, int arg1, int arg2)
{
	if (!g_errFlag || g_errPrioTable[code] > g_errPrio) {
		g_errFlag = 1;
		g_errCode = code;
		g_errArg1 = arg1;
		g_errArg2 = arg2;
		g_errPrio = g_errPrioTable[code];
	}
}
