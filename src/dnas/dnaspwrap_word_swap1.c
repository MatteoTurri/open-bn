/* DNAS1 word transform: rotate left by 13. */

#include "dnaspwrap_impl.h"

/* Rotates left by 13 every word of [p, pend) outside the exclusion table. */
int _sce_dnas_pwrap_word_swap1(int p, int pend, int base, int count, const int *table)
{
	if (p < pend) {
		do {
			if (_sce_dnas_pwrap_is_scramble_area(p - base, count, table) > 0)
				W(p) = ROTL32(W(p), 13);
			p += 4;
		} while (p < pend);
	}
	return 0;
}
