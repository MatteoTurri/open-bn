/* crypto/evp/names.c */
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
#include "evp.h"
#include "objects.h"

/* SSL-C: the EVP_OBJ_* functions, on OBJ_NAME_* (o_names.c): OBJ_NAME_TYPE_CIPHER_METH 2,
 * OBJ_NAME_TYPE_MD_METH 1. */
int EVP_OBJ_add_cipher(c)
EVP_CIPHER *c;
	{
	int ret;
	int nid;

	nid=EVP_CIPHER_nid(c);
	ret=OBJ_NAME_add(OBJ_nid2sn(nid),OBJ_NAME_TYPE_CIPHER_METH,(char *)c);
	if (!ret) return(0);
	ret=OBJ_NAME_add(OBJ_nid2ln(nid),OBJ_NAME_TYPE_CIPHER_METH,(char *)c);
	return(ret);
	}

int EVP_OBJ_add_digest(md)
EVP_MD *md;
	{
	int ret;
	char *sn;
	int type;
	int pkey_type;

	type=EVP_MD_type(md);
	sn=OBJ_nid2sn(type);
	ret=OBJ_NAME_add(sn,OBJ_NAME_TYPE_MD_METH,(char *)md);
	if (!ret) return(0);
	ret=OBJ_NAME_add(OBJ_nid2ln(type),OBJ_NAME_TYPE_MD_METH,(char *)md);
	if (!ret) return(0);
	pkey_type=EVP_MD_pkey_type(md);
	if ((type != pkey_type) && (pkey_type != 0))
		{
		ret=OBJ_NAME_add(OBJ_nid2sn(pkey_type),
			OBJ_NAME_ALIAS|OBJ_NAME_TYPE_MD_METH,sn);
		if (!ret) return(0);
		ret=OBJ_NAME_add(OBJ_nid2ln(pkey_type),
			OBJ_NAME_ALIAS|OBJ_NAME_TYPE_MD_METH,sn);
		}
	return(ret);
	}

EVP_CIPHER *EVP_OBJ_get_cipherbyname(name)
char *name;
	{
	EVP_CIPHER *ret;

	ret=(EVP_CIPHER *)OBJ_NAME_get(name,OBJ_NAME_TYPE_CIPHER_METH);
	return(ret);
	}

EVP_MD *EVP_OBJ_get_digestbyname(name)
char *name;
	{
	EVP_MD *ret;

	ret=(EVP_MD *)OBJ_NAME_get(name,OBJ_NAME_TYPE_MD_METH);
	return(ret);
	}

void EVP_OBJ_cleanup()
	{
	OBJ_NAME_cleanup(OBJ_NAME_TYPE_CIPHER_METH);
	OBJ_NAME_cleanup(OBJ_NAME_TYPE_MD_METH);
	return;
	}

int EVP_OBJ_add_cipher_alias(to,from)
char *to;
char *from;
	{
	int ret;

	ret=OBJ_NAME_add(from,OBJ_NAME_ALIAS|OBJ_NAME_TYPE_CIPHER_METH,to);
	return(ret);
	}

int EVP_OBJ_add_digest_alias(to,from)
char *to;
char *from;
	{
	int ret;

	ret=OBJ_NAME_add(from,OBJ_NAME_ALIAS|OBJ_NAME_TYPE_MD_METH,to);
	return(ret);
	}

int EVP_OBJ_delete_cipher_alias(name)
char *name;
	{
	int ret;

	ret=OBJ_NAME_remove(name,OBJ_NAME_ALIAS|OBJ_NAME_TYPE_CIPHER_METH);
	return(ret);
	}

int EVP_OBJ_delete_digest_alias(name)
char *name;
	{
	int ret;

	ret=OBJ_NAME_remove(name,OBJ_NAME_ALIAS|OBJ_NAME_TYPE_MD_METH);
	return(ret);
	}
