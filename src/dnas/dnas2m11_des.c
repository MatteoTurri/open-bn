/*
 * DNAS2 family 11: DES (ECB, CBC, EDE) through BSAFE; the twin of
 * dnas2m12_des.c with its own keys.
 */

#include <string.h>

#include "../../ref/rcom-1.1.0/bsafe.h"
#include "dnas2_bsafe.h"
#include "dnas2m11_des.h"

static unsigned char *dnas2_unmask8(unsigned char *key, unsigned char *blk);
static int dnas2_encrypt_obj(void *keyData, void *methods, unsigned char *cipherName, unsigned char *feedbackName,
    int *feedbackParams, void *keyInfoType, void *objOut);
static int dnas2_decrypt_obj(void *keyData, void *methods, unsigned char *cipherName, unsigned char *feedbackName,
    int *feedbackParams, void *keyInfoType, void *objOut);

/* XOR key and the BSAFE method names it hides (AI_FeedbackCipher parameters). */
static unsigned char dnas2_nameMask[8] = {
#include "../../assets/dnas/dnas2m11_des_name_mask.inc"
};
static const Bsafe8 dnas2_name_nopad = { {
#include "../../assets/dnas/dnas2m11_des_name_nopad.inc"
} }; /* "nopad" */
static const Bsafe8 dnas2_name_des = { {
#include "../../assets/dnas/dnas2m11_des_name_des.inc"
} }; /* "des" */
static const Bsafe8 dnas2_name_ecb = { {
#include "../../assets/dnas/dnas2m11_des_name_ecb.inc"
} }; /* "ecb" */
static const Bsafe8 dnas2_name_cbc = { {
#include "../../assets/dnas/dnas2m11_des_name_cbc.inc"
} }; /* "cbc" */
static const Bsafe8 dnas2_name_des_ede = { {
#include "../../assets/dnas/dnas2m11_des_name_des_ede.inc"
} }; /* "des_ede" */

/* XORs the 8-byte key into `blk` and returns `blk`. */
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
int __sce_dnas2_symbol11097(void *key, void *iv, void *objOut)
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
int __sce_dnas2_symbol11107(void *key1, void *key2, void *key3, void *ivData, void **objOut)
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
int __sce_dnas2_symbol11100(void *obj, void *in, void *out, unsigned int count)
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

/* In-place form of __sce_dnas2_symbol11100. */
int __sce_dnas2_symbol11101(void *obj, void *buf, unsigned int count)
{
	return __sce_dnas2_symbol11100(obj, buf, buf, count);
}

/* Same as __sce_dnas2_symbol11100. */
int __sce_dnas2_symbol11110(void *obj, void *in, void *out, unsigned int count)
{
	return __sce_dnas2_symbol11100(obj, in, out, count);
}

/* Same as __sce_dnas2_symbol11101. */
int __sce_dnas2_symbol11111(void *obj, void *buf, unsigned int count)
{
	return __sce_dnas2_symbol11100(obj, buf, buf, count);
}

/* Finishes an encryption (B_EncryptFinal into a scratch buffer), destroys
 * the object and returns the Final status. */
int __sce_dnas2_symbol11105(void *obj)
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

/* Same as __sce_dnas2_symbol11105. */
int __sce_dnas2_symbol11113(void *obj)
{
	return __sce_dnas2_symbol11105(obj);
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
int __sce_dnas2_symbol11096(void *key, void *iv, void *objOut)
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
int __sce_dnas2_symbol11106(void *key1, void *key2, void *key3, void *ivData, void **objOut)
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
int __sce_dnas2_symbol11098(void *obj, void *in, void *out, unsigned int count)
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

/* In-place form of __sce_dnas2_symbol11098. */
int __sce_dnas2_symbol11099(void *obj, void *buf, unsigned int count)
{
	return __sce_dnas2_symbol11098(obj, buf, buf, count);
}

/* Same as __sce_dnas2_symbol11098. */
int __sce_dnas2_symbol11108(void *obj, void *in, void *out, unsigned int count)
{
	return __sce_dnas2_symbol11098(obj, in, out, count);
}

/* Same as __sce_dnas2_symbol11099. */
int __sce_dnas2_symbol11109(void *obj, void *buf, unsigned int count)
{
	return __sce_dnas2_symbol11098(obj, buf, buf, count);
}

/* Finishes a decryption, destroys the object and returns the Final status. */
int __sce_dnas2_symbol11104(void *obj)
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

/* Same as __sce_dnas2_symbol11104. */
int __sce_dnas2_symbol11112(void *obj)
{
	return __sce_dnas2_symbol11104(obj);
}

/* DES-ECB encryption: (in, key, out, count); 0 or -1. */
int __sce_dnas2_symbol11060(void *in, void *key, void *out, int count)
{
	void *local;
	int status;
	status = dnas2_des_ecb_encrypt_obj(key, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol11100(local, in, out, count);
		if (status == 0) {
			status = __sce_dnas2_symbol11105(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* DES-ECB decryption: (in, key, out, count); 0 or -1. */
int __sce_dnas2_symbol11059(void *in, void *key, void *out, int count)
{
	void *local;
	int status;
	status = dnas2_des_ecb_decrypt_obj(key, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol11098(local, in, out, count);
		if (status == 0) {
			status = __sce_dnas2_symbol11104(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* DES-CBC encryption: (in, key, out, iv, count); 0 or -1. */
int __sce_dnas2_symbol11058(void *in, void *key, void *out, void *iv, unsigned int count)
{
	void *local;
	int status;
	status = __sce_dnas2_symbol11097(key, iv, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol11100(local, in, out, count);
		if (status == 0) {
			status = __sce_dnas2_symbol11105(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* DES-CBC decryption: (in, key, out, iv, count); 0 or -1. */
int __sce_dnas2_symbol11093(void *in, void *key, void *out, void *iv, unsigned int count)
{
	void *local;
	int status;
	status = __sce_dnas2_symbol11096(key, iv, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol11098(local, in, out, count);
		if (status == 0) {
			status = __sce_dnas2_symbol11104(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Triple-DES-ECB encryption: (in, k1, k2, k3, out, count); 0 or -1. */
int __sce_dnas2_symbol11123(void *in, void *key1, void *key2, void *key3, void *out, void *count)
{
	void *local;
	int status;
	status = dnas2_des_ede_ecb_encrypt_obj(key1, key2, key3, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol11110(local, in, out, (unsigned int)count);
		if (status == 0) {
			status = __sce_dnas2_symbol11113(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Triple-DES-ECB decryption: (in, k1, k2, k3, out, count); 0 or -1. */
int __sce_dnas2_symbol11122(void *in, void *key1, void *key2, void *key3, void *out, void *count)
{
	void *local;
	int status;
	status = dnas2_des_ede_ecb_decrypt_obj(key1, key2, key3, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol11108(local, in, out, (unsigned int)count);
		if (status == 0) {
			status = __sce_dnas2_symbol11112(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Triple-DES-CBC encryption: (in, k1, k2, k3, out, iv, count); 0 or -1. */
int __sce_dnas2_symbol11089(void *in, void *key1, void *key2, void *key3, void *out, void *iv, unsigned int count)
{
	void *local;

	if (__sce_dnas2_symbol11107(key1, key2, key3, iv, &local) == 0) {
		if (__sce_dnas2_symbol11110(local, in, out, count) == 0) {
			return __sce_dnas2_symbol11113(local) == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Triple-DES-CBC decryption: (in, k1, k2, k3, out, iv, count); 0 or -1. */
int __sce_dnas2_symbol11119(void *in, void *key1, void *key2, void *key3, void *out, void *iv, unsigned int count)
{
	void *local;

	if (__sce_dnas2_symbol11106(key1, key2, key3, iv, &local) == 0) {
		if (__sce_dnas2_symbol11108(local, in, out, count) == 0) {
			return __sce_dnas2_symbol11112(local) == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Two-key triple-DES-ECB encryption (k1, k2, k1): (in, k1, k2, out, count). */
int __sce_dnas2_symbol11118(void *in, void *key1, void *key2, void *out, void *count)
{
	return __sce_dnas2_symbol11123(in, key1, key2, key1, out, count);
}

/* Two-key triple-DES-ECB decryption: (in, k1, k2, out, count). */
int __sce_dnas2_symbol11117(void *in, void *key1, void *key2, void *out, void *count)
{
	return __sce_dnas2_symbol11122(in, key1, key2, key1, out, count);
}

/* Two-key triple-DES-CBC encryption: (in, k1, k2, out, iv, count). */
int __sce_dnas2_symbol11116(void *in, void *key1, void *key2, void *out, void *iv, unsigned int count)
{
	return __sce_dnas2_symbol11089(in, key1, key2, key1, out, iv, count);
}

/* Two-key triple-DES-CBC decryption: (in, k1, k2, out, iv, count). */
int __sce_dnas2_symbol11114(void *in, void *key1, void *key2, void *out, void *iv, unsigned int count)
{
	return __sce_dnas2_symbol11119(in, key1, key2, key1, out, iv, count);
}

/* Same as __sce_dnas2_symbol11093 (DES-CBC decryption). */
int __sce_dnas2_symbol11094(void *in, void *key, void *out, void *iv, unsigned int count)
{
	void *local;
	int status;
	status = __sce_dnas2_symbol11096(key, iv, &local);
	if (status == 0) {
		status = __sce_dnas2_symbol11098(local, in, out, count);
		if (status == 0) {
			status = __sce_dnas2_symbol11104(local);
			return status == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Same as __sce_dnas2_symbol11119 (triple-DES-CBC decryption). */
int __sce_dnas2_symbol11088(void *in, void *key1, void *key2, void *key3, void *out, void *iv, unsigned int count)
{
	void *local;

	if (__sce_dnas2_symbol11106(key1, key2, key3, iv, &local) == 0) {
		if (__sce_dnas2_symbol11108(local, in, out, count) == 0) {
			return __sce_dnas2_symbol11112(local) == 0 ? 0 : -1;
		}
	}
	return -1;
}

/* Same as __sce_dnas2_symbol11114 (two-key triple-DES-CBC decryption). */
int __sce_dnas2_symbol11115(void *in, void *key1, void *key2, void *out, void *iv, unsigned int count)
{
	return __sce_dnas2_symbol11088(in, key1, key2, key1, out, iv, count);
}
