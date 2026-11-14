/*
 * sceCcc*: libccc, the PS2 SDK character encoding converter (SJIS, JIS,
 * UCS2/UTF-16, UTF-8). g_jisToUcs/g_ucsToJis are the conversion tables set by
 * sceCccSetTable; unmapped characters become the per-encoding error character.
 */

#include "ccc.h"

static unsigned short g_errCharSJIS = 0;
static unsigned int g_errCharUTF8 = 0;
static unsigned int g_errCharUTF16 = 0;
static const unsigned short *g_jisToUcs = 0;
static const unsigned short *g_ucsToJis = 0;

static inline unsigned int ccc_decode_utf16(const unsigned short **s)
{
	const unsigned short *cur = *s;
	unsigned short high = *cur;
	unsigned short low;
	unsigned int r;
	unsigned int lim;

	cur++;
	if ((unsigned short)(high + 0x2800) < 0x400) {
		low = *cur;
		cur++;
		if ((unsigned short)(low + 0x2400) < 0x400) {
			r = ((unsigned int)high << 16) | low;
		} else {
			r = g_errCharUTF16;
		}
	} else {
		unsigned int d;

		r = g_errCharUTF16;
		d = (unsigned short)(high + 0x2400);
		lim = 1023;
		if (lim < d)
			r = high;
	}
	*s = cur;
	return r;
}

static inline unsigned int ccc_encode_utf16(unsigned short **s, unsigned int ucs)
{
	unsigned int u = ucs;
	unsigned short *cur = *s;
	int valid;
	unsigned int d;
	unsigned int lim;

	if (u > 0x10FFFF) {
		valid = 0;
		goto merge;
	}
	d = u - 0xD800;
	lim = 0x7FF;
	valid = lim < d;
merge:
	u = valid ? ucs : g_errCharUTF16;

	if (u <= 0xFFFF) {
		*cur = (unsigned short)u;
		cur++;
	} else {
		u -= 0x10000;
		*cur = (unsigned short)(0xD800 + (u >> 10));
		cur++;
		*cur = (unsigned short)(0xDC00 + (u & 0x3FF));
		cur++;
	}
	*s = cur;
	return u;
}

static inline unsigned int ccc_decode_utf8(const unsigned char **s)
{
	const unsigned char *cur = *s;
	unsigned char lead = *cur;
	unsigned int acc;
	unsigned int limit;

	cur++;
	if ((signed char)lead >= 0) {
		acc = lead;
		goto done;
	}
	if ((unsigned char)(lead + 62) >= 51) {
		acc = g_errCharUTF8;
		goto done;
	}
	acc = lead & 0x3f;
	limit = 0x800;
	acc = (acc << 6) | (*cur & 0x3f);
	cur++;
	if (acc >= limit) {
		do {
			acc &= limit - 1;
			limit <<= 5;
			acc = (acc << 6) | (*cur & 0x3f);
			cur++;
		} while (acc >= limit);
	}
done:
	*s = cur;
	return acc;
}

static inline void ccc_encode_utf8(unsigned char **s, unsigned int ucs)
{
	unsigned char *cur = *s;
	unsigned int u;

	u = ((int)ucs > -1) ? ucs : g_errCharUTF8;

	if (u < 128) {
		*cur = (unsigned char)u;
		cur++;
	} else {
		unsigned int limit = 2048;
		int extra = 1;
		unsigned char mask;
		unsigned char *end;

		while (u >= limit) {
			extra++;
			limit <<= 5;
		}
		cur += extra + 1;
		end = cur;
		mask = (1 << (7 - extra)) - 1;
		extra--;
		while (extra != -1) {
			end--;
			*end = (unsigned char)((u & 0x3f) | 0x80);
			u >>= 6;
			extra--;
		}
		*(end - 1) = (unsigned char)(~mask | (u & (mask >> 1)));
	}
	*s = cur;
}

/* SJIS -> UCS through g_jisToUcs; holes give the UTF-8 error character. */
static inline unsigned int ccc_sjis_to_ucs(int c)
{
	unsigned short v = g_jisToUcs[c];
	return v ? v : g_errCharUTF8;
}

int sceCccDecodeSJIS(const unsigned char **s)
{
	const unsigned char *cur = *s;
	unsigned char lead = *cur;
	unsigned char trail;
	int row;
	int col;
	int ext;
	unsigned int result;

	cur++;
	if ((signed char)lead >= 0)
		goto asis;
	if ((unsigned char)(lead + 96) < 64) {
		result = lead;
		goto out;
	}
	if (lead < 253)
		goto body;
asis:
	result = lead;
	goto out;
body:
	ext = 0;
	row = 0;
	col = 0;
	if ((unsigned char)(lead + 127) < 31) {
		row = ((lead - 129) << 1) + 1;
	} else if ((unsigned char)(lead + 32) < 16) {
		row = (lead << 1) - 385;
	} else if ((unsigned char)(lead + 16) < 13) {
		ext = 1;
		{
			int base = (lead - 240) << 1;
			int lo = lead < 244;
			row = base + 7;
			if (!lo)
				row = base + 69;
		}
	}
	if (row == 0)
		goto invalid;

	trail = *cur;
	cur++;
	if ((unsigned char)(trail - 64) < 63) {
		col = trail - 63;
	} else if ((unsigned char)(trail + 128) < 31) {
		col = trail - 64;
	} else if ((unsigned char)(trail + 97) < 94) {
		col = trail - 158;
		row++;
	}

	if (ext) {
		if (row == 7)
			row = 1;
		else if (row == 11)
			row = 5;
		else if (row == 0x4d)
			row = 15;
	}
	if (row == 0)
		goto invalid;
	if (col != 0)
		goto compose;
invalid:
	result = g_errCharSJIS;
	*s = cur;
	return result;
compose:
	result = ((unsigned int)ext << 15) | ((row + 32) << 8) | (col + 32);
	result &= 0xffff;
out:
	*s = cur;
	return result;
}

int sceCccDecodeSJIS_forceErrCode(const unsigned char **s, unsigned short errCode)
{
	const unsigned char *cur = *s;
	unsigned char lead = *cur;
	unsigned char trail;
	int row;
	int col;
	int ext;
	unsigned int result;

	cur++;
	if ((signed char)lead >= 0)
		goto asis;
	if ((unsigned char)(lead + 96) < 64) {
		result = lead;
		goto out;
	}
	if (lead < 253)
		goto body;
asis:
	result = lead;
	goto out;
body:
	ext = 0;
	row = 0;
	col = 0;
	if ((unsigned char)(lead + 127) < 31) {
		row = ((lead - 129) << 1) + 1;
	} else if ((unsigned char)(lead + 32) < 16) {
		row = (lead << 1) - 385;
	} else if ((unsigned char)(lead + 16) < 13) {
		ext = 1;
		{
			int base = (lead - 240) << 1;
			int lo = lead < 244;
			row = base + 7;
			if (!lo)
				row = base + 69;
		}
	}
	if (row == 0)
		goto invalid;

	trail = *cur;
	cur++;
	if ((unsigned char)(trail - 64) < 63) {
		col = trail - 63;
	} else if ((unsigned char)(trail + 128) < 31) {
		col = trail - 64;
	} else if ((unsigned char)(trail + 97) < 94) {
		col = trail - 158;
		row++;
	}

	if (ext) {
		if (row == 7)
			row = 1;
		else if (row == 11)
			row = 5;
		else if (row == 0x4d)
			row = 15;
	}
	if (row == 0)
		goto invalid;
	if (col != 0)
		goto compose;
invalid:
	result = errCode;
	goto out;
compose:
	result = ((unsigned int)ext << 15) | ((row + 32) << 8) | (col + 32);
	result &= 0xffff;
out:
	*s = cur;
	return result;
}

void sceCccEncodeSJIS(unsigned char **s, unsigned int c)
{
	unsigned char *cur;
	int valid;
	unsigned int jrow, row, col;
	unsigned int b;
	int ext;

	c &= 0xffff;
	cur = *s;
	if (c < 128)
		goto valid1;
	if ((unsigned short)(c - 160) >= 64)
		goto vcheck;
valid1:
	valid = 1;
	goto merge;
vcheck:
	{
		int topbit = (c >> 15) & 1;
		unsigned int vrow = (c >> 8) & 0x7f;
		unsigned int vcol = c & 0x7f;
		int v;

		if (vrow - 33 >= 94)
			goto inv;
		if (vcol < 33)
			goto inv;
		if (vcol >= 127)
			goto inv;
		if (topbit == 0)
			goto ok;
		if (vrow == 34 || vrow == 38 || vrow == 39)
			goto inv;
		if (vrow - 41 < 3)
			goto inv;
		if (vrow - 48 >= 62)
			goto ok;
inv:
		v = 0;
		goto vdone;
ok:
		v = 1;
vdone:
		valid = v;
	}
merge:
	c = valid ? c : g_errCharSJIS;

	if (c < 128) {
		*cur = (unsigned char)c;
		cur++;
		goto done;
	}
	if ((unsigned short)(c - 160) < 64) {
		*cur = (unsigned char)c;
		cur++;
		goto done;
	}

	ext = (c >> 15) & 1;
	jrow = (c >> 8) & 0x7f;
	row = jrow - 32;
	col = (c & 0x7f) - 32;

	if (ext)
		goto lext;
	{
		unsigned char *save = cur;

		cur++;
		if (row < 63)
			b = (jrow + 225) >> 1;
		else
			b = (jrow + 353) >> 1;
		*save = (unsigned char)b;
	}
	goto second;
lext:
	if (row < 16)
		b = ((jrow + 447) >> 1) - (row >> 3) * 3;
	else
		b = (jrow + 379) >> 1;
	*cur = (unsigned char)b;
	cur++;
second:
	if (!(row & 1)) {
		*cur = (unsigned char)(col + 158);
		cur++;
	} else {
		unsigned char *save = cur;
		cur++;
		*save = (unsigned char)(col < 64 ? col + 63 : col + 64);
	}
done:
	*s = cur;
}

int sceCccUTF8toUTF16(unsigned short *dst, int n, const unsigned char *src)
{
	int total = 0;
	unsigned short buf[8];
	unsigned int c;

	if ((unsigned int)n < 2)
		return 0;
	n -= 2;
	for (;;) {
		unsigned int u;
		unsigned short *p;
		int units;
		unsigned int bytes;

		c = ccc_decode_utf8(&src);
		if (c == 0)
			goto term;
		u = c;
		p = buf;
		ccc_encode_utf16(&p, u);
		units = p - buf;
		bytes = units * 2;
		if (n < bytes)
			goto term;
		n -= bytes;
		{
			unsigned short *q = buf;

			units--;
			while (units != -1) {
				*dst++ = *q++;
				units--;
			}
		}
		total += 1;
	}
term:
	*dst = 0;
	return total;
}

int sceCccUTF8toSJIS(unsigned char *dst, int n, const unsigned char *src)
{
	unsigned int c;
	int total = 0;
	unsigned char buf[16];

	if (n == 0)
		return 0;
	n--;
	for (;;) {
		unsigned int jis;
		unsigned short v;
		unsigned char *start;
		unsigned char *p;
		unsigned int len;

		c = ccc_decode_utf8(&src);
		if (c == 0)
			goto term;
		p = buf;
		jis = g_errCharSJIS;
		start = p;
		if (c > 0xFFFF)
			v = 0;
		else
			v = g_ucsToJis[c];
		if (v)
			jis = v;
		if (jis == 0)
			goto term;
		sceCccEncodeSJIS(&p, jis);
		len = p - start;
		if (n < len)
			goto term;
		n -= len;
		{
			unsigned char *q = start;

			len--;
			while (len != (unsigned int)-1) {
				*dst++ = *q++;
				len--;
			}
		}
		total += 1;
	}
term:
	*dst = 0;
	return total;
}

int sceCccSJIStoUTF8(unsigned char *dst, int n, const unsigned char *src)
{
	int total = 0;
	unsigned char buf[16];
	int c;

	if (n == 0)
		return 0;
	n--;

	while ((c = sceCccDecodeSJIS(&src)) != 0) {
		unsigned int ucs;
		unsigned char *p;
		unsigned int len;

		p = buf;
		ucs = ccc_sjis_to_ucs(c);
		if (ucs == 0)
			break;
		ccc_encode_utf8(&p, ucs);
		len = p - buf;
		if (n < len)
			break;
		n -= len;
		{
			unsigned char *q = buf;

			len--;
			while (len != (unsigned int)-1) {
				*dst++ = *q++;
				len--;
			}
		}
		total += 1;
	}
	*dst = 0;
	return total;
}

int sceCccSJIStoUTF16(unsigned short *dst, int n, const unsigned char *src)
{
	int total = 0;
	unsigned short buf[8];
	int c;

	if ((unsigned int)n < 2)
		return 0;
	n -= 2;

	while ((c = sceCccDecodeSJIS(&src)) != 0) {
		unsigned int ucs;
		unsigned short *p;
		int units;
		unsigned int bytes;
		unsigned short v;

		p = buf;
		v = g_jisToUcs[c];
		ucs = v;
		if (ucs == 0)
			ucs = g_errCharUTF16;
		ccc_encode_utf16(&p, ucs);
		units = p - buf;
		bytes = units * 2;
		if (n < bytes)
			goto term;
		n -= bytes;
		{
			unsigned short *q = buf;

			units--;
			while (units != -1) {
				*dst++ = *q++;
				units--;
			}
		}
		total += 1;
	}
term:
	*dst = 0;
	return total;
}

int sceCccUTF16toSJIS(unsigned char *dst, int n, const unsigned short *src)
{
	unsigned int c;
	int total = 0;
	unsigned char buf[16];

	if (n == 0)
		return 0;
	n--;

	while ((c = ccc_decode_utf16(&src)) != 0) {
		unsigned int jis;
		unsigned short v;
		unsigned char *start;
		unsigned char *p;
		unsigned int len;

		p = buf;
		jis = g_errCharSJIS;
		start = p;
		if (c > 0xFFFF)
			v = 0;
		else
			v = g_ucsToJis[c];
		if (v)
			jis = v;
		if (jis == 0)
			goto term;
		sceCccEncodeSJIS(&p, jis);
		len = p - start;
		if (n < len)
			goto term;
		n -= len;
		{
			unsigned char *q = start;

			len--;
			while (len != (unsigned int)-1) {
				*dst++ = *q++;
				len--;
			}
		}
		total += 1;
	}
term:
	*dst = 0;
	return total;
}

int sceCccUTF16toUTF8(unsigned char *dst, int n, const unsigned short *src)
{
	int total = 0;
	unsigned char buf[16];
	unsigned int c;

	if (n == 0)
		return 0;
	n--;

	while ((c = ccc_decode_utf16(&src)) != 0) {
		unsigned char *p;
		unsigned int len;

		p = buf;
		ccc_encode_utf8(&p, c);
		len = p - buf;
		if (n < len)
			goto term;
		n -= len;
		{
			unsigned char *q = buf;

			len--;
			while (len != (unsigned int)-1) {
				*dst++ = *q++;
				len--;
			}
		}
		total += 1;
	}
term:
	*dst = 0;
	return total;
}
void sceCccSetTable(void *jisToUcs, void *ucsToJis)
{
	g_jisToUcs = jisToUcs;
	g_ucsToJis = ucsToJis;
}

int sceCccStrlenUTF16(const unsigned short *s)
{
	int n = 0;
	while (sceCccDecodeUTF16(&s))
		n++;
	return n;
}

int sceCccStrlenUTF8(const unsigned char *s)
{
	int n = 0;
	while (sceCccDecodeUTF8(&s))
		n++;
	return n;
}

int sceCccStrlenSJIS(const unsigned char *s)
{
	int n = 0;
	while (sceCccDecodeSJIS(&s))
		n++;
	return n;
}

int sceCccIsValidUCS4(int c)
{
	if (c < 0)
		return 0;
	return 1;
}

int sceCccIsValidUCS2(unsigned int c)
{
	return c <= 0xFFFF;
}

int sceCccIsValidJIS(unsigned short code)
{
	int topbit = code >> 15;
	int row = (code >> 8) & 0x7f;
	unsigned int col = code & 0x7f;

	if ((unsigned int)(row - 33) >= 94)
		goto fail;
	if (col < 33)
		goto fail;
	if (col < 127)
		goto continue_check;
fail:
	return 0;
continue_check:
	if (topbit == 0)
		goto ok;
	if (row == 34 || row == 38 || row == 39)
		goto fail;
	if ((unsigned int)(row - 41) < 3)
		goto fail;
	if ((unsigned int)(row - 48) < 62)
		goto fail;
ok:
	return 1;
}

int sceCccIsValidUTF8(int c)
{
	if (c > -1)
		return 1;
	return 0;
}

int sceCccIsValidUTF16(unsigned int c)
{
	if (c > 0x10FFFF)
		return 0;
	return !((unsigned int)(c - 0xD800) < 0x800);
}

int sceCccIsValidSJIS(unsigned short code)
{
	int topbit;
	int row;
	unsigned int col;

	if (code < 128)
		goto ret1;
	if ((unsigned short)(code - 160) >= 64)
		goto mainb;
ret1:
	return 1;
mainb:
	topbit = code >> 15;
	row = (code >> 8) & 0x7f;
	col = code & 0x7f;
	if ((unsigned int)(row - 33) >= 94)
		goto fail;
	if (col < 33)
		goto fail;
	if (col < 127)
		goto continue_check;
fail:
	return 0;
continue_check:
	if (topbit == 0)
		goto ok;
	if (row == 34 || row == 38 || row == 39)
		goto fail;
	if ((unsigned int)(row - 41) < 3)
		goto fail;
	if ((unsigned int)(row - 48) < 62)
		goto fail;
ok:
	return 1;
}

int sceCccDecodeUTF16(const unsigned short **s)
{
	const unsigned short *cur = *s;
	unsigned short high = *cur;
	unsigned short low;
	unsigned int r;
	int is_high;
	unsigned int lim;

	is_high = (unsigned short)(high + 0x2800) < 0x400;
	cur++;
	if (is_high) {
		low = *cur;
		cur++;
		if ((unsigned short)(low + 0x2400) < 0x400) {
			r = ((unsigned int)high << 16) | low;
		} else {
			r = g_errCharUTF16;
		}
	} else {
		unsigned int d;

		r = g_errCharUTF16;
		d = (unsigned short)(high + 0x2400);
		lim = 1023;
		if (lim < d)
			r = high;
	}
	*s = cur;
	return r;
}

int sceCccDecodeUTF8(const unsigned char **s)
{
	const unsigned char *cur = *s;
	unsigned char lead = *cur;
	unsigned int acc;
	unsigned int limit;

	cur++;
	if ((signed char)lead >= 0) {
		acc = lead;
		goto done;
	}
	if ((unsigned char)(lead + 62) >= 51) {
		acc = g_errCharUTF8;
		goto done;
	}
	acc = lead & 0x3f;
	limit = 0x800;
	acc = (acc << 6) | (*cur & 0x3f);
	cur++;
	if (acc >= limit) {
		do {
			acc &= limit - 1;
			limit <<= 5;
			acc = (acc << 6) | (*cur & 0x3f);
			cur++;
		} while (acc >= limit);
	}
done:
	*s = cur;
	return acc;
}

void sceCccEncodeUTF16(unsigned short **s, unsigned int ucs)
{
	unsigned short *cur = *s;
	int valid;
	unsigned int d;
	unsigned int lim;

	if (ucs > 0x10FFFF) {
		valid = 0;
		goto merge;
	}
	d = ucs - 0xD800;
	lim = 0x7FF;
	valid = lim < d;
merge:
	if (!valid)
		ucs = g_errCharUTF16;

	if (ucs <= 0xFFFF) {
		*cur = (unsigned short)ucs;
		cur++;
	} else {
		ucs -= 0x10000;
		*cur = (unsigned short)(0xD800 + (ucs >> 10));
		cur++;
		*cur = (unsigned short)(0xDC00 + (ucs & 0x3FF));
		cur++;
	}
	*s = cur;
}

void sceCccEncodeUTF8(unsigned char **s, unsigned int ucs)
{
	unsigned char *cur = *s;
	unsigned int u;

	u = ((int)ucs > -1) ? ucs : g_errCharUTF8;

	if (u < 128) {
		*cur = (unsigned char)u;
		cur++;
	} else {
		unsigned int limit = 2048;
		int extra = 1;
		unsigned char mask;
		unsigned char *end;

		while (u >= limit) {
			extra++;
			limit <<= 5;
		}
		cur += extra + 1;
		end = cur;
		mask = (1 << (7 - extra)) - 1;
		extra--;
		while (extra != -1) {
			end--;
			*end = (unsigned char)((u & 0x3f) | 0x80);
			u >>= 6;
			extra--;
		}
		*(end - 1) = (unsigned char)(~mask | (u & (mask >> 1)));
	}
	*s = cur;
}

unsigned int sceCccUCStoJIS(unsigned int ucs, unsigned short def)
{
	unsigned short v;

	if (ucs > 0xFFFF)
		v = 0;
	else
		v = g_ucsToJis[ucs];
	return v ? v : def;
}

unsigned int sceCccJIStoUCS(unsigned int code, unsigned int def)
{
	unsigned short v = g_jisToUcs[(unsigned short)code];
	return v ? v : def;
}

unsigned short sceCccSetErrorCharSJIS(unsigned short c)
{
	unsigned short old = g_errCharSJIS;
	g_errCharSJIS = c;
	return old;
}

unsigned int sceCccSetErrorCharUTF8(unsigned int c)
{
	unsigned int old = g_errCharUTF8;
	g_errCharUTF8 = c;
	return old;
}

unsigned int sceCccSetErrorCharUTF16(unsigned int c)
{
	unsigned int old = g_errCharUTF16;
	g_errCharUTF16 = c;
	return old;
}
