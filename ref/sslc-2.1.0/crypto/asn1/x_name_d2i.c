/*
 * d2i_X509_NAME, from crypto/asn1/x_name.c of SSLeay.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "objects.h"
#include "asn1_mac.h"


X509_NAME *d2i_X509_NAME(a,pp,length)
X509_NAME **a;
unsigned char **pp;
long length;
	{
	int set=0,i;
	int idx=0;
	unsigned char *orig;
#line 176 /* the line number is part of the binary */
	M_ASN1_D2I_vars(a,X509_NAME *,X509_NAME_new);

	orig= *pp;
	if (sk_num(ret->entries) > 0)
		{
		while (sk_num(ret->entries) > 0)
			X509_NAME_ENTRY_free((X509_NAME_ENTRY *)
				sk_pop(ret->entries));
		}

	M_ASN1_D2I_Init();
	M_ASN1_D2I_start_sequence();
	for (;;)
		{
		if (M_ASN1_D2I_end_sequence())
			break;
		M_ASN1_D2I_get_set(ret->entries,d2i_X509_NAME_ENTRY,X509_NAME_ENTRY_free);
		for (; idx < sk_num(ret->entries); idx++)
			{
			((X509_NAME_ENTRY *)sk_value(ret->entries,idx))->set=
				set;
			}
		set++;
		}

	i=(int)(c.p-orig);
	if (!BUF_MEM_grow(ret->bytes,i)) goto err;
	memcpy(ret->bytes->data,orig,i);
	ret->bytes->length=i;
	ret->modified=0;

	M_ASN1_D2I_Finish(a,X509_NAME_free,ASN1_F_D2I_X509_NAME);
	}

