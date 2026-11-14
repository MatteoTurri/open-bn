/*
 * Base64 encoder and decoder (one-shot and stateful) with C linkage.
 */

#include <stdlib.h>
#include "bn_base64.h"

extern "C" {

static const char b64_enc_tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void bn_base64_encode(unsigned char *out, const unsigned char *in, int n)
{
	while (n >= 3) {
		*out++ = b64_enc_tbl[in[0] >> 2];
		*out++ = b64_enc_tbl[((in[0] & 0x3) << 4) | (in[1] >> 4)];
		*out++ = b64_enc_tbl[((in[1] & 0xf) << 2) | (in[2] >> 6)];
		*out++ = b64_enc_tbl[in[2] & 0x3f];
		in += 3;
		n -= 3;
	}
	if (n > 0) {
		*out++ = b64_enc_tbl[in[0] >> 2];
		if (n == 1) {
			*out++ = b64_enc_tbl[(in[0] & 0x3) << 4];
			*out++ = '=';
		} else {
			*out++ = b64_enc_tbl[((in[0] & 0x3) << 4) | (in[1] >> 4)];
			*out++ = b64_enc_tbl[(in[1] & 0xf) << 2];
		}
		*out++ = '=';
	}
	*out = 0;
}

/* Inverse table: 128 entries, -1 for non-base64 characters. Callers reject
   bytes with bit 7 set before indexing it. */
/* clang-format off */
static const signed char b64_dec_tbl[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
};
/* clang-format on */

#define B64V(c) (((c) & ~0x7f) ? -1 : b64_dec_tbl[(c)])

/*
 * Decodes the base64 quartets of `in` (at most n characters, n < 0 means no
 * limit) into `out` and returns the number of bytes written. Stops at the
 * NUL, at the first non-base64 character or at '=' padding.
 */
int bn_base64_decode(unsigned char *out, const char *in, int n)
{
	unsigned char *o = out;
	char q[4];

	if (n < 0)
		n = 0x7fffffff;
	while (n >= 4 && *in != 0) {
		q[0] = *in++;
		n--;
		if (q[0] & ~0x7f)
			goto done;
		if (b64_dec_tbl[q[0]] == -1)
			goto done;
		q[1] = *in++;
		n--;
		if (q[1] & ~0x7f)
			goto done;
		if (b64_dec_tbl[q[1]] == -1)
			goto done;
		q[2] = *in++;
		n--;
		if (q[2] != '=') {
			if (q[2] & ~0x7f)
				goto done;
			if (b64_dec_tbl[q[2]] == -1)
				goto done;
		}
		q[3] = *in++;
		n--;
		if (q[3] != '=') {
			if (q[3] & ~0x7f)
				goto done;
			if (b64_dec_tbl[q[3]] == -1)
				goto done;
		}
		*o++ = (B64V(q[0]) << 2) | ((B64V(q[1]) >> 4) & 3);
		if (q[2] != '=') {
			*o++ = ((B64V(q[1]) & 0xf) << 4) | ((B64V(q[2]) >> 2) & 0xf);
			if (q[3] != '=')
				*o++ = ((B64V(q[2]) & 3) << 6) | (B64V(q[3]) & 0x3f);
		}
	}
done:
	return o - out;
}

BN_B64_DECODER *bn_base64_decoder_new(void)
{
	return (BN_B64_DECODER *)malloc(sizeof(BN_B64_DECODER));
}

void bn_base64_decoder_free(BN_B64_DECODER *d)
{
	free(d);
}

/*
 * Stateful decoder: collects base64 characters into the context quartet,
 * skipping CR/LF, and decodes each complete quartet. Returns the number of
 * bytes produced, or -1 on a non-base64 character.
 * A quartet with '=' at position 0 or 1 is not decoded (end of stream); a
 * quartet that yields fewer than 3 bytes resets the count and stops.
 */
int bn_base64_decoder_decode(BN_B64_DECODER *d, const char *in, unsigned char *out)
{
	char buf[4];
	int n;
	int total;
	int c;
	int r;

	total = 0;
	*(unsigned int *)buf = *(unsigned int *)d->buf;
	n = d->count;

	for (;;) {
		while (n < 4 && (c = *in++) != 0) {
			if (c == 13 || c == 10)
				continue;
			if (c != 61) {
				if (c & ~0x7f)
					return -1;
				if (b64_dec_tbl[c] == -1)
					return -1;
			}
			buf[n] = c;
			n++;
		}

		if (n < 4 || buf[0] == 61 || buf[1] == 61) {
			d->count = n;
			*(unsigned int *)d->buf = *(unsigned int *)buf;
			return total;
		}

		r = bn_base64_decode(out, buf, 4);
		out += r;
		total += r;
		if (r < 3)
			break;
		n = 0;
	}
	d->count = 0;
	return total;
}

} /* extern "C" */
