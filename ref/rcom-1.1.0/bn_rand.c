/*
 * BN_rand of RCOM: the random bytes come from R_rand_bytes on the random object rng, the buffer has
 * one more byte, buf[1] is set only when there are two bytes, and a failed allocation reports no
 * error.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"
#include "r_rand.h"


int BN_rand(rnd, rd, bits, top, bottom)
BIGNUM *rnd;
void *rd;
int bits;
int top;
int bottom;
	{
	unsigned char *buf=NULL;
	int ret=0,bit,bytes,mask;

	bytes=(bits+7)/8;
	bit=(bits-1)%8;
	mask=0xff<<bit;

	buf=(unsigned char *)R_malloc(bytes+1);
	if (buf == NULL)
		goto err;

	R_rand_bytes(rd,buf,bytes+1);
	if (top)
		{
		if (bit == 0)
			{
			buf[0]=1;
			if (bytes >= 2)
				buf[1]|=0x80;
			}
		else
			{
			buf[0]|=(3<<(bit-1));
			buf[0]&= ~(mask<<1);
			}
		}
	else
		{
		buf[0]|=(1<<bit);
		buf[0]&= ~(mask<<1);
		}
	if (bottom)
		buf[bytes-1]|=1;
	if (BN_bin2bn(buf,bytes,rnd) == NULL)
		goto err;
	ret=1;
err:
	if (buf != NULL)
		{
		memset(buf,0,bytes);
		R_free(buf);
		}
	return(ret);
	}
