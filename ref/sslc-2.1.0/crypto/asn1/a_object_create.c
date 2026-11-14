/*
 * ASN1_OBJECT_create, from crypto/asn1/a_object.c of SSLeay.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "asn1.h"
#include "objects.h"

ASN1_OBJECT *ASN1_OBJECT_create(nid,data,len,sn,ln)
int nid;
unsigned char *data;
int len;
char *sn,*ln;
	{
	ASN1_OBJECT o;

	o.sn=sn;
	o.ln=ln;
	o.data=data;
	o.nid=nid;
	o.length=len;
	o.flags=ASN1_OBJECT_FLAG_DYNAMIC|
		ASN1_OBJECT_FLAG_DYNAMIC_STRINGS|ASN1_OBJECT_FLAG_DYNAMIC_DATA;
	return(OBJ_dup(&o));
	}

