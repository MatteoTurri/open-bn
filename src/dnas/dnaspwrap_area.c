/* DNAS1 exclusion-list lookup: 0 if offset is in table[0..count), 1 otherwise. */

#include "dnaspwrap_impl.h"

/* Returns 0 when `offset` is in the exclusion table, 1 otherwise. */
int _sce_dnas_pwrap_is_scramble_area(int offset, int count, const int *table)
{
	int i;

	for (i = 0; i < count; i++) {
		if (offset == table[i])
			return 0;
	}
	return 1;
}
