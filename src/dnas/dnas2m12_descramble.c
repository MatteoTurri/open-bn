/*
 * DNAS2 table dispatcher __sce_dnas2_symbol12000 (same scheme as dnas2m11_scramble.c).
 */

#include "dnas2m12_descramble.h"
#include "dnas2m12_xform_desc.h"
#include "dnas2scramble.h"

/* The inverse transforms, selected by the 4-bit fields of a descriptor. */
static DnasHandler dnas2_unscramble_ops[8] = {
	(DnasHandler)__sce_dnas2_symbol12036,
	(DnasHandler)__sce_dnas2_symbol12037,
	(DnasHandler)__sce_dnas2_symbol12038,
	(DnasHandler)__sce_dnas2_symbol12039,
	(DnasHandler)__sce_dnas2_symbol12040,
	(DnasHandler)__sce_dnas2_symbol12033,
	(DnasHandler)__sce_dnas2_symbol12034,
	(DnasHandler)__sce_dnas2_symbol12035,
};

/* Undoes the scrambling of [start, end) with the inverse transforms listed in the descriptor of `id`. */
int __sce_dnas2_symbol12000(void *start, void *end, void *base, int id, void *nexcl, void *excl)
{
	int desc = __sce_dnas2_symbol12004(id);
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
