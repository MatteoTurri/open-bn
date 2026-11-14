/*
 * EVP_CIPHER_CTX_iv, from crypto/evp/evp_lib.c.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "evp.h"

unsigned char *EVP_CIPHER_CTX_iv(ctx)
EVP_CIPHER_CTX *ctx;
	{
	return(ctx->iv);
	}
