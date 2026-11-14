/* DNAS1 inverse transform of _sce_dnas_pwrap_mix3. */

#include "dnaspwrap_impl.h"

/* Undoes mix3 on every word of [p, pend) outside the exclusion table. */
int _sce_dnas_pwrap_de_mix3(int p, int pend, int base, int count, const int *table)
{
	if (p < pend) {
		do {
			if (_sce_dnas_pwrap_is_scramble_area(p - base, count, table) > 0)
				W(p) = PERM32_INV(ROTR32(ROTR32(W(p) ^ K1, 8), 13));
			p += 4;
		} while (p < pend);
	}
	return 0;
}
