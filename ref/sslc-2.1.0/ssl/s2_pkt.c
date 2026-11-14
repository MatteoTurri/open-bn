/* ssl/s2_pkt.c */
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
#include <errno.h>
#define USE_SOCKETS
#include "ssl_locl.h"

/* SSLerr(SSL_F_GET_SERVER_HELLO,SSL_R_PEER_ERROR_NO_CIPHER);
 * SSLerr(SSL_F_GET_SERVER_HELLO,SSL_R_PEER_ERROR_NO_CERTIFICATE);
 * SSLerr(SSL_F_GET_SERVER_HELLO,SSL_R_PEER_ERROR_CERTIFICATE);
 * SSLerr(SSL_F_GET_SERVER_HELLO,SSL_R_PEER_ERROR_UNSUPPORTED_CERTIFICATE_TYPE);
 * SSLerr(SSL_F_GET_SERVER_HELLO,SSL_R_UNKNOWN_REMOTE_ERROR_TYPE);
 */

#ifndef NOPROTO
static int read_n(SSL *s,unsigned int n,unsigned int max,unsigned int extend);
static int do_ssl_write(SSL *s, char *buf, unsigned int len);
static int write_pending(SSL *s, char *buf, unsigned int len);
static int ssl_mt_error(int n);
#else
static int read_n();
static int do_ssl_write();
static int write_pending();
static int ssl_mt_error();
#endif

int ssl2_peek(s,buf,len)
SSL *s;
char *buf;
int len;
	{
	int ret;

	ret=ssl2_read(s,buf,len);
	if (ret > 0)
	        {
		s->s2->ract_data_length+=ret;
		s->s2->ract_data-=ret;
		}
	return(ret);
	}

/* SSL_read -
 * This routine will return 0 to len bytes, decrypted etc if required.
 */
int ssl2_read(s, buf, len)
SSL *s;
char *buf;
int len;
	{
	int n;
	unsigned char mac[MAX_MAC_SIZE];
	unsigned char *p;
	int i;
	unsigned int mac_size=0;
	void (*cb)()=NULL;
	void *cbarg=NULL;

	if ((s->s2->rbuf == NULL) && !ssl2_setup_buffers(s))
		return(-1);

	if (SSL_state(s) & SSL_ST_INIT)
		{
		if (!s->in_handshake)
			{
			if (s->handshake_func == NULL)
				{
#line 74 /* the line number is part of the binary */
				SSLerr(125,1101);
				return(-1);
				}
			n=s->handshake_func(s);
			if (n < 0) return(n);
			if (n == 0)
				{
				SSLerr(125,229);
				return(-1);
				}
			}
		}

	clear_sys_error();
	s->rwstate=SSL_NOTHING;
	if (len <= 0) return(len);

	if (s->s2->ract_data_length != 0) /* read from buffer */
		{
		if (len > s->s2->ract_data_length)
			n=s->s2->ract_data_length;
		else
			n=len;

		memcpy(buf,s->s2->ract_data,(unsigned int)n);
		s->s2->ract_data_length-=n;
		s->s2->ract_data+=n;
		if (s->s2->ract_data_length == 0)
			s->rstate=SSL_ST_READ_HEADER;
		return(n);
		}

	if (s->rstate == SSL_ST_READ_HEADER)
		{
		if (s->first_packet)
			{
			if ((s->s2->rbuf == NULL) &&
				!ssl2_alloc_read_buf(s,s->rbuf_size))
				return(-1);
			n=read_n(s,5,s->rbuf_size,0);
			if (n <= 0) return(n); /* error or non-blocking */
			s->first_packet=0;

			p=s->packet;
			if (!((p[0] & 0x80) && (
				(p[2] == SSL2_MT_CLIENT_HELLO) ||
				(p[2] == SSL2_MT_SERVER_HELLO))))
				{
				SSLerr(125,175);
				return(-1);
				}
			}
		else
			{
			n=read_n(s,2,s->rbuf_size,0);
			if (n <= 0) return(n); /* error or non-blocking */
			}
		/* part read stuff */

		s->rstate=SSL_ST_READ_BODY;

		p=s->packet;
		/* Do header */
		/*s->s2->padding=0;*/
		s->s2->escape=0;
		s->s2->rlength=(((unsigned int)p[0])<<8)|((unsigned int)p[1]);
		if ((p[0] & TWO_BYTE_BIT))		/* Two byte header? */
			{
			s->s2->three_byte_header=0;
			s->s2->rlength&=TWO_BYTE_MASK;	
			}
		else
			{
			s->s2->three_byte_header=1;
			s->s2->rlength&=THREE_BYTE_MASK;

			/* security >s2->escape */
			s->s2->escape=((p[0] & SEC_ESC_BIT))?1:0;
			}
		}

	if (s->rstate == SSL_ST_READ_BODY)
		{
		n=s->s2->rlength+2+s->s2->three_byte_header;
		if (s->rbuf_size < n)
			if (!ssl2_alloc_read_buf(s,n)) return(-1);
		if (n > (int)s->packet_length)
			{
			n-=s->packet_length;
			i=read_n(s,(unsigned int)n,(unsigned int)n,1);
			if (i <= 0) return(i); /* ERROR */
			}

		p= &(s->packet[2]);
		s->rstate=SSL_ST_READ_HEADER;
		if (s->s2->three_byte_header)
			s->s2->padding= *(p++);
		else	s->s2->padding=0;

		/* Data portion */
		if (s->s2->clear_text)
			{
			s->s2->mac_data=p;
			s->s2->ract_data=p;
			s->s2->pad_data=NULL;
			}
		else
			{
			mac_size=s->read_hash_size;
			s->s2->mac_data=p;
			s->s2->ract_data= &p[mac_size];
			s->s2->pad_data= &p[mac_size+
				s->s2->rlength-s->s2->padding];
			}

		s->s2->ract_data_length=s->s2->rlength;
		/* added a check for length > max_size in case
		 * encryption was not turned on yet due to an error */
		if ((!s->s2->clear_text) &&
			(s->s2->rlength >= mac_size))
			{
			ssl2_enc(s,0);
			s->s2->ract_data_length-=mac_size;
			ssl2_mac(s,mac,0);
			s->s2->ract_data_length-=s->s2->padding;
			if (	(memcmp(mac,s->s2->mac_data,
				(unsigned int)mac_size) != 0) ||
				(s->s2->rlength%EVP_CIPHER_CTX_block_size(s->enc_read_ctx) != 0))
				{
				SSLerr(125,113);
				return(-1);
				}
			}
		INC32(s->s2->read_sequence); /* expect next number */
		/* s->s2->ract_data is now available for processing */

		/* If a 0 byte packet was sent, return 0, otherwise
		 * we play havoc with people using select with
		 * blocking sockets.  Let them handle a packet at a time,
		 * they should really be using non-blocking sockets. */
		if (s->s2->ract_data_length == 0)
			return(0);
		n=ssl2_read(s,buf,len);
		if (!s->in_handshake)
			{
			if (s->app_data_cb != NULL)
				{
				cb=s->app_data_cb;
				cbarg=s->app_data_cb_arg;
				}
			else if (s->ctx->ctx_app_data_cb != NULL)
				{ cb=s->ctx->ctx_app_data_cb; cbarg=s->ctx->ctx_app_data_cb_arg; }
			if (cb != NULL)
				cb(s,4,cbarg);
			}
		return(n);
		}
	else
		{
		SSLerr(125,126);
			return(-1);
		}
	}

#line 234 /* the line number is part of the binary */
static int read_n(s, n, max, extend)
SSL *s;
unsigned int n;
unsigned int max;
unsigned int extend;
	{
	int i,off,newb;

	/* if there is stuff still in the buffer from a previous read,
	 * and there is more than we want, take some. */
	if (s->s2->rbuf_left >= (int)n)
		{
		if (extend)
			s->packet_length+=n;
		else
			{
			s->packet= &(s->s2->rbuf[s->s2->rbuf_offs]);
			s->packet_length=n;
			}
		s->s2->rbuf_left-=n;
		s->s2->rbuf_offs+=n;
		return(n);
		}

	if (!s->read_ahead) max=n;
	/* SSL-C: the limit is the configured read buffer size, not the protocol constant. */

	if (max > (unsigned int)s->rbuf_size)
		max=s->rbuf_size;
	

	/* Else we want more than we have.
	 * First, if there is some left or we want to extend */
	off=0;
	if ((s->s2->rbuf_left != 0) || ((s->packet_length != 0) && extend))
		{
		newb=s->s2->rbuf_left;
		if (extend)
			{
			off=s->packet_length;
			if (s->packet != s->s2->rbuf)
				memcpy(s->s2->rbuf,s->packet,
					(unsigned int)newb+off);
			}
		else if (s->s2->rbuf_offs != 0)
			{
			memcpy(s->s2->rbuf,&(s->s2->rbuf[s->s2->rbuf_offs]),
				(unsigned int)newb);
			s->s2->rbuf_offs=0;
			}
		s->s2->rbuf_left=0;
		}
	else
		newb=0;

	/* off is the offset to start writing too.
	 * r->s2->rbuf_offs is the 'unread data', now 0. 
	 * newb is the number of new bytes so far
	 */
	s->packet=s->s2->rbuf;
	while (newb < (int)n)
		{
		clear_sys_error();
		if (s->rbio != NULL)
			{
			s->rwstate=SSL_READING;
			i=BIO_read(s->rbio,(char *)&(s->s2->rbuf[off+newb]),
				max-newb);
			}
		else
			{
			SSLerr(SSL_F_READ_N,211);
			i= -1;
			}
#ifdef PKT_DEBUG
		if (s->debug & 0x01) sleep(1);
#endif
		if (i <= 0)
			{
			s->s2->rbuf_left+=newb;
			return(i);
			}
		newb+=i;
		}

	/* record unread data */
	if (newb > (int)n)
		{
		s->s2->rbuf_offs=n+off;
		s->s2->rbuf_left=newb-n;
		}
	else
		{
		s->s2->rbuf_offs=0;
		s->s2->rbuf_left=0;
		}
	if (extend)
		s->packet_length+=n;
	else
		s->packet_length=n;
	s->rwstate=SSL_NOTHING;
	return(n);
	}

int ssl2_write(s, buf, len)
SSL *s;
char *buf;
int len;
	{
	unsigned int n,tot;
	int i;
	void (*cb)()=NULL;
	void *cbarg=NULL;

	if (SSL_state(s) & SSL_ST_INIT)
		{
		if (!s->in_handshake)
			{
			if (s->handshake_func == NULL)
				{
#line 352 /* the line number is part of the binary */
				SSLerr(127,1101);
				return(-1);
				}
			i=s->handshake_func(s);
			if (i < 0) return(i);
			if (i == 0)
				{
				SSLerr(127,229);
				return(-1);
				}
			}
		}

	if (s->error)
		{
		ssl2_write_error(s);
		if (s->error)
			return(-1);
		}

	clear_sys_error();
	s->rwstate=SSL_NOTHING;
	if (len <= 0) return(len);

	tot=s->s2->wnum;
	s->s2->wnum=0;

	n=(len-tot);
again:
		{
		i=do_ssl_write(s,&(buf[tot]),n);
		if (!s->in_handshake)
			{
			if (s->app_data_cb != NULL)
				{
				cb=s->app_data_cb;
				cbarg=s->app_data_cb_arg;
				}
			else if (s->ctx->ctx_app_data_cb != NULL)
				{
				cb=s->ctx->ctx_app_data_cb;
				cbarg=s->ctx->ctx_app_data_cb_arg;
				}
			if (cb != NULL)
				cb(s,8,cbarg);
			}
		if (i <= 0)
			{
			s->s2->wnum=tot;
			return(i);
			}
		if (i == (int)n) return(tot+i);

		n-=i;
		tot+=i;
		}
	goto again;
	}

#line 410 /* the line number is part of the binary */
static int write_pending(s,buf,len)
SSL *s;
char *buf;
unsigned int len;
	{
	int i;

	/* s->s2->wpend_len != 0 MUST be true. */

	/* check that they have given us the same buffer to
	 * write */
	if ((s->s2->wpend_tot > (int)len) || (s->s2->wpend_buf != buf))
		{
		SSLerr(SSL_F_WRITE_PENDING,SSL_R_BAD_WRITE_RETRY);
		return(-1);
		}

	for (;;)
		{
		clear_sys_error();
		if (s->wbio != NULL)
			{
			s->rwstate=SSL_WRITING;
			i=BIO_write(s->wbio,
				(char *)&(s->s2->write_ptr[s->s2->wpend_off]),
				(unsigned int)s->s2->wpend_len);
			}
		else
			{
			SSLerr(SSL_F_WRITE_PENDING,SSL_R_WRITE_BIO_NOT_SET);
			i= -1;
			}
#ifdef PKT_DEBUG
		if (s->debug & 0x01) sleep(1);
#endif
		if (i == s->s2->wpend_len)
			{
			s->s2->wpend_len=0;
			s->rwstate=SSL_NOTHING;
			return(s->s2->wpend_ret);
			}
		else if (i <= 0)
			return(i);
		s->s2->wpend_off+=i;
		s->s2->wpend_len-=i;
		}
	}

static int do_ssl_write(s, buf, len)
SSL *s;
char *buf;
unsigned int len;
	{
	unsigned int j,k,olen,p,mac_size,bs;
	unsigned int hlen;
	register unsigned char *pp;

	olen=len;

	/* first check if there is data from an encryption waiting to
	 * be sent - it must be sent because the other end is waiting.
	 * This will happen with non-blocking IO.  We print it and then
	 * return.
	 */
	if (s->s2->wpend_len != 0) return(write_pending(s,buf,len));

	/* set mac_size to mac size */
	if (s->s2->clear_text)
		mac_size=0;
	else
		mac_size=s->write_hash_size;

	/* lets set the pad p */
	/* RCOM: the record limits come from the size of the write buffer
	 * (s->wbuf_size), not from the SSL2_MAX_RECORD_LENGTH_*
	 * constants */
	if (s->s2->clear_text)
		{
		if (len > s->wbuf_size-2)
			len=s->wbuf_size-2;
		p=0;
		s->s2->three_byte_header=0;
		/* len=len; */
		}
	else
		{
		bs=EVP_CIPHER_CTX_block_size(s->enc_read_ctx);
		j=len+mac_size;
		if ((j > s->wbuf_size-3) &&
			(!s->s2->escape))
			{
			if (j > s->wbuf_size-2)
				j=s->wbuf_size-2;
			/* set k to the max number of bytes with 2
			 * byte header */
			k=j-(j%bs);
			/* how many data bytes? */
			len=k-mac_size; 
			s->s2->three_byte_header=0;
			p=0;
			}
		else if ((bs <= 1) && (!s->s2->escape))
			{
			/* len=len; */
			s->s2->three_byte_header=0;
			p=0;
			}
		else /* 3 byte header */
			{
			/*len=len; */
			p=(j%bs);
			p=(p == 0)?0:(bs-p);
			if (s->s2->escape)
				s->s2->three_byte_header=1;
			else
				s->s2->three_byte_header=(p != 0);
			}
		}
	/* mac_size is the number of MAC bytes
	 * len is the number of data bytes we are going to send
	 * p is the number of padding bytes
	 * if p == 0, it is a 2 byte header */

	/* RCOM: the data start right after the header actually used */
	hlen=2;
	if (s->s2->three_byte_header == 1)
		hlen=3;

	s->s2->wlength=len;
	s->s2->padding=p;
	s->s2->mac_data= &(s->s2->wbuf[hlen]);
	s->s2->wact_data= &(s->s2->wbuf[hlen+mac_size]);
	/* we copy the data into s->s2->wbuf */
	memcpy(s->s2->wact_data,buf,len);
#ifdef PURIFY
	if (p)
		memset(&(s->s2->wact_data[len]),0,p);
#endif

	if (!s->s2->clear_text)
		{
		s->s2->wact_data_length=len+p;
		ssl2_mac(s,s->s2->mac_data,1);
		s->s2->wlength+=p+mac_size;
		ssl2_enc(s,1);
		}

	/* package up the header */
	s->s2->wpend_len=s->s2->wlength;
	if (s->s2->three_byte_header) /* 3 byte header */
		{
		pp=s->s2->mac_data;
		pp-=3;
		pp[0]=(s->s2->wlength>>8)&(THREE_BYTE_MASK>>8);
		if (s->s2->escape) pp[0]|=SEC_ESC_BIT;
		pp[1]=s->s2->wlength&0xff;
		pp[2]=s->s2->padding;
		s->s2->wpend_len+=3;
		}
	else
		{
		pp=s->s2->mac_data;
		pp-=2;
		pp[0]=((s->s2->wlength>>8)&(TWO_BYTE_MASK>>8))|TWO_BYTE_BIT;
		pp[1]=s->s2->wlength&0xff;
		s->s2->wpend_len+=2;
		}
	s->s2->write_ptr=pp;
	
	INC32(s->s2->write_sequence); /* expect next number */

	/* lets try to actually write the data */
	s->s2->wpend_tot=olen;
	s->s2->wpend_buf=(char *)buf;

	s->s2->wpend_ret=len;

	s->s2->wpend_off=0;
	return(write_pending(s,buf,olen));
	}

#line 589 /* the line number is part of the binary */
int ssl2_part_read(s,f,i)
SSL *s;
unsigned long f;
int i;
	{
	unsigned char *p;
	int j;

	/* check for error */
	if ((s->init_num == 0) && (i >= 3))
		{
		p=(unsigned char *)s->init_buf->data;
		if (p[0] == SSL2_MT_ERROR)
			{
			j=(p[1]<<8)|p[2];
			SSLerr((int)f,ssl_mt_error(j));
			}
		}

	if (i < 0)
		{
		/* ssl2_return_error(s); */
		/* for non-blocking io,
		 * this is not fatal */
		return(i);
		}
	else
		{
		s->init_num+=i;
		return(0);
		}
	}

int ssl2_do_write(s)
SSL *s;
	{
	int ret;

	ret=ssl2_write(s,(char *)&(s->init_buf->data[s->init_off]),
		s->init_num);
	if (ret == s->init_num)
		return(1);
	if (ret < 0)
		return(-1);
	s->init_off+=ret;
	s->init_num-=ret;
	return(0);
	}

static int ssl_mt_error(n)
int n;
	{
	int ret;

	switch (n)
		{
	/* SSL-C: the SSL_R_* values are the fork's. */
	case SSL2_PE_NO_CIPHER:
		ret=203;
		break;
	case SSL2_PE_NO_CERTIFICATE:
		ret=202;
		break;
	case SSL2_PE_BAD_CERTIFICATE:
		ret=201;
		break;
	case SSL2_PE_UNSUPPORTED_CERTIFICATE_TYPE:
		ret=204;
		break;
	default:
		ret=253;
		break;
		}
	return(ret);
	}

void ssl2_return_error(s,err)
SSL *s;
int err;
	{
	if (!s->error)
		{
		s->error=3;
		s->error_code=err;

		ssl2_write_error(s);
		}
	}

void ssl2_write_error(s)
SSL *s;
	{
	char buf[3];
	int i,error;
	void (*cb)()=NULL;
	char *arg=NULL;

	buf[0]=SSL2_MT_ERROR;
	buf[1]=(s->error_code>>8)&0xff;
	buf[2]=(s->error_code)&0xff;

/*	state=s->rwstate;*/
	error=s->error;
	s->error=0;
	i=ssl2_write(s,&(buf[3-error]),error);
/*	if (i == error) s->rwstate=state; */

	if (i < 0)
		s->error=error;
	else if (i != s->error)
		{
		s->error=error-i;
		if (s->debug)
			{
			s->rwstate=SSL_WRITING;
			BIO_flush(s->wbio);
			s->rwstate=SSL_NOTHING;
			}
		if (s->alert_info_cb != NULL)
			{
			cb=s->alert_info_cb;
			arg=s->alert_info_cb_arg;
			}
		else if (s->ctx->ctx_alert_info_cb != NULL)
			{
			cb=s->ctx->ctx_alert_info_cb;
			arg=s->ctx->ctx_alert_info_cb_arg;
			}
		if (cb != NULL)
			cb(s,SSL_CB_ALERT|SSL_CB_WRITE,s->error_code,arg);
		}
	/* else
		s->error=0; */
	}
