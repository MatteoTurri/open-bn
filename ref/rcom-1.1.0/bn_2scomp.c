/*
 * bn_2s_comp: the two's complement of a word array, an RCOM addition to the big numbers: the first
 * words are negated up to the first nonzero result, the words after it are complemented.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

void bn_2s_comp(r,a,n)
BN_ULONG *r,*a;
int n;
	{
	BN_ULONG t;
	int i;

	for (i=0; i<n; i++)
		{
		t = a[i];
		t = ~t+1;
		r[i] = t;
		if (t != 0) break;
		}
	i++;
	for (; i<n; i++)
		r[i] = ~a[i];
	}
