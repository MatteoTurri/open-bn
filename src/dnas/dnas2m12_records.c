/*
 * DNAS2 module 12: one-time initialization, session record builders and their
 * key material. Bodies run inside self-modifying regions bounded by
 * SCE_DNAS_PROT_BEGIN/END (dnas_prot.h).
 */
#define SCE_DNAS2_SHIM __sce_dnas2_symbol12002	/* module protection shim, see dnas_prot.h */
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "dnas_err.h"
#include "dnas_prot.h"
#include "dnas2m12_des.h"
#include "dnas2m12_identity.h"
#include "dnas2m12_records.h"
#include "dnas2m12_slots.h"
#include "dnas2m12_rand.h"

/* file-local helpers defined below */
static int dnas2_decrypt_offset(void *enc, unsigned int *out);

/* DES-decrypts the n blocks of the encrypted name enc into buf with the fixed name key. */
static int dnas2_m12_decrypt_name(void *enc, void *buf, int n)
{
	char key[8] = {
#include "../../assets/dnas/dnas2m12_records_name_key.inc"
	};

	return __sce_dnas2_symbol12060(enc, key, buf, n);
}

/* Decrypts the memory device ("/dev/mem", two blocks) into buf: mapped to read the
 * identity block. */
static int dnas2_m12_mem_device_name(void *buf)
{
	char enc[16] = {
#include "../../assets/dnas/dnas2m12_records_enc_mem_device_name.inc"
	};
	int r;

	SCE_DNAS_PROT_BEGIN();
	r = dnas2_m12_decrypt_name(enc, buf, 2);
	SCE_DNAS_PROT_END();
	return r;
}

/* Initializes the module once (__sce_dnas2_symbol12062). */
int __sce_dnas2_symbol12068(void)
{
	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 == 0) {
		__sce_dnas2_symbol12062();
		__sce_dnas2_symbol12074 = 1;
	}
	SCE_DNAS_PROT_END();
	return 0;
}

/* Unprotected entry point of __sce_dnas2_symbol12076. */
int __sce_dnas2_symbol12070(void)
{
	return __sce_dnas2_symbol12076();
}

/* Unprotected entry point of __sce_dnas2_symbol12073. */
int __sce_dnas2_symbol12067(void)
{
	return __sce_dnas2_symbol12073();
}

/* Unprotected entry point of __sce_dnas2_symbol12072. */
int __sce_dnas2_symbol12064(void *p)
{
	return __sce_dnas2_symbol12072((unsigned char *)p);
}

/* Unprotected entry point of __sce_dnas2_symbol12071. */
int __sce_dnas2_symbol12063(void)
{
	return __sce_dnas2_symbol12071();
}

/* Key material of __sce_dnas2_symbol12066: key and IV of the last step, key and IV
 * of the step-key schedule, encrypted chain of steps. */
static const unsigned char dnas2_blkrec_last_key[8] = {
#include "../../assets/dnas/dnas2m12_records_blkrec_last_key.inc"
};
static const unsigned char dnas2_blkrec_last_iv[8] = {
#include "../../assets/dnas/dnas2m12_records_blkrec_last_iv.inc"
};
static const unsigned char dnas2_blkrec_sched_key[8] = {
#include "../../assets/dnas/dnas2m12_records_blkrec_sched_key.inc"
};
static const unsigned char dnas2_blkrec_sched_iv[8] = {
#include "../../assets/dnas/dnas2m12_records_blkrec_sched_iv.inc"
};
static const unsigned char dnas2_blkrec_chain[8] = {
#include "../../assets/dnas/dnas2m12_records_blkrec_chain.inc"
};

static __inline__ void dnas2_cp11(unsigned char *d, const unsigned char *s, int n)
{
	int i;

	for (i = 0; i < n; i++)
		d[i] = s[i];
}

static int dnas2_m12_read_id_block(void *arg);
static int dnas2_m12_walk_chain(unsigned char *sched, void *rec, void *lastKey, void *lastIv,
    const unsigned char *chain, unsigned char *blob, void *id);

/*
 * Builds the 168-byte record to send. Needs an open session (else -8); each of
 * the three preparation helpers (descriptor, 64-byte block, 4-byte id) returns
 * SCE_DNAS_ERR_ID_FAILED on failure. The record concatenates 32 bytes of blk, the id, 32 of
 * blk + 32, 32 of the block (+24), 32 of blk + 64, the id again and 32 of
 * blk + 96; it is signed and packed, and its last 32 bytes go to out. All
 * local buffers are wiped.
 */
int __sce_dnas2_symbol12066(unsigned char *blk, unsigned char *out)
{
	int r;
	unsigned char lastKey[16];
	unsigned char lastIv[16];
	unsigned char schedKey[16];
	unsigned char schedIv[16];
	unsigned char chain[16];
	unsigned char encChain[16];
	unsigned char rec[176];
	unsigned char sched[128];
	unsigned char blob[64];
	unsigned char id[16];
	unsigned int m;
	const unsigned char *src;

	src = dnas2_blkrec_last_key;
	memcpy(lastKey, src, 8);
	src = dnas2_blkrec_last_iv;
	memcpy(lastIv, src, 8);
	src = dnas2_blkrec_sched_key;
	memcpy(schedKey, src, 8);
	src = dnas2_blkrec_sched_iv;
	memcpy(schedIv, src, 8);
	src = dnas2_blkrec_chain;
	memcpy(encChain, src, 8);
	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 != 1) {
		r = -8;
	} else if (dnas2_m12_decrypt_name(encChain, chain, 1) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else if (dnas2_m12_read_id_block(blob) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else if (__sce_dnas2_symbol12072(id) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else {
		dnas2_cp11(rec, blk, 32);
		dnas2_cp11(rec + 32, id, 4);
		dnas2_cp11(rec + 36, blk + 32, 32);
		dnas2_cp11(rec + 68, blob + 24, 32);
		dnas2_cp11(rec + 100, blk + 64, 32);
		dnas2_cp11(rec + 132, id, 4);
		dnas2_cp11(rec + 136, blk + 96, 32);
		if (__sce_dnas2_symbol12059(blk, schedKey, sched, schedIv, 16) < 0) {
			r = SCE_DNAS_ERR_ID_FAILED;
		} else if (dnas2_m12_walk_chain(sched, rec, lastKey, lastIv, chain, blob, id) < 0) {
			r = SCE_DNAS_ERR_ID_FAILED;
		} else {
			dnas2_cp11(out, rec + 136, 32);
			r = 0;
		}
	}
	for (m = 0; m < 168; m++)
		rec[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 128; m++)
		sched[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 56; m++)
		blob[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 4; m++)
		id[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 8; m++)
		chain[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Same, for __sce_dnas2_symbol12065. */
static const unsigned char dnas2_idrec_last_key[8] = {
#include "../../assets/dnas/dnas2m12_records_idrec_last_key.inc"
};
static const unsigned char dnas2_idrec_last_iv[8] = {
#include "../../assets/dnas/dnas2m12_records_idrec_last_iv.inc"
};
static const unsigned char dnas2_idrec_sched_key[8] = {
#include "../../assets/dnas/dnas2m12_records_idrec_sched_key.inc"
};
static const unsigned char dnas2_idrec_sched_iv[8] = {
#include "../../assets/dnas/dnas2m12_records_idrec_sched_iv.inc"
};
static const unsigned char dnas2_idrec_chain[8] = {
#include "../../assets/dnas/dnas2m12_records_idrec_chain.inc"
};

/*
 * Same preparation and wipes as __sce_dnas2_symbol12066, with a different
 * 168-byte record layout (short interleaved copies of id and block, two
 * 40-byte repeats and a 6-byte tail).
 */
int __sce_dnas2_symbol12065(unsigned char *out)
{
	int r;
	unsigned char lastKey[16];
	unsigned char lastIv[16];
	unsigned char schedKey[16];
	unsigned char schedIv[16];
	unsigned char chain[16];
	unsigned char encChain[16];
	unsigned char rec[176];
	unsigned char sched[128];
	unsigned char blob[64];
	unsigned char id[16];
	unsigned int m;
	const unsigned char *src;

	src = dnas2_idrec_last_key;
	memcpy(lastKey, src, 8);
	src = dnas2_idrec_last_iv;
	memcpy(lastIv, src, 8);
	src = dnas2_idrec_sched_key;
	memcpy(schedKey, src, 8);
	src = dnas2_idrec_sched_iv;
	memcpy(schedIv, src, 8);
	src = dnas2_idrec_chain;
	memcpy(encChain, src, 8);
	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 != 1) {
		r = -8;
	} else if (dnas2_m12_decrypt_name(encChain, chain, 1) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else if (dnas2_m12_read_id_block(blob) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else if (__sce_dnas2_symbol12072(id) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else {
		dnas2_cp11(rec, id, 2);
		dnas2_cp11(rec + 2, blob + 24, 4);
		dnas2_cp11(rec + 6, id + 2, 2);
		dnas2_cp11(rec + 8, blob + 28, 4);
		dnas2_cp11(rec + 12, id + 3, 1);
		dnas2_cp11(rec + 13, blob + 32, 4);
		dnas2_cp11(rec + 17, id + 2, 1);
		dnas2_cp11(rec + 18, blob + 36, 4);
		dnas2_cp11(rec + 22, id + 1, 1);
		dnas2_cp11(rec + 23, blob + 40, 4);
		dnas2_cp11(rec + 27, id, 1);
		dnas2_cp11(rec + 28, blob + 44, 4);
		dnas2_cp11(rec + 32, id, 4);
		dnas2_cp11(rec + 36, blob + 48, 4);
		dnas2_cp11(rec + 40, id + 2, 2);
		dnas2_cp11(rec + 42, blob + 52, 4);
		dnas2_cp11(rec + 46, blob + 24, 32);
		dnas2_cp11(rec + 78, id, 4);
		dnas2_cp11(rec + 82, rec + 40, 40);
		dnas2_cp11(rec + 122, rec, 40);
		dnas2_cp11(rec + 162, blob + 32, 6);
		if (__sce_dnas2_symbol12059(rec, schedKey, sched, schedIv, 16) < 0) {
			r = SCE_DNAS_ERR_ID_FAILED;
		} else if (dnas2_m12_walk_chain(sched, rec, lastKey, lastIv, chain, blob, id) < 0) {
			r = SCE_DNAS_ERR_ID_FAILED;
		} else {
			dnas2_cp11(out, rec + 136, 32);
			r = 0;
		}
	}
	for (m = 0; m < 168; m++)
		rec[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 128; m++)
		sched[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 56; m++)
		blob[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 4; m++)
		id[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 8; m++)
		chain[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Two DES keys: the first, encrypted with the second, decrypts the pointer to
 * the identity block. */
static const unsigned char dnas2_ptr_keys[16] = {
#include "../../assets/dnas/dnas2m12_records_ptr_keys.inc"
};
/* Device offset to map and offset in the map of the encrypted pointer to the
 * identity block, DES-encrypted (one block each). */
static unsigned char dnas2_enc_map_off[8] = {
#include "../../assets/dnas/dnas2m12_records_enc_map_off.inc"
};
static unsigned char dnas2_enc_ptr_off[8] = {
#include "../../assets/dnas/dnas2m12_records_enc_ptr_off.inc"
};

static int dnas2_m12_read_id_block(void *arg)
{
	int r;
	unsigned char path[16];
	unsigned char key[16];
	unsigned char ptrKey[16];
	unsigned char ptrBuf[16];
	int off1;
	int off2;
	int fd;
	unsigned char *map;
	int i;
	unsigned int z;
	unsigned int off;
	unsigned char *ptrKeys, *encPtr, *ptr;
	const unsigned char *keySrc;

	fd = -1;
	map = 0;
	keySrc = dnas2_ptr_keys;
	memcpy(key, keySrc, 16);
	SCE_DNAS_PROT_BEGIN();
	if (dnas2_decrypt_offset(dnas2_enc_map_off, &off1) < 0) {
		r = -1;
	} else if (dnas2_decrypt_offset(dnas2_enc_ptr_off, &off2) < 0) {
		r = -1;
	} else if (dnas2_m12_mem_device_name(path) < 0) {
		r = -1;
	} else if ((fd = open((const char *)path, O_RDWR)) < 0) {
		r = -1;
	} else if ((map = (unsigned char *)mmap(0, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, off1)) == 0) {
		r = -1;
	} else {
		ptrKeys = key;
		encPtr = map + off2;
		if (__sce_dnas2_symbol12061(ptrKeys, key + 8, ptrKey, 1) < 0) {
			r = -1;
		} else if (__sce_dnas2_symbol12060(encPtr, ptrKey, ptrBuf, 1) < 0) {
			r = -1;
		} else {
			off = *(unsigned int *)ptrBuf ^ *(unsigned int *)(ptrBuf + 4);
			for (i = 0; i < 16; i++)
				ptrKeys[i] =
				    (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
			i = 0;
			ptr = map + off;
			for (; i < 8; i++)
				ptrKey[i] =
				    (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
			for (i = 0; i < 8; i++)
				ptrBuf[i] =
				    (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
			if (__sce_dnas2_symbol12077(arg, ptr, 256, 16384) < 0) {
				r = -1;
			} else if (munmap(map, 0x200000) < 0) {
				r = -1;
			} else {
				map = 0;
				if (close(fd) < 0) {
					r = -1;
				} else {
					fd = -1;
					r = 0;
				}
			}
		}
	}
	for (z = 0; z < 16; z++)
		path[z] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	if (map != 0)
		munmap(map, 0x200000);
	if (fd >= 0)
		close(fd);
	off1 = 0;
	off2 = 0;
	SCE_DNAS_PROT_END();
	return r;
}

/*
 * Walks a chain of states (chain[state] is the next state, 0 ends it); each
 * state signs or verifies its 16-byte record sched[state]. States 0 and 5 first
 * process extra blob fields. A last step processes (lastKey, lastIv). Returns 0, or -1
 * at the first error.
 */
static int dnas2_m12_walk_chain(unsigned char *sched, void *rec, void *lastKey, void *lastIv,
    const unsigned char *chain, unsigned char *blob, void *id)
{
	int state;
	int r;

	SCE_DNAS_PROT_BEGIN();
	state = chain[0];
loop:
	{
		switch (state) {
		case 0:
			r = __sce_dnas2_symbol12058(rec, blob + 40, rec, blob + 32, 21);
			if (r < 0) {
				state = -1;
				goto out;
			}
		case 1:
		case 2:
		case 3:
			r = __sce_dnas2_symbol12059(rec, sched + ((state << 4) + 8), rec, sched + (state << 4), 21);
			if (r < 0) {
				state = -1;
				goto out;
			}
			break;
		case 5:
			r = __sce_dnas2_symbol12059(rec, blob + 32, rec, blob + 24, 21);
			if (r < 0) {
				state = -1;
				goto out;
			}
		case 4:
		case 6:
		case 7:
			r = __sce_dnas2_symbol12058(rec, sched + ((state << 4) + 8), rec, sched + (state << 4), 21);
			if (r < 0) {
				state = -1;
				goto out;
			}
			break;
		}
		if (state == 0)
			goto done;
		state = chain[state];
		goto loop;
	}
done:
	r = __sce_dnas2_symbol12059(rec, lastKey, rec, lastIv, 21);
	state = (r < 0) ? -1 : 0;
out:
	SCE_DNAS_PROT_END();
	return state;
}

/* Unprotected entry point of __sce_dnas2_symbol12075. */
int __sce_dnas2_symbol12069(const char *name, char *buf, unsigned int maxlen, int *outlen)
{
	return __sce_dnas2_symbol12075(name, buf, maxlen, outlen);
}

static void dnas2_be32_get(unsigned int *out, const unsigned char *p)
{
	*out = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

/* DES key of the encrypted offsets. */
static unsigned char dnas2_offset_key[8] = {
#include "../../assets/dnas/dnas2m12_records_offset_key.inc"
};

/* Decrypts a word with the module DES key, stores it big-endian in *out, wipes the buffer. */
static int dnas2_decrypt_offset(void *enc, unsigned int *out)
{
	unsigned char buf[16];
	unsigned char *w;
	int r;
	int i;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12060(enc, dnas2_offset_key, buf, 1) < 0) {
		r = -1;
	} else {
		dnas2_be32_get(out, buf);
		w = buf;
		for (i = 0; i < 8; i++)
			w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
		r = 0;
	}
	SCE_DNAS_PROT_END();
	return r;
}
