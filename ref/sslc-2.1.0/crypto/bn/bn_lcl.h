/* crypto/bn/bn_lcl.h */
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

#ifndef HEADER_BN_LCL_H
#define HEADER_BN_LCL_H

#include "bn.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*************************************************************
 * Using the long long type
 */
#define Lw(t)    (((BN_ULONG)(t))&BN_MASK2)
#define Hw(t)    (((BN_ULONG)((t)>>BN_BITS2))&BN_MASK2)

/* RCOM: does not compute &(a)->d[(a)->top-1] when top is 0. */
#define bn_fix_top(a) \
        { \
        BN_ULONG *fix_top_l; \
        if ((a)->top > 0) \
                { \
                fix_top_l= &((a)->d[(a)->top-1]); \
                while ((a)->top > 0) \
                        { \
                        if (*(fix_top_l--)) break; \
                        (a)->top--; \
                        } \
                } \
	}

/* #define bn_expand(n,b) ((((b)/BN_BITS2) <= (n)->max)?(n):bn_expand2((n),(b))) */

/* RCOM: the recursion table of bn_mul_rec_words/bn_sqr_rec_words, set up on the stack by
 * BN_mul/BN_sqr. */
struct bn_rec_dtab
	{
	int depth;	/* recursion levels left */
	int size;
	void (*mul8)();	/* bn_mul_comba8 */
	void (*sqr8)();	/* bn_sqr_comba8 */
	int reserved[4];
	};

#ifdef BN_LLONG
#define mul_add(r,a,w,c) { \
	BN_ULLONG t; \
	t=(BN_ULLONG)w * (a) + (r) + (c); \
	(r)= Lw(t); \
	(c)= Hw(t); \
	}

#define mul(r,a,w,c) { \
	BN_ULLONG t; \
	t=(BN_ULLONG)w * (a) + (c); \
	(r)= Lw(t); \
	(c)= Hw(t); \
	}

/* RCOM/later-OpenSSL addition (read off bn_mul_comba8/bn_sqr_comba8
 * in the binary): the comba-style multiply/square kernels use these two
 * extra macros, absent from stock SSLeay 0.9.0b's bn_lcl.h (stock has no
 * comba at all). `mul_add_c` is the plain per-term multiply-accumulate;
 * `mul_add_c2` is the doubled form used for off-diagonal square terms
 * (`a[i]*a[j]` counted twice) via `sqr_add_c`/`sqr_add_c2`. */
#define mul_add_c(a,b,c0,c1,c2) { \
	t=(BN_ULLONG)(a)*(b); \
	l=Lw(t); \
	h=Hw(t); \
	c0+=l; \
	if (c0<l) h++; \
	c1+=h; \
	if (c1<h) c2++; \
	}

#define mul_add_c2(a,b,c0,c1,c2) { \
	t=(BN_ULLONG)(a)*(b); \
	t2=t+t; \
	if (t2<t) c2++; \
	l=Lw(t2); \
	h=Hw(t2); \
	c0+=l; \
	if (c0<l) { \
		if (++h==0) c2++; \
	} \
	c1+=h; \
	if (c1<h) c2++; \
	}

#define sqr_add_c(a,i,c0,c1,c2) mul_add_c((a)[i],(a)[i],c0,c1,c2)
#define sqr_add_c2(a,i,j,c0,c1,c2) mul_add_c2((a)[i],(a)[j],c0,c1,c2)

#else
/*************************************************************
 * No long long type
 */

#define LBITS(a)	((a)&BN_MASK2l)
#define HBITS(a)	(((a)>>BN_BITS4)&BN_MASK2l)
#define	L2HBITS(a)	((BN_ULONG)((a)&BN_MASK2l)<<BN_BITS4)

#define LLBITS(a)	((a)&BN_MASKl)
#define LHBITS(a)	(((a)>>BN_BITS2)&BN_MASKl)
#define	LL2HBITS(a)	((BN_ULLONG)((a)&BN_MASKl)<<BN_BITS2)

#define mul64(l,h,bl,bh) \
	{ \
	BN_ULONG m,m1,lt,ht; \
 \
	lt=l; \
	ht=h; \
	m =(bh)*(lt); \
	lt=(bl)*(lt); \
	m1=(bl)*(ht); \
	ht =(bh)*(ht); \
	m=(m+m1)&BN_MASK2; if (m < m1) ht+=L2HBITS(1L); \
	ht+=HBITS(m); \
	m1=L2HBITS(m); \
	lt=(lt+m1)&BN_MASK2; if (lt < m1) ht++; \
	(l)=lt; \
	(h)=ht; \
	}

#define sqr64(lo,ho,in) \
	{ \
	BN_ULONG l,h,m; \
 \
	h=(in); \
	l=LBITS(h); \
	h=HBITS(h); \
	m =(l)*(h); \
	l*=l; \
	h*=h; \
	h+=(m&BN_MASK2h1)>>(BN_BITS4-1); \
	m =(m&BN_MASK2l)<<(BN_BITS4+1); \
	l=(l+m)&BN_MASK2; if (l < m) h++; \
	(lo)=l; \
	(ho)=h; \
	}

#define mul_add(r,a,bl,bh,c) { \
	BN_ULONG l,h; \
 \
	h= (a); \
	l=LBITS(h); \
	h=HBITS(h); \
	mul64(l,h,(bl),(bh)); \
 \
	/* non-multiply part */ \
	l=(l+(c))&BN_MASK2; if (l < (c)) h++; \
	(c)=(r); \
	l=(l+(c))&BN_MASK2; if (l < (c)) h++; \
	(c)=h&BN_MASK2; \
	(r)=l; \
	}

#define mul(r,a,bl,bh,c) { \
	BN_ULONG l,h; \
 \
	h= (a); \
	l=LBITS(h); \
	h=HBITS(h); \
	mul64(l,h,(bl),(bh)); \
 \
	/* non-multiply part */ \
	l+=(c); if ((l&BN_MASK2) < (c)) h++; \
	(c)=h&BN_MASK2; \
	(r)=l&BN_MASK2; \
	}

#endif

#ifndef NOPROTO

BIGNUM *bn_expand2(BIGNUM *b, int bits);

#ifdef X86_ASM
void bn_add_words(BN_ULONG *r,BN_ULONG *a,int num);
#endif

/* RCOM: the multiplication kernels of later OpenSSL and the fork's
 * recursive and word-size Montgomery ones. */
void bn_mul_normal(BN_ULONG *r,BN_ULONG *a,int na,BN_ULONG *b,int nb);
void bn_sqr_normal(BN_ULONG *r, BN_ULONG *a, int n, BN_ULONG *tmp);
void bn_mul_comba8(BN_ULONG *r,BN_ULONG *a,BN_ULONG *b);
void bn_sqr_comba8(BN_ULONG *r,BN_ULONG *a);
void bn_sqr_comba4(BN_ULONG *r,BN_ULONG *a);
void bn_mul_rec_words(BN_ULONG *r,BN_ULONG *a,BN_ULONG *b,BN_ULONG *t,
	struct bn_rec_dtab *dt);
void bn_sqr_rec_words(BN_ULONG *r,BN_ULONG *a,BN_ULONG *t,
	struct bn_rec_dtab *dt);
void bn_from_montgomery_words(BN_ULONG *rp,BN_ULONG *r,BN_ULONG *np,int nl,
	BN_ULONG n0);
void bn_2s_comp(BN_ULONG *r,BN_ULONG *a,int n);

#else

BIGNUM *bn_expand2();
#ifdef X86_ASM
BN_ULONG bn_add_words();
#endif

void bn_mul_normal();
void bn_sqr_normal();
void bn_mul_comba8();
void bn_sqr_comba8();
void bn_sqr_comba4();
void bn_mul_rec_words();
void bn_sqr_rec_words();
void bn_from_montgomery_words();
void bn_2s_comp();

#endif

#ifdef  __cplusplus
}
#endif

#endif
