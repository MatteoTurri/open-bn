/*
 * RC2 in CBC mode and its two block functions.
 */
#include "rc2.h"
#include "rc2_locl.h"

void RC2_cbc_encrypt(in, out, length, ks, iv, encrypt)
unsigned char *in;
unsigned char *out;
long length;
RC2_KEY *ks;
unsigned char *iv;
int encrypt;
	{
	register unsigned long tin0,tin1;
	register unsigned long tout0,tout1,xor0,xor1;
	register long l=length;
	unsigned long tin[2];

	if (encrypt)
		{
		c2l(iv,tout0);
		c2l(iv,tout1);
		iv-=8;
		for (l-=8; l>=0; l-=8)
			{
			c2l(in,tin0);
			c2l(in,tin1);
			tin0^=tout0;
			tin1^=tout1;
			tin[0]=tin0;
			tin[1]=tin1;
			RC2_encrypt(tin,ks);
			tout0=tin[0]; l2c(tout0,out);
			tout1=tin[1]; l2c(tout1,out);
			}
		if (l != -8)
			{
			c2ln(in,tin0,tin1,l+8);
			tin0^=tout0;
			tin1^=tout1;
			tin[0]=tin0;
			tin[1]=tin1;
			RC2_encrypt(tin,ks);
			tout0=tin[0]; l2c(tout0,out);
			tout1=tin[1]; l2c(tout1,out);
			}
		l2c(tout0,iv);
		l2c(tout1,iv);
		}
	else
		{
		c2l(iv,xor0);
		c2l(iv,xor1);
		iv-=8;
		for (l-=8; l>=0; l-=8)
			{
			c2l(in,tin0); tin[0]=tin0;
			c2l(in,tin1); tin[1]=tin1;
			RC2_decrypt(tin,ks);
			tout0=tin[0]^xor0;
			tout1=tin[1]^xor1;
			l2c(tout0,out);
			l2c(tout1,out);
			xor0=tin0;
			xor1=tin1;
			}
		if (l != -8)
			{
			c2l(in,tin0); tin[0]=tin0;
			c2l(in,tin1); tin[1]=tin1;
			RC2_decrypt(tin,ks);
			tout0=tin[0]^xor0;
			tout1=tin[1]^xor1;
			l2cn(tout0,tout1,out,l+8);
			xor0=tin0;
			xor1=tin1;
			}
		l2c(xor0,iv);
		l2c(xor1,iv);
		}
	tin0=tin1=tout0=tout1=xor0=xor1=0;
	tin[0]=tin[1]=0;
	}


void RC2_encrypt(d,key)
unsigned long *d;
RC2_KEY *key;
	{
	RC2_INT x0,x1,x2,x3;
	RC2_INT *p0;
	unsigned long l;

	p0= &(key->data[0]);

	l=d[0];
	x0=l;
	x1=(RC2_INT)(l>>16L);
	l=d[1];
	x2=l;
	x3=(RC2_INT)(l>>16L);

	x0=x0+p0[0];
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[1];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[2];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[3];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[4];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[5];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[6];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[7];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[8];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[9];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[10];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[11];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[12];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[13];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[14];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[15];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[16];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[17];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[18];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[19];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0+=p0[x3&0x3f];
	x1+=p0[x0&0x3f];
	x2+=p0[x1&0x3f];
	x3+=p0[x2&0x3f];
	x0=x0+p0[20];
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[21];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[22];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[23];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[24];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[25];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[26];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[27];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[28];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[29];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[30];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[31];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[32];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[33];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[34];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[35];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[36];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[37];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[38];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[39];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[40];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[41];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[42];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[43];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0+=p0[x3&0x3f];
	x1+=p0[x0&0x3f];
	x2+=p0[x1&0x3f];
	x3+=p0[x2&0x3f];
	x0=x0+p0[44];
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[45];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[46];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[47];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[48];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[49];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[50];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[51];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[52];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[53];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[54];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[55];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[56];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[57];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[58];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[59];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x0=x0+p0[60];
	x3=(x3<<5)|((x3&0xffff)>>11);
	x0=(x3&x2)+(~x3&x1)+x0;
	x1=x1+p0[61];
	x0=(x0<<1)|((x0&0xffff)>>15);
	x1=(x0&x3)+(~x0&x2)+x1;
	x2=x2+p0[62];
	x1=(x1<<2)|((x1&0xffff)>>14);
	x2=(x1&x0)+(~x1&x3)+x2;
	x3=x3+p0[63];
	x2=(x2<<3)|((x2&0xffff)>>13);
	x3=(x2&x1)+(~x2&x0)+x3;
	x3=(x3<<5)|((x3&0xffff)>>11);

	d[0]=(x0&0xffff)|(x1<<16);
	d[1]=(x2&0xffff)|(x3<<16);
	}


void RC2_decrypt(d,key)
unsigned long *d;
RC2_KEY *key;
	{
	RC2_INT x0,x1,x2,x3;
	RC2_INT *p0;
	unsigned long l;

	p0= &(key->data[0]);

	l=d[0];
	x0=(RC2_INT)l&0xffff;
	x1=(RC2_INT)(l>>16L);
	l=d[1];
	x2=(RC2_INT)l&0xffff;
	x3=(RC2_INT)(l>>16L);

	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[63]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[62]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[61]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[60]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[59]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[58]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[57]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[56]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[55]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[54]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[53]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[52]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[51]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[50]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[49]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[48]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[47]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[46]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[45]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[44]-(x3&x2)-(~x3&x1);
	x3-=p0[x2&0x3f];
	x2-=p0[x1&0x3f];
	x1-=p0[x0&0x3f];
	x0-=p0[x3&0x3f];
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[43]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[42]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[41]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[40]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[39]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[38]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[37]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[36]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[35]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[34]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[33]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[32]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[31]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[30]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[29]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[28]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[27]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[26]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[25]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[24]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[23]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[22]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[21]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[20]-(x3&x2)-(~x3&x1);
	x3-=p0[x2&0x3f];
	x2-=p0[x1&0x3f];
	x1-=p0[x0&0x3f];
	x0-=p0[x3&0x3f];
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[19]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[18]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[17]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[16]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[15]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[14]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[13]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[12]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[11]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[10]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[9]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[8]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[7]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[6]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[5]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[4]-(x3&x2)-(~x3&x1);
	x3=(x3<<11)|((x3&0xffff)>>5);
	x3=x3-p0[3]-(x2&x1)-(~x2&x0);
	x2=(x2<<13)|((x2&0xffff)>>3);
	x2=x2-p0[2]-(x1&x0)-(~x1&x3);
	x1=(x1<<14)|((x1&0xffff)>>2);
	x1=x1-p0[1]-(x0&x3)-(~x0&x2);
	x0=(x0<<15)|((x0&0xffff)>>1);
	x0=x0-p0[0]-(x3&x2)-(~x3&x1);

	d[0]=(x0&0xffff)|(x1<<16);
	d[1]=(x2&0xffff)|(x3<<16);
	}
