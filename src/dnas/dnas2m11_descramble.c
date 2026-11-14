/*
 * DNAS2 table dispatcher __sce_dnas2_symbol11000 (same scheme as dnas2m11_scramble.c).
 */

#include "dnas2m11_descramble.h"
#include "dnas2m11_xform_desc.h"
#include "dnas2scramble.h"

/* The inverse transforms, selected by the 4-bit fields of a descriptor. */
static DnasHandler dnas2_unscramble_ops[8] = {
	(DnasHandler)__sce_dnas2_symbol11036,
	(DnasHandler)__sce_dnas2_symbol11037,
	(DnasHandler)__sce_dnas2_symbol11038,
	(DnasHandler)__sce_dnas2_symbol11039,
	(DnasHandler)__sce_dnas2_symbol11040,
	(DnasHandler)__sce_dnas2_symbol11033,
	(DnasHandler)__sce_dnas2_symbol11034,
	(DnasHandler)__sce_dnas2_symbol11035,
};

/* Undoes the scrambling of [start, end) with the inverse transforms listed in the descriptor of `id`. */
int __sce_dnas2_symbol11000(void *start, void *end, void *base, int id, void *nexcl, void *excl)
{
	int desc = __sce_dnas2_symbol11004(id);
	int count = (int)((unsigned int)desc >> 28);
	int i = 0;

	if (count != 0) {
		do {
			dnas2_unscramble_ops[(desc >> (i * 4)) & 0xF](start, end, base, nexcl, excl);
			i++;
		} while (i < count);
	}
	return 0;
}
