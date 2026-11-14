/*
 * X509_NAME_ENTRY_new and X509_NAME_ENTRY_free, from crypto/asn1/x_name.c of SSLeay.
 */
/*
 * d2i_X509_NAME_ENTRY and i2d_X509_NAME_ENTRY, from crypto/asn1/x_name.c of SSLeay.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "objects.h"
#include "asn1_mac.h"

X509_NAME_ENTRY *X509_NAME_ENTRY_new()
	{
	X509_NAME_ENTRY *ret=NULL;
	ASN1_CTX c;

#line 238 /* the line number is part of the binary */
	M_ASN1_New_Malloc(ret,X509_NAME_ENTRY);
	ret->unk_00=0;
/*	M_ASN1_New(ret->object,ASN1_OBJECT_new);*/
	ret->object=NULL;
	ret->set=0;
	M_ASN1_New(ret->value,ASN1_STRING_new);
	return(ret);
	M_ASN1_New_Error(ASN1_F_X509_NAME_ENTRY_NEW);
	}

void X509_NAME_ENTRY_free(a)
X509_NAME_ENTRY *a;
	{
	if (a == NULL) return;
	ASN1_OBJECT_free(a->object);
	ASN1_BIT_STRING_free(a->value);
	Free((char *)a);
	}
