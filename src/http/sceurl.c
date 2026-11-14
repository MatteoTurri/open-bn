/*
 * URL percent-encoding and decoding.
 */

#include "http/scehttp.h"
#include "sceurl.h"

static int url_escaped_len(const unsigned char *s);
static int url_escaped_len_n(const unsigned char *s, int n);

/* Output length of each byte: 1 = kept as is (A-Z a-z 0-9 * - . _),
 * 3 = percent-encoded. */
/* clang-format off */
static char g_urlCharClass[256] = {
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0x00 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0x10 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 3, 1, 1, 3,  /* 0x20 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3,  /* 0x30 */
    3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 0x40 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 1,  /* 0x50 */
    3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 0x60 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3,  /* 0x70 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0x80 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0x90 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0xA0 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0xB0 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0xC0 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0xD0 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0xE0 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  /* 0xF0 */
};
/* clang-format on */

/* Returns the length of s once percent-encoded. */
static int url_escaped_len(const unsigned char *s)
{
	int len = 0;

	if (*s != 0) {
		do {
			len += g_urlCharClass[*s];
			s++;
		} while (*s != 0);
	}
	return len;
}

/* Returns the length of the first n bytes of s once percent-encoded. */
static int url_escaped_len_n(const unsigned char *s, int n)
{
	int len = 0;
	int i = n - 1;

	if (n != 0) {
		do {
			len += g_urlCharClass[*s];
			s++;
			i--;
		} while (i != -1);
	}
	return len;
}

char *sceURLEscape(const char *str)
{
	static const char *hex = "0123456789ABCDEF";
	const unsigned char *s = (const unsigned char *)str;
	int len = url_escaped_len(s);
	char *buf = xmalloc(len + 1);
	char *out;

	if (!buf)
		return 0;

	out = buf;

	if (*s != 0) {
		do {
			if (g_urlCharClass[*s] == 3) {
				*out = '%';
				out[1] = hex[*s >> 4];
				out[2] = hex[*s & 0xF];
				out += 3;
			} else {
				*out = *s;
				out += 1;
			}
			s++;
		} while (*s != 0);
	}

	*out = 0;

	return buf;
}

char *sceURLLibEscapeN(const unsigned char *s, int n)
{
	static const char *hex = "0123456789ABCDEF";
	int len = url_escaped_len_n(s, n);
	char *buf = xmalloc(len + 1);
	char *out;

	n = n - 1;

	if (!buf)
		return 0;

	out = buf;

	if (n != -1) {
		do {
			if (g_urlCharClass[*s] == 3) {
				*out = '%';
				out[1] = hex[*s >> 4];
				out[2] = hex[*s & 0xF];
				out += 3;
			} else {
				*out = *s;
				out += 1;
			}
			s++;
			n--;
		} while (n != -1);
	}

	*out = 0;

	return buf;
}

static __inline__ int url_ishex(int c)
{
	int ok = 0;

	if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
		ok = 1;
	return ok;
}

/* Returns the decoded length of s, or -1 if a %XX sequence is incomplete or
 * a byte is not allowed unescaped. */
static int url_unescape_len(const unsigned char *s)
{
	int len = 0;

	while (*s != 0) {
		if (*s == '%') {
			if (s[1] == 0 || !url_ishex(s[1]) || s[2] == 0 || !url_ishex(s[2]))
				return -1;
			len += 3;
		} else {
			len++;
			if (*s != '+' && g_urlCharClass[*s] != 1)
				return -1;
		}
		s++;
	}
	return len;
}

/* Returns the value of a hex digit, -1 if c is not one. */
static __inline__ int url_hexval(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return -1;
}

/* Decodes the %XX sequences of s into a new buffer. Returns 0 if an escape
 * is malformed or allocation fails. */
char *sceURLUnescape(const char *escaped)
{
	const unsigned char *str = (const unsigned char *)escaped;
	int len;
	char *out;
	char *dst;
	unsigned char c;

	len = url_unescape_len(str);
	if (len < 0)
		return 0;

	out = xmalloc(len + 1);
	if (out == 0)
		return 0;

	dst = out;
	c = *str;
	if (c != 0) {
		do {
			if (c == '%') {
				*dst = (char)((url_hexval(str[1]) << 4) + url_hexval(str[2]));
				dst++;
				str += 3;
			} else {
				*dst++ = *str++;
			}
			c = *str;
		} while (c != 0);
	}
	*dst = 0;

	return out;
}
