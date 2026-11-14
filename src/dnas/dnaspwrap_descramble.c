/* DNAS1 descrambling driver: applies the inverse transforms selected by the seed nibbles. */

#include "dnaspwrap_impl.h"

static const pwrap_xform_fn g_descrambleTable[8] = {
	_sce_dnas_pwrap_de_sample_xor,
	_sce_dnas_pwrap_de_sample_xor2,
	_sce_dnas_pwrap_de_word_swap1,
	_sce_dnas_pwrap_de_word_swap2,
	_sce_dnas_pwrap_de_word_swap3,
	_sce_dnas_pwrap_de_mix1,
	_sce_dnas_pwrap_de_mix2,
	_sce_dnas_pwrap_de_mix3,
};

/* Descrambles [p, pend) with the inverse transforms, in the same order as _sce_dnas_pwrap_scramble. */
int _sce_dnas_pwrap_descramble(int p, int pend, int base, int seed_key, int count, const int *table)
{
	int seed = _sce_dnas_pwrap_clean_id(seed_key);
	int nrounds = (unsigned)seed >> 28;
	int i;

	for (i = 0; i < nrounds; i++) {
		int nibble = (seed >> (i * 4)) & 0xf;
		g_descrambleTable[nibble](p, pend, base, count, table);
	}
	return 0;
}
