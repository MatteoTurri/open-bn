/*
 * The X509_STORE_CTX functions (crypto/x509/x509_st.c of SSL-C).
 */








#include <stdio.h>
#include "cryptlib.h"
#include "lhash.h"
#include "x509.h"

static STACK *x509_store_ctx_method=NULL;
static int x509_store_ctx_num=0;

int X509_STORE_CTX_get_ex_new_index(argl,argp,new_func,dup_func,free_func)
long argl;
char *argp;
int (*new_func)();
int (*dup_func)();
void (*free_func)();
        {
        x509_store_ctx_num++;
        return(CRYPTO_get_ex_new_index(x509_store_ctx_num-1,
		&x509_store_ctx_method,
                argl,argp,new_func,dup_func,free_func));
        }

/* RCOM: an addition of the fork (not in stock SSLeay 0.9.0b). */
int X509_STORE_CTX_cleanup_ex_data()
	{
	if (x509_store_ctx_method != NULL)
		{
		sk_pop_free(x509_store_ctx_method,CRYPTO_EX_DATA_FUNCS_free);
		x509_store_ctx_method=NULL;
		x509_store_ctx_num=0;
		}
	return(1);
	}

int X509_STORE_CTX_set_ex_data(ctx,idx,data)
X509_STORE_CTX *ctx;
int idx;
char *data;
	{
	return(CRYPTO_set_ex_data(&ctx->ex_data,idx,data));
	}

char *X509_STORE_CTX_get_ex_data(ctx,idx)
X509_STORE_CTX *ctx;
int idx;
	{
	return(CRYPTO_get_ex_data(&ctx->ex_data,idx));
	}

/* SSL-C: allocates the X509_STORE_CTX without initializing it (X509_STORE_CTX_init does). */




X509_STORE_CTX *X509_STORE_CTX_new()
	{
	X509_STORE_CTX *ret=NULL;

	ret=(X509_STORE_CTX *)Malloc(sizeof(X509_STORE_CTX));
	if (ret == NULL)
		{
#line 187 /* the line number is part of the binary */
		X509err(234,33);
		}
	return(ret);
	}

void X509_STORE_CTX_init(ctx,store,x509,chain)
X509_STORE_CTX *ctx;
X509_STORE *store;
X509 *x509;
STACK *chain;
	{
	ctx->ctx=store;
	ctx->current_method=0;
	ctx->cert=x509;
	ctx->untrusted=chain;
	ctx->depth=10;
	ctx->valid=0;
	ctx->last_untrusted=0;
	ctx->chain=NULL;
	ctx->error_depth=0;
	ctx->error=0;
	ctx->current_cert=NULL;
	memset(&(ctx->ex_data),0,sizeof(CRYPTO_EX_DATA));
	/* SSL-C: also sets up the ex_data. */
	CRYPTO_new_ex_data(x509_store_ctx_method,(char *)ctx,&(ctx->ex_data));
	}

/* SSL-C: cleans up only when ctx->chain is set, then frees. */
void X509_STORE_CTX_free(ctx)
X509_STORE_CTX *ctx;
	{
	if (ctx->chain != NULL)
		X509_STORE_CTX_cleanup(ctx);
	Free(ctx);
	return;
	}

void X509_STORE_CTX_cleanup(ctx)
X509_STORE_CTX *ctx;
	{
	if (ctx->chain != NULL)
		{
		sk_pop_free(ctx->chain,X509_free);
		ctx->chain=NULL;
		}
	CRYPTO_free_ex_data(x509_store_ctx_method,(char *)ctx,&(ctx->ex_data));
	memset(&ctx->ex_data,0,sizeof(CRYPTO_EX_DATA));
	}

/* SSL-C: accessor of depth. */
void X509_STORE_CTX_set_depth(ctx,depth)
X509_STORE_CTX *ctx;
int depth;
	{
	ctx->depth=depth;
	return;
	}

int X509_STORE_CTX_get_error(ctx)
X509_STORE_CTX *ctx;
	{
	return(ctx->error);
	}

void X509_STORE_CTX_set_error(ctx,err)
X509_STORE_CTX *ctx;
int err;
	{
	ctx->error=err;
	}
