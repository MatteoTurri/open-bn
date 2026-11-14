/*
 * DNAS2 code scrambling: transform every word in [start, end) except those whose
 * offset from base appears in the exclusion list excl[0..nexcl).
 */

#include "dnas2m11_exclusion.h"
#include "dnas2scramble.h"

/* Byte permutation A: b0->2, b1->0, b2->3, b3->1. */
int __sce_dnas2_symbol11015(int start, int end, char *base, int nexcl, const int *excl)
{
	int addr = start;
	if (addr < end) {
		do {
			if (__sce_dnas2_symbol11005(addr - (int)base, nexcl, excl) > 0) {
				int w = *(int *)addr;
				unsigned int r = ((unsigned int)w << 16) & 0x00FF0000;
				r |= (w >> 8) & 0xFF;
				r |= ((unsigned int)w << 8) & 0xFF000000;
				r |= (w >> 16) & 0xFF00;
				*(unsigned int *)addr = r;
			}
			addr += 4;
		} while (addr < end);
	}
	return 0;
}

/* Rotates left by 13, then XORs with 0x09F8ED17. */
int __sce_dnas2_symbol11007(int start, int end, char *base, int nexcl, const int *excl)
{
	int addr = start;
	if (addr < end) {
		do {
			if (__sce_dnas2_symbol11005(addr - (int)base, nexcl, excl) > 0) {
				unsigned int w = *(unsigned int *)addr;
				*(unsigned int *)addr = (((w << 13) & 0xFFFFE000) | (w >> 19)) ^ 0x09F8ED17;
			}
			addr += 4;
		} while (addr < end);
	}
	return 0;
}

/* Rotates left by 8, XORs with 0xA95128C3, then applies byte permutation A. */
int __sce_dnas2_symbol11008(int start, int end, char *base, int nexcl, const int *excl)
{
	int addr = start;
	if (addr < end) {
		do {
			if (__sce_dnas2_symbol11005(addr - (int)base, nexcl, excl) > 0) {
				unsigned int w = *(unsigned int *)addr;
				unsigned int rotl8 = ((w << 8) & 0xFFFFFF00u) | (w >> 24);
				int x = (int)(rotl8 ^ 0xA95128C3u);
				unsigned int r = ((unsigned int)x << 16) & 0x00FF0000u;
				r |= (x >> 8) & 0xFF;
				r |= ((unsigned int)x << 8) & 0xFF000000u;
				r |= (x >> 16) & 0xFF00;
				*(unsigned int *)addr = r;
			}
			addr += 4;
		} while (addr < end);
	}
	return 0;
}
