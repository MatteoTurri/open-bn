/*
 * R_rand_meth_sha1: the default random generator of RCOM, on SHA-1. The state is 48 bytes, the first
 * 24 starting as 0, 1, ..., 23.
 */

#include <stdio.h>
#include "evp.h"
#include "r_rand.h"
#include "r_com.h"
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>

/* The state of the generator. */
typedef struct {
    /* 0x00 */ unsigned char buf[24];  /* starts as 0, 1, ..., 23 */
    /* 0x18 */ unsigned char *ptr;  /* the second 24-byte block of state */
    /* 0x1C */ unsigned int count;  /* mixing count, incremented by sha1_rand_seed and digested */
    /* 0x20 */ EVP_MD *md;  /* EVP_sha1_digest() */
    /* 0x24 */ EVP_MD_CTX *ctx;  /* cached digest context; 0: use a temporary one */
    /* 0x28 */ int entropy;  /* bits of entropy: command 3 reads it, command 16 adds to it */
    /* 0x2C */ int flags;  /* 2 = initialized */
} Sha1RandState; /* size = 0x30 */

static int sha1_rand_bytes(RRandMeth *rand, unsigned char *buf, int len);
static int sha1_rand_seed(RRandMeth *rand, void *data, unsigned int len);
static int sha1_rand_ctrl(RRandMeth *rand, int cmd, void *arg);

static Sha1RandState g_sha1State = {
    { 0,  1,  2,  3,  4,  5,  6,  7,
      8,  9, 10, 11, 12, 13, 14, 15,
     16, 17, 18, 19, 20, 21, 22, 23 }
};

static RRandMeth g_sha1Meth = {
    sha1_rand_bytes,
    sha1_rand_seed,
    sha1_rand_ctrl,
    &g_sha1State,
    0
};

/* The argument is not used. */
RRandMeth *R_rand_meth_sha1(void *unused)
{
    if ((g_sha1State.flags & 2) == 0) {
        int n = 2;

        g_sha1State.flags |= 2;
        g_sha1State.ptr = g_sha1State.buf;
        g_sha1State.md = EVP_sha1_digest();
        g_sha1State.ctx = 0;
        sha1_rand_ctrl(&g_sha1Meth, 16, &n);
    }
    return &g_sha1Meth;
}

/* Produces len random bytes: each round digests the state, copies the digest backwards into the
 * output until one of them runs out, then mixes the state with a second digest that includes the one
 * just used. Returns len. */
static int sha1_rand_bytes(RRandMeth *rand, unsigned char *buf, int len)
{
    int k;
    int j;
    int mdSize;
    int i;
    int n;
    unsigned int outLen;
    EVP_MD_CTX *ctx;
    unsigned char md[32];
    Sha1RandState *st;
    int seeded;

    st = rand->state;
    ctx = st->ctx;
    if (ctx == 0) {
        ctx = EVP_MD_CTX_new();
        if (ctx == 0)
            return 0;
    }

    seeded = sha1_rand_seed(rand, buf, len);
    mdSize = EVP_MD_size(st->md);

    for (i = 0; i < len; ) {
        EVP_DigestInit(ctx, st->md);
        EVP_DigestUpdate(ctx, (unsigned char *)st, 24);
        EVP_DigestUpdate(ctx, st->ptr, 24);
        EVP_DigestUpdate(ctx, (unsigned char *)&st->count, 4);
        st->count++;
        EVP_DigestFinal(ctx, md, &outLen);

        n = mdSize;
        while (i < len && n != 0) {
            n--;
            buf[i++] = md[n];
        }

        EVP_DigestInit(ctx, st->md);
        EVP_DigestUpdate(ctx, (unsigned char *)st, 24);
        EVP_DigestUpdate(ctx, st->ptr, 24);
        EVP_DigestUpdate(ctx, (unsigned char *)&st->count, 4);
        EVP_DigestUpdate(ctx, md, mdSize);
        EVP_DigestFinal(ctx, md, &outLen);

        j = (st->count * (mdSize / 2)) % 24;
        k = (mdSize / 2 + j) % 24;

        for (n = 0; n < mdSize / 2; n++) {
            st->buf[(j + n) % 24] ^= md[n];
            st->ptr[(k + n) % 24] ^= md[mdSize / 2 + n];
        }
    }

    if (st->ctx == 0)
        EVP_MD_CTX_free(ctx);
    return len;
}

/* Mixes data into the 48-byte state: SHA-1 of (data || state[0..23] || state[24..47] || count++),
 * the two halves of the digest XORed into the two 24-byte blocks at offsets derived from count.
 * Returns len. */
static int sha1_rand_seed(RRandMeth *rand, void *data, unsigned int len)
{
    int k;
    int j;
    int mdSize;
    int i;
    unsigned int outLen;
    EVP_MD_CTX *ctx;
    unsigned char md[32];
    Sha1RandState *st;

    st = rand->state;
    ctx = st->ctx;
    if (ctx == 0) {
        ctx = EVP_MD_CTX_new();
        if (ctx == 0)
            return 0;
    }

    mdSize = EVP_MD_size(st->md);
    EVP_DigestInit(ctx, st->md);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestUpdate(ctx, (unsigned char *)st, 24);
    EVP_DigestUpdate(ctx, st->ptr, 24);
    EVP_DigestUpdate(ctx, (unsigned char *)&st->count, 4);
    st->count++;
    EVP_DigestFinal(ctx, md, &outLen);

    j = (st->count * (mdSize / 2)) % 24;
    k = (mdSize / 2 + j) % 24;

    for (i = 0; i < mdSize / 2; i++) {
        st->buf[(j + i) % 24] ^= md[i];
        st->ptr[(k + i) % 24] ^= md[mdSize / 2 + i];
    }

    if (st->ctx == 0)
        EVP_MD_CTX_free(ctx);
    return len;
}

/* The ctrl of the generator: 1 installs a private copy of the state in the generator arg; 2 frees
 * the cached context and the state, if allocated (flag 1); 3 writes the entropy count to *arg; 16
 * adds entropy: when *arg is 1 up to 256 bytes from /dev/random (opened non-blocking with FIONBIO),
 * then in any case time() and one more to the count. Other commands: 0. */
static int sha1_rand_ctrl(RRandMeth *rand, int cmd, void *arg)
{
    Sha1RandState *st;
    Sha1RandState *old;
    int *out;
    int *pv;
    int ret;  /* unused, but part of the layout */
    unsigned char buf[256];
    int fd;
    long n;

    switch (cmd) {
    case 1:
        {
            if (rand == 0)
                rand = &g_sha1Meth;
            st = (Sha1RandState *)R_malloc(48);
            if (st == 0)
                return 1;
            old = rand->state;
            st->ptr = old->ptr;
            memcpy(st, old, 24);
            st->entropy = old->entropy;
            st->count = 0;
            st->md = old->md;
            st->ctx = EVP_MD_CTX_new();
            st->flags = 1;
            ((RRandMeth *)arg)->state = st;
            return 0;
        }
    case 2:
        if (rand == 0)
            rand = &g_sha1Meth;
        st = rand->state;
        if (st == 0)
            return 0;
        if (st->ctx != 0) {
            EVP_MD_CTX_free(st->ctx);
            st->ctx = 0;
        }
        if (st->flags & 1)
            R_free(rand->state);
        return 0;
    case 3:
        {
            st = rand->state;
            out = (int *)arg;
            if (arg == 0)
                return 0;
            *out = st->entropy;
            return 0;
        }
    case 16:
        {
            st = rand->state;
            pv = (int *)arg;
            if (*pv == 1) {
                fd = 0;
                n = 1;
                fd = open("/dev/random", O_RDONLY);
                if (fd < 0)
                    goto noDevice;
                n = ioctl(fd, FIONBIO, &n);
                if (n == 0) {
                    n = read(fd, buf, 256);
                    if (n > 0) {
                        sha1_rand_seed(rand, buf, n);
                        st->entropy = st->entropy + n * 8;
                    }
                }
                close(fd);
            noDevice:
                ;
            }
            time(&n);
            sha1_rand_seed(rand, &n, 4);
            st->entropy++;
            return 0;
        }
    }
    return 0;
}
