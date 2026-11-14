/*
 * Page history: up to 32 visited pages and the index of the current one.
 * HisExtension_Jump loads a page of the history again; the Small/amx natives
 * "his*" expose it to scripts.
 */

#include <string>
#include <vector>

#include "amx.h"
#include "natives/smallext.h"
#include "app/buffer.h"
#include "xml/xml_filedescriptor.h"
#include "thread/thposixthread.h"

#include "script/scenewindow.h"

#include "script/script.h"

#include "script/thxmlloader.h"
#include "hisextension.h"
#include "script/scriptabort.h"

/* One page of the history: the loader arguments that load it again. */
struct History {
	/* 0x00 */ string url;
	/* 0x04 */ string name;
	/* 0x08 */ string script;
	/* 0x0C */ Buffer buf;
	/* 0x10 */ bool postForm; /* send buf as a form (loader flag 8) */
	/* 0x14 */ bool postEscaped; /* send buf URL-escaped (loader flag 0x10) */

	History(const string &a, const string &b, const string &c, Buffer d, bool e, bool f)
	        : url(a), name(b), script(c), buf(d), postForm(e), postEscaped(f)
	{}
	~History() {}
};

static vector<History> s_hisList;
static int s_hisIndex;

void HisExtension_Init()
{
	s_hisList.erase(s_hisList.begin(), s_hisList.end());
	s_hisIndex = -1;
}

void HisExtension_Exit()
{
	s_hisList.erase(s_hisList.begin(), s_hisList.end());
	s_hisIndex = -1;
}

/* Walks the history and does nothing with it. */
static void HisNotifyChanged()
{
	unsigned int i;

	for (i = 0; i < s_hisList.size(); i++)  /* redundant as written */
		;
}

void HisExtension_Add(const string &a, const string &b, const string &c, Buffer buf, bool f1, bool f2)
{
	if (s_hisIndex >= 0) {
		while (s_hisIndex < (int)s_hisList.size() - 1)
			s_hisList.pop_back();
	}

	s_hisList.push_back(History(a, b, c, buf, f1, f2));

	while (s_hisList.size() > 32)
		s_hisList.erase(s_hisList.begin());

	s_hisIndex = s_hisList.size() - 1;
	HisNotifyChanged();
}

/* Completion of a history load: opens the page window and raises an error
   if the HTTP status is not 200. */
static void HisLoadDone(void *userData, int result, SceneWindow::CreateArg *arg)
{
	(void)userData;
	if (arg)
		CreateSceneWindow(*arg);
	if (result != 200)
		Error::Raise(2, 2, result);
}

/* Moves the current index by d and, if it stays inside the history, loads that
   page again. Returns 1 if the load was started, 0 otherwise. */
int HisExtension_Jump(int d)
{
	int n = s_hisIndex + d;

	if (n >= 0 && n < (int)s_hisList.size()) {
		SceneWindow::StartFadeoutMainWindow(500);

		s_hisIndex = s_hisIndex + d;

		thXmlLoader *t = new thXmlLoader(s_hisList[s_hisIndex].url, s_hisList[s_hisIndex].name,
		    s_hisList[s_hisIndex].script, 0,
		    (s_hisList[s_hisIndex].postForm ? THXML_POST_FORM | THXML_NO_HISTORY : THXML_NO_HISTORY)
		        | (s_hisList[s_hisIndex].postEscaped ? THXML_POST_ESCAPED : 0),
		    0, HisLoadDone, s_hisList[s_hisIndex].buf);
		t->addThreadIdleList(2, 1);
		t->release();
		return 1;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_hisClear(AMX *amx, cell *params)
{
	s_hisList.erase(s_hisList.begin(), s_hisList.end());
	s_hisIndex = -1;
	HisNotifyChanged();
	return 0;
}

/* Jumping from the main window ends the running script (AMX_ERR_EXIT). */
static cell AMX_NATIVE_CALL n_hisJump(AMX *amx, cell *params)
{
	if (GetSceneWindow(amx) == SceneWindow::main_window)
		amx_RaiseError(amx, AMX_ERR_EXIT);
	return HisExtension_Jump(params[1]);
}

static cell AMX_NATIVE_CALL n_hisCheck(AMX *amx, cell *params)
{
	int n = s_hisIndex + params[1];
	int r = 0;

	if (n >= 0)
		r = n < (int)s_hisList.size();
	return r;
}

AMX_NATIVE_INFO smallext_his[] = {
	{ "hisClear", n_hisClear },
	{ "hisJump", n_hisJump },
	{ "hisCheck", n_hisCheck },
	{ 0, 0 },
};
