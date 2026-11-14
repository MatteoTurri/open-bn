/*
 * DNAS2 module 12 protected functions: identity block reader, key-slot
 * handling and PFS file helpers; counterparts of the module 11 functions in
 * dnas2m11_identity.c. Bodies run inside self-modifying regions bounded by
 * SCE_DNAS_PROT_BEGIN/END (dnas_prot.h).
 */
#define SCE_DNAS2_SHIM __sce_dnas2_symbol12002	/* module protection shim, see dnas_prot.h */
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "dnas_err.h"
#include "dnas_prot.h"
#include "lib/scepfsd.h"
#include "dnas2m12_des.h"
#include "dnas2m12_signed_msg.h"
#include "dnas2m12_blocks.h"
#include "dnas2m12_identity.h"
#include "dnas2m12_slots.h"
#include "dnas2m12_rand.h"
#include "dnas2m12_rsa.h"

#include <string.h>

/*
 * Data assets. Offsets are stored DES-encrypted (one block) and decrypted with
 * dnas2_offset_key: the device offset to map, the offset in the map of the
 * encrypted pointer to the identity block, and the offsets in the key file of
 * the 128-byte record and of the 16-byte key slot.
 */
static unsigned char dnas2_enc_map_off[8] = {
#include "../../assets/dnas/dnas2m12_identity_enc_map_off.inc"
};

/* DES key and encrypted RSA public key of the key-file record. */
static unsigned char dnas2_rec_pubkey_key[8] = {
#include "../../assets/dnas/dnas2m12_identity_rec_pubkey_key.inc"
};

static unsigned char dnas2_rec_pubkey[162] = {
#include "../../assets/dnas/dnas2m12_identity_rec_pubkey.inc"
};

/* Tag appended to the identity record before it is signed. */
static unsigned char dnas2_id_tag[8] = {
#include "../../assets/dnas/dnas2m12_identity_id_tag.inc"
};

/* Source and DES key of the two keys that sign the identity record. */
static unsigned char dnas2_sig_key_src[16] = {
#include "../../assets/dnas/dnas2m12_identity_sig_key_src.inc"
};

static unsigned char dnas2_sig_key_mask[8] = {
#include "../../assets/dnas/dnas2m12_identity_sig_key_mask.inc"
};

/* Set to 1 once the module is initialized. */
int __sce_dnas2_symbol12074 = 0;

/* Two DES keys: the first, encrypted with the second, decrypts the pointer to
 * the identity block. */
static const unsigned char dnas2_ptr_keys[16] = {
#include "../../assets/dnas/dnas2m12_identity_ptr_keys.inc"
};

/* file-local functions defined below */
static int dnas2_decrypt_offset(void *enc, unsigned int *out);
static int dnas2_m12_keyfile_read(unsigned char *out, unsigned int outlen, unsigned int n, int off);
static int dnas2_m12_keyfile_write(unsigned char *in, unsigned int inlen, unsigned int n, int off);
static int dnas2_m12_flag_file_name(void *buf);
static int dnas2_m12_data_dir_name(void *buf);
static int dnas2_m12_mem_device_name(void *buf);
static int dnas2_m12_exists(const char *name);
static int dnas2_m12_write_file(const char *name, void *buf, int len);
static int dnas2_m12_read_file(const char *name, char *buf, unsigned int maxlen, int *outlen);
static int dnas2_m12_remove(const char *name);
static int dnas2_m12_mkdir(const char *name);
static int dnas2_m12_rmdir_all(const char *name);
static int dnas2_m12_read_id_block(void *arg);
static int dnas2_memcmp(const unsigned char *a, const unsigned char *b, unsigned int n);

static unsigned char dnas2_enc_rec_off[8] = {
#include "../../assets/dnas/dnas2m12_identity_enc_rec_off.inc"
};

static unsigned char dnas2_enc_slot_off[8] = {
#include "../../assets/dnas/dnas2m12_identity_enc_slot_off.inc"
};

/* Source and DES key of the three keys and IV of the key-file cipher. */
static unsigned char dnas2_tdes_key_src[32] = {
#include "../../assets/dnas/dnas2m12_identity_tdes_key_src.inc"
};
static unsigned char dnas2_tdes_key_mask[8] = {
#include "../../assets/dnas/dnas2m12_identity_tdes_key_mask.inc"
};

/* Overwrite the n bytes at p with random bytes. */
#define DNAS2_P4_WIPE(p, n)						\
	do {								\
		unsigned char *_w = (unsigned char *)(p);		\
		unsigned int _i;					\
		for (_i = 0; _i < (n); _i++)				\
			_w[_i] = (unsigned char)((double)__sce_dnas2_symbol12079() *		\
				 (1.0 / 2147483648.0) * 256.0);		\
	} while (0)

/* 512-byte work buffer, aligned for PFS I/O. */
static unsigned char dnas2_m12_buf512[512] __attribute__((aligned(64)));

static unsigned char dnas2_enc_ptr_off[8] = {
#include "../../assets/dnas/dnas2m12_identity_enc_ptr_off.inc"
};

/* data and helpers of __sce_dnas2_symbol12092 */

static unsigned char dnas2_offset_key[8] = {
#include "../../assets/dnas/dnas2m12_identity_offset_key.inc"
};

static int dnas2_m12_create_flag_file(void);
static int dnas2_m12_decrypt_name(void *enc, void *buf, int n);
static int dnas2_m12_pfs_write(int off, const void *src, int n);
static void dnas2_be32_get(unsigned int *out, const unsigned char *p);

static __inline__ unsigned char *dnas2_cp(unsigned char *d, const unsigned char *s, unsigned int n)
{
	unsigned int j;

	for (j = 0; j < n; j++)
		d[j] = s[j];
	return d + n;
}

/* Module 12 counterpart of __sce_dnas2_symbol11072. */
int __sce_dnas2_symbol12094(unsigned char *blk)
{
	int r;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 == 0) {
		r = __sce_dnas2_symbol12087(blk);
		if (r != 1) {
			r = SCE_DNAS_ERR_ID_MEDIUM;
		} else {
			__sce_dnas2_symbol12062();
			__sce_dnas2_symbol12074 = r;
			r = 0;
		}
	} else {
		r = 0;
	}
	SCE_DNAS_PROT_END();
	return r;
}

/* Module 12 counterpart of __sce_dnas2_symbol11080. */
int __sce_dnas2_symbol12076(void)
{
	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 == 1)
		__sce_dnas2_symbol12074 = 0;
	SCE_DNAS_PROT_END();
	return 0;
}

/*
 * Reads the identity block from the memory-mapped device. Derive the two
 * offsets, ask __sce_dnas2_symbol12086 for the medium class (0 or 1, else
 * SCE_DNAS_ERR_ID_MEDIUM), builds the device name, opens it and maps 2 MB, locate the block and
 * extract its four parts (class 0 also waits for the status bit). For class 1
 * the block is passed to __sce_dnas2_symbol12085. The result is packed into
 * out (256 bytes) and signed. Returns 256, or -8/SCE_DNAS_ERR_ID_FAILED/SCE_DNAS_ERR_ID_BLOCK/SCE_DNAS_ERR_ID_MEDIUM. All local
 * buffers are wiped.
 */
int __sce_dnas2_symbol12092(void *arg, unsigned char *out)
{
	int r;
	unsigned char part1[40];
	unsigned char part2[56];
	unsigned char part3[32];
	unsigned char part4[40];
	unsigned char skey[16];
	unsigned char path[16];
	unsigned char key[16];
	unsigned char ptrKey[16];
	unsigned char ptrBuf[16];
	unsigned char flag;
	int off1;
	int off2;
	unsigned char *status;
	int fd;
	unsigned char *map;
	int i, t;
	unsigned int z, m, n;
	unsigned char *p, *ptrKeys, *encPtr, *tail, *sig, *ptr;
	const unsigned char *keySrc, *tag;
	unsigned int off;

	fd = -1;
	map = 0;
	flag = 0;
	keySrc = dnas2_ptr_keys;
	memcpy(key, keySrc, 16);
	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 != 1) {
		r = -8;
	} else if (dnas2_decrypt_offset(&dnas2_enc_map_off, &off1) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else if (dnas2_decrypt_offset(dnas2_enc_ptr_off, &off2) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else if (__sce_dnas2_symbol12086(arg, &flag) < 0) {
		r = SCE_DNAS_ERR_ID_MEDIUM;
	} else if (flag >= 2) {
		r = SCE_DNAS_ERR_ID_MEDIUM;
	} else if (dnas2_m12_mem_device_name(path) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else if ((fd = open((const char *)path, O_RDWR)) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else if ((map = (unsigned char *)mmap(0, 0x200000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, off1)) == 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else {
		ptrKeys = key;
		encPtr = map + off2;
		if (__sce_dnas2_symbol12061(ptrKeys, key + 8, ptrKey, 1) < 0) {
			r = SCE_DNAS_ERR_ID_FAILED;
		} else if (__sce_dnas2_symbol12060(encPtr, ptrKey, ptrBuf, 1) < 0) {
			r = SCE_DNAS_ERR_ID_FAILED;
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
			if (__sce_dnas2_symbol12103(part1, ptr, 256, 16384) < 0
			    || __sce_dnas2_symbol12077(part2, ptr, 256, 16384) < 0) {
				r = SCE_DNAS_ERR_ID_FAILED;
				goto done;
			}
			if (flag == 0) {
				if (__sce_dnas2_symbol12107(&status, ptr, 256, 16384) < 0) {
					r = SCE_DNAS_ERR_ID_FAILED;
					goto done;
				}
				status[0] = (unsigned char)((status[0] & 0xf0) + 3);
				n = 0;
				do {
					if ((status[0] & 3) != 3)
						break;
					n++;
					sleep(1);
				} while (n < 21);
				if (status[0] & 4) {
					r = SCE_DNAS_ERR_ID_BLOCK;
					goto done;
				}
				if (__sce_dnas2_symbol12105(part3, ptr, 256, 16384) < 0) {
					r = SCE_DNAS_ERR_ID_BLOCK;
					goto done;
				}
				if (__sce_dnas2_symbol12106(part4, ptr, 256, 16384) < 0) {
					r = SCE_DNAS_ERR_ID_BLOCK;
					goto done;
				}
			}
			if (munmap(map, 0x200000) < 0) {
				r = SCE_DNAS_ERR_ID_FAILED;
				goto done;
			}
			map = 0;
			if (close(fd) < 0) {
				r = SCE_DNAS_ERR_ID_FAILED;
				goto done;
			}
			fd = -1;
			if (flag == 1 && __sce_dnas2_symbol12085(arg, part3, part4) < 0) {
				r = SCE_DNAS_ERR_ID_MEDIUM;
				goto done;
			}
			z = 0;
			for (; z < 256; z++)
				out[z] = 0;
			dnas2_cp(out, part3, 32);
			p = dnas2_cp(out + 32, part1, 40);
			p = dnas2_cp(p, part2, 56);
			dnas2_cp(p, part4, 40);
			tail = out + 232;
			sig = out + 240;
			for (i = 0; i < 16; i++)
				tail[i] =
				    (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
			p = tail + 16;
			tag = dnas2_id_tag;
			for (t = 0; t < 8; t++)
				p[t] = tag[t];
			if (__sce_dnas2_symbol12059(out, tail, out, sig, 29) < 0) {
				r = SCE_DNAS_ERR_ID_FAILED;
			} else if (__sce_dnas2_symbol12061(&dnas2_sig_key_src, &dnas2_sig_key_mask, skey, 2) < 0) {
				r = SCE_DNAS_ERR_ID_FAILED;
			} else {
				r = (__sce_dnas2_symbol12059(tail, skey, tail, skey + 8, 3) < 0)
				    ? SCE_DNAS_ERR_ID_FAILED
				    : 256;
			}
		}
	}
done:
	if (map != 0)
		munmap(map, 0x200000);
	if (fd >= 0)
		close(fd);
	for (m = 0; m < 32; m++)
		part3[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 40; m++)
		part1[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 56; m++)
		part2[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 40; m++)
		part4[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 16; m++)
		skey[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 16; m++)
		path[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	off1 = 0;
	off2 = 0;
	SCE_DNAS_PROT_END();
	return r;
}

/* Module 12 counterpart of __sce_dnas2_symbol11069. */
int __sce_dnas2_symbol12091(void *blk, void *src, void *ctx, void *out, void *out16)
{
	int r, ret;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 != 1) {
		r = -8;
	} else {
		ret = __sce_dnas2_symbol12084(blk, src, ctx, out, out16);
		r = (ret >= 0) ? 0 : SCE_DNAS_ERR_ID_FAILED;
	}
	SCE_DNAS_PROT_END();
	return r;
}

/* 7 arguments */

/* Module 12 counterpart of __sce_dnas2_symbol11067. */
int __sce_dnas2_symbol12072(unsigned char *out)
{
	int r;
	unsigned char k1[128];
	unsigned char k2[128];
	unsigned char pubkey[176];
	int handle;
	int len;
	unsigned int i, m;
	unsigned char *t, *d;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 != 1) {
		r = -8;
	} else if (dnas2_decrypt_offset(dnas2_enc_rec_off, &handle) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else {
		d = pubkey;
		t = dnas2_rec_pubkey;
		for (i = 0; i < 162; i++)
			d[i] = t[i];
		if (__sce_dnas2_symbol12060(pubkey, dnas2_rec_pubkey_key, pubkey, 6) < 0
		    || dnas2_m12_keyfile_read(k1, 128, 512, handle) < 0
		    || __sce_dnas2_symbol12110(k1, 128, pubkey, 162, k2, 128, &len) < 0) {
			r = SCE_DNAS_ERR_ID_FAILED;
		} else {
			d = out;
			t = k2;
			for (i = 0; i < 4; i++)
				d[i] = t[i];
			r = 4;
		}
	}
	for (m = 0; m < 128; m++)
		k1[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 128; m++)
		k2[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 162; m++)
		pubkey[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	handle = 0;
	SCE_DNAS_PROT_END();
	return r;
}

/* Module 12 counterpart of __sce_dnas2_symbol11068. */
int __sce_dnas2_symbol12090(void *slot)
{
	int r, ret, h;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 != 1) {
		r = -8;
	} else if (dnas2_decrypt_offset(dnas2_enc_slot_off, &h) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else {
		ret = dnas2_m12_keyfile_read(slot, 16, 512, h);
		r = (ret < 0) ? SCE_DNAS_ERR_ID_FAILED : 16;
	}
	h = 0;
	SCE_DNAS_PROT_END();
	return r;
}

/* Module 12 counterpart of __sce_dnas2_symbol11079. */
int __sce_dnas2_symbol12101(void *slot)
{
	int r, ret, h;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 != 1) {
		r = -8;
	} else if (dnas2_decrypt_offset(dnas2_enc_slot_off, &h) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else {
		ret = dnas2_m12_keyfile_write(slot, 16, 512, h);
		r = (ret < 0) ? SCE_DNAS_ERR_ID_FAILED : 16;
	}
	h = 0;
	SCE_DNAS_PROT_END();
	return r;
}

/* Module 12 counterpart of __sce_dnas2_symbol11071. */
int __sce_dnas2_symbol12073(void)
{
	int invalid = -1;
	int r, ret;
	unsigned int i;
	unsigned char buf[32];
	unsigned char *d;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 != 1) {
		r = -8;
	} else if (dnas2_m12_flag_file_name(buf) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else {
		ret = dnas2_m12_exists(buf);
		r = (ret > invalid) ? ret : SCE_DNAS_ERR_ID_FAILED;
	}
	d = buf;
	for (i = 0; i < 24; i++)
		d[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Module 12 counterpart of __sce_dnas2_symbol11075. */
int __sce_dnas2_symbol12097(void)
{
	unsigned char *p, *q;
	int r;
	unsigned char desc[32];
	int h1, h2;
	int i;
	unsigned int m;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 != 1) {
		r = -8;
	} else if (dnas2_decrypt_offset(dnas2_enc_rec_off, &h1) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else if (dnas2_decrypt_offset(dnas2_enc_slot_off, &h2) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else if (dnas2_m12_flag_file_name(desc) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else {
		r = dnas2_m12_remove(desc);
		p = dnas2_m12_buf512;
		for (i = 0; i < 512; i++)
			p[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
		q = dnas2_m12_buf512;
		dnas2_m12_keyfile_write(q, 512, 512, h1);
		for (i = 0; i < 512; i++)
			q[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
		dnas2_m12_keyfile_write(q, 512, 512, h2);
		for (i = 0; i < 512; i++)
			q[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
		if (r < 0)
			r = SCE_DNAS_ERR_ID_FAILED;
		else
			r = 0;
	}
	for (m = 0; m < 24; m++)
		desc[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	h1 = 0;
	h2 = 0;
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_write_record(void *p)
{
	int r, ret, h;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 != 1) {
		r = -8;
	} else if (dnas2_decrypt_offset(dnas2_enc_rec_off, &h) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else {
		ret = dnas2_m12_keyfile_write(p, 128, 512, h);
		r = (ret < 0) ? SCE_DNAS_ERR_ID_FAILED : 128;
	}
	h = 0;
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_create_flag_file(void)
{
	unsigned char buf[32];
	unsigned char *w;
	unsigned int i;
	int r;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_flag_file_name(buf) < 0)
		r = -1;
	else
		r = dnas2_m12_write_file(buf, 0, 0);
	w = buf;
	for (i = 0; i < 24; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Module 12 counterpart of __sce_dnas2_symbol11078. */
int __sce_dnas2_symbol12100(void *rec, void *slot)
{
	int r, ret;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 != 1) {
		r = -8;
	} else if (dnas2_m12_write_record(rec) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else if (__sce_dnas2_symbol12101(slot) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else {
		ret = dnas2_m12_create_flag_file();
		r = (ret < 0) ? SCE_DNAS_ERR_ID_FAILED : 144;
	}
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_read_id_block(void *arg)
{
	int r;
	unsigned char path[16];
	unsigned char key[16];
	unsigned char mkey[16];
	unsigned char moff[16];
	int off1;
	int off2;
	int fd;
	unsigned char *map;
	int i;
	unsigned int z;
	unsigned int off;
	unsigned char *kb, *mp, *ptr;
	const unsigned char *kp;

	fd = -1;
	map = 0;
	kp = dnas2_ptr_keys;
	memcpy(key, kp, 16);
	SCE_DNAS_PROT_BEGIN();
	if (dnas2_decrypt_offset(&dnas2_enc_map_off, &off1) < 0) {
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
		kb = key;
		mp = map + off2;
		if (__sce_dnas2_symbol12061(kb, key + 8, mkey, 1) < 0) {
			r = -1;
		} else if (__sce_dnas2_symbol12060(mp, mkey, moff, 1) < 0) {
			r = -1;
		} else {
			off = *(unsigned int *)moff ^ *(unsigned int *)(moff + 4);
			for (i = 0; i < 16; i++)
				kb[i] =
				    (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
			i = 0;
			ptr = map + off;
			for (; i < 8; i++)
				mkey[i] =
				    (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
			for (i = 0; i < 8; i++)
				moff[i] =
				    (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
			if (__sce_dnas2_symbol12103(arg, ptr, 256, 16384) < 0) {
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

/* Module 12 counterpart of __sce_dnas2_symbol11066. */
int __sce_dnas2_symbol12071(void)
{
	int r;
	unsigned char desc[48];
	unsigned char data[144];
	unsigned char sig[128];
	unsigned char pubkey[176];
	int handle;
	int len;
	unsigned int i, m;
	unsigned char *t, *d;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12074 != 1) {
		r = -8;
	} else if (dnas2_decrypt_offset(dnas2_enc_rec_off, &handle) < 0) {
		r = SCE_DNAS_ERR_ID_FAILED;
	} else {
		d = pubkey;
		t = dnas2_rec_pubkey;
		for (i = 0; i < 162; i++)
			d[i] = t[i];
		if (__sce_dnas2_symbol12060(pubkey, dnas2_rec_pubkey_key, pubkey, 6) < 0) {
			r = SCE_DNAS_ERR_ID_FAILED;
		} else if (dnas2_m12_read_id_block(desc) < 0) {
			r = SCE_DNAS_ERR_ID_FAILED;
		} else if (dnas2_m12_keyfile_read(data, 128, 512, handle) < 0) {
			r = SCE_DNAS_ERR_ID_FAILED;
		} else if (__sce_dnas2_symbol12110(data, 128, pubkey, 162, sig, 128, &len) < 0) {
			r = SCE_DNAS_ERR_ID_FAILED;
		} else {
			r = dnas2_memcmp(desc + 24, sig + 28, 16) == 0;
		}
	}
	handle = 0;
	for (m = 0; m < 40; m++)
		desc[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 128; m++)
		data[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 128; m++)
		sig[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (m = 0; m < 162; m++)
		pubkey[m] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Module 12 counterpart of __sce_dnas2_symbol11077. */
int __sce_dnas2_symbol12099(const char *name, void *buf, int len)
{
	int r;
	unsigned char desc[16];
	unsigned char blk[464];
	unsigned int i;
	unsigned char *w1, *w2;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_data_dir_name(desc) < 0 || (unsigned int)(r = dnas2_m12_exists(desc)) > 1
	    || (r == 0 && dnas2_m12_mkdir(desc) < 0)) {
		r = -1;
	} else {
		strcpy(blk, desc);
		strcat(blk, name);
		r = dnas2_m12_write_file(blk, buf, len);
	}
	w1 = blk;
	for (i = 0; i < 280; i++)
		w1[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	w2 = desc;
	for (i = 0; i < 16; i++)
		w2[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Module 12 counterpart of __sce_dnas2_symbol11074. */
int __sce_dnas2_symbol12075(const char *name, char *buf, unsigned int maxlen, int *outlen)
{
	int r;
	unsigned char desc[16];
	unsigned char blk[464];
	unsigned int i;
	unsigned char *w1, *w2;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_data_dir_name(desc) < 0 || dnas2_m12_exists(desc) != 1) {
		r = -1;
	} else {
		strcpy(blk, desc);
		strcat(blk, name);
		r = dnas2_m12_read_file(blk, buf, maxlen, outlen);
	}
	w1 = blk;
	for (i = 0; i < 280; i++)
		w1[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	w2 = desc;
	for (i = 0; i < 16; i++)
		w2[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* Module 12 counterpart of __sce_dnas2_symbol11076. */
int __sce_dnas2_symbol12098(void)
{
	int r, ret;
	unsigned int i;
	unsigned char buf[16];
	unsigned char *w;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_data_dir_name(buf) < 0) {
		r = -1;
	} else {
		r = dnas2_m12_exists(buf);
		if ((unsigned int)r >= 2) {
			r = -1;
		} else if (r == 1) {
			ret = dnas2_m12_rmdir_all(buf);
			if (ret < 0)
				r = -1;
		}
	}
	w = buf;
	for (i = 0; i < 16; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

/* DES-decrypts the n blocks of the encrypted name enc into buf with the fixed name key. */
static int dnas2_m12_decrypt_name(void *enc, void *buf, int n)
{
	char key[8] = {
#include "../../assets/dnas/dnas2m12_identity_name_key.inc"
	};

	return __sce_dnas2_symbol12060(enc, key, buf, n);
}

/* Decrypts the PFS device that the partition is mounted on ("pfs0:", one block) into buf. */
static int dnas2_m12_mount_device(void *buf)
{
	char enc[8] = {
#include "../../assets/dnas/dnas2m12_identity_enc_mount_device.inc"
	};
	int r;

	SCE_DNAS_PROT_BEGIN();
	r = dnas2_m12_decrypt_name(enc, buf, 1);
	SCE_DNAS_PROT_END();
	return r;
}

/* Decrypts the name of the flag file ("/etc/access_flag25", three blocks) into buf. */
static int dnas2_m12_flag_file_name(void *buf)
{
	char enc[24] = {
#include "../../assets/dnas/dnas2m12_identity_enc_flag_file_name.inc"
	};
	int r;

	SCE_DNAS_PROT_BEGIN();
	r = dnas2_m12_decrypt_name(enc, buf, 3);
	SCE_DNAS_PROT_END();
	return r;
}

/* Decrypts the HDD partition ("hdd0:__net", two blocks) into buf: mounted on the PFS
 * device and opened as the key file. */
static int dnas2_m12_partition_name(void *buf)
{
	char enc[16] = {
#include "../../assets/dnas/dnas2m12_identity_enc_partition_name.inc"
	};
	int r;

	SCE_DNAS_PROT_BEGIN();
	r = dnas2_m12_decrypt_name(enc, buf, 2);
	SCE_DNAS_PROT_END();
	return r;
}

/* Decrypts the data directory ("/drdata/", two blocks) into buf. */
static int dnas2_m12_data_dir_name(void *buf)
{
	char enc[16] = {
#include "../../assets/dnas/dnas2m12_identity_enc_data_dir_name.inc"
	};
	int r;

	SCE_DNAS_PROT_BEGIN();
	r = dnas2_m12_decrypt_name(enc, buf, 2);
	SCE_DNAS_PROT_END();
	return r;
}

/* Decrypts the memory device ("/dev/mem", two blocks) into buf: mapped to read the
 * identity block. */
static int dnas2_m12_mem_device_name(void *buf)
{
	char enc[16] = {
#include "../../assets/dnas/dnas2m12_identity_enc_mem_device_name.inc"
	};
	int r;

	SCE_DNAS_PROT_BEGIN();
	r = dnas2_m12_decrypt_name(enc, buf, 2);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_pfs_read(int off, void *dst, int n)
{
	char path[16];
	unsigned char *w;
	unsigned int i;
	int fd = -1;
	int r;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_partition_name(path) < 0) {
		r = -1;
	} else if ((fd = scepfsOpen(path, 1)) < 0) {
		r = -1;
	} else if (scepfsLseek(fd, off, 0) < 0) {
		r = -1;
	} else if (n != scepfsRead(fd, dst, n)) {
		r = -1;
	} else if (scepfsClose(fd) < 0) {
		r = -1;
	} else {
		fd = -1;
		r = n;
	}
	if (fd >= 0)
		scepfsClose(fd);
	w = (unsigned char *)path;
	for (i = 0; i < 16; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_pfs_write(int off, const void *src, int n)
{
	char path[16];
	unsigned char *w;
	unsigned int i;
	int fd = -1;
	int r;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_partition_name(path) < 0) {
		r = -1;
	} else if ((fd = scepfsOpen(path, 3)) < 0) {
		r = -1;
	} else if (scepfsLseek(fd, off, 0) < 0) {
		r = -1;
	} else if (n != scepfsWrite(fd, src, n)) {
		r = -1;
	} else if (scepfsClose(fd) < 0) {
		r = -1;
	} else {
		fd = -1;
		r = n;
	}
	if (fd >= 0)
		scepfsClose(fd);
	w = (unsigned char *)path;
	for (i = 0; i < 16; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_tdes_encrypt(void *in, void *out, unsigned int count)
{
	unsigned char buf[32];
	int r;
	int ret;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12061(dnas2_tdes_key_src, dnas2_tdes_key_mask, buf, 4) < 0) {
		r = -1;
	} else {
		ret = __sce_dnas2_symbol12112(in, buf, buf + 8, buf + 16, out, buf + 24, count);
		r = (ret >= 0) ? 0 : -1;
	}
	DNAS2_P4_WIPE(buf, 32);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_tdes_decrypt(void *in, void *out, unsigned int count)
{
	unsigned char buf[32];
	int r;
	int ret;

	SCE_DNAS_PROT_BEGIN();
	if (__sce_dnas2_symbol12061(dnas2_tdes_key_src, dnas2_tdes_key_mask, buf, 4) < 0) {
		r = -1;
	} else {
		ret = __sce_dnas2_symbol12111(in, buf, buf + 8, buf + 16, out, buf + 24, count);
		r = (ret >= 0) ? 0 : -1;
	}
	DNAS2_P4_WIPE(buf, 32);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_keyfile_read(unsigned char *out, unsigned int outlen, unsigned int n, int off)
{
	unsigned char tmp[n];
	int r;
	unsigned int j;
	int i;
	unsigned char *q;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_pfs_read(off, dnas2_m12_buf512, n) < 0) {
		r = -1;
	} else if (dnas2_m12_tdes_decrypt(dnas2_m12_buf512, tmp, n >> 3) < 0) {
		r = -1;
	} else {
		for (j = 0; j < outlen; j++)
			out[j] = tmp[j];
		r = outlen;
	}
	for (i = 0; i < n; i++)
		tmp[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	q = dnas2_m12_buf512;
	for (i = 0; i < 512; i++)
		q[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_keyfile_write(unsigned char *in, unsigned int inlen, unsigned int n, int off)
{
	unsigned char tmp[n];
	unsigned char *q;
	unsigned int j;
	int i;
	int r, ret;

	SCE_DNAS_PROT_BEGIN();
	q = dnas2_m12_buf512;
	for (j = 0; j < inlen; j++)
		tmp[j] = in[j];
	if (dnas2_m12_tdes_encrypt(tmp, q, n >> 3) < 0) {
		r = -1;
	} else {
		ret = dnas2_m12_pfs_write(off, q, n);
		if (ret < 0)
			r = -1;
		else
			r = inlen;
	}
	for (i = 0; i < n; i++)
		tmp[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	for (i = 0; i < 512; i++)
		q[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_mount(unsigned char *dev)
{
	unsigned char path[16];
	unsigned char *w;
	unsigned int i;
	unsigned int t;
	int r;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_mount_device(dev) < 0) {
		r = -1;
	} else if (dnas2_m12_partition_name(path) < 0) {
		r = -1;
	} else {
		for (t = 0; t < 10; t++) {
			if (scepfsMount(dev, path, 2, 0, 0) >= 0)
				break;
			if (t == 9) {
				r = -1;
				goto out;
			}
			dev[3]++;
		}
		r = 0;
	}
out:
	w = path;
	for (i = 0; i < 16; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_exists(const char *name)
{
	iox_stat_t st;
	char path[16];
	char full[1200];
	char *w;
	char *q;
	unsigned int i;
	int r;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_mount((unsigned char *)path) < 0) {
		r = -1;
	} else {
		strcpy(full, path);
		strcat(full, name);
		if (scepfsGetstat(full, &st) < 0)
			r = (scepfsUmount(path) >= 0) ? 0 : -1;
		else
			r = (scepfsUmount(path) < 0) ? -1 : 1;
	}
	w = path;
	for (i = 0; i < 8; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	q = full;
	for (i = 0; i < 1024; i++)
		q[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_write_file(const char *name, void *buf, int len)
{
	char path[16];
	char full[1200];
	char *w;
	char *q;
	unsigned int i;
	int r, fd;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_mount((unsigned char *)path) < 0) {
		r = -1;
		goto wipe;
	}
	strcpy(full, path);
	strcat(full, name);
	fd = scepfsOpen(full, 1539, 511);
	if (fd < 0) {
		r = -1;
		goto umount;
	}
	if (len != 0) {
		if (len != scepfsWrite(fd, buf, len)) {
			r = -1;
			goto closeit;
		}
	}
	if (scepfsClose(fd) < 0) {
		r = -1;
		goto umount;
	}
	fd = -1;
	r = (scepfsUmount(path) >= 0) ? 0 : -1;
	goto wipe;
closeit:
	scepfsClose(fd);
umount:
	scepfsUmount(path);
wipe:
	w = path;
	for (i = 0; i < 8; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	q = full;
	for (i = 0; i < 1024; i++)
		q[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_read_file(const char *name, char *buf, unsigned int maxlen, int *outlen)
{
	char path[16];
	char full[1200];
	char *w;
	char *q;
	unsigned int i;
	int r, fd;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_mount((unsigned char *)path) < 0) {
		r = -1;
		goto err;
	}
	strcpy(full, path);
	strcat(full, name);
	fd = scepfsOpen(full, 1);
	if (fd < 0) {
		r = -1;
		goto umount;
	}
	*outlen = scepfsLseek(fd, 0, 2);
	if (maxlen < (unsigned int)*outlen) {
		r = -1;
		goto closeit;
	}
	if (scepfsLseek(fd, 0, 0) < 0) {
		r = -1;
		goto closeit;
	}
	if (*outlen != scepfsRead(fd, buf, *outlen)) {
		r = -1;
		goto closeit;
	}
	if (scepfsClose(fd) < 0) {
		r = -1;
		goto umount;
	}
	if (scepfsUmount(path) < 0) {
		r = -1;
		goto err;
	}
	r = 0;
	goto wipe;
closeit:
	scepfsClose(fd);
umount:
	scepfsUmount(path);
err:
	buf[0] = 0;
wipe:
	w = path;
	for (i = 0; i < 8; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	q = full;
	for (i = 0; i < 1024; i++)
		q[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_remove(const char *name)
{
	char path[16];
	char full[1200];
	char *w;
	char *q;
	unsigned int i;
	int r, ret;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_mount((unsigned char *)path) < 0) {
		r = -1;
	} else {
		strcpy(full, path);
		strcat(full, name);
		if (scepfsRemove(full) < 0) {
			scepfsUmount(path);
			r = -1;
		} else {
			ret = scepfsUmount(path);
			r = (ret >= 0) ? 0 : -1;
		}
	}
	w = path;
	for (i = 0; i < 8; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	q = full;
	for (i = 0; i < 1024; i++)
		q[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_mkdir(const char *name)
{
	char path[16];
	char full[1200];
	char *w;
	char *q;
	unsigned int i;
	int r, ret;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_mount((unsigned char *)path) < 0) {
		r = -1;
	} else {
		strcpy(full, path);
		strcat(full, name);
		if (scepfsMkdir(full, 511) < 0) {
			scepfsUmount(path);
			r = -1;
		} else {
			ret = scepfsUmount(path);
			r = (ret >= 0) ? 0 : -1;
		}
	}
	w = path;
	for (i = 0; i < 8; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	q = full;
	for (i = 0; i < 1024; i++)
		q[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_m12_rmdir_all(const char *name)
{
	iox_dirent_t de;
	char path[16];
	char full[1344];
	char *w;
	char *q;
	unsigned int i;
	int r, fd;

	SCE_DNAS_PROT_BEGIN();
	if (dnas2_m12_mount((unsigned char *)path) < 0) {
		r = -1;
		goto wipe;
	}
	strcpy(full, path);
	strcat(full, name);
	fd = scepfsDopen(full);
	if (fd < 0) {
		r = -1;
		goto umount;
	}
	while (scepfsDread(fd, &de) > 0) {
		if ((de.stat.mode & 0xf000) != 0x2000)
			continue;
		strcpy(full, path);
		strcat(full, name);
		strcat(full, de.name);
		if (scepfsRemove(full) < 0) {
			r = -1;
			goto dclose;
		}
	}
	if (scepfsDclose(fd) < 0) {
		r = -1;
		goto umount;
	}
	fd = -1;
	r = (scepfsUmount(path) >= 0) ? 0 : -1;
	goto wipe;
dclose:
	scepfsDclose(fd);
umount:
	scepfsUmount(path);
wipe:
	w = path;
	for (i = 0; i < 8; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	q = full;
	for (i = 0; i < 1024; i++)
		q[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}

static int dnas2_memcmp(const unsigned char *a, const unsigned char *b, unsigned int n)
{
	unsigned int i;
	int r;

	SCE_DNAS_PROT_BEGIN();
	for (i = 0; i < n; i++) {
		if (a[i] < b[i]) {
			r = -1;
			goto out;
		}
		if (b[i] < a[i]) {
			r = 1;
			goto out;
		}
	}
	r = 0;
out:
	SCE_DNAS_PROT_END();
	return r;
}

/* Reads a big-endian 32-bit word. */
static void dnas2_be32_get(unsigned int *out, const unsigned char *p)
{
	*out = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

/* Decrypts a word of enc, stores it big-endian in *out, wipes the buffer. */
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
		r = 0;
	}
	w = buf;
	for (i = 0; i < 8; i++)
		w[i] = (unsigned char)((double)__sce_dnas2_symbol12079() * (1.0 / 2147483648.0) * 256.0);
	SCE_DNAS_PROT_END();
	return r;
}
