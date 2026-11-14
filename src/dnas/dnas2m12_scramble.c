/*
 * DNAS2 table dispatcher __sce_dnas2_symbol12003 (same scheme as dnas2m11_scramble.c).
 */

#include "dnas2m12_scramble.h"
#include "dnas2m12_xform_desc.h"
#include "dnas2scramble.h"

/* The transforms, selected by the 4-bit fields of a descriptor. */
static DnasHandler dnas2_scramble_ops[8] = {
	(DnasHandler)__sce_dnas2_symbol12010,
	(DnasHandler)__sce_dnas2_symbol12011,
	(DnasHandler)__sce_dnas2_symbol12013,
	(DnasHandler)__sce_dnas2_symbol12014,
	(DnasHandler)__sce_dnas2_symbol12015,
	(DnasHandler)__sce_dnas2_symbol12007,
	(DnasHandler)__sce_dnas2_symbol12008,
	(DnasHandler)__sce_dnas2_symbol12009,
};

/* Scrambles [start, end) with the transforms listed in the descriptor of `id` (see dnas2m11_scramble.c). */
int __sce_dnas2_symbol12003(void *start, void *end, void *base, int id, void *nexcl, void *excl)
{
	int desc = __sce_dnas2_symbol12004(id);
	int count = (int)((unsigned int)desc >> 28);
	int i = 0;

	if (count != 0) {
		do {
			dnas2_scramble_ops[(desc >> (i * 4)) & 0xF](start, end, base, nexcl, excl);
			i++;
		} while (i < count);
	}
	return 0;
}
