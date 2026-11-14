/*
 * Script: host of the Small (AMX) virtual machine of a PSML page. Native API
 * tables, program loading and module references, the per-frame run of every
 * script, message queues, user data, callbacks, and the AMX <-> object glue.
 */

#include "scenewindow.h"
#include "script.h"
#include "app/strutil.h"
#include "app/module.h"
#include "xml/xmltagtable.h"
#include <string.h>
#include <stdlib.h>
#include "app/systemoptions.h"
#include "app/command.h"
#include "xml/xml_filedescriptor.h"
#include "thread/thposixthread.h"
#include "scene/object.h"
#include "scene/objecttable.h"
#include "natives/natives_img.h"
#include "natives/timeextension.h"
#include "input/mypad.h"
#include "app/jobinit2.h"
#include "ui/navimenu.h"
#include "ime/textim.h"
#include "thxmlloader.h"
/* Init registers the first 23 tables of natives/smallext.h into the standard
   table, the last 14 into the local one. */
#include "natives/smallext.h"
#include "natives/hisextension.h"
#include "natives/natives_cdl.h"
#include "natives/xmlextension.h"
#include "natives/shortcutextension.h"
#include "app/systemparam.h"
#include "script/scriptabort.h"
#include "scene/animation.h"

inline Script::MessageStr::MessageStr(int k, const string &v) : key(k), value(v)
{}

int Script::instance_count = 0;
int Script::released_count = 0;
int Script::network_vm_sumsize = 0;
int Script::busy_count = 0;
int Script::https_busy_count = 0;

static const long TAG_AMX_OWNER = 0x4a424f48; /* "HOBJ": handle of the Object owning the AMX */
static const long TAG_SWND = 0x444e5753; /* "SWND": SceneWindow of a scene */

/* Every live Script, in creation order. */
static list<Script *> s_scriptList;

/* Defined in amx.c, not declared by this revision of amx.h. */
extern "C" {
int amx_NumNatives(__amx *amx, int *out);
}

/* Native API tables, by name: the standard one, and the one also offered to
   local (non-network) scripts. */
static map<string, AMX_NATIVE> s_publicNatives;
static map<string, AMX_NATIVE> s_privateNatives;

/* Also missing from this revision of amx.h. */
extern "C" {
int amx_NumLibraries(__amx *amx, int *number);
}

static const long TAG_SCRIPT_OWNER = 0x52435350; /* "PSCR": the Script owning the AMX */

/* Glue between the Small VM and the object tree. Tags are user data keys,
   shown as their bytes read in memory. */

static const long TAG_SMAL = 0x4c414d53; /* "SMAL" */
static const long TAG_LOCK = 0x4b434f4c; /* "LOCK" */
static const long TAG_HNDL = 0x4c444e48; /* "HNDL" */

/* Animations attached to an object as user data ("ANMP", "ANMR", "ANMS", "ANMC", "ANMT"). */
static const long TAG_PMAN = 0x504d4e41;
static const long TAG_RMAN = 0x524d4e41;
static const long TAG_SMAN = 0x534d4e41;
static const long TAG_CMAN = 0x434d4e41;
static const long TAG_TMAN = 0x544d4e41;

static inline Animation *HookAnimation(Hook *hook, long tag)
{
	return (Animation *)hook->GetUserData(tag);
}

/* Removes the animation from the hook and returns it. */
static inline Animation *TakeHookAnimation(Hook *hook, long tag)
{
	return (Animation *)hook->DelUserData(tag);
}

/* SceneWindow of a scene, its "SWND" user data. */
static inline SceneWindow *SceneWindowOf(Object *scene)
{
	return (SceneWindow *)scene->GetUserData(TAG_SWND);
}

/* Top modal window, or 0. */

static inline void *LastModalWindow()
{
	return SceneWindow::modal_window_list.size()
	    ? SceneWindow::modal_window_list[SceneWindow::modal_window_list.size() - 1]
	    : 0;
}

/* Adds the natives of info to table, up to count entries or the NULL name (count -1). */
static void RegistNativeTable(map<string, AMX_NATIVE> &table, const AMX_NATIVE_INFO *info, int count)
{
	int i;

	for (i = 0; info[i].name != 0 && (i < count || count == -1); i++)
		table[info[i].name] = info[i].func;
}

void Script::Init()
{
	HisExtension_Init();
	ImgExtension_Init();
	DnasExtension_Init();
	XmlExtension_Init();
	ShortcutExtension_Init();
	SystemParam::Init();
	s_publicNatives.clear();
	s_privateNatives.clear();
	RegistNativeTable(s_publicNatives, smallext_cons, -1);
	RegistNativeTable(s_publicNatives, smallext_float, -1);
	RegistNativeTable(s_publicNatives, smallext_long, -1);
	RegistNativeTable(s_publicNatives, smallext_time, -1);
	RegistNativeTable(s_publicNatives, smallext_3d, -1);
	RegistNativeTable(s_publicNatives, smallext_skb, -1);
	RegistNativeTable(s_publicNatives, smallext_msgbox, -1);
	RegistNativeTable(s_publicNatives, smallext_wnd, -1);
	RegistNativeTable(s_publicNatives, smallext_msg, -1);
	RegistNativeTable(s_publicNatives, smallext_sys, -1);
	RegistNativeTable(s_publicNatives, smallext_snd, -1);
	RegistNativeTable(s_publicNatives, smallext_list, -1);
	RegistNativeTable(s_publicNatives, smallext_img, -1);
	RegistNativeTable(s_publicNatives, smallext_model, -1);
	RegistNativeTable(s_publicNatives, smallext_math, -1);
	RegistNativeTable(s_publicNatives, smallext_xml, -1);
	RegistNativeTable(s_publicNatives, smallext_dom, -1);
	RegistNativeTable(s_publicNatives, smallext_his, -1);
	RegistNativeTable(s_publicNatives, smallext_dnas, -1);
	RegistNativeTable(s_publicNatives, smallext_cookie, -1);
	RegistNativeTable(s_publicNatives, smallext_string, -1);
	RegistNativeTable(s_publicNatives, smallext_xmlload, -1);
	RegistNativeTable(s_publicNatives, smallext_core, -1);
	RegistNativeTable(s_privateNatives, smallext_3d_p, -1);
	RegistNativeTable(s_privateNatives, smallext_dnas_p, -1);
	RegistNativeTable(s_privateNatives, smallext_sysup, -1);
	RegistNativeTable(s_privateNatives, smallext_boot, -1);
	RegistNativeTable(s_privateNatives, smallext_movie, -1);
	RegistNativeTable(s_privateNatives, smallext_dat, -1);
	RegistNativeTable(s_privateNatives, smallext_browser, -1);
	RegistNativeTable(s_privateNatives, smallext_file, -1);
	RegistNativeTable(s_privateNatives, smallext_syscnf, -1);
	RegistNativeTable(s_privateNatives, smallext_xml_p, -1);
	RegistNativeTable(s_privateNatives, smallext_sys_p, -1);
	RegistNativeTable(s_privateNatives, smallext_play, -1);
	RegistNativeTable(s_privateNatives, smallext_shortcut, -1);
	RegistNativeTable(s_privateNatives, smallext_initterm_p, -1);
}

void Script::Exit()
{
	HisExtension_Exit();
	ImgExtension_Exit();
	DnasExtension_Exit();
	XmlExtension_Exit();
	ShortcutExtension_Exit();
	s_publicNatives.clear();
	s_privateNatives.clear();
}

/* Loads the program image a from the window cache, registers modules and
   natives, finds the public function b and parses c as "key=value;..."
   arguments. On failure state is 4. */
Script::Script(Object *obj, const string &a, const string &b, const string &c)
        : file(a), uri(), program(0), stp_cache(0), state(0), flags(2)
{
	instance_count++;

	SceneWindow *win = SceneWindowOf(obj->scene);
	uri = win->GetFileURI(file);

	void *image = win->GetFileImage(file);
	if (image == 0) {
		state = 4;
		return;
	}
	if (win->IsFromNetwork())
		flags |= 2;
	else
		flags &= ~2;

	program = LoadProgram(image, win->psml_version);
	if (program == 0) {
		state = 4;
		return;
	}
	if (flags & 2)
		network_vm_sumsize += stp_cache;

	void *h;
	if (obj == 0)  /* redundant as written */
		h = 0;
	else
		h = obj->GetUserData(TAG_HNDL);
	amx_SetUserData(&amx, TAG_AMX_OWNER, h);

	if (!(flags & 2)) {
		AddRefModule();
		RegistModule();
	}

	if (RegisterAPI() != 0) {
		int n;
		amx_NumNatives(&amx, &n);
		for (int i = 0; i < n; i++)  /* redundant as written */
			;
		state = 4;
		return;
	}

	if (!b.empty()) {
		if (amx_FindPublic(&amx, b.c_str(), &entry_index) != 0) {
			state = 4;
			return;
		}
	} else {
		entry_index = -1;
	}

	{
		StringUtil su = c;
		args.clear();
		vector<StringUtil> items = su.Separate(";");
		if (!items.empty())  /* redundant as written */
			for (unsigned int i = 0; i < items.size(); i++) {
				vector<StringUtil> kv = items[i].Separate("=");
				if (kv.size() == 1) {
					StrTrimLeft(kv[0]);
					StrTrimRight(kv[0]);
					args[kv[0]] = "";
				} else if (kv.size() >= 2) {
					StrTrimLeft(kv[0]);
					StrTrimRight(kv[0]);
					StrTrimLeft(kv[1]);
					StrTrimRight(kv[1]);
					args[kv[0]] = kv[1];
				}
			}
	}

	s_scriptList.push_back(this);
}

Script::~Script()
{
	instance_count--;
	if (state == 3)
		released_count--;
	if (state != 4) {
		DoCallback(CALLBACK_ON_DESTROY);
		if (!(flags & 2))
			DecRefModule();
	}
	if (program != 0) {
		if (flags & 0x10)
			memset(program, 0, stp_cache);
		free(program);
		if (flags & 2)
			network_vm_sumsize -= stp_cache;
	}
	SetBusy(false);
	SetHttpsBusy(false);
}

int Script::GetWindowMode()
{
	void *cell;
	amx_GetUserData(&amx, TAG_AMX_OWNER, &cell);
	Object *owner = ObjectTable::Get((int)cell);
	SceneWindow *win = SceneWindowOf(owner->scene);
	return win->mode;
}

SceneWindow *Script::GetSceneWindow()
{
	void *cell;
	amx_GetUserData(&amx, TAG_AMX_OWNER, &cell);
	Object *owner = ObjectTable::Get((int)cell);
	return SceneWindowOf(owner->scene);
}

/* Checks an AMX header for content version 100 (file version 5, stack up to
   256 KB) or 110 (file version 6, up to 1 MB); other versions skip the file
   version check. */
int Script::IsValidAmx(const AMX_HEADER *hdr, int ver)
{
	bool mismatch = false;

	switch (ver) {
	case 100:
		mismatch = hdr->file_version ^ 5;
		break;
	case 110:
		mismatch = hdr->file_version ^ 6;
		break;
	}
	if (mismatch)
		return 0;
	if (hdr->stp > (ver == 100 ? 0x40000 : 0x100000))
		return 0;
	return 1;
}

/* Loads, by name, the library modules a program depends on. */
void Script::LoadModule(void *program)
{
	__amx localAmx;
	AMX_HEADER fileHdr;
	memcpy(&fileHdr, program, 52);  /* the header of file versions up to 6 (no name table) */
	void *buf = malloc(fileHdr.stp);
	if (buf != 0) {
		memcpy(buf, program, fileHdr.size);
		memset(&localAmx, 0, 120);
		if (amx_Init(&localAmx, buf) == 0) {
			int count;
			amx_NumLibraries(&localAmx, &count);
			for (int i = 0; i < count; i++) {
				AMX_HEADER *hdr = (AMX_HEADER *)localAmx.base;
				AMX_FUNCSTUB *lib =
				    (AMX_FUNCSTUB *)((char *)hdr + hdr->libraries + i * sizeof(AMX_FUNCSTUB));
				string name(lib->name);
				Module::Load(name);
			}
		}
	}
	if (buf != 0)
		free(buf);
}

/* Same as LoadModule, but releases the modules. */
void Script::ReleaseModule(void *program)
{
	__amx localAmx;
	AMX_HEADER fileHdr;
	memcpy(&fileHdr, program, 52);  /* the header of file versions up to 6 (no name table) */
	void *buf = malloc(fileHdr.stp);
	if (buf != 0) {
		memcpy(buf, program, fileHdr.size);
		memset(&localAmx, 0, 120);
		if (amx_Init(&localAmx, buf) == 0) {
			int count;
			amx_NumLibraries(&localAmx, &count);
			for (int i = 0; i < count; i++) {
				AMX_HEADER *hdr = (AMX_HEADER *)localAmx.base;
				AMX_FUNCSTUB *lib =
				    (AMX_FUNCSTUB *)((char *)hdr + hdr->libraries + i * sizeof(AMX_FUNCSTUB));
				string name(lib->name);
				Module::Release(name);
			}
		}
	}
	if (buf != 0)
		free(buf);
}

/* Take or drop a reference on each library module of the loaded program. */
void Script::AddRefModule()
{
	int count;
	amx_NumLibraries(&amx, &count);
	for (int i = 0; i < count; i++) {
		AMX_HEADER *hdr = (AMX_HEADER *)amx.base;
		AMX_FUNCSTUB *lib = (AMX_FUNCSTUB *)((char *)hdr + hdr->libraries + i * sizeof(AMX_FUNCSTUB));
		string name(lib->name);
		Module::Load(name);
	}
}

void Script::DecRefModule()
{
	int count;
	amx_NumLibraries(&amx, &count);
	for (int i = 0; i < count; i++) {
		AMX_HEADER *hdr = (AMX_HEADER *)amx.base;
		AMX_FUNCSTUB *lib = (AMX_FUNCSTUB *)((char *)hdr + hdr->libraries + i * sizeof(AMX_FUNCSTUB));
		string name(lib->name);
		Module::Release(name);
	}
}

/* Registers the natives of each library module in this AMX. */

void Script::RegistModule()
{
	int count;
	amx_NumLibraries(&amx, &count);
	for (int i = 0; i < count; i++) {
		AMX_HEADER *hdr = (AMX_HEADER *)amx.base;
		AMX_FUNCSTUB *lib = (AMX_FUNCSTUB *)((char *)hdr + hdr->libraries + i * sizeof(AMX_FUNCSTUB));
		string name(lib->name);
		Module::Regist(&amx, name);
	}
}

/* Copies the program image into a new buffer and initializes the AMX on it;
   returns the buffer, or 0. A network script fails if the stack sizes of all
   network scripts would exceed 2 MB (not checked for content version 100). */
void *Script::LoadProgram(void *program, int flag)
{
	AMX_HEADER fileHdr;
	memcpy(&fileHdr, program, 52);  /* the header of file versions up to 6 (no name table) */
	stp_cache = fileHdr.stp;
	if (flag != 100) {
		if (flags & 2) {
			if ((unsigned int)0x200000 < (unsigned int)(network_vm_sumsize + fileHdr.stp))
				return 0;
		}
	}
	void *buf = malloc(fileHdr.stp);
	void *result;
	if (buf != 0) {
		memcpy(buf, program, fileHdr.size);
		memset(&amx, 0, sizeof(amx));
		amx_SetUserData(&amx, TAG_SCRIPT_OWNER, this);
		if (amx_Init(&amx, buf) != 0) {
			free(buf);
			result = 0;
		} else {
			result = buf;
		}
	} else {
		result = 0;
	}
	return result;
}

/* Resolves the natives of the loaded program that are still unresolved by
   looking them up in table; returns 19 (AMX_ERR_NOTFOUND) if one is missing. */
static int amx_Register(__amx *amx, const map<string, AMX_NATIVE> *table)
{
	AMX_HEADER *hdr;
	AMX_FUNCSTUB *func;
	int number, i, err;

	hdr = (AMX_HEADER *)amx->base;
	func = (AMX_FUNCSTUB *)((char *)hdr + hdr->natives);
	amx_NumNatives(amx, &number);
	err = 0;
	for (i = 0; i < number; i++) {
		if (func->address == 0) {
			map<string, AMX_NATIVE>::const_iterator it = table->find(func->name);

			if (it != table->end())
				func->address = (unsigned int)(*it).second;
			else
				err = 19;
		}
		func++;
	}
	return err;
}

/* Registers the standard natives, then, for a local script, the alternative
   table; returns 19 if neither resolves all of them. */
int Script::RegisterAPI()
{
	if (amx_Register(&amx, &s_publicNatives) == 0)
		return 0;
	if (!(flags & 2) && amx_Register(&amx, &s_privateNatives) == 0)
		return 0;
	return 19;
}

/* Per frame: runs the image and time extensions, opens the navigation menu on
   a pad request, deletes the released scripts and runs the others. Scripts of
   window modes 0 and 2 wait while a modal window is up or loading, or while
   the text input method is open on a window that is not PSML 100; mode 3
   scripts run only for the top modal window. */
void Script::RunAll()
{
	ImgExtension_Run();
	TimeExtension_Run();
	if ((pPad0 != 0 && (pPad0->m_buttons[2].w & 1)) || (pPad1 != 0 && (pPad1->m_buttons[2].w & 1)))
		NaviMenu::Open();

	int textIMActive = TextIM::IsOpened();
	if (textIMActive != 0) {
		Object *clientObj = TextIM::GetClientObject();
		if (clientObj != 0) {
			SceneWindow *win = ::GetSceneWindow(clientObj);
			if (win->psml_version == 100)
				textIMActive = 0;
		}
	}

	list<Script *>::iterator it = s_scriptList.begin();
	if (it == s_scriptList.end())
		return;
	if (!running_status)
		return;
	do {
		if ((*it)->state == 3) {
			delete *it;
			it = s_scriptList.erase(it);
			goto next;
		}
		switch ((*it)->GetWindowMode()) {
		case 1:
			break;
		case 0:
		case 2:
			/* A modal window or the text input method has the focus. */
			if (textIMActive || LastModalWindow() != 0 || thXmlLoader::for_modal_window_count != 0) {
				it++;
				goto next;
			}
			break;
		case 3:
			/* Runs only if its window is the top modal one. */
			if (textIMActive || (*it)->GetSceneWindow() != LastModalWindow()) {
				it++;
				goto next;
			}
			break;
		}
		(*it)->Run();
		it++;
next:
		if (it == s_scriptList.end())
			break;
	} while (running_status);
}

/* Deletes the released scripts. */
void Script::RemoveAll()
{
	for (list<Script *>::iterator it = s_scriptList.begin(); it != s_scriptList.end();) {
		Script *item = *it;
		if (item->state == 3) {
			if (item != 0)
				delete item;
			it = s_scriptList.erase(it);
		} else {
			it++;
		}
	}
}

void Script::Run()
{
	if (state == 2)
		return;
	DoCallback(CALLBACK_EVERY_FRAME);
	if (flags & 1)
		return;
	cell retval;
	int ret;
	if (state == 0) {
		state = 1;
		ret = amx_Exec(&amx, &retval, entry_index, 0);
	} else {
		ret = amx_Exec(&amx, &retval, -2, 0);
	}
	if (ret != AMX_ERR_SLEEP) {
		if (state != 3)
			state = 2;
		if ((unsigned int)ret >= 2)
			Error::Raise(3, 2, 0);
	}
}

void Script::Release()
{
	state = 3;
	released_count++;
}

/* Queues an integer message; the queue holds at most 256. */
void Script::SendMessage(int a, int b, int c, int d, int e)
{
	if (int_queue.size() + 1 <= 256) {
		MessageInt m;
		MessageInt *p = &m;
		p->a = a;
		p->b = b;
		p->c = c;
		p->d = d;
		p->e = e;
		int_queue.push_back(*p);
	}
}

int Script::RecvMessage(int &a, int &b, int &c, int &d, int &e)
{
	if (int_queue.begin() != int_queue.end()) {
		MessageInt m = *int_queue.begin();
		int_queue.erase(int_queue.begin());
		a = m.a;
		b = m.b;
		c = m.c;
		d = m.d;
		e = m.e;
		return 1;
	}
	return 0;
}

int Script::PeekMessage(int &a, int &b, int &c, int &d, int &e)
{
	if (int_queue.begin() != int_queue.end()) {
		MessageInt m = *int_queue.begin();
		a = m.a;
		b = m.b;
		c = m.c;
		d = m.d;
		e = m.e;
		return 1;
	}
	return 0;
}

int Script::FindMessage(int key, int &b, int &c, int &d, int &e)
{
	if (!int_queue.empty()) {  /* redundant as written */
		for (vector<MessageInt>::iterator it = int_queue.begin(); it != int_queue.end(); it++) {
			if (key == (*it).a) {
				b = (*it).b;
				c = (*it).c;
				d = (*it).d;
				e = (*it).e;
				int_queue.erase(it);
				return 1;
			}
		}
		return 0;
	}
	return 0;
}

void Script::SendMessage(int key, const string &value)
{
	if (string_queue.size() + 1 <= 256)
		string_queue.push_back(MessageStr(key, value));
}

int Script::RecvMessage(int &key, string &value)
{
	if (string_queue.begin() != string_queue.end()) {
		MessageStr m = *string_queue.begin();
		string_queue.erase(string_queue.begin());
		key = m.key;
		value = m.value;
		return 1;
	}
	return 0;
}

int Script::PeekMessage(int &key, string &value)
{
	if (string_queue.begin() != string_queue.end()) {
		MessageStr m = *string_queue.begin();
		key = m.key;
		value = m.value;
		return 1;
	}
	return 0;
}

int Script::FindMessage(int key, string &value)
{
	if (string_queue.empty())  /* redundant as written */
		return 0;
	for (vector<MessageStr>::iterator it = string_queue.begin(); it != string_queue.end(); it++) {
		if (key == (*it).key) {
			value = (*it).value;
			string_queue.erase(it);
			return 1;
		}
	}
	return 0;
}

static void ScriptObject_OnDestroy(Hook *hook, void *a, void *b)
{
	Script *s;
	Animation *m;
	unsigned h;

	(void)a;
	(void)b;

	s = (Script *)hook->DelUserData(TAG_SMAL);
	if (s)
		s->Release();

	m = HookAnimation(hook, TAG_PMAN);
	if (m)
		delete m;
	m = HookAnimation(hook, TAG_RMAN);
	if (m)
		delete m;
	m = HookAnimation(hook, TAG_SMAN);
	if (m)
		delete m;
	m = HookAnimation(hook, TAG_CMAN);
	if (m)
		delete m;
	m = HookAnimation(hook, TAG_TMAN);
	if (m)
		delete m;

	if (hook == 0)  /* redundant as written */
		h = 0;
	else
		h = (unsigned)hook->GetUserData(TAG_HNDL);
	ObjectTable::Remove(h);
}

static void ScriptObject_OnUpdate(Hook *hook, void *a, void *b)
{
	Animation *m;
	Animation *d;

	(void)a;
	(void)b;

	m = HookAnimation(hook, TAG_PMAN);
	if (m) {
		m->Do((Object *)hook);
		if (m->m_state == 2) {
			d = TakeHookAnimation(hook, TAG_PMAN);
			if (d)
				delete d;
		}
	}

	m = HookAnimation(hook, TAG_RMAN);
	if (m) {
		m->Do((Object *)hook);
		if (m->m_state == 2) {
			d = TakeHookAnimation(hook, TAG_RMAN);
			if (d)
				delete d;
		}
	}

	m = HookAnimation(hook, TAG_SMAN);
	if (m) {
		m->Do((Object *)hook);
		if (m->m_state == 2) {
			d = TakeHookAnimation(hook, TAG_SMAN);
			if (d)
				delete d;
		}
	}

	m = HookAnimation(hook, TAG_CMAN);
	if (m) {
		m->Do((Object *)hook);
		if (m->m_state == 2) {
			d = TakeHookAnimation(hook, TAG_CMAN);
			if (d)
				delete d;
		}
	}

	m = HookAnimation(hook, TAG_TMAN);
	if (m) {
		m->Do((Object *)hook);
		if (m->m_state == 2) {
			d = TakeHookAnimation(hook, TAG_TMAN);
			if (d)
				delete d;
		}
	}
}

/* Registers the lifetime callbacks, puts obj in the object table and stores
   its handle as "HNDL" user data. If that fails obj is deleted and 0 returned. */
unsigned Object__CommonSetup(Object *obj)
{
	obj->SetCallback(1, ScriptObject_OnDestroy, 0);
	obj->SetCallback(0, ScriptObject_OnUpdate, 0);
	unsigned handle = ObjectTable::Regist(obj);
	obj->SetUserData(TAG_HNDL, (void *)handle);
	if (handle == 0)
		delete obj;
	return handle;
}

void Object__DisableDestroyFromScript(Object *obj)
{
	obj->SetUserData(TAG_LOCK, (void *)1);
}

/* Returns 0, after deleting it, if the script failed to load. */
Script *CreateScript(Object *obj, const string &a, const string &b, const string &c)
{
	Script *s = new Script(obj, a, b, c);
	if (s->state != 4)
		return s;
	delete s;
	return 0;
}

/* Returns the AMX string at cell index; strings over 4 KB go through malloc. */
string CellToString(__amx *amx, int index)
{
	/* Shared buffer for strings up to 4 KB. */
	static char s_cellBuf[4096];
	cell *addr;
	if (amx_GetAddr(amx, index, &addr) == 0) {
		int len;
		amx_StrLen(addr, &len);
		if (len + 1 > 4096) {
			char *buf = (char *)malloc(len + 1);
			amx_GetString(buf, addr);
			string text = buf;
			free(buf);
			return text;
		}
		amx_GetString(s_cellBuf, addr);
		return s_cellBuf;
	}
	return "";
}

/* Document and node list of the script window. */
Document *GetDocument(__amx *amx)
{
	return &GetSceneWindow(amx)->document;
}

XmlTagTable *GetNodeList(__amx *amx)
{
	return &GetSceneWindow(amx)->node_list;
}

/* SceneWindow of the scene that owns amx. */
SceneWindow *GetSceneWindow(__amx *amx)
{
	void *cell;
	amx_GetUserData(amx, TAG_AMX_OWNER, &cell);
	Object *owner = ObjectTable::Get((int)cell);
	return SceneWindowOf(owner->scene);
}

SceneWindow *GetSceneWindow(Object *obj)
{
	return SceneWindowOf(obj->scene);
}

Script *GetSceneScript(SceneWindow *win)
{
	if (win != 0) {
		Object *owner = (Object *)win->scene;
		if (owner != 0)
			return (Script *)owner->GetUserData(TAG_SMAL);
	}
	return 0;
}

void *Script::SetUserData(unsigned int id, void *data)
{
	map<unsigned int, void *>::const_iterator it = user_data.find(id);
	void *old = (it != user_data.end()) ? it->second : 0;

	user_data[id] = data;
	return old;
}

void *Script::GetUserData(unsigned int id)
{
	map<unsigned int, void *>::const_iterator it = user_data.find(id);
	if (it != user_data.end())
		return it->second;
	return 0;
}

void *Script::DelUserData(unsigned int id)
{
	map<unsigned int, void *>::iterator it = user_data.find(id);
	if (it != user_data.end()) {
		void *old = (*it).second;
		user_data.erase(it);
		return old;
	}
	return 0;
}

void Script::SetCallback(CallbackType type, unsigned int key, void (*fn)(__amx *))
{
	for (vector<CallbackData>::iterator it = callbacks[type].begin(); it != callbacks[type].end(); ++it) {
		if ((*it).key == key) {
			(*it).fn = fn;
			return;
		}
	}
	CallbackData e;
	CallbackData *p = &e;
	p->key = key;
	p->fn = fn;
	callbacks[type].push_back(*p);
}

void Script::DoCallback(CallbackType type)
{
	for (vector<CallbackData>::iterator it = callbacks[type].begin(); it != callbacks[type].end(); ++it)
		(*it).fn(&amx);
}

void Script::TranslateAll(void (*fn)(Script *))
{
	if (fn == 0)
		return;
	for (list<Script *>::iterator it = s_scriptList.begin(); it != s_scriptList.end(); it++)
		fn(*it);
}

void Script::SetBusy(bool busy)
{
	if (busy) {
		if (!(flags & 4)) {
			flags |= 4;
			busy_count++;
		}
	} else {
		if (flags & 4) {
			flags &= ~4;
			busy_count--;
		}
	}
}

void Script::SetHttpsBusy(bool busy)
{
	if (busy) {
		if (!(flags & 8)) {
			flags |= 8;
			https_busy_count++;
		}
	} else {
		if (flags & 8) {
			flags &= ~8;
			https_busy_count--;
		}
	}
}
