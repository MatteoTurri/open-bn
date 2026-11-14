/* DNAS1 word transform: permute, rotate left by 13 and 8, XOR with K1. */

#include "dnaspwrap_impl.h"

/* Applies mix3 to every word of [p, pend) outside the exclusion table. */
int _sce_dnas_pwrap_mix3(int p, int pend, int base, int count, const int *table)
{
	if (p < pend) {
		do {
			if (_sce_dnas_pwrap_is_scramble_area(p - base, count, table) > 0)
				W(p) = ROTL32(ROTL32(PERM32(W(p)), 13), 8) ^ K1;
			p += 4;
		} while (p < pend);
	}
	return 0;
}
