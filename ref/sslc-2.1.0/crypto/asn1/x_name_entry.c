/*
 * d2i_X509_NAME_ENTRY and i2d_X509_NAME_ENTRY, from crypto/asn1/x_name.c of SSLeay.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "objects.h"
#include "asn1_mac.h"

int i2d_X509_NAME_ENTRY(a,pp)
X509_NAME_ENTRY *a;
unsigned char **pp;
	{
	M_ASN1_I2D_vars(a);

	M_ASN1_I2D_len(a->object,i2d_ASN1_OBJECT);
	M_ASN1_I2D_len(a->value,i2d_ASN1_PRINTABLE);

	M_ASN1_I2D_seq_total();

	M_ASN1_I2D_put(a->object,i2d_ASN1_OBJECT);
	M_ASN1_I2D_put(a->value,i2d_ASN1_PRINTABLE);

	M_ASN1_I2D_finish();
	}

X509_NAME_ENTRY *d2i_X509_NAME_ENTRY(a,pp,length)
X509_NAME_ENTRY **a;
unsigned char **pp;
long length;
	{
#line 62 /* the line number is part of the binary */
	M_ASN1_D2I_vars(a,X509_NAME_ENTRY *,X509_NAME_ENTRY_new);

	M_ASN1_D2I_Init();
	M_ASN1_D2I_start_sequence();
	M_ASN1_D2I_get(ret->object,d2i_ASN1_OBJECT);
	M_ASN1_D2I_get(ret->value,d2i_ASN1_PRINTABLE);
	ret->set=0;
	M_ASN1_D2I_Finish(a,X509_NAME_ENTRY_free,ASN1_F_D2I_X509_NAME_ENTRY);
	}

