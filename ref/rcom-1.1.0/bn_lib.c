/*
 * BN_value_one and the version string of the RCOM big numbers. The BIGNUM of RCOM has a flags field
 * (BN_FLG_MALLOCED, BN_FLG_STATIC_DATA, BN_FLG_FREE) that SSLeay 0.9.0b does not have.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

char *BN_version="Big Number part of RCOM 1.1.0 30-Jan-2002";

BIGNUM *BN_value_one()
	{
	static const BN_ULONG data_one=1L;
	static BIGNUM const_one={(BN_ULONG *)&data_one,1,1,0};

	return(&const_one);
	}

