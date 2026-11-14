/*
 * DNAS2 profile loader and record reader: loads the protected profile into
 * the static state (profile, reader), opens the session and delivers decrypted record
 * data to the caller.
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "dnas_err.h"
#include "lib/scepfsd.h"
#include "dnas2m12_des.h"
#include "dnas2m65_des.h"
#include "dnas2keyload.h"
#include "dnas2m12_records.h"
#include "dnas2m12_rand.h"
#include "dnas2read.h"
#include "dnas2m65_rsa.h"
#include "dnas2session.h"
#include "dnas2_key.h"

/* Reader state: the open record, its decrypted bytes and cipher contexts. */
struct dnas2_reader {
	/* 0x00 */ int fd;
	/* 0x04 */ unsigned int total;
	/* 0x08 */ int stream_len;
	/* 0x0C */ unsigned int left;
	/* 0x10 */ unsigned int mode;
	/* 0x14 */ int len;
	/* 0x18 */ int sel;
	/* 0x1C */ int idx;
	/* 0x20 */ int seen;
	/* 0x24 */ unsigned int have;  /* decrypted bytes not yet delivered */
	/* 0x28 */ unsigned int pos;  /* where they start in buf */
	/* 0x2C */ unsigned char buf[32768];  /* read buffer */
	/* 0x802C */ void *ctx[2];  /* the two cipher contexts */
};

struct dnas2_tbl {
	/* 0x00 */ int opened;  /* session open */
	/* 0x04 */ unsigned char vendor[32];
	/* 0x24 */ unsigned char title[16];
	/* 0x34 */ unsigned char subtitle[32];
	/* 0x54 */ unsigned char rec_key[32];
	/* 0x74 */ unsigned char dig[20];
	/* 0x88 */ unsigned char cipher_key[24];
	/* 0xA0 */ unsigned char iv[8];
	/* 0xA8 */ unsigned char tbl_key[64];
	/* 0xE8 */ unsigned int tbl_len;
	/* 0xEC */ unsigned char tbl_skip[20];
	/* 0x100 */ unsigned char sel[512][4];  /* selection table */
	/* 0x900 */ unsigned char rec_keys[16384];  /* per-record keys and markers */
	/* 0x4900 */ int (*read_fn)(int, void *, unsigned int);  /* read */
	/* 0x4904 */ long (*seek_fn)(int, long, int);  /* lseek */
};

static struct dnas2_tbl profile;
static struct dnas2_reader reader;

static int dnas2_session_io(void *buf, int n);
static int dnas2_skip(unsigned int n, int mode);
static int dnas2_read(unsigned char *buf, unsigned int n, int mode);
static int dnas2_decrypt_blocks(void *buf, unsigned int n, int mode);
static int dnas2_set_record_cipher(unsigned char *p);
static int dnas2_unwrap_profile(unsigned char *hdr, void *hdrArg, unsigned char *prof, unsigned int profSize,
    unsigned int *outLen);


/* Copies a string without its terminator. */
static __inline__ void dnas2_strcpy(unsigned char *d, const char *s)
{
	unsigned int i;

	for (i = 0; i < strlen(s); i++)
		d[i] = s[i];
}

/* Byte copies: one with an unsigned index, one with an int index. */
static __inline__ void dnas2_memcpy(void *dst, const void *src, unsigned int n)
{
	unsigned char *d = dst;
	const unsigned char *s = src;
	unsigned int i;

	if (n) {
		i = 0;
		do {
			d[i] = s[i];
			i++;
		} while (i < n);
	}
}

static __inline__ void dnas2_cpy(unsigned char *d, const unsigned char *s, int n)
{
	int i;

	for (i = 0; i < n; i++)
		d[i] = s[i];
}

struct dnas2_body_hdr {
	unsigned int len;
	unsigned char sig[16];
};

/* debug trace of the module, empty in release builds */
#define DNAS2_TRACE(args)	do { } while (0)

/*
 * Loads the DNAS2 profile: clears profile, copies the three strings, picks read/lseek
 * or scepfsRead/scepfsLseek, reads 32 KB, checks the magic 0x20005, then decodes
 * the stream in 128-byte blocks for tags 50-54 and 56, each checked against a
 * 16-byte marker and copied into profile. Finally clears reader, sets it up, and seeks
 * the descriptor to 0x807C.
 */
int __sce_dnas2_symbol65037(unsigned char *inbuf, unsigned int inbufLen, unsigned char *hdr, void *hdrArg, int fd,
    const char *vendor, const char *title, const char *subtitle, int usepfs)
{
	unsigned char blk[128];
	unsigned char plain[128];
	unsigned char body[32768];
	unsigned char page[32768];
	unsigned char digest[20];
	unsigned int profLen;
	unsigned int len;
	struct dnas2_tbl *tbl = &profile;
	struct dnas2_reader *rdr = &reader;
	struct dnas2_body_hdr *h;
	unsigned char *src, *cur;
	unsigned int n;
	int r;
	int cmp;

	__sce_dnas2_symbol12062();
	{
		unsigned int i;
		for (i = 0; i < sizeof(profile); i++)
			((unsigned char *)tbl)[i] = 0;
	}
	dnas2_strcpy(tbl->vendor, vendor);
	dnas2_strcpy(tbl->title, title);
	dnas2_strcpy(tbl->subtitle, subtitle);
	if (inbuf) {
		r = dnas2_unwrap_profile(hdr, hdrArg, inbuf, inbufLen, &profLen);
		if (r)
			goto fail;
	}
	if (usepfs == 0) {
		tbl->read_fn = read;
		tbl->seek_fn = lseek;
	} else {
		tbl->read_fn = scepfsRead;
		tbl->seek_fn = scepfsLseek;
	}
	if (inbuf == 0) {
		r = tbl->read_fn(fd, page, 0x8000);
		if (r != 0x8000)
			goto fail;
		src = page;
	} else {
		src = inbuf;
	}
	dnas2_memcpy(&len, src, 4);
	src += 4;
	if (len != 0x20005) {
		r = SCE_DNAS_ERR_HEADER;
		goto fail;
	}
	src += 124;
	dnas2_cpy(blk, src, 128);
	src += 128;
	if (__sce_dnas2_symbol12206(blk, 50, &len, plain)) {
		r = SCE_DNAS_ERR_HEADER;
		goto fail;
	}
	cmp = memcmp(plain + 32, "a17d375e63102ca5", 16);
	r = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto fail;
	dnas2_memcpy(tbl->rec_key, plain, 32);
	dnas2_cpy(blk, src, 128);
	src += 128;
	r = __sce_dnas2_symbol12206(blk, 51, &len, plain);
	if (r)
		goto fail;
	cmp = memcmp(plain + 20, "c5a0f964ea216068", 16);
	r = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto fail;
	dnas2_memcpy(tbl->dig, plain, 20);
	dnas2_cpy(blk, src, 128);
	src += 128;
	r = __sce_dnas2_symbol12206(blk, 52, &len, plain);
	if (r)
		goto fail;
	h = (struct dnas2_body_hdr *)plain;
	cmp = memcmp(h->sig, "9ae8e2ce4741cd0f", 16);
	r = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto fail;
	{
		unsigned char *d = body;
		unsigned int i = 0;

		n = h->len;
		DNAS2_TRACE(("rec %u\n", n));
		if (n) {
			unsigned int k = h->len;

			do {
				d[i] = src[i];
				i++;
			} while (i < k);
		}
	}
	__sce_dnas2_symbol12149(body, n, digest);
	cmp = memcmp(digest, tbl->dig, 20);
	r = SCE_DNAS2_ERR_DIGEST;
	if (cmp)
		goto fail;
	r = __sce_dnas2_symbol65043(tbl->rec_key, 0x8000, body, body);
	if (r)
		goto fail;
	cur = body;
	r = __sce_dnas2_symbol12206(cur, 53, &len, plain);
	if (r)
		goto fail;
	cmp = memcmp(plain + 32, "42d95555748faffd", 16);
	r = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto fail;
	dnas2_memcpy(tbl->cipher_key, plain, 32);
	cur += 128;
	r = __sce_dnas2_symbol12206(cur, 54, &len, plain);
	if (r)
		goto fail;
	cmp = memcmp(plain + 32, "c783d3ea9e4928fc", 16);
	r = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto fail;
	dnas2_memcpy(tbl->tbl_key, plain, 32);
	cur += 256;
	r = __sce_dnas2_symbol12206(cur, 56, &len, plain);
	if (r)
		goto fail;
	cmp = memcmp(plain + 12, "890a39e4335e5fff", 16);
	r = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto fail;
	dnas2_memcpy(&tbl->tbl_len, plain, 4);
	for (len = 0; len < 8; len++)
		tbl->tbl_skip[len] = plain[len + 4];
	cur += 256;
	r = __sce_dnas2_symbol65043(tbl->tbl_key, tbl->tbl_len, cur, cur);
	if (r)
		goto fail;
	cmp = memcmp(cur + tbl->tbl_skip[0] + (tbl->tbl_skip[1] + 1024) + 8192, "5e34571f513dc909", 16);
	r = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto fail;
	{
		unsigned char *selDst, *keyDst;
		unsigned int i;

		cur += tbl->tbl_skip[0];
		selDst = tbl->sel[0];
		for (i = 0; i < 1024; i++)
			selDst[i] = cur[i];
		cur += 1024;
		cur += tbl->tbl_skip[1];
		keyDst = tbl->rec_keys;
		for (i = 0; i < 8192; i++)
			keyDst[i] = cur[i];
	}
	{
		unsigned char *d = (unsigned char *)rdr;
		unsigned int i;

		for (i = 0; i < sizeof(*rdr); i++)
			d[i] = 0;
	}
	rdr->fd = fd;
	rdr->sel = 132;
	rdr->idx = 81;
	rdr->len = tbl->tbl_skip[2];
	r = __sce_dnas2_symbol12121(tbl->cipher_key, tbl->iv, rdr->ctx);
	if (r)
		goto fail;
	if (tbl->read_fn(rdr->fd, &len, 4) != 4) {
		r = SCE_DNAS_ERR_HEADER;
		goto fail;
	}
	if (len != 0x20005) {
		r = SCE_DNAS_ERR_BUSY;
		goto fail;
	}
	tbl->seek_fn(rdr->fd, 0x807c, 1);
	return 4;

fail:
	return r;
}

/* Returns the total length of the session stream (__sce_dnas2_symbol65040). */
int __sce_dnas2_symbol65036(void)
{
	return __sce_dnas2_symbol65040();
}

/* Opens the session with the three strings stored in profile by
 * __sce_dnas2_symbol65037; on success mark profile as open. */
int __sce_dnas2_symbol65038(void)
{
	struct dnas2_tbl *tbl = &profile;
	int status =
	    __sce_dnas2_symbol65041(tbl->vendor, tbl->title, tbl->subtitle, &reader.stream_len, dnas2_session_io);
	if (status != 0)
		return status;
	profile.opened = 1;
	return 0;
}

/* Marks profile as closed. */
int __sce_dnas2_symbol65035(void)
{
	profile.opened = 0;
	return 0;
}

/* Session-open flag and 4-byte field read. */
static inline int dnas2_opened(struct dnas2_tbl *t)
{
	return t->opened;
}

static inline int dnas2_read_word(void *field)
{
	return dnas2_read((unsigned char *)field, 4, 2);
}

/* error site with its own code: set r and go to fail if c */
#define DNAS2_CHECK(c, e)	do { r = (e); if (c) goto fail; } while (0)

/*
 * Reads a record: reads and re-reads its header field by field, checks a 16-byte
 * marker against the table twice, skips the padding to 8 bytes, then delivers
 * n bytes to the caller in one of two ways depending on mode and the
 * session-open flag.
 */
int __sce_dnas2_symbol65039(void *buf, unsigned int n)
{
	unsigned char mark[128];
	struct dnas2_reader *rdr;
	struct dnas2_tbl *tbl;
	int r;
	int oldlen;
	int tmp;
	int pad;
	int cmp;

	rdr = &reader;
	tbl = &profile;
	if (rdr->total == 0) {
		if (rdr->seen == 1) {
			r = dnas2_read(mark, 16, 2);
			if (r != 16)
				goto fail;
			cmp = memcmp(mark, tbl->rec_keys + 1 + 16 * rdr->idx, 16);
			DNAS2_CHECK(cmp != 0, SCE_DNAS2_ERR_MARKER);
		} else {
			rdr->seen = 1;
		}

		tmp = tbl->sel[rdr->sel][0];
		dnas2_set_record_cipher(tbl->rec_keys + 32 * tmp);

		r = dnas2_read(rdr->buf, rdr->len, 2);
		if (rdr->len != r)
			goto fail;
		r = dnas2_read_word(&rdr->mode);
		if (r != 4)
			goto fail;
		r = dnas2_read_word(&rdr->total);
		if (r != 4)
			goto fail;
		r = dnas2_read_word(&rdr->left);
		if (r != 4)
			goto fail;
		oldlen = rdr->len;
		r = dnas2_read_word(&rdr->len);
		if (r != 4)
			goto fail;
		r = dnas2_read_word(&rdr->sel);
		if (r != 4)
			goto fail;
		tmp = rdr->idx;
		r = dnas2_read_word(&rdr->idx);
		if (r != 4)
			goto fail;
		r = dnas2_read(mark, 16, 2);
		if (r != 16)
			goto fail;
		cmp = memcmp(mark, tbl->rec_keys + 1 + 16 * tmp, 16);
		DNAS2_CHECK(cmp != 0, SCE_DNAS2_ERR_MARKER);

		tmp = rdr->total + oldlen + 56;
		pad = 8 - (tmp & 7);
		if (pad == 8)
			pad = 0;
		r = dnas2_skip(pad, 2);
		if (pad != r)
			goto fail;
	}

	if (rdr->mode < 2 || (rdr->mode == 2 && dnas2_opened(tbl) == 0) || (rdr->mode == 3 && dnas2_opened(tbl) == 0)) {
		DNAS2_CHECK(rdr->total < n || rdr->left < n, -1);
		r = dnas2_read((unsigned char *)buf, n, 2);
		if (n != r)
			goto fail;
		rdr->total -= r;
		rdr->left -= r;
	} else if ((rdr->mode == 2 && dnas2_opened(tbl) == 1) || (rdr->mode == 3 && dnas2_opened(tbl) == 1)) {
		if (rdr->total >= n) {
			r = __sce_dnas2_symbol65042(buf, n);
			if (n != r)
				goto fail;
			rdr->total -= r;
		}
	}
	return n;

fail:
	return r;
}

/* Forwards with mode 2. */
static int dnas2_session_io(void *buf, int n)
{
	return dnas2_read(buf, n, 2);
}

/* Like dnas2_read, but discards the n bytes instead of delivering them. */
static int dnas2_skip(unsigned int n, int mode)
{
	struct dnas2_reader *rdr;
	struct dnas2_tbl *tbl;
	unsigned int rem;
	unsigned int have;
	unsigned int pad;
	unsigned int want;
	unsigned char *bp;
	unsigned char *bq;
	int r;

	tbl = &profile;
	rdr = &reader;
	rem = n;
	while (rem != 0) {
		have = rdr->have;
		if (have != 0) {
			if (rem >= have) {
				rem = rem - have;
				rdr->have = 0;
				continue;
			}
			rdr->have = rdr->have - rem;
			rdr->pos = rdr->pos + rem;
			return n;
		}
		if (rem > 32767) {
			bp = rdr->buf;
			r = tbl->read_fn(rdr->fd, bp, 0x8000);
			if (r != 0x8000)
				goto shortret;
			dnas2_decrypt_blocks(bp, 0x8000, mode);
			rem = rem - 32768;
			continue;
		}
		pad = 8 - (rem & 7);
		if (pad == 8)
			pad = 0;
		want = pad + rem;
		bq = rdr->buf;
		r = tbl->read_fn(rdr->fd, bq, want);
		if (want != r)
			goto shortret;
		dnas2_decrypt_blocks(bq, r, mode);
		if (pad == 0) {
			rdr->have = 0;
			rdr->pos = 0;
		} else {
			rdr->have = pad;
			rdr->pos = rem;
		}
		rem = 0;
	}
	return n;
shortret:
	return r;
}

/* Reads n bytes into dst through the internal 32 KB buffer, decrypting each
 * block. Reads are rounded up to 8 bytes and the excess is kept in have/pos
 * for the next call. Returns n, or the short count returned by the read. */
static int dnas2_read(unsigned char *buf, unsigned int n, int mode)
{
	unsigned int rem;
	unsigned int have;
	unsigned int pad;
	unsigned int want;
	unsigned int j;
	int i;
	int r;
	unsigned char *src;
	struct dnas2_reader *rdr;
	struct dnas2_tbl *tbl;
	unsigned char *bp;
	unsigned char *bq;
	unsigned char *dst;

	dst = buf;
	tbl = &profile;
	rdr = &reader;
	rem = n;
	while (rem != 0) {
		if (rdr->have != 0) {
			if (rem >= rdr->have) {
				src = rdr->buf + rdr->pos;
				for (j = 0; j < rdr->have; j++)
					dst[j] = src[j];
				have = rdr->have;
				rdr->have = 0;
				dst = dst + have;
				rem = rem - have;
				continue;
			}
			src = rdr->buf + rdr->pos;
			for (j = 0; j < rem; j++)
				dst[j] = src[j];
			rdr->have = rdr->have - rem;
			rdr->pos = rdr->pos + rem;
			return n;
		}
		if (rem > 32767) {
			bp = rdr->buf;
			r = tbl->read_fn(rdr->fd, bp, 0x8000);
			if (r != 0x8000)
				goto shortret;
			dnas2_decrypt_blocks(bp, 0x8000, mode);
			for (i = 0; i < 32768; i++)
				dst[i] = bp[i];
			dst = dst + 32768;
			rem = rem - 32768;
			continue;
		}
		pad = 8 - (rem & 7);
		if (pad == 8)
			pad = 0;
		want = pad + rem;
		bq = rdr->buf;
		r = tbl->read_fn(rdr->fd, bq, want);
		if (want != r)
			goto shortret;
		dnas2_decrypt_blocks(bq, r, mode);
		j = 0;
		if (rem != 0) {
			do {
				dst[j] = bq[j];
				j++;
			} while (j < rem);
		}
		if (pad == 0) {
			rdr->have = 0;
			rdr->pos = 0;
		} else {
			rdr->have = pad;
			rdr->pos = rem;
		}
		rem = 0;
	}
	return n;
shortret:
	return r;
}

/* Decrypt n / 8 blocks with one of the two cipher contexts; mode 2 uses both
 * in sequence and stops at the first error. */
static int dnas2_decrypt_blocks(void *buf, unsigned int n, int mode)
{
	int r = 0;
	struct dnas2_reader *rdr = &reader;

	if (mode == 0) {
		r = __sce_dnas2_symbol12124(rdr->ctx[1], buf, n >> 3);
	} else if (mode == 1) {
		r = __sce_dnas2_symbol12124(rdr->ctx[0], buf, n >> 3);
	} else {
		if (mode != 2 || (r = __sce_dnas2_symbol12124(rdr->ctx[0], buf, n >> 3)) != 0)
			return r;
		r = __sce_dnas2_symbol12124(rdr->ctx[1], buf, n >> 3);
	}
	return r;
}

/* Sets up the second cipher context from the key at keyIv and the IV at keyIv + 24. */
static int dnas2_set_record_cipher(unsigned char *keyIv)
{
	return __sce_dnas2_symbol12121(keyIv, keyIv + 24, &reader.ctx[1]);
}

/* ------------------------------------------------------------------ *
 * Record opening for __sce_dnas2_symbol65037: four RSA decryptions with two
 * BER public keys, checked against 16-character ASCII markers, two SHA-1
 * digests and the two symmetric cipher variants; every buffer is refilled with
 * random bytes as soon as it is no longer needed.
 * ------------------------------------------------------------------ */

struct DnasBlob128 {
	unsigned char b[128];
};

/* RSA public keys of the header (the one that opens its key blocks, the one of its
 * digest) and the seeds of the header and profile body keys. */
static const struct DnasPubKey dnas2_prof_key_pubkey = { {
#include "../../assets/dnas/dnas2read_prof_key_pubkey.inc"
} };

static const struct DnasPubKey dnas2_prof_digest_pubkey = { {
#include "../../assets/dnas/dnas2read_prof_digest_pubkey.inc"
} };

static const struct DnasBlob128 dnas2_prof_hdr_seed = { {
#include "../../assets/dnas/dnas2read_prof_hdr_seed.inc"
} };

static const struct DnasBlob128 dnas2_prof_body_seed = { {
#include "../../assets/dnas/dnas2read_prof_body_seed.inc"
} };

/* Verifies the 4 KB header hdr, decrypts the profile prof in place, moves its data
 * to the start, fills the rest of the profSize bytes with random bytes and stores
 * the data length in *outLen. */
static int dnas2_unwrap_profile(unsigned char *hdr, void *hdrArg, unsigned char *prof, unsigned int profSize,
    unsigned int *outLen)
{
	struct DnasPubKey keyPub;
	struct DnasPubKey digestPub;
	struct DnasKey rsaA;
	struct DnasKey rsaB;
	struct DnasKey profRsa1;
	struct DnasKey profRsa2;
	unsigned char seedKey[32];
	unsigned char cbcKey[32];
	unsigned char profKey[32];
	unsigned char hashKey[32];
	unsigned char bodyKey[32];
	unsigned char plain[128];
	unsigned char work[128];
	unsigned char digest[20];
	struct DnasBlob128 hdrSeed;
	unsigned char seed[128];
	unsigned char mix[64];
	struct DnasBlob128 bodySeed;
	unsigned char rnd[16];
	unsigned int len;
	unsigned int tag;
	unsigned int ver;
	unsigned int dataLen;
	unsigned int gapLen;
	unsigned int off;
	unsigned int pad;
	int ret;
	int cmp;  /* result of the memcmp of each branch */
	int i;
	unsigned int j;
	unsigned int c;
	unsigned int k;
	unsigned char *clr = 0;  /* redundant as written */
	unsigned char *dst = 0;  /* redundant as written */
	unsigned char *from = 0;  /* redundant as written */
	unsigned char *tail = 0;  /* redundant as written */
	unsigned char *src = 0;  /* redundant as written */

	keyPub = dnas2_prof_key_pubkey;
	digestPub = dnas2_prof_digest_pubkey;
	hdrSeed = dnas2_prof_hdr_seed;
	bodySeed = dnas2_prof_body_seed;

	clr = (unsigned char *)&rsaB;
	for (k = 0; k < sizeof(rsaB); k++)
		clr[k] = 0;
	dst = rsaB.data;
	src = digestPub.b;
	rsaB.len = 162;
	for (i = 0; i < 162; i++)
		dst[i] = src[i];
	clr = (unsigned char *)&rsaA;
	for (k = 0; k < sizeof(rsaA); k++)
		clr[k] = 0;
	dst = rsaA.data;
	src = keyPub.b;
	rsaA.len = 162;
	for (i = 0; i < 162; i++)
		dst[i] = src[i];

	__sce_dnas2_symbol12149(hdr + 256, 3840, digest);
	memcpy(plain, hdr, 128);
	ret = __sce_dnas2_symbol65032(hdr + 128, 128, &rsaB, plain);
	if (ret)
		goto out;
	if (memcmp(plain + 20, "1d150532b0ad3f0e", 16))
		ret = SCE_DNAS2_ERR_MARKER;
	if (ret)
		goto out;
	if (memcmp(plain, digest, 20))
		ret = SCE_DNAS2_ERR_DIGEST;
	if (ret)
		goto out;
	ret = __sce_dnas2_symbol65032(hdr + 256, 128, &rsaA, plain);
	if (ret)
		goto out;
	if (memcmp(plain + 4, "29e61970424bb42e", 16))
		ret = SCE_DNAS2_ERR_MARKER;
	if (ret)
		goto out;
	src = hdr + 384;
	ret = __sce_dnas2_symbol12206(src, *(unsigned int *)plain, &len, src);
	if (ret)
		goto out;
	if (memcmp(hdr + 416, "47a45a47f688666c", 16))
		ret = SCE_DNAS2_ERR_MARKER;
	if (ret)
		goto out;

	memcpy(seedKey, hdr + 384, 32);
	memcpy(seed, &hdrSeed, 128);
	memcpy(cbcKey, seed, 32);
	memcpy(seed, seedKey, 32);
	ret = __sce_dnas2_symbol12068();
	if (ret)
		goto out;
	ret = __sce_dnas2_symbol12064(rnd);
	if (ret != 4)
		goto out;
	dst = seed + 32;
	src = rnd;
	for (c = 0; c < 4; c++)
		dst[c] = src[c];
	ret = __sce_dnas2_symbol65034(cbcKey, 128, seed, seed);
	if (ret)
		goto out;
	memcpy(cbcKey, seed + 96, 32);
	src = hdr + 512;
	ret = __sce_dnas2_symbol65033(cbcKey, 2048, src, src);
	if (ret)
		goto out;

	ret = __sce_dnas2_symbol65032(hdr + 1536, 128, &rsaA, work);
	if (ret)
		goto out;
	cmp = memcmp(work + 4, "5ca39a0d50d91b79", 16);
	ret = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto out;
	dst = (unsigned char *)&tag;
	src = work;
	for (c = 0; c < 4; c++)
		dst[c] = src[c];
	for (j = 0; j < 128; j++)
		work[j] = DNAS2_RAND_BYTE;
	ret = __sce_dnas2_symbol12206(hdr + 640, tag, &len, work);
	if (ret)
		goto out;
	cmp = memcmp(work + 32, "cd6a3b1f00a3b83a", 16);
	ret = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto out;
	{
		unsigned char *dst = hashKey;

		src = work;
		for (c = 0; c < 32; c++)
			dst[c] = src[c];
	}
	for (j = 0; j < 128; j++)
		work[j] = DNAS2_RAND_BYTE;
	ret = __sce_dnas2_symbol65033(hashKey, 128, hdr + 896, work);
	if (ret)
		goto out;
	cmp = memcmp(work + 64, "bf35e902809e3044", 16);
	ret = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto out;
	{
		unsigned char *dst = mix;

		src = work;
		for (c = 0; c < 64; c++)
			dst[c] = src[c];
	}
	for (j = 0; j < 128; j++)
		work[j] = DNAS2_RAND_BYTE;

	src = hdr + 1024;
	dst = (unsigned char *)&profRsa1.len;
	for (c = 0; c < 4; c++)
		dst[c] = src[c];
	src += 4;
	dst = profRsa1.data;
	for (c = 0; c < 252; c++)
		dst[c] = src[c];
	src += 252;
	dst = (unsigned char *)&profRsa2.len;
	for (c = 0; c < 4; c++)
		dst[c] = src[c];
	src += 4;
	dst = profRsa2.data;
	for (c = 0; c < 252; c++)
		dst[c] = src[c];
	for (j = 0; j < 4; j++)
		hdr[j] = DNAS2_RAND_BYTE;

	dst = (unsigned char *)&ver;
	for (c = 0; c < 4; c++)
		dst[c] = prof[c];
	if (ver != 0x20005) {
		ret = -10203;
		goto out;
	}
	ret = __sce_dnas2_symbol65032(prof + 128, 128, &profRsa1, plain);
	if (ret)
		goto out;
	cmp = memcmp(plain + 40, "c59285a68ee22a39", 16);
	ret = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto out;
	{
		unsigned char *dst = profKey;

		src = plain;
		for (c = 0; c < 32; c++)
			dst[c] = src[c];
	}
	src = plain + 32;
	dst = (unsigned char *)&dataLen;
	for (c = 0; c < 4; c++)
		dst[c] = src[c];
	src += 4;
	dst = (unsigned char *)&gapLen;
	for (c = 0; c < 4; c++)
		dst[c] = src[c];
	for (j = 0; j < 128; j++)
		plain[j] = DNAS2_RAND_BYTE;

	off = dataLen + gapLen + 404;
	pad = 8 - (off & 7);
	if (pad == 8)
		pad = 0;
	off += pad;
	ret = __sce_dnas2_symbol65032(&prof[off + 256], 128, &profRsa2, plain);
	if (ret)
		goto out;
	cmp = memcmp(plain + 20, "4324eff6930ea3a8", 16);
	ret = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto out;
	__sce_dnas2_symbol12149(prof + 256, off, digest);
	cmp = memcmp(digest, plain, 20);
	ret = SCE_DNAS2_ERR_DIGEST;
	if (cmp)
		goto out;
	for (j = 0; j < 128; j++)
		plain[j] = DNAS2_RAND_BYTE;
	for (j = 0; j < 20; j++)
		digest[j] = DNAS2_RAND_BYTE;

	dst = work;
	src = bodySeed.b;
	for (i = 0; i < 128; i++)
		dst[i] = src[i];
	for (j = 0; j < 128; j++)
		bodySeed.b[j] = DNAS2_RAND_BYTE;
	{
		unsigned char *dst = cbcKey;

		src = work;
		for (i = 0; i < 32; i++)
			dst[i] = src[i];
	}
	dst = work;
	src = profKey;
	for (i = 0; i < 32; i++)
		dst[i] = src[i];
	dst = work + 32;
	src = mix;
	for (c = 0; c < 64; c++)
		dst[c] = src[c];
	ret = __sce_dnas2_symbol65034(cbcKey, 128, work, work);
	if (ret)
		goto out;
	{
		unsigned char *dst = bodyKey;

		from = work + 96;
		for (c = 0; c < 32; c++)
			dst[c] = from[c];
	}
	for (j = 0; j < 128; j++)
		work[j] = DNAS2_RAND_BYTE;
	for (j = 0; j < 64; j++)
		mix[j] = DNAS2_RAND_BYTE;
	for (j = 0; j < 32; j++)
		profKey[j] = DNAS2_RAND_BYTE;

	src = prof + 256;
	ret = __sce_dnas2_symbol65033(bodyKey, off, src, src);
	if (ret)
		goto out;
	cmp = memcmp(src + (dataLen + gapLen + 388), "f71af3f27c6e4a6d", 16);
	ret = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto out;
	src = (unsigned char *)(gapLen + (unsigned int)prof + 640);
	for (c = 0; c < dataLen; c++)
		prof[c] = src[c];
	tail = prof + dataLen;
	for (j = 0; j < profSize - dataLen; j++)
		tail[j] = DNAS2_RAND_BYTE;
	*outLen = dataLen;
	return 0;
out:
	return ret;
}
