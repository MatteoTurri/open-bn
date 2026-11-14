/*
 * ALG_ErrorCode: the translation of the algorithm error codes into the BSAFE ones.
 */
#include "bsafe.h"

/* 0 stays 0; the other codes are translated by class of result. */
int ALG_ErrorCode(int code)
{
	if (code == 0)
		return 0;

	switch (code) {
	case 256:
	case 1001:
		return 16;
	case 263:
		return 7;
	case 100:
	case 101:
	case 257:
	case 258:
	case 259:
	case 260:
	case 262:
	case 264:
	case 266:
	case 500:
	case 501:
	case 503:
	case 504:
	case 507:
	case 1002:
	case 1007:
	case 1009:
	case 1010:
	case 1011:
	case 1012:
	case 1201:
	case 1202:
	case 1203:
		return 5;
	case 261:
	case 265:
	case 267:
		return 5;
	case 268:
	case 505:
	case 1008:
		return 10;
	default:
		return code;
	}
}
