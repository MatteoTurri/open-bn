/*
 * DNAS2 code scrambling: transform every word in [start, end) except those whose
 * offset from base appears in the exclusion list excl[0..nexcl).
 */

#include "dnas2m11_exclusion.h"
#include "dnas2scramble.h"

/* Rotates right by 13. */
int __sce_dnas2_symbol11038(int start, int end, char *base, int nexcl, const int *excl)
{
	int addr = start;
	if (addr < end) {
		do {
			if (__sce_dnas2_symbol11005(addr - (int)base, nexcl, excl) > 0) {
				int w = *(int *)addr;
				*(unsigned int *)addr =
				    ((w >> 13) & 0x0007FFFF) | (((unsigned int)w << 19) & 0xFFF80000);
			}
			addr += 4;
		} while (addr < end);
	}
	return 0;
}

/* Rotates right by 8. */
int __sce_dnas2_symbol11039(int start, int end, char *base, int nexcl, const int *excl)
{
	int addr = start;
	if (addr < end) {
		do {
			if (__sce_dnas2_symbol11005(addr - (int)base, nexcl, excl) > 0) {
				int w = *(int *)addr;
				*(unsigned int *)addr =
				    ((w >> 8) & 0x00FFFFFF) | (((unsigned int)w << 24) & 0xFF000000);
			}
			addr += 4;
		} while (addr < end);
	}
	return 0;
}

/* Byte permutation B: b0->1, b1->3, b2->0, b3->2. */
int __sce_dnas2_symbol11040(int start, int end, char *base, int nexcl, const int *excl)
{
	int addr = start;
	if (addr < end) {
		do {
			if (__sce_dnas2_symbol11005(addr - (int)base, nexcl, excl) > 0) {
				int w = *(int *)addr;
				unsigned int r = ((unsigned int)w << 8) & 0xFF00;
				r |= ((unsigned int)w << 16) & 0xFF000000;
				r |= (w >> 16) & 0xFF;
				r |= (w >> 8) & 0x00FF0000;
				*(unsigned int *)addr = r;
			}
			addr += 4;
		} while (addr < end);
	}
	return 0;
}

/* XORs with 0x09F8ED17, then rotates right by 13. */
int __sce_dnas2_symbol11033(int start, int end, char *base, int nexcl, const int *excl)
{
	int addr = start;
	if (addr < end) {
		do {
			if (__sce_dnas2_symbol11005(addr - (int)base, nexcl, excl) > 0) {
				int w = *(int *)addr ^ 0x09F8ED17;
				*(unsigned int *)addr =
				    ((w >> 13) & 0x0007FFFF) | (((unsigned int)w << 19) & 0xFFF80000);
			}
			addr += 4;
		} while (addr < end);
	}
	return 0;
}

/* Applies byte permutation B, XORs with 0xA95128C3, then rotates right by 8. */
int __sce_dnas2_symbol11034(int start, int end, char *base, int nexcl, const int *excl)
{
	int addr = start;
	if (addr < end) {
		do {
			if (__sce_dnas2_symbol11005(addr - (int)base, nexcl, excl) > 0) {
				int w = *(int *)addr;
				unsigned int perm;
				int x;
				unsigned int r;
				perm = ((unsigned int)w << 8) & 0xFF00u;
				perm |= ((unsigned int)w << 16) & 0xFF000000u;
				perm |= (w >> 16) & 0xFFu;
				perm |= (w >> 8) & 0x00FF0000u;
				x = (int)(perm ^ 0xA95128C3u);
				r = ((unsigned int)(x >> 8) & 0x00FFFFFFu) | (((unsigned int)x << 24) & 0xFF000000u);
				*(unsigned int *)addr = r;
			}
			addr += 4;
		} while (addr < end);
	}
	return 0;
}
