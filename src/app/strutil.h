/*
 * Inline string helpers (trim, mid, find) used by the native-function
 * modules.
 */

#ifndef STRUTIL_H
#define STRUTIL_H

#include "app/stringutil.h"

static inline StringUtil StrMid(const string &s, int pos, int n)
{
	return s.substr(pos, n);
}

static inline void StrMakeLower(string &t)
{
	for (unsigned int i = 0; i < t.length(); i++) {
		if (t[i] >= 'A' && t[i] <= 'Z')
			t[i] += 32;
	}
}

static inline void StrTrimLeft(string &t, const char *ws = " \t\r\n")
{
	int pos = t.find_first_not_of(ws);
	if (pos < 0) {
		t = "";
	} else {
		t = StrMid(t, pos, t.length() - pos);
	}
}

static inline void StrTrimRight(string &t, const char *ws = " \t\r\n")
{
	int pos = t.find_last_not_of(ws);
	if (pos < 0) {
		t = "";
	} else {
		t = StrMid(t, 0, pos + 1);
	}
}

static inline void StrDelete(string &t, int pos, int n = 1)
{
	t.erase(pos, n);
}

static inline int StrFind(const string &t, const char *s, int pos = 0)
{
	return t.find(s, pos);
}

static inline int StrFind(const string &t, char c, int pos = 0)
{
	return t.find(c, pos);
}

static inline int StrFindOneOf(const string &t, const char *s)
{
	return t.find_first_of(s);
}

#endif
