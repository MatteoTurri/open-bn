/*
 * The random generator objects of RCOM (R_rand_*): a 20-byte object of three methods (bytes, seed,
 * ctrl), a state and flags. ctrl commands: 1 init, 2 cleanup, 3 entropy count, 16 add entropy.
 */

#include "r_rand.h"

#include "r_com.h"
#include <string.h>

int R_rand_set_default(RRandMeth *meth)
{
    RRandMeth *old = R_rand_get_default();
    R_rand_free(old);
    memcpy(old, meth, 20);
    return 0;
}

int R_rand_seed(RRandMeth *rand, void *data, unsigned int len)
{
    if (rand == 0 || rand->seedFn == 0)
        return -1;
    else
        return rand->seedFn(rand, data, len);
}

int R_rand_bytes(RRandMeth *rand, void *buf, unsigned int len)
{
    if (rand == 0 || rand->bytesFn == 0)
        return -1;
    else
        return rand->bytesFn(rand, buf, len);
}


/* Copies meth into a new 20-byte object and initializes it with ctrlFn(new, 1, new); with meth 0
 * it calls itself with the default generator. Flag 1 is set before the copy, which overwrites it. A
 * nonzero result of the ctrl is a failure: the object is freed and 0 returned. */
RRandMeth *R_rand_new(meth)
    RRandMeth *meth;
{
    RRandMeth *r;
    int rc;

    r = 0;

    if (meth == 0)
        return R_rand_new(R_rand_get_default());

    if ((r = (RRandMeth *)R_malloc(20)) == 0)
        return 0;

    memset(r, 0, 20);
    r->flags = r->flags | 1;

    if (meth != 0) {
        memcpy(r, meth, 20);
        r->state = 0;
        if (meth->ctrlFn != 0) {
            rc = meth->ctrlFn(meth, 1, r);
            if (rc != 0) {
                R_free(r);
                r = 0;
            }
        }
    }

    return r;
}

void R_rand_free(RRandMeth *rand)
{
    if (rand == 0)
        return;
    if (rand->ctrlFn != 0)
        rand->ctrlFn(rand, 2, rand);
    if (rand->flags & 1)
        R_free(rand);
}

int R_rand_lib_cleanup(void *unused0, void *unused1)
{
    R_rand_free(R_rand_get_default());
    return 0;
}

int R_rand_add_entropy(RRandMeth *rand, int source)
{
    if (rand == 0 || rand->ctrlFn == 0)
        return 0;
    return rand->ctrlFn(rand, 16, &source);
}

int R_rand_entropy_count(RRandMeth *rand, int *outCount)
{
    int count;
    int ctrlResult;

    ctrlResult = 0;
    if (rand == 0 || rand->ctrlFn == 0) {
        count = 0;
    } else {
        ctrlResult = rand->ctrlFn(rand, 3, &count);
    }
    if (outCount != 0)
        *outCount = count;
    return ctrlResult;
}
