/* DNAS1 scrambling driver: applies the transforms selected by the seed nibbles. */

#include "dnaspwrap_impl.h"

static const pwrap_xform_fn g_scrambleTable[8] = {
	_sce_dnas_pwrap_sample_xor,
	_sce_dnas_pwrap_sample_xor2,
	_sce_dnas_pwrap_word_swap1,
	_sce_dnas_pwrap_word_swap2,
	_sce_dnas_pwrap_word_swap3,
	_sce_dnas_pwrap_mix1,
	_sce_dnas_pwrap_mix2,
	_sce_dnas_pwrap_mix3,
};

/* Scrambles [p, pend): seed >> 28 rounds, round i applying the transform of nibble i of the cleaned seed. */
int _sce_dnas_pwrap_scramble(int p, int pend, int base, int seed_key, int count, const int *table)
{
	int seed = _sce_dnas_pwrap_clean_id(seed_key);
	int nrounds = (unsigned)seed >> 28;
	int i;

	for (i = 0; i < nrounds; i++) {
		int nibble = (seed >> (i * 4)) & 0xf;
		g_scrambleTable[nibble](p, pend, base, count, table);
	}
	return 0;
}
