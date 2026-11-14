/*
 * bn_from_montgomery_words: the Montgomery reduction of RCOM on word arrays. r (2 * nl words, a * b)
 * is reduced in place with the modulus np and n0, carrying one word across the steps; the result goes
 * to rp, minus np when it is not smaller.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

void bn_from_montgomery_words(rp, r, np, nl, n0)
BN_ULONG *rp,*r,*np;
int nl;
BN_ULONG n0;
	{
	BN_ULONG v;
	BN_ULONG c;
	BN_ULONG *ap,*bp;
	int i;

	c=0;
	ap=r;
	bp=&(r[nl]);
	i=0;
	while (i<nl)
		{
		v=bn_mul_add_words(ap,np,nl,(*ap*n0));
		v+=c;
		c=(v<c);
		if ((*bp+=v) < v) c++;
		ap++;
		bp++;
		i++;
		}

	i=nl-1;
	if (!c)
		{
		if (ap[i] == np[i])
			{
			while (i > 0)
				{
				if (ap[i] != np[i]) break;
				i--;
				}
			}
		c=(ap[i]>=np[i]);
		}

	if (c)
		bn_sub_words(rp,ap,np,nl);
	else
		{
		for (i=0; i<nl; i++)
			rp[i]=ap[i];
		}
	}
