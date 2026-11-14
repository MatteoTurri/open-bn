/* DNAS2 module 11: protected key-slot decoders and their encrypted tables. */
#define SCE_DNAS2_SHIM __sce_dnas2_symbol11002
#include <stdlib.h>

#include "dnas_prot.h"
#include "dnas2m11_des.h"
#include "dnas2m11_slots.h"
#include "dnas2m11_twopass.h"

/*
 * Data assets: the wrapped keys and the DES key of the two slot decoders, the DES
 * key of the slot tables, and the encrypted triple-DES keys of slots 2 to 5.
 */
static char dnas2_dec_a_keys[32] = {
#include "../../assets/dnas/dnas2m11_slots_dec_a_keys.inc"
};
static char dnas2_dec_a_key[8] = {
#include "../../assets/dnas/dnas2m11_slots_dec_a_key.inc"
};
static char dnas2_dec_b_keys[32] = {
#include "../../assets/dnas/dnas2m11_slots_dec_b_keys.inc"
};
static char dnas2_dec_b_key[8] = {
#include "../../assets/dnas/dnas2m11_slots_dec_b_key.inc"
};
static char dnas2_slot_key[8] = {
#include "../../assets/dnas/dnas2m11_slots_slot_key.inc"
};
static char dnas2_slot2_keys[32] = {
#include "../../assets/dnas/dnas2m11_slots_slot2_keys.inc"
};
static char dnas2_slot3_keys[32] = {
#include "../../assets/dnas/dnas2m11_slots_slot3_keys.inc"
};
static char dnas2_slot4_keys[32] = {
#include "../../assets/dnas/dnas2m11_slots_slot4_keys.inc"
};
static char dnas2_slot5_keys[32] = {
#include "../../assets/dnas/dnas2m11_slots_slot5_keys.inc"
};

/* Reads a big-endian 32-bit word. */
static void dnas2_be32_get(unsigned int *out, const unsigned char *p)
{
	*out = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

/* Decrypts `entry` of the slot table at blob + pos + 120 (triple-DES key from
 * keyTable) and stores in *hdl the slot data address blob + pos + 384 + 128 * entry;
 * -1 if it ends past blob + size. */
static int dnas2_open_slot(int *hdl, unsigned char *blob, unsigned int pos, unsigned int size, int entry,
    void *keyTable)
{
	unsigned char block[8];
	unsigned char keys[4][8];
	unsigned int n;
	unsigned char *pk, *pb;
	unsigned int base, q, lim, off, len;
	int r;
	int i;

	SCE_DNAS_PROT_BEGIN();
	base = (unsigned int)blob + pos;
	pk = keys[0];
	off = (entry << 3) + 120;
	q = base + off;
	if (__sce_dnas2_symbol11059(keyTable, dnas2_slot_key, pk, 4) < 0) {
		r = -1;
	} else if (__sce_dnas2_symbol11088((void *)q, pk, keys[1], keys[2], block, keys[3], 1) < 0) {
		r = -1;
	} else {
		dnas2_be32_get(&n, block);
		len = (n << 7) + 384;
		lim = base + len;
		r = ((unsigned int)blob + size < lim + 8) ? -1 : 0;
		*hdl = lim;
	}
	pb = block;
	for (i = 0; i < 8; i++)
		pb[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	for (i = 0; i < 32; i++)
		((unsigned char *)keys)[i] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Decodes key slot 2 of the blob (blob, pos, size) into `out` with
 * __sce_dnas2_symbol11155; -1 if the slot is out of range. */
int __sce_dnas2_symbol11081(unsigned char *out, unsigned char *blob, unsigned int pos, unsigned int size)
{
	int r;
	unsigned char buf[128];
	int hdl;
	unsigned int len;
	unsigned int i;
	int j;
	unsigned char *q, *w;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_open_slot(&hdl, blob, pos, size, 2, dnas2_slot2_keys) < 0) {
		r = -1;
	} else {
		__sce_dnas2_symbol11155(buf, 128, (int *)&len, (unsigned char *)hdl, 72, dnas2_dec_a_keys,
		    dnas2_dec_a_key);
		q = buf;
		for (i = 0; i < len; i++)
			out[i] = q[i];
		r = 0;
	}
	w = buf;
	for (j = 0; j < 128; j++)
		w[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Decodes key slot 3 of the blob (blob, pos, size) into `out` with
 * __sce_dnas2_symbol11155; -1 if the slot is out of range. */
int __sce_dnas2_symbol11082(unsigned char *out, unsigned char *blob, unsigned int pos, unsigned int size)
{
	int r;
	unsigned char buf[128];
	int hdl;
	unsigned int len;
	unsigned int i;
	int j;
	unsigned char *q, *w;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_open_slot(&hdl, blob, pos, size, 3, dnas2_slot3_keys) < 0) {
		r = -1;
	} else {
		__sce_dnas2_symbol11155(buf, 128, (int *)&len, (unsigned char *)hdl, 88, dnas2_dec_a_keys,
		    dnas2_dec_a_key);
		q = buf;
		for (i = 0; i < len; i++)
			out[i] = q[i];
		r = 0;
	}
	w = buf;
	for (j = 0; j < 128; j++)
		w[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Decodes key slot 4 of the blob (blob, pos, size) into `out` with
 * __sce_dnas2_symbol11156; -1 if the slot is out of range. */
int __sce_dnas2_symbol11083(unsigned char *out, unsigned char *blob, unsigned int pos, unsigned int size)
{
	int r;
	unsigned char buf[128];
	int hdl;
	unsigned int len;
	unsigned int i;
	int j;
	unsigned char *q, *w;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_open_slot(&hdl, blob, pos, size, 4, dnas2_slot4_keys) < 0) {
		r = -1;
	} else {
		__sce_dnas2_symbol11156(buf, 128, (int *)&len, (unsigned char *)hdl, 64, dnas2_dec_b_keys,
		    dnas2_dec_b_key);
		q = buf;
		for (i = 0; i < len; i++)
			out[i] = q[i];
		r = 0;
	}
	w = buf;
	for (j = 0; j < 128; j++)
		w[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Decodes key slot 5 of the blob (blob, pos, size) into `out` with
 * __sce_dnas2_symbol11156; -1 if the slot is out of range. */
int __sce_dnas2_symbol11084(unsigned char *out, unsigned char *blob, unsigned int pos, unsigned int size)
{
	int r;
	unsigned char buf[128];
	int hdl;
	unsigned int len;
	unsigned int i;
	int j;
	unsigned char *q, *w;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_open_slot(&hdl, blob, pos, size, 5, dnas2_slot5_keys) < 0) {
		r = -1;
	} else {
		__sce_dnas2_symbol11156(buf, 128, (int *)&len, (unsigned char *)hdl, 72, dnas2_dec_b_keys,
		    dnas2_dec_b_key);
		q = buf;
		for (i = 0; i < len; i++)
			out[i] = q[i];
		r = 0;
	}
	w = buf;
	for (j = 0; j < 128; j++)
		w[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Stores the limit base + len + 127; returns -1 if base + used is below it. */
int __sce_dnas2_symbol11085(unsigned char **out, unsigned char *base, unsigned int len, unsigned int used)
{
	int r;
	unsigned char *lim;

	SCE_DNAS_PROT_BEGIN();
	lim = base + len + 127;
	r = (base + used < lim) ? -1 : 0;
	*out = lim;
	SCE_DNAS_PROT_END();
	return r;
}

/* Derives 24 bytes into `out` from the 512 bytes before p + n. */
int __sce_dnas2_symbol11158(unsigned char *out, unsigned char *p, int n)
{
	unsigned char buf[128];
	int len;
	unsigned int i;
	int j;
	unsigned char *q, *w;

	SCE_DNAS_PROT_BEGIN();
	__sce_dnas2_symbol11155(buf, 128, &len, p + n - 512, 56, dnas2_dec_a_keys, dnas2_dec_a_key);
	q = buf;
	for (i = 0; i < 24; i++)
		out[i] = q[i];
	w = buf;
	for (j = 0; j < 128; j++)
		w[j] = (unsigned char)((double)rand() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return 0;
}
