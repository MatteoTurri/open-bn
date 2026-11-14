/* crypto/asn1/a_bytes.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 * See crypto/README for full SSLeay licensing terms.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "asn1_mac.h"

/* ASN1err(ASN1_F_ASN1_TYPE_NEW,ASN1_R_ERROR_STACK);
 * ASN1err(ASN1_F_D2I_ASN1_TYPE_BYTES,ASN1_R_ERROR_STACK);
 * ASN1err(ASN1_F_D2I_ASN1_TYPE_BYTES,ASN1_R_WRONG_TYPE);
 * ASN1err(ASN1_F_ASN1_COLLATE_PRIMATIVE,ASN1_R_WRONG_TAG);
 */

static unsigned long tag2bit[32]={
0,	0,	0,	B_ASN1_BIT_STRING,	/* tags  0 -  3 */
B_ASN1_OCTET_STRING,	0,	0,		B_ASN1_UNKNOWN,/* tags  4- 7 */
B_ASN1_UNKNOWN,	B_ASN1_UNKNOWN,	B_ASN1_UNKNOWN,	B_ASN1_UNKNOWN,/* tags  8-11 */
B_ASN1_UNKNOWN,	B_ASN1_UNKNOWN,	B_ASN1_UNKNOWN,	B_ASN1_UNKNOWN,/* tags 12-15 */
0,	0,	B_ASN1_NUMERICSTRING,B_ASN1_PRINTABLESTRING,
B_ASN1_T61STRING,B_ASN1_VIDEOTEXSTRING,B_ASN1_IA5STRING,0,
0,B_ASN1_GRAPHICSTRING,B_ASN1_ISO64STRING,B_ASN1_GENERALSTRING,
B_ASN1_UNIVERSALSTRING,B_ASN1_UNKNOWN,B_ASN1_BMPSTRING,B_ASN1_UNKNOWN,
	};

#ifndef NOPROTO
static int asn1_collate_primative(ASN1_STRING *a, ASN1_CTX *c);
#else
static int asn1_collate_primative();
#endif

/* type is a 'bitmap' of acceptable string types to be accepted.
 */
#line 37 /* the line number is part of the binary */
ASN1_STRING *d2i_ASN1_type_bytes(a, pp, length, type)
ASN1_STRING **a;
unsigned char **pp;
long length;
int type;
	{
	ASN1_STRING *ret=NULL;
	unsigned char *p,*s;
	long len;
	int inf,tag,xclass;
	int i=0;

	p= *pp;
	inf=ASN1_get_object(&p,&len,&tag,&xclass,length);
	if (inf & 0x80) goto err;

	if (tag >= 32)
		{
		i=ASN1_R_TAG_VALUE_TOO_HIGH;;
		goto err;
		}
	if (!(tag2bit[tag] & type))
		{
		i=ASN1_R_WRONG_TYPE;
		goto err;
		}

	/* If a bit-string, exit early */
	if (tag == V_ASN1_BIT_STRING)
		return(d2i_ASN1_BIT_STRING(a,pp,length));

	if ((a == NULL) || ((*a) == NULL))
		{
		if ((ret=ASN1_STRING_new()) == NULL) return(NULL);
		}
	else
		ret=(*a);

	if (len != 0)
		{
		s=(unsigned char *)Malloc((int)len+1);
		if (s == NULL)
			{
			i=ERR_R_MALLOC_FAILURE;
			goto err;
			}
		memcpy(s,p,(int)len);
		s[len]='\0';
		p+=len;
		}
	else
		s=NULL;

	if (ret->data != NULL) Free((char *)ret->data);
	ret->length=(int)len;
	ret->data=s;
	ret->type=tag;
	if (a != NULL) (*a)=ret;
	*pp=p;
	return(ret);
err:
	ASN1err(ASN1_F_D2I_ASN1_TYPE_BYTES,i);
	if ((ret != NULL) && ((a == NULL) || (*a != ret)))
		ASN1_STRING_free(ret);
	return(NULL);
	}

int i2d_ASN1_bytes(a, pp, tag, xclass)
ASN1_STRING *a;
unsigned char **pp;
int tag;
int xclass;
	{
	int ret,r,constructed;
	unsigned char *p;

	if (a == NULL)  return(0);

	if (tag == V_ASN1_BIT_STRING)
		return(i2d_ASN1_BIT_STRING(a,pp));
		
	ret=a->length;
	r=ASN1_object_size(0,ret,tag);
	if (pp == NULL) return(r);
	p= *pp;

	if ((tag == V_ASN1_SEQUENCE) || (tag == V_ASN1_SET))
		constructed=1;
	else
		constructed=0;
	ASN1_put_object(&p,constructed,ret,tag,xclass);
	memcpy(p,a->data,a->length);
	p+=a->length;
	*pp= p;
	return(r);
	}

#line 135 /* the line number is part of the binary */
ASN1_STRING *d2i_ASN1_bytes(a, pp, length, Ptag, Pclass)
ASN1_STRING **a;
unsigned char **pp;
long length;
int Ptag;
int Pclass;
	{
	ASN1_STRING *ret=NULL;
	unsigned char *p,*s;
	long len;
	int inf,tag,xclass;
	int i=0;

	if ((a == NULL) || ((*a) == NULL))
		{
		if ((ret=ASN1_STRING_new()) == NULL) return(NULL);
		}
	else
		ret=(*a);

	p= *pp;
	inf=ASN1_get_object(&p,&len,&tag,&xclass,length);
	if (inf & 0x80)
		{
		i=ASN1_R_BAD_OBJECT_HEADER;
		goto err;
		}

	if (tag != Ptag)
		{
		i=ASN1_R_WRONG_TAG;
		goto err;
		}

	if (inf & V_ASN1_CONSTRUCTED)
		{
		ASN1_CTX c;

		c.pp=pp;
		c.p=p;
		c.inf=inf;
		c.slen=len;
		c.tag=Ptag;
		c.xclass=Pclass;
		c.max=(length == 0)?0:(p+length);
		if (!asn1_collate_primative(ret,&c)) 
			goto err; 
		else
			{
			p=c.p;
			}
		}
	else
		{
		if (len != 0)
			{
			if ((ret->length < len+1) || (ret->data == NULL))
				{
				if (ret->data != NULL) Free((char *)ret->data);
				s=(unsigned char *)Malloc((int)len+1);
				if (s == NULL)
					{
					i=ERR_R_MALLOC_FAILURE;
					goto err;
					}
				}
			else
				s=ret->data;
			memcpy(s,p,(int)len);
			/* an alowance for strings :-) */
			s[len]='\0';
			p+=len;
			}
		else
			{
			s=NULL;
			if (ret->data != NULL) Free((char *)ret->data);
			}

		ret->length=(int)len;
		ret->data=s;
		ret->type=Ptag;
		}

	if (a != NULL) (*a)=ret;
	*pp=p;
	return(ret);
err:
	if ((ret != NULL) && ((a == NULL) || (*a != ret)))
		ASN1_STRING_free(ret);
	ASN1err(ASN1_F_D2I_ASN1_BYTES,i);
	return(NULL);
	}


/* We are about to parse 0..n d2i_ASN1_bytes objects, we are to collapes
 * them into the one struture that is then returned */
/* There have been a few bug fixes for this function from
 * Paul Keogh <paul.keogh@sse.ie>, many thanks to him */
static int asn1_collate_primative(a,c)
ASN1_STRING *a;
ASN1_CTX *c;
	{
	ASN1_STRING *os=NULL;
	BUF_MEM b;
	int num;

	b.length=0;
	b.max=0;
	b.data=NULL;

	if (a == NULL)
		{
		c->error=ERR_R_PASSED_NULL_PARAMETER;
		goto err;
		}

	num=0;
	for (;;)
		{
		if (c->inf & 1)
			{
			c->eos=ASN1_check_infinite_end(&c->p,
				(long)(c->max-c->p));
			if (c->eos) break;
			}
		else
			{
			if (c->slen <= 0) break;
			}

		c->q=c->p;
		if (d2i_ASN1_bytes(&os,&c->p,c->max-c->p,c->tag,c->xclass)
			== NULL)
			{
			c->error=ERR_R_ASN1_LIB;
			goto err;
			}

		if (!BUF_MEM_grow(&b,num+os->length))
			{
			c->error=ERR_R_BUF_LIB;
			goto err;
			}
		memcpy(&(b.data[num]),os->data,os->length);
		if (!(c->inf & 1))
			c->slen-=(c->p-c->q);
		num+=os->length;
		}

	if (!asn1_Finish(c)) goto err;

	a->length=num;
	if (a->data != NULL) Free(a->data);
	a->data=(unsigned char *)b.data;
	if (os != NULL) ASN1_STRING_free(os);
	return(1);
err:
	ASN1err(ASN1_F_ASN1_COLLATE_PRIMATIVE,c->error);
	if (os != NULL) ASN1_STRING_free(os);
	if (b.data != NULL) Free(b.data);
	return(0);
	}

