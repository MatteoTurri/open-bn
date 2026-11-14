/*
 * DNAS2 transform descriptor: reduce each 4-bit nibble of id modulo 8; the top
 * nibble (the round count) defaults to 5 when its low three bits are zero.
 */

#include "dnas2m12_xform_desc.h"

/* Returns the transform descriptor of `id`: the count nibble and the seven table indices. */
int __sce_dnas2_symbol12004(int id)
{
	int top = (id >> 28) & 0xf;
	int hi;
	unsigned int r = 0;
	int i;

	if (top & 7)
		hi = top % 8;
	else
		hi = 5;
	for (i = 0; i < 7; i++)
		r |= (((id >> (i * 4)) & 0xf) % 8) << (i * 4);
	return (r & 0x0fffffff) | (hi << 28);
}
