/*
 * DNAS2 session stream: receives the protected content records from the host,
 * decrypts and verifies them, and delivers the plain stream to the caller.
 */

#include <string.h>
#include <stdlib.h>

#include "dnas_err.h"
#include "dnas2m12_des.h"
#include "dnas2m12_rc6.h"
#include "dnas2keyload.h"
#include "dnas2m12_records.h"
#include "dnas2m12_rand.h"
#include "dnas2m65_rc6.h"
#include "dnas2session.h"

/* One record of the session stream. */
struct dnas2_sess_rec {
	/* 0x000 */ unsigned char busy;
	/* 0x001 */ unsigned char pad_001[3];
	/* 0x004 */ int left;  /* bytes left to deliver */
	/* 0x008 */ unsigned int body_pos;
	/* 0x00C */ unsigned char dec_key[16];
	/* 0x01C */ unsigned char dec_iv[16];
	/* 0x02C */ unsigned char pad_02C[32];
	/* 0x04C */ unsigned char hdr_key[32];
	/* 0x06C */ unsigned char enc_key[16];
	/* 0x07C */ unsigned char enc_iv[16];
	/* 0x08C */ unsigned char hash_state;
	/* 0x08D */ unsigned char digest[23];
	/* 0x0A4 */ unsigned int body_len;
	/* 0x0A8 */ unsigned int data_len;
	/* 0x0AC */ int pad_0AC;
	/* 0x0B0 */ unsigned char unk_0B0[4];
	/* 0x0B4 */ int size;
	/* 0x0B8 */ unsigned char skip_blocks;
	/* 0x0B9 */ unsigned char unk_0B9;
	/* 0x0BA */ unsigned char fill_blocks;
	/* 0x0BB */ unsigned char unk_0BB;
	/* 0x0BC */ unsigned char skip_left;
	/* 0x0BD */ unsigned char fill_left;
	/* 0x0BE */ unsigned char align_pad;
	/* 0x0BF */ unsigned char pad_used;  /* random padding bytes already used */
	/* 0x0C0 */ int sig_key;
	/* 0x0C4 */ unsigned char end_mark[64];  /* end marker collected from the stream */
	/* 0x104 */ int end_len;
	/* 0x108 */ unsigned char state;  /* delivery state (0..3) */
	/* 0x109 */ unsigned char sig[16];
	/* 0x119 */ unsigned char pad_119[51];
	/* 0x14C */ unsigned int sidx;  /* next byte of sig */
	/* 0x150 */ int pad_150;
};

struct dnas2_sess {
	/* 0x000 */ unsigned char unk_000[4];
	/* 0x004 */ int nbytes;
	/* 0x008 */ unsigned char pad_008[240];
	/* 0x0F8 */ unsigned int nrecs;
	/* 0x0FC */ unsigned int cur_rec;
	/* 0x100 */ struct dnas2_sess_rec recs[10];
	/* 0xE48 */ unsigned char key[32];
	/* 0xE68 */ unsigned char pad_E68[32];
	/* 0xE88 */ unsigned char buf[65536];
	/* 0x10E88 */ unsigned int head;  /* read position of the drain */
	/* 0x10E8C */ unsigned int tail;  /* write position */
	/* 0x10E90 */ unsigned char dec_key[16];
	/* 0x10EA0 */ unsigned char dec_iv[16];
	/* 0x10EB0 */ void *dec_ctx;
	/* 0x10EB4 */ unsigned char enc_key[16];
	/* 0x10EC4 */ unsigned char enc_iv[16];
	/* 0x10ED4 */ void *enc_ctx;
	/* 0x10ED8 */ unsigned char in_blk[16];  /* plaintext block being built */
	/* 0x10EE8 */ unsigned int in_len;
	/* 0x10EEC */ unsigned char out_blk[16];  /* block to encrypt into buf */
	/* 0x10EFC */ unsigned int out_len;
	/* 0x10F00 */ unsigned char hash_buf[64];  /* digest accumulator */
	/* 0x10F40 */ unsigned int hash_len;
	/* 0x10F44 */ int (*io)(void *, int);
};

static int deliver(unsigned char *dst, unsigned int cnt);
static struct dnas2_sess sess;
static int sess_once = 0;

/* Inline copies: one through generic pointers with an unsigned index, one
   by bytes with an int index. */
static __inline__ void dnas2_memcpy(void *dst, const void *src, unsigned int n)
{
	unsigned char *d = dst;
	const unsigned char *s = src;
	unsigned int i;

	for (i = 0; i < n; i++)
		d[i] = s[i];
}

static __inline__ void dnas2_cpy(unsigned char *d, const unsigned char *s, int n)
{
	int i;

	for (i = 0; i < n; i++)
		d[i] = s[i];
}

#define RB() ((unsigned char)(unsigned long)(rand() * (1.0 / 2147483648.0) * 256.0))

/* Opens the session: resets the state, checks the module (SCE_DNAS_ERR_BEFORE_SERVICE, SCE_DNAS_ERR_BUSY), then
 * reads the header packets through `io`, decrypts them with RSA key slots and
 * RC6 and checks their ASCII markers (SCE_DNAS2_ERR_MARKER on a mismatch).
 * Stores the stream length in *streamLen; the three strings are not used. */
int __sce_dnas2_symbol65041(unsigned char *vendor, unsigned char *title, unsigned char *subtitle,
    unsigned int *streamLen, int (*io)(void *, int))
{
	struct dnas2_sess *ses = &sess;
	unsigned char buf[128];
	unsigned char rec[128];
	unsigned int len;
	unsigned int tag;
	unsigned int tag2;
	int ret;
	int cmp;
	unsigned char *p;
	unsigned char *keyp;
	unsigned char *s;
	unsigned int k;
	struct dnas2_sess_rec *r;

	{
		unsigned int i;
		unsigned char *p = (unsigned char *)ses;
		for (i = 0; i < sizeof(*ses); i++)
			p[i] = 0;
	}

	__sce_dnas2_symbol12062();
	if (sess_once == 0) {
		ret = __sce_dnas2_symbol12068();
		if (ret)
			goto err;
		sess_once = 1;
	}
	if (__sce_dnas2_symbol12067() != 1) {
		ret = SCE_DNAS_ERR_BEFORE_SERVICE;
		goto err;
	}
	if (__sce_dnas2_symbol12063() <= 0) {
		ret = SCE_DNAS_ERR_BUSY;
		goto err;
	}

	ses->io = io;
	tag = 7;
	p = ses->buf;
	ret = ses->io(buf, 128);
	if (ret != 128)
		goto err;

	dnas2_cpy(p, buf, 128);
	p += 128;
	ses->tail += 128;
	ret = __sce_dnas2_symbol12206(buf, tag, &len, rec);
	if (ret)
		goto err;
	cmp = memcmp(rec + 36, "96011a8e95fd1ffc", 16);
	ret = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto err;
	s = rec;
	dnas2_memcpy(ses->key, s, 32);
	keyp = ses->key;
	s += 32;
	dnas2_memcpy(&ses->nbytes, s, 4);
	{
		unsigned int i;
		for (i = 0; i < sizeof(rec); i++)
			rec[i] = RB();
	}
	{
		unsigned int i;
		unsigned char *d = (unsigned char *)&tag;
		for (i = 0; i < sizeof(tag); i++)
			d[i] = RB();
	}

	ret = ses->io(buf, 128);
	if (ret != 128)
		goto err;
	dnas2_cpy(p, buf, 128);
	p += 128;

	ses->tail += 128;
	ret = ses->io(p, ses->nbytes);
	if (ses->nbytes != ret)
		goto err;
	p += ret;
	ses->tail += ret;
	ret = ses->io(p, 128);
	{
		int prev = ret;
		if (ret != 128)
			goto err;
		p += 128;
		ses->tail += 128;
		ret = ses->io(p, 128);
		if (ret != prev)
			goto err;
	}
	p += 128;
	ses->tail += 128;
	ret = __sce_dnas2_symbol65047(keyp, 128, p - 128, rec);
	if (ret)
		goto err;
	cmp = memcmp(rec + 10, "bb1ad0d4e569ab48", 16);
	ret = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto err;
	tag2 = rec[2];
	{
		unsigned int i;
		for (i = 0; i < sizeof(rec); i++)
			rec[i] = RB();
	}
	{
		unsigned int i;
		for (i = 0; i < sizeof(ses->key); i++)
			keyp[i] = RB();
	}
	p -= 256;
	ret = __sce_dnas2_symbol12206(p, tag2, &len, rec);
	if (ret)
		goto err;
	dnas2_memcpy(&ses->nrecs, rec, 4);
	cmp = memcmp(rec + (ses->nrecs * 8 + 4), "1ca31d99e4e74102", 16);
	ret = SCE_DNAS2_ERR_MARKER;
	if (cmp)
		goto err;
	{
		unsigned int i;
		unsigned char *d = (unsigned char *)&tag2;
		for (i = 0; i < sizeof(tag2); i++)
			d[i] = RB();
	}
	s = rec + 4;
	for (k = 0; k < ses->nrecs; k++) {
		r = &ses->recs[k];
		dnas2_memcpy(r->unk_0B0, s, 4);
		s += 4;
	}
	for (k = 0; k < ses->nrecs; k++) {
		r = &ses->recs[k];
		dnas2_memcpy(&r->size, s, 4);
		s += 4;
	}
	{
		unsigned int i;
		for (i = 0; i < sizeof(rec); i++)
			rec[i] = RB();
	}
	for (k = 0; k < ses->nrecs; k++) {
		r = &ses->recs[k];
		dnas2_memcpy(r->sig, "cfe9b0068e5c24d4", 16);
	}
	len = 0;
	for (k = 0; k < ses->nrecs; k++) {
		r = &ses->recs[k];
		len = len + r->size + 896;
	}
	len = len + (ses->nbytes + 512);
	*streamLen = len;
	return 0;

err:
	{
		int i;
		for (i = 0; i < 128; i++)
			rec[i] = RB();
	}
	{
		int i;
		for (i = 0; i < 128; i++)
			buf[i] = RB();
	}
	return ret;
}

/* Size of the delivered stream: per record size + 896, plus 512 and nbytes. */
int __sce_dnas2_symbol65040(void)
{
	struct dnas2_sess *ses = &sess;
	unsigned int k;
	int len = 0;
	for (k = 0; k < ses->nrecs; k++)
		len = len + ses->recs[k].size + 896;
	return len + (ses->nbytes + 512);
}

/*
 * Delivery round: while the caller has room, for each record not yet served
 * exchanges five 128-byte packets with the host through ses->io, verifies and
 * unpacks them, fills the record and the session buffers, then drains ses->buf
 * into the caller buffer. Every buffer that held sensitive data is refilled
 * with random bytes as soon as it is no longer needed.
 */
int __sce_dnas2_symbol65042(unsigned char *out, unsigned int n)
{
	unsigned char pkt[128];
	unsigned char plain[128];
	unsigned char keyPkt[128];
	unsigned char infoPkt[128];
	unsigned char encSeed[128];
	unsigned char wrapKey[32];
	unsigned int len;
	unsigned int hdrTag;
	unsigned int keyTag;
	unsigned int infoTag;
	unsigned int cnt = n;
	unsigned char *dst = out;
	struct dnas2_sess_rec *rec;
	unsigned char *p;
	unsigned char *q;
	unsigned char *s;
	int r;
	int cmp;
	int got1, got2, got3, got4, got5;
	unsigned int i;
	struct dnas2_sess *ses = &sess;

	while (cnt != 0) {
		if (ses->cur_rec == ses->nrecs)
			return -299;
		rec = &ses->recs[ses->cur_rec];
		if (rec->busy == 0) {
			rec->busy = 1;
			p = &ses->buf[ses->tail];
			r = ses->io(keyPkt, 128);
			got1 = r;
			if (r != 128)
				goto Lret;
			r = ses->io(p, 128);
			got2 = r;
			if (r != got1)
				goto Lret;
			p += 256;
			ses->tail += 256;
			r = ses->io(p, 128);
			got3 = r;
			if (r != got2)
				goto Lret;
			p += 128;
			hdrTag = 11;
			ses->tail += 128;
			r = ses->io(p, 128);
			if (r != got3)
				goto Lret;
			dnas2_cpy(pkt, p, 128);
			r = __sce_dnas2_symbol12206(pkt, hdrTag, &len, plain);
			if (r)
				goto Lret;
			s = plain + 32;
			cmp = memcmp(s, "d7ab6093e3900cb2", 16);
			r = SCE_DNAS2_ERR_MARKER;
			if (cmp)
				goto Lret;
			dnas2_memcpy(rec->hdr_key, plain, 32);
			q = p + 128;
			ses->tail += 128;
			for (i = 0; i < 128; i++)
				plain[i] = RB();
			for (i = 0; i < 128; i++)
				pkt[i] = RB();
			{
				unsigned char *d = (unsigned char *)&hdrTag;
				for (i = 0; i < 4; i++)
					d[i] = RB();
			}
			r = ses->io(q, 128);
			got4 = r;
			if (r != 128)
				goto Lret;
			p = q + 128;
			ses->tail += 128;
			r = ses->io(p, 128);
			if (r != got4)
				goto Lret;
			dnas2_cpy(infoPkt, p, 128);
			r = __sce_dnas2_symbol65047(rec->hdr_key, 128, infoPkt, plain);
			if (r)
				goto Lret;
			cmp = memcmp(plain + 10, "a5713c8bdbe8d420", 16);
			r = SCE_DNAS2_ERR_MARKER;
			if (cmp)
				goto Lret;
			p += 128;
			keyTag = plain[0];
			rec->sig_key = plain[2];
			infoTag = plain[4];
			ses->tail += 128;
			for (i = 0; i < 128; i++)
				plain[i] = RB();
			for (i = 0; i < 128; i++)
				infoPkt[i] = RB();
			for (i = 0; i < 32; i++)
				rec->hdr_key[i] = RB();
			r = __sce_dnas2_symbol12206(keyPkt, keyTag, &len, plain);
			if (r)
				goto Lret;
			{
				unsigned char *d = (unsigned char *)&keyTag;
				for (i = 0; i < 4; i++)
					d[i] = RB();
			}
			for (i = 0; i < 128; i++)
				keyPkt[i] = RB();
			cmp = memcmp(plain + 32, "980f3dc290f950bd", 16);
			r = SCE_DNAS2_ERR_MARKER;
			if (cmp)
				goto Lret;
			dnas2_memcpy(rec->dec_key, plain, 32);
			dnas2_cpy(ses->dec_key, rec->dec_key, 16);
			dnas2_cpy(ses->dec_iv, rec->dec_iv, 16);
			for (i = 0; i < 128; i++)
				plain[i] = RB();
			for (i = 0; i < 128; i++)
				keyPkt[i] = RB();
			for (i = 0; i < 32; i++)
				rec->dec_key[i] = RB();
			r = __sce_dnas2_symbol12237(ses->dec_key, ses->dec_iv, &ses->dec_ctx);
			if (r)
				goto Lret;
			for (i = 0; i < 16; i++)
				ses->dec_key[i] = RB();
			for (i = 0; i < 16; i++)
				ses->dec_iv[i] = RB();
			r = __sce_dnas2_symbol12206(p - 512, infoTag, &len, plain);
			if (r)
				goto Lret;
			{
				unsigned char *d = (unsigned char *)&infoTag;
				for (i = 0; i < 4; i++)
					d[i] = RB();
			}
			cmp = memcmp(plain + 11, "bdbf6d4db9be0aa0", 16);
			r = SCE_DNAS2_ERR_MARKER;
			if (cmp)
				goto Lret;
			{
				int v;
				int pad;
				s = plain;
				dnas2_memcpy(&rec->data_len, s, 4);
				s += 8;
				rec->skip_blocks = *s++;
				v = (rec->skip_blocks << 4) + rec->data_len + 16;
				pad = 16 - (v & 15);
				if (pad == 16)
					pad = 0;
				rec->unk_0B9 = *s++;
				rec->fill_blocks = *s;
				rec->skip_left = rec->skip_blocks;
				rec->body_len = v + pad;
				rec->fill_left = rec->fill_blocks;
				for (i = 0; i < 128; i++)
					plain[i] = RB();
				rec->align_pad = pad;
			}
			rec->hash_state = 1;
			for (i = 0; i < 128; i++)
				encSeed[i] = RB();
			r = __sce_dnas2_symbol12066(encSeed, rec->enc_key);
			if (r)
				goto Lret;
			r = __sce_dnas2_symbol12065(wrapKey);
			if (r)
				goto Lret;
			r = __sce_dnas2_symbol65048(wrapKey, 128, encSeed, p - 640);
			if (r)
				goto Lret;
			for (i = 0; i < 128; i++)
				encSeed[i] = RB();
			for (i = 0; i < 32; i++)
				wrapKey[i] = RB();
			dnas2_cpy(ses->enc_key, rec->enc_key, 16);
			dnas2_cpy(ses->enc_iv, rec->enc_iv, 16);
			r = __sce_dnas2_symbol12238(ses->enc_key, ses->enc_iv, &ses->enc_ctx);
			if (r)
				goto Lret;
			for (i = 0; i < 32; i++)
				rec->enc_key[i] = RB();
			rec->body_pos = 0;
			rec->left = rec->size;
		}

		if (ses->tail > ses->head) {
			p = &ses->buf[ses->head];

			for (i = ses->head; i < ses->tail; i++) {
				*dst++ = *p++;
				cnt--;
				ses->head++;
				if (cnt == 0)
					return n;
			}
			ses->head = 0;
			ses->tail = 0;
		}

		if (rec->busy == 1 && rec->left == 0 && ses->tail == 0) {
			unsigned int i;
			unsigned char *d = (unsigned char *)rec;
			for (i = 0; i < 4; i++)
				d[i] = 0;
			ses->cur_rec++;
		} else if (cnt < (unsigned int)rec->left) {
			r = deliver(dst, cnt);
			if ((int)cnt != r)
				goto Lret;
			dst += r;
			cnt = 0;
			rec->left -= r;
		} else {
			r = deliver(dst, rec->left);
			if (rec->left != r)
				goto Lret;
			dst += r;
			cnt -= r;
			rec->left = 0;

			r = ses->io(pkt, 128);
			got5 = r;
			if (r != 128)
				goto Lret;
			r = __sce_dnas2_symbol12206(pkt, rec->sig_key, &len, plain);
			if (r)
				goto Lret;
			cmp = memcmp(plain + 20, "30fad789bb372f79", 16);
			r = SCE_DNAS2_ERR_MARKER;
			if (cmp)
				goto Lret;
			__sce_dnas2_symbol12150(ses->hash_buf, ses->hash_len, rec->digest, rec->body_len, 4);
			cmp = memcmp(rec->digest, plain, 20);
			r = SCE_DNAS2_ERR_DIGEST;
			if (cmp)
				goto Lret;
			ses->hash_len = 0;
			if (ses->enc_ctx) {
				r = __sce_dnas2_symbol12256(ses->enc_ctx);
				if (r)
					return r;
				ses->enc_ctx = 0;
			}
			if (ses->dec_ctx) {
				r = __sce_dnas2_symbol12255(ses->dec_ctx);
				if (r)
					return r;
				ses->dec_ctx = 0;
			}
			r = ses->io(&ses->buf[ses->tail], 128);
			if (r != got5)
				return r;
			ses->tail += 128;
		}
	}
	return n;

Lret:
	return r;
}

/*
 * Hands cnt bytes of the session stream to the caller. Drains ses->buf first,
 * then advances the current record by one step: 16-byte blocks read through
 * ses->io and decrypted, random padding blocks, single bytes of the body or of
 * the signed tail, gathered in in_blk. Each full in_blk is decrypted into
 * out_blk, where the state machine separates the end marker (end_mark) from the
 * text; each full out_blk is encrypted at the tail of ses->buf.
 */
#define NEXT_SIG(c) \
	if (rec->sidx < 16) \
		(c) = rec->sig[rec->sidx++]; \
	else \
		(c) = RB()

static int deliver(unsigned char *out, unsigned int cnt)
{
	unsigned int n;
	unsigned int lim = 100000;
	/* block size, end marker and its length */
	unsigned int bs = 16;
	const char *magic = "4571cd4f3dce5a9b";
	unsigned int mlen = 16;
	struct dnas2_sess *ses = &sess;
	unsigned char *dst = out;
	struct dnas2_sess_rec *rec = &ses->recs[ses->cur_rec];
	unsigned char c;
	unsigned char *q;
	unsigned char *p;
	unsigned int i;
	int r;
	int cmp;
	unsigned int carry, filled;

	n = cnt;

	while (n != 0) {
		if (ses->tail > ses->head) {
			q = &ses->buf[ses->head];

			for (i = ses->head; i < ses->tail; i++) {
				*dst++ = *q++;
				n--;
				ses->head++;
				if (n == 0)
					return cnt;
			}
			ses->head = 0;
			ses->tail = 0;
		}
		if (rec->skip_left != 0) {
			p = ses->in_blk;
			if (rec->skip_left * 16 != 0) {
				r = ses->io(p, bs);
				if (r != bs)
					goto fail;
				for (i = 0; i < 16; i++) {
					ses->hash_buf[ses->hash_len++] = p[i];
					if (ses->hash_len == 64) {
						__sce_dnas2_symbol12150(ses->hash_buf, 64, rec->digest, rec->body_len,
						    rec->hash_state);
						rec->hash_state = 2;
						ses->hash_len = 0;
					}
				}
				r = __sce_dnas2_symbol12251(ses->dec_ctx, p, p, 1);
				if (r)
					goto fail;
				rec->skip_left--;
				rec->body_pos += bs;
			}
			continue;
		}
		if (rec->fill_left != 0) {
			unsigned char *w;

			if (rec->fill_left * 16 != 0) {
				for (i = 0, w = p = ses->out_blk; i < bs; i++)
					*w++ = RB();
				r = __sce_dnas2_symbol12253(ses->enc_ctx, p, &ses->buf[ses->tail], 1);
				if (r)
					goto fail;
				ses->tail += bs;
				rec->fill_left--;
			}
			continue;
		}
		if (rec->body_pos < rec->body_len) {
			r = ses->io(&c, 1);
			if (r != 1)
				goto fail;
			if (rec->body_pos >= rec->skip_blocks * 16 + rec->data_len && rec->state == 0) {
				rec->state = 1;
				lim = ses->in_len;
			}
			rec->body_pos++;
			ses->in_blk[ses->in_len++] = c;
			ses->hash_buf[ses->hash_len++] = c;
			if (ses->hash_len == 64) {
				__sce_dnas2_symbol12150(ses->hash_buf, 64, rec->digest, rec->body_len, rec->hash_state);
				rec->hash_state = 2;
				ses->hash_len = 0;
			}
		} else {
			if (rec->pad_used > 16) {
				r = -1;
				goto fail;
			}
			if (rec->sidx < 16) {
				c = rec->sig[rec->sidx++];
			} else {
				c = RB();
				rec->pad_used++;
			}
			ses->in_blk[ses->in_len++] = c;
		}
		if (ses->in_len != bs)
			continue;

		q = &ses->buf[ses->tail];
		if (rec->state < 3) {
			r = __sce_dnas2_symbol12251(ses->dec_ctx, ses->in_blk, ses->in_blk, 1);
			if (r)
				goto fail;
		}
		ses->in_len = 0;
		if (rec->unk_0BB != 0) {
			ses->in_len = rec->unk_0BB;
			rec->unk_0BB = 0;
		}
		carry = rec->unk_0BB;
		filled = ses->out_len;
		if (filled < carry)
			i = carry;
		else
			i = filled;
		p = ses->out_blk;
		for (; i < bs; i++) {
			if (rec->state == 0) {
				p[i] = ses->in_blk[ses->in_len++];
			} else if (rec->state == 1) {
				if (ses->in_len < lim) {
					p[ses->out_len++] = ses->in_blk[ses->in_len++];
				} else {
					NEXT_SIG(c);
					p[ses->out_len++] = c;
					rec->end_mark[rec->end_len++] = ses->in_blk[ses->in_len++];
				}
			} else if (rec->state == 2) {
				NEXT_SIG(c);
				p[ses->out_len++] = c;
				rec->end_mark[rec->end_len++] = ses->in_blk[ses->in_len++];
				if (rec->end_len == mlen) {
					cmp = memcmp(rec->end_mark, magic, mlen);
					r = SCE_DNAS2_ERR_MARKER;
					if (cmp)
						goto fail;
					rec->state = 3;
				}
			} else if (rec->state == 3) {
				NEXT_SIG(c);
				p[ses->out_len++] = c;
			}
		}
		if (rec->end_len != 0 && rec->state == 1)
			rec->state = 2;
		r = __sce_dnas2_symbol12253(ses->enc_ctx, p, q, 1);
		if (r)
			goto fail;
		ses->out_len = 0;
		ses->tail += bs;
		for (i = ses->in_len; i < bs; i++) {
			if (rec->state == 0 || (rec->state == 1 && ses->in_len < lim)) {
				p[ses->out_len++] = ses->in_blk[ses->in_len++];
			} else if (rec->state == 1) {
				NEXT_SIG(c);
				p[ses->out_len++] = c;
				rec->end_mark[rec->end_len++] = ses->in_blk[ses->in_len++];
			} else if (rec->state == 2) {
				NEXT_SIG(c);
				p[ses->out_len++] = c;
				rec->end_mark[rec->end_len++] = ses->in_blk[ses->in_len++];
				if (rec->end_len == mlen) {
					cmp = memcmp(rec->end_mark, magic, mlen);
					r = SCE_DNAS2_ERR_MARKER;
					if (cmp)
						goto fail;
					rec->state = 3;
				}
			} else if (rec->state == 3) {
				NEXT_SIG(c);
				p[ses->out_len++] = c;
			}
		}
		if (rec->end_len != 0 && rec->state == 1)
			rec->state = 2;
		ses->in_len = 0;
	}
	return cnt - n;

fail:
	return r;
}
