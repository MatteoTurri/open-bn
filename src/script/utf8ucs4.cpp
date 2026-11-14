/*
 * Software keyboard natives of the Small scripts (skb*), and changeUTF8toUCS4.
 */

#include "amx.h"
#include "natives/smallext.h"
#include "scenewindow.h"
#include "script.h"
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include "xml/xml_filedescriptor.h"

#include "lib/ccc.h"
#include "scene/object.h"
#include "scene/objecttable.h"
#include "ime/textim.h"

/* Keyboard session: -2 none, 0 open, 1 confirmed, -1 cancelled. */
static int s_skbState;

/* Text of the last confirmed session. */
static string s_skbString;

/* Close callback of the keyboard: publishes the text and the result for skbSync. */
static void SkbDone(bool ok, const string &s)
{
	if (ok) {
		s_skbString = s;
		s_skbState = 1;
	} else {
		s_skbString = "";
		s_skbState = -1;
	}
}

/* Converts a UTF-8 string into a 0-terminated UCS4 array, tracing every character on stderr. */
int changeUTF8toUCS4(const unsigned char *src, unsigned *out)
{
	unsigned buf[2048];
	int i;
	unsigned c;

	for (i = 0; (c = sceCccDecodeUTF8(&src)) != 0; i++) {
		buf[i] = c;
		fprintf(stderr, "sceCccDecodeUTF8 = %x\n", c);
	}
	buf[i] = 0;
	i = 0;
	if (buf[0] != 0) {  /* redundant as written */
		while (buf[i] != 0) {
			*out = buf[i];
			fprintf(stderr, "ucs4 = %x\n", buf[i]);
			out++;
			i++;
		}
	}
	*out = 0;
	return 0;
}

/* Natives of the skb* module. */

static const long TAG_JBOH = 0x4A424F48;

/* skbStart(objId, x, y, w, h, maxlen, &outLen, packed): opens the software
   keyboard and puts the script to sleep (AMX_ERR_SLEEP) until skbSync reports
   the end of the session. */
static cell AMX_NATIVE_CALL n_skbStart(AMX *amx, cell *params)
{
	Object *obj;
	Object *owner;
	void *ownerId;
	cell *out = 0;

	amx_GetAddr(amx, params[7], &out);

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

	if (TextIM::Open(obj, SkbDone, params[2], params[3], params[4], params[5], params[6], out, params[8] != 0, 0)
	    != 0) {
		s_skbState = 0;
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return 0;
	}
	return -1;
}

/* skbStartEx(objId, x, y, w, h, maxlen, text, packed, modes): like skbStart,
   with an initial text (converted to UCS4) and a string of digits, each one a
   keyboard mode (up to 15). */
static cell AMX_NATIVE_CALL n_skbStartEx(AMX *amx, cell *params)
{
	string text = CellToString(amx, params[7]);
	unsigned int *ucs;
	unsigned int modes[16];
	Object *obj;
	Object *owner;
	void *ownerId;
	int ret;

	ucs = (unsigned int *)malloc((sceCccStrlenUTF8((const unsigned char *)text.c_str()) + 1) * 4);
	if (ucs != 0)
		changeUTF8toUCS4((const unsigned char *)text.c_str(), ucs);
	modes[0] = 0;
	string mode = CellToString(amx, params[9]);
	if (mode.c_str() != 0) {
		const char *str = mode.c_str();
		int i;

		for (i = 0; i < 15 && str[i] != 0; i++) {
			char c[2];

			c[0] = str[i];
			c[1] = 0;
			modes[i] = strtol(c, 0, 10);
		}
		modes[i] = 0;
	}

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

	ret = TextIM::Open(obj, SkbDone, params[2], params[3], params[4], params[5], params[6], (int *)ucs,
	    params[8] != 0, modes);
	if (ucs != 0)
		free(ucs);
	if (ret != 0) {
		s_skbState = 0;
		amx_RaiseError(amx, AMX_ERR_SLEEP);
		return 0;
	}
	return -1;
}

/* skbSync(): -1 cancelled, 0 still open, 1 confirmed; a finished session goes
   back to -2 (none). */
static cell AMX_NATIVE_CALL n_skbSync(AMX *amx, cell *params)
{
	switch (s_skbState) {
	case 0:
		return 0;
	case 1:
		s_skbState = -2;
		return 1;
	case -1:
		s_skbState = -2;
		return -1;
	case -2:
	default:
		return -2;
	}
}

/* The range [p, q] must lie entirely in the heap or entirely in the stack of the script. */
static inline int amx_addr_ok(AMX *amx, cell p, cell q)
{
	if (p >= 0 && p < amx->hea && q >= 0 && q < amx->hea)
		return 1;
	if (p >= amx->stk && p < amx->stp && q >= amx->stk && q < amx->stp)
		return 1;
	return 0;
}

/* skbGetString(dest[], maxlen): copies the session text into the script, cut to maxlen*4-1 characters. */
static cell AMX_NATIVE_CALL n_skbGetString(AMX *amx, cell *params)
{
	cell *dest;

	if ((unsigned int)(params[2] * 4) < s_skbString.length() + 1)
		s_skbString = s_skbString.substr(0, params[2] * 4 - 1);
	if (!amx_addr_ok(amx, params[1], s_skbString.length() + params[1])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[1], &dest);
	amx_SetString(dest, (char *)s_skbString.c_str(), 1);
	return 0;
}

/* skbGetRect(&x, &y, &w, &h): each out-parameter is written only if its address is valid. */
static cell AMX_NATIVE_CALL n_skbGetRect(AMX *amx, cell *params)
{
	int x, y, w, h;
	cell *p;

	TextIM::GetSkbRect(x, y, w, h);
	if (amx_GetAddr(amx, params[1], &p) == 0)
		*p = x;
	if (amx_GetAddr(amx, params[2], &p) == 0)
		*p = y;
	if (amx_GetAddr(amx, params[3], &p) == 0)
		*p = w;
	if (amx_GetAddr(amx, params[4], &p) == 0)
		*p = h;
	return 0;
}

AMX_NATIVE_INFO smallext_skb[] = {
	{ "skbStart", n_skbStart },
	{ "skbStartEx", n_skbStartEx },
	{ "skbSync", n_skbSync },
	{ "skbGetString", n_skbGetString },
	{ "skbGetRect", n_skbGetRect },
	{ 0, 0 },
};
