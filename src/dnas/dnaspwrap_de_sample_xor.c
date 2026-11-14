/* DNAS1 inverse transform: XOR with K1. */

#include "dnaspwrap_impl.h"

/* Undoes sample_xor (XOR with K1) on [p, pend) outside the exclusion table. */
int _sce_dnas_pwrap_de_sample_xor(int p, int pend, int base, int count, const int *table)
{
	if (p < pend) {
		do {
			if (_sce_dnas_pwrap_is_scramble_area(p - base, count, table) > 0)
				W(p) ^= K1;
			p += 4;
		} while (p < pend);
	}
	return 0;
}
