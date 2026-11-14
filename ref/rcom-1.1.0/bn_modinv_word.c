/*
 * BN_mod_inverse_word: the inverse of the word base 2^32 modulo n (the n0 of Montgomery) with the
 * extended Euclidean algorithm on registers, starting from q = floor(2^32 / n) and r = 2^32 mod n
 * computed from -n. The sign is tracked apart. Returns 0 when n is not invertible.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

BN_ULONG BN_mod_inverse_word(n)
BN_ULONG n;
	{
	BN_ULONG a,b,k1,k0,r,q,ret,t;
	int sign;

	k1=0;
	k0=1;
	a=0;
	b=n;
	sign=1;

	t= -b;
	q=t/b+1;
	r=t%b;

	t=a;
	a=b;
	b=r;

	t=k0;
	r=k0;
	k0=k1;
	k1=t;

	sign= -sign;

	while (b != 0)
		{
		q=a/b;
		r=a%b;
		t=q*k1+k0;
		k0=k1;
		k1=t;
		a=b;
		b=r;
		sign= -sign;
		}

	if (sign < 0)
		k0=n-k0;

	if (a == 1)
		ret=k0%n;
	else
		ret=0;

	return(ret);
	}
