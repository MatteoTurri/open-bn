/*
 * Parameter tables of the DNAS2 self-modifying code shims.
 */

#ifndef DNAS2SELFMOD_H
#define DNAS2SELFMOD_H

/*
 * Parameter tables (exported data): a count followed by count pairs
 * {key << 1, offset}; offsets are relative to the data area that starts
 * right after the pairs.
 */
typedef struct {
	/* 0x00 */ unsigned int key;
	/* 0x04 */ int off;
} dnas2_param_ent; /* size = 0x08 */

typedef struct {
	/* 0x00 */ int count;
	/* 0x04 */ dnas2_param_ent ent[1];
} dnas2_param_tbl;

extern dnas2_param_tbl __sce_dnas2_symbol11001;
extern dnas2_param_tbl __sce_dnas2_symbol12001;

/*
 * DNAS2_PARAM_FIND: offset of the data of key k in res, -1 if absent.
 * DNAS2_PARAM_DATA: address of the data at offset off.
 * DNAS2_PARAM_CLEAR: zero the three outputs.
 */
#define DNAS2_PARAM_FIND(t, k, res) do {				\
	int i_;								\
	for ((res) = -1, i_ = 0; i_ < (t)->count; i_++) {		\
		if (((t)->ent[i_].key >> 1) == (k)) {			\
			(res) = (t)->ent[i_].off;			\
			break;						\
		}							\
	}								\
} while (0)
#define DNAS2_PARAM_DATA(p, tbl, off) do {				\
	dnas2_param_ent *e_;						\
	(p) = (unsigned char *)(tbl);					\
	e_ = ((dnas2_param_tbl *)(p))->ent;				\
	(p) = (unsigned char *)(e_ + ((dnas2_param_tbl *)(p))->count);	\
	(p) += (off);							\
} while (0)
#define DNAS2_PARAM_CLEAR(a, b, c) do {				\
	*(a) = 0;							\
	*(b) = 0;							\
	*(c) = 0;							\
} while (0)

#endif
