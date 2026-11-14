/*
 * BASE64 encoder and line decoder of the HTTP library.
 */

#include "scebase64.h"

static const char *g_base64Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int sceBASE64Encoder(unsigned char *src, char *dst, int len)
{
	char *dstStart = dst;
	int chunk = 0;
	int one;

	while (len >= 3) {
		chunk = (src[0] << 16) | (src[1] << 8) | src[2];

		dst[0] = g_base64Table[(unsigned int)chunk >> 18];
		dst[1] = g_base64Table[(chunk >> 12) & 0x3f];
		dst[2] = g_base64Table[(chunk >> 6) & 0x3f];
		src += 3;
		len -= 3;
		dst[3] = g_base64Table[chunk & 0x3f];
		dst += 4;
	}

	one = len < 2;
	if (len > 0)
		chunk = src[0] << 16;
	if (!one)
		chunk |= src[1] << 8;
	if (len > 0) {
		dst[0] = g_base64Table[(chunk >> 18) & 0x3f];
		dst[1] = g_base64Table[(chunk >> 12) & 0x3f];
		if (!one)
			dst[2] = g_base64Table[(chunk >> 6) & 0x3f];
		else
			dst[2] = '=';
		dst[3] = '=';
		dst += 4;
	}

	return dst - dstStart;
}

/* Inverse alphabet: the 6-bit value of each byte, -1 if not BASE64. */
/* clang-format off */
static signed char g_base64DecodeTable[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
    -1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
};
/* clang-format on */

/* Decodes one BASE64 line, 4 characters to 3 bytes, with '=' padding in the
 * last group. Returns the decoded length, or -1 on an invalid character or a
 * length that is not a multiple of 4. */
int sceBASE64LineDecoder(const char *src, char *dst, int len)
{
	const unsigned char *p = (const unsigned char *)src;
	int outLen;
	int chunk;
	int b;

	outLen = 0;
	if (len > 76)
		len = 76;

	while (len >= 5) {
		b = g_base64DecodeTable[p[0]];
		if (b == -1)
			return -1;
		chunk = b << 18;
		b = g_base64DecodeTable[p[1]];
		if (b == -1)
			return -1;
		chunk |= b << 12;
		b = g_base64DecodeTable[p[2]];
		if (b == -1)
			return -1;
		chunk |= b << 6;
		b = g_base64DecodeTable[p[3]];
		if (b == -1)
			return -1;
		chunk |= b;

		p += 4;
		len -= 4;

		dst[0] = (char)(chunk >> 16);
		dst[1] = (char)(chunk >> 8);
		dst[2] = (char)chunk;
		dst += 3;

		outLen += 3;
	}

	if (len != 4)
		return -1;

	b = g_base64DecodeTable[p[0]];
	if (b == -1)
		return -1;
	chunk = b << 18;
	b = g_base64DecodeTable[p[1]];
	if (b == -1)
		return -1;
	chunk |= b << 12;

	if (p[2] == '=') {
		if (p[3] != '=')
			return -1;
		outLen += 1;
	} else {
		b = g_base64DecodeTable[p[2]];
		if (b == -1)
			return -1;
		chunk |= b << 6;
		if (p[3] != '=') {
			b = g_base64DecodeTable[p[3]];
			if (b == -1)
				return -1;
			chunk |= b;
			outLen += 3;
		} else {
			outLen += 2;
		}
	}

	dst[0] = (char)(chunk >> 16);
	dst[1] = (char)(chunk >> 8);
	dst[2] = (char)chunk;

	return outLen;
}
