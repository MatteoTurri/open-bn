/* DNAS1 inverse word transforms (rotations, permutation, mixes). */

#include "dnaspwrap_impl.h"

/* Rotates right by 13. */
int _sce_dnas_pwrap_de_word_swap1(int p, int pend, int base, int count, const int *table)
{
	if (p < pend) {
		do {
			if (_sce_dnas_pwrap_is_scramble_area(p - base, count, table) > 0)
				W(p) = ROTR32(W(p), 13);
			p += 4;
		} while (p < pend);
	}
	return 0;
}

/* Rotates right by 8. */
int _sce_dnas_pwrap_de_word_swap2(int p, int pend, int base, int count, const int *table)
{
	if (p < pend) {
		do {
			if (_sce_dnas_pwrap_is_scramble_area(p - base, count, table) > 0)
				W(p) = ROTR32(W(p), 8);
			p += 4;
		} while (p < pend);
	}
	return 0;
}

/* Inverse of the byte permutation PERM32. */
int _sce_dnas_pwrap_de_word_swap3(int p, int pend, int base, int count, const int *table)
{
	if (p < pend) {
		do {
			if (_sce_dnas_pwrap_is_scramble_area(p - base, count, table) > 0)
				W(p) = PERM32_INV(W(p));
			p += 4;
		} while (p < pend);
	}
	return 0;
}

/* Inverse of _sce_dnas_pwrap_mix1. */
int _sce_dnas_pwrap_de_mix1(int p, int pend, int base, int count, const int *table)
{
	if (p < pend) {
		do {
			if (_sce_dnas_pwrap_is_scramble_area(p - base, count, table) > 0)
				W(p) = ROTR32(W(p) ^ K1, 13);
			p += 4;
		} while (p < pend);
	}
	return 0;
}

/* Inverse of _sce_dnas_pwrap_mix2. */
int _sce_dnas_pwrap_de_mix2(int p, int pend, int base, int count, const int *table)
{
	if (p < pend) {
		do {
			if (_sce_dnas_pwrap_is_scramble_area(p - base, count, table) > 0)
				W(p) = ROTR32(PERM32_INV(W(p)) ^ K2, 8);
			p += 4;
		} while (p < pend);
	}
	return 0;
}
