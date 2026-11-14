/*
 * The random generator object of RCOM (R_rand_*): its methods and state.
 */
#ifndef RCOM_R_RAND_H
#define RCOM_R_RAND_H

typedef struct RRandMeth {
    /* 0x00 */ int (*bytesFn)(struct RRandMeth *rand, unsigned char *buf, int len);
    /* 0x04 */ int (*seedFn)(struct RRandMeth *rand, void *data, unsigned int len);
    /* 0x08 */ int (*ctrlFn)(struct RRandMeth *rand, int cmd, void *arg);
    /* 0x0C */ void *state;
    /* 0x10 */ int flags;  /* 1: allocated, freed with R_free */
} RRandMeth; /* size = 0x14 */

/* r_rand.c */
int R_rand_add_entropy(RRandMeth *rand, int source);
int R_rand_bytes(RRandMeth *rand, void *buf, unsigned int len);
int R_rand_entropy_count(RRandMeth *rand, int *outCount);
void R_rand_free(RRandMeth *rand);
int R_rand_lib_cleanup(void *unused0, void *unused1);
RRandMeth *R_rand_new(RRandMeth *meth);
int R_rand_seed(RRandMeth *rand, void *data, unsigned int len);
int R_rand_set_default(RRandMeth *meth);

/* r_rand_def.c */
RRandMeth *R_rand_get_default(void);

/* r_rand_sha1.c */
RRandMeth *R_rand_meth_sha1(void *unused);

#endif
