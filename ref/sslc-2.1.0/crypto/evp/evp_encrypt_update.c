/* crypto/evp/evp_enc2.c */
/*
 * EVP_CIPHER_CTX_set_cipher and EVP_EncryptUpdate of SSL-C, not in SSLeay 0.9.0b.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "evp.h"


void EVP_EncryptUpdate(ctx,out,outl,in,inl)
EVP_CIPHER_CTX *ctx;
unsigned char *out;
int *outl;
unsigned char *in;
int inl;
	{
	int bl,i,b;

	bl=ctx->buf_len;
	b=ctx->cipher->block_size;
	*outl=0;
	if (inl == 0)
		{
		if (bl != b)
			return;
		}
	if (bl != 0)
		{
		if ((bl+inl) < b)
			{
			memcpy(&(ctx->buf[bl]),in,inl);
			ctx->buf_len+=inl;
			return;
			}
		else
			{
			i=b-bl;
			if (i != 0)
				memcpy(&(ctx->buf[bl]),in,i);
			ctx->cipher->do_cipher(ctx,out,ctx->buf,b);
			inl-=i;
			in+=i;
			out+=b;
			*outl+=b;
			}
		}
	bl=inl%b;
	inl-=bl;
	if (inl > 0)
		{
		ctx->cipher->do_cipher(ctx,out,in,inl);
		*outl+=inl;
		}
	if (bl != 0)
		memcpy(ctx->buf,&(in[inl]),bl);
	ctx->buf_len=bl;
	}
