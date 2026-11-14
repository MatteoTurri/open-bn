/*
 * DNAS2 family 12: RC6 in CBC mode through BSAFE, with whole-message
 * encrypt and decrypt calls.
 */

#include <string.h>

#include "../../ref/rcom-1.1.0/bsafe.h"
#include "dnas2_bsafe.h"
#include "dnas2m12_rc6.h"

/* XOR key and the BSAFE method names it hides: "rc6" (20 rounds), "cbc", "nopad". */
static unsigned char dnas2_nameMask[8] = {
#include "../../assets/dnas/dnas2m12_rc6_name_mask.inc"
};
static const Bsafe8 dnas2_name_rc6 = { {
#include "../../assets/dnas/dnas2m12_rc6_name_rc6.inc"
} };
static const Bsafe8 dnas2_name_cbc = { {
#include "../../assets/dnas/dnas2m12_rc6_name_cbc.inc"
} };
static const Bsafe8 dnas2_name_nopad = { {
#include "../../assets/dnas/dnas2m12_rc6_name_nopad.inc"
} };

static int dnas2_rc6_decrypt(void *, unsigned char *, unsigned char *, unsigned char *, int, unsigned int);
static int dnas2_rc6_decrypt_obj(unsigned char *, unsigned char *, unsigned int, BsafeAlgorithmObj **);

/* XORs the 8-byte mask into `blk` and returns `blk`. */
static unsigned char *dnas2_unmask8(unsigned char *mask, unsigned char *blk)
{
	int i;

	for (i = 0; i < 8; i++)
		blk[i] ^= mask[i];

	return blk;
}

/* Encrypts nblk 16-byte blocks in one call (Init, Update, Final) with RC6-CBC:
 * key keyData of keyLen bytes, 16-byte IV iv. */
static int dnas2_rc6_encrypt(void *in, unsigned char *keyData, unsigned char *out, unsigned char *iv, int nblk,
    unsigned int keyLen)
{
	Bsafe8 cipherName = dnas2_name_rc6;
	Bsafe8 feedbackName = dnas2_name_cbc;
	Bsafe8 paddingName = dnas2_name_nopad;
	B_AlgorithmMethod *chooser[3] = { &AM_RC6_ENCRYPT, &AM_CBC_ENCRYPT, 0 };
	ITEM keyItem;
	ITEM ivItem;
	B_BLK_CIPHER_W_FEEDBACK_PARAMS params;
	int rounds;
	int outLen1;
	int outLen2;
	BsafeKeyObj *keyObj;
	BsafeAlgorithmObj *alg;
	int status;

	memset(&keyItem, 0, 8);
	memset(&ivItem, 0, 8);
	keyObj = 0;
	alg = 0;
	do {
		if ((status = B_CreateKeyObject(&keyObj)) != 0)
			break;
		keyItem.len = keyLen;
		keyItem.data = keyData;
		if ((status = B_SetKeyInfo(keyObj, &KI_Item, &keyItem)) != 0)
			break;
		if ((status = B_CreateAlgorithmObject(&alg)) != 0)
			break;
		ivItem.data = iv;
		ivItem.len = 16;
		rounds = 20;
		params.encryptionMethodName = dnas2_unmask8(dnas2_nameMask, cipherName.b);
		params.encryptionParams = &rounds;
		params.feedbackMethodName = dnas2_unmask8(dnas2_nameMask, feedbackName.b);
		params.feedbackParams = &ivItem;
		params.paddingMethodName = dnas2_unmask8(dnas2_nameMask, paddingName.b);
		params.paddingParams = 0;
		if ((status = B_SetAlgorithmInfo(alg, &AI_FeedbackCipher, &params)) != 0)
			break;
		if ((status = B_EncryptInit(alg, keyObj, chooser, 0)) != 0)
			break;
		if ((status = B_EncryptUpdate(alg, out, &outLen1, nblk << 4, in, nblk << 4, 0, 0)) != 0)
			break;
		status = B_EncryptFinal(alg, out + outLen1, &outLen2, (nblk << 4) - outLen1, 0, 0);
	} while (0);
	B_DestroyKeyObject(&keyObj);
	B_DestroyAlgorithmObject(&alg);
	return status;
}

/* As dnas2_rc6_encrypt, decrypting. */
static int dnas2_rc6_decrypt(void *in, unsigned char *keyData, unsigned char *out, unsigned char *iv, int nblk,
    unsigned int keyLen)
{
	Bsafe8 cipherName = dnas2_name_rc6;
	Bsafe8 feedbackName = dnas2_name_cbc;
	Bsafe8 paddingName = dnas2_name_nopad;
	B_AlgorithmMethod *chooser[3] = { &AM_RC6_DECRYPT, &AM_CBC_DECRYPT, 0 };
	ITEM keyItem;
	ITEM ivItem;
	B_BLK_CIPHER_W_FEEDBACK_PARAMS params;
	int rounds;
	int outLen1;
	int outLen2;
	BsafeKeyObj *keyObj;
	BsafeAlgorithmObj *alg;
	int status;

	memset(&keyItem, 0, 8);
	memset(&ivItem, 0, 8);
	keyObj = 0;
	alg = 0;
	do {
		if ((status = B_CreateKeyObject(&keyObj)) != 0)
			break;
		keyItem.len = keyLen;
		keyItem.data = keyData;
		if ((status = B_SetKeyInfo(keyObj, &KI_Item, &keyItem)) != 0)
			break;
		if ((status = B_CreateAlgorithmObject(&alg)) != 0)
			break;
		ivItem.data = iv;
		ivItem.len = 16;
		rounds = 20;
		params.encryptionMethodName = dnas2_unmask8(dnas2_nameMask, cipherName.b);
		params.encryptionParams = &rounds;
		params.feedbackMethodName = dnas2_unmask8(dnas2_nameMask, feedbackName.b);
		params.feedbackParams = &ivItem;
		params.paddingMethodName = dnas2_unmask8(dnas2_nameMask, paddingName.b);
		params.paddingParams = 0;
		if ((status = B_SetAlgorithmInfo(alg, &AI_FeedbackCipher, &params)) != 0)
			break;
		if ((status = B_DecryptInit(alg, keyObj, chooser, 0)) != 0)
			break;
		if ((status = B_DecryptUpdate(alg, out, &outLen1, nblk << 4, in, nblk << 4, 0, 0)) != 0)
			break;
		status = B_DecryptFinal(alg, out + outLen1, &outLen2, (nblk << 4) - outLen1, 0, 0);
	} while (0);
	B_DestroyKeyObject(&keyObj);
	B_DestroyAlgorithmObject(&alg);
	return status;
}

/* RC6-CBC encryption with a 16-byte key: (in, key, out, iv, count). */
int __sce_dnas2_symbol12235(void *in, void *key, void *out, void *iv, unsigned int count)
{
	return dnas2_rc6_encrypt(in, key, out, iv, count, 16);
}

/* RC6-CBC decryption with a 16-byte key: (in, key, out, iv, count). */
int __sce_dnas2_symbol12233(void *in, void *key, void *out, void *iv, unsigned int count)
{
	return dnas2_rc6_decrypt(in, key, out, iv, count, 16);
}

/* RC6-CBC encryption with a 24-byte key: (in, key, out, iv, count). */
int __sce_dnas2_symbol12241(void *in, void *key, void *out, void *iv, unsigned int count)
{
	return dnas2_rc6_encrypt(in, key, out, iv, count, 24);
}

/* RC6-CBC decryption with a 24-byte key: (in, key, out, iv, count). */
int __sce_dnas2_symbol12239(void *in, void *key, void *out, void *iv, unsigned int count)
{
	return dnas2_rc6_decrypt(in, key, out, iv, count, 24);
}

/* RC6-CBC encryption with a 32-byte key: (in, key, out, iv, count). */
int __sce_dnas2_symbol12247(void *in, void *key, void *out, void *iv, unsigned int count)
{
	return dnas2_rc6_encrypt(in, key, out, iv, count, 32);
}

/* RC6-CBC decryption with a 32-byte key: (in, key, out, iv, count). */
int __sce_dnas2_symbol12245(void *in, void *key, void *out, void *iv, unsigned int count)
{
	return dnas2_rc6_decrypt(in, key, out, iv, count, 32);
}

/* In-place RC6-CBC encryption with a 16-byte key: (buf, key, iv, count). */
int __sce_dnas2_symbol12236(void *buf, void *key, void *iv, unsigned int count)
{
	return dnas2_rc6_encrypt(buf, key, buf, iv, count, 16);
}

/* In-place RC6-CBC decryption with a 16-byte key: (buf, key, iv, count). */
int __sce_dnas2_symbol12234(void *buf, void *key, void *iv, unsigned int count)
{
	return dnas2_rc6_decrypt(buf, key, buf, iv, count, 16);
}

/* In-place RC6-CBC encryption with a 24-byte key: (buf, key, iv, count). */
int __sce_dnas2_symbol12242(void *buf, void *key, void *iv, unsigned int count)
{
	return dnas2_rc6_encrypt(buf, key, buf, iv, count, 24);
}

/* In-place RC6-CBC decryption with a 24-byte key: (buf, key, iv, count). */
int __sce_dnas2_symbol12240(void *buf, void *key, void *iv, unsigned int count)
{
	return dnas2_rc6_decrypt(buf, key, buf, iv, count, 24);
}

/* In-place RC6-CBC encryption with a 32-byte key: (buf, key, iv, count). */
int __sce_dnas2_symbol12248(void *buf, void *key, void *iv, unsigned int count)
{
	return dnas2_rc6_encrypt(buf, key, buf, iv, count, 32);
}

/* In-place RC6-CBC decryption with a 32-byte key: (buf, key, iv, count). */
int __sce_dnas2_symbol12246(void *buf, void *key, void *iv, unsigned int count)
{
	return dnas2_rc6_decrypt(buf, key, buf, iv, count, 32);
}

/* Creates an RC6-CBC encryption object (key keyData of keyLen bytes, 16-byte IV iv)
 * and returns it in *objOut. */
static int dnas2_rc6_encrypt_obj(unsigned char *keyData, unsigned char *iv, unsigned int keyLen,
    BsafeAlgorithmObj **objOut)
{
	Bsafe8 cipherName = dnas2_name_rc6;
	Bsafe8 feedbackName = dnas2_name_cbc;
	Bsafe8 paddingName = dnas2_name_nopad;
	B_AlgorithmMethod *chooser[3] = { &AM_RC6_ENCRYPT, &AM_CBC_ENCRYPT, 0 };
	ITEM keyItem;
	ITEM ivItem;
	B_BLK_CIPHER_W_FEEDBACK_PARAMS params;
	int rounds;
	BsafeKeyObj *keyObj;
	BsafeAlgorithmObj *alg;
	int status;

	memset(&keyItem, 0, 8);
	memset(&ivItem, 0, 8);
	keyObj = 0;
	alg = 0;
	do {
		if ((status = B_CreateKeyObject(&keyObj)) != 0)
			break;
		keyItem.len = keyLen;
		keyItem.data = keyData;
		if ((status = B_SetKeyInfo(keyObj, &KI_Item, &keyItem)) != 0)
			break;
		if ((status = B_CreateAlgorithmObject(&alg)) != 0)
			break;
		ivItem.data = iv;
		ivItem.len = 16;
		rounds = 20;
		params.encryptionMethodName = dnas2_unmask8(dnas2_nameMask, cipherName.b);
		params.encryptionParams = &rounds;
		params.feedbackMethodName = dnas2_unmask8(dnas2_nameMask, feedbackName.b);
		params.feedbackParams = &ivItem;
		params.paddingMethodName = dnas2_unmask8(dnas2_nameMask, paddingName.b);
		params.paddingParams = 0;
		if ((status = B_SetAlgorithmInfo(alg, &AI_FeedbackCipher, &params)) != 0)
			break;
		status = B_EncryptInit(alg, keyObj, chooser, 0);
	} while (0);
	B_DestroyKeyObject(&keyObj);
	*objOut = alg;
	return status;
}

/* RC6-CBC encryption object for a 16-byte key: (key, iv, &obj). */
int __sce_dnas2_symbol12238(void *key, void *iv, void *objOut)
{
	return dnas2_rc6_encrypt_obj(key, iv, 16, objOut);
}

/* RC6-CBC encryption object for a 24-byte key: (key, iv, &obj). */
int __sce_dnas2_symbol12244(void *key, void *iv, void *objOut)
{
	return dnas2_rc6_encrypt_obj(key, iv, 24, objOut);
}

/* RC6-CBC encryption object for a 32-byte key: (key, iv, &obj). */
int __sce_dnas2_symbol12250(void *key, void *iv, void *objOut)
{
	return dnas2_rc6_encrypt_obj(key, iv, 32, objOut);
}

/* Encrypts count * 16 bytes with an object from 12238/12244/12250; -2 unless
 * exactly that many bytes come out. */
int __sce_dnas2_symbol12253(void *obj, void *in, void *out, unsigned int count)
{
	int status;
	unsigned int outLen;
	unsigned int len;
	unsigned int n = count * 16;

	status = B_EncryptUpdate(obj, out, &outLen, n, in, n, 0, 0);
	len = outLen;
	if (status == 0)
		status = (len == n) ? 0 : -2;
	return status;
}

/* In-place form of __sce_dnas2_symbol12253. */
int __sce_dnas2_symbol12254(void *obj, void *buf, unsigned int count)
{
	return __sce_dnas2_symbol12253(obj, buf, buf, count);
}

/* Finishes an encryption (B_EncryptFinal into a scratch buffer), destroys
 * the object and returns the Final status. */
int __sce_dnas2_symbol12256(void *obj)
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

/* As dnas2_rc6_encrypt_obj, with B_DecryptInit. */
static int dnas2_rc6_decrypt_obj(unsigned char *keyData, unsigned char *iv, unsigned int keyLen,
    BsafeAlgorithmObj **objOut)
{
	Bsafe8 cipherName = dnas2_name_rc6;
	Bsafe8 feedbackName = dnas2_name_cbc;
	Bsafe8 paddingName = dnas2_name_nopad;
	B_AlgorithmMethod *chooser[3] = { &AM_RC6_DECRYPT, &AM_CBC_DECRYPT, 0 };
	ITEM keyItem;
	ITEM ivItem;
	B_BLK_CIPHER_W_FEEDBACK_PARAMS params;
	int rounds;
	BsafeKeyObj *keyObj;
	BsafeAlgorithmObj *alg;
	int status;

	memset(&keyItem, 0, 8);
	memset(&ivItem, 0, 8);
	keyObj = 0;
	alg = 0;
	do {
		if ((status = B_CreateKeyObject(&keyObj)) != 0)
			break;
		keyItem.len = keyLen;
		keyItem.data = keyData;
		if ((status = B_SetKeyInfo(keyObj, &KI_Item, &keyItem)) != 0)
			break;
		if ((status = B_CreateAlgorithmObject(&alg)) != 0)
			break;
		ivItem.data = iv;
		ivItem.len = 16;
		rounds = 20;
		params.encryptionMethodName = dnas2_unmask8(dnas2_nameMask, cipherName.b);
		params.encryptionParams = &rounds;
		params.feedbackMethodName = dnas2_unmask8(dnas2_nameMask, feedbackName.b);
		params.feedbackParams = &ivItem;
		params.paddingMethodName = dnas2_unmask8(dnas2_nameMask, paddingName.b);
		params.paddingParams = 0;
		if ((status = B_SetAlgorithmInfo(alg, &AI_FeedbackCipher, &params)) != 0)
			break;
		status = B_DecryptInit(alg, keyObj, chooser, 0);
	} while (0);
	B_DestroyKeyObject(&keyObj);
	*objOut = alg;
	return status;
}

/* RC6-CBC decryption object for a 16-byte key: (key, iv, &obj). */
int __sce_dnas2_symbol12237(void *key, void *iv, void *objOut)
{
	return dnas2_rc6_decrypt_obj(key, iv, 16, objOut);
}

/* RC6-CBC decryption object for a 24-byte key: (key, iv, &obj). */
int __sce_dnas2_symbol12243(void *key, void *iv, void *objOut)
{
	return dnas2_rc6_decrypt_obj(key, iv, 24, objOut);
}

/* RC6-CBC decryption object for a 32-byte key: (key, iv, &obj). */
int __sce_dnas2_symbol12249(void *key, void *iv, void *objOut)
{
	return dnas2_rc6_decrypt_obj(key, iv, 32, objOut);
}

/* Decrypts count * 16 bytes; -2 unless exactly that many bytes come out. */
int __sce_dnas2_symbol12251(void *obj, void *in, void *out, unsigned int count)
{
	int status;
	unsigned int outLen;
	unsigned int len;
	unsigned int n = count * 16;

	status = B_DecryptUpdate(obj, out, &outLen, n, in, n, 0, 0);
	len = outLen;
	if (status == 0)
		status = (len == n) ? 0 : -2;
	return status;
}

/* In-place form of __sce_dnas2_symbol12251. */
int __sce_dnas2_symbol12252(void *obj, void *buf, unsigned int count)
{
	return __sce_dnas2_symbol12251(obj, buf, buf, count);
}

/* Finishes a decryption, destroys the object and returns the Final status. */
int __sce_dnas2_symbol12255(void *obj)
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
