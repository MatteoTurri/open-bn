/*
 * DNAS2 table dispatcher __sce_dnas2_symbol11003: runs a list of handlers
 * packed in the descriptor of `id`.
 */

#include "dnas2m11_scramble.h"
#include "dnas2m11_xform_desc.h"
#include "dnas2scramble.h"

/* The transforms, selected by the 4-bit fields of a descriptor. */
static DnasHandler dnas2_scramble_ops[8] = {
	(DnasHandler)__sce_dnas2_symbol11010,
	(DnasHandler)__sce_dnas2_symbol11011,
	(DnasHandler)__sce_dnas2_symbol11013,
	(DnasHandler)__sce_dnas2_symbol11014,
	(DnasHandler)__sce_dnas2_symbol11015,
	(DnasHandler)__sce_dnas2_symbol11007,
	(DnasHandler)__sce_dnas2_symbol11008,
	(DnasHandler)__sce_dnas2_symbol11009,
};

/* Scrambles [start, end) with the transforms listed in the descriptor of `id`:
 * the handler count in its top 4 bits and one 4-bit table index per handler,
 * from the lowest nibble up. */
int __sce_dnas2_symbol11003(void *start, void *end, void *base, int id, void *nexcl, void *excl)
{
	int desc = __sce_dnas2_symbol11004(id);
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
