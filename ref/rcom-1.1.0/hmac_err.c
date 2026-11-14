/*
 * ERR_load_HMAC_strings and the function names of the HMAC error library of RCOM.
 */
#include <stdio.h>
#include "cryptlib.h"
#include "hmac.h"

static ERR_STRING_DATA HMAC_str_functs[]=
	{
{ERR_PACK(0,HMAC_F_HMAC_NEW,0),	"HMAC_NEW"},
{0,NULL},
	};

void ERR_load_HMAC_strings()
	{
	static int init=1;

	if (init)
		{
		init=0;
		ERR_load_strings(ERR_LIB_HMAC,HMAC_str_functs);
		}
	}
