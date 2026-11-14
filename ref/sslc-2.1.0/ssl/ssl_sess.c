/* ssl/ssl_sess.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <stdio.h>
#include "lhash.h"
#include "rand.h"
#include "ssl_locl.h"
#include "r_rand.h"

#ifndef NOPROTO
static void SSL_SESSION_list_remove(SSL_CTX *ctx, SSL_SESSION *s);
static void SSL_SESSION_list_add(SSL_CTX *ctx,SSL_SESSION *s);
static int ssl_session_remove_locked(SSL_CTX *ctx, SSL_SESSION *c);
#else
static void SSL_SESSION_list_remove();
static void SSL_SESSION_list_add();
static int ssl_session_remove_locked();
#endif

static ssl_session_num=0;
static STACK *ssl_session_meth=NULL;

SSL_SESSION *SSL_get_session(ssl)
SSL *ssl;
	{
	return(ssl->session);
	}

int SSL_SESSION_get_ex_new_index(argl,argp,new_func,dup_func,free_func)
long argl;
char *argp;
int (*new_func)();
int (*dup_func)();
void (*free_func)();
        {
        ssl_session_num++;
        return(CRYPTO_get_ex_new_index(ssl_session_num-1,
		&ssl_session_meth,
                argl,argp,new_func,dup_func,free_func));
        }

int SSL_SESSION_set_ex_data(s,idx,arg)
SSL_SESSION *s;
int idx;
char *arg;
	{
	return(CRYPTO_set_ex_data(&s->ex_data,idx,arg));
	}

char *SSL_SESSION_get_ex_data(s,idx)
SSL_SESSION *s;
int idx;
	{
	return(CRYPTO_get_ex_data(&s->ex_data,idx));
	}

SSL_SESSION *SSL_SESSION_new()
	{
	SSL_SESSION *ss;

	ss=(SSL_SESSION *)R_malloc(sizeof(SSL_SESSION));
	if (ss == NULL)
		{
#line 71 /* the line number is part of the binary */
		SSLerr(SSL_F_SSL_SESSION_NEW,ERR_R_MALLOC_FAILURE);
		return(0);
		}
	memset(ss,0,sizeof(SSL_SESSION));

	ss->references=1;
	ss->timeout=60*5+4; /* 5 minute timeout by default */
	ss->rtime=R_time_new();
	ss->prev=NULL;
	ss->next=NULL;
	CRYPTO_new_ex_data(ssl_session_meth,(char *)ss,&ss->ex_data);
	return(ss);
	}

int ssl_get_new_session(s, session)
SSL *s;
int session;
	{
	SSL_SESSION *ss=NULL;

	if ((ss=SSL_SESSION_new()) == NULL) return(0);

	/* If the context has a default timeout, use it */
	if (s->ctx->session_timeout == 0)
		ss->timeout=SSL_get_default_timeout(s);
	else
		ss->timeout=s->ctx->session_timeout;

	if (s->session != NULL)
		{
		SSL_SESSION_free(s->session);
		s->session=NULL;
		}

	if (session)
		{
		if (s->version == SSL2_CLIENT_VERSION)
			{
			ss->ssl_version=SSL2_VERSION;
			ss->session_id_length=SSL2_SSL_SESSION_ID_LENGTH;
			}
		else if (s->version == SSL3_VERSION)
			{
			ss->ssl_version=SSL3_VERSION;
			ss->session_id_length=SSL3_SSL_SESSION_ID_LENGTH;
			}
		else if (s->version == TLS1_VERSION)
			{
			ss->ssl_version=TLS1_VERSION;
			ss->session_id_length=SSL3_SSL_SESSION_ID_LENGTH;
			}
		else
			{
			SSLerr(181,259);
			SSL_SESSION_free(ss);
			return(0);
			}

		for (;;)
			{
			SSL_SESSION *r;

			R_rand_bytes(R_rand_get_default(),ss->session_id,
				ss->session_id_length);
			CRYPTO_r_lock(CRYPTO_LOCK_SSL_CTX);
			r=(SSL_SESSION *)lh_retrieve(s->ctx->sessions,
				(char *)ss);
			CRYPTO_r_unlock(CRYPTO_LOCK_SSL_CTX);
			if (r == NULL) break;
			/* else - woops a session_id match */
			}
		}
	else
		{
		ss->session_id_length=0;
		}

	memcpy(ss->sid_ctx,s->sid_ctx,s->sid_ctx_length);
	ss->sid_ctx_length=s->sid_ctx_length;
	s->session=ss;
	ss->ssl_version=s->version;

	return(1);
	}

#line 157 /* the line number is part of the binary */
int ssl_get_prev_session(s,session_id,len)
SSL *s;
unsigned char *session_id;
int len;
	{
	SSL_SESSION *ret=NULL,data;
	int rc;

	/* conn_init();*/
	data.ssl_version=s->version;
	data.session_id_length=len;
	if (len > SSL_MAX_SSL_SESSION_ID_LENGTH)
		return(0);
	memcpy(data.session_id,session_id,len);

	if (!(s->ctx->session_cache_mode & SSL_SESS_CACHE_NO_INTERNAL_LOOKUP))
		{
		CRYPTO_r_lock(CRYPTO_LOCK_SSL_CTX);
		ret=(SSL_SESSION *)lh_retrieve(s->ctx->sessions,(char *)&data);
		CRYPTO_r_unlock(CRYPTO_LOCK_SSL_CTX);
		}

	if (ret == NULL)
		{
		int copy=1;

		s->ctx->sess_miss++;
		ret=NULL;
		if ((s->ctx->get_session_cb != NULL) &&
			((ret=s->ctx->get_session_cb(s,session_id,len,&copy))
				!= NULL))
			{
			s->ctx->sess_cb_hit++;

			/* The following should not return 1, otherwise,
			 * things are very strange */
			SSL_CTX_add_session(s->ctx,ret);
			/* auto free it */
			if (!copy)
				SSL_SESSION_free(ret);
			}
		if (ret == NULL) return(0);
		}

	if (s->sid_ctx_length != 0)
		{
		if (s->sid_ctx_length != ret->sid_ctx_length) goto err;
		if (memcmp(s->sid_ctx,ret->sid_ctx,s->sid_ctx_length) != 0) goto err;
		}

	if (ret->cipher == NULL)
		{
		char buf[5],*p;
		unsigned long l;

		p=buf;
		l=ret->cipher_id;
		l2n(l,p);
		if ((ret->ssl_version>>8) == SSL3_VERSION_MAJOR)
			ret->cipher=ssl_get_cipher_by_char(s,&(buf[2]));
		else 
			ret->cipher=ssl_get_cipher_by_char(s,&(buf[1]));
		if (ret->cipher == NULL)
			return(0);
		}























	CRYPTO_add(&ret->references,1,CRYPTO_LOCK_SSL_SESSION);

		{
		void *now,*exp;

		now=R_time_new();
		exp=R_time_new();
		R_time_offset(exp,ret->rtime,ret->timeout,0);
		rc=R_time_cmp(exp,R_time(now));
		R_time_free(exp);
		R_time_free(now);
		if (rc == -1)
			{
			s->ctx->sess_timeout++;
			/* remove it from the cache */
			SSL_CTX_remove_session(s->ctx,ret);
			SSL_SESSION_free(ret);	/* again to actually Free it */
			return(0);
			}
		}

	s->ctx->sess_hit++;

	/* again, just leave the session 
	 * if it is the same session, we have just incremented and
	 * then decremented the reference count :-) */
	if (s->session != NULL)
		SSL_SESSION_free(s->session);
	s->session=ret;
	return(1);
err:
	return(0);
	}


int SSL_CTX_add_session(ctx,c)
SSL_CTX *ctx;
SSL_SESSION *c;
	{
	int ret=0;
	SSL_SESSION *s;

	/* conn_init(); */
	R_locked_add(&c->references,1,CRYPTO_LOCK_SSL_SESSION,ERR_file_name,281);

#line 283 /* the line number is part of the binary */
	CRYPTO_w_lock(CRYPTO_LOCK_SSL_CTX);
	s=(SSL_SESSION *)lh_insert(ctx->sessions,(char *)c);

	/* Put on the end of the queue unless it is already in the cache */
	if (s == NULL)
		SSL_SESSION_list_add(ctx,c);

	/* If the same session if is being 're-added', Free the old
	 * one when the last person stops using it.
	 * This will also work if it is alread in the cache.
	 * The references will go up and then down :-) */
	if (s != NULL)
		{
		SSL_SESSION_free(s);
		ret=0;
		}
	else
		{
		ret=1;

		if (SSL_CTX_ctrl(ctx,43,0,NULL) > 0)
			{
			while (SSL_CTX_ctrl(ctx,20,0,NULL) >
				SSL_CTX_ctrl(ctx,43,0,NULL))
				{
				if (ctx->session_cache_tail != NULL)
					{
					if (!ssl_session_remove_locked(ctx,
						ctx->session_cache_tail))
						break;
					else
						ctx->sess_cache_full++;
					}
				}
			}
		}
	CRYPTO_w_unlock(CRYPTO_LOCK_SSL_CTX);
	return(ret);
	}

int SSL_CTX_remove_session(ctx,c)
SSL_CTX *ctx;
SSL_SESSION *c;
	{
	int ret=0;

	if ((c != NULL) && (c->session_id_length != 0))
		{
		CRYPTO_w_lock(CRYPTO_LOCK_SSL_CTX);
		ret=ssl_session_remove_locked(ctx,c);
		CRYPTO_w_unlock(CRYPTO_LOCK_SSL_CTX);
		}
	else
		ret=0;
	return(ret);
	}

static int ssl_session_remove_locked(ctx,c)
SSL_CTX *ctx;
SSL_SESSION *c;
	{
	SSL_SESSION *r;
	int ret=0;

	r=(SSL_SESSION *)lh_delete(ctx->sessions,(char *)c);
	if (r != NULL)
		{
		ret=1;
		SSL_SESSION_list_remove(ctx,c);
		}
	if (ret)
		{
		r->not_resumable=1;
		if (ctx->remove_session_cb != NULL)
			ctx->remove_session_cb(ctx,r);
		SSL_SESSION_free(r);
		}
	return(ret);
	}

void SSL_SESSION_free(ss)
SSL_SESSION *ss;
	{
	int i;

#line 370 /* the line number is part of the binary */
	i=CRYPTO_add(&ss->references,-1,CRYPTO_LOCK_SSL_SESSION);
#ifdef REF_PRINT
	REF_PRINT("SSL_SESSION",ss);
#endif
	if (i > 0) return;
#ifdef REF_CHECK
	if (i < 0)
		{
		fprintf(stderr,"SSL_SESSION_free, bad reference count\n");
		abort(); /* ok */
		}
#endif

	CRYPTO_free_ex_data(ssl_session_meth,(char *)ss,&ss->ex_data);

	memset(ss->key_arg,0,SSL_MAX_KEY_ARG_LENGTH);
	memset(ss->master_key,0,SSL_MAX_MASTER_KEY_LENGTH);
	memset(ss->session_id,0,SSL_MAX_SSL_SESSION_ID_LENGTH);
	if (ss->rtime != NULL) R_time_free(ss->rtime);
	if (ss->cert != NULL) ssl_cert_free(ss->cert);
	if (ss->peer != NULL) SSLCERT_free(ss->peer);
	if (ss->ciphers != NULL) sk_free(ss->ciphers);
	memset(ss,0,sizeof(*ss));
	Free(ss);
	}


void SSL_SESSION_reference_inc(s)
SSL_SESSION *s;
	{
	R_locked_add(&s->references,1,CRYPTO_LOCK_SSL_SESSION,ERR_file_name,399);
	}

int SSL_set_session(s, session)
SSL *s;
SSL_SESSION *session;
	{
	int ret=0;
	SSL_METHOD *meth;

	if (session != NULL)
		{
		meth=s->ctx->method->get_ssl_method(session->ssl_version);
		if (meth == NULL)
			meth=s->method->get_ssl_method(session->ssl_version);
		if (meth == NULL)
			{
#line 416 /* the line number is part of the binary */
			SSLerr(196,240);
			goto err;
			}

		if (meth != s->method)
			{
			if (!SSL_set_ssl_method(s,meth))
				goto err;
			if (!s->ctx->session_timeout)
				session->timeout=SSL_get_default_timeout(s);
			else
				session->timeout=s->ctx->session_timeout;
			}

		CRYPTO_add(&session->references,1,CRYPTO_LOCK_SSL_SESSION);
		if (s->session != NULL)
			SSL_SESSION_free(s->session);
		s->session=session;
		ret=1;
		}
	else
		{
		if (s->session != NULL)
			{
			SSL_SESSION_free(s->session);
			s->session=NULL;
			}
		meth=s->ctx->method;
		if (meth != s->method)
			{
			if (!SSL_set_ssl_method(s,meth))
				goto err;
			}
		ret=1;
		}
err:
	return(ret);
	}

long SSL_SESSION_set_timeout(s,t)
SSL_SESSION *s;
long t;
	{
	if (s == NULL) return(0);
	s->timeout=t;
	return(1);
	}

long SSL_SESSION_get_timeout(s)
SSL_SESSION *s;
	{
	if (s == NULL) return(0);
	return(s->timeout);
	}

void *SSL_SESSION_get_time(s,t)
SSL_SESSION *s;
void *t;
	{
	if ((s == NULL) || (t == NULL)) return(0);
	memcpy(t,s->rtime,R_time_size());
	return(t);
	}

void *SSL_SESSION_set_time(s,t)
SSL_SESSION *s;
void *t;
	{
	if ((s == NULL) || (t == NULL)) return(0);
	memcpy(s->rtime,t,R_time_size());
	return(t);
	}

#undef SSL_CTX_get_timeout
#undef SSL_CTX_set_timeout

long SSL_CTX_set_timeout(ctx,t)
SSL_CTX *ctx;
long t;
	{
	long l;

	if (ctx == NULL) return(0);
	l=ctx->session_timeout;
	ctx->session_timeout=t;
	return(l);
	}

long SSL_CTX_get_timeout(ctx)
SSL_CTX *ctx;
	{
	if (ctx == NULL) return(0);
	return(ctx->session_timeout);
	}

typedef struct timeout_param_st
	{
	SSL_CTX *ctx;
	void *rtime;
	LHASH *cache;
	} TIMEOUT_PARAM;

static void timeout(s,p)
SSL_SESSION *s;
TIMEOUT_PARAM *p;
	{
	int rc;
	void *exp;

	exp=R_time_new();
	R_time_offset(exp,s->rtime,s->timeout,0);
	rc=R_time_cmp(exp,p->rtime);
	R_time_free(exp);
	if (rc <= 0) /* timeout */
		{
		/* The reason we don't call SSL_CTX_remove_session() is to
		 * save on locking overhead */
		lh_delete(p->cache,(char *)s);
		SSL_SESSION_list_remove(p->ctx,s);
		s->not_resumable=1;
		if (p->ctx->remove_session_cb != NULL)
			p->ctx->remove_session_cb(p->ctx,s);
		SSL_SESSION_free(s);
		}
	}

void SSL_CTX_flush_sessions(s,t)
SSL_CTX *s;
long t;
	{
	unsigned long i;
	TIMEOUT_PARAM tp;

	tp.ctx=s;
	tp.rtime=R_time_new();
	tp.cache=SSL_CTX_sessions(s);
	if (tp.cache == NULL) return;
	if (t == 0)
		R_time_import("99991231235959.9Z",17,2,tp.rtime);
	else
		R_time(tp.rtime);
#line 565 /* the line number is part of the binary */
	CRYPTO_w_lock(CRYPTO_LOCK_SSL_CTX);
	i=tp.cache->down_load;
	tp.cache->down_load=0;
	lh_doall_arg(tp.cache,(void (*)())timeout,(char *)&tp);
	tp.cache->down_load=i;
	CRYPTO_w_unlock(CRYPTO_LOCK_SSL_CTX);
	R_time_free(tp.rtime);
	}

int ssl_clear_bad_session(s)
SSL *s;
	{
	if (	(s->session != NULL) &&
		!(s->shutdown & SSL_SENT_SHUTDOWN) &&
		(SSL_in_init(s) != SSL_state(s)) &&
		!SSL_in_before(s) &&
		!SSL_is_init_finished(s))
		{
		s->session->not_resumable=1;
		SSL_CTX_remove_session(s->ctx,s->session);
		return(1);
		}
	else
		return(0);
	}

/* locked by SSL_CTX in the calling function */
static void SSL_SESSION_list_remove(ctx,s)
SSL_CTX *ctx;
SSL_SESSION *s;
	{
	if ((s->next == NULL) || (s->prev == NULL)) return;

	if (s->next == (SSL_SESSION *)&(ctx->session_cache_tail))
		{ /* last element in list */
		if (s->prev == (SSL_SESSION *)&(ctx->session_cache_head))
			{ /* only one element in list */
			ctx->session_cache_head=NULL;
			ctx->session_cache_tail=NULL;
			}
		else
			{
			ctx->session_cache_tail=s->prev;
			s->prev->next=(SSL_SESSION *)&(ctx->session_cache_tail);
			}
		}
	else
		{
		if (s->prev == (SSL_SESSION *)&(ctx->session_cache_head))
			{ /* first element in list */
			ctx->session_cache_head=s->next;
			s->next->prev=(SSL_SESSION *)&(ctx->session_cache_head);
			}
		else
			{ /* middle of list */
			s->next->prev=s->prev;
			s->prev->next=s->next;
			}
		}
	s->prev=s->next=NULL;
	}

static void SSL_SESSION_list_add(ctx,s)
SSL_CTX *ctx;
SSL_SESSION *s;
	{
	if ((s->next != NULL) && (s->prev != NULL))
		SSL_SESSION_list_remove(ctx,s);

	if (ctx->session_cache_head == NULL)
		{
		ctx->session_cache_head=s;
		ctx->session_cache_tail=s;
		s->prev=(SSL_SESSION *)&(ctx->session_cache_head);
		s->next=(SSL_SESSION *)&(ctx->session_cache_tail);
		}
	else
		{
		s->next=ctx->session_cache_head;
		s->next->prev=s;
		s->prev=(SSL_SESSION *)&(ctx->session_cache_head);
		ctx->session_cache_head=s;
		}
	}

X509 *SSL_SESSION_get_peer_certificate(s)
SSL_SESSION *s;
	{
	return(s->peer);
	}

int SSL_SESSION_get_master_key_length(s)
SSL_SESSION *s;
	{
	return(s->master_key_length);
	}

unsigned char *SSL_SESSION_get_master_key(s)
SSL_SESSION *s;
	{
	return(s->master_key);
	}

unsigned int SSL_SESSION_get_session_id_length(s)
SSL_SESSION *s;
	{
	return(s->session_id_length);
	}

unsigned char *SSL_SESSION_get_session_id(s)
SSL_SESSION *s;
	{
	return(s->session_id);
	}
