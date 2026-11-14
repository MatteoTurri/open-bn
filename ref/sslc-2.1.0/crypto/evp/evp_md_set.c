/*
 * EVP_MD_CTX_set_digest (evp_md.c of SSL-C).
 */

#include <stdio.h>
#include "cryptlib.h"
#include "evp.h"


int EVP_MD_CTX_set_digest(ctx,md)
EVP_MD_CTX *ctx;
EVP_MD *md;
	{
	if (ctx->digest != NULL)
		{
		memset(ctx->md_data,0,ctx->md_data_size);
		if (ctx->md_data_size != md->ctx_size)
			{
			R_free(ctx->md_data);
			ctx->md_data=NULL;
			ctx->md_data_size=0;
			}
		}
	if ((md != NULL) && (ctx->md_data == NULL))
		{
		if ((ctx->md_data=R_malloc(md->ctx_size)) == NULL)
			{
#line 41 /* the line number is part of the binary */
			EVPerr(EVP_F_EVP_MD_CTX_SET_DIGEST,ERR_R_MALLOC_FAILURE);
			return(0);
			}
		}
	ctx->digest=md;
	if (md != NULL)
		ctx->md_data_size=md->ctx_size;
	if (ctx->md_data_size > 0)
		memset(ctx->md_data,0,ctx->md_data_size);
	return(1);
	}
