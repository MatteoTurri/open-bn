/*
 * dsa_sign_cb and dsa_sign_setup_cb: DSA_sign and DSA_sign_setup of SSLeay (crypto/dsa/dsa_sign.c)
 * with BIGNUMs on the stack, the Montgomery context cached and BN_rand with a random object.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "asn1.h"
#include "dsa.h"
#include "r_rand.h"
#line 30  /* the line number is part of the binary */
int dsa_sign_cb(type,dgst,dlen,sig,siglen,dsa)
int type;
unsigned char *dgst;
int dlen;
unsigned char *sig;
unsigned int *siglen;
DSA *dsa;
	{
	int ret=0;
	int i;
	int reason=ERR_R_BN_LIB;
	BN_CTX ctx;
	BIGNUM m;
	BIGNUM xr;
	BIGNUM s;
	BIGNUM *r=NULL;
	BIGNUM *kinv=NULL;
	unsigned char rbuf[52];
	unsigned char sbuf[52];
	ASN1_INTEGER rbs;
	ASN1_INTEGER sbs;
	int len;
	unsigned char *p;

	BN_CTX_init(&ctx);
	BN_init(&m);
	BN_init(&xr);
	BN_init(&s);

	i=BN_num_bytes(dsa->q);
	if ((dlen > i) || (dlen > 52))
		{
		reason=100;
		goto err;
		}

	if ((dsa->kinv == NULL) || (dsa->r == NULL))
		{
		if (!DSA_sign_setup(dsa,&ctx,&kinv,&r)) goto err;
		}
	else
		{
		kinv=dsa->kinv;
		r=dsa->r;
		dsa->kinv=NULL;
		dsa->r=NULL;
		}

	if (BN_bin2bn(dgst,dlen,&m) == NULL) goto err;

	if (!BN_mod_mul(&xr,dsa->priv_key,r,dsa->q,&ctx)) goto err;
	if (!BN_add(&s,&xr,&m)) goto err;
	if (BN_cmp(&s,dsa->q) > 0)
		BN_sub(&s,&s,dsa->q);
	if (!BN_mod_mul(&s,&s,kinv,dsa->q,&ctx)) goto err;

	rbs.data=rbuf;
	rbs.type=V_ASN1_INTEGER;
	rbs.length=BN_bn2bin(r,rbs.data);
	sbs.data=sbuf;
	sbs.type=V_ASN1_INTEGER;
	sbs.length=BN_bn2bin(&s,sbs.data);

	len =i2d_ASN1_INTEGER(&rbs,NULL);
	len+=i2d_ASN1_INTEGER(&sbs,NULL);

	p=sig;
	ASN1_put_object(&p,1,len,V_ASN1_SEQUENCE,0);
	i2d_ASN1_INTEGER(&rbs,&p);
	i2d_ASN1_INTEGER(&sbs,&p);
	*siglen=(p-sig);
	ret=1;
	goto end;
err:
	DSAerr(105,reason);
end:
	BN_CTX_free(&ctx);
	if (kinv != NULL) BN_clear_free(kinv);
	if (r != NULL) BN_clear_free(r);
	BN_clear_free(&m);
	BN_clear_free(&xr);
	BN_clear_free(&s);
	return(ret);
	}













int dsa_sign_setup_cb(dsa,ctx_in,kinvp,rp)
DSA *dsa;
BN_CTX *ctx_in;
BIGNUM **kinvp;
BIGNUM **rp;
	{
	int ret=0;
	BN_CTX *ctx;
	BIGNUM k;
	BIGNUM *kinv=NULL;
	BIGNUM *r=NULL;

	if (ctx_in == NULL)
		{
		if ((ctx=BN_CTX_new()) == NULL) goto err;
		}
	else
		ctx=ctx_in;

	if ((dsa->method_mont_p == NULL) && (dsa->flags & 1))
		{
		if ((dsa->method_mont_p=(char *)BN_MONT_CTX_new()) != NULL)
			if (!BN_MONT_CTX_set_word((BN_MONT_CTX *)dsa->method_mont_p,dsa->p,ctx))
				goto err;
		}

	BN_init(&k);
	kinv=NULL;
	r=BN_new();
	if (r == NULL) goto err;

	for (;;)
		{
		if (!BN_rand(&k,R_rand_get_default(),BN_num_bits(dsa->q),1,0)) goto err;
		if (BN_cmp(&k,dsa->q) >= 0)
			BN_sub(&k,&k,dsa->q);
		if (!BN_is_zero(&k)) break;
		}

	if (!BN_mod_exp_mont(r,dsa->g,&k,dsa->p,ctx,(BN_MONT_CTX *)dsa->method_mont_p)) goto err;
	if (!BN_mod(r,r,dsa->q,ctx)) goto err;

	if ((kinv=BN_mod_inverse(NULL,&k,dsa->q,ctx)) == NULL) goto err;

	if (*rp != NULL) BN_clear_free(*rp);
	if (*kinvp != NULL) BN_clear_free(*kinvp);
	*rp=r;
	*kinvp=kinv;
	r=NULL;
	kinv=NULL;
	ret=1;
	goto end;
err:
	DSAerr(106,2051);
end:
	if (ctx_in == NULL) BN_CTX_free(ctx);
	if (kinv != NULL) BN_clear_free(kinv);
	if (r != NULL) BN_clear_free(r);
	BN_clear_free(&k);
	return(ret);
	}
