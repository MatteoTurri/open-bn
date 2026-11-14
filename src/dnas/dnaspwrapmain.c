/*
 * DNAS1 self-modifying code shim: makes the page of a protected region
 * writable, scrambles or descrambles it, flushes the caches and restores the
 * page protection.
 */

#include <sys/cachectl.h>
#include <sys/mman.h>

#include "dnaspwrap_impl.h"

/*
 * Region descriptor table: a count followed by count pairs {id << 1, offset};
 * offsets are relative to the data area that starts right after the pairs.
 */
typedef struct {
	unsigned int key;  /* id << 1 */
	int off;
} PwrapInfoEnt;

typedef struct {
	int count;
	PwrapInfoEnt ent[1];
} PwrapInfoTbl;

extern PwrapInfoTbl _sce_dnas_pwrap_info_tbl;

/*
 * PWRAP_INFO_FIND: offset of the data of id k, -1 if absent.
 * PWRAP_INFO_DATA: address of the data at offset off.
 * PWRAP_INFO_CLEAR: zero the three outputs.
 */
#define PWRAP_INFO_FIND(t, k, off) do {					\
	int i_;								\
	for ((off) = -1, i_ = 0; i_ < (t)->count; i_++) {		\
		if (((t)->ent[i_].key >> 1) == (k)) {			\
			(off) = (t)->ent[i_].off;			\
			break;						\
		}							\
	}								\
} while (0)
#define PWRAP_INFO_DATA(p, tbl, off) do {				\
	PwrapInfoEnt* e_;						\
	(p) = (unsigned char*)(tbl);					\
	e_ = ((PwrapInfoTbl*)(p))->ent;					\
	(p) = (unsigned char*)(e_ + ((PwrapInfoTbl*)(p))->count);	\
	(p) += (off);							\
} while (0)
#define PWRAP_INFO_CLEAR(a, b, c) do {					\
	*(a) = 0;							\
	*(b) = 0;							\
	*(c) = 0;							\
} while (0)

static void pwrap_info_lookup();

/*
 * idflags = (id << 1) | dir. dir 0 descrambles [p, p + len), dir 1 scrambles
 * [p - len, p), with len = lenxcode ^ code. The region descriptor is the
 * 20 bytes right before the region.
 */
/* clang-format off */
void _sce_dnas_pwrap_main(p, lenxcode, code, idflags)
	int p;
	int lenxcode;
	int code;
	unsigned int idflags;
{
	/* clang-format on */
	int len;
	int id;
	int dir;
	int desc;
	int off;
	int base;
	int size;
	int a;
	int b;
	int c;

	id = idflags >> 1;
	dir = idflags & 1;
	len = lenxcode ^ code;

	if (dir == 0)
		desc = p - 20;
	else
		desc = p - len - 20;

	off = desc % 4096;
	base = desc - off;
	size = len + 20;
	mprotect((void *)base, off + size, PROT_READ | PROT_WRITE | PROT_EXEC);

	if (dir == 0) {
		*(int *)desc = desc;
		pwrap_info_lookup(id, &a, &b, &c);
		_sce_dnas_pwrap_descramble(p, p + len, p - a, code, b, (const int *)c);
	} else {
		*(int *)desc = 0;
		pwrap_info_lookup(id, &a, &b, &c);
		_sce_dnas_pwrap_scramble(p - len, p, (p - len) - a, code, b, (const int *)c);
	}

	cacheflush((void *)desc, len + 20, 2);
	cacheflush((void *)desc, len + 20, 1);
	mprotect((void *)base, len + 20 + off, PROT_READ | PROT_EXEC);
}

/*
 * Looks up id in the descriptor table: returns its two data words and a
 * pointer to the third; zeros all outputs if id is 0 or absent.
 */
/* clang-format off */
static void pwrap_info_lookup(id, out0, out1, outp)
	unsigned int id;
	int* out0;
	int* out1;
	int* outp;
{
	/* clang-format on */
	PwrapInfoTbl *t;
	int off;
	unsigned char *p = 0;  /* redundant as written */

	if (id != 0) {
		t = &_sce_dnas_pwrap_info_tbl;
		PWRAP_INFO_FIND(t, id, off);
		if (off >= 0) {
			PWRAP_INFO_DATA(p, t, off);
			*out0 = *(int *)p;
			*out1 = *(int *)(p + 4);
			*outp = (int)(p + 8);
		} else
			PWRAP_INFO_CLEAR(out0, out1, outp);
	} else
		PWRAP_INFO_CLEAR(out0, out1, outp);
}
