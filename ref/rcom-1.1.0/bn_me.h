/*
 * The modular exponentiation methods of RCOM (BN_ME_CTX_new/free/set, BN_ME_CTX_mod_exp, the BN_bnme_*
 * cache): the method table and the context that points to it.
 */
#ifndef BN_ME_H
#define BN_ME_H

typedef struct BN_ME_METH {
    /* 0x00 */ int nwords;  /* modulus words this method covers (BN_bnme_set) */
    /* 0x04 */ char *name;  /* "Word"; the DH and RSA get-info calls return it */
    /* 0x08 */ int (*initFn)(void *self);  /* self-check, 0 = ok */
    /* 0x0C */ int (*opFn)();  /* (ctx, r, a, p, bnctx): r = a^p on the modulus of the context */
    /* 0x10 */ int (*constructFn)();  /* (meth, ctxOut): a new context in *ctxOut */
    /* 0x14 */ int (*destroyFn)();  /* (ctx) */
    /* 0x18 */ int (*setFn)();  /* (ctx, mod, mode, flags, extra) */
    /* 0x1C */ void *pad_1C[5];  /* unused, but part of the layout */
} BN_ME_METH; /* size = 0x30 */

/* A context of a method: the method table comes first. */
typedef struct BN_ME_CTX {
    /* 0x00 */ BN_ME_METH *meth;
} BN_ME_CTX;

/* The context of BN_ME_METH_word; its BN_MONT_CTX follows at 0x20 in the same allocation. */
typedef struct BN_ME_WORD {
    /* 0x00 */ BN_ME_CTX base;
    /* 0x04 */ int pad_04[3];  /* unused, but part of the layout */
    /* 0x10 */ struct bn_mont_ctx_st *mont;
    /* 0x14 */ unsigned char *prog;  /* exponent program from BN_gen_exp_bits */
    /* 0x18 */ void *extra;
} BN_ME_WORD;

/* A method of the BN_bnme_* cache, and of the tables BN_library_init registers (closed by an
 * entry with getMeth == 0). */
typedef struct BNME_ENTRY {
    /* 0x00 */ void *getMeth;  /* BN_ME_METH *(*)(void) */
    /* 0x04 */ int minBits;
    /* 0x08 */ int maxBits;
    /* 0x0C */ int flags;  /* 1: soft, 2: the bounds are in bits, not words */
} BNME_ENTRY; /* size = 0x10 */

struct bignum_st;
struct bignum_ctx;

/* bn_bnme.c */
int BN_library_init(BNME_ENTRY *table);
void *BN_bnme_get(int bits, int flags);
int BN_bnme_set(void *getMeth, int minBits, int maxBits, int flags);
int BN_bnme_insert(void *getMeth, int minBits, int maxBits, int flags, int idx);
int BN_bnme_delete(int idx);

/* bn_me.c */
int BN_default_init(void);

/* bn_me_data.c: the method of a context created without one */
extern BN_ME_METH *(*bn_mod_exp_meth_default)(void);

/* bn_me_ctx.c */
void *BN_ME_CTX_new(BN_ME_METH *methods, void **ctxOut);
void BN_ME_CTX_free(BN_ME_CTX *ctxArg);
int BN_ME_CTX_set(BN_ME_CTX *ctxArg, void *mod, int mode, int flags, void *extra);

/* bn_me_modexp.c */
int BN_ME_CTX_mod_exp(BN_ME_CTX *mectx, struct bignum_st *r, struct bignum_st *a,
                      struct bignum_st *p, void *unused5, struct bignum_ctx *ctx);

/* bn_me_word.c */
BN_ME_METH *BN_ME_METH_word(void);
int bn_mont_ctx_new_word(BN_ME_METH *meth, void **out);
int bn_mont_ctx_free_word(BN_ME_WORD *ctxArg);
int bn_mont_ctx_set_word(BN_ME_WORD *ctxArg, struct bignum_st *mod, int mode, int flags, struct bignum_ctx *bnctx);

/* bn_modexp_word.c */
int bn_mod_exp_word(BN_ME_WORD *mectx, struct bignum_st *r, struct bignum_st *a,
                    struct bignum_st *p, struct bignum_ctx *ctx);

/* bn_mont_useit.c */
int bn_mont_ctx_useit(void *ctx);

/* bn_genexp.c: the exponent programs of the word method */
int BN_gen_exp_bits(struct bignum_st *e, unsigned char **out, int flags, struct bignum_ctx *ctx);
int BN_gen_exp_string(unsigned char *buf, struct bignum_st *e, int mode);

#endif
