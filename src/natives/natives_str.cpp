/*
 * Small/amx natives for strings (table smallext_string): sprintf, snprintf,
 * atoi, atof, strcmp, strncmp, and the format engine behind the first two.
 */

#include <string>

#include <stdio.h>
#include <stdlib.h>

#include "amx.h"
#include "natives/smallext.h"
#include "script/scenewindow.h"
#include "script/script.h"
#include "bntrace.h"

/* Width and precision being collected after a '%'. */
static string s_spec;
/* Nonzero while a format specification is being collected. */
static int s_inSpec;

static int amxFormat(string &out, AMX *amx, const cell *fmt, const cell *args, int nargs);

/*
 * Handles one character of a format specification. Digits and '.' are
 * collected in s_spec (returns 0); a conversion character (c, d, x, f, s)
 * prints its argument and returns 1, the number of arguments consumed.
 * Numeric conversions build the libc format "%" + s_spec + conversion.
 */
static int amxFormatSpec(string &out, AMX *amx, char c, cell arg)
{
	cell *p;

	if ((c >= '0' && c <= '9') || c == '.') {
		s_spec += c;
		return 0;
	}

	switch (c) {
	case 'c':
		amx_GetAddr(amx, arg, &p);
		out += (char)*p;
		s_inSpec = 0;
		return 1;
	case 'd': {
		amx_GetAddr(amx, arg, &p);
		BN_TRACE(32, *p);
		string fmt = "%" + s_spec + "d";
		char tag[16];
		BN_TRACE_FMT(tag, fmt);  /* unused, but part of the layout */
		char buf[48];
		sprintf(buf, fmt.c_str(), *p);
		out += buf;
		s_inSpec = 0;
		return 1;
	}
	case 'x': {
		amx_GetAddr(amx, arg, &p);
		string fmt = "%" + BN_TRACE(64, s_spec) + "x";
		char tag[16];
		BN_TRACE_FMT(tag, fmt);  /* unused, but part of the layout */
		char buf[16];
		sprintf(buf, fmt.c_str(), *p);
		out += buf;
		s_inSpec = 0;
		BN_ASSERT(BN_TRACE(64, buf)[0] != 0);
		return 1;
	}
	case 'f': {
		amx_GetAddr(amx, arg, &p);
		string fmt = "%" + BN_TRACE(32, s_spec) + "f";
		char buf[32];
		sprintf(buf, fmt.c_str(), amx_ctof(*p));
		out += buf;
		s_inSpec = 0;
		return 1;
	}
	case 's':
		amx_GetAddr(amx, arg, &p);
		amxFormat(out, amx, p, 0, 0);
		s_inSpec = 0;
		return 1;
	case '%':
		out += c;
		s_inSpec = 0;
		return 0;
	default:
		out += c;
		s_inSpec = 0;
		return 0;
	}
}

/*
 * Format engine of sprintf and snprintf: appends fmt to out, expanding the
 * % specifications from args (nargs cells). fmt is a script string, packed
 * (four characters per cell, first in the most significant byte; first cell
 * >= 256) or unpacked (one character per cell). Returns the number of
 * arguments consumed.
 */
static int amxFormat(string &out, AMX *amx, const cell *fmt, const cell *args, int nargs)
{
	int n = 0;

	s_inSpec = 0;
	if ((ucell)*fmt >= 256) {
		int sh = 3, i = 0;
		char c;

		for (; (c = (char)((ucell)fmt[i] >> (sh * 8))) != '\0';) {
			if (s_inSpec) {
				n += amxFormatSpec(out, amx, c, args[n]);
			} else if (args != 0 && c == '%') {
				if (n >= nargs)
					return 0;
				s_inSpec = 1;
				s_spec = "";
			} else {
				out += c;
			}
			if (sh == 0)
				i++;
			sh = (sh + 3) & 3;
		}
	} else {
		int j;

		for (j = 0; fmt[j] != 0; j++) {
			if (s_inSpec) {
				n += amxFormatSpec(out, amx, (char)fmt[j], args[n]);
			} else if (args != 0 && fmt[j] == '%') {
				if (n >= nargs)
					return 0;
				s_inSpec = 1;
				s_spec = "";
			} else {
				out += (char)fmt[j];
			}
		}
	}
	return n;
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

/* sprintf(dest[], const format[], ...) */
static cell AMX_NATIVE_CALL n_sprintf(AMX *amx, cell *params)
{
	string out;
	cell *fmt;
	cell *dest;

	amx_GetAddr(amx, params[2], &fmt);
	amxFormat(out, amx, fmt, &params[3], ((ucell)params[0] >> 2) - 2);

	if (!amx_addr_ok(amx, params[1], out.length() + params[1])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[1], &dest);
	amx_SetString(dest, (char *)out.c_str(), 1);
	return 0;
}

/*
 * snprintf(dest[], maxlength, const format[], ...): as sprintf, truncated to
 * maxlength cells (maxlength*4-1 characters).
 */
static cell AMX_NATIVE_CALL n_snprintf(AMX *amx, cell *params)
{
	string out;
	cell *fmt;
	cell *dest;

	amx_GetAddr(amx, params[3], &fmt);
	amxFormat(out, amx, fmt, &params[4], ((ucell)params[0] >> 2) - 3);

	if (params[2] * 4 < out.length() + 1)
		out = out.substr(0, params[2] * 4 - 1);

	if (!amx_addr_ok(amx, params[1], out.length() + params[1])) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[1], &dest);
	amx_SetString(dest, (char *)out.c_str(), 1);
	return 0;
}

static cell AMX_NATIVE_CALL n_atoi(AMX *amx, cell *params)
{
	string s = CellToString(amx, params[1]);

	return atoi(s.c_str());
}

/* atof: returns the float bits in the cell. */
static cell AMX_NATIVE_CALL n_atof(AMX *amx, cell *params)
{
	string s = CellToString(amx, params[1]);
	float f = (float)atof(s.c_str());

	return amx_ftoc(f);
}

/*
 * strcmp on packed script strings (four characters per cell, first in the
 * most significant byte). A bad first address returns -1, a bad second one 1.
 */
static cell AMX_NATIVE_CALL n_strcmp(AMX *amx, cell *params)
{
	cell *s1, *s2;
	int i;

	if (amx_GetAddr(amx, params[1], &s1) != 0)
		return -1;
	if (amx_GetAddr(amx, params[2], &s2) != 0)
		return 1;

	for (i = 0;; i++) {
		cell c = s1[i];

		if (((c & 0x000000ff) == 0 && c == s2[i])
		    || ((c & 0x0000ff00) == 0 && (ucell)(c & 0xffffff00) == (ucell)(s2[i] & 0xffffff00))
		    || ((c & 0x00ff0000) == 0 && (ucell)(c & 0xffff0000) == (ucell)(s2[i] & 0xffff0000))
		    || ((c & 0xff000000) == 0 && (s2[i] & 0xff000000) == 0))
			return 0;
		if ((ucell)s1[i] > (ucell)s2[i])
			return 1;
		if ((ucell)s1[i] < (ucell)s2[i])
			return -1;
	}
}

/*
 * strncmp: as strcmp on the first params[3] characters (whole cells, then a
 * mask for the remaining 1..3 characters).
 */
static cell AMX_NATIVE_CALL n_strncmp(AMX *amx, cell *params)
{
	cell *s1, *s2;
	cell mask;
	int cells, rem, i;

	if (amx_GetAddr(amx, params[1], &s1) != 0)
		return -1;
	if (amx_GetAddr(amx, params[2], &s2) != 0)
		return 1;

	cells = (ucell)params[3] >> 2;
	rem = params[3] & 3;

	for (i = 0; i < cells; i++) {
		cell c = s1[i];

		if (((c & 0x000000ff) == 0 && c == s2[i])
		    || ((c & 0x0000ff00) == 0 && (ucell)(c & 0xffffff00) == (ucell)(s2[i] & 0xffffff00))
		    || ((c & 0x00ff0000) == 0 && (ucell)(c & 0xffff0000) == (ucell)(s2[i] & 0xffff0000))
		    || ((c & 0xff000000) == 0 && (s2[i] & 0xff000000) == 0))
			return 0;
		if ((ucell)s1[i] > (ucell)s2[i])
			return 1;
		if ((ucell)s1[i] < (ucell)s2[i])
			return -1;
	}

	mask = 0;
	switch (rem) {
	case 0:
		return 0;
	case 1:
		mask = 0xff000000;
		break;
	case 2:
		mask = 0xffff0000;
		break;
	case 3:
		mask = 0xffffff00;
		break;
	}
	{
		ucell m1 = s1[i] & mask;
		ucell m2 = s2[i] & mask;

		if (m1 == m2)
			return 0;
		if (m1 > m2)
			return 1;
		return -1;
	}
}

AMX_NATIVE_INFO smallext_string[] = {
	{ "sprintf", n_sprintf },
	{ "snprintf", n_snprintf },
	{ "atoi", n_atoi },
	{ "atof", n_atof },
	{ "strcmp", n_strcmp },
	{ "strncmp", n_strncmp },
	{ 0, 0 },
};
