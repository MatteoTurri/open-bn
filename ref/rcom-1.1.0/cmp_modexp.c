/*
 * CMP_* modular exponentiation: the windowed ladder in Montgomery form, the CRT forms for RSA and the
 * shortcuts for the small exponents.
 */

#include "cmp.h"
#include "bsafe.h"


#include "../../src/rcom/checksurrender.h"


/* The windowed modular exponentiation: checks base < mod, shortcuts the small exponents, computes
 * the Montgomery coefficient and the window table, then CMP_ModExpPrecomp1; the table is wiped and
 * freed. */

int CMP_ModExp(CMPInt *base, CMPInt *e, CMPInt *mod, CMPInt *out, void *ctx)
{
    int n0;
    int wbits;
    void *table;
    int tableLen;
    int extra;
    int ebits;
    int rc;

    tableLen = 0;
    extra = 0;
    table = 0;

    do {
        if ((rc = CheckSurrender(ctx)) != 0)
            break;
        if (CMP_Compare(base, mod) >= 0) {
            rc = 0x108;
            break;
        }
        ebits = CMP_BitLengthOfCMPInt(e);
        if (ebits < 2)
            return CMP_ModExpSpecialExponent(base, e, mod, out, ebits, ctx);
        if ((rc = CMP_ComputeMontCoeff(mod, &n0)) != 0)
            break;
        if ((rc = CMP_ComputeWindowTable(e, &wbits, (unsigned long **)&table,
                                         &tableLen, &extra)) != 0)
            break;
        rc = CMP_ModExpPrecomp1(base, (unsigned long *)table, tableLen, wbits,
                                extra, mod, n0, out, ctx);
    } while (0);

    if (table != 0) {
        T_memset(table, 0, tableLen * 4);
        T_free(table);
    }
    return rc;
}

/* Builds the table of the odd powers of the base in Montgomery form (b, b^3, b^5, ...) up to
 * maxWindow, then hands the walk of the exponent to CMP_ModExpPrecomp2. */
int CMP_ModExpPrecomp1(CMPInt *base, unsigned long *table, int tableLen,
                       int wbits, int maxWindow, CMPInt *mod, unsigned long n0,
                       CMPInt *out, void *ctx)
{
    CMPInt T;
    CMPInt *tbl;
    CMPInt *q;
    CMPInt *pp;
    int count;
    int i;
    int k;
    int rc;

    CMP_Constructor(&T);
    do {
        count = 1 << (wbits - 1);
        tbl = (CMPInt *)T_malloc(count * sizeof(CMPInt));
        if (tbl == 0) {
            rc = 0x100;
            count = 0;
            break;
        }
        for (i = 0; i < count; i++)
            CMP_Constructor(&tbl[i]);
        if ((rc = CMP_ConvertToMont(base, mod, &tbl[0])) != 0)
            break;
        if (maxWindow != 1) {
            if ((rc = CMP_MontSquare(&tbl[0], mod, n0, &T)) != 0)
                break;
            if ((rc = CheckSurrender(ctx)) != 0)
                break;
            q = tbl + 1;
            pp = tbl;
            for (i = 1, k = 3; i < count && k <= maxWindow; pp++, k += 2, i++) {
                if ((rc = CMP_MontProduct(pp, &T, mod, n0, q)) != 0)
                    break;
                q++;
            }
            if (rc != 0)
                break;
        }
        rc = CMP_ModExpPrecomp2(tbl, count, table, tableLen, n0, &T, mod, out,
                                ctx);
    } while (0);

    if (tbl != 0) {
        for (i = 0; i < count; i++)
            CMP_Destructor(&tbl[i]);
        T_free(tbl);
    }
    CMP_Destructor(&T);
    return rc;
}

/* Walks the (bits used, window value) pairs of CMP_ComputeWindowTable: squares once per bit and
 * multiplies by tbl[(value - 1) / 2] at each window. The accumulator alternates between *t and *out
 * (cur says which), and the final conversion from Montgomery form follows it. count is not used. */
int CMP_ModExpPrecomp2(CMPInt *tbl, int count, unsigned long *table,
                       int tableLen, unsigned long n0, CMPInt *t, CMPInt *mod,
                       CMPInt *out, void *ctx)
{
    unsigned long *p;
    int k;
    int used;
    int val;
    int cur;
    int rc;

    do {
        if ((rc = CheckSurrender(ctx)) != 0)
            break;
        if ((rc = CMP_Move(tbl, t)) != 0)
            break;
        cur = 0;
        k = 2;
        p = table + 2;
        used = table[0];
        val = table[1];
        do {
            if ((rc = CheckSurrender(ctx)) != 0)
                break;
            if (cur == 1) {
                if ((rc = CMP_MontSquare(out, mod, n0, t)) != 0)
                    break;
                used--;
                cur = 0;
            }
            while (used > 0) {
                if ((rc = CMP_MontSquare(t, mod, n0, out)) != 0)
                    break;
                used--;
                cur = 1;
                if (used <= 0)
                    break;
                if ((rc = CMP_MontSquare(out, mod, n0, t)) != 0)
                    break;
                used--;
                cur = 0;
            }
            if (rc != 0)
                break;
            if (val != 0) {
                val = (val - 1) >> 1;
                if (cur == 0) {
                    if ((rc = CMP_MontProduct(&tbl[val], t, mod, n0, out)) != 0)
                        break;
                    cur = 1;
                } else {
                    if ((rc = CMP_MontProduct(&tbl[val], out, mod, n0, t)) != 0)
                        break;
                    cur = 0;
                }
            }
            if (k >= tableLen)
                break;
            used = p[0];
            val = p[1];
            p += 2;
            k += 2;
        } while (used != 0);
        if (rc != 0)
            break;
        if (cur == 0) {
            rc = CMP_ConvertFromMont(t, mod, n0, out);
            break;
        }
        if ((rc = CMP_ConvertFromMont(out, mod, n0, t)) != 0)
            break;
        rc = CMP_Move(t, out);
    } while (0);
    return rc;
}

/* Two-prime RSA CRT exponentiation, through the multi-prime routine with two-element arrays. */
int CMP_ModExpCRT(CMPInt *out, CMPInt *p, CMPInt *q, CMPInt *dp, CMPInt *dq,
                  CMPInt *coeff, CMPInt *base, void *ctx)
{
    CMPInt primes[2];
    CMPInt exps[2];

    primes[0] = *p;
    primes[1] = *q;
    exps[0] = *dp;
    exps[1] = *dq;
    return CMP_ModExpMultiPrimeCRT(out, 2, primes, exps, coeff, base, ctx);
}

/* Multi-prime RSA CRT exponentiation, recombined with Garner's algorithm. The two leading primes and
 * exponents are swapped on entry and back on exit. r[] holds the residues, r[0] being *out. The
 * results of the multiply and add into r[0] are tested but not kept in rc: an error there returns
 * 0. */
int CMP_ModExpMultiPrimeCRT(CMPInt *base, int nprimes, CMPInt *primes,
                            CMPInt *exps, CMPInt *coeff, CMPInt *out, void *ctx)
{
    CMPInt B;
    CMPInt A;
    int sign;
    CMPInt **r;
    int i;
    int rc;

    CMP_Constructor(&A);
    CMP_Constructor(&B);
    CMP_SwapContents(primes, primes + 1);
    r = 0;
    CMP_SwapContents(exps, exps + 1);

    do {
        if ((rc = CheckSurrender(ctx)) != 0)
            break;
        r = (CMPInt **)T_malloc(nprimes * 4);
        if (r == 0) {
            rc = 0x100;
            break;
        }
        T_memset(r, 0, nprimes * 4);
        for (i = 0; i < nprimes; i++) {
            if (i == 0) {
                r[0] = out;
            } else {
                r[i] = (CMPInt *)T_malloc(sizeof(CMPInt));
                if (r[i] == 0) {
                    rc = 0x100;
                    break;
                }
                CMP_Constructor(r[i]);
            }
            if ((rc = CMP_ModularReduce(base, &primes[i], &A)) != 0)
                break;
            if ((rc = CMP_ModExp(&A, &exps[i], &primes[i], r[i], ctx)) != 0)
                break;
        }
        if (rc != 0)
            break;
        if ((rc = CMP_Move(primes, &B)) != 0)
            break;
        i = 1;
        for (;;) {
            if ((rc = CMP_SubtractSigned(r[i], 1, r[0], 1, &A, &sign)) != 0)
                break;
            if (sign < 0) {
                if (CMP_Compare(&primes[i], &A) >= 0) {
                    if ((rc = CMP_Subtract(&primes[i], &A, r[i])) != 0)
                        break;
                } else {
                    if ((rc = CMP_ModularReduce(&A, &primes[i], r[i])) != 0)
                        break;
                    if ((rc = CMP_Subtract(&primes[i], r[i], &A)) != 0)
                        break;
                    CMP_SwapContents(&A, r[i]);
                }
            } else {
                CMP_SwapContents(&A, r[i]);
            }
            if ((rc = CMP_Multiply(r[i], &coeff[i - 1], &A)) != 0)
                break;
            if ((rc = CMP_ModularReduce(&A, &primes[i], r[i])) != 0)
                break;
            if (CMP_Multiply(&B, r[i], &A) != 0)
                break;
            if (CMP_AddInPlace(&A, r[0]) != 0)
                break;
            if (i >= nprimes - 1)
                break;
            if ((rc = CMP_Multiply(&B, &primes[i], &A)) != 0)
                break;
            if ((rc = CMP_SwapContents(&B, &A)) != 0)
                break;
            i++;
        }
    } while (0);

    if (r != 0) {
        for (i = 1; i < nprimes; i++) {
            if (r[i] != 0) {
                CMP_Destructor(r[i]);
                T_free(r[i]);
            }
        }
        T_free(r);
    }
    CMP_Destructor(&A);
    CMP_Destructor(&B);
    CMP_SwapContents(primes, primes + 1);
    CMP_SwapContents(exps, exps + 1);
    return rc;
}

/* The exponents 0, 1, 2 and 3, by bit length and low word. */
int CMP_ModExpSpecialExponent(CMPInt *base, CMPInt *e, CMPInt *mod, CMPInt *out,
                              int ebits, void *ctx)
{
    if (ebits < 2) {
        if (e->value[0] == 0)
            return CMP_CMPWordToCMPInt(1, out);
        return CMP_Move(base, out);
    }
    if (e->value[0] == 3)
        return CMP_ModCube(base, mod, out, ctx);
    return CMP_ModMultiply(base, base, mod, out);
}

/* out = base^3 mod mod: two multiply-and-reduce steps with a CheckSurrender between them. */
int CMP_ModCube(CMPInt *base, CMPInt *mod, CMPInt *out, void *ctx)
{
    CMPInt t;
    CMPInt u;
    int rc;

    CMP_Constructor(&t);
    CMP_Constructor(&u);
    do {
        if ((rc = CMP_Multiply(base, base, &t)) != 0)
            break;
        if ((rc = CMP_Divide(&t, mod, &u, out)) != 0)
            break;
        if ((rc = CheckSurrender(ctx)) != 0)
            break;
        if ((rc = CMP_Multiply(base, out, &t)) != 0)
            break;
        rc = CMP_Divide(&t, mod, &u, out);
    } while (0);
    CMP_Destructor(&t);
    CMP_Destructor(&u);
    return rc;
}

/* Scans the exponent from the top with CMP_GetBitsAtIndex and stores (bits used, window value) for
 * each window in a new array; *maxWindow gets the largest window value. 0x104 when *table is not 0,
 * 0x100 when the allocation fails. */
int CMP_ComputeWindowTable(CMPInt *e, int *wbits, unsigned long **table,
                           int *tableLen, int *maxWindow)
{
    int bits;
    int wsize;
    int n;
    int i;
    int idx;
    int best;
    int rc;
    int used;
    int val;
    int u;

    best = 0;
    do {
        if (*table != 0) {
            rc = 0x104;
            break;
        }
        bits = CMP_BitLengthOfCMPInt(e);
        wsize = CMP_GetWindowSize(bits);
        *wbits = wsize;
        n = ((bits + wsize - 2) / wsize) * 2 + 2;
        *tableLen = n;
        *table = (unsigned long *)T_malloc(n * 4);
        if (*table == 0) {
            *tableLen = 0;
            rc = 0x100;
            break;
        }
        T_memset(*table, 0, *tableLen * 4);
        i = 0;
        idx = bits - 2;
        do {
            if ((rc = CMP_GetBitsAtIndex(e, idx, *wbits, &used, &val)) != 0)
                break;
            u = used;
            (*table)[i] = u;
            (*table)[i + 1] = val;
            i += 2;
            idx -= u;
            if (best < val)
                best = val;
        } while (idx >= 0);
    } while (0);
    *maxWindow = best;
    return rc;
}

/* The window size for an exponent of the given bit length. */
int CMP_GetWindowSize(int bits)
{
    if (bits >= 1020)
        return 6;
    if (bits >= 382)
        return 5;
    if (bits >= 80)
        return 4;
    if (bits > 18)
        return 3;
    return 2;
}

/* One window of the exponent from bit bitIndex down: skips the zero bits (counted in *outUsed),
 * takes up to maxBits bits from the first 1 and makes the window odd by dropping its trailing
 * zeros. */
int CMP_GetBitsAtIndex(CMPInt *x, int bitIndex, int maxBits, int *outUsed,
                       int *outValue)
{
    unsigned long word;
    int wordIdx;
    int bitInWord;
    int count;

    wordIdx = bitIndex / 32;
    bitInWord = bitIndex - wordIdx * 32;
    word = x->value[wordIdx];
    wordIdx--;
    count = 0;
    word <<= 31 - bitInWord;
    while ((word & 0x80000000) == 0) {
        count++;
        bitIndex--;
        bitInWord--;
        if (bitIndex < 0) {
            *outUsed = count;
            *outValue = word >> 31;
            return 0;
        }
        if (bitInWord < 0) {
            word = x->value[wordIdx];
            wordIdx--;
            bitInWord = 31;
        } else {
            word <<= 1;
        }
    }
    bitIndex = bitInWord + 1;
    if (bitIndex < maxBits) {
        if (wordIdx >= 0)
            word |= x->value[wordIdx] >> bitIndex;
        else
            maxBits = bitIndex;
    }
    word >>= 32 - maxBits;
    while ((word & 1) == 0) {
        word >>= 1;
        maxBits--;
    }
    *outUsed = count + maxBits;
    *outValue = word;
    return 0;
}
