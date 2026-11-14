/* DNAS1 word transforms: byte permutation and mixes. */

#include "dnaspwrap_impl.h"

/* Applies the byte permutation PERM32. */
int _sce_dnas_pwrap_word_swap3(int p, int pend, int base, int count, const int *table)
{
	if (p < pend) {
		do {
			if (_sce_dnas_pwrap_is_scramble_area(p - base, count, table) > 0)
				W(p) = PERM32(W(p));
			p += 4;
		} while (p < pend);
	}
	return 0;
}

/* Rotates left by 13, then XORs with K1. */
int _sce_dnas_pwrap_mix1(int p, int pend, int base, int count, const int *table)
{
	if (p < pend) {
		do {
			if (_sce_dnas_pwrap_is_scramble_area(p - base, count, table) > 0)
				W(p) = ROTL32(W(p), 13) ^ K1;
			p += 4;
		} while (p < pend);
	}
	return 0;
}

/* Rotates left by 8, XORs with K2, then applies PERM32. */
int _sce_dnas_pwrap_mix2(int p, int pend, int base, int count, const int *table)
{
	if (p < pend) {
		do {
			if (_sce_dnas_pwrap_is_scramble_area(p - base, count, table) > 0)
				W(p) = PERM32(ROTL32(W(p), 8) ^ K2);
			p += 4;
		} while (p < pend);
	}
	return 0;
}
