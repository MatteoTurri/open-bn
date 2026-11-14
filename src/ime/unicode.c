/*
 * IME charset conversions (EUC-JP, UCS2, UTF8, SJIS) through cached iconv
 * handles; the result is allocated with TextIM_arg.alloc.
 */

#include <iconv.h>
#include <string.h>

#include "lib/ccc.h"

#include "textim_debug.h"
#include "textim_arg.h"
#include "unicode.h"

/* Cached iconv handles; -1 means not open. g_cd2 and g_cd3 are reset and
   closed, but no conversion in this file uses them. */
static iconv_t g_cd0 = (iconv_t)-1, g_cd1 = (iconv_t)-1, g_cd2 = (iconv_t)-1, g_cd3 = (iconv_t)-1, g_cd4 = (iconv_t)-1,
               g_cd5 = (iconv_t)-1;

static const char g_csUcs2[] = "UCS2";
static const char g_csEucJp[] = "EUC-JP";
static const char g_csUtf8[] = "UTF8";

int unicode_strlen(unsigned short *s)
{
	int n = 0;
	if (*s != 0) {
		do {
			s++;
			n++;
		} while (*s != 0);
	}
	return n;
}

int unicode_init(void)
{
	g_cd0 = (iconv_t)-1;
	g_cd1 = (iconv_t)-1;
	g_cd2 = (iconv_t)-1;
	g_cd3 = (iconv_t)-1;
	g_cd4 = (iconv_t)-1;
	g_cd5 = (iconv_t)-1;
	return 1;
}

void unicode_shutdown(void)
{
	if (g_cd0 != (iconv_t)-1) {
		iconv_close(g_cd0);
		g_cd0 = (iconv_t)-1;
	}
	if (g_cd1 != (iconv_t)-1) {
		iconv_close(g_cd1);
		g_cd1 = (iconv_t)-1;
	}
	if (g_cd2 != (iconv_t)-1) {
		iconv_close(g_cd2);
		g_cd2 = (iconv_t)-1;
	}
	if (g_cd3 != (iconv_t)-1) {
		iconv_close(g_cd3);
		g_cd3 = (iconv_t)-1;
	}
	if (g_cd4 != (iconv_t)-1) {
		iconv_close(g_cd4);
		g_cd4 = (iconv_t)-1;
	}
	if (g_cd5 != (iconv_t)-1) {
		iconv_close(g_cd5);
		g_cd5 = (iconv_t)-1;
	}
}

void *unicode_euc2uni(const char *in, int inlen)
{
	char buf[4096];
	char *outp;
	unsigned int outleft;
	iconv_t old;

	old = g_cd0;
	if (old == (iconv_t)-1) {
		g_cd0 = iconv_open(g_csUcs2, g_csEucJp);
		if (g_cd0 == old)
			return 0;
	}
	outp = buf;
	outleft = sizeof(buf) - 2;
	iconv(g_cd0, 0, 0, 0, 0);
	iconv(g_cd0, (char **)&in, (unsigned int *)&inlen, &outp, &outleft);
	*(unsigned short *)outp = 0;
	outleft = (sizeof(buf) - 2) - outleft;
	outleft += 2;
	outp = (char *)TextIM_arg.alloc(outleft);
	if (!outp)
		return 0;
	memcpy(outp, buf, outleft);
	return outp;
}

void *unicode_uni2euc(const unsigned short *in, int inlen)
{
	char buf[2048];
	unsigned int inleft;
	char *outp;
	unsigned int outleft;
	iconv_t old;

	old = g_cd1;
	if (old == (iconv_t)-1) {
		g_cd1 = iconv_open(g_csEucJp, g_csUcs2);
		if (g_cd1 == old)
			return 0;
	}
	inleft = inlen * 2;
	outp = buf;
	outleft = sizeof(buf) - 1;
	iconv(g_cd1, 0, 0, 0, 0);
	iconv(g_cd1, (char **)&in, &inleft, &outp, &outleft);
	*outp = 0;
	outleft = (sizeof(buf) - 1) - outleft;
	outleft += 1;
	outp = (char *)TextIM_arg.alloc(outleft);
	if (!outp)
		return 0;
	memcpy(outp, buf, outleft);
	return outp;
}

void *unicode_sjis2uni(char *s, int len)
{
	unsigned short buf[2048];
	unsigned int count;
	int c, w;
	void *dst;
	unsigned int size;

	count = 0;
	while ((c = sceCccDecodeSJIS_forceErrCode((const unsigned char **)&s, 0)) != 0 && count < 2047
	    && (w = sceCccJIStoUCS(c, 0)) != 0) {
		buf[count] = (unsigned short)w;
		count++;
	}
	buf[count] = 0;
	count++;
	size = count * 2;
	dst = TextIM_arg.alloc(size);
	if (dst == 0)
		return 0;
	memcpy(dst, buf, size);
	return dst;
}

void *unicode_euc2utf8(const char *in, int inlen)
{
	char buf[2048];
	char *outp;
	unsigned int outleft;
	iconv_t old;

	old = g_cd4;
	if (old == (iconv_t)-1) {
		g_cd4 = iconv_open(g_csUtf8, g_csEucJp);
		if (g_cd4 == old)
			return 0;
	}
	outp = buf;
	outleft = sizeof(buf) - 1;
	iconv(g_cd4, 0, 0, 0, 0);
	iconv(g_cd4, (char **)&in, (unsigned int *)&inlen, &outp, &outleft);
	*outp = 0;
	outleft = (sizeof(buf) - 1) - outleft;
	outp = (char *)TextIM_arg.alloc(outleft + 1);
	if (!outp)
		return 0;
	memcpy(outp, buf, outleft + 1);
	return outp;
}

void *unicode_uni2utf8(const unsigned short *in, int inbytes)
{
	char buf[2048];
	char *outp;
	unsigned int outleft;
	unsigned int size;
	iconv_t old;

	old = g_cd5;
	if (old == (iconv_t)-1) {
		g_cd5 = iconv_open(g_csUtf8, g_csUcs2);
		if (g_cd5 == old)
			return 0;
	}
	outp = buf;
	outleft = sizeof(buf) - 1;
	iconv(g_cd5, 0, 0, 0, 0);
	iconv(g_cd5, (char **)&in, (unsigned int *)&inbytes, &outp, &outleft);
	*outp = 0;
	outleft = (sizeof(buf) - 1) - outleft;
	size = outleft + 4;
	outleft += 2;
	outp = (char *)TextIM_arg.alloc(size);
	if (!outp)
		return 0;
	memcpy(outp, buf, outleft + 2);
	return outp;
}
