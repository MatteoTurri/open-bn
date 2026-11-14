/* crypto/pem/pem_lib.c */
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
#include "cryptlib.h"
#include "buffer.h"
#include "objects.h"
#include "evp.h"
#include "rand.h"
#include "x509.h"
#include "pem.h"
#ifndef NO_DES
#include "des.h"
#endif


#define MIN_LENGTH	4
#define PEM_BUFSIZE	1024

/* PEMerr(PEM_F_PEM_WRITE_BIO,ERR_R_MALLOC_FAILURE);
 * PEMerr(PEM_F_PEM_READ_BIO,ERR_R_MALLOC_FAILURE);
 */

#ifndef NOPROTO
static int load_iv(unsigned char **fromp,unsigned char *to, int num);
#else
static int load_iv();
#endif

/* SSL-C: this unit keeps PEM_do_header, PEM_get_EVP_CIPHER_CTX and load_iv; PEM_read_bio is in
 * pem_lib_readbio.c, and the other functions of pem_lib.c are not in the binary. */

/* SSL-C: cipher is an EVP_CIPHER_CTX already set up with cipher and IV, and EVP_DecryptInit is
 * called again on it with NULL type and iv. The key buffer is 32 bytes. */
int PEM_do_header(cipher, data, plen, callback)
EVP_CIPHER_CTX *cipher;
unsigned char *data;
long *plen;
int (*callback)();
	{
	int i,j,o,klen;
	long len;
	unsigned char key[32];
	char buf[PEM_BUFSIZE];

	len= *plen;

	if (EVP_CIPHER_CTX_cipher(cipher) == NULL) return(1);
	if (callback == NULL)
		klen=pem_pwd_def_cb(buf,PEM_BUFSIZE,0);
	else
		klen=callback(buf,PEM_BUFSIZE,0);
	if (klen <= 0)
		{
#line 388 /* the line number is part of the binary */
		PEMerr(PEM_F_PEM_DO_HEADER,PEM_R_BAD_PASSWORD_READ);
		return(0);
		}
	if (!EVP_BytesToKey(EVP_CIPHER_CTX_cipher(cipher),EVP_md5_digest(),
		EVP_CIPHER_CTX_iv(cipher),(unsigned char *)buf,klen,1,key,NULL))
		return(0);

	j=(int)len;
	if (!EVP_DecryptInit(cipher,NULL,key,NULL))
		return(0);
	EVP_DecryptUpdate(cipher,data,&i,data,j);
	o=EVP_DecryptFinal(cipher,&(data[i]),&j);
	EVP_CIPHER_CTX_cleanup(cipher);
	memset((char *)buf,0,sizeof(buf));
	memset((char *)key,0,sizeof(key));
	j+=i;
	if (!o)
		{
#line 407 /* the line number is part of the binary */
		PEMerr(PEM_F_PEM_DO_HEADER,PEM_R_BAD_DECRYPT);
		return(0);
		}
	*plen=j;
	return(1);
	}

/* SSL-C: cipher is an EVP_CIPHER_CTX the caller allocated, set up here with EVP_DecryptInit; the
 * IV is a local buffer of EVP_CIPHER_iv_length(enc) bytes. */
int PEM_get_EVP_CIPHER_CTX(header,cipher)
char *header;
EVP_CIPHER_CTX *cipher;
	{
	EVP_CIPHER *enc=NULL;
	char *p,c;
	unsigned char iv[EVP_MAX_IV_LENGTH];

	EVP_CIPHER_CTX_cleanup(cipher);
	if ((header == NULL) || (*header == '\0') || (*header == '\n'))
		return(1);
	if (strncmp(header,"Proc-Type: ",11) != 0)
#line 432 /* the line number is part of the binary */
		{ PEMerr(PEM_F_PEM_GET_EVP_CIPHER_CTX,PEM_R_NOT_PROC_TYPE); return(0); }
	header+=11;
	if (*header != '4') return(0); header++;
	if (*header != ',') return(0); header++;
	if (strncmp(header,"ENCRYPTED",9) != 0)
		{ PEMerr(PEM_F_PEM_GET_EVP_CIPHER_CTX,PEM_R_NOT_ENCRYPTED); return(0); }
	for (; (*header != '\n') && (*header != '\0'); header++)
		;
	if (*header == '\0')
		{ PEMerr(PEM_F_PEM_GET_EVP_CIPHER_CTX,PEM_R_SHORT_HEADER); return(0); }
	header++;
	if (strncmp(header,"DEK-Info: ",10) != 0)
		{ PEMerr(PEM_F_PEM_GET_EVP_CIPHER_CTX,PEM_R_NOT_DEK_INFO); return(0); }
	header+=10;

	p=header;
	for (;;)
		{
		c= *header;
		if (!(	((c >= 'A') && (c <= 'Z')) || (c == '-') ||
			((c >= '0') && (c <= '9'))))
			break;
		header++;
		}
	*header='\0';
	enc=EVP_OBJ_get_cipherbyname(p);
	*header=c;
	header++;

	if (enc == NULL)
		{
		PEMerr(PEM_F_PEM_GET_EVP_CIPHER_CTX,PEM_R_UNSUPPORTED_ENCRYPTION);
		return(0);
		}
	if (!load_iv((unsigned char **)&header,iv,EVP_CIPHER_iv_length(enc)))
		return(0);

	return(EVP_DecryptInit(cipher,enc,NULL,iv));
	}

static int load_iv(fromp,to,num)
unsigned char **fromp,*to;
int num;
	{
	int v,i;
	unsigned char *from;

	from= *fromp;
	for (i=0; i<num; i++) to[i]=0;
	num*=2;
	for (i=0; i<num; i++)
		{
		if ((*from >= '0') && (*from <= '9'))
			v= *from-'0';
		else if ((*from >= 'A') && (*from <= 'F'))
			v= *from-'A'+10;
		else if ((*from >= 'a') && (*from <= 'f'))
			v= *from-'a'+10;
		else
			{
			PEMerr(PEM_F_LOAD_IV,PEM_R_BAD_IV_CHARS);
			return(0);
			}
		from++;
		to[i/2]|=v<<(long)((!(i&1))*4);
		}

	*fromp=from;
	return(1);
	}
