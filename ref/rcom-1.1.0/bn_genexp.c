/*
 * The exponent programs of the word method: BN_gen_exp_bits serializes an exponent into a header
 * [lenHi, lenLo, windowBits, tableSize] and the windows of BN_gen_exp_string. The public exponents 3, 17 and
 * 65537 use precomputed tables.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"
#include "bn_me.h"

static const unsigned char g_exp_tab_e3[12] =
	{0x00,0x08,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00};
static const unsigned char g_exp_tab_e17[12] =
	{0x00,0x08,0x01,0x01,0x01,0x04,0x01,0x00,0x00,0x00,0x00,0x00};
static const unsigned char g_exp_tab_e65537[12] =
	{0x00,0x08,0x01,0x01,0x01,0x10,0x01,0x00,0x00,0x00,0x00,0x00};


int BN_gen_exp_bits(e,out,flags,ctx)
BIGNUM *e;
unsigned char **out;
int flags;
BN_CTX *ctx;
	{
	int nbits;
	int n;
	int nbytes;
	int windowBits,tableSize;
	unsigned char *buf;
	BIGNUM *tmp;

	buf=0;

	nbits=e->top*32;

	tmp= &(ctx->bn[ctx->tos]);

	if (e->top == 0)
		return(0);

	if (e->top == 1) {
		if (e->top == 1 && e->d[0] == 0x10001) {
			buf=(unsigned char *)g_exp_tab_e65537;
		} else if (e->d[0] == 17 && e->top == 1) {
			buf=(unsigned char *)g_exp_tab_e17;
		} else if (e->d[0] == 3 && e->top == 1) {
			buf=(unsigned char *)g_exp_tab_e3;
		}
		windowBits=1;
		tableSize=1;
		n=32;
	} else {
		if (nbits >= 256) {
			windowBits=5;
			tableSize=16;
			n=7;
		} else {
			if (nbits >= 128) {
				windowBits=4;
				tableSize=8;
				n=8;
			} else {
				windowBits=3;
				tableSize=4;
				n=11;
			}
		}
	}

	nbytes=(e->top*n+3)/2;

	if (buf == 0) {
		bn_wexpand(tmp,nbytes);
		buf=(unsigned char *)tmp->d;
		n=BN_gen_exp_string(buf+4,e,windowBits);
		n=n+2;
		buf[0]=(n>>8)&0xff;
		buf[1]=n&0xff;
		buf[2]=windowBits;
		buf[3]=tableSize;
	} else {
		n=8;
	}

	*out=buf;
	return(n+2);
	}

/* The trailing zeros of i, with b[0] = 6. The table of window size m is the suffix of 2^m bytes,
 * whose first byte is m. */
static const unsigned char g_gen_exp_ntz_tab[64] =
	{
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
	};

static const unsigned char *g_gen_exp_mode_tab[7] =
	{
	&g_gen_exp_ntz_tab[63],
	&g_gen_exp_ntz_tab[62],
	&g_gen_exp_ntz_tab[60],
	&g_gen_exp_ntz_tab[56],
	&g_gen_exp_ntz_tab[48],
	&g_gen_exp_ntz_tab[32],
	&g_gen_exp_ntz_tab[0]
	};

int BN_gen_exp_string(buf,e,mode)
unsigned char *buf;
BIGNUM *e;
int mode;
	{
	unsigned char *p;
	BN_ULONG mask;
	const unsigned char *tbl;
	BN_ULONG cur,next,w;
	BN_ULONG *dp;
	int retlen;
	unsigned int k;
	unsigned int acc;
	unsigned int idx;
	int nbits;
	unsigned int c;
	unsigned int bits;
	int n;

	if (mode > 6)
		mode=6;
	tbl=g_gen_exp_mode_tab[mode];
	mask=(1 << mode)-1;
	bits=0;
	nbits=e->top*32;
	p=buf+(((e->top*32+mode-1)/mode)*2+2);
	*p-- =0;
	*p-- =0;
	n=e->top;
	dp=e->d;
	cur=w= *dp++;
	if (n < 2)
		next=0;
	else
		next= *dp++;
	acc=0;
	retlen=0;

	for (;;)
		{
		for (;;)
			{
			idx=cur & mask;
			c=tbl[idx];
			if (c == 0)
				break;
			acc+=c;
			if (acc >= 256)
				{
				*p-- =acc;
				acc-=256;
				*p-- =0;
				}
			bits+=c;
			if (bits >= 32)
				{
				if (n < 2)
					break;
				n--;
				w=next;
				next=(n >= 2)? *dp++ : 0;
				bits-=32;
				}
			if (bits == 0)
				cur=w;
			else
				cur=(w >> bits) | (next << (32-bits));
			}
		k=idx;
		if (idx == 0)
			break;
		*p-- =acc;
		acc=mode;
		*p-- =k;
		bits+=mode;
		if (bits >= 32)
			{
			if (n < 2)
				break;
			n--;
			w=next;
			next=(n >= 2)? *dp++ : 0;
			bits-=32;
			}
		if (bits == 0)
			cur=w;
		else
			cur=(w >> bits) | (next << (32-bits));
		}
	p++;
	retlen=2;
	while (*p != 0)
		{
		buf[0]=p[0];
		buf[1]=p[1];
		buf+=2;
		p+=2;
		retlen+=2;
		}
	buf[0]=0;
	buf[1]=0;
	return(retlen);
	}
