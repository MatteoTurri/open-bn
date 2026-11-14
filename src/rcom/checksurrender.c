/*
 * CheckSurrender: polls an optional caller-supplied hook asking whether the
 * current RCOM operation should give up.
 */

#include "checksurrender.h"

/* Returns 1 if the hook asks to give up, 0 if there is no hook. */
int CheckSurrender(struct SurrenderHook *h)
{
	if (h == 0)
		return 0;
	return h->fn(h->arg) != 0;
}
