/*
 * bn_mul_normal: the schoolbook product of OpenSSL (the longer operand first, then one
 * bn_mul_add_words per word of the shorter one, unrolled by 4).
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

void bn_mul_normal(r, a, na, b, nb)
BN_ULONG *r,*a,*b;
int na,nb;
	{
	BN_ULONG *rr;

	if (na < nb)
		{
		int itmp;
		BN_ULONG *ltmp;

		itmp=na; na=nb; nb=itmp;
		ltmp=a; a=b; b=ltmp;
		}
	rr= &(r[na]);
	rr[0]=bn_mul_words(r,a,na,b[0]);

	for (;;)
		{
		if (--nb <= 0) return;
		rr[1]=bn_mul_add_words(&(r[1]),a,na,b[1]);
		if (--nb <= 0) return;
		rr[2]=bn_mul_add_words(&(r[2]),a,na,b[2]);
		if (--nb <= 0) return;
		rr[3]=bn_mul_add_words(&(r[3]),a,na,b[3]);
		if (--nb <= 0) return;
		rr[4]=bn_mul_add_words(&(r[4]),a,na,b[4]);

		rr += 4;
		r += 4;
		b += 4;
		}
	}
