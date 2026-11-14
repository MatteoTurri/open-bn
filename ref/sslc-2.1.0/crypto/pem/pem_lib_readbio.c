/*
 * PEM_read_bio, from crypto/pem/pem_lib.c of SSLeay.
 */
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

/* SSL-C: the EVP_ENCODE_CTX is allocated (EVP_ENCODE_CTX_new) and freed on both paths. */
int PEM_read_bio(bp, name, header, data, len)
BIO *bp;
char **name;
char **header;
unsigned char **data;
long *len;
	{
	EVP_ENCODE_CTX *ctx;
	int end=0,i,k,bl=0,hl=0,nohead=0;
	char buf[256];
	BUF_MEM *nameB;
	BUF_MEM *headerB;
	BUF_MEM *dataB,*tmpB;

	nameB=BUF_MEM_new();
	headerB=BUF_MEM_new();
	dataB=BUF_MEM_new();
	ctx=EVP_ENCODE_CTX_new();
	if ((nameB == NULL) || (headerB == NULL) || (dataB == NULL) || (ctx == NULL))
		{
		#line 646 /* the line number is part of the binary */
		PEMerr(PEM_F_PEM_READ_BIO,ERR_R_MALLOC_FAILURE);
		goto err;
		}

	buf[254]='\0';
	for (;;)
		{
		i=BIO_gets(bp,buf,254);

		if (i <= 0)
			{
			PEMerr(PEM_F_PEM_READ_BIO,PEM_R_NO_START_LINE);
			goto err;
			}

		while ((i >= 0) && (buf[i] <= ' ')) i--;
		buf[++i]='\n'; buf[++i]='\0';

		if (strncmp(buf,"-----BEGIN ",11) == 0)
			{
			i=strlen(&(buf[11]));

			if (strncmp(&(buf[11+i-6]),"-----\n",6) != 0)
				continue;
			if (!BUF_MEM_grow(nameB,i+9))
				{
				PEMerr(PEM_F_PEM_READ_BIO,ERR_R_MALLOC_FAILURE);
				goto err;
				}
			strncpy(nameB->data,&(buf[11]),(unsigned int)i-6);
			nameB->data[i-6]='\0';
			break;
			}
		}
	hl=0;
	if (!BUF_MEM_grow(headerB,256))
		{ PEMerr(PEM_F_PEM_READ_BIO,ERR_R_MALLOC_FAILURE); goto err; }
	headerB->data[0]='\0';
	for (;;)
		{
		i=BIO_gets(bp,buf,254);
		if (i <= 0) break;

		while ((i >= 0) && (buf[i] <= ' ')) i--;
		buf[++i]='\n'; buf[++i]='\0';

		if (buf[0] == '\n') break;
		if (!BUF_MEM_grow(headerB,hl+i+9))
			{ PEMerr(PEM_F_PEM_READ_BIO,ERR_R_MALLOC_FAILURE); goto err; }
		if (strncmp(buf,"-----END ",9) == 0)
			{
			nohead=1;
			break;
			}
		strncpy(&(headerB->data[hl]),buf,(unsigned int)i);
		headerB->data[hl+i]='\0';
		hl+=i;
		}

	bl=0;
	if (!BUF_MEM_grow(dataB,1024))
		{ PEMerr(PEM_F_PEM_READ_BIO,ERR_R_MALLOC_FAILURE); goto err; }
	dataB->data[0]='\0';
	if (!nohead)
		{
		for (;;)
			{
			i=BIO_gets(bp,buf,254);
			if (i <= 0) break;

			while ((i >= 0) && (buf[i] <= ' ')) i--;
			buf[++i]='\n'; buf[++i]='\0';

			if (i != 65) end=1;
			if (strncmp(buf,"-----END ",9) == 0)
				break;
			if (i > 65) break;
			if (!BUF_MEM_grow(dataB,i+bl+9))
				{
				PEMerr(PEM_F_PEM_READ_BIO,ERR_R_MALLOC_FAILURE);
				goto err;
				}
			strncpy(&(dataB->data[bl]),buf,(unsigned int)i);
			dataB->data[bl+i]='\0';
			bl+=i;
			if (end)
				{
				buf[0]='\0';
				i=BIO_gets(bp,buf,254);
				if (i <= 0) break;

				while ((i >= 0) && (buf[i] <= ' ')) i--;
				buf[++i]='\n'; buf[++i]='\0';

				break;
				}
			}
		}
	else
		{
		tmpB=headerB;
		headerB=dataB;
		dataB=tmpB;
		bl=hl;
		}
	i=strlen(nameB->data);
	if (	(strncmp(buf,"-----END ",9) != 0) ||
		(strncmp(nameB->data,&(buf[9]),(unsigned int)i) != 0) ||
		(strncmp(&(buf[9+i]),"-----\n",6) != 0))
		{
		PEMerr(PEM_F_PEM_READ_BIO,PEM_R_BAD_END_LINE);
		goto err;
		}

	EVP_DecodeInit(ctx);
	i=EVP_DecodeUpdate(ctx,
		(unsigned char *)dataB->data,&bl,
		(unsigned char *)dataB->data,bl);
	if (i < 0)
		{
		PEMerr(PEM_F_PEM_READ_BIO,PEM_R_BAD_BASE64_DECODE);
		goto err;
		}
	i=EVP_DecodeFinal(ctx,(unsigned char *)&(dataB->data[bl]),&k);
	if (i < 0)
		{
		PEMerr(PEM_F_PEM_READ_BIO,PEM_R_BAD_BASE64_DECODE);
		goto err;
		}
	bl+=k;

	if (bl == 0) goto err;
	*name=nameB->data;
	*header=headerB->data;
	*data=(unsigned char *)dataB->data;
	*len=bl;
	R_free(nameB);
	R_free(headerB);
	R_free(dataB);
	EVP_ENCODE_CTX_free(ctx);
	return(1);
err:
	/* NOTE: bug: the guard of the second free tests nameB instead of headerB (harmless:
	 * BUF_MEM_free checks for NULL). */
	if (ctx != NULL)
		EVP_ENCODE_CTX_free(ctx);
	if (nameB != NULL)
		BUF_MEM_free(nameB);
	if (nameB != NULL)
		BUF_MEM_free(headerB);
	if (dataB != NULL)
		BUF_MEM_free(dataB);
	return(0);
	}
