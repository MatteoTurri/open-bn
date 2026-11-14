/*
 * The word kernels of the RCOM big numbers: bn_mul_add_words, bn_mul_words, bn_sqr_words,
 * bn_add_words (the BN_LLONG forms, unrolled by 4).
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

/* RCOM: the same scalar carry as bn_mul_words, with rp[k] added to the carry before the multiply-add. */
BN_ULONG bn_mul_add_words(rp,ap,num,w)
BN_ULONG *rp,*ap;
int num;
BN_ULONG w;
	{
	BN_ULONG c;
	BN_ULLONG ll;

	c=0;
	if (num <= 0) return(c);
	for (;;)
		{
		ll=(BN_ULLONG)w*ap[0]+rp[0]+c;
		rp[0]=Lw(ll);
		c=Hw(ll);
		if (--num == 0) break;

		ll=(BN_ULLONG)w*ap[1]+rp[1]+c;
		rp[1]=Lw(ll);
		c=Hw(ll);
		if (--num == 0) break;

		ll=(BN_ULLONG)w*ap[2]+rp[2]+c;
		rp[2]=Lw(ll);
		c=Hw(ll);
		if (--num == 0) break;

		ll=(BN_ULLONG)w*ap[3]+rp[3]+c;
		rp[3]=Lw(ll);
		c=Hw(ll);
		if (--num == 0) break;

		ap+=4;
		rp+=4;
		}
	return(c);
	}

/* RCOM: a scalar 32-bit carry, recomputed from the 64-bit product at each step. */
BN_ULONG bn_mul_words(rp,ap,num,w)
BN_ULONG *rp,*ap;
int num;
BN_ULONG w;
	{
	BN_ULONG c;
	BN_ULLONG ll;

	c=0;
	if (num <= 0) return(c);
	for (;;)
		{
		ll=(BN_ULLONG)w*ap[0]+c;
		rp[0]=Lw(ll);
		c=Hw(ll);
		if (--num == 0) break;

		ll=(BN_ULLONG)w*ap[1]+c;
		rp[1]=Lw(ll);
		c=Hw(ll);
		if (--num == 0) break;

		ll=(BN_ULLONG)w*ap[2]+c;
		rp[2]=Lw(ll);
		c=Hw(ll);
		if (--num == 0) break;

		ll=(BN_ULLONG)w*ap[3]+c;
		rp[3]=Lw(ll);
		c=Hw(ll);
		if (--num == 0) break;

		ap+=4;
		rp+=4;
		}
	return(c);
	}

/* RCOM: returns at once when n <= 0. */
void bn_sqr_words(r,a,n)
BN_ULONG *r,*a;
int n;
        {
	if (n <= 0) return;

	for (;;)
		{
		BN_ULLONG t;

		t=(BN_ULLONG)(a[0])*(a[0]);
		r[0]=Lw(t); r[1]=Hw(t);
		if (--n == 0) break;

		t=(BN_ULLONG)(a[1])*(a[1]);
		r[2]=Lw(t); r[3]=Hw(t);
		if (--n == 0) break;

		t=(BN_ULLONG)(a[2])*(a[2]);
		r[4]=Lw(t); r[5]=Hw(t);
		if (--n == 0) break;

		t=(BN_ULLONG)(a[3])*(a[3]);
		r[6]=Lw(t); r[7]=Hw(t);
		if (--n == 0) break;

		a+=4;
		r+=8;
		}
	}

/* RCOM: returns 0 at once when n <= 0 (SSLeay always runs the loop once). */
BN_ULONG bn_add_words(r,a,b,n)
BN_ULONG *r,*a,*b;
int n;
        {
	BN_ULLONG ll;

	ll=0;
	if (n <= 0) return(0);

	for (;;)
		{
		ll+= (BN_ULLONG)a[0]+b[0];
		r[0]=(BN_ULONG)ll&BN_MASK2;
		ll>>=BN_BITS2;
		if (--n <= 0) break;

		ll+= (BN_ULLONG)a[1]+b[1];
		r[1]=(BN_ULONG)ll&BN_MASK2;
		ll>>=BN_BITS2;
		if (--n <= 0) break;

		ll+= (BN_ULLONG)a[2]+b[2];
		r[2]=(BN_ULONG)ll&BN_MASK2;
		ll>>=BN_BITS2;
		if (--n <= 0) break;

		ll+= (BN_ULLONG)a[3]+b[3];
		r[3]=(BN_ULONG)ll&BN_MASK2;
		ll>>=BN_BITS2;
		if (--n <= 0) break;

		a+=4;
		b+=4;
		r+=4;
		}
	return(ll&BN_MASK2);
	}
