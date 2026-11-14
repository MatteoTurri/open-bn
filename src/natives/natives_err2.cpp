/*
 * Small/amx natives "err2000x" (table smallext_movie): path and URL helpers
 * for the movie/album scripts, and the album name list. The native names are
 * generated and carry no meaning.
 */

#include "amx.h"
#include "natives/smallext.h"
#include "script/scenewindow.h"
#include "script/script.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include "app/bnmisc.h"

/* Album names kept by err20007..err20009, one per line. */
string albumNameList;

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
 * err20001(path, buf): turns a path into a URL. "pfs:" paths become
 * "file://" + the whole path, any other "scheme:rest" becomes "scheme://rest".
 */
static cell AMX_NATIVE_CALL n_err20001(AMX *amx, cell *params)
{
	static char s_url[1040];
	static char s_scheme[512];
	static char s_rest[512];
	string s = CellToString(amx, params[1]);
	const char *p = s.c_str();
	cell *dest;
	int i, n;

	memset(s_scheme, 0, 512);
	for (i = 0; i < 511; i++) {
		if (*p == 0 || *p == ':') {
			s_scheme[i] = *p++;
			break;
		}
		s_scheme[i] = *p++;
	}
	memset(s_rest, 0, 512);
	for (i = 0; i < 511; i++) {
		if (*p == 0) {
			s_rest[i] = 0;
			break;
		}
		s_rest[i] = *p++;
	}
	if (strcasecmp(s_scheme, "pfs:") == 0) {
		const char *q = s_rest;

		n = 0;
		for (i = 0; i < 511; i++) {  /* redundant as written */
			if (*q == '/') {
				if (n != 0)
					break;
				q++;
				n++;
			} else
				q++;
		}
		sprintf(s_url, "file://%s", s.c_str());
	} else
		sprintf(s_url, "%s//%s", s_scheme, s_rest);

	if (!amx_addr_ok(amx, params[2], strlen(s_url) + params[2])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[2], &dest);
	amx_SetString(dest, s_url, 1);
	return 0;
}

static cell AMX_NATIVE_CALL n_err20004(AMX *amx, cell *params)
{
	return bnCreateMovieTable();
}

/*
 * err20005(dir, path, num, buf): appends the file name of path (from its last
 * '/') to dir; with num > 0 turns it into "<base>_<num>.<ext>" (three-character
 * extension).
 */
static cell AMX_NATIVE_CALL n_err20005(AMX *amx, cell *params)
{
	static char s_dir[512];
	static char s_name[512];
	static char s_ext[8];
	string a = CellToString(amx, params[1]);
	string b = CellToString(amx, params[2]);
	int num = params[3];
	cell *dest;
	int i;

	memset(s_dir, 0, 512);
	strcpy(s_dir, b.c_str());
	memset(s_name, 0, 512);
	strcpy(s_name, a.c_str());
	for (i = strlen(s_dir); i >= 0; i--) {
		if (s_dir[i] == '/') {
			strcat(s_name, &s_dir[i]);
			break;
		}
	}
	if (num > 0) {
		strcpy(s_dir, s_name);
		s_dir[strlen(s_name) - 4] = 0;
		strcpy(s_ext, &s_name[strlen(s_name) - 3]);
		sprintf(s_name, "%s_%d.%s", s_dir, num, s_ext);
	}

	if (!amx_addr_ok(amx, params[4], strlen(s_name) + params[4])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[4], &dest);
	amx_SetString(dest, s_name, 1);
	return 0;
}

/* err20006(path, buf): the directory of path (everything before the last '/'). */
static cell AMX_NATIVE_CALL n_err20006(AMX *amx, cell *params)
{
	static char s_path[512];
	string s = CellToString(amx, params[1]);
	cell *dest;
	int i;

	memset(s_path, 0, 512);
	strcpy(s_path, s.c_str());
	for (i = strlen(s_path); i >= 0; i--) {
		if (s_path[i] == '/') {
			s_path[i] = 0;
			break;
		}
	}

	if (!amx_addr_ok(amx, params[2], strlen(s_path) + params[2])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[2], &dest);
	amx_SetString(dest, s_path, 1);
	return 0;
}

/* err20007(): clears the album name list. */
static cell AMX_NATIVE_CALL n_err20007(AMX *amx, cell *params)
{
	albumNameList = "";
	return 0;
}

/* err20008(name): appends a line to the album name list. */
static cell AMX_NATIVE_CALL n_err20008(AMX *amx, cell *params)
{
	string s = CellToString(amx, params[1]);

	s += "\n";
	albumNameList += s;
	return 0;
}

/* err20009(name): -1 when the list already contains this line, else 0. */
static cell AMX_NATIVE_CALL n_err20009(AMX *amx, cell *params)
{
	string s = CellToString(amx, params[1]);

	s += "\n";
	if (strstr(albumNameList.c_str(), s.c_str()) != 0)
		return -1;
	return 0;
}

static cell AMX_NATIVE_CALL n_err20002(AMX *amx, cell *params)
{
	return 0;
}
static cell AMX_NATIVE_CALL n_err20003(AMX *amx, cell *params)
{
	return 0;
}

AMX_NATIVE_INFO smallext_movie[] = {
	{ "err20001", n_err20001 },
	{ "err20002", n_err20002 },
	{ "err20003", n_err20003 },
	{ "err20004", n_err20004 },
	{ "err20005", n_err20005 },
	{ "err20006", n_err20006 },
	{ "err20007", n_err20007 },
	{ "err20008", n_err20008 },
	{ "err20009", n_err20009 },
	{ 0, 0 },
};
