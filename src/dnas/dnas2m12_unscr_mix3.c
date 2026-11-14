/*
 * DNAS2 code scrambling: transform every word in [start, end) except those whose
 * offset from base appears in the exclusion list excl[0..nexcl).
 */

#include "dnas2m12_exclusion.h"
#include "dnas2scramble.h"

/* XORs with 0x09F8ED17, rotates right by 8 and by 13, then applies byte permutation B. */
int __sce_dnas2_symbol12035(int start, int end, char *base, int nexcl, const int *excl)
{
	int addr = start;
	if (addr < end) {
		do {
			if (__sce_dnas2_symbol12005(addr - (int)base, nexcl, excl) > 0) {
				int w = *(int *)addr ^ 0x09F8ED17;
				unsigned int rot8 =
				    ((unsigned int)(w >> 8) & 0x00FFFFFFu) | (((unsigned int)w << 24) & 0xFF000000u);
				int x = (int)rot8;
				unsigned int rot13 =
				    ((unsigned int)(x >> 13) & 0x0007FFFFu) | (((unsigned int)x << 19) & 0xFFF80000u);
				int y = (int)rot13;
				unsigned int r = ((unsigned int)(y << 8) & 0xFF00u)
				    | ((unsigned int)(y << 16) & 0xFF000000u) | ((unsigned int)(y >> 16) & 0xFFu)
				    | ((unsigned int)(y >> 8) & 0x00FF0000u);
				*(unsigned int *)addr = r;
			}
			addr += 4;
		} while (addr < end);
	}
	return 0;
}
