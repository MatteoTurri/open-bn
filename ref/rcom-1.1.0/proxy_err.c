/*
 * ERR_load_PROXY_strings and the function names of the PROXY error library of RCOM.
 */
#include <stdio.h>
#include "cryptlib.h"


#define PROXY_F_ADD_NOPROXY                  100
#define PROXY_F_PROXY_ENTRY_NEW              101
#define PROXY_F_PROXY_ENTRY_SET_SERVER       102

static ERR_STRING_DATA PROXY_str_functs[]=
	{
{ERR_PACK(0,PROXY_F_ADD_NOPROXY,0),	"ADD_NOPROXY"},
{ERR_PACK(0,PROXY_F_PROXY_ENTRY_NEW,0),	"PROXY_ENTRY_new"},
{ERR_PACK(0,PROXY_F_PROXY_ENTRY_SET_SERVER,0),	"PROXY_ENTRY_set_server"},
{0,NULL},
	};

void ERR_load_PROXY_strings()
	{
	static int init=1;

	if (init)
		{
		init=0;
		ERR_load_strings(ERR_LIB_PROXY,PROXY_str_functs);
		}
	}
