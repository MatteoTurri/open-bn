/*
 * DNAS2: RSA decryption of a 128-byte block with one of the 100 built-in keys.
 */

#include "dnas2m12_des.h"
#include "dnas2keyload.h"
#include "dnas2m12_rand.h"
#include "dnas2m12_rsa.h"

static int dnas2_load_key(unsigned int idx, unsigned char *buf, unsigned int *len);

typedef struct {
	/* 0x00 */ unsigned char hdr[8];
	/* 0x08 */ unsigned char tag[8];
	/* 0x10 */ unsigned char body[184];
} Dnas2KeySlot;   /* size = 0xC8 */

/* The encrypted key table. */
static Dnas2KeySlot dnas2_keyslots[100] = {
#include "../../assets/dnas/dnas2_keyslots.inc"
};

/* Decrypts key slot `idx` into a local record, returns its length (big-endian
 * word at +4) in *len and copies that many bytes from +8 into `buf`. A length
 * of 177 or more is a corrupt record (-1). The local record is always
 * overwritten with random bytes. */
static int dnas2_load_key(unsigned int idx, unsigned char *buf, unsigned int *len)
{
	unsigned char rec[200];
	Dnas2KeySlot *e;
	unsigned char *w;
	unsigned char *src;
	unsigned int n;
	unsigned int j;
	int i;

	e = &dnas2_keyslots[idx];
	if (__sce_dnas2_symbol12118(e->body, e, rec, e->tag, 23) < 0)
		return -1;
	n = (rec[4] << 24) | (rec[5] << 16) | (rec[6] << 8) | rec[7];
	*len = n;
	if (n >= 177) {
		w = rec;
		for (i = 0; i < 200; i++)
			w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
		return -1;
	}
	j = 0;
	src = rec + 8;
	if (n != 0) {
		do {
			buf[j] = src[j];
			j++;
		} while (j < *len);
	}
	w = rec;
	for (i = 0; i < 200; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	return 0;
}

/* Decrypts a 128-byte RSA block with the key in slot
 * `idx` (0..99), then overwrites the key on the stack with random bytes. */
int __sce_dnas2_symbol12206(unsigned char *in, unsigned int idx, unsigned int *outLen, unsigned char *out)
{
	unsigned char keybuf[176];
	int n;
	int status;
	int i;

	__sce_dnas2_symbol12062();
	if (idx >= 100)
		return -1;
	if (dnas2_load_key(idx, keybuf, &n) < 0)
		return -1;

	status = __sce_dnas2_symbol12110(in, 128, keybuf, n, out, 128, outLen);
	for (i = 0; i < 176; i++)
		keybuf[i] =
		    (unsigned char)(unsigned long)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	return status;
}
