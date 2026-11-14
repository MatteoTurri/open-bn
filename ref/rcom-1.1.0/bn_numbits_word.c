/*
 * BN_num_bits_word.
 */

#include <stdio.h>
#include "cryptlib.h"
#include "bn_lcl.h"

int BN_num_bits_word(l)
BN_ULONG l;
	{
	static const char bits[256]={
		0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		};
	int shift;

#if defined(THIRTY_TWO_BIT) || defined(SIXTY_FOUR_BIT) || defined(SIXTY_FOUR_BIT_LONG)
	if (l & 0xffff0000L)
		{
		if (l & 0xff000000L)
			shift=24;
		else	shift=16;
		}
	else
#endif
		{
#if defined(SIXTEEN_BIT) || defined(THIRTY_TWO_BIT) || defined(SIXTY_FOUR_BIT) || defined(SIXTY_FOUR_BIT_LONG)
		if (l & 0xff00L)
			shift=8;
		else
#endif
			shift=0;
		}
	return(bits[l>>shift]+shift);
	}
