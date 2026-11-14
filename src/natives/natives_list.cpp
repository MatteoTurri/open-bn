/*
 * Small/amx natives "list*": a list of integers owned by the running Script.
 * Every native looks up the Script through the AMX user data; an index out of
 * range raises AMX_ERR_NATIVE and the native returns 0.
 */

#include "amx.h"
#include "natives/smallext.h"
#include <vector>
#include "script/script.h"

static const long TAG_PSCR = 0x52435350;   /* 'PSCR': user data tag of the Script */

static inline Script *AmxScript(AMX *amx)
{
	Script *s;

	amx_GetUserData(amx, TAG_PSCR, (void **)&s);
	return s;
}

static cell AMX_NATIVE_CALL n_listSize(AMX *amx, cell *params)
{
	Script *s = AmxScript(amx);

	return s->int_list.size();
}

static cell AMX_NATIVE_CALL n_listGetAt(AMX *amx, cell *params)
{
	Script *s = AmxScript(amx);
	int i = params[1];

	if (i < 0 || i >= (int)s->int_list.size()) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	return s->int_list[i];
}

static cell AMX_NATIVE_CALL n_listGetLast(AMX *amx, cell *params)
{
	Script *s = AmxScript(amx);

	if (s->int_list.empty()) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	return s->int_list[s->int_list.size() - 1];
}

static cell AMX_NATIVE_CALL n_listAddLast(AMX *amx, cell *params)
{
	Script *s = AmxScript(amx);

	s->int_list.push_back(params[1]);
	return 0;
}

static cell AMX_NATIVE_CALL n_listInsert(AMX *amx, cell *params)
{
	Script *s = AmxScript(amx);
	int i = params[1];

	if (i < 0 || i >= (int)s->int_list.size()) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	s->int_list.insert(s->int_list.begin() + i, params[2]);
	return 0;
}

static cell AMX_NATIVE_CALL n_listRemoveAt(AMX *amx, cell *params)
{
	Script *s = AmxScript(amx);
	int i = params[1];

	if (i < 0 || i >= (int)s->int_list.size()) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	s->int_list.erase(s->int_list.begin() + i);
	return 0;
}

static cell AMX_NATIVE_CALL n_listRemoveLast(AMX *amx, cell *params)
{
	Script *s = AmxScript(amx);

	if (s->int_list.empty()) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	s->int_list.pop_back();
	return 0;
}

static cell AMX_NATIVE_CALL n_listRemoveAll(AMX *amx, cell *params)
{
	Script *s = AmxScript(amx);

	s->int_list.clear();
	return 0;
}

AMX_NATIVE_INFO smallext_list[] = {
	{ "listSize", n_listSize },
	{ "listGetAt", n_listGetAt },
	{ "listGetLast", n_listGetLast },
	{ "listAddLast", n_listAddLast },
	{ "listInsert", n_listInsert },
	{ "listRemoveAt", n_listRemoveAt },
	{ "listRemoveLast", n_listRemoveLast },
	{ "listRemoveAll", n_listRemoveAll },
	{ 0, 0 },
};
