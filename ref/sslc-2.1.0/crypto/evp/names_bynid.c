/*
 * EVP_OBJ_get_digestbynid, EVP_OBJ_get_cipherbynid, EVP_OBJ_get_digestbyobj and
 * EVP_OBJ_get_cipherbyobj.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "evp.h"
#include "objects.h"

EVP_MD *EVP_OBJ_get_digestbynid(nid)
int nid;
	{
	char *sn;

	sn=OBJ_nid2sn(nid);
	return(EVP_OBJ_get_digestbyname(sn));
	}

EVP_CIPHER *EVP_OBJ_get_cipherbynid(nid)
int nid;
	{
	char *sn;

	sn=OBJ_nid2sn(nid);
	return(EVP_OBJ_get_cipherbyname(sn));
	}

EVP_MD *EVP_OBJ_get_digestbyobj(a)
ASN1_OBJECT *a;
	{
	int nid;

	nid=OBJ_obj2nid(a);
	return(EVP_OBJ_get_digestbynid(nid));
	}

EVP_CIPHER *EVP_OBJ_get_cipherbyobj(a)
ASN1_OBJECT *a;
	{
	int nid;

	nid=OBJ_obj2nid(a);
	return(EVP_OBJ_get_cipherbynid(nid));
	}
