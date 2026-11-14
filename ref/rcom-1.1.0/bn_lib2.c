/*
 * BN_copy, BN_clear and the word setters of the RCOM big numbers.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

/* RCOM: the copy unrolled by 8 goes through four temporaries, and d[0] is cleared only when d is
 * allocated. */
BIGNUM *BN_copy(a, b)
BIGNUM *a;
BIGNUM *b;
	{
	int i;
	BN_ULONG *A,*B;
	BN_ULONG t1,t2,t3,t4;

	if (a == b) return(a);
	if (bn_wexpand(a,b->top) == NULL) return(NULL);

	A=a->d;
	B=b->d;
	i=b->top>>3;
	while (i > 0)
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
		i--;
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

	a->top=b->top;
	if ((a->top == 0) && (a->d != NULL))
		a->d[0]=0;
	a->neg=b->neg;
	return(a);
	}
