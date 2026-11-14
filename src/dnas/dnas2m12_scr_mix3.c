/*
 * DNAS2 code scrambling: transform every word in [start, end) except those whose
 * offset from base appears in the exclusion list excl[0..nexcl).
 */

#include "dnas2m12_exclusion.h"
#include "dnas2scramble.h"

/* Applies byte permutation A, rotates left by 13 then by 8, then XORs with 0x09F8ED17. */
int __sce_dnas2_symbol12009(int start, int end, char *base, int nexcl, const int *excl)
{
	int addr = start;
	if (addr < end) {
		do {
			if (__sce_dnas2_symbol12005(addr - (int)base, nexcl, excl) > 0) {
				int w = *(int *)addr;
				unsigned int perm;
				unsigned int rot13;
				unsigned int rot8;
				perm = ((unsigned int)w << 16) & 0x00FF0000u;
				perm |= (unsigned int)((w >> 8) & 0xFF);
				perm |= ((unsigned int)w << 8) & 0xFF000000u;
				perm |= (unsigned int)((w >> 16) & 0xFF00);
				rot13 = (perm << 13) & 0xFFFFE000u;
				rot13 |= perm >> 19;
				rot8 = (rot13 << 8) & 0xFFFFFF00u;
				rot8 |= rot13 >> 24;
				rot8 ^= 0x09F8ED17u;
				*(unsigned int *)addr = rot8;
			}
			addr += 4;
		} while (addr < end);
	}
	return 0;
}
