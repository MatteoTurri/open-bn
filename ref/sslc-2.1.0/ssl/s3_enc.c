/* ssl/s3_enc.c */
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
#include "evp.h"
#include "ssl_locl.h"

/* SSL-C: the digest context is the md_ctx that SSL_new allocates, not an EVP_MD_CTX on the
 * stack. The digest buffers are EVP_MAX_MD_SIZE, 32 in the fork (evp.h). */








/* SSL-C: the EVP_MD is stateful: ssl3_mac uses hash itself as the digest context. */
#define RCOM_HCTX(h)	((EVP_MD_CTX *)(h))

static unsigned char ssl3_pad_1[48]={
	0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
	0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
	0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
	0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
	0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
	0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36 };

static unsigned char ssl3_pad_2[48]={
	0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,
	0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,
	0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,
	0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,
	0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,
	0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c };

#ifndef NO_PROTO
static int ssl3_handshake_mac(SSL *s, EVP_MD_CTX *in_ctx,
	unsigned char *sender, int len, unsigned char *p);
#else
static int ssl3_handshake_mac();
#endif

/* SSL-C: same derivation as the stock one, with allocated EVP digest contexts. */
static void ssl3_generate_key_block(s,km,num)
SSL *s;
unsigned char *km;
int num;
	{
	EVP_MD_CTX *m5;
	EVP_MD_CTX *s1;
	unsigned char buf[16],smd[EVP_MAX_MD_SIZE];
	unsigned char c='A';
	unsigned int n;
	int i,j,k;
	int md5_len,sha1_len;

	m5=EVP_MD_CTX_new();
	s1=EVP_MD_CTX_new();
	EVP_DigestInit(m5,EVP_md5_digest());
	EVP_DigestInit(s1,EVP_sha1_digest());
	md5_len=EVP_MD_CTX_size(m5);
	sha1_len=EVP_MD_CTX_size(s1);

	k=0;
	for (i=0; i<num; i+=md5_len)
		{
		k++;
		for (j=0; j<k; j++)
			buf[j]=c;
		c++;
		EVP_DigestInit(s1,NULL);
		EVP_DigestUpdate(s1,buf,k);
		EVP_DigestUpdate(s1,s->session->master_key,
			s->session->master_key_length);
		EVP_DigestUpdate(s1,s->s3->server_random,SSL3_RANDOM_SIZE);
		EVP_DigestUpdate(s1,s->s3->client_random,SSL3_RANDOM_SIZE);
		EVP_DigestFinal(s1,smd,&n);

		EVP_DigestInit(m5,NULL);
		EVP_DigestUpdate(m5,s->session->master_key,
			s->session->master_key_length);
		EVP_DigestUpdate(m5,smd,sha1_len);
		if ((i+md5_len) > num)
			{
			EVP_DigestFinal(m5,smd,&n);
			memcpy(km,smd,(num-i));
			}
		else
			EVP_DigestFinal(m5,km,&n);

		km+=md5_len;
		}
	memset(smd,0,sha1_len);
	EVP_MD_CTX_free(m5);
	EVP_MD_CTX_free(s1);
	}

#line 145 /* the line number is part of the binary */
int ssl3_change_cipher_state(s,which)
SSL *s;
int which;
	{
	struct ssl_rw_st *rw;
	unsigned char *p,*key_block,*mac_secret;
	unsigned char exp_key[RCOM_MAX_KEY_LENGTH];
	unsigned char exp_iv[RCOM_MAX_KEY_LENGTH];
	unsigned char *ms,*key,*iv,*er1,*er2;
	EVP_CIPHER_CTX *dd;
	EVP_CIPHER *c;
	EVP_MD *m;
	int exp,n,i,j,k;
	EVP_MD_CTX *md;

	exp=(s->s3->tmp.new_cipher->algorithms & SSL_EXPORT)?1:0;
	c=s->s3->tmp.new_sym_enc;
	m=s->s3->tmp.new_hash;
	key_block=s->s3->tmp.key_block;

	if (which & SSL3_CC_READ)
		{
		rw=(struct ssl_rw_st *)&(s->read_hash_size);
		memset(&(s->s3->read_sequence[0]),0,8);
		mac_secret= &(s->s3->read_mac_secret[0]);
		}
	else
		{
		rw=(struct ssl_rw_st *)&(s->write_hash_size);
		memset(&(s->s3->write_sequence[0]),0,8);
		mac_secret= &(s->s3->write_mac_secret[0]);
		}

	if ((rw->enc_ctx == NULL) &&
		((rw->enc_ctx=EVP_CIPHER_CTX_new()) == NULL))
		goto err;
	if ((rw->hash == NULL) &&
		((rw->hash=(EVP_MD *)EVP_MD_CTX_new()) == NULL))
		goto err;
	s->hash_free=EVP_MD_CTX_free;
	if (!EVP_DigestInit((EVP_MD_CTX *)rw->hash,m))
		goto err;
	rw->mac_size=EVP_MD_size(m);
	dd=rw->enc_ctx;
	if (!EVP_CIPHER_CTX_init(dd))
		goto err;

	p=s->s3->tmp.key_block;
	i=EVP_MD_size(m);
	if (exp)
		j=s->s3->tmp.new_cipher->algorithm2 & 0xff;
	else
		j=EVP_CIPHER_key_length(c);
	k=EVP_CIPHER_iv_length(c);
	if (	(which == SSL3_CHANGE_CIPHER_CLIENT_WRITE) ||
		(which == SSL3_CHANGE_CIPHER_SERVER_READ))
		{
		ms=  &(p[ 0]); n=i+i;
		key= &(p[ n]); n+=j+j;
		iv=  &(p[ n]); n+=k+k;
		er1= &(s->s3->client_random[0]);
		er2= &(s->s3->server_random[0]);
		}
	else
		{
		n=i;
		ms=  &(p[ n]); n+=i+j;
		key= &(p[ n]); n+=j+k;
		iv=  &(p[ n]); n+=k;
		er1= &(s->s3->server_random[0]);
		er2= &(s->s3->client_random[0]);
		}

	if (n > s->s3->tmp.key_block_length)
		{
		SSLerr(SSL_F_SSL3_CHANGE_CIPHER_STATE,157);
		goto err2;
		}

	memcpy(mac_secret,ms,i);
	if (exp)
		{
		/* In here I set both the read and write key/iv to the
		 * same value since only the correct one will be used :-).
		 *
		 * SSL-C: an allocated EVP_MD_CTX; the second EVP_DigestInit passes NULL
		 * and keeps the digest set by the first. */




		md=EVP_MD_CTX_new();
		EVP_DigestInit(md,EVP_md5_digest());
		EVP_DigestUpdate(md,key,j);
		EVP_DigestUpdate(md,er1,SSL3_RANDOM_SIZE);
		EVP_DigestUpdate(md,er2,SSL3_RANDOM_SIZE);
		EVP_DigestFinal(md,&(exp_key[0]),NULL);
		key= &(exp_key[0]);

		if (k > 0)
			{
			EVP_DigestInit(md,NULL);
			EVP_DigestUpdate(md,er1,SSL3_RANDOM_SIZE);
			EVP_DigestUpdate(md,er2,SSL3_RANDOM_SIZE);
			EVP_DigestFinal(md,&(exp_iv[0]),NULL);
			iv= &(exp_iv[0]);
			}
		EVP_MD_CTX_free(md);
		}

	s->session->key_arg_length=0;

	if (!EVP_CipherInit(dd,c,key,iv,(which & SSL3_CC_WRITE)))
		goto err2;

	memset(&(exp_key[0]),0,sizeof(exp_key));
	memset(&(exp_iv[0]),0,sizeof(exp_iv));
	return(1);
err:
	SSLerr(SSL_F_SSL3_CHANGE_CIPHER_STATE,ERR_R_MALLOC_FAILURE);
err2:
	return(0);
	}

int ssl3_setup_key_block(s)
SSL *s;
	{
	unsigned char *p;
	EVP_CIPHER *c;
	EVP_MD *hash;
	int num,exp;

	if (s->s3->tmp.key_block_length != 0)
		return(1);

	if (!ssl_cipher_get_evp(s->session->cipher,&c,&hash))
		{
		SSLerr(158,SSL_R_CIPHER_OR_HASH_UNAVAILABLE);
		return(0);
		}

	s->s3->tmp.new_sym_enc=c;
	s->s3->tmp.new_hash=hash;

	exp=(s->session->cipher->algorithms & SSL_EXPORT)?1:0;

	num=EVP_CIPHER_key_length(c)+EVP_MD_size(hash)+EVP_CIPHER_iv_length(c);
	num*=2;
	num++;













	ssl3_cleanup_key_block(s);

	if ((p=(unsigned char *)Malloc(num)) == NULL)
		goto err;

	s->s3->tmp.key_block_length=num;
	s->s3->tmp.key_block=p;

	ssl3_generate_key_block(s,p,num);

	return(1);
err:
	SSLerr(158,ERR_R_MALLOC_FAILURE);
	return(0);
	}

void ssl3_cleanup_key_block(s)
SSL *s;
	{
	if (s->s3->tmp.key_block != NULL)
		{
		memset(s->s3->tmp.key_block,0,
			s->s3->tmp.key_block_length);
		Free(s->s3->tmp.key_block);
		s->s3->tmp.key_block=NULL;
		}
	s->s3->tmp.key_block_length=0;
	}

int ssl3_enc(s,send)
SSL *s;
int send;
	{
	SSL3_RECORD *rec;
	EVP_CIPHER_CTX *ds;
	unsigned long l;
	int bs,i;
	EVP_CIPHER *enc;

	if (send)
		{
		ds=s->enc_write_ctx;
		rec= &(s->s3->wrec);
		}
	else
		{
		ds=s->enc_read_ctx;
		rec= &(s->s3->rrec);
		}

	if (ds == NULL)
		enc=NULL;
	else
		enc=EVP_CIPHER_CTX_cipher(ds);

	if ((s->session == NULL) || (ds == NULL) || (enc == NULL))
		{
		memcpy(rec->data,rec->input,rec->length);
		rec->input=rec->data;
		}
	else
		{
		l=rec->length;
		bs=EVP_CIPHER_CTX_block_size(ds);

		/* This should be using (bs-1) and bs instead of 7 and 8 */
		if ((bs != 1) && send)
			{
			i=bs-((int)l%bs);

			/* we need to add 'i-1' padding bytes */
			l+=i;
			rec->length+=i;
			rec->input[l-1]=(i-1);
			}

		EVP_Cipher(ds,rec->data,rec->input,l);

		if ((bs != 1) && !send)
			{
			i=rec->data[l-1]+1;
			if (i > bs)
				{


				SSLerr(SSL_F_SSL3_ENC,SSL_R_BLOCK_CIPHER_PAD_IS_WRONG);
				ssl3_send_alert(s,SSL3_AL_FATAL,SSL_AD_DECRYPT_ERROR);
				return(0);
				}
			rec->length-=i;
			}
		}
	return(1);
	}

int ssl3_cert_verify_mac(s,ctx,p)
SSL *s;
EVP_MD_CTX *ctx;
unsigned char *p;
	{
	return(ssl3_handshake_mac(s,ctx,NULL,0,p));
	}

int ssl3_final_finish_mac(s,ctx1,ctx2,sender,len,p)
SSL *s;
EVP_MD_CTX *ctx1,*ctx2;
unsigned char *sender;
int len;
unsigned char *p;
	{
	int ret,sha1_len;

	ret=ssl3_handshake_mac(s,ctx1,sender,len,p);
	if (ret == 0)
		return(0);
	p+=ret;
	sha1_len=ssl3_handshake_mac(s,ctx2,sender,len,p);
	if (sha1_len == 0)
		return(0);
	ret+=sha1_len;
	return(ret);
	}

/* SSL-C: no EVP_MD_CTX on the stack: the context is the SSL's md_ctx, copied with
 * EVP_MD_CTX_copy. */
static int ssl3_handshake_mac(s,in_ctx,sender,len,p)
SSL *s;
EVP_MD_CTX *in_ctx;
unsigned char *sender;
int len;
unsigned char *p;
	{
	unsigned int ret;
	int npad,n;
	unsigned int i;
	unsigned char md_buf[EVP_MAX_MD_SIZE];

	if (!EVP_MD_CTX_copy(s->md_ctx,in_ctx))
		return(0);

	n=EVP_MD_CTX_size(s->md_ctx);
	npad=(48/n)*n;

	if (sender != NULL)
		EVP_DigestUpdate(s->md_ctx,sender,len);
	EVP_DigestUpdate(s->md_ctx,s->session->master_key,
		s->session->master_key_length);
	EVP_DigestUpdate(s->md_ctx,ssl3_pad_1,npad);
	EVP_DigestFinal(s->md_ctx,md_buf,&i);

	EVP_DigestInit(s->md_ctx,EVP_MD_CTX_digest(s->md_ctx));
	EVP_DigestUpdate(s->md_ctx,s->session->master_key,
		s->session->master_key_length);
	EVP_DigestUpdate(s->md_ctx,ssl3_pad_2,npad);
	EVP_DigestUpdate(s->md_ctx,md_buf,i);
	EVP_DigestFinal(s->md_ctx,p,&ret);

	return((int)ret);
	}

int ssl3_mac(ssl,md,send)
SSL *ssl;
unsigned char *md;
int send;
	{
	SSL3_RECORD *rec;
	unsigned char *mac_sec,*seq;
	EVP_MD *hash;
	unsigned char *p,rec_char;
	unsigned int md_size;
	int npad,i;

	if (send)
		{
		rec= &(ssl->s3->wrec);
		mac_sec= &(ssl->s3->write_mac_secret[0]);
		seq= &(ssl->s3->write_sequence[0]);
		hash=ssl->write_hash;
		md_size=ssl->write_hash_size;
		}
	else
		{
		rec= &(ssl->s3->rrec);
		mac_sec= &(ssl->s3->read_mac_secret[0]);
		seq= &(ssl->s3->read_sequence[0]);
		hash=ssl->read_hash;
		md_size=ssl->read_hash_size;
		}

	npad=(48/md_size)*md_size;

	/* Chop the digest off the end :-) */

	EVP_DigestInit(  RCOM_HCTX(hash),NULL);
	EVP_DigestUpdate(RCOM_HCTX(hash),mac_sec,md_size);
	EVP_DigestUpdate(RCOM_HCTX(hash),ssl3_pad_1,npad);
	EVP_DigestUpdate(RCOM_HCTX(hash),seq,8);
	rec_char=rec->type;
	EVP_DigestUpdate(RCOM_HCTX(hash),&rec_char,1);
	p=md;
	s2n(rec->length,p);
	EVP_DigestUpdate(RCOM_HCTX(hash),md,2);
	EVP_DigestUpdate(RCOM_HCTX(hash),rec->input,rec->length);
	EVP_DigestFinal( RCOM_HCTX(hash),md,NULL);

	EVP_DigestInit(  RCOM_HCTX(hash),NULL);
	EVP_DigestUpdate(RCOM_HCTX(hash),mac_sec,md_size);
	EVP_DigestUpdate(RCOM_HCTX(hash),ssl3_pad_2,npad);
	EVP_DigestUpdate(RCOM_HCTX(hash),md,md_size);
	EVP_DigestFinal( RCOM_HCTX(hash),md,&md_size);

	for (i=7; i>=0; i--)
		if (++seq[i]) break; 

	return(md_size);
	}

int ssl3_generate_master_secret(s,out,p,len)
SSL *s;
unsigned char *out;
unsigned char *p;
int len;
	{
	static unsigned char *salt[3]={
		(unsigned char *)"A",
		(unsigned char *)"BB",
		(unsigned char *)"CCC",
		};
	unsigned char buf[EVP_MAX_MD_SIZE];
	int i,ret=0;
	unsigned int n;

	for (i=0; i<3; i++)
		{
		if (!EVP_DigestInit(s->md_ctx,s->ctx->sha1))
			return(0);
		EVP_DigestUpdate(s->md_ctx,salt[i],strlen((char *)salt[i]));
		EVP_DigestUpdate(s->md_ctx,p,len);
		EVP_DigestUpdate(s->md_ctx,&(s->s3->client_random[0]),
			SSL3_RANDOM_SIZE);
		EVP_DigestUpdate(s->md_ctx,&(s->s3->server_random[0]),
			SSL3_RANDOM_SIZE);
		EVP_DigestFinal(s->md_ctx,buf,&n);

		if (!EVP_DigestInit(s->md_ctx,s->ctx->md5))
			return(0);
		EVP_DigestUpdate(s->md_ctx,p,len);
		EVP_DigestUpdate(s->md_ctx,buf,n);
		EVP_DigestFinal(s->md_ctx,out,&n);
		out+=n;
		ret+=n;
		}
	return(ret);
	}

int ssl3_alert_code(code)
int code;
	{
	switch (code)
		{
	case SSL_AD_CLOSE_NOTIFY:	return(SSL3_AD_CLOSE_NOTIFY);
	case SSL_AD_UNEXPECTED_MESSAGE:	return(SSL3_AD_UNEXPECTED_MESSAGE);
	case SSL_AD_BAD_RECORD_MAC:	return(SSL3_AD_BAD_RECORD_MAC);
	case SSL_AD_DECRYPTION_FAILED:	return(SSL3_AD_BAD_RECORD_MAC);
	case SSL_AD_RECORD_OVERFLOW:	return(SSL3_AD_BAD_RECORD_MAC);
	case SSL_AD_DECOMPRESSION_FAILURE:return(SSL3_AD_DECOMPRESSION_FAILURE);
	case SSL_AD_HANDSHAKE_FAILURE:	return(SSL3_AD_HANDSHAKE_FAILURE);
	case SSL_AD_NO_CERTIFICATE:	return(SSL3_AD_NO_CERTIFICATE);
	case SSL_AD_BAD_CERTIFICATE:	return(SSL3_AD_BAD_CERTIFICATE);
	case SSL_AD_UNSUPPORTED_CERTIFICATE:return(SSL3_AD_UNSUPPORTED_CERTIFICATE);
	case SSL_AD_CERTIFICATE_REVOKED:return(SSL3_AD_CERTIFICATE_REVOKED);
	case SSL_AD_CERTIFICATE_EXPIRED:return(SSL3_AD_CERTIFICATE_EXPIRED);
	case SSL_AD_CERTIFICATE_UNKNOWN:return(SSL3_AD_CERTIFICATE_UNKNOWN);
	case SSL_AD_ILLEGAL_PARAMETER:	return(SSL3_AD_ILLEGAL_PARAMETER);
	case SSL_AD_UNKNOWN_CA:		return(SSL3_AD_BAD_CERTIFICATE);
	case SSL_AD_ACCESS_DENIED:	return(SSL3_AD_HANDSHAKE_FAILURE);
	case SSL_AD_DECODE_ERROR:	return(SSL3_AD_HANDSHAKE_FAILURE);
	case SSL_AD_DECRYPT_ERROR:	return(SSL3_AD_HANDSHAKE_FAILURE);
	case SSL_AD_EXPORT_RESTRICION:	return(SSL3_AD_HANDSHAKE_FAILURE);
	case SSL_AD_PROTOCOL_VERSION:	return(SSL3_AD_HANDSHAKE_FAILURE);
	case SSL_AD_INSUFFICIENT_SECURITY:return(SSL3_AD_HANDSHAKE_FAILURE);
	case SSL_AD_INTERNAL_ERROR:	return(SSL3_AD_HANDSHAKE_FAILURE);
	case SSL_AD_USER_CANCLED:	return(SSL3_AD_HANDSHAKE_FAILURE);
	case SSL_AD_NO_RENEGOTIATION:	return(-1); /* Don't send it :-) */
	default:			return(-1);
		}
	}

int ssl3_init_finished_mac(s)
SSL *s;
	{
	if (EVP_DigestInit((s->s3->finish_dgst1),s->ctx->md5) == 0)
		return(0);
	if (EVP_DigestInit((s->s3->finish_dgst2),s->ctx->sha1) == 0)
		return(0);
	return(1);
	}
