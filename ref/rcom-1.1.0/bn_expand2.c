/*
 * bn_expand2.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

/* RCOM: checks the stack with R_DIAG_check_stack, refuses BIGNUMs with BN_FLG_STATIC_DATA, and
 * allocates a new array, copying the old words and freeing the old one, instead of reallocating. */

BIGNUM *bn_expand2(b, words)
BIGNUM *b;
int words;
	{
	static int bn_expand2_diag_count=0;
	BN_ULONG *A,*B,*a;
	char marker;
	int i;
	BN_ULONG t1,t2,t3,t4;

	marker=0;
	R_DIAG_check_stack((unsigned int)&marker,"./bn/../bn/bn_lib.c",335,++bn_expand2_diag_count);

	if (words > b->max)
		{
		if (b->flags & BN_FLG_STATIC_DATA)
			return(NULL);

		a=A=(BN_ULONG *)R_malloc(sizeof(BN_ULONG)*(words+1));
		if (A == NULL)
			return(NULL);

		B=b->d;
		if (B != NULL)
			{
			for (i=b->top>>3; i>0; i--)
				{
				t1=B[0];
				t2=B[1];
				t3=B[2];
				t4=B[3];
				A[0]=t1;
				A[1]=t2;
				A[2]=t3;
				A[3]=t4;
				t1=B[4];
				t2=B[5];
				t3=B[6];
				t4=B[7];
				A[4]=t1;
				A[5]=t2;
				A[6]=t3;
				A[7]=t4;
				A+=8;
				B+=8;
				}
			switch (b->top&7)
				{
			case 7:
				A[6]=B[6];
			case 6:
				A[5]=B[5];
			case 5:
				A[4]=B[4];
			case 4:
				A[3]=B[3];
			case 3:
				A[2]=B[2];
			case 2:
				A[1]=B[1];
			case 1:
				A[0]=B[0];
			case 0:
				;
				}
			R_free(b->d);
			}
		b->d=a;
		b->max=words;
		}
	return(b);
	}
