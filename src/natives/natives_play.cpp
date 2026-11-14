/*
 * Small/amx native "play": hands a RealPlayer URL (.ram/.rm) to the external
 * player application.
 */

#include <string>
#include <map>

#include "amx.h"
#include "natives/smallext.h"

#include "app/strutil.h"
#include "app/command.h"
#include "app/variables.h"

#include "script/script.h"

/* Drops the query string (everything from '?' on). */
static string StripQuery(string s)
{
	unsigned int p = s.find_first_of("?");

	if (p != (unsigned int)-1)
		return s.substr(0, p);
	return s;
}

/* File name part of a URL (after the last '/'), "noname" when there is no slash. */
static string BaseName(string s)
{
	string t = StripQuery(s);
	unsigned int p = t.find_last_of("/");

	if (p != (unsigned int)-1)
		return t.substr(p + 1);
	return "noname";
}

/* Extension of the URL file name (after the last '.'), empty when there is none. */
static string Extension(string s)
{
	string t = BaseName(s);
	unsigned int p = t.find_last_of(".");

	if (p != (unsigned int)-1)
		return t.substr(p + 1);
	return "";
}

static int SetResumeNetBrowse(const string &v)
{
	Variables::SetVariable("resume_netbrowse", v);
	return 0;
}

/* Case-insensitive compare, as SchemeCmp in natives_img.cpp. */
static inline int ExtCmp(const StringUtil &ext, StringUtil pat)
{
	StringUtil head(ext);
	head.MakeUpper();
	pat.MakeUpper();
	return head.compare(pat);
}

/* Extensions handled by RealPlayer. */
static const char *s_realExt[] = { "ram", "rm", 0 };

/*
 * play(url, resume): for a RealPlayer URL runs the "ExecApp" command and ends
 * the script (AMX_ERR_EXIT; AMX_ERR_NATIVE if the command fails); any other
 * URL is ignored. resume is stored in the "resume_netbrowse" variable.
 */
static cell AMX_NATIVE_CALL n_play(AMX *amx, cell *params)
{
	string url = CellToString(amx, params[1]);
	string resume = CellToString(amx, params[2]);
	StringUtil ext = Extension(url);

	SetResumeNetBrowse(resume);

	int found = 0;
	for (int i = 0; s_realExt[i] != 0; i++) {
		if (ExtCmp(ext, s_realExt[i]) == 0) {
			found = 1;
			break;
		}
	}
	if (found == 1) {
		Command cmd("ExecApp");

		cmd.SetArg("appname", "RealPlayer");
		cmd.SetArg("arglist", url);
		if (cmd.CallHandler() != Command::Result_ok) {
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}
		amx_RaiseError(amx, AMX_ERR_EXIT);
		return 0;
	}
	return 0;
}

AMX_NATIVE_INFO smallext_play[] = {
	{ "play", n_play },
	{ 0, 0 },
};
