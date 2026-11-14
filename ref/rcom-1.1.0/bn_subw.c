/*
 * bn_sub_words, an RCOM addition: subtracts two word arrays with borrow, unrolled by 4; returns the
 * borrow at once when n <= 0.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

BN_ULONG bn_sub_words(r,a,b,n)
BN_ULONG *r,*a,*b;
int n;
	{
	int borrow;
	BN_ULONG t1,t2;

	if (n <= 0) return(0);
	borrow=0;
	for (;;)
		{
		t1=a[0];
		t2=b[0];
		r[0]=(t1-t2-borrow)&BN_MASK2;
		if (t1 != t2) borrow=(t1<t2);
		if (--n <= 0) break;

		t1=a[1];
		t2=b[1];
		r[1]=(t1-t2-borrow)&BN_MASK2;
		if (t1 != t2) borrow=(t1<t2);
		if (--n <= 0) break;

		t1=a[2];
		t2=b[2];
		r[2]=(t1-t2-borrow)&BN_MASK2;
		if (t1 != t2) borrow=(t1<t2);
		if (--n <= 0) break;

		t1=a[3];
		t2=b[3];
		r[3]=(t1-t2-borrow)&BN_MASK2;
		if (t1 != t2) borrow=(t1<t2);
		if (--n <= 0) break;

		a+=4;
		b+=4;
		r+=4;
		}
	return(borrow);
	}
