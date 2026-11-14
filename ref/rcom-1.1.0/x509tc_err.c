/*
 * ERR_load_X509TC_strings and the function names of the X509TC error library of RCOM.
 */
#include <stdio.h>
#include "cryptlib.h"


#define X509TC_F_X509TC_DUMMY                 100

static ERR_STRING_DATA X509TC_str_functs[]=
	{
{ERR_PACK(0,X509TC_F_X509TC_DUMMY,0),	"X509TC_DUMMY"},
{0,NULL},
	};

void ERR_load_X509TC_strings()
	{
	static int init=1;

	if (init)
		{
		init=0;
		ERR_load_strings(ERR_LIB_X509TC,X509TC_str_functs);
		}
	}
