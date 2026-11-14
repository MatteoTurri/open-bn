/*
 * dsa_verify_cb: DSA_verify of SSLeay (crypto/dsa/dsa_vrf.c) with BIGNUMs on the stack, the two
 * exponentiations merged into BN_mod_exp2_mont with the cached Montgomery context, the 4-argument
 * BN_mod_inverse and the M_ASN1_D2I_ macros written out, with c.line.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "asn1.h"
#include "asn1_mac.h"
#include "dsa.h"
#line 27  /* the line number is part of the binary */
int dsa_verify_cb(type,dgst,dgst_len,sigbuf,siglen,dsa)
int type;
unsigned char *dgst;
int dgst_len;
unsigned char *sigbuf;
int siglen;
DSA *dsa;
	{
	ASN1_CTX c;
	ASN1_INTEGER *bs=NULL;
	unsigned char **pp= &sigbuf;
	long length=siglen;
	int ret=0;
	BN_MONT_CTX *mont;
	BN_CTX *ctx;
	BIGNUM u1;
	BIGNUM u2;
	BIGNUM r;
	BIGNUM t1;

	ctx=BN_CTX_new();
	if (ctx == NULL) goto err;

	BN_init(&u1);
	BN_init(&u2);
	BN_init(&r);
	BN_init(&t1);

	c.p= *pp;
	c.max=(length == 0)?0:(c.p+length);
	if (!asn1_GetSequence(&c,&length)) { c.line=__LINE__; goto err; }
	c.q=c.p; if (d2i_ASN1_INTEGER(&bs,&c.p,c.slen) == NULL) { c.line=__LINE__; goto err; } c.slen-=(c.p-c.q);
	if (BN_bin2bn(bs->data,bs->length,&r) == NULL) goto err_bn;
	/* the second INTEGER of the SEQUENCE */
	c.q=c.p; if (d2i_ASN1_INTEGER(&bs,&c.p,c.slen) == NULL) { c.line=__LINE__; goto err; } c.slen-=(c.p-c.q);
	if (BN_bin2bn(bs->data,bs->length,&u1) == NULL) goto err_bn;
	if (!asn1_Finish(&c)) goto err;

	if ((dsa->method_mont_p == NULL) && (dsa->flags & 1))
		{
		if ((dsa->method_mont_p=(char *)BN_MONT_CTX_new()) != NULL)
			if (!BN_MONT_CTX_set_word((BN_MONT_CTX *)dsa->method_mont_p,dsa->p,ctx))
				goto err;
		}
	mont=(BN_MONT_CTX *)dsa->method_mont_p;

	/* Calculate W = inv(S) mod Q
	 * save W in u2 */
	if (BN_mod_inverse(&u2,&u1,dsa->q,ctx) == NULL) goto err_bn;

	/* save M in u1 */
	if (BN_bin2bn(dgst,dgst_len,&u1) == NULL) goto err_bn;

	/* u1 = M * w mod q */
	if (!BN_mod_mul(&u1,&u1,&u2,dsa->q,ctx)) goto err_bn;

	/* u2 = r * w mod q */
	if (!BN_mod_mul(&u2,&r,&u2,dsa->q,ctx)) goto err_bn;

	/* v = ( g^u1 * y^u2 mod p ) mod q, in a single BN_mod_exp2_mont */
	if (!BN_mod_exp2_mont(&t1,dsa->g,&u1,dsa->pub_key,&u2,dsa->p,ctx,mont))
		goto err_bn;
	/* let u1 = u1 mod q */
	if (!BN_mod(&u1,&t1,dsa->q,ctx)) goto err_bn;
	/* V is now in u1.  If the signature is correct, it will be
	 * equal to R. */

	ret=(BN_ucmp(&u1,&r) == 0);
	if (0)
		{
err: /* ASN1 error */
		DSAerr(108,c.error);
		}
	if (0)
		{
err_bn: /* BN error */
		DSAerr(108,2051);
		}
	if (bs != NULL) ASN1_STRING_free(bs);
	BN_free(&r);
	BN_free(&u1);
	BN_free(&u2);
	BN_free(&t1);
	if (ctx != NULL) BN_CTX_free(ctx);
	return(ret);
	}
