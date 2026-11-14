/*
 * The RSA context of the algae layer (ALG_RSA*): the key, and a buffer that collects the input in
 * blocks of the size of the modulus, each one transformed with the modular exponentiation.
 */

#include "cmp.h"
#include "bsafe_kitype.h"
#include "bsafe.h"


#include "alg.h"


/* Converts the modulus and the exponent of the key into ctx (an exponent of 1 is error 17) and
 * allocates the block buffer. Errors go through ALG_ErrorCode. */
static int alg_rsa_block(ALG_RSAContext *ctx, unsigned char *out,
			 unsigned int *produced, int outmax,
			 const unsigned char *in, void *surrender);

int ALG_RSAInit(ALG_RSAContext *ctx, A_RSA_KEY *key)
{
	CMPInt one;
	int status;

	CMP_Constructor(&ctx->modulus);
	CMP_Constructor(&ctx->exponent);
	CMP_Constructor(&one);
	ctx->buf = 0;

	do {
		if ((status = CMP_OctetStringToCMPInt
		     (key->modulus.data, key->modulus.len, &ctx->modulus)) != 0)
			break;
		if ((status = CMP_OctetStringToCMPInt
		     (key->exponent.data, key->exponent.len, &ctx->exponent)) != 0)
			break;
		if ((status = CMP_CMPWordToCMPInt(1, &one)) != 0)
			break;
		if (CMP_Compare(&ctx->exponent, &one) == 0) {
			status = 17;
			break;
		}
		ctx->bufSize = (CMP_BitLengthOfCMPInt(&ctx->modulus) + 7) / 8;
		if ((ctx->buf = T_malloc(ctx->bufSize)) == 0) {
			status = 16;
			break;
		}
		ctx->used = 0;
	} while (0);

	CMP_Destructor(&one);
	if (status == 0)
		return 0;
	return ALG_ErrorCode(status);
}


/* Collects the input in blocks: an input that does not fill the block is only queued; otherwise
 * the queued block is completed and transformed, then the whole blocks of the input, and the rest
 * is queued. surrender is passed to the modular exponentiation. */
int ALG_RSAUpdate(ALG_RSAContext *ctx, unsigned char *out,
		  unsigned int *outlen, int outmax, const unsigned char *in,
		  unsigned int inlen, void *surrender)
{
	unsigned int avail;
	unsigned int produced;
	int r;

	*outlen = 0;

	if (ctx->bufSize > ctx->used + inlen) {
		T_memcpy((unsigned char *)ctx->buf + ctx->used, in, inlen);
		ctx->used = ctx->used + inlen;
		return 0;
	}

	if (ctx->used != 0) {
		avail = ctx->bufSize - ctx->used;
		T_memcpy((unsigned char *)ctx->buf + ctx->used, in, avail);
		in += avail;
		r = alg_rsa_block(ctx, out, &produced, outmax,
				  (unsigned char *)ctx->buf, surrender);
		inlen -= avail;
		if (r != 0)
			return ALG_ErrorCode(r);
		out += produced;
		outmax -= produced;
		*outlen = *outlen + produced;
	}

	while (inlen >= ctx->bufSize) {
		r = alg_rsa_block(ctx, out, &produced, outmax, in, surrender);
		if (r != 0)
			return ALG_ErrorCode(r);
		in += ctx->bufSize;
		inlen -= ctx->bufSize;
		*outlen = *outlen + produced;
		out += produced;
		outmax -= produced;
	}

	ctx->used = inlen;
	T_memcpy(ctx->buf, in, inlen);
	return 0;
}

int ALG_RSAFinal(ALG_RSAContext *ctx)
{
	if (ctx->used != 0)
		return 6;
	ctx->used = 0;
	return 0;
}

/* Transforms one block: the bufSize input bytes become a CMPInt, go through modOps[0] with the
 * exponent and the modulus, and come back as an octet string of the size of the modulus. 10 when
 * the output buffer is too short. */
static int alg_rsa_block(ALG_RSAContext *ctx, unsigned char *out,
			 unsigned int *produced, int outmax,
			 const unsigned char *in, void *surrender)
{
	CMPInt src;
	CMPInt dst;
	int status;
	int bits;

	CMP_Constructor(&dst);
	CMP_Constructor(&src);

	do {
		*produced = ctx->bufSize;
		if (outmax < *produced) {
			status = 10;
			break;
		}
		if ((status = CMP_OctetStringToCMPInt
		     (in, ctx->bufSize, &src)) != 0)
			break;
		if ((status = ctx->modOps[0](&src, &ctx->exponent, &ctx->modulus, &dst, surrender)) != 0)
			break;
		bits = CMP_BitLengthOfCMPInt(&ctx->modulus);
		status = CMP_CMPIntToFixedLenOctetStr(&dst, (bits + 7) / 8, outmax,
						      produced, out);
	} while (0);

	CMP_Destructor(&src);
	CMP_Destructor(&dst);
	if (status == 0)
		return 0;
	return ALG_ErrorCode(status);
}

void ALG_RSAContextDestroy(ALG_RSAContext *ctx)
{
	CMP_Destructor(&ctx->modulus);
	CMP_Destructor(&ctx->exponent);
	if (ctx->buf != 0) {
		T_memset(ctx->buf, 0, ctx->bufSize);
		T_free(ctx->buf);
	}
}
