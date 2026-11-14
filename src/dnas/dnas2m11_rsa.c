/*
 * DNAS2 RSA decryption (BSAFE): decrypts a 1024-bit block with a BER-encoded
 * public key. Returns 0, or -1 on any failure.
 */

#include <string.h>

#include "../../ref/rcom-1.1.0/bsafe.h"
#include "dnas2_bsafe.h"
#include "dnas2m11_rsa.h"

/* RSA public-key decryption of the 128-byte block in, with the BER key keyData of
 * keyLen bytes; the length goes to *outLen. 0 or -1. */
int __sce_dnas2_symbol11087(unsigned char *in, unsigned int inLen, unsigned char *keyData, unsigned int keyLen,
    unsigned char *out, unsigned int maxOut, unsigned int *outLen)
{
	B_AlgorithmMethod *chooser[2] = { &AM_RSA_DECRYPT, 0 };
	ITEM keyItem;
	BsafeKeyObj *key;
	unsigned int updLen;
	unsigned int finLen;
	BsafeAlgorithmObj *alg;
	int status;

	alg = 0;
	key = 0;
	memset(&keyItem, 0, sizeof(keyItem));

	for (;;) {
		if (inLen != 128) {
			status = -1;
			break;
		}
		if (maxOut < 128) {
			status = -1;
			break;
		}
		keyItem.len = keyLen;
		keyItem.data = (unsigned char *)T_malloc(keyLen);
		if (keyItem.data == 0) {
			status = -1;
			break;
		}
		T_memcpy(keyItem.data, keyData, keyItem.len);
		status = B_CreateAlgorithmObject(&alg);
		if (status != 0)
			break;
		status = B_SetAlgorithmInfo(alg, &AI_PKCS_RSAPublic, 0);
		if (status != 0)
			break;
		status = B_CreateKeyObject(&key);
		if (status != 0)
			break;
		status = B_SetKeyInfo(key, &KI_RSAPublicBER, &keyItem);
		if (status != 0)
			break;
		status = B_DecryptInit(alg, key, chooser, 0);
		if (status != 0)
			break;
		status = B_DecryptUpdate(alg, out, &updLen, maxOut, in, inLen, 0, 0);
		if (status != 0)
			break;
		status = B_DecryptFinal(alg, out + updLen, &finLen, maxOut - updLen, 0, 0);
		if (status != 0)
			break;
		*outLen = updLen + finLen;
		break;
	}

	if (status != 0)
		status = -1;
	B_DestroyAlgorithmObject(&alg);
	B_DestroyKeyObject(&key);
	T_free(keyItem.data);
	return status;
}
