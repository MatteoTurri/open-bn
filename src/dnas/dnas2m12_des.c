/*
 * DNAS2 family 12: DES (ECB, CBC, EDE) through BSAFE, and SHA-1.
 * The PRNG group and its wrappers are in dnas2m12_rand.c.
 */

#include <string.h>

#include "../../ref/rcom-1.1.0/bsafe.h"
#include "dnas2_bsafe.h"
#include "dnas2m12_des.h"

/* Static helpers: XOR of an 8-byte block with the block key, and creation of
 * the BSAFE key and algorithm objects with B_EncryptInit or B_DecryptInit. */
static unsigned char *dnas2_unmask8(unsigned char *key, unsigned char *blk);
static int dnas2_encrypt_obj(void *keyData, void *methods, unsigned char *cipherName, unsigned char *feedbackName,
    int *feedbackParams, void *keyInfoType, void *objOut);
static int dnas2_decrypt_obj(void *keyData, void *methods, unsigned char *cipherName, unsigned char *feedbackName,
    int *feedbackParams, void *keyInfoType, void *objOut);

/* XOR key and the BSAFE method names it hides (AI_FeedbackCipher parameters). */

static unsigned char dnas2_nameMask[8] = {
#include "../../assets/dnas/dnas2m12_des_name_mask.inc"
};
static const Bsafe8 dnas2_name_nopad = { {
#include "../../assets/dnas/dnas2m12_des_name_nopad.inc"
} }; /* "nopad" */
static const Bsafe8 dnas2_name_des = { {
#include "../../assets/dnas/dnas2m12_des_name_des.inc"
} }; /* "des" */
static const Bsafe8 dnas2_name_ecb = { {
#include "../../assets/dnas/dnas2m12_des_name_ecb.inc"
} }; /* "ecb" */
static const Bsafe8 dnas2_name_cbc = { {
#include "../../assets/dnas/dnas2m12_des_name_cbc.inc"
} }; /* "cbc" */
static const Bsafe8 dnas2_name_des_ede = { {
#include "../../assets/dnas/dnas2m12_des_name_des_ede.inc"
} }; /* "des_ede" */

static int dnas2_encrypt_obj(void *keyData, void *methods, unsigned char *cipherName, unsigned char *feedbackName,
    int *feedbackParams, void *keyInfoType, void *objOut);

/* XORs the 8-byte block key into `blk` and returns `blk`. */
static unsigned char *dnas2_unmask8(unsigned char *key, unsigned char *blk)
{
	int i;

	for (i = 0; i < 8; i++)
		blk[i] ^= key[i];

	return blk;
}

/* Creates the key object (key data `keyData`, type `keyInfoType`) and an
 * AI_FeedbackCipher algorithm object (cipher cipherName, feedback feedbackName with feedbackParams,
 * "nopad"), binds them with B_EncryptInit and returns the object in *objOut. */
static int dnas2_encrypt_obj(void *keyData, void *methods, unsigned char *cipherName, unsigned char *feedbackName,
    int *feedbackParams, void *keyInfoType, void *objOut)
{
	Bsafe8 paddingName = dnas2_name_nopad;
	B_BLK_CIPHER_W_FEEDBACK_PARAMS params;
	BsafeKeyObj *key;
	BsafeAlgorithmObj *alg;
	int status;

	key = 0;
	alg = 0;
	do {
		if ((status = B_CreateKeyObject(&key)) != 0)
			break;
		if ((status = B_SetKeyInfo(key, keyInfoType, keyData)) != 0)
			break;
		if ((status = B_CreateAlgorithmObject(&alg)) != 0)
			break;
		params.encryptionMethodName = cipherName;
		params.encryptionParams = 0;
		params.feedbackMethodName = feedbackName;
		params.feedbackParams = feedbackParams;
		params.paddingMethodName = dnas2_unmask8(dnas2_nameMask, paddingName.b);
		params.paddingParams = 0;
		if ((status = B_SetAlgorithmInfo(alg, &AI_FeedbackCipher, &params)) != 0)
			break;
		status = B_EncryptInit(alg, key, methods, 0);
	} while (0);
	B_DestroyKeyObject(&key);
	*(void **)objOut = alg;
	return status;
}

/* DES-ECB encryption object for the 8-byte key `key`, returned in *objOut. */
static int dnas2_des_ecb_encrypt_obj(void *key, void *objOut)
{
	B_AlgorithmMethod *chooser[3] = { &AM_DES_ENCRYPT, &AM_ECB_ENCRYPT, 0 };
	Bsafe8 cipherBuf = dnas2_name_des;
	Bsafe8 modeBuf = dnas2_name_ecb;
	int len = 8;

	return dnas2_encrypt_obj(key, chooser, dnas2_unmask8(dnas2_nameMask, cipherBuf.b),
	    dnas2_unmask8(dnas2_nameMask, modeBuf.b), &len, &KI_DES8, objOut);
}

/* DES-CBC encryption object: (key, iv, &obj). */
int __sce_dnas2_symbol12122(void *key, void *iv, void *objOut)
{
	B_AlgorithmMethod *info[3] = { &AM_DES_ENCRYPT, &AM_CBC_ENCRYPT, 0 };
	struct {
		ITEM iv;
		char pad[8];
		Bsafe8 cipherBuf;
	} buf1;
	Bsafe8 modeBuf;
	void *mask, *cipherName, *modeName;

	memset(&buf1, 0, 8);

	mask = dnas2_nameMask;
	buf1.cipherBuf = dnas2_name_des;
	modeBuf = dnas2_name_cbc;

	buf1.iv.data = iv;
	buf1.iv.len = 8;

	cipherName = dnas2_unmask8(mask, (unsigned char *)&buf1.cipherBuf);
	modeName = dnas2_unmask8(mask, (unsigned char *)&modeBuf);

	return dnas2_encrypt_obj(key, info, cipherName, modeName, (int *)&buf1, &KI_DES8, objOut);
}

/* Triple-DES-ECB encryption object for the keys key1, key2, key3, returned in *objOut. */
static int dnas2_des_ede_ecb_encrypt_obj(void *key1, void *key2, void *key3, void *objOut)
{
	B_AlgorithmMethod *chooser[3] = { &AM_ECB_ENCRYPT, &AM_DES_EDE_ENCRYPT, 0 };
	unsigned char blk[24];
	Bsafe8 cipherBuf = dnas2_name_des_ede;
	Bsafe8 modeBuf = dnas2_name_ecb;
	int len;
	int r;

	T_memcpy(blk, key1, 8);
	T_memcpy(blk + 8, key2, 8);
	T_memcpy(blk + 16, key3, 8);
	len = 8;
	r = dnas2_encrypt_obj(blk, chooser, dnas2_unmask8(dnas2_nameMask, cipherBuf.b),
	    dnas2_unmask8(dnas2_nameMask, modeBuf.b), &len, &KI_24Byte, objOut);
	T_memset(blk, 0, 24);
	return r;
}

/* Triple-DES-CBC encryption object: (k1, k2, k3, iv, &obj). */
int __sce_dnas2_symbol12132(void *key1, void *key2, void *key3, void *ivData, void **objOut)
{
	B_AlgorithmMethod *chooser[3] = { &AM_CBC_ENCRYPT, &AM_DES_EDE_ENCRYPT, 0 };
	unsigned char blk[24];
	ITEM iv = { 0 };
	Bsafe8 cipherBuf = dnas2_name_des_ede;
	Bsafe8 modeBuf = dnas2_name_cbc;
	int r;

	T_memcpy(blk, key1, 8);
	T_memcpy(blk + 8, key2, 8);
	T_memcpy(blk + 16, key3, 8);
	iv.data = ivData;
	iv.len = 8;
	r = dnas2_encrypt_obj(blk, chooser, dnas2_unmask8(dnas2_nameMask, cipherBuf.b),
	    dnas2_unmask8(dnas2_nameMask, modeBuf.b), (int *)&iv, &KI_24Byte, objOut);
	T_memset(blk, 0, 24);
	return r;
}

/* Encrypts count * 8 bytes with an object from the Init calls; -2 unless
 * exactly that many bytes come out (in == out works in place). */
int __sce_dnas2_symbol12125(void *obj, void *in, void *out, unsigned int count)
{
	int status;
	unsigned int outLen;
	unsigned int len;
	unsigned int n = count * 8;

	status = B_EncryptUpdate(obj, out, &outLen, n, in, n, 0, 0);
	len = outLen;
	if (status == 0)
		status = (len == n) ? 0 : -2;
	return status;
}

/* In-place form of __sce_dnas2_symbol12125. */
int __sce_dnas2_symbol12126(void *obj, void *buf, unsigned int count)
{
	return __sce_dnas2_symbol12125(obj, buf, buf, count);
}

/* Same as __sce_dnas2_symbol12125. */
int __sce_dnas2_symbol12135(void *obj, void *in, void *out, unsigned int count)
{
	return __sce_dnas2_symbol12125(obj, in, out, count);
}

/* Same as __sce_dnas2_symbol12126. */
int __sce_dnas2_symbol12136(void *obj, void *buf, unsigned int count)
{
	return __sce_dnas2_symbol12125(obj, buf, buf, count);
}

/* Finishes an encryption (B_EncryptFinal into a scratch buffer), destroys
 * the object and returns the Final status. */
int __sce_dnas2_symbol12130(void *obj)
{
	char outbuf[16];
	unsigned int outLen;
	BsafeAlgorithmObj *o = obj;
	int status;

	do {
		if ((status = B_EncryptFinal(o, outbuf, &outLen, 0, 0, 0)) != 0)
			break;
	} while (0);
	B_DestroyAlgorithmObject(&o);
	return status;
}

/* Same as __sce_dnas2_symbol12130. */
int __sce_dnas2_symbol12138(void *obj)
{
	return __sce_dnas2_symbol12130(obj);
}

/* As dnas2_encrypt_obj, with B_DecryptInit. */
static int dnas2_decrypt_obj(void *keyData, void *methods, unsigned char *cipherName, unsigned char *feedbackName,
    int *feedbackParams, void *keyInfoType, void *objOut)
{
	Bsafe8 paddingName = dnas2_name_nopad;
	B_BLK_CIPHER_W_FEEDBACK_PARAMS params;
	BsafeKeyObj *key;
	BsafeAlgorithmObj *alg;
	int status;

	key = 0;
	alg = 0;
	do {
		if ((status = B_CreateKeyObject(&key)) != 0)
			break;
		if ((status = B_SetKeyInfo(key, keyInfoType, keyData)) != 0)
			break;
		if ((status = B_CreateAlgorithmObject(&alg)) != 0)
			break;
		params.encryptionMethodName = cipherName;
		params.encryptionParams = 0;
		params.feedbackMethodName = feedbackName;
		params.feedbackParams = feedbackParams;
		params.paddingMethodName = dnas2_unmask8(dnas2_nameMask, paddingName.b);
		params.paddingParams = 0;
		if ((status = B_SetAlgorithmInfo(alg, &AI_FeedbackCipher, &params)) != 0)
			break;
		status = B_DecryptInit(alg, key, methods, 0);
	} while (0);
	B_DestroyKeyObject(&key);
	*(void **)objOut = alg;
	return status;
}

/* DES-ECB decryption object for the 8-byte key `key`, returned in *objOut. */
static int dnas2_des_ecb_decrypt_obj(void *key, void *objOut)
{
	B_AlgorithmMethod *chooser[3] = { &AM_DES_DECRYPT, &AM_ECB_DECRYPT, 0 };
	Bsafe8 cipherBuf = dnas2_name_des;
	Bsafe8 modeBuf = dnas2_name_ecb;
	int len = 8;

	return dnas2_decrypt_obj(key, chooser, dnas2_unmask8(dnas2_nameMask, cipherBuf.b),
	    dnas2_unmask8(dnas2_nameMask, modeBuf.b), &len, &KI_DES8, objOut);
}

/* DES-CBC decryption object: (key, iv, &obj). */
int __sce_dnas2_symbol12121(void *key, void *iv, void *objOut)
{
	B_AlgorithmMethod *info[3] = { &AM_DES_DECRYPT, &AM_CBC_DECRYPT, 0 };
	struct {
		ITEM iv;
		char pad[8];
		Bsafe8 cipherBuf;
	} buf1;
	Bsafe8 modeBuf;
	void *mask, *cipherName, *modeName;

	memset(&buf1, 0, 8);

	mask = dnas2_nameMask;
	buf1.cipherBuf = dnas2_name_des;
	modeBuf = dnas2_name_cbc;

	buf1.iv.data = iv;
	buf1.iv.len = 8;

	cipherName = dnas2_unmask8(mask, (unsigned char *)&buf1.cipherBuf);
	modeName = dnas2_unmask8(mask, (unsigned char *)&modeBuf);

	return dnas2_decrypt_obj(key, info, cipherName, modeName, (int *)&buf1, &KI_DES8, objOut);
}

/* Triple-DES-ECB decryption object for the keys key1, key2, key3, returned in *objOut. */
static int dnas2_des_ede_ecb_decrypt_obj(void *key1, void *key2, void *key3, void *objOut)
{
	B_AlgorithmMethod *chooser[3] = { &AM_ECB_DECRYPT, &AM_DES_EDE_DECRYPT, 0 };
	unsigned char blk[24];
	Bsafe8 cipherBuf = dnas2_name_des_ede;
	Bsafe8 modeBuf = dnas2_name_ecb;
	int len;
	int r;

	T_memcpy(blk, key1, 8);
	T_memcpy(blk + 8, key2, 8);
	T_memcpy(blk + 16, key3, 8);
	len = 8;
	r = dnas2_decrypt_obj(blk, chooser, dnas2_unmask8(dnas2_nameMask, cipherBuf.b),
	    dnas2_unmask8(dnas2_nameMask, modeBuf.b), &len, &KI_24Byte, objOut);
	T_memset(blk, 0, 24);
	return r;
}

/* Triple-DES-CBC decryption object: (k1, k2, k3, iv, &obj). */
int __sce_dnas2_symbol12131(void *key1, void *key2, void *key3, void *ivData, void **objOut)
{
	B_AlgorithmMethod *chooser[3] = { &AM_CBC_DECRYPT, &AM_DES_EDE_DECRYPT, 0 };
	unsigned char blk[24];
	ITEM iv = { 0 };
	Bsafe8 cipherBuf = dnas2_name_des_ede;
	Bsafe8 modeBuf = dnas2_name_cbc;
	int r;

	T_memcpy(blk, key1, 8);
	T_memcpy(blk + 8, key2, 8);
	T_memcpy(blk + 16, key3, 8);
	iv.data = ivData;
	iv.len = 8;
	r = dnas2_decrypt_obj(blk, chooser, dnas2_unmask8(dnas2_nameMask, cipherBuf.b),
	    dnas2_unmask8(dnas2_nameMask, modeBuf.b), (int *)&iv, &KI_24Byte, objOut);
	T_memset(blk, 0, 24);
	return r;
}

/* Decrypts count * 8 bytes; -2 unless exactly that many bytes come out. */
int __sce_dnas2_symbol12123(void *obj, void *in, void *out, unsigned int count)
{
	int status;
	unsigned int outLen;
	unsigned int len;
	unsigned int n = count * 8;

	status = B_DecryptUpdate(obj, out, &outLen, n, in, n, 0, 0);
	len = outLen;
	if (status == 0)
		status = (len == n) ? 0 : -2;
	return status;
}

/* In-place form of __sce_dnas2_symbol12123. */
int __sce_dnas2_symbol12124(void *obj, void *buf, unsigned int count)
{
	return __sce_dnas2_symbol12123(obj, buf, buf, count);
}

/* Same as __sce_dnas2_symbol12123. */
int __sce_dnas2_symbol12133(void *obj, void *in, void *out, unsigned int count)
{
	return __sce_dnas2_symbol12123(obj, in, out, count);
}

/* Same as __sce_dnas2_symbol12124. */
int __sce_dnas2_symbol12134(void *obj, void *buf, unsigned int count)
{
	return __sce_dnas2_symbol12123(obj, buf, buf, count);
}

/* Finishes a decryption, destroys the object and returns the Final status. */
int __sce_dnas2_symbol12129(void *obj)
{
	char outbuf[16];
	unsigned int outLen;
	BsafeAlgorithmObj *o = obj;
	int status;

	do {
		if ((status = B_DecryptFinal(o, outbuf, &outLen, 0, 0, 0)) != 0)
			break;
	} while (0);
	B_DestroyAlgorithmObject(&o);
	return status;
}

/* Same as __sce_dnas2_symbol12129. */
int __sce_dnas2_symbol12137(void *obj)
{
	return __sce_dnas2_symbol12129(obj);
}

/* DES-ECB encryption: (in, key, out, count); 0 or -1. */
int __sce_dnas2_symbol12061(void *in, void *key, void *out, int count)
{
	void *local;
	int status;
	status = dnas2_des_ecb_encrypt_obj(key, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol12125(local, in, out, count);
		if (status == 0) {
			status = __sce_dnas2_symbol12130(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* DES-ECB decryption: (in, key, out, count); 0 or -1. */
int __sce_dnas2_symbol12060(void *in, void *key, void *out, int count)
{
	void *local;
	int status;
	status = dnas2_des_ecb_decrypt_obj(key, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol12123(local, in, out, count);
		if (status == 0) {
			status = __sce_dnas2_symbol12129(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* DES-CBC encryption: (in, key, out, iv, count); 0 or -1. */
int __sce_dnas2_symbol12059(void *in, void *key, void *out, void *iv, unsigned int count)
{
	void *local;

	if (__sce_dnas2_symbol12122(key, iv, &local) == 0) {
		if (__sce_dnas2_symbol12125(local, in, out, count) == 0) {
			return __sce_dnas2_symbol12130(local) == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* DES-CBC decryption: (in, key, out, iv, count); 0 or -1. */
int __sce_dnas2_symbol12118(void *in, void *key, void *out, void *iv, unsigned int count)
{
	void *local;
	int status;
	status = __sce_dnas2_symbol12121(key, iv, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol12123(local, in, out, count);
		if (status == 0) {
			status = __sce_dnas2_symbol12129(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Triple-DES-ECB encryption: (in, k1, k2, k3, out, count); 0 or -1. */
int __sce_dnas2_symbol12148(void *in, void *key1, void *key2, void *key3, void *out, void *count)
{
	void *local;
	int status;
	status = dnas2_des_ede_ecb_encrypt_obj(key1, key2, key3, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol12135(local, in, out, (unsigned int)count);
		if (status == 0) {
			status = __sce_dnas2_symbol12138(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Triple-DES-ECB decryption: (in, k1, k2, k3, out, count); 0 or -1. */
int __sce_dnas2_symbol12147(void *in, void *key1, void *key2, void *key3, void *out, void *count)
{
	void *local;
	int status;
	status = dnas2_des_ede_ecb_decrypt_obj(key1, key2, key3, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol12133(local, in, out, (unsigned int)count);
		if (status == 0) {
			status = __sce_dnas2_symbol12137(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Triple-DES-CBC encryption: (in, k1, k2, k3, out, iv, count); 0 or -1. */
int __sce_dnas2_symbol12112(void *in, void *key1, void *key2, void *key3, void *out, void *iv, unsigned int count)
{
	void *local;

	if (__sce_dnas2_symbol12132(key1, key2, key3, iv, &local) == 0) {
		if (__sce_dnas2_symbol12135(local, in, out, count) == 0) {
			return __sce_dnas2_symbol12138(local) == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Triple-DES-CBC decryption: (in, k1, k2, k3, out, iv, count); 0 or -1. */
int __sce_dnas2_symbol12144(void *in, void *key1, void *key2, void *key3, void *out, void *iv, unsigned int count)
{
	void *local;

	if (__sce_dnas2_symbol12131(key1, key2, key3, iv, &local) == 0) {
		if (__sce_dnas2_symbol12133(local, in, out, count) == 0) {
			return __sce_dnas2_symbol12137(local) == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Two-key triple-DES-ECB encryption (k1, k2, k1): (in, k1, k2, out, count). */
int __sce_dnas2_symbol12143(void *in, void *key1, void *key2, void *out, void *count)
{
	return __sce_dnas2_symbol12148(in, key1, key2, key1, out, count);
}

/* Two-key triple-DES-ECB decryption: (in, k1, k2, out, count). */
int __sce_dnas2_symbol12142(void *in, void *key1, void *key2, void *out, void *count)
{
	return __sce_dnas2_symbol12147(in, key1, key2, key1, out, count);
}

/* Two-key triple-DES-CBC encryption: (in, k1, k2, out, iv, count). */
int __sce_dnas2_symbol12141(void *in, void *key1, void *key2, void *out, void *iv, unsigned int count)
{
	return __sce_dnas2_symbol12112(in, key1, key2, key1, out, iv, count);
}

/* Two-key triple-DES-CBC decryption: (in, k1, k2, out, iv, count). */
int __sce_dnas2_symbol12139(void *in, void *key1, void *key2, void *out, void *iv, unsigned int count)
{
	return __sce_dnas2_symbol12144(in, key1, key2, key1, out, iv, count);
}

/* Same as __sce_dnas2_symbol12118 (DES-CBC decryption). */
int __sce_dnas2_symbol12058(void *in, void *key, void *out, void *iv, unsigned int count)
{
	void *local;
	int status;

	status = __sce_dnas2_symbol12121(key, iv, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol12123(local, in, out, count);
		if (status == 0) {
			status = __sce_dnas2_symbol12129(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Same as __sce_dnas2_symbol12144 (triple-DES-CBC decryption). */
int __sce_dnas2_symbol12111(void *in, void *key1, void *key2, void *key3, void *out, void *iv, unsigned int count)
{
	void *local;

	if (__sce_dnas2_symbol12131(key1, key2, key3, iv, &local) == 0) {
		if (__sce_dnas2_symbol12133(local, in, out, count) == 0) {
			return __sce_dnas2_symbol12137(local) == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Same as __sce_dnas2_symbol12139 (two-key triple-DES-CBC decryption). */
int __sce_dnas2_symbol12140(void *in, void *key1, void *key2, void *out, void *iv, unsigned int count)
{
	return __sce_dnas2_symbol12111(in, key1, key2, key1, out, iv, count);
}

/* SHA-1 initial state. */
static void dnas2_sha1_init(unsigned int *h)
{
	h[0] = 0x67452301;
	h[1] = 0xefcdab89;
	h[2] = 0x98badcfe;
	h[3] = 0x10325476;
	h[4] = 0xc3d2e1f0;
}

/* SHA-1 transform: four loops of 20 rounds, with W[16] expanded in place from round 16. */
static void dnas2_sha1_transform(unsigned int *w, unsigned int *h)
{
	unsigned int a, b, c, d, e, t;
	int i;
	unsigned int j;

	a = h[0];
	b = h[1];
	c = h[2];
	d = h[3];
	e = h[4];
	i = 0;

	while (i < 20) {
		j = i & 0xf;
		if (i >= 16) {
			unsigned int x = w[(j + 13) & 0xf] ^ w[(j + 8) & 0xf] ^ w[(j + 2) & 0xf] ^ w[j];

			w[j] = (x << 1) | (x >> 31);
		}
		t = ((a << 5) | (a >> 27)) + ((b & c) | (~b & d)) + e + w[j] + 0x5a827999;
		e = d;
		d = c;
		b = (b << 30) | (b >> 2);
		c = b;
		b = a;
		a = t;
		i++;
	}
	while (i < 40) {
		j = i & 0xf;
		if (i >= 16) {
			unsigned int x = w[(j + 13) & 0xf] ^ w[(j + 8) & 0xf] ^ w[(j + 2) & 0xf] ^ w[j];

			w[j] = (x << 1) | (x >> 31);
		}
		t = ((a << 5) | (a >> 27)) + (b ^ c ^ d) + e + w[j] + 0x6ed9eba1;
		e = d;
		d = c;
		b = (b << 30) | (b >> 2);
		c = b;
		b = a;
		a = t;
		i++;
	}
	while (i < 60) {
		j = i & 0xf;
		if (i >= 16) {
			unsigned int x = w[(j + 13) & 0xf] ^ w[(j + 8) & 0xf] ^ w[(j + 2) & 0xf] ^ w[j];

			w[j] = (x << 1) | (x >> 31);
		}
		t = ((a << 5) | (a >> 27)) + ((b & (c | d)) | (c & d)) + e + w[j] + 0x8f1bbcdc;
		e = d;
		d = c;
		b = (b << 30) | (b >> 2);
		c = b;
		b = a;
		a = t;
		i++;
	}
	while (i < 80) {
		j = i & 0xf;
		if (i >= 16) {
			unsigned int x = w[(j + 13) & 0xf] ^ w[(j + 8) & 0xf] ^ w[(j + 2) & 0xf] ^ w[j];

			w[j] = (x << 1) | (x >> 31);
		}
		t = ((a << 5) | (a >> 27)) + (b ^ c ^ d) + e + w[j] + 0xca62c1d6;
		e = d;
		d = c;
		b = (b << 30) | (b >> 2);
		c = b;
		b = a;
		a = t;
		i++;
	}

	h[0] += a;
	h[1] += b;
	h[2] += c;
	h[3] += d;
	h[4] += e;
}

/* SHA-1 of `len` bytes of `data` into the 20-byte `out`. */
void __sce_dnas2_symbol12149(unsigned char *data, int len, unsigned char *out)
{
	unsigned int w[16];
	unsigned int state[8];
	unsigned char block[64];
	unsigned int bits = len << 3;
	unsigned char *p;
	int i;
	int k;

	dnas2_sha1_init(state);

	while (len >= 48) {
		if (len < 64) {
			unsigned char *q, *r;
			int j;

			p = block;
			for (i = 0; i < len; i++)
				p[i] = data[i];
			q = p + len;
			*q = 0x80;
			if (63 - len > 0) {
				r = q + 1;
				for (j = 0; j < 63 - len; j++)
					r[j] = 0;
			}
		} else {
			p = data;
		}
		for (k = 0; k < 16; k++)
			w[k] = (p[4 * k] << 24) | (p[4 * k + 1] << 16) | (p[4 * k + 2] << 8) | p[4 * k + 3];
		dnas2_sha1_transform(w, state);
		len -= 64;
		data += 64;
	}

	if (len > 0) {
		unsigned char *q, *r;
		int j;

		p = block;
		for (i = 0; i < len; i++)
			p[i] = data[i];
		q = p + len;
		*q = 0x80;
		if (55 - len > 0) {
			r = q + 1;
			for (j = 0; j < 55 - len; j++)
				r[j] = 0;
		}
		for (k = 0; k < 14; k++)
			w[k] = (p[4 * k] << 24) | (p[4 * k + 1] << 16) | (p[4 * k + 2] << 8) | p[4 * k + 3];
	} else {
		w[0] = len ? 0 : 0x80000000;
		for (k = 1; k < 14; k++)
			w[k] = 0;
	}
	w[14] = 0;
	w[15] = bits;
	dnas2_sha1_transform(w, state);

	for (k = 0; k < 5; k++) {
		out[0] = (unsigned char)(state[k] >> 24);
		out[1] = (unsigned char)(state[k] >> 16);
		out[2] = (unsigned char)(state[k] >> 8);
		out[3] = (unsigned char)state[k];
		out += 4;
	}
}

/* Streaming variant of 12149. Flags:
 *   bit0 = 1 -> fresh state; 0 -> state loaded from the 20 big-endian bytes
 *               of `digest` (in/out).
 *   bit1 = 1 -> fails if len is not a multiple of 64.
 *   bit2 = 1 -> final compression with W[14..15] = bit length.
 * The bit length comes from the 4th argument (nbytes << 3), not from len.
 */
void __sce_dnas2_symbol12150(unsigned char *data, int len, unsigned char *digest, int nbytes, int flags)
{
	unsigned int w[16];
	unsigned int state[8];
	unsigned char block[64];
	unsigned int bits = nbytes << 3;
	unsigned char *p;
	int i;
	int k;

	if (flags & 1) {
		dnas2_sha1_init(state);
	} else {
		p = digest;
		for (k = 0; k < 5; k++) {
			state[k] = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
			p += 4;
		}
	}

	if (flags & 2) {
		if (len % 64 > 0)
			return;
	}

	while (len >= 48) {
		if (len < 64) {
			unsigned char *q, *r;
			int j;

			p = block;
			for (i = 0; i < len; i++)
				p[i] = data[i];
			q = p + len;
			*q = 0x80;
			if (63 - len > 0) {
				r = q + 1;
				for (j = 0; j < 63 - len; j++)
					r[j] = 0;
			}
		} else {
			p = data;
		}
		for (k = 0; k < 16; k++)
			w[k] = (p[4 * k] << 24) | (p[4 * k + 1] << 16) | (p[4 * k + 2] << 8) | p[4 * k + 3];
		dnas2_sha1_transform(w, state);
		len -= 64;
		data += 64;
	}

	if (len > 0) {
		unsigned char *q, *r;
		int j;

		p = block;
		for (i = 0; i < len; i++)
			p[i] = data[i];
		q = p + len;
		*q = 0x80;
		if (55 - len > 0) {
			r = q + 1;
			for (j = 0; j < 55 - len; j++)
				r[j] = 0;
		}
		for (k = 0; k < 14; k++)
			w[k] = (p[4 * k] << 24) | (p[4 * k + 1] << 16) | (p[4 * k + 2] << 8) | p[4 * k + 3];
	} else {
		w[0] = len ? 0 : 0x80000000;
		for (k = 1; k < 14; k++)
			w[k] = 0;
	}

	if (flags & 4) {
		w[14] = 0;
		w[15] = bits;
		dnas2_sha1_transform(w, state);
	}

	for (k = 0; k < 5; k++) {
		digest[0] = (unsigned char)(state[k] >> 24);
		digest[1] = (unsigned char)(state[k] >> 16);
		digest[2] = (unsigned char)(state[k] >> 8);
		digest[3] = (unsigned char)state[k];
		digest += 4;
	}
}
