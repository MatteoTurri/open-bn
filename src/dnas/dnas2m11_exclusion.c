/* DNAS2 exclusion-list lookup: 0 if target is in arr[0..count), 1 otherwise. */

#include "dnas2m11_exclusion.h"

/* Returns 0 when `target` is in the exclusion list, 1 otherwise. */
int __sce_dnas2_symbol11005(int target, int count, const int *arr)
{
	int i;
	for (i = 0; i < count; i++) {
		if (target == arr[i])
			return 0;
	}
	return 1;
}
