/*
 * DNAS2 code scrambling: transform every word in [start, end) except those whose
 * offset from base appears in the exclusion list excl[0..nexcl).
 */

#include "dnas2m12_exclusion.h"
#include "dnas2scramble.h"

/* Rotates left by 13. */
int __sce_dnas2_symbol12013(int start, int end, char *base, int nexcl, const int *excl)
{
	int addr = start;
	if (addr < end) {
		do {
			if (__sce_dnas2_symbol12005(addr - (int)base, nexcl, excl) > 0) {
				unsigned int w = *(unsigned int *)addr;
				*(unsigned int *)addr = ((w << 13) & 0xFFFFE000) | (w >> 19);
			}
			addr += 4;
		} while (addr < end);
	}
	return 0;
}
