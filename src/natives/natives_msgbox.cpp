/*
 * Small/amx natives "msgbox*": a modal message box opened from a script.
 */

#include <new>
#include <string>

#include "amx.h"
#include "natives/smallext.h"

#include "script/script.h"

#include "ui/msgbox.h"

/*
 * msgboxStart(type, msg, yes, no, flag): opens the box and suspends the script
 * (AMX_ERR_SLEEP); msgboxSync reads the answer when the script resumes.
 */
static cell AMX_NATIVE_CALL n_msgboxStart(AMX *amx, cell *params)
{
	int type = params[1];
	string msg = CellToString(amx, params[2]);
	string yes = CellToString(amx, params[3]);
	string no = CellToString(amx, params[4]);
	int flag = params[5];

	MsgBox::Open(type, msg.c_str(), yes.c_str(), no.c_str(), flag);
	amx_RaiseError(amx, AMX_ERR_SLEEP);
	return 0;
}

/* msgboxSync(): the answer of the box, 0 while there is none yet. */
static cell AMX_NATIVE_CALL n_msgboxSync(AMX *amx, cell *params)
{
	return MsgBox::Sync();
}

AMX_NATIVE_INFO smallext_msgbox[] = {
	{ "msgboxStart", n_msgboxStart },
	{ "msgboxSync", n_msgboxSync },
	{ 0, 0 },
};
