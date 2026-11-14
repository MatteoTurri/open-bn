/* crypto/evp/evp_enc2.c */
/*
 * EVP_CIPHER_CTX_set_cipher and EVP_EncryptUpdate of SSL-C, not in SSLeay 0.9.0b.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "evp.h"


int EVP_CIPHER_CTX_set_cipher(ctx,cipher)
EVP_CIPHER_CTX *ctx;
EVP_CIPHER *cipher;
	{
	if (ctx->cipher != NULL)
		{
		if (ctx->cipher_data != NULL)
			memset(ctx->cipher_data,0,ctx->cipher_data_size);
		if ((cipher == NULL) || (ctx->cipher_data_size != cipher->ctx_size))
			{
			R_free(ctx->cipher_data);
			ctx->cipher_data=NULL;
			}
		}
	if ((cipher != NULL) && (ctx->cipher_data == NULL) && (cipher->ctx_size > 0))
		{
		if ((ctx->cipher_data=(char *)R_malloc(cipher->ctx_size)) == NULL)
			{
#line 95 /* the line number is part of the binary */
			EVPerr(EVP_F_EVP_CIPHER_CTX_SET_CIPHER,ERR_R_MALLOC_FAILURE);
			return(0);
			}
		}
	if (cipher != NULL)
		{
		ctx->cipher_data_size=cipher->ctx_size;
		if (cipher->ctx_size > 0)
			memset(ctx->cipher_data,0,cipher->ctx_size);
		ctx->cipher=cipher;
		}
	ctx->key_len=0;
	ctx->ex_long=0;
	return(1);
	}
