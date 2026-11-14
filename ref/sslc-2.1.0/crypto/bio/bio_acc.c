/* crypto/bio/bio_acc.c */
/* SSL-C: the BIO accessors, macros of bio.h in SSLeay, are functions here. */

#include <stdio.h>
#include <errno.h>
#include "crypto.h"
#include "cryptlib.h"
#include "bio.h"
#include "stack.h"

void BIO_set_flags(b,f)
BIO *b;
int f;
	{
	b->flags|=(f);
	}

int BIO_get_flags(b)
BIO *b;
	{
	return(b->flags);
	}

void BIO_set_retry_special(b)
BIO *b;
	{
	b->flags|=(BIO_FLAGS_IO_SPECIAL|BIO_FLAGS_SHOULD_RETRY);
	}

void BIO_set_retry_read(b)
BIO *b;
	{
	b->flags|=(BIO_FLAGS_READ|BIO_FLAGS_SHOULD_RETRY);
	}

void BIO_set_retry_write(b)
BIO *b;
	{
	b->flags|=(BIO_FLAGS_WRITE|BIO_FLAGS_SHOULD_RETRY);
	}

void BIO_clear_flags(b,f)
BIO *b;
int f;
	{
	b->flags&= ~(f);
	}

void BIO_clear_retry_flags(b)
BIO *b;
	{
	b->flags&= ~(BIO_FLAGS_RWS|BIO_FLAGS_SHOULD_RETRY);
	}

int BIO_get_retry_flags(b)
BIO *b;
	{
	return(b->flags&(BIO_FLAGS_RWS|BIO_FLAGS_SHOULD_RETRY));
	}

int BIO_should_read(a)
BIO *a;
	{
	return(a->flags & BIO_FLAGS_READ);
	}

int BIO_should_write(a)
BIO *a;
	{
	return(a->flags & BIO_FLAGS_WRITE);
	}

int BIO_should_io_special(a)
BIO *a;
	{
	return(a->flags & BIO_FLAGS_IO_SPECIAL);
	}

int BIO_retry_type(a)
BIO *a;
	{
	return(a->flags & BIO_FLAGS_RWS);
	}

int BIO_should_retry(a)
BIO *a;
	{
	return(a->flags & BIO_FLAGS_SHOULD_RETRY);
	}

void BIO_set_cb(b,cb)
BIO *b;
long (*cb)();
	{
	b->callback=cb;
	}

void BIO_set_cb_arg(b,arg)
BIO *b;
char *arg;
	{
	b->cb_arg=arg;
	}

char *BIO_get_cb_arg(b)
BIO *b;
	{
	return(b->cb_arg);
	}

long (*BIO_get_cb(b))()
BIO *b;
	{
	return(b->callback);
	}

void BIO_set_bio_cb(b,cb,arg)
BIO *b;
long (*cb)();
char *arg;
	{
	b->callback=cb;
	b->cb_arg=arg;
	}

char *BIO_method_name(b)
BIO *b;
	{
	return(b->method->name);
	}

int BIO_method_type(b)
BIO *b;
	{
	return(b->method->type);
	}
