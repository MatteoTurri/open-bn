/*
 * Helper type of gm_graph with inline members: the per-mode matrix stack.
 */

#ifndef GM_TYPES_H
#define GM_TYPES_H

#include "vec_types.h"

/* One matrix stack per matrix mode (0..7). */
struct mat4_stack {
	/* 0x00 */ int fMaxDepth;  /* 31 */
	/* 0x04 */ int fDepth;  /* current depth */
	/* 0x08 */ mat4 *fBase;  /* 16-byte aligned storage of the 32 levels; top = fBase[fDepth] */
	/* 0x0C */ vec4 *fScales;  /* one per level, after the 32 matrices */
	/* 0x10 */ char pad_10[0x50 - 0x10];
	/* 0x50 */ int fDerivedValid;  /* cleared whenever the top matrix changes (load, multiply, pop); never set */
	/* 0x54 */ void *fRawAlloc;  /* unaligned allocation */
	/* 0x58 */ char pad_58[0x60 - 0x58];
	void clear();
	void load_identity();
	void load_matrix(const mat4 &m);
	void load_matrix(const float *m) { load_matrix(*(const mat4 *)m); }
	void mult_matrix(const mat4 &m);

	mat4_stack()
	{
		fMaxDepth = 31;
		fDepth = 0;
		void *raw = new char[2576];
		int aligned = ((int)raw + 15) / 16 * 16;
		fBase = (mat4 *)aligned;
		fRawAlloc = raw;
		fScales = (vec4 *)(aligned + 2048);
		clear();
	}
	~mat4_stack()
	{
		if (fRawAlloc)
			delete[] (char *)fRawAlloc;
	}
}; /* size = 0x60 */

#endif
