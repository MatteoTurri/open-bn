/*
 * EVP_MD_CTX_init (evp_md.c of SSL-C): sets up the md_data buffer of an EVP_MD_CTX.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "evp.h"


int EVP_MD_CTX_init(ctx)
EVP_MD_CTX *ctx;
	{
	ctx->flags=0;
	if (ctx->digest != NULL)
		{
		if (ctx->md_data_size != ctx->digest->ctx_size)
			{
			if (ctx->md_data != NULL)
				{
				memset(ctx->md_data,0,ctx->md_data_size);
				R_free(ctx->md_data);
				}
			ctx->md_data=(char *)R_malloc(ctx->digest->ctx_size);
			if (ctx->md_data == NULL)
				{
#line 71 /* the line number is part of the binary */
				EVPerr(EVP_F_EVP_MD_CTX_INIT,ERR_R_MALLOC_FAILURE);
				return(0);
				}
			ctx->md_data_size=ctx->digest->ctx_size;
			}
		}
	if (ctx->md_data != NULL)
		memset(ctx->md_data,0,ctx->md_data_size);
	return(1);
	}
