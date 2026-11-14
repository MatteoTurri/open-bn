/*
 * KIT_RSAPublicBER: the key info type of an RSA public key in a BER SubjectPublicKeyInfo, between
 * the KIT_RSAPublic key and keyinfo_ber.c.
 */

#include "aglobal.h"
#include "bsafe_kitype.h"
#include "asn.h"
#include "bsafe.h"
#include "berder.h"


/* KIT_RSAPublicBER, the template of RSAPublicKey (a SEQUENCE of two INTEGERs) and the two
 * algorithm OIDs accepted (rsaEncryption 1.2.840.113549.1.1.1 and 2.5.8.1.1). */

B_KeyInfoType KIT_RSAPublicBER = { KIT_RSAPublicBERAddInfo, KIT_RSAPublicBERMakeInfo };

static AsnTemplateEntry RSAPublicBERTemplate[] = {
	{ 0x30, 0, 0, 0, 0 },
	{ 2, OldBERGetCanonicalInteger, OldBERSetNonNegativeInteger, 1, 0 },
	{ 2, OldBERGetCanonicalInteger, OldBERSetNonNegativeInteger, 2, 0 },
	{ 0x103, 0, 0, 0, 0 }
};
static unsigned char OID_rsaValue[] = {
	0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01
};
static unsigned char OID_rsaAltValue[] = { 0x55, 0x08, 0x01, 0x01 };
static ITEM OID_rsaAlt = { OID_rsaAltValue, 4 };
static ITEM OID_rsa = { OID_rsaValue, 9 };

int KI_RSAPublicBER(void **info)
{
    *info = &KIT_RSAPublicBER;
    return 1;
}

int KIT_RSAPublicBERAddInfo(void *ctx, ITEM *in)
{
	A_RSA_KEY pub;
	ITEM key;
	ITEM alg;
	ITEM params;
	void *values[3];
	int status;

	status = DecodePublicKeyInfo(&alg, &params, &key, in);
	if (status != 0)
		return status;
	if (B_ItemEqual(&alg, &OID_rsa) == 0 &&
	    B_ItemEqual(&alg, &OID_rsaAlt) == 0)
		return BE_WRONG_KEY_INFO;
	T_memset(values, 0, 12);
	values[1] = &pub.modulus;
	values[2] = &pub.exponent;
	status = _A_BSafeError(ASN_Decode(RSAPublicBERTemplate, 0, key.data,
					  key.len, 0, values));
	if (status != 0)
		return status;
	return KIT_RSAPublicAddInfo(ctx, &pub);
}

int KIT_RSAPublicBERMakeInfo(void **out, void *key)
{
	ITEM buf;
	void *values[3];
	A_RSA_KEY *info;
	ITEM *item;
	int status;

	status = B_KeyGetInfo(key, (void **)&info, &KIT_RSAPublic);
	if (status != 0)
		return status;
	status = B_MemoryPoolAlloc(key, out, 8);
	if (status != 0)
		return status;
	buf.data = 0;
	item = (ITEM *)*out;
	do {
		T_memset(values, 0, 12);
		values[1] = &info->modulus;
		values[2] = &info->exponent;
		if ((status = _A_BSafeError(ASN_EncodeAlloc(RSAPublicBERTemplate, 0,
							    values, &buf))) != 0)
			break;
		if ((status = EncodePublicKeyInfoAlloc(item, &OID_rsa,
						       &ASN_NullEncoding, &buf)) != 0)
			break;
		status = B_MemoryPoolAdoptData(key, (void **)&item->data, item->len);
	} while (0);
	T_free(buf.data);
	return status;
}
