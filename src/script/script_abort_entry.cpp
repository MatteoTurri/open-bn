/*
 * Script abort entry points: each decodes an obfuscated string (ROT13, then
 * base64) and hands it to DNAS. Error handling is in script_error.cpp.
 */

#include "dnas/dnas2m61_verify.h"
#include "script_abort_entry.h"

extern "C" {

/* Decodes base64 in place; stores the decoded length in *out and returns the
   number of characters consumed. Stops at the first character outside the
   alphabet. */
static int scriptResolve(char *buf, int *out)
{
	char *src = buf;
	char *dst = buf;
	int acc = 0;
	int bits = 0;
	int c, v;

	for (;;) {
		c = *src;
		if (c >= 'A' && c <= 'Z')
			v = c - 'A';
		else if (c >= 'a' && c <= 'z')
			v = c - 'a' + 26;
		else if (c >= '0' && c <= '9')
			v = c - '0' + 52;
		else if (c == '+')
			v = 62;
		else if (c == '/')
			v = 63;
		else
			v = -1;
		if (v < 0)
			break;
		if (bits)
			*dst++ = (acc << bits) | (v >> (6 - bits));
		acc = v;
		src++;
		bits = (bits + 2) & 7;
	}
	*out = dst - buf;
	return src - buf;
}

/* ROT13 on letters and swap of the base64 specials with their stand-ins
   ('+' <-> '&', '/' <-> '#', '=' <-> '~'), in place. */
static void scriptPrepare(char *buf)
{
	int i;
	for (i = 0; buf[i]; i++) {
		if (buf[i] >= 'a' && buf[i] <= 'z')
			buf[i] = (buf[i] - 'a' + 13) % 26 + 'a';
		else if (buf[i] >= 'A' && buf[i] <= 'Z')
			buf[i] = (buf[i] - 'A' + 13) % 26 + 'A';
		else if (buf[i] == '+')
			buf[i] = '&';
		else if (buf[i] == '/')
			buf[i] = '#';
		else if (buf[i] == '=')
			buf[i] = '~';
		else if (buf[i] == '&')
			buf[i] = '+';
		else if (buf[i] == '#')
			buf[i] = '/';
		else if (buf[i] == '~')
			buf[i] = '=';
	}
}

/* Decodes the embedded key and passes it, with its length, to DNAS, which
   decodes buf (size bytes) in place and stores the result length in *outsize;
   Abort2 and Abort3 differ only in the key. */
int Script__Abort(int size, void *buf, int *outsize)
{
	char key[17] = "C2R8LQOzCIA3Xt~~";
	int out;

	scriptPrepare(key);
	scriptResolve(key, &out);
	return __sce_dnas2_symbol61003(size, (unsigned char *)buf, out, (unsigned char *)key, (unsigned int *)outsize);
}

int Script__Abort2(int size, void *buf, int *outsize)
{
	char key[21] = "nwS9owZ0BUMhnzgfKGL~";
	int out;

	scriptPrepare(key);
	scriptResolve(key, &out);
	return __sce_dnas2_symbol61003(size, (unsigned char *)buf, out, (unsigned char *)key, (unsigned int *)outsize);
}

int Script__Abort3(int size, void *buf, int *outsize)
{
	char key[17] = "MmSZn04mnJpbLwR~";
	int out;

	scriptPrepare(key);
	scriptResolve(key, &out);
	return __sce_dnas2_symbol61003(size, (unsigned char *)buf, out, (unsigned char *)key, (unsigned int *)outsize);
}
}
