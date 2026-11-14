/*
 * bn_sqr_comba4: the 4-word square of OpenSSL, column by column.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

void bn_sqr_comba4(r,a)
BN_ULONG *r,*a;
	{
	BN_ULLONG t,t2;
	BN_ULONG l,h;
	BN_ULONG c1,c2,c3;

	c1=0;
	c2=0;
	c3=0;
	sqr_add_c(a,0,c1,c2,c3);
	r[0]=c1;
	c1=0;
	sqr_add_c2(a,1,0,c2,c3,c1);
	r[1]=c2;
	c2=0;
	sqr_add_c(a,1,c3,c1,c2);
	sqr_add_c2(a,2,0,c3,c1,c2);
	r[2]=c3;
	c3=0;
	sqr_add_c2(a,3,0,c1,c2,c3);
	sqr_add_c2(a,2,1,c1,c2,c3);
	r[3]=c1;
	c1=0;
	sqr_add_c(a,2,c2,c3,c1);
	sqr_add_c2(a,3,1,c2,c3,c1);
	r[4]=c2;
	c2=0;
	sqr_add_c2(a,3,2,c3,c1,c2);
	r[5]=c3;
	c3=0;
	sqr_add_c(a,3,c1,c2,c3);
	r[6]=c1;
	r[7]=c2;
	}
