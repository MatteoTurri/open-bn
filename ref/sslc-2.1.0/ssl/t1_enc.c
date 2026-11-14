/* ssl/t1_enc.c */
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
#include "hmac.h"
#include "ssl_locl.h"



/* SSL-C: the two HMAC contexts are allocated (HMAC_new/HMAC_free) */
static int tls1_P_hash(md,sec,sec_len,seed,seed_len,out,olen)
EVP_MD *md;
unsigned char *sec;
int sec_len;
unsigned char *seed;
int seed_len;
unsigned char *out;
int olen;
	{
	int chunk,n;
	int ret=0;
	unsigned int j;
	HMAC_CTX *ctx=NULL;
	HMAC_CTX *ctx_tmp=NULL;
	unsigned char A1[HMAC_MAX_MD_CBLOCK];
	unsigned int A1_len;
	
	chunk=EVP_MD_size(md);

	ctx=HMAC_new();
	ctx_tmp=HMAC_new();
	if ((ctx == NULL) || (ctx_tmp == NULL)) goto err;

	if (!HMAC_Init(ctx,sec,sec_len,md)) goto err;
	HMAC_Update(ctx,seed,seed_len);
	if (!HMAC_Final(ctx,A1,&A1_len)) goto err;

	n=0;
	for (;;)
		{
		if (!HMAC_Init(ctx,NULL,0,NULL)) goto err; /* re-init */
		HMAC_Update(ctx,A1,A1_len);
		if (!HMAC_copy(ctx_tmp,ctx)) goto err; /* Copy for A2 */
		HMAC_Update(ctx,seed,seed_len);

		if (olen > chunk)
			{
			if (!HMAC_Final(ctx,out,&j)) goto err;
			out+=j;
			olen-=j;
			if (!HMAC_Final(ctx_tmp,A1,&A1_len)) goto err; /* calc the next A1 value */
			}
		else	/* last one */
			{
			if (!HMAC_Final(ctx,A1,&A1_len)) goto err;
			memcpy(out,A1,olen);
			break;
			}
		}
	ret=1;
err:
	if (ctx != NULL) HMAC_free(ctx);
	if (ctx_tmp != NULL) HMAC_free(ctx_tmp);
	memset(A1,0,sizeof(A1));
	return(ret);
	}

/* SSL-C: tls1_PRF returns a status (1 ok, 0 error) */
static int tls1_PRF(md5,sha1,label,label_len,sec,slen,out1,out2,olen)
EVP_MD *md5;
EVP_MD *sha1;
unsigned char *label;
int label_len;
unsigned char *sec;
int slen;
unsigned char *out1;
unsigned char *out2;
int olen;
	{
	int len,i;
	unsigned char *S1,*S2;

	len=slen/2;
	S1=sec;
	S2= &(sec[len]);
	len+=(slen&1); /* add for odd, make longer */

	if (!tls1_P_hash(md5 ,S1,len,label,label_len,out1,olen) ||
		!tls1_P_hash(sha1,S2,len,label,label_len,out2,olen))
		return(0);

	for (i=0; i<olen; i++)
		out1[i]^=out2[i];
	return(1);
	}

static int tls1_generate_key_block(s,km,tmp,num)
SSL *s;
unsigned char *km,*tmp;
int num;
	{
	unsigned char *p;
	unsigned char buf[SSL3_RANDOM_SIZE*2+
		TLS_MD_MAX_CONST_SIZE];
	p=buf;

	memcpy(p,TLS_MD_KEY_EXPANSION_CONST,
		TLS_MD_KEY_EXPANSION_CONST_SIZE);
	p+=TLS_MD_KEY_EXPANSION_CONST_SIZE;
	memcpy(p,s->s3->server_random,SSL3_RANDOM_SIZE);
	p+=SSL3_RANDOM_SIZE;
	memcpy(p,s->s3->client_random,SSL3_RANDOM_SIZE);
	p+=SSL3_RANDOM_SIZE;

	return(tls1_PRF(s->ctx->md5,s->ctx->sha1,buf,p-buf,
		s->session->master_key,s->session->master_key_length,
		km,tmp,num));
	}

int tls1_change_cipher_state(s,which)
SSL *s;
int which;
	{
	struct ssl_rw_st *rw;
	unsigned char *p,*key_block,*mac_secret;
	unsigned char *exp_label;
	unsigned char buf[RCOM_MAX_KEY_LENGTH+
		SSL3_RANDOM_SIZE*2];
	unsigned char tmp1[RCOM_MAX_KEY_LENGTH];
	unsigned char tmp2[RCOM_MAX_KEY_LENGTH];
	unsigned char iv1[RCOM_MAX_KEY_LENGTH*2];
	unsigned char iv2[RCOM_MAX_KEY_LENGTH*2];
	unsigned char *ms,*key,*iv,*er1,*er2;
	int client_write;
	EVP_CIPHER_CTX *dd;
	EVP_CIPHER *c;
	EVP_MD *m;
	int exp,n,i,j,k,exp_label_len;

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
		((rw->hash=(EVP_MD *)HMAC_new()) == NULL))
		goto err;
	s->hash_free=HMAC_free;
	if (!HMAC_Init((HMAC_CTX *)rw->hash,NULL,0,m))
		goto err;
	rw->mac_size=EVP_MD_size(m);
	dd=rw->enc_ctx;
	if (!EVP_CIPHER_CTX_init(dd))
		goto err;

	p=s->s3->tmp.key_block;
	i=EVP_MD_size(m);
	j=s->s3->tmp.new_cipher->algorithm2 & 0xff;
	if (!j)
		j=EVP_CIPHER_key_length(c);
	k=EVP_CIPHER_iv_length(c);
	er1= &(s->s3->client_random[0]);
	er2= &(s->s3->server_random[0]);
	if (	(which == SSL3_CHANGE_CIPHER_CLIENT_WRITE) ||
		(which == SSL3_CHANGE_CIPHER_SERVER_READ))
		{
		ms=  &(p[ 0]); n=i+i;
		key= &(p[ n]); n+=j+j;
		iv=  &(p[ n]); n+=k+k;
		exp_label=(unsigned char *)TLS_MD_CLIENT_WRITE_KEY_CONST;
		exp_label_len=TLS_MD_CLIENT_WRITE_KEY_CONST_SIZE;
		client_write=1;
		}
	else
		{
		n=i;
		ms=  &(p[ n]); n+=i+j;
		key= &(p[ n]); n+=j+k;
		iv=  &(p[ n]); n+=k;
		exp_label=(unsigned char *)TLS_MD_SERVER_WRITE_KEY_CONST;
		exp_label_len=TLS_MD_SERVER_WRITE_KEY_CONST_SIZE;
		client_write=0;
		}

	if (n > s->s3->tmp.key_block_length)
		{
#line 257 /* the line number is part of the binary */
		SSLerr(210,157);
		goto err2;
		}

	memcpy(mac_secret,ms,i);
#ifdef TLS_DEBUG
printf("which = %04X\nmac key=",which);
{ int z; for (z=0; z<i; z++) printf("%02X%c",ms[z],((z+1)%16)?' ':'\n'); }
#endif
	if (exp)
		{
		/* In here I set both the read and write key/iv to the
		 * same value since only the correct one will be used :-).
		 */
		p=buf;
		memcpy(p,exp_label,exp_label_len);
		p+=exp_label_len;
		memcpy(p,s->s3->client_random,SSL3_RANDOM_SIZE);
		p+=SSL3_RANDOM_SIZE;
		memcpy(p,s->s3->server_random,SSL3_RANDOM_SIZE);
		p+=SSL3_RANDOM_SIZE;
		if (!tls1_PRF(s->ctx->md5,s->ctx->sha1,buf,p-buf,key,j,
			tmp1,tmp2,EVP_CIPHER_key_length(c)))
			goto err2;
		key=tmp1;

		if (k > 0)
			{
			p=buf;
			memcpy(p,TLS_MD_IV_BLOCK_CONST,
				TLS_MD_IV_BLOCK_CONST_SIZE);
			p+=TLS_MD_IV_BLOCK_CONST_SIZE;
			memcpy(p,s->s3->client_random,SSL3_RANDOM_SIZE);
			p+=SSL3_RANDOM_SIZE;
			memcpy(p,s->s3->server_random,SSL3_RANDOM_SIZE);
			p+=SSL3_RANDOM_SIZE;
			if (!tls1_PRF(s->ctx->md5,s->ctx->sha1,
				buf,p-buf,"",0,iv1,iv2,k*2))
				goto err2;
			if (client_write)
				iv=iv1;
			else
				iv= &(iv1[k]);
			}
		}

	s->session->key_arg_length=0;

	if (!EVP_CipherInit(dd,c,key,iv,(which & SSL3_CC_WRITE)))
		goto err2;
#ifdef TLS_DEBUG
printf("which = %04X\nkey=",which);
{ int z; for (z=0; z<EVP_CIPHER_key_length(c); z++) printf("%02X%c",key[z],((z+1)%16)?' ':'\n'); }
printf("\niv=");
{ int z; for (z=0; z<k; z++) printf("%02X%c",iv[z],((z+1)%16)?' ':'\n'); }
printf("\n");
#endif

	memset(tmp1,0,sizeof(tmp1));
	memset(tmp2,0,sizeof(tmp2));
	memset(iv1,0,sizeof(iv1));
	memset(iv2,0,sizeof(iv2));
	return(1);
err:


	SSLerr(210,33);
err2:
	return(0);
	}

int tls1_setup_key_block(s)
SSL *s;
	{
	unsigned char *p1,*p2;
	EVP_CIPHER *c;
	EVP_MD *hash;
	int num,exp;

	if (s->s3->tmp.key_block_length != 0)
		return(1);

	if (!ssl_cipher_get_evp(s->session->cipher,&c,&hash))
		{
		SSLerr(212,SSL_R_CIPHER_OR_HASH_UNAVAILABLE);
		return(0);
		}

	s->s3->tmp.new_sym_enc=c;
	s->s3->tmp.new_hash=hash;

	exp=(s->session->cipher->algorithms & SSL_EXPORT)?1:0;

	num=EVP_CIPHER_key_length(c)+EVP_MD_size(hash)+EVP_CIPHER_iv_length(c);
	num*=2;
	num++;

	ssl3_cleanup_key_block(s);

	if ((p1=(unsigned char *)Malloc(num)) == NULL)
		goto err;
	if ((p2=(unsigned char *)Malloc(num)) == NULL)
		goto err;

	s->s3->tmp.key_block_length=num;
	s->s3->tmp.key_block=p1;


#ifdef TLS_DEBUG
printf("client random\n");
{ int z; for (z=0; z<SSL3_RANDOM_SIZE; z++) printf("%02X%c",s->s3->client_random[z],((z+1)%16)?' ':'\n'); }
printf("server random\n");
{ int z; for (z=0; z<SSL3_RANDOM_SIZE; z++) printf("%02X%c",s->s3->server_random[z],((z+1)%16)?' ':'\n'); }
printf("pre-master\n");
{ int z; for (z=0; z<s->session->master_key_length; z++) printf("%02X%c",s->session->master_key[z],((z+1)%16)?' ':'\n'); }
#endif
	if (!tls1_generate_key_block(s,p1,p2,num))
		goto err2;
	memset(p2,0,num);
	Free(p2);
#ifdef TLS_DEBUG
printf("\nkey block\n");
{ int z; for (z=0; z<num; z++) printf("%02X%c",p1[z],((z+1)%16)?' ':'\n'); }
#endif

	return(1);
err:




	SSLerr(212,ERR_R_MALLOC_FAILURE);
err2:
	return(0);
	}

int tls1_enc(s,send)
SSL *s;
int send;
	{
	SSL3_RECORD *rec;
	EVP_CIPHER_CTX *ds;
	unsigned long l;
	int bs,i,ii,j,k,n=0;
	EVP_CIPHER *enc;
	struct ssl_rw_st *rw;

	if (send)
		{
		rw=(struct ssl_rw_st *)&(s->write_hash_size);
		rec= &(s->s3->wrec);
		}
	else
		{
		rw=(struct ssl_rw_st *)&(s->read_hash_size);
		rec= &(s->s3->rrec);
		}

	if (rw->hash != NULL)
		n=rw->mac_size;
	ds=rw->enc_ctx;
	if (rw->enc_ctx == NULL)
		enc=NULL;
	else
		enc=EVP_CIPHER_CTX_cipher(rw->enc_ctx);

	if ((s->session == NULL) || (ds == NULL) || (enc == NULL))
		{
		memcpy(rec->data,rec->input,rec->length);
		rec->input=rec->data;
		}
	else
		{
		l=rec->length;
		bs=EVP_CIPHER_CTX_block_size(ds);

		if ((bs != 1) && send)
			{
			i=bs-((int)l%bs);

			/* Add weird padding of upto 256 bytes */

			/* we need to add 'i' padding bytes of value j */
			j=i-1;
			if (s->options & SSL_OP_TLS_BLOCK_PADDING_BUG)
				{
				if (s->s3->flags & TLS1_FLAGS_TLS_PADDING_BUG)
					j++;
				}
			for (k=(int)l; k<(int)(l+i); k++)
				rec->input[k]=j;
			l+=i;
			rec->length+=i;
			}

		EVP_Cipher(ds,rec->data,rec->input,l);

		if ((bs != 1) && !send)
			{
			ii=i=rec->data[l-1];
			i++;
			if (s->options&SSL_OP_TLS_BLOCK_PADDING_BUG)
				{
				/* First packet is even in size, so check */
				if ((memcmp(s->s3->read_sequence,
					"\0\0\0\0\0\0\0\0",8) == 0) && !(ii & 1))
					s->s3->flags|=TLS1_FLAGS_TLS_PADDING_BUG;
				if (s->s3->flags & TLS1_FLAGS_TLS_PADDING_BUG)
					i--;
				}
			if (i > (int)rec->length)
				{
#line 470 /* the line number is part of the binary */
				SSLerr(211,129);
				ssl3_send_alert(s,SSL3_AL_FATAL,SSL_AD_DECRYPTION_FAILED);
				return(0);
				}
			for (j=(int)(l-i); j<(int)l; j++)
				{
				if (rec->data[j] != ii)
					{
					SSLerr(211,147);
					ssl3_send_alert(s,SSL3_AL_FATAL,SSL_AD_DECRYPTION_FAILED);
					return(0);
					}
				}
			rec->length-=i;
			}
		}
	return(1);
	}

int tls1_cert_verify_mac(s,in_ctx,out)
SSL *s;
EVP_MD_CTX *in_ctx;
unsigned char *out;
	{
	unsigned int ret;

	if (!EVP_MD_CTX_copy(s->md_ctx,in_ctx))
		return(0);
	EVP_DigestFinal(s->md_ctx,out,&ret);
	return((int)ret);
	}

int tls1_final_finish_mac(s,in1_ctx,in2_ctx,str,slen,out)
SSL *s;
EVP_MD_CTX *in1_ctx,*in2_ctx;
unsigned char *str;
int slen;
unsigned char *out;
	{
	unsigned int i;
	unsigned char buf[TLS_MD_MAX_CONST_SIZE+MD5_DIGEST_LENGTH+SHA_DIGEST_LENGTH];
	unsigned char *q,buf2[12];

	q=buf;
	memcpy(q,str,slen);
	q+=slen;

	if (!EVP_MD_CTX_copy(s->md_ctx,in1_ctx))
		return(0);
	EVP_DigestFinal(s->md_ctx,q,&i);
	q+=i;
	if (!EVP_MD_CTX_copy(s->md_ctx,in2_ctx))
		return(0);
	EVP_DigestFinal(s->md_ctx,q,&i);
	q+=i;

	if (!tls1_PRF(s->ctx->md5,s->ctx->sha1,buf,q-buf,
		s->session->master_key,s->session->master_key_length,
		out,buf2,12))
		return(0);

	return((int)12);
	}

int tls1_mac(ssl,md,send)
SSL *ssl;
unsigned char *md;
int send;
	{
	SSL3_RECORD *rec;
	unsigned char *mac_sec,*seq;
	unsigned int md_size;
	int i;
	HMAC_CTX *hash;
	EVP_MD *m;
	unsigned char buf[5]; 

	if (send)
		{
		rec= &(ssl->s3->wrec);
		mac_sec= &(ssl->s3->write_mac_secret[0]);
		seq= &(ssl->s3->write_sequence[0]);
		hash=(HMAC_CTX *)ssl->write_hash;
		}
	else
		{
		rec= &(ssl->s3->rrec);
		mac_sec= &(ssl->s3->read_mac_secret[0]);
		seq= &(ssl->s3->read_sequence[0]);
		hash=(HMAC_CTX *)ssl->read_hash;
		}

	m=HMAC_digest(hash);
	md_size=EVP_MD_size(m);

	buf[0]=rec->type;
	buf[1]=TLS1_VERSION_MAJOR;
	buf[2]=TLS1_VERSION_MINOR;
	buf[3]=rec->length>>8;
	buf[4]=rec->length&0xff;

	/* I should fix this up TLS TLS TLS TLS TLS XXXXXXXX */
	if (!HMAC_Init(hash,mac_sec,md_size,NULL))
		return(0);
	HMAC_Update(hash,seq,8);
	HMAC_Update(hash,buf,5);
	HMAC_Update(hash,rec->input,rec->length);
	if (!HMAC_Final(hash,md,&md_size))
		return(0);

#ifdef TLS_DEBUG
printf("sec=");
{int z; for (z=0; z<md_size; z++) printf("%02X ",mac_sec[z]); printf("\n"); }
printf("seq=");
{int z; for (z=0; z<8; z++) printf("%02X ",seq[z]); printf("\n"); }
printf("buf=");
{int z; for (z=0; z<5; z++) printf("%02X ",buf[z]); printf("\n"); }
printf("rec=");
{int z; for (z=0; z<rec->length; z++) printf("%02X ",buf[z]); printf("\n"); }
#endif

	for (i=7; i>=0; i--)
		if (++seq[i]) break; 

#ifdef TLS_DEBUG
{int z; for (z=0; z<md_size; z++) printf("%02X ",md[z]); printf("\n"); }
#endif
	return(md_size);
	}

int tls1_generate_master_secret(s,out,p,len)
SSL *s;
unsigned char *out;
unsigned char *p;
int len;
	{
	unsigned char buf[SSL3_RANDOM_SIZE*2+TLS_MD_MASTER_SECRET_CONST_SIZE];
	unsigned char buff[SSL_MAX_MASTER_KEY_LENGTH];

	/* Setup the stuff to munge */
	memcpy(buf,TLS_MD_MASTER_SECRET_CONST,
		TLS_MD_MASTER_SECRET_CONST_SIZE);
	memcpy(&(buf[TLS_MD_MASTER_SECRET_CONST_SIZE]),
		s->s3->client_random,SSL3_RANDOM_SIZE);
	memcpy(&(buf[SSL3_RANDOM_SIZE+TLS_MD_MASTER_SECRET_CONST_SIZE]),
		s->s3->server_random,SSL3_RANDOM_SIZE);
	if (tls1_PRF(s->ctx->md5,s->ctx->sha1,
		buf,TLS_MD_MASTER_SECRET_CONST_SIZE+SSL3_RANDOM_SIZE*2,p,len,
		s->session->master_key,buff,SSL3_MASTER_SECRET_SIZE) == 0)
		return(0);
	return(SSL3_MASTER_SECRET_SIZE);
	}

int tls1_alert_code(code)
int code;
	{
	switch (code)
		{
	case SSL_AD_CLOSE_NOTIFY:	return(SSL3_AD_CLOSE_NOTIFY);
	case SSL_AD_UNEXPECTED_MESSAGE:	return(SSL3_AD_UNEXPECTED_MESSAGE);
	case SSL_AD_BAD_RECORD_MAC:	return(SSL3_AD_BAD_RECORD_MAC);
	case SSL_AD_DECRYPTION_FAILED:	return(TLS1_AD_DECRYPTION_FAILED);
	case SSL_AD_RECORD_OVERFLOW:	return(TLS1_AD_RECORD_OVERFLOW);
	case SSL_AD_DECOMPRESSION_FAILURE:return(SSL3_AD_DECOMPRESSION_FAILURE);
	case SSL_AD_HANDSHAKE_FAILURE:	return(SSL3_AD_HANDSHAKE_FAILURE);
	case SSL_AD_NO_CERTIFICATE:	return(-1);
	case SSL_AD_BAD_CERTIFICATE:	return(SSL3_AD_BAD_CERTIFICATE);
	case SSL_AD_UNSUPPORTED_CERTIFICATE:return(SSL3_AD_UNSUPPORTED_CERTIFICATE);
	case SSL_AD_CERTIFICATE_REVOKED:return(SSL3_AD_CERTIFICATE_REVOKED);
	case SSL_AD_CERTIFICATE_EXPIRED:return(SSL3_AD_CERTIFICATE_EXPIRED);
	case SSL_AD_CERTIFICATE_UNKNOWN:return(SSL3_AD_CERTIFICATE_UNKNOWN);
	case SSL_AD_ILLEGAL_PARAMETER:	return(SSL3_AD_ILLEGAL_PARAMETER);
	case SSL_AD_UNKNOWN_CA:		return(TLS1_AD_UNKNOWN_CA);
	case SSL_AD_ACCESS_DENIED:	return(TLS1_AD_ACCESS_DENIED);
	case SSL_AD_DECODE_ERROR:	return(TLS1_AD_DECODE_ERROR);
	case SSL_AD_DECRYPT_ERROR:	return(TLS1_AD_DECRYPT_ERROR);
	case SSL_AD_EXPORT_RESTRICION:	return(TLS1_AD_EXPORT_RESTRICION);
	case SSL_AD_PROTOCOL_VERSION:	return(TLS1_AD_PROTOCOL_VERSION);
	case SSL_AD_INSUFFICIENT_SECURITY:return(TLS1_AD_INSUFFICIENT_SECURITY);
	case SSL_AD_INTERNAL_ERROR:	return(TLS1_AD_INTERNAL_ERROR);
	case SSL_AD_USER_CANCLED:	return(TLS1_AD_USER_CANCLED);
	case SSL_AD_NO_RENEGOTIATION:	return(TLS1_AD_NO_RENEGOTIATION);
	default:			return(-1);
		}
	}

