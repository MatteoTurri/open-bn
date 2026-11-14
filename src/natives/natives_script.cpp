/*
 * Small/amx natives for pads, scripts and system parameters (tables
 * smallext_sys, smallext_sys_p and smallext_cookie): PadState, PadAnalog,
 * AttachScript, Abort, Get/SetPubVar, GetArg, ScriptState,
 * Get/SetSystemParam[Str], FrameRate[F], Mailto, SystemCommand, System,
 * SetCookie and GetCookie.
 */

#include "amx.h"
#include "natives/smallext.h"

#include <string>
#include <map>
#include <vector>
#include <stdlib.h>
#include <string.h>

#include "app/strutil.h"

#include "scene/object.h"

#include "scene/objecttable.h"

#include "script/script.h"
#include "ime/textim.h"
#include "input/mypad.h"
#include "app/command.h"
#include "app/systemoptions.h"
#include "script/scenewindow.h"
#include "app/buffer.h"
#include "app/systemparam.h"
#include "http/httpfile.h"
#include "http/sceurl.h"
#include "image/thimagedecoder.h"
#include "fs/thloadicon.h"
#include "thread/thposixthread.h"
#include "script/thxmlloader.h"
#include "xml/xml_filedescriptor.h"
#include "app/biff.h"

/* User data tags: 'HOBJ' = the object that owns the AMX, 'SMAL' = the Script attached to an object. */
static const long TAG_JBOH = 0x4A424F48;
static const unsigned int TAG_LAMS = 0x4C414D53;
/* 'PSCR': user data tag of the AMX holding its own Script. */
static const long TAG_RCSP = 0x52435350;

/*
 * PadState(&btn, &trg, &rel, &old [, port]): buttons of the pad (port 0 or 1;
 * -1, the default, is the active pad). All zero while the software keyboard
 * is open.
 */
static cell AMX_NATIVE_CALL n_PadState(AMX *amx, cell *params)
{
	int port = -1;
	int i;
	cell *p;
	myPad *pad;

	if (((ucell)params[0] >> 2) >= 5)
		port = params[5];

	if (TextIM::IsOpened()) {
		for (i = 0; i < 4; i++) {
			if (amx_GetAddr(amx, params[i + 1], &p) == 0)
				*p = 0;
		}
		return 0;
	}

	pad = 0;
	if (port == 0)
		pad = pPad0;
	else if (port == 1)
		pad = pPad1;
	else if (port == -1) {
		/* active pad: the first one if any button is down or released, else the second */
		myPad *cand = pPad0;

		if (cand != 0 && (cand->m_buttons[0].w | cand->m_buttons[3].w) != 0)
			pad = cand;
		else
			pad = pPad1;
	}
	if (pad == 0)
		return 0;

	if (amx_GetAddr(amx, params[1], &p) == 0)
		*p = pad->m_buttons[0].w;
	if (amx_GetAddr(amx, params[2], &p) == 0)
		*p = pad->m_buttons[2].w;
	if (amx_GetAddr(amx, params[3], &p) == 0)
		*p = pad->m_buttons[3].w;
	if (amx_GetAddr(amx, params[4], &p) == 0)
		*p = pad->m_buttons[1].w;
	return 0;
}

/* Centers an analog stick value on 0, with a dead zone of +-28 around 128. */
static inline int PadAnalogValue(int v)
{
	int d = v - 128;

	if (d >= 29)
		return v - 156;
	if (d <= -29)
		return v - 100;
	return 0;
}

/* Clamps v between its two bounds. */
static inline int PadAnalogClamp(int v, int lo, int hi)
{
	return (v < lo) ? lo : (v > hi) ? hi : v;
}

/*
 * PadAnalog(&a0, ..., &a15 [, port]): the sixteen analog axes, summed over
 * the selected pads (port 0, 1, or -1 = both) and clamped to the range of each
 * axis; all zero while the software keyboard is open.
 */
static cell AMX_NATIVE_CALL n_PadAnalog(AMX *amx, cell *params)
{
	/* Range of each axis: the four sticks -100..100, the others 0..256. */
	static const int PadAnalogMin[16] = { -100, -100, -100, -100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	static const int PadAnalogMax[16] = { 100, 100, 100, 100, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256,
		256 };
	int port = -1;
	int i;

	if (((ucell)params[0] >> 2) >= 17)
		port = params[17];

	if (TextIM::IsOpened()) {
		cell *p;

		for (i = 0; i < 16; i++) {
			if (amx_GetAddr(amx, params[i + 1], &p) == 0)
				*p = 0;
		}
		return 0;
	}

	myPad *pads[3] = { 0 };
	if (port == 0)
		pads[0] = pPad0;
	else if (port == 1)
		pads[0] = pPad1;
	else if (port == -1) {
		pads[0] = pPad0;
		pads[1] = pPad1;
	}

	int acc[16] = { 0 };
	for (i = 0; pads[i] != 0; i++) {
		acc[0] += PadAnalogValue(pads[i]->m_axis[0]);
		acc[1] += PadAnalogValue(pads[i]->m_axis[1]);
		acc[2] += PadAnalogValue(pads[i]->m_axis[2]);
		acc[3] += PadAnalogValue(pads[i]->m_axis[3]);
		acc[4] += pads[i]->m_press[0];
		acc[5] += pads[i]->m_press[1];
		acc[6] += pads[i]->m_press[2];
		acc[7] += pads[i]->m_press[3];
		acc[8] += pads[i]->m_press[4];
		acc[9] += pads[i]->m_press[5];
		acc[10] += pads[i]->m_press[6];
		acc[11] += pads[i]->m_press[7];
		acc[12] += pads[i]->m_press[8];
		acc[13] += pads[i]->m_press[9];
		acc[14] += pads[i]->m_press[10];
		acc[15] += pads[i]->m_press[11];
	}

	{
		cell *p;

		for (i = 0; i < 16; i++) {
			if (amx_GetAddr(amx, params[1 + i], &p) == 0)
				*p = PadAnalogClamp(acc[i], PadAnalogMin[i], PadAnalogMax[i]);
		}
	}
	return 0;
}

/* The script attached to an object, user data 'SMAL'. */
static inline Script *ObjectScript(Object *owner)
{
	return (Script *)owner->GetUserData(TAG_LAMS);
}

/* Removes it from the object and returns it. */
static inline Script *TakeObjectScript(Object *owner)
{
	return (Script *)owner->DelUserData(TAG_LAMS);
}

/*
 * AttachScript(objid, func[], file[], arg[]): creates a new Script on the
 * object (which must be in the same group as the caller), detaches and
 * releases the old one and attaches the new one as user data 'SMAL'. An empty
 * file name means the file of the calling script; attaching to the caller's
 * own object ends the running script (AMX_ERR_EXIT).
 */
static cell AMX_NATIVE_CALL n_AttachScript(AMX *amx, cell *params)
{
	Object *obj;
	Object *owner;
	Object *self;
	void *ownerId;
	void *curp;
	void *selfId;
	string *cur;
	Script *old;
	Script *sc;

	obj = ObjectTable::Get(params[1]);
	if (obj == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	amx_GetUserData(amx, TAG_JBOH, &ownerId);
	owner = ObjectTable::Get((int)ownerId);
	if (owner->scene != obj->scene) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	amx_GetUserData(amx, TAG_RCSP, &curp);
	cur = (string *)curp;
	amx_GetUserData(amx, TAG_JBOH, &selfId);
	self = ObjectTable::Get((int)selfId);

	{
		string func = CellToString(amx, params[2]);
		string file = CellToString(amx, params[3]);

		if (file.length() == 0)
			file = *cur;

		string arg = CellToString(amx, params[4]);

		sc = CreateScript(obj, file, func, arg);
		if (sc == 0) {
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}
		old = TakeObjectScript(obj);
		if (old != 0)
			old->Release();
		obj->SetUserData(TAG_LAMS, sc);
		if (obj == self) {
			amx_RaiseError(amx, AMX_ERR_EXIT);
			return 0;
		}
		return 0;
	}
}

/*
 * Abort(objid): closes the Script attached to the object, only within the
 * caller's group; aborting the caller's own object ends the running script
 * (AMX_ERR_EXIT).
 */
static cell AMX_NATIVE_CALL n_Abort(AMX *amx, cell *params)
{
	Object *obj;
	Object *owner;
	Object *self;
	Script *script;
	void *ownerId;
	void *selfId;

	obj = ObjectTable::Get(params[1]);
	if (obj == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	amx_GetUserData(amx, TAG_JBOH, &ownerId);
	owner = ObjectTable::Get((int)ownerId);
	if (owner->scene != obj->scene) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	amx_GetUserData(amx, TAG_JBOH, &selfId);
	self = ObjectTable::Get((int)selfId);

	script = TakeObjectScript(obj);
	if (script != 0)
		script->Release();

	if (obj == self) {
		amx_RaiseError(amx, AMX_ERR_EXIT);
		return 0;
	}
	return 0;
}

/* SetPubVar(objid, name, value): sets a public variable of the script of the object (same group only). */
static cell AMX_NATIVE_CALL n_SetPubVar(AMX *amx, cell *params)
{
	Object *obj;
	Object *owner;
	Script *sc;
	void *ownerId;
	cell amx_addr;
	cell *phys;

	obj = ObjectTable::Get(params[1]);
	if (obj == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	amx_GetUserData(amx, TAG_JBOH, &ownerId);
	owner = ObjectTable::Get((int)ownerId);
	if (owner->scene != obj->scene) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	sc = ObjectScript(obj);
	if (sc == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	string name = CellToString(amx, params[2]);

	if (amx_FindPubVar(&sc->amx, name.c_str(), &amx_addr) == 0) {
		if (amx_GetAddr(&sc->amx, amx_addr, &phys) == 0)
			*phys = params[3];
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	return 0;
}

/* GetPubVar(objid, name): reads a public variable of the script of the object; no group check. */
static cell AMX_NATIVE_CALL n_GetPubVar(AMX *amx, cell *params)
{
	Object *obj;
	Script *sc;
	cell amx_addr;
	cell *phys;

	obj = ObjectTable::Get(params[1]);
	if (obj == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	sc = ObjectScript(obj);
	if (sc == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	string name = CellToString(amx, params[2]);

	if (amx_FindPubVar(&sc->amx, name.c_str(), &amx_addr) == 0) {
		if (amx_GetAddr(&sc->amx, amx_addr, &phys) == 0)
			return *phys;
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	return 0;
}

/*
 * GetArg(name[], dest[], size): looks name up in the arguments of the calling
 * script and copies the value, truncated to size cells. Returns 1 when the
 * key exists, else 0.
 */
static cell AMX_NATIVE_CALL n_GetArg(AMX *amx, cell *params)
{
	Script *sc;
	void *scp;
	cell *dest;
	cell addr, end;
	int max;
	int ok;
	int found;

	amx_GetUserData(amx, TAG_RCSP, &scp);
	sc = (Script *)scp;

	string key = CellToString(amx, params[1]);
	string val;

	{
		map<string, string>::const_iterator it = sc->args.find(key);

		if (it != sc->args.end()) {
			val = (*it).second;
			found = 1;
		} else {
			val = "";
			found = 0;
		}
	}

	max = params[3] * 4;
	if (max < val.length() + 1)
		val = string(val, 0, max - 1);

	cell len = val.length();
	addr = params[2];
	end = len + addr;
	if (addr >= 0 && addr < amx->hea && end >= 0 && end < amx->hea)
		ok = 1;
	else
		ok = addr >= amx->stk && addr < amx->stp && end >= amx->stk && end < amx->stp;
	if (!ok) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[2], &dest);
	amx_SetString(dest, (char *)val.c_str(), 1);
	return found;
}

/* ScriptState(objid): 0 when the object has no Script, 1 while its state is 0 or 1, else 2. */
static cell AMX_NATIVE_CALL n_ScriptState(AMX *amx, cell *params)
{
	Object *obj;
	Script *sc;

	obj = ObjectTable::Get(params[1]);
	if (obj == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	sc = ObjectScript(obj);
	if (sc == 0)
		return 0;
	switch (sc->state) {
	case 0:
	case 1:
		return 1;
	case 2:
		return 2;
	case 3 ... 2147483647:  /* redundant as written */
		return 2;
	}
	return 2;
}


/*
 * SetSystemParam(id, value): sets one of the SystemParam values. Ids 500, 503,
 * 504 and 602 are always allowed, the others only for a window that does not
 * come from the network.
 */
static cell AMX_NATIVE_CALL n_SetSystemParam(AMX *amx, cell *params)
{
	switch (params[1]) {
	case 500:
		SystemParam::padhelp_onoff = params[2];
		return 0;
	case 503:
		SystemParam::fg_color_a = params[2];
		if (SceneWindow::fg_window != 0)
			SceneWindow::fg_window->target_alpha = params[2] / 255.0f;
		return 0;
	case 504:
		SystemParam::fgbar_color_a = params[2];
		return 0;
	case 602:
		SystemParam::enable_navimenu = (params[2] != 0);
		return 0;
	}
	if (GetSceneWindow(amx)->IsFromNetwork())
		return 0;
	switch (params[1]) {
	case 603:
		SystemParam::alert_illegal_disc = (params[2] != 0);
		return 0;
	case 400:
		SystemParam::auth_save = (params[2] != 0);
		return 0;
	case 401:
		SystemParam::auth_ok = (params[2] != 0);
		return 0;
	case 501:
		SystemParam::page_max = params[2];
		return 0;
	case 502:
		SystemParam::page_no = params[2];
		return 0;
	case 600:
		SystemParam::enable_se = (params[2] != 0);
		return 0;
	case 601:
		SystemParam::enable_bgm = (params[2] != 0);
		return 0;
	}
	return 0;
}

#include "fs/thdbaccess.h"

#include "fs/thmediachecker.h"

/*
 * GetSystemParam(id): reads a system parameter (SystemParam, SysOption, busy
 * counters). Ids 0, 1, 500, 503, 504 and 602 always answer, the others only
 * for a window that does not come from the network.
 */
static cell AMX_NATIVE_CALL n_GetSystemParam(AMX *amx, cell *params)
{
	switch (params[1]) {
	case 0:
		return 640;
	case 1:
		return 448;
	case 500:
		return SystemParam::padhelp_onoff;
	case 503:
		return SystemParam::fg_color_a;
	case 504:
		return SystemParam::fgbar_color_a;
	case 602:
		return SystemParam::enable_navimenu;
	}
	if (GetSceneWindow(amx)->IsFromNetwork())
		return 0;
	switch (params[1]) {
	case 603:
		return SystemParam::alert_illegal_disc;
	case 200:
		return Biff::Exist();
	case 201:
		return Biff::Sound();
	case 210: {
		SceneWindow *w = SceneWindow::main_window;

		if (w == 0)
			return 0;
		return strncasecmp(w->uri.c_str(), "https:", 6) == 0;
	}
	case 211:
		return Script::https_busy_count > 0;
	case 100:
		return (HttpFile::netbusy_count > 0 || thImageDecoder::instance_count > 0
		    || thLoadIcon::instance_count > 0 || thXmlLoader::instance_count > 0
		    || thDBAccess::BusyCounter::instance_count > 0 || Script::busy_count > 0);
	case 101:
		return HttpFile::netbusy_count > 0;
	case 102:
		return (thImageDecoder::instance_count + thLoadIcon::instance_count) > 0;
	case 103:
		return thXmlLoader::instance_count > 0;
	case 104:
		return thDBAccess::BusyCounter::instance_count > 0;
	case 105:
		return Script::busy_count > 0;
	case 300:
		return thMediaChecker::getCDVDType();
	case 301:
		return SystemParam::network_up;
	case 700:
		return SysOption.m_language;
	case 501:
		return SystemParam::page_max;
	case 502:
		return SystemParam::page_no;
	case 600:
		return SystemParam::enable_se;
	case 601:
		return SystemParam::enable_bgm;
	case 701:
		return SysOption.m_keyboardType;
	case 702:
		return SysOption.m_keyboardRepeatWait;
	case 703:
		return SysOption.m_keyboardRepeatSpeed;
	case 704:
		return SysOption.m_mouseSpeed;
	case 705:
		return SysOption.m_mouseDClick;
	case 706:
		return SysOption.m_mouseChangeLR;
	case 707:
		return SysOption.m_atokMode;
	case 708:
		return SysOption.m_atokKeyBind;
	case 709:
		return SysOption.m_softkbOnOff;
	case 710:
		return SysOption.m_softkbQwerty;
	case 711:
		return SysOption.m_spdif;
	case 712:
		return SysOption.m_aspect;
	case 713:
		return SysOption.m_video;
	case 714:
		return SysOption.m_timeZone;
	case 715:
		return SysOption.m_timeZoneCity;
	case 716:
		return SysOption.m_summerTime;
	case 717:
		return SysOption.m_timeNotation;
	case 718:
		return SysOption.m_dateNotation;
	case 719:
		return SysOption.m_autoDiag;
	case 800:
		return SysOption.unk_20;
	}
	return 0;
}

/*
 * SetSystemParamStr(id, str): sets one of the string system parameters; as
 * SetSystemParam, some ids are refused to a window from the network.
 */
static cell AMX_NATIVE_CALL n_SetSystemParamStr(AMX *amx, cell *params)
{
	string v = CellToString(amx, params[2]);

	switch (params[1]) {
	case 500:
		SystemParam::padhelp_str_rr = v;
		return 0;
	case 501:
		SystemParam::padhelp_str_rd = v;
		return 0;
	case 502:
		SystemParam::padhelp_str_ru = v;
		return 0;
	case 503:
		SystemParam::padhelp_str_rl = v;
		return 0;
	}
	if (!GetSceneWindow(amx)->IsFromNetwork()) {
		switch (params[1]) {
		case 602:
			SystemParam::padhelp_str_def_rr = v;
			return 0;
		case 600:
			SystemParam::padhelp_str_def_rl = v;
			return 0;
		case 601:
			SystemParam::padhelp_str_def_rd = v;
			return 0;
		case 603:
			SystemParam::padhelp_str_def_ru = v;
			return 0;
		case 400:
			SystemParam::auth_id = v;
			return 0;
		case 401:
			SystemParam::auth_pass = v;
			return 0;
		}
	}
	return 0;
}

/*
 * GetSystemParamStr(id, dest, size): a string parameter (500..503 = pad help
 * labels, 900 = the version "0.31", only for a window that does not come from
 * the network), truncated to size cells.
 */
static cell AMX_NATIVE_CALL n_GetSystemParamStr(AMX *amx, cell *params)
{
	string s;
	cell *dest;
	cell addr, end;
	int max;
	int ok;

	switch (params[1]) {
	case 500:
		s = SystemParam::padhelp_str_rr;
		break;
	case 501:
		s = SystemParam::padhelp_str_rd;
		break;
	case 502:
		s = SystemParam::padhelp_str_ru;
		break;
	case 503:
		s = SystemParam::padhelp_str_rl;
		break;
	default:
		if (GetSceneWindow(amx)->IsFromNetwork())
			break;
		if (params[1] == 900)
			s = "0.31";
		break;
	}

	max = params[3] * 4;
	if (max < s.length() + 1)
		s = string(s, 0, max - 1);

	cell len = s.length();
	addr = params[2];
	end = len + addr;
	if (addr >= 0 && addr < amx->hea && end >= 0 && end < amx->hea)
		ok = 1;
	else
		ok = addr >= amx->stk && addr < amx->stp && end >= amx->stk && end < amx->stp;
	if (!ok) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[2], &dest);
	amx_SetString(dest, (char *)s.c_str(), 1);
	return 0;
}

static cell AMX_NATIVE_CALL n_FrameRate(AMX *amx, cell *params)
{
	return SysOption.m_frameRate;
}

/* FrameRateF(): the frame rate as a float. */
static cell AMX_NATIVE_CALL n_FrameRateF(AMX *amx, cell *params)
{
	float f = (float)SysOption.m_frameRate;

	return amx_ftoc(f);
}

/* Completion callback of the window created by Mailto. */
static void ScriptWindowDone(void *userData, int result, SceneWindow::CreateArg *arg)
{
	SceneWindow *win;

	if (arg == 0)
		return;
	win = CreateSceneWindow(*arg);
	if (win == 0)
		return;
	win->hide_flag = 1;
}

/*
 * Mailto(to[], subject[], body[]): URL-escapes the three fields (at most 255
 * characters each) and opens the mailer page file:/opt0/bn/script/mailer/main.xml
 * with "to=...;subject=...;body=..." in a new window; the script sleeps
 * meanwhile (AMX_ERR_SLEEP).
 */
static cell AMX_NATIVE_CALL n_Mailto(AMX *amx, cell *params)
{
	string to = CellToString(amx, params[1]);
	string subject = CellToString(amx, params[2]);
	string body = CellToString(amx, params[3]);

	if (to.length() >= 256 || subject.length() >= 256 || body.length() >= 256) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	char *eto = sceURLEscape(to.c_str());
	char *esubject = sceURLEscape(subject.c_str());
	char *ebody = sceURLEscape(body.c_str());

	string url = string("to=") + eto + ";subject=" + esubject + ";body=" + ebody;

	free(eto);
	free(esubject);
	free(ebody);

	{
		thXmlLoader *ld = new thXmlLoader("file:/opt0/bn/script/mailer/main.xml", "", url,
		    SceneWindow::MODE_MODAL, 0, 0, ScriptWindowDone, Buffer());

		ld->addThreadIdleList(2, 1);
		ld->release();
	}
	amx_RaiseError(amx, AMX_ERR_SLEEP);
	return 0;
}

/* The script that runs amx, user data 'RCSP' of the AMX. */
static inline Script *AmxScript(AMX *amx)
{
	Script *s;

	amx_GetUserData(amx, TAG_RCSP, (void **)&s);
	return s;
}

/*
 * SystemCommand(cmd, &arg): command 100 (busy flag) is allowed to any window;
 * the others (10100..10500) only to a window that does not come from the
 * network.
 */
static cell AMX_NATIVE_CALL n_SystemCommand(AMX *amx, cell *params)
{
	if (params[1] == 100) {
		cell *p;

		if (((ucell)params[0] >> 2) < 2) {
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}
		amx_GetAddr(amx, params[2], &p);
		AmxScript(amx)->SetBusy(*p != 0);
		return 0;
	}

	if (GetSceneWindow(amx)->IsFromNetwork())
		return 0;

	switch (params[1]) {
	case 10100: {
		cell *p;

		if (((ucell)params[0] >> 2) < 2) {
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}
		amx_GetAddr(amx, params[2], &p);
		AmxScript(amx)->SetHttpsBusy(*p != 0);
		return 0;
	}
	case 10200:
		Biff::Clear();
		return 0;
	case 10300: {
		cell *p;

		if (((ucell)params[0] >> 2) < 2) {
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}
		amx_GetAddr(amx, params[2], &p);
		if (*p != 0)
			GetSceneWindow(amx)->hide_flag = 1;
		else
			GetSceneWindow(amx)->hide_flag = 0;
		return 0;
	}
	case 10400: {
		SceneWindow::StartFadeoutMainWindow(500);
		if (AmxScript(amx)->state == 3) {
			amx_RaiseError(amx, AMX_ERR_EXIT);
			return 0;
		}
		return 0;
	}
	case 10401: {
		SceneWindow::StartFadeoutBGWindow(500);
		if (AmxScript(amx)->state == 3) {
			amx_RaiseError(amx, AMX_ERR_EXIT);
			return 0;
		}
		return 0;
	}
	case 10500: {
		AmxScript(amx)->flags |= 0x10;
		return 0;
	}
	}
	return 0;
}

AMX_NATIVE_INFO smallext_sys[] = {
	{ "PadState", n_PadState },
	{ "PadAnalog", n_PadAnalog },
	{ "AttachScript", n_AttachScript },
	{ "Abort", n_Abort },
	{ "SetPubVar", n_SetPubVar },
	{ "GetPubVar", n_GetPubVar },
	{ "GetArg", n_GetArg },
	{ "ScriptState", n_ScriptState },
	{ "GetSystemParam", n_GetSystemParam },
	{ "SetSystemParam", n_SetSystemParam },
	{ "GetSystemParamStr", n_GetSystemParamStr },
	{ "SetSystemParamStr", n_SetSystemParamStr },
	{ "FrameRate", n_FrameRate },
	{ "FrameRateF", n_FrameRateF },
	{ "Mailto", n_Mailto },
	{ "SystemCommand", n_SystemCommand },
	{ 0, 0 },
};

/*
 * System(cmd): runs a Command described by "name;key=value;key=value...".
 * Keys and values are trimmed; a key without '=' gets an empty value. A
 * failing handler raises AMX_ERR_NATIVE.
 */
static cell AMX_NATIVE_CALL n_System(AMX *amx, cell *params)
{
	StringUtil su = CellToString(amx, params[1]);
	vector<StringUtil> v = su.Separate(";");

	if (v.size() == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	Command cmd(v[0]);
	for (unsigned int i = 1; i < v.size(); i++) {
		vector<StringUtil> kv = v[i].Separate("=");
		if (kv.size() == 1) {
			StrTrimLeft(kv[0]);
			StrTrimRight(kv[0]);
			cmd.SetArg(kv[0], "");
		} else if (kv.size() >= 2) {
			StrTrimLeft(kv[0]);
			StrTrimRight(kv[0]);
			StrTrimLeft(kv[1]);
			StrTrimRight(kv[1]);
			cmd.SetArg(kv[0], kv[1]);
		}
	}
	if (cmd.CallHandler() != Command::Result_ok) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	return 0;
}

AMX_NATIVE_INFO smallext_sys_p[] = {
	{ "System", n_System },
	{ 0, 0 },
};

/* The first n characters of s. */
static inline StringUtil Prefix(const string &s, unsigned int n)
{
	return s.substr(0, n);
}

/* Case-insensitive compare of pre with pat. */
static inline int SchemeCmp(const StringUtil &pre, StringUtil pat)
{
	StringUtil head(pre);
	head.MakeUpper();
	pat.MakeUpper();
	return head.compare(pat);
}

/*
 * Normalizes the URL of a cookie in place: drops everything after the last
 * '/' and prefixes "http://~~local~~/" to a URL that is not http: or https:.
 */
static void NormalizeCookieUrl(string &url)
{
	unsigned int pos = url.find_last_of("/");
	if (pos != (unsigned int)-1)
		url.erase(pos + 1);

	string u = url;
	if (SchemeCmp(Prefix(u, 5), "http:") != 0 && SchemeCmp(Prefix(u, 6), "https:") != 0)
		url = string("http://~~local~~/") + url;
}

/* Meant to cut s to 4096 characters. */
static void ClampTo4K(const string &s)
{
	if (s.length() > 4096) {
		string t = s.substr(0, 4096);  /* NOTE: bug: the truncated copy is discarded, s is left as it is */
	}
}

/*
 * SetCookie(cookie[]): stores the cookie for the current URL of the window
 * (normalized by NormalizeCookieUrl).
 */
static cell AMX_NATIVE_CALL n_SetCookie(AMX *amx, cell *params)
{
	string name = CellToString(amx, params[1]);
	string url = GetSceneWindow(amx)->uri;

	NormalizeCookieUrl(url);
	ClampTo4K(name);
	HttpFile::add_cookie_from_strings(url.c_str(), name.c_str());
	return 0;
}

/*
 * GetCookie(name[], dest[], size): value of the cookie name for the current
 * URL of the window, truncated to size cells. Returns 1 when the cookie exists.
 */
static cell AMX_NATIVE_CALL n_GetCookie(AMX *amx, cell *params)
{
	sceHTTPCookieList *list;
	sceHTTPCookieList *n;
	cell *dest;
	cell addr, end;
	int max;
	int ok;
	int found;

	string name = CellToString(amx, params[1]);
	string url = GetSceneWindow(amx)->uri;

	NormalizeCookieUrl(url);
	list = HttpFile::get_cookies_from_uri(url.c_str());

	found = 0;

	string value;

	for (n = list; n != 0; n = n->next) {
		char **kv = &n->cookie.name;

		if (name == kv[0]) {
			value = kv[1];
			ClampTo4K(value);
			found = 1;
			break;
		}
	}
	sceHTTPFreeCookieList(list);

	if (found) {
		max = params[3] * 4;
		if (max < value.length() + 1)
			value = string(value, 0, max - 1);

		cell len = value.length();
		addr = params[2];
		end = len + addr;
		if (addr >= 0 && addr < amx->hea && end >= 0 && end < amx->hea)
			ok = 1;
		else
			ok = addr >= amx->stk && addr < amx->stp && end >= amx->stk && end < amx->stp;
		if (!ok) {
			amx_RaiseError(amx, AMX_ERR_MEMACCESS);
			return 0;
		}
		amx_GetAddr(amx, params[2], &dest);
		amx_SetString(dest, (char *)value.c_str(), 1);
	}
	return found;
}

AMX_NATIVE_INFO smallext_cookie[] = {
	{ "SetCookie", n_SetCookie },
	{ "GetCookie", n_GetCookie },
	{ 0, 0 },
};
