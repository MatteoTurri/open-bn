/*
 * i2t_ASN1_OBJECT_ex: the SSL-C variant of i2t_ASN1_OBJECT, which always writes the numeric form
 * of the OID.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "buffer.h"
#include "x509.h"

int i2t_ASN1_OBJECT_ex(buf,buf_len,a)
char *buf;
int buf_len;
ASN1_OBJECT *a;
	{
	int i,idx=0,n=0,len;
	unsigned long l;
	unsigned char *p;
	char tbuf[32];

	if (buf_len <= 0) return(0);

	if ((a == NULL) || (a->data == NULL))
		{
		buf[0]='\0';
		return(0);
		}

	len=a->length;
	p=a->data;

	idx=0;
	l=0;
	while (idx < a->length)
		{
		l|=(p[idx]&0x7f);
		if (!(p[idx] & 0x80)) break;
		l<<=7L;
		idx++;
		}
	idx++;
	i=(int)(l/40);
	if (i > 2) i=2;
	l-=(long)(i*40);

	sprintf(tbuf,"%d.%ld",i,l);
	i=strlen(tbuf);
	strncpy(buf,tbuf,buf_len);
	buf_len-=i;
	buf+=i;
	n+=i;

	l=0;
	for (; idx<len; idx++)
		{
		l|=p[idx]&0x7f;
		if (!(p[idx] & 0x80))
			{
			sprintf(tbuf,".%ld",l);
			i=strlen(tbuf);
			if (buf_len > 0)
				strncpy(buf,tbuf,buf_len);
			buf_len-=i;
			buf+=i;
			n+=i;
			l=0;
			}
		l<<=7L;
		}

	buf[buf_len-1]='\0';
	return(n);
	}
