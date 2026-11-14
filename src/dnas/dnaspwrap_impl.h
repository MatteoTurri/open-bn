/*
 * DNAS1 code scrambling ("password wrap"): word transforms, their inverses
 * and the scramble/descramble drivers.
 */

#ifndef DNAS_PWRAP_IMPL_H
#define DNAS_PWRAP_IMPL_H

extern int _sce_dnas_pwrap_is_scramble_area(int offset, int count, const int *table);

/*
 * Each transform scans [p, pend) in 4-byte steps and changes the words whose
 * offset p - base is not in the exclusion table:
 *   mix1 = ROTL13(v) ^ K1              mix2 = PERM32(ROTL8(v) ^ K2)
 *   mix3 = ROTL8(ROTL13(PERM32(v))) ^ K1
 *   word_swap1/2/3 = ROTL13(v) / ROTL8(v) / PERM32(v)
 *   sample_xor/sample_xor2 = v ^ K1 / v ^ K2
 * The de_* functions are the inverses. scramble/descramble run seed >> 28
 * rounds; round i applies the transform selected by nibble i of the seed:
 * 0 sample_xor, 1 sample_xor2, 2-4 word_swap1-3, 5-7 mix1-3.
 */
#define ROTL32(v, n) \
    (((v) << (n) & (0xFFFFFFFFu << (n))) | ((v) >> (32 - (n))))
#define ROTR32(v, n) \
    (((int)(v) >> (n) & (0xFFFFFFFFu >> (n))) | \
     ((v) << (32 - (n)) & (0xFFFFFFFFu << (32 - (n)))))
#define PERM32(v) \
    (((v) << 16 & 0x00FF0000) | ((int)(v) >> 8 & 0xFF) | \
     ((v) << 8 & 0xFF000000) | ((int)(v) >> 16 & 0xFF00))
#define PERM32_INV(v) \
    (((v) << 8 & 0xFF00) | ((v) << 16 & 0xFF000000) | \
     ((int)(v) >> 16 & 0xFF) | ((int)(v) >> 8 & 0xFF0000))

/* A transform or its inverse, as the scramble/descramble tables list them. */
typedef int (*pwrap_xform_fn)(int p, int pend, int base, int count, const int *table);

#define K1 0x09F8ED17
#define K2 0xA95128C3
#define W(p) (*(unsigned int *)(p))

int _sce_dnas_pwrap_sample_xor(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_sample_xor2(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_de_sample_xor(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_de_sample_xor2(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_word_swap1(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_word_swap2(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_word_swap3(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_de_word_swap1(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_de_word_swap2(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_de_word_swap3(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_mix1(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_mix2(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_mix3(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_de_mix1(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_de_mix2(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_de_mix3(int p, int pend, int base, int count, const int *table);
int _sce_dnas_pwrap_scramble(int p, int pend, int base, int seed_key, int count, const int *table);
int _sce_dnas_pwrap_descramble(int p, int pend, int base, int seed_key, int count, const int *table);

extern int _sce_dnas_pwrap_clean_id(int);
#endif
