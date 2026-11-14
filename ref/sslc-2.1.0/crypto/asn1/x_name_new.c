/*
 * X509_NAME_new and X509_NAME_free, from crypto/asn1/x_name.c of SSLeay.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "objects.h"
#include "asn1_mac.h"


X509_NAME *X509_NAME_new()
	{
	X509_NAME *ret=NULL;
	ASN1_CTX c;

#line 219 /* the line number is part of the binary */
	M_ASN1_New_Malloc(ret,X509_NAME);
	ret->unk_00=0;

	if ((ret->entries=sk_new(NULL)) == NULL) { c.line=__LINE__; goto err2; }
	M_ASN1_New(ret->bytes,BUF_MEM_new);
	ret->modified=1;
	return(ret);
	M_ASN1_New_Error(ASN1_F_X509_NAME_NEW);
	}

void X509_NAME_free(a)
X509_NAME *a;
	{
	BUF_MEM_free(a->bytes);
	sk_pop_free(a->entries,X509_NAME_ENTRY_free);
	Free((char *)a);
	}

