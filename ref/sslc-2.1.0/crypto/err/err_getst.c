/*
 * pid_hash, pid_cmp and ERR_get_state, from crypto/err/err.c of SSLeay.
 */
/* crypto/err/err.c */
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
#include "crypto.h"
#include "cryptlib.h"
#include "buffer.h"
#include "err.h"
#include "err_locl.h"
#include "crypto.h"


#ifndef NOPROTO
static unsigned long pid_hash(ERR_STATE *pid);
static int pid_cmp(ERR_STATE *a,ERR_STATE *pid);
#else
static unsigned long pid_hash();
static int pid_cmp();
#endif

static unsigned long pid_hash(a)
ERR_STATE *a;
	{
	return(a->pid*13);
	}

static int pid_cmp(a,b)
ERR_STATE *a,*b;
	{
	return((int)((long)a->pid - (long)b->pid));
	}

ERR_STATE *ERR_get_state()
	{
	static ERR_STATE fallback;
	ERR_STATE *ret=NULL,tmp,*tmpp;
	int i;
	unsigned long pid;

	pid=(unsigned long)R_thread_id();

#line 583 /* the line number is part of the binary */
	CRYPTO_r_lock(CRYPTO_LOCK_ERR);
	if (err_thread_hash == NULL)
		{
		CRYPTO_r_unlock(CRYPTO_LOCK_ERR);
		CRYPTO_w_lock(CRYPTO_LOCK_ERR);
		if (err_thread_hash == NULL)
			{
			err_thread_hash=lh_new(pid_hash,pid_cmp);
			CRYPTO_w_unlock(CRYPTO_LOCK_ERR);
			if (err_thread_hash == NULL) return(&fallback);
			}
		else
			CRYPTO_w_unlock(CRYPTO_LOCK_ERR);
		}
	else
		{
		tmp.pid=pid;
		ret=(ERR_STATE *)lh_retrieve(err_thread_hash,(char *)&tmp);
		CRYPTO_r_unlock(CRYPTO_LOCK_ERR);
		}

	/* ret == the error state, if NULL, make a new one */
	if (ret == NULL)
		{
		ret=(ERR_STATE *)Malloc(sizeof(ERR_STATE));
		if (ret == NULL) return(&fallback);
		ret->pid=pid;
		ret->top=0;
		ret->bottom=0;
		for (i=0; i<ERR_NUM_ERRORS; i++)
			{
			ret->err_data[i]=NULL;
			ret->err_data_flags[i]=0;
			}
#line 623 /* the line number is part of the binary */
		CRYPTO_w_lock(CRYPTO_LOCK_ERR);
		tmpp=(ERR_STATE *)lh_insert(err_thread_hash,(char *)ret);
		CRYPTO_w_unlock(CRYPTO_LOCK_ERR);
		if (tmpp != NULL) /* old entry - should not happen */
			{
			ERR_STATE_free(tmpp);
			}
		}
	return(ret);
	}
