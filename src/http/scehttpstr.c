/*
 * String helpers of the HTTP library (allocating copies and concatenation,
 * prefix comparison, decimal parsing).
 */

#include <ctype.h>
#include <string.h>

#include "http/scehttp.h"
#include <stdarg.h>

/* strdup with xmalloc; 0 for a NULL s. */
char *sceHTTPLibStrsave(const char *s)
{
	char *buf;

	if (s == 0)
		return 0;
	buf = xmalloc(strlen(s) + 1);
	if (buf == 0)
		return 0;
	strcpy(buf, s);
	return buf;
}

/* Returns a copy of at most n bytes of s, NUL-terminated. */
char *sceHTTPLibStrnsave(const char *s, unsigned int n)
{
	unsigned int len;
	char *buf;

	if (s == 0)
		return 0;

	len = strlen(s);
	if (len < n)
		n = len;

	buf = xmalloc(n + 1);
	if (buf == 0)
		return 0;

	if (n != 0)
		strncpy(buf, s, n);

	buf[n] = 0;
	return buf;
}

/* Appends s1 to s0, reallocating s0 (or copying s1 if s0 is NULL).
 * Returns s0 unchanged if s1 is NULL, empty or s0 itself, 0 on failure. */
char *sceHTTPLibStrcatsave(char *s0, const char *s1)
{
	if (s1 == 0 || *s1 == 0 || s1 == s0)
		return s0;

	if (s0 != 0) {
		unsigned int len0 = strlen(s0);
		unsigned int len1 = strlen(s1);
		char *buf = xrealloc(s0, len0 + len1 + 1);

		if (buf == 0)
			return 0;
		strcpy(buf + len0, s1);
		s0 = buf;
	} else {
		s0 = xmalloc(strlen(s1) + 1);
		if (s0 == 0)
			return 0;
		strcpy(s0, s1);
	}
	return s0;
}

/* Returns a new string made of s followed by the NULL-terminated list of
 * arguments, and frees s. */
char *sceHTTPLibVStrcat(char *s, ...)
{
	va_list ap;
	char *first;
	char *acc;
	char *r;
	char *q;

	r = s;
	va_start(ap, s);
	first = r;
	if (s == 0) {
		acc = 0;
	} else {
		acc = sceHTTPLibStrsave(s);
		if (acc == 0)
			return 0;
	}
	while ((q = va_arg(ap, char *)) != 0) {
		r = sceHTTPLibStrcatsave(acc, q);
		if (r == 0) {
			xfree(acc);
			break;
		}
		acc = r;
	}
	va_end(ap);
	if (r != 0 && first != 0)
		xfree(first);  /* NOTE: bug: with no varargs this frees and returns s */
	return r;
}

int sceHTTPLibStrcasecmp(const char *s1, const char *s2)
{
	const char *p1 = s1;
	const char *p2 = s2;
	char c1, c2;
	int diff;

	for (;;) {
		c1 = *p1;
		c2 = *p2;
		diff = __ctype_toupper[c1] - __ctype_toupper[c2];
		if (diff != 0)
			return diff;
		if (c1 == 0)
			return 0;
		p1++;
		p2++;
	}
}

/* Compares s1 with the prefix s2, ignoring case; 0 if s1 starts with s2. */
int sceHTTPLibStrcasesub(const char *s1, const char *s2)
{
	unsigned int len;
	int diff;

	if (s2 == 0)
		return 0;
	len = strlen(s2);
	while (len != 0) {
		diff = __ctype_toupper[*s1] - __ctype_toupper[*s2];
		if (diff != 0)
			return diff;
		s1++;
		len--;
		s2++;
	}
	return 0;
}

/* Compares s1 with the prefix s2; 0 if s1 starts with s2. */
int sceHTTPLibStrsub(const char *s1, const char *s2)
{
	unsigned int len;
	int diff;

	if (s2 == 0)
		return 0;
	len = strlen(s2);
	while (len != 0) {
		diff = *s1 - *s2;
		if (diff != 0)
			return diff;
		s1++;
		len--;
		s2++;
	}
	return 0;
}

/* Parses a decimal number into a 64-bit value and stores the end in *end. */
long long sceHTTPLibStrtodi(const char *p, const char **end)
{
	long long val = 0;

	if (p == 0)
		goto out;
	while (isdigit(*p)) {
		val = val * 10 + (*p - '0');
		p++;
	}
out:
	*end = p;
	return val;
}
