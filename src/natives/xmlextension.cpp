/*
 * XML module of the PSBBN application: the "replace words" (`$$name$$`
 * macros substituted in document URLs) and the Small/amx natives "xml*"
 * that load XML documents in background thXmlLoader threads.
 */

#include <iostream.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "xml/xml.h"
#include "xml/xml_element.h"
#include "xml/xml_document.h"
#include "xml/xmltagtable.h"
#include "xml/xml_filedescriptor.h"
#include "xml/sax.h"
#include "xml/xml_parser.h"
#include "xml/xml_scannerstate.h"
#include "xml/xml_inputstream.h"
#include "thread/thposixthread.h"
#include "app/systemoptions.h"

#include "app/stringutil.h"

#include "app/bnsysconf.h"

#include "xmlextension.h"

#include "amx.h"
#include "natives/smallext.h"

#include "script/script.h"

#include "script/scenewindow.h"

#include "script/scriptabort.h"
#include "app/buffer.h"
#include "script/thxmlloader.h"

static const long TAG_PSCR = 0x52435350;        /* 'PSCR' */
static const unsigned int TAG_DLX = 0x444c585f; /* '_XLD' */

static inline StringUtil XmlMid(const string &s, int pos, int n)
{
	return s.substr(pos, n);
}

static inline void XmlTrimLeft(string &s, const char *ws = " \t\r\n")
{
	int pos = s.find_first_not_of(ws);
	if (pos < 0) {
		s = "";
	} else {
		StringUtil t = XmlMid(s, pos, s.length() - pos);
		s = t;
	}
}

static inline void XmlTrimRight(string &s, const char *ws = " \t\r\n")
{
	int pos = s.find_last_not_of(ws);
	if (pos < 0) {
		s = "";
	} else {
		StringUtil t = XmlMid(s, 0, pos + 1);
		s = t;
	}
}

static inline int XmlToInt(string s)
{
	return atoi(s.c_str());
}

static const int XML_ERR_ABORTED = 0xfffea070;

class XmlLoadData {
public:
	/* Range of HTTP status codes not raised as errors; c and d tell whether a and b are set. */
	struct TrapErrorRange {
		int a, b, c, d;
		TrapErrorRange() : a(0), b(0), c(0), d(0) {}
		~TrapErrorRange() {}
		void swap_if_need();

		void SetMin(int v)
		{
			c = 1;
			a = v;
			swap_if_need();
		}
		void SetMax(int v)
		{
			d = 1;
			a = v;
			swap_if_need();
		}  /* NOTE: bug: stores the upper bound in `a`, not `b` */
	};

	static int instance_count;

	/* 0x00 */ thXmlLoader *thread;
	/* 0x04 */ int state;  /* 1 = done, 2 = failed */
	/* 0x08 */ int error;  /* HTTP status or error code */
	/* 0x0C */ int mode;  /* window mode (SceneWindow::MODE_*), from xmlLoadStart argument 4 */
	/* 0x10 */ Buffer buf;
	/* 0x14 */ int postForm;  /* send buf as a form (loader flag 8) */
	/* 0x18 */ int postEscaped;  /* send buf URL-escaped as a form, version 100 (loader flag 16) */
	/* 0x1C */ int timeout;  /* option 100 (default 500), clamped to [0, 1000] */
	/* 0x20 */ bool opt200;  /* option 200: load into the node list instead of a window (4 bytes) */
	/* 0x24 */ int flags;  /* bit 0 = option 10100: window hidden */
	/* 0x28 */ int info;  /* node list index, returned by xmlLoadGetInfo(100) */
	/* 0x2C */ vector<TrapErrorRange> traps;

	XmlLoadData()
	        : thread(0), state(0), error(200), mode(0), postForm(0), postEscaped(0), timeout(500), opt200(0),
	          flags(0), info(0)
	{
		++instance_count;
	}

	~XmlLoadData()
	{
		--instance_count;
		if (thread) {
			thread->abort();
			thread->release();
			thread = 0;
		}
	}

	/* Stops the load in progress and marks it as failed. */
	void Abort()
	{
		cout << "<<< xmlload aborted : " << thread->m_uri << " >>>" << endl;
		state = 2;
		error = XML_ERR_ABORTED;
		if (thread->m_tid != 0)
			pthread_kill(thread->m_tid, 16);
		if (thread != 0) {  /* redundant as written */
			thread->abort();
			thread->release();
			thread = 0;
		}
	}
};

int XmlLoadData::instance_count = 0;

/* Orders [a, b] once both bounds are set (c = SetMin, d = SetMax). */
inline void XmlLoadData::TrapErrorRange::swap_if_need()
{
	if (c == 0)
		return;
	if (d == 0)
		return;
	if (a > b) {
		int t = a;

		a = b;
		b = t;
	}
}

struct ReplaceWord {
	/* 0x00 */ string from;
	/* 0x04 */ string to;

	ReplaceWord(const string &a, const string &b) : from(a), to(b) {}
	ReplaceWord(const ReplaceWord &o) : from(o.from), to(o.to) {}
};

static vector<ReplaceWord> s_replaceWords;
static pthread_mutex_t s_xmlMutex;

/* The script that runs amx, user data 'PSCR' of the AMX. */
static inline Script *AmxScript(AMX *amx)
{
	Script *s;

	amx_GetUserData(amx, TAG_PSCR, (void **)&s);
	return s;
}

/* The XML load state of a script, user data '_XLD'. */
static inline XmlLoadData *ScriptXmlLoadData(Script *owner)
{
	return (XmlLoadData *)owner->GetUserData(TAG_DLX);
}

/* Script callback: deletes the XML load state. */
static void XmlLoadDelete(AMX *amx)
{
	XmlLoadData *d;

	d = ScriptXmlLoadData(AmxScript(amx));
	if (d != 0)
		delete d;
}

static inline XmlLoadData *AmxXmlLoadData(AMX *amx)
{
	return ScriptXmlLoadData(AmxScript(amx));
}

/*
 * Completion callback of thXmlLoader; `userdata` is the AMX and `status` the
 * HTTP status. Does nothing if the script is being destroyed (state 3).
 */
static void XmlLoadDone(void *userdata, int status, SceneWindow::CreateArg *arg)
{
	AMX *amx = (AMX *)userdata;
	XmlLoadData *d;

	if (AmxScript(amx)->state == 3)
		return;
	d = AmxXmlLoadData(amx);
	if (d->opt200 != 0) {
		if (arg != 0) {
			XmlTagTable *list = GetNodeList(amx);
			Document doc = *GetDocument(amx);
			Node root = arg->doc.get_documentElement();

			Node n = doc.importNode(root, true);

			d->info = TagList_Add(*list, n);
		}
	} else {
		if (arg != 0)
			CreateSceneWindow(*arg)->hide_flag = d->flags;
	}
	d->state = status == 200 ? 1 : 2;
	d->error = status;
	if (d->state == 2) {
		int found = 0;
		XmlLoadData::TrapErrorRange *p;

		for (p = d->traps.begin(); p != d->traps.end(); p++) {
			if (p->c != 0 && p->a > d->error)
				continue;
			if (p->d != 0 && d->error > p->b)
				continue;
			found = 1;
		}
		if (!found) {
			if (d->error == (int)0xfffea06c)
				Error::Raise(1, 2, (int)0xfffea06c);
			else if (d->mode == SceneWindow::MODE_MAIN && d->opt200 == 0)
				Error::Raise(1, 1, d->error);
			else
				Error::Raise(1, 2, d->error);
		}
	}
	if (d->thread != 0) {
		d->thread->abort();
		d->thread->release();
		d->thread = 0;
	}
}

/*
 * xmlLoadStart(url, post, arg, mode): applies the replace words and the
 * window path to `url`, recycles or creates the load state and starts a
 * thXmlLoader with XmlLoadDone as completion callback.
 */
static cell AMX_NATIVE_CALL n_xmlLoadStart(AMX *amx, cell *params)
{
	SceneWindow *w = GetSceneWindow(amx);
	string url = CellToString(amx, params[1]);
	string post = CellToString(amx, params[2]);
	string arg = CellToString(amx, params[3]);
	XmlLoadData *d;
	thXmlLoader *ld;

	XmlExtension_Replace(url);
	w->MakeFullPath(url);
	d = ScriptXmlLoadData(AmxScript(amx));
	if (d != 0) {
		if (d->thread != 0) {
			d->thread->abort();
			d->thread->release();
			d->thread = 0;
		}
	} else {
		d = new XmlLoadData;
		AmxScript(amx)->SetUserData(TAG_DLX, d);
		AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_DLX, XmlLoadDelete);
	}
	d->mode = SceneWindow::MODE_MAIN;
	switch (params[4]) { /* 0 main, 1 background, 2 modal */
	case 0:
		d->mode = SceneWindow::MODE_MAIN;
		break;
	case 2:
		if (!w->IsFromNetwork())
			d->mode = SceneWindow::MODE_MODAL;
		break;
	case 1:
		d->mode = SceneWindow::MODE_BG;
		break;
	}
	ld = new thXmlLoader(url, post, arg, d->mode,
	    d->opt200 ? THXML_DOCUMENT_ONLY
	              : (d->postForm != 0 ? THXML_POST_FORM : (d->postEscaped != 0 ? THXML_POST_ESCAPED : 0)),
	    amx, XmlLoadDone, d->buf);
	d->thread = ld;
	ld->m_clientPsmlVersion = w->psml_version;
	((thXmlLoader *)d->thread)->m_fadeoutTime = d->timeout;
	d->thread->addThreadIdleList(2, 1);
	return 0;
}

static const int XML_POST_MAX = 0x10000;

/*
 * xmlLoadSetPost(...): sets the POST body of the next load. In a version 100
 * window the body is the string params[1] and params[2] sets postEscaped; otherwise
 * params[1] selects the source: 0 = string, 1 = string and sets postForm,
 * 2 = params[3] bytes of AMX memory. Bodies over 64 KB raise AMX error 10.
 */
static cell AMX_NATIVE_CALL n_xmlLoadSetPost(AMX *amx, cell *params)
{
	SceneWindow *w = GetSceneWindow(amx);

	if (w->psml_version == 100) {
		XmlLoadData *d = AmxXmlLoadData(amx);

		if (d == 0) {
			d = new XmlLoadData;
			AmxScript(amx)->SetUserData(TAG_DLX, d);
			AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_DLX, XmlLoadDelete);
		}
		string s = CellToString(amx, params[1]);
		d->buf = Buffer(s);
		d->postEscaped = params[2] != 0;
		if (d->buf.size() > XML_POST_MAX) {
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}
		return 0;
	}

	XmlLoadData *d = AmxXmlLoadData(amx);

	if (d == 0) {
		d = new XmlLoadData;
		AmxScript(amx)->SetUserData(TAG_DLX, d);
		AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_DLX, XmlLoadDelete);
	}
	switch (params[1]) {
	case 0: {
		string s = CellToString(amx, params[2]);
		d->buf = Buffer(s);
		break;
	}
	case 1: {
		string s = CellToString(amx, params[2]);
		d->buf = Buffer(s);
		d->postForm = 1;
		break;
	}
	case 2: {
		cell *p;
		amx_GetAddr(amx, params[2], &p);
		d->buf = Buffer(p, params[3]);
		break;
	}
	default:
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	if (d->buf.size() > XML_POST_MAX) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	return 0;
}

/*
 * xmlLoadSetOptionI(option, value): 100 = timeout, clamped to [0, 1000];
 * 200 = load into the node list; 10100 = hide the window (local windows only).
 */

static cell AMX_NATIVE_CALL n_xmlLoadSetOptionI(AMX *amx, cell *params)
{
	XmlLoadData *d = AmxXmlLoadData(amx);

	if (d == 0) {
		d = new XmlLoadData;
		AmxScript(amx)->SetUserData(TAG_DLX, d);
		AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_DLX, XmlLoadDelete);
	}
	switch (params[1]) {
	case 100:
		d->timeout = params[2];
		if (d->timeout < 0)
			d->timeout = 0;
		if (d->timeout > 1000)
			d->timeout = 1000;
		return 0;
	case 200:
		d->opt200 = params[2] != 0;
		return 0;
	default:
		if (GetSceneWindow(amx)->IsFromNetwork())
			return 0;
		if (params[1] != 10100)
			return 0;
		if (params[2] != 0)
			d->flags |= 1;
		else
			d->flags &= ~1;
		return 0;
	}
}

/*
 * xmlLoadTrapError(list): replaces the HTTP status ranges that XmlLoadDone
 * must not raise as errors; `list` is comma-separated, each item `N` or `A:B`.
 */
static cell AMX_NATIVE_CALL n_xmlLoadTrapError(AMX *amx, cell *params)
{
	XmlLoadData *d = AmxXmlLoadData(amx);

	if (d == 0) {
		d = new XmlLoadData;
		AmxScript(amx)->SetUserData(TAG_DLX, d);
		AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, TAG_DLX, XmlLoadDelete);
	}
	StringUtil list = CellToString(amx, params[1]);
	d->traps.clear();
	vector<StringUtil> items = list.Separate(",");
	if (!items.empty())
		for (unsigned int i = 0; i < items.size(); i++) {
			vector<StringUtil> kv = items[i].Separate(":");
			if (kv.size() == 1) {
				XmlTrimLeft(kv[0]);
				XmlTrimRight(kv[0]);
				if (kv[0].length() != 0) {
					int v = XmlToInt(kv[0]);
					XmlLoadData::TrapErrorRange r;
					r.SetMin(v);
					r.SetMax(v);
					d->traps.push_back(r);
				}
			} else if (kv.size() == 2) {
				XmlTrimLeft(kv[0]);
				XmlTrimRight(kv[0]);
				XmlTrimLeft(kv[1]);
				XmlTrimRight(kv[1]);
				XmlLoadData::TrapErrorRange r;
				if (kv[0].length() != 0)
					r.SetMin(XmlToInt(kv[0]));
				if (kv[1].length() != 0)
					r.SetMax(XmlToInt(kv[1]));
				d->traps.push_back(r);
			}
		}
	return 0;
}

/* xmlLoadSync(): 0 while loading, 1 if done, -1 if failed or not started; clears the result. */
static cell AMX_NATIVE_CALL n_xmlLoadSync(AMX *amx, cell *params)
{
	XmlLoadData *d = AmxXmlLoadData(amx);

	if (d == 0)
		return -1;
	if (d->thread)
		return 0;
	switch (d->state) {
	case 0:
		return -1;
	case 1:
		d->state = 0;
		return 1;
	case 2:
		d->state = 0;
		return -1;
	}
	return -1;
}

/* xmlLoadGetError(): error code of the last load; AMX error 10 without a load state. */
static cell AMX_NATIVE_CALL n_xmlLoadGetError(AMX *amx, cell *params)
{
	XmlLoadData *d = AmxXmlLoadData(amx);

	if (d == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	return d->error;
}

/* xmlLoadGetInfo(what, &out): only what = 100 (node list index) exists; the string branch is dead. */
static cell AMX_NATIVE_CALL n_xmlLoadGetInfo(AMX *amx, cell *params)
{
	int ok = 0;
	string s;
	cell *dest;
	cell *ip;
	cell len, addr, end;
	int val = 0;
	int valid;
	XmlLoadData *d = AmxXmlLoadData(amx);

	if (d == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	switch (params[1]) {
	case 100:
		val = d->info;
		break;
	default:
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	if (ok) {  /* redundant as written */
		if (params[3] * 4 < s.length() + 1)
			s = string(s, 0, params[3] * 4 - 1);
		len = s.length();
		addr = params[2];
		end = len + addr;
		if (addr >= 0 && addr < amx->hea && end >= 0 && end < amx->hea)
			valid = 1;
		else
			valid = addr >= amx->stk && addr < amx->stp && end >= amx->stk && end < amx->stp;
		if (!valid) {
			amx_RaiseError(amx, AMX_ERR_MEMACCESS);
			return 0;
		}
		amx_GetAddr(amx, params[2], &dest);
		amx_SetString(dest, (char *)s.c_str(), 1);
	} else {
		if (amx_GetAddr(amx, params[2], &ip) == 0)
			*ip = val;
	}
	return 0;
}

/* True if [p, q] lies entirely in the AMX heap or entirely in the script stack. */
static inline int amx_addr_ok(AMX *amx, cell p, cell q)
{
	if (p >= 0 && p < amx->hea && q >= 0 && q < amx->hea)
		return 1;
	if (p >= amx->stk && p < amx->stp && q >= amx->stk && q < amx->stp)
		return 1;
	return 0;
}

/* xmlGetURI(dst, size): copies the URI of the current window, truncated to `size` cells. */
static cell AMX_NATIVE_CALL n_xmlGetURI(AMX *amx, cell *params)
{
	cell *cptr;
	string v = GetSceneWindow(amx)->uri;

	if ((unsigned int)(params[2] * 4) < v.length() + 1)
		v = v.substr(0, params[2] * 4 - 1);
	if (!amx_addr_ok(amx, params[1], v.length() + params[1])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	amx_SetString(cptr, (char *)v.c_str(), 1);
	return 0;
}

/* xmlMakeFullPath(path, size): applies the replace words and the window path to `path`, in place. */
static cell AMX_NATIVE_CALL n_xmlMakeFullPath(AMX *amx, cell *params)
{
	cell *cptr;
	string s = CellToString(amx, params[1]);
	SceneWindow *w = GetSceneWindow(amx);

	XmlExtension_Replace(s);
	w->MakeFullPath(s);
	if ((unsigned int)(params[2] * 4) < s.length() + 1)
		s = s.substr(0, params[2] * 4 - 1);
	if (!amx_addr_ok(amx, params[1], s.length() + params[1])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	amx_SetString(cptr, (char *)s.c_str(), 1);
	return 0;
}

/* TranslateAll callback of xmlLoadAbortAll: aborts the script's load in progress, if any. */
static void XmlLoadAbortOne(Script *s)
{
	XmlLoadData *d = ScriptXmlLoadData(s);

	if (d == 0 || d->thread == 0)
		return;
	d->Abort();
	Error::Raise(1, 2, XML_ERR_ABORTED);
}

static cell AMX_NATIVE_CALL n_xmlLoadAbortAll(AMX *amx, cell *params)
{
	Script::TranslateAll(XmlLoadAbortOne);
	return 0;
}

AMX_NATIVE_INFO smallext_xmlload[] = {
	{ "xmlLoadStart", n_xmlLoadStart },
	{ "xmlLoadSetPost", n_xmlLoadSetPost },
	{ "xmlLoadSync", n_xmlLoadSync },
	{ "xmlLoadGetError", n_xmlLoadGetError },
	{ "xmlLoadTrapError", n_xmlLoadTrapError },
	{ "xmlLoadSetOptionI", n_xmlLoadSetOptionI },
	{ "xmlLoadGetInfo", n_xmlLoadGetInfo },
	{ 0, 0 },
};
AMX_NATIVE_INFO smallext_xml[] = {
	{ "xmlGetURI", n_xmlGetURI },
	{ "xmlMakeFullPath", n_xmlMakeFullPath },
	{ 0, 0 },
};
AMX_NATIVE_INFO smallext_xml_p[] = {
	{ "xmlLoadAbortAll", n_xmlLoadAbortAll },
	{ 0, 0 },
};

/* bnEnumSysconf callback: appends the pair to the vector `arg`. */
static int XmlAddSysconfWord(const char *key, const char *value, void *arg)
{
	((vector<ReplaceWord> *)arg)->push_back(ReplaceWord(key, value));
	return 0;
}

/*
 * Rebuilds the replace word table (the SERVER_ID entries of the system
 * configuration, then the built-in pages) and publishes it under the lock.
 */
void XmlExtension_UpdateReplaceWord()
{
	vector<ReplaceWord> list;

	int r = bnEnumSysconf("SERVER_ID", 0, XmlAddSysconfWord, &list);
	if (r)
		printf("bnEnumSysconf returned %d\n", r);

	list.push_back(ReplaceWord("BUILDIN_TOPMENU", "file:/opt0/bn/script/top.xml"));
	list.push_back(ReplaceWord("BUILDIN_DOWNLOAD", "file:/opt0/bn/script/system/cdl.xml"));
	list.push_back(ReplaceWord("BUILDIN_DOWNLOAD2", "file:/opt0/bn/script/system/download.xml"));
	list.push_back(ReplaceWord("BUILDIN_PLAY", "file:/opt0/bn/script/system/play.xml"));
	list.push_back(ReplaceWord("FEEGA-MEMBER", "file:/opt0/bn/script/edclient/member_menu.xml"));
	list.push_back(ReplaceWord("FEEGA-SERVICE", "file:/opt0/bn/script/edclient/service_menu.xml"));
	list.push_back(ReplaceWord("FEEGA-AUTHOR", "file:/opt0/bn/script/edclient/author.xml"));
	list.push_back(ReplaceWord("FEEGA-SERVICE-BUY", "file:/opt0/bn/script/edclient/buy.xml"));
	list.push_back(ReplaceWord("FEEGA-SERVICE-CANCEL", "file:/opt0/bn/script/edclient/cancel.xml"));
	list.push_back(ReplaceWord("FEEGA-SERVICE-HISTORY", "file:/opt0/bn/script/edclient/history.xml"));
	list.push_back(ReplaceWord("FEEGA-SERVICE-SERVICE-LIST", "file:/opt0/bn/script/edclient/service_list.xml"));
	list.push_back(ReplaceWord("FEEGA-MEMBER-REGIST", "file:/opt0/bn/script/edclient/regist.xml"));
	list.push_back(ReplaceWord("FEEGA-MEMBER-CHANGE", "file:/opt0/bn/script/edclient/change.xml"));
	list.push_back(ReplaceWord("FEEGA-MEMBER-QUIT", "file:/opt0/bn/script/edclient/quit.xml"));
	list.push_back(ReplaceWord("FEEGA-MEMBER-AGREEMENT", "file:/opt0/bn/script/edclient/agreement.xml"));
	list.push_back(ReplaceWord("FEEGA-MEMBER-ID-SAVE", "file:/opt0/bn/script/edclient/id_save_menu.xml"));
	list.push_back(ReplaceWord("FEEGA-MEMBER-GUIDE", "file:/opt0/bn/script/feegaguide/scene/term_B_3.xml"));

	pthread_mutex_lock(&s_xmlMutex);
	s_replaceWords = list;
	pthread_mutex_unlock(&s_xmlMutex);
}

void XmlExtension_Init()
{
	pthread_mutex_init(&s_xmlMutex, 0);
}

void XmlExtension_Exit()
{}

/* Replaces every `$$from$$` in `str` with its replacement; returns at once if there is no "$$". */
void XmlExtension_Replace(string &str)
{
	if (str.find("$$") == (unsigned)-1)
		return;

	pthread_mutex_lock(&s_xmlMutex);

	for (vector<ReplaceWord>::iterator i = s_replaceWords.begin(); i != s_replaceWords.end(); ++i) {
		StringUtil s(str);
		s.Replace("$$" + i->from + "$$", i->to);
		str = s;
	}

	pthread_mutex_unlock(&s_xmlMutex);
}
