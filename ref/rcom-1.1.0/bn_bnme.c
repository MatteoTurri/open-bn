/*
 * The cache of the modular exponentiation methods (BN_bnme_*): up to 32 entries
 * {getMeth, minBits, maxBits, flags} sorted by minBits, and BN_library_init, which fills it from a
 * table.
 */

#include "bn_me.h"
#include <string.h>

#define BNME_MAX 32


static BNME_ENTRY g_bnme_array[BNME_MAX];
static int g_bnme_count = 0;

/* Returns entry idx of the cache, 0 past the end. */

void *BN_bnme_get_info(int idx)
{
    if (idx >= g_bnme_count)
        return 0;
    else
        return &g_bnme_array[idx];
}

int BN_bnme_delete(int idx)
{
    int i;

    if (idx < g_bnme_count) {
        i = idx;
        while (i + 1 < g_bnme_count) {
            memcpy(&g_bnme_array[i], &g_bnme_array[i + 1], 16);
            i++;
        }
        g_bnme_count = g_bnme_count - 1;
        return 0;
    }
    return 1;
}

/* Inserts at idx, moving the following entries up. */

int BN_bnme_insert(void *getMeth, int minBits, int maxBits, int flags, int idx)
{
    int i;

    if (g_bnme_count >= BNME_MAX)
        return 1;
    i = g_bnme_count;
    while (i > idx) {
        memcpy(&g_bnme_array[i], &g_bnme_array[i - 1], 16);
        i = i - 1;
    }
    g_bnme_array[i].getMeth = getMeth;
    g_bnme_array[i].minBits = minBits;
    g_bnme_array[i].maxBits = maxBits;
    g_bnme_array[i].flags = flags;
    g_bnme_count = g_bnme_count + 1;
    return 0;
}


void BN_bnme_clear(void)
{
    g_bnme_count = 0;
}

/* Registers the method getter getMeth (called without arguments, it returns the BN_ME_METH) for
 * [minBits, maxBits], keeping the cache sorted by minBits and resolving the overlaps with the
 * following entries. Flags: 0x2 the bounds are in bits (otherwise in words, converted), 0x1 soft.
 * A bound of -1 comes from meth->nwords (0 and 0x7FFFFFFF when nwords is 0). */
int BN_bnme_set(getMeth,minBits,maxBits,flags)
void *getMeth;
int minBits;
int maxBits;
int flags;
{
    int i;
    int soft;
    int ret;
    int eflags;
    int best;
    BN_ME_METH *meth;

    best = -1;
    meth = (*(BN_ME_METH *(*)(void))getMeth)();
    ret = (*meth->initFn)(meth);
    if (ret != 0)
        return ret;
    if (!(flags & 2)) {
        if (minBits != -1)
            minBits = minBits * 32 - 31;
        if (maxBits != -1)
            maxBits = maxBits * 32;
    }
    if (minBits == -1 || maxBits == -1) {
        if (minBits == -1)
            minBits = (meth->nwords != 0) ? ((meth->nwords - 1) * 32 + 1) : 0;
        if (maxBits == -1)
            maxBits = (meth->nwords != 0) ? (meth->nwords * 32) : 0x7fffffff;
    }
    flags |= 2;
    soft = flags & 1;
    best = -1;
    for (i = 0; i < g_bnme_count; i++) {
        eflags = g_bnme_array[i].flags & 1;
        if (eflags == 0 && best == -1)
            best = i;
        if ((soft && eflags) == soft) {
            if (g_bnme_array[i].minBits >= minBits)
                break;
        }
    }
    if (i == g_bnme_count) {
        if (soft)
            i = (best != -1) ? best : 0;
        ret = BN_bnme_insert(getMeth, minBits, maxBits, flags, i);
        return ret;
    } else {
        ret = BN_bnme_insert(getMeth, minBits, maxBits, flags, i);
        if (ret != 0)
            return ret;
        best = best + 1;
        i = i + 1;
        if (i == g_bnme_count)
            return 0;
        if (soft) {
            for (;;) {
                eflags = g_bnme_array[i].flags & 1;
                if (eflags != soft) {
                    break;
                } else {
                    if (g_bnme_array[i].maxBits <= maxBits) {
                        BN_bnme_delete(i);
                        best = best - 1;
                    } else {
                        if (g_bnme_array[i].minBits <= maxBits) {
                            g_bnme_array[i].minBits = maxBits + 1;
                            break;
                        }
                    }
                    if (i >= best)
                        break;
                }
            }
        } else {
            for (;;) {
                if (g_bnme_array[i].maxBits <= maxBits) {
                    BN_bnme_delete(i);
                } else {
                    if (g_bnme_array[i].minBits <= maxBits) {
                        g_bnme_array[i].minBits = maxBits + 1;
                        break;
                    } else
                        break;
                }
                if (i >= g_bnme_count)
                    break;
            }
        }
    }
    return 0;
}

/* Returns the getter of the first entry whose range holds bits (words unless flags has 0x2); a
 * soft entry is remembered and the search goes on. */

void *BN_bnme_get(int bits, int flags)
{
    int i;
    int best;

    best = -1;
    if (!(flags & 2))
        bits = bits * 32;
    for (i = 0; i < g_bnme_count; i++) {
        if (!(g_bnme_array[i].maxBits < bits)) {
            if (!(g_bnme_array[i].minBits > bits)) {
                if (!(g_bnme_array[i].flags & 1)) {
                    return g_bnme_array[i].getMeth;
                } else {
                    best = i;
                    continue;
                }
            }
        }
        if (!(g_bnme_array[i].maxBits < bits))
            break;
    }
    if (best != -1) {
        return g_bnme_array[best].getMeth;
    } else {
        return 0;
    }
}

int BN_library_init(BNME_ENTRY *table)
{
    int i;

    i = 0;
    while (table[i].getMeth != 0) {
        BN_bnme_set(table[i].getMeth,
                    table[i].minBits,
                    table[i].maxBits,
                    table[i].flags);
        i = i + 1;
    }
    return 0;
}
