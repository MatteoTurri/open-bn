/*
 * DNAS2 module 12 protection shim: makes a code region writable, runs the
 * transform selected by bit 0 of flags, flushes the caches, write-protects it.
 */

#include <sys/cachectl.h>
#include <sys/mman.h>

#include "dnas2selfmod.h"
#include "dnas2m12_descramble.h"
#include "dnas2m12_scramble.h"

static void dnas2_params12(unsigned int sel, void **baseOff, void **nexcl, void **excl);

/* Called from a protected function with the region label and the descriptor
 * words: descrambles the region that follows the label (bit 0 of flags clear)
 * or scrambles the one that precedes it (bit 0 set), with the transform list
 * of `id` and the parameters selected by flags >> 1. */
int __sce_dnas2_symbol12002(char *label, unsigned int maskedLen, unsigned int id, unsigned int flags)
{
	void *baseOff, *nexcl, *excl;
	unsigned int sel = flags >> 1;
	unsigned int scramble = flags & 1;
	unsigned int len = maskedLen ^ id;
	char *start;
	char *page;
	unsigned int n;
	int off;

	if (scramble == 0)
		start = label - 20;
	else
		start = label - len - 20;

	off = (int)start % 4096;
	page = start - off;
	n = len + 20;
	mprotect(page, off + n, PROT_READ | PROT_WRITE | PROT_EXEC);

	if (scramble == 0) {
		*(int *)start = (int)start;
		dnas2_params12(sel, &baseOff, &nexcl, &excl);
		__sce_dnas2_symbol12000(label, label + len, label - (int)baseOff, id, nexcl, excl);
	} else {
		*(int *)start = 0;
		dnas2_params12(sel, &baseOff, &nexcl, &excl);
		__sce_dnas2_symbol12003(label - len, label, (label - len) - (int)baseOff, id, nexcl, excl);
	}

	cacheflush(start, len + 20, 2);
	cacheflush(start, len + 20, 1);
	return mprotect(page, len + 20 + off, PROT_READ | PROT_EXEC);
}

/* Looks up the scramble parameters of `sel` in the module table: base offset,
 * exclusion count and list; all zero when sel is 0 or not found. */
static void dnas2_params12(unsigned int sel, void **baseOff, void **nexcl, void **excl)
{
	dnas2_param_tbl *tbl;
	int dataOff;
	unsigned char *data = 0;  /* redundant as written */

	if (sel != 0) {
		tbl = &__sce_dnas2_symbol12001;
		DNAS2_PARAM_FIND(tbl, sel, dataOff);
		if (dataOff >= 0) {
			DNAS2_PARAM_DATA(data, tbl, dataOff);
			*baseOff = *(void **)data;
			*nexcl = *(void **)(data + 4);
			*excl = data + 8;
		} else
			DNAS2_PARAM_CLEAR(baseOff, nexcl, excl);
	} else
		DNAS2_PARAM_CLEAR(baseOff, nexcl, excl);
}
