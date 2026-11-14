/*
 * Small/amx natives for messages between scripts: SendMessage, RecvMessage,
 * PeekMessage, FindMessage and their *Str variants. A message goes only to an
 * object of the same scene window as the sender.
 */

#include <new>
#include <string>

#include "amx.h"
#include "natives/smallext.h"
#include "scene/hook.h"
#include "xml/xml_filedescriptor.h"
#include "scene/object.h"
#include "scene/objecttable.h"
#include "script/scenewindow.h"
#include "script/script.h"

static const long TAG_RCSP = 0x52435350;
static const unsigned int TAG_LAMS = 0x4C414D53;

/* The script attached to an object, user data 'SMAL'. */
static inline Script *ObjectScript(Object *owner)
{
	return (Script *)owner->GetUserData(TAG_LAMS);
}

/*
 * SendMessage(objId, msg, a, b, c, d): queues a message on the script of the
 * object; nothing is sent across windows or to a terminated script.
 */
static cell AMX_NATIVE_CALL n_SendMessage(AMX *amx, cell *params)
{
	Object *obj;
	Script *sc;
	string *mine;
	string *his;

	obj = ObjectTable::Get(params[1]);
	if (obj == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	mine = &GetSceneWindow(amx)->key;
	his = &GetSceneWindow(obj)->key;
	if (mine->compare(*his) != 0)
		return 0;

	sc = ObjectScript(obj);
	if (sc == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	if (params[2] < 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	if (sc->state == 2)
		return 0;
	sc->SendMessage(params[2], params[3], params[4], params[5], params[6]);
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
 * RecvMessage(&msg, &a, &b, &c, &d): each out-parameter is written only when
 * its address is valid; returns the result of Script::RecvMessage.
 */
static cell AMX_NATIVE_CALL n_RecvMessage(AMX *amx, cell *params)
{
	int m, a, b, c, d;
	cell *p;
	int r;

	r = AmxScript(amx)->RecvMessage(m, a, b, c, d);
	if (amx_GetAddr(amx, params[1], &p) == 0)
		*p = m;
	if (amx_GetAddr(amx, params[2], &p) == 0)
		*p = a;
	if (amx_GetAddr(amx, params[3], &p) == 0)
		*p = b;
	if (amx_GetAddr(amx, params[4], &p) == 0)
		*p = c;
	if (amx_GetAddr(amx, params[5], &p) == 0)
		*p = d;
	return r;
}

/* PeekMessage: as RecvMessage, without removing the message. */
static cell AMX_NATIVE_CALL n_PeekMessage(AMX *amx, cell *params)
{
	int m, a, b, c, d;
	cell *p;
	int r;

	r = AmxScript(amx)->PeekMessage(m, a, b, c, d);
	if (amx_GetAddr(amx, params[1], &p) == 0)
		*p = m;
	if (amx_GetAddr(amx, params[2], &p) == 0)
		*p = a;
	if (amx_GetAddr(amx, params[3], &p) == 0)
		*p = b;
	if (amx_GetAddr(amx, params[4], &p) == 0)
		*p = c;
	if (amx_GetAddr(amx, params[5], &p) == 0)
		*p = d;
	return r;
}

/* FindMessage(msg, &a, &b, &c, &d): takes the first queued message with id msg. */
static cell AMX_NATIVE_CALL n_FindMessage(AMX *amx, cell *params)
{
	int a, b, c, d;
	cell *p;
	int r;

	r = AmxScript(amx)->FindMessage(params[1], a, b, c, d);
	if (amx_GetAddr(amx, params[2], &p) == 0)
		*p = a;
	if (amx_GetAddr(amx, params[3], &p) == 0)
		*p = b;
	if (amx_GetAddr(amx, params[4], &p) == 0)
		*p = c;
	if (amx_GetAddr(amx, params[5], &p) == 0)
		*p = d;
	return r;
}

/*
 * SendMessageStr(objId, msg, str): as SendMessage with a string payload;
 * there is no check on the state of the target script.
 */
static cell AMX_NATIVE_CALL n_SendMessageStr(AMX *amx, cell *params)
{
	Object *obj;
	Script *sc;
	string *mine;
	string *his;

	obj = ObjectTable::Get(params[1]);
	if (obj == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	mine = &GetSceneWindow(amx)->key;
	his = &GetSceneWindow(obj)->key;
	if (mine->compare(*his) != 0)
		return 0;

	sc = ObjectScript(obj);
	if (sc == 0) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	{
		string s = CellToString(amx, params[3]);

		if (params[2] < 0) {
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}
		sc->SendMessage(params[2], s);
		return 0;
	}
}

/* True when [p,q] lies entirely in the script heap or entirely in its stack. */
static inline int amx_addr_ok(AMX *amx, cell p, cell q)
{
	if (p >= 0 && p < amx->hea && q >= 0 && q < amx->hea)
		return 1;
	if (p >= amx->stk && p < amx->stp && q >= amx->stk && q < amx->stp)
		return 1;
	return 0;
}

/*
 * RecvMessageStr(&msg, buf, size): as RecvMessage with a string payload,
 * truncated to size cells (size*4-1 characters).
 */
static cell AMX_NATIVE_CALL n_RecvMessageStr(AMX *amx, cell *params)
{
	void *ud;
	Script *sc;
	int m;
	cell *cptr;
	int r;

	amx_GetUserData(amx, TAG_RCSP, &ud);
	sc = (Script *)ud;
	{
		string s;

		r = sc->RecvMessage(m, s);
		if (amx_GetAddr(amx, params[1], &cptr) == 0)
			*cptr = m;
		if ((unsigned int)(params[3] * 4) < s.length() + 1)
			s = s.substr(0, params[3] * 4 - 1);
		if (!amx_addr_ok(amx, params[2], s.length() + params[2])) {
			amx_RaiseError(amx, 5);
			return 0;
		}
		amx_GetAddr(amx, params[2], &cptr);
		amx_SetString(cptr, (char *)s.c_str(), 1);
	}
	return r;
}

/* PeekMessageStr: as RecvMessageStr, without removing the message. */
static cell AMX_NATIVE_CALL n_PeekMessageStr(AMX *amx, cell *params)
{
	void *ud;
	Script *sc;
	int m;
	cell *cptr;
	int r;

	amx_GetUserData(amx, TAG_RCSP, &ud);
	sc = (Script *)ud;
	{
		string s;

		r = sc->PeekMessage(m, s);
		if (amx_GetAddr(amx, params[1], &cptr) == 0)
			*cptr = m;
		if ((unsigned int)(params[3] * 4) < s.length() + 1)
			s = s.substr(0, params[3] * 4 - 1);
		if (!amx_addr_ok(amx, params[2], s.length() + params[2])) {
			amx_RaiseError(amx, 5);
			return 0;
		}
		amx_GetAddr(amx, params[2], &cptr);
		amx_SetString(cptr, (char *)s.c_str(), 1);
	}
	return r;
}

/* FindMessageStr(msg, buf, size): takes the first queued string message with id msg. */
static cell AMX_NATIVE_CALL n_FindMessageStr(AMX *amx, cell *params)
{
	void *ud;
	Script *sc;
	cell *cptr;
	int r;

	amx_GetUserData(amx, TAG_RCSP, &ud);
	sc = (Script *)ud;
	{
		string s;

		r = sc->FindMessage(params[1], s);
		if ((unsigned int)(params[3] * 4) < s.length() + 1)
			s = s.substr(0, params[3] * 4 - 1);
		if (!amx_addr_ok(amx, params[2], s.length() + params[2])) {
			amx_RaiseError(amx, 5);
			return 0;
		}
		amx_GetAddr(amx, params[2], &cptr);
		amx_SetString(cptr, (char *)s.c_str(), 1);
	}
	return r;
}

AMX_NATIVE_INFO smallext_msg[] = {
	{ "SendMessage", n_SendMessage },
	{ "RecvMessage", n_RecvMessage },
	{ "PeekMessage", n_PeekMessage },
	{ "FindMessage", n_FindMessage },
	{ "SendMessageStr", n_SendMessageStr },
	{ "RecvMessageStr", n_RecvMessageStr },
	{ "PeekMessageStr", n_PeekMessageStr },
	{ "FindMessageStr", n_FindMessageStr },
	{ 0, 0 },
};
