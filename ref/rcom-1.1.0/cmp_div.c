/*
 * CMP_* multiplication, division, modular reduction and greatest common divisor.
 */

#include "cmp.h"
#include "bsafe.h"


/* dst = a * b, one CMP_VectorMultiply pass per word of a, then the zero top words are trimmed. */
int CMP_Multiply(CMPInt *a, CMPInt *b, CMPInt *dst)
{
    int blen = b->length;
    int alen = a->length;
    int total = blen + alen;
    unsigned long *dv;
    int i;

    if (dst->allocated < total) {
        int rc = CMP_reallocNoCopy(total, dst);
        if (rc != 0)
            return rc;
    }
    dv = dst->value;
    T_memset(dv, 0, total * 4);

    for (i = 0; i < alen; i++)
        CMP_VectorMultiply(a->value[i], b, 0, blen, dst, i);

    while (total > 0) {
        total--;
        if (dv[total] != 0)
            break;
    }
    total++;
    dst->length = total;
    return 0;
}

/* *rem = x mod modulus and *quot = x / modulus: the loop of CMP_ModularReduce, building the quotient
 * one word at a time (shift by one word, add the estimated digit, decrement it each time the trial
 * product had to be corrected). */
int CMP_Divide(CMPInt *x, CMPInt *modulus, CMPInt *quot, CMPInt *rem)
{
    CMPInt A;
    CMPInt B;
    int shift;
    int xlen;
    int mlen;
    int i;
    int rc;

    CMP_Constructor(&A);
    CMP_Constructor(&B);

    do {
        if (quot->allocated <= 0) {
            if ((rc = CMP_reallocNoCopy(2, quot)) != 0)
                break;
        }
        quot->value[0] = 0;
        quot->length = 1;
        if (CMP_Compare(x, modulus) < 0) {
            rc = CMP_Move(x, rem);
            break;
        }
        if ((rc = CMP_GetOffsetOfMSB(modulus, &shift)) != 0)
            break;
        if (shift == 32) {
            rc = 0x106;
            break;
        }
        if (shift > 0) {
            if ((rc = CMP_ShiftLeftByBits(shift, modulus)) != 0)
                break;
            if ((rc = CMP_ShiftLeftByBits(shift, x)) != 0)
                break;
        }
        xlen = x->length;
        mlen = modulus->length;
        if ((rc = CMP_Move(x, rem)) != 0)
            break;
        if (mlen < xlen) {
            rc = CMP_ShiftRightByCMPWords(xlen - mlen, rem);
        } else {
            quot->value[0] = 1;
            quot->length = 1;
            rc = CMP_SubtractInPlace(modulus, rem);
        }
        if (rc != 0)
            break;
        if ((rc = CMP_reallocNoCopy(xlen, &B)) != 0)
            break;
        if ((rc = CMP_reallocNoCopy(2, &A)) != 0)
            break;
        i = xlen - mlen - 1;
        while (i >= 0) {
            if (CMP_Compare(rem, modulus) >= 0) {
                if ((rc = CMP_SubtractInPlace(modulus, rem)) != 0)
                    break;
                if ((rc = CMP_AddCMPWord(1, quot)) != 0)
                    break;
            }
            if ((rc = CMP_AppendWord(x->value[i], rem)) != 0)
                break;
            if ((rc = CMP_ShiftLeftByCMPWords(1, quot)) != 0)
                break;
            if (mlen < rem->length) {
                if ((rc = CMP_EstimateMSWQuotient(rem, modulus, &A, &B)) != 0)
                    break;
                while (CMP_Compare(&B, rem) > 0) {
                    if ((rc = CMP_SubtractCMPWord(1, &A)) != 0)
                        break;
                    if ((rc = CMP_SubtractInPlace(modulus, &B)) != 0)
                        break;
                }
                if (rc != 0)
                    break;
                if ((rc = CMP_AddCMPWord(A.value[0], quot)) != 0)
                    break;
                if ((rc = CMP_SubtractInPlace(&B, rem)) != 0)
                    break;
            }
            i--;
        }
        if (rc != 0)
            break;
        if (shift > 0) {
            CMP_ShiftRightByBits(shift, rem);
            CMP_ShiftRightByBits(shift, x);
            CMP_ShiftRightByBits(shift, modulus);
        }
        if (CMP_Compare(rem, modulus) < 0)
            break;
        if ((rc = CMP_SubtractInPlace(modulus, rem)) != 0)
            break;
        rc = CMP_AddCMPWord(1, quot);
    } while (0);

    CMP_Destructor(&A);
    CMP_Destructor(&B);
    return rc;
}

/* out = x mod modulus by schoolbook long division, the quotient thrown away. Both operands are
 * normalized in place (the top bit of the modulus at bit 31) and shifted back at the end. */
int CMP_ModularReduce(CMPInt *x, CMPInt *modulus, CMPInt *out)
{
    CMPInt q;
    CMPInt prod;
    int shift;
    int xlen;
    int mlen;
    int i;
    int rc;

    CMP_Constructor(&q);
    CMP_Constructor(&prod);

    do {
        if (CMP_Compare(x, modulus) < 0) {
            rc = CMP_Move(x, out);
            break;
        }
        if ((rc = CMP_GetOffsetOfMSB(modulus, &shift)) != 0)
            break;
        if (shift == 32) {
            rc = 0x107;
            break;
        }
        if (shift > 0) {
            if ((rc = CMP_ShiftLeftByBits(shift, modulus)) != 0)
                break;
            if ((rc = CMP_ShiftLeftByBits(shift, x)) != 0)
                break;
        }
        xlen = x->length;
        mlen = modulus->length;
        if ((rc = CMP_Move(x, out)) != 0)
            break;
        if (mlen < xlen)
            rc = CMP_ShiftRightByCMPWords(xlen - mlen, out);
        else
            rc = CMP_SubtractInPlace(modulus, out);
        if (rc != 0)
            break;
        if ((rc = CMP_reallocNoCopy(xlen, &prod)) != 0)
            break;
        if ((rc = CMP_reallocNoCopy(2, &q)) != 0)
            break;
        i = xlen - mlen - 1;
        while (i >= 0) {
            if (CMP_Compare(out, modulus) >= 0) {
                if ((rc = CMP_SubtractInPlace(modulus, out)) != 0)
                    break;
            }
            if ((rc = CMP_AppendWord(x->value[i], out)) != 0)
                break;
            if (mlen < out->length) {
                if ((rc = CMP_EstimateMSWQuotient(out, modulus, &q, &prod)) != 0)
                    break;
                while (CMP_Compare(&prod, out) > 0) {
                    if ((rc = CMP_SubtractInPlace(modulus, &prod)) != 0)
                        break;
                }
                if (rc != 0)
                    break;
                if ((rc = CMP_SubtractInPlace(&prod, out)) != 0)
                    break;
            }
            i--;
        }
        if (rc != 0)
            break;
        if (shift > 0) {
            CMP_ShiftRightByBits(shift, out);
            CMP_ShiftRightByBits(shift, x);
            CMP_ShiftRightByBits(shift, modulus);
        }
        if (CMP_Compare(out, modulus) < 0)
            break;
        rc = CMP_SubtractInPlace(modulus, out);
    } while (0);

    CMP_Destructor(&q);
    CMP_Destructor(&prod);
    return rc;
}

/* Prepends a new least significant word: shifts the array up by one word and writes `word` at index
 * 0, growing as needed; when the top word is 0 (a new CMPInt) index 0 is just overwritten. */
int CMP_AppendWord(unsigned long word, CMPInt *x)
{
    int len = x->length;

    if (x->value[len - 1] != 0) {
        if (x->allocated < len + 1) {
            int rc = CMP_realloc(len + 2, x);
            if (rc != 0)
                return rc;
        }
        T_memmove(x->value + 1, x->value, len * 4);
        x->value[0] = word;
        x->length++;
        return 0;
    }
    x->value[0] = word;
    x->length = 1;
    return 0;
}

/* One trial quotient word for CMP_ModularReduce: divides the two leading words of the remainder by
 * the leading word of the modulus (on scratch copies of their top words) and returns q and
 * prod = q * modulus; both are 0 when the remainder is not longer than the modulus. The returns of
 * CMP_ShiftRightByCMPWords and of the last CMP_Multiply are not checked. */

int CMP_EstimateMSWQuotient(CMPInt *rem, CMPInt *modulus, CMPInt *q, CMPInt *prod)
{
    CMPInt topRem;
    CMPInt topMod;
    unsigned long quotWord;
    unsigned long remWord;
    int mlen;
    int rlen;
    int n;
    int m;
    unsigned long dv;
    unsigned long hi;
    unsigned long lo;
    int rc;

    CMP_Constructor(&topRem);
    CMP_Constructor(&topMod);

    do {
        mlen = modulus->length;
        rlen = rem->length;
        if (mlen >= rlen) {
            if (prod->allocated <= 0) {
                if ((rc = CMP_reallocNoCopy(2, prod)) != 0)
                    break;
            }
            prod->value[0] = 0;
            prod->length = 1;
            if (q->allocated <= 0) {
                if ((rc = CMP_reallocNoCopy(2, q)) != 0)
                    break;
            }
            q->value[0] = 0;
            q->length = 1;
            rc = 0;
            break;
        }
        if ((rc = CMP_Move(rem, &topRem)) != 0)
            break;
        if ((rc = CMP_Move(modulus, &topMod)) != 0)
            break;
        n = rlen - 3;
        if (n < 0)
            n = 0;
        CMP_ShiftRightByCMPWords(n, &topRem);
        m = mlen - 2;
        if (m < 0)
            m = 0;
        CMP_ShiftRightByCMPWords(m, &topMod);
        dv = topMod.value[topMod.length - 1];
        hi = topRem.value[topRem.length - 1];
        lo = topRem.value[topRem.length - 2];
        CMP_DivideTwoWordsByWord(dv, hi, lo, &quotWord, &remWord);
        q->value[0] = quotWord;
        q->length = 1;
        CMP_Multiply(q, modulus, prod);
    } while (0);

    CMP_Destructor(&topRem);
    CMP_Destructor(&topMod);
    return rc;
}

/* (hi:lo) / divisor in two half-word steps through CMP_DivOneAndHalfWordsByWord; 0xFFFFFFFF when
 * the quotient does not fit in one word. */
void CMP_DivideTwoWordsByWord(unsigned long divisor, unsigned long hi,
                              unsigned long lo, unsigned long *quot,
                              unsigned long *rem)
{
    unsigned long q1, r1;
    unsigned long q0, r0;

    if (hi < divisor) {
        CMP_DivOneAndHalfWordsByWord(divisor, hi, lo, &q1, &r1);
        CMP_DivOneAndHalfWordsByWord(divisor, r1, lo << 16, &q0, &r0);
        *quot = (q1 << 16) + q0;
        *rem = r0;
    } else {
        *quot = 0xffffffff;
        *rem = 0;
    }
}

/* Divides the 48-bit value (hi:lo) >> 16 by divisor: the trial quotient hi / (divisor >> 16) is
 * corrected downwards, comparing q * divisor (built from 16 x 16 products) with the dividend. */
void CMP_DivOneAndHalfWordsByWord(unsigned long divisor, unsigned long hi,
                                  unsigned long lo, unsigned long *quot,
                                  unsigned long *rem)
{
    unsigned long dhi = divisor >> 16;
    unsigned long hh = hi >> 16;
    unsigned long target = (hi << 16) | (lo >> 16);
    unsigned long q = hi / dhi;
    unsigned long qlo = q & 0xffff;
    unsigned long qhi = q >> 16;
    unsigned long dlo = divisor & 0xffff;
    unsigned long p_lohi = qlo * dhi;
    unsigned long p_hihi = qhi * dhi;
    unsigned long p_hilo = dlo * qhi;
    unsigned long mid = p_hilo + p_lohi;
    unsigned long prodHi, prodLo, midLow, r;

    prodHi = p_hihi;
    if (mid < p_lohi)
        prodHi += 0x10000;
    prodHi += mid >> 16;
    midLow = mid << 16;
    prodLo = qlo * dlo + midLow;
    if (prodLo < midLow)
        prodHi++;

    while (prodHi > hh || (prodHi == hh && prodLo > target)) {
        q--;
        if (prodLo >= divisor) {
            prodLo -= divisor;
        } else {
            prodLo = ~(divisor - prodLo) + 1;
            prodHi--;
        }
    }

    if (prodLo <= target)
        r = target - prodLo;
    else
        r = ~(prodLo - target) + 1;
    *rem = r;
    *quot = q;
}

/* *out = x mod word for a modulus of 16 bits: Horner over the 16-bit halves of x from the top, with
 * R = 2^16 mod word. Larger moduli go to CMP_FullCMPWordModReduce. */
int CMP_CMPWordModularReduce(CMPInt *x, unsigned long word, unsigned long *out)
{
    unsigned long R;
    unsigned long w;
    unsigned long hi;
    unsigned long lo;
    int i;

    if (word == 0)
        return 0x107;

    if ((word & 0xffff0000) != 0)
        return CMP_FullCMPWordModReduce(x, word, out);

    R = 0x10000 % word;

    w = x->value[x->length - 1];
    hi = (w & 0xffff0000) >> 16;
    lo = w & 0xffff;
    *out = hi % word;
    *out = *out * R + lo;
    *out = *out % word;

    for (i = x->length - 2; i >= 0; i--) {
        w = x->value[i];
        hi = (w & 0xffff0000) >> 16;
        lo = w & 0xffff;
        *out = *out * R + hi;
        *out = *out % word;
        *out = *out * R + lo;
        *out = *out % word;
    }
    return 0;
}

/* *out = x mod word for a modulus wider than 16 bits: the same Horner scan over the words of x, on
 * CMPInts so that the products do not overflow. A holds the modulus, B 2^32 mod word, C the
 * remainder and D the product. */
int CMP_FullCMPWordModReduce(CMPInt *x, unsigned long word, unsigned long *out)
{
    CMPInt A;
    CMPInt B;
    CMPInt C;
    CMPInt D;
    unsigned long R;
    int i;
    int rc;

    CMP_Constructor(&A);
    CMP_Constructor(&B);
    CMP_Constructor(&C);
    CMP_Constructor(&D);

    R = -word;
    if (R >= word)
        R = R % word;

    do {
        if ((rc = CMP_reallocNoCopy(1, &A)) != 0)
            break;
        A.value[0] = word;
        A.length = 1;
        if ((rc = CMP_reallocNoCopy(1, &B)) != 0)
            break;
        B.value[0] = R;
        B.length = 1;
        if ((rc = CMP_reallocNoCopy(2, &D)) != 0)
            break;
        D.value[0] = x->value[x->length - 1];
        D.length = 1;
        if ((rc = CMP_ModularReduce(&D, &A, &C)) != 0)
            break;
        i = x->length - 2;
        while (i >= 0
               && (rc = CMP_Multiply(&B, &C, &D)) == 0
               && (rc = CMP_AddCMPWord(x->value[i], &D)) == 0) {
            if ((rc = CMP_ModularReduce(&D, &A, &C)) != 0)
                break;
            i--;
        }
    } while (0);

    if (rc == 0)
        *out = C.value[0];
    CMP_Destructor(&A);
    CMP_Destructor(&B);
    CMP_Destructor(&C);
    CMP_Destructor(&D);
    return rc;
}

/* gcd(a, b) into *out by Euclid, rotating A, out and B through the arguments of
 * CMP_ModularReduce. `state` tells which of the three holds the gcd when the loop stops; 3 means
 * that it never ran (an operand was zero): error 0x10A. */
int CMP_ComputeGCD(CMPInt *a, CMPInt *b, CMPInt *out)
{
    CMPInt A;
    CMPInt B;
    int state;
    int rc;

    CMP_Constructor(&A);
    CMP_Constructor(&B);

    do {
        if (CMP_Compare(a, b) >= 0) {
            if ((rc = CMP_Move(a, &A)) != 0)
                break;
            if ((rc = CMP_Move(b, out)) != 0)
                break;
        } else {
            if ((rc = CMP_Move(b, &A)) != 0)
                break;
            if ((rc = CMP_Move(a, out)) != 0)
                break;
        }
        state = 3;
        while (out->value[out->length - 1] != 0) {
            if ((rc = CMP_ModularReduce(&A, out, &B)) != 0)
                break;
            state = 0;
            if (B.value[B.length - 1] == 0)
                break;
            if ((rc = CMP_ModularReduce(out, &B, &A)) != 0)
                break;
            state = 2;
            if (A.value[A.length - 1] == 0)
                break;
            if ((rc = CMP_ModularReduce(&B, &A, out)) != 0)
                break;
            state = 1;
        }
        if (rc != 0)
            break;
        switch (state) {
        case 1:
            rc = CMP_Move(&A, out);
            break;
        case 2:
            rc = CMP_Move(&B, out);
            break;
        case 3:
            if (A.value[A.length - 1] != 0)
                rc = CMP_Move(&A, out);
            else
                rc = 0x10A;
            break;
        }
    } while (0);

    CMP_Destructor(&A);
    CMP_Destructor(&B);
    return rc;
}

/* The extended Euclid: gcd(a, b) into *gcd and the Bezout coefficients into *u and *v. The
 * remainders rotate through A, B and *gcd; CMP_UpdateCoefficients carries the coefficients with
 * their signs, since CMPInts are magnitudes. A and B start as the larger and the smaller operand;
 * when that swapped them the two coefficients are exchanged at the end. */
int CMP_ComputeExtendedGCD(CMPInt *a, CMPInt *b, CMPInt *u, CMPInt *v,
                           CMPInt *gcd)
{
    CMPInt A;
    CMPInt B;
    CMPInt C;
    CMPInt D;
    CMPInt E;
    int signA;
    int signB;
    int noswap = 0;
    CMPInt *big;
    CMPInt *small;
    int state;
    int rc;

    signB = 1;
    signA = 1;
    CMP_Constructor(&A);
    CMP_Constructor(&B);
    CMP_Constructor(&C);
    CMP_Constructor(&D);
    CMP_Constructor(&E);

    do {
        if (CMP_Compare(a, b) >= 0) {
            big = a;
            small = b;
            if ((rc = CMP_Move(big, &A)) != 0)
                break;
            if ((rc = CMP_Move(b, &B)) != 0)
                break;
            noswap = 1;
        } else {
            small = a;
            big = b;
            if ((rc = CMP_Move(big, &A)) != 0)
                break;
            if ((rc = CMP_Move(a, &B)) != 0)
                break;
            noswap = 0;
        }
        if ((rc = CMP_reallocNoCopy(1, &C)) != 0)
            break;
        C.length = 1;
        C.value[0] = 1;
        if (u->allocated <= 0) {
            if ((rc = CMP_reallocNoCopy(1, u)) != 0)
                break;
        }
        u->length = 1;
        u->value[0] = 0;
        if (small->value[small->length - 1] == 0) {
            if (big->value[big->length - 1] == 0) {
                rc = 0x10A;
                break;
            }
            if ((rc = CMP_Move(big, gcd)) != 0)
                break;
            if (v->allocated <= 0) {
                if ((rc = CMP_reallocNoCopy(1, v)) != 0)
                    break;
            }
            v->length = 1;
            v->value[0] = 1;
            break;
        }
        if ((rc = CMP_Divide(&A, &B, v, gcd)) != 0)
            break;
        if (gcd->value[gcd->length - 1] == 0) {
            v->length = 1;
            v->value[0] = 1;
            rc = CMP_Move(small, gcd);
            break;
        }
        do {
            if ((rc = CMP_UpdateCoefficients(&D, &E, v, &signA, &signB, &C, u)) != 0)
                break;
            if ((rc = CMP_Divide(&B, gcd, v, &A)) != 0)
                break;
            state = 1;
            if (A.value[A.length - 1] == 0)
                break;
            if ((rc = CMP_UpdateCoefficients(&D, &E, v, &signA, &signB, &C, u)) != 0)
                break;
            if ((rc = CMP_Divide(gcd, &A, v, &B)) != 0)
                break;
            state = 2;
            if (B.value[B.length - 1] == 0)
                break;
            if ((rc = CMP_UpdateCoefficients(&D, &E, v, &signA, &signB, &C, u)) != 0)
                break;
            if ((rc = CMP_Divide(&A, &B, v, gcd)) != 0)
                break;
            state = 0;
        } while (gcd->value[gcd->length - 1] != 0);
        if (rc != 0)
            break;
        switch (state) {
        case 0:
            CMP_Move(&B, gcd);
            break;
        case 2:
            CMP_Move(&A, gcd);
            break;
        }
        if ((rc = CMP_Multiply(u, big, &D)) != 0)
            break;
        if (signB > 0)
            rc = CMP_Subtract(&D, gcd, &E);
        else
            rc = CMP_Add(&D, gcd, &E);
        if (rc != 0)
            break;
        if ((rc = CMP_Divide(&E, small, v, &D)) != 0)
            break;
        if (signB > 0) {
            if ((rc = CMP_Subtract(big, v, &E)) != 0)
                break;
            rc = CMP_Move(&E, v);
        } else {
            if ((rc = CMP_Subtract(small, u, &E)) != 0)
                break;
            rc = CMP_Move(&E, u);
        }
    } while (0);

    if (noswap == 0 && rc == 0) {
        do {
            if ((rc = CMP_Move(u, &D)) != 0)
                break;
            if ((rc = CMP_Move(v, u)) != 0)
                break;
            rc = CMP_Move(&D, v);
        } while (0);
    }

    CMP_Destructor(&A);
    CMP_Destructor(&B);
    CMP_Destructor(&C);
    CMP_Destructor(&D);
    CMP_Destructor(&E);
    return rc;
}

/* One step of the coefficient recurrence: (A, B) <- (B, A - q * B) on the signed pairs (A, signA)
 * and (B, signB), T and C as scratch: an add when the signs differ, an absolute difference when they
 * agree. The return of CMP_AbsDifference is not checked. */
int CMP_UpdateCoefficients(CMPInt *A, CMPInt *B, CMPInt *q, int *signA,
                           int *signB, CMPInt *C, CMPInt *T)
{
    int sign;
    int oldSignA;
    int rc;

    oldSignA = *signA;
    *signA = *signB;
    do {
        if ((rc = CMP_Multiply(q, T, B)) != 0)
            break;
        if (*signB != oldSignA) {
            if ((rc = CMP_Add(C, B, A)) != 0)
                break;
            *signB = -*signB;
        } else {
            if (CMP_AbsDifference(C, B, &sign, A) != 0)
                break;
            *signB = *signB * sign;
        }
        if ((rc = CMP_Move(T, C)) != 0)
            break;
        rc = CMP_Move(A, T);
    } while (0);
    return rc;
}

/* dst = |a - b|; *sign = CMP_Compare(a, b), -1 when a < b. */
int CMP_AbsDifference(CMPInt *a, CMPInt *b, int *sign, CMPInt *dst)
{
    int cmp = CMP_Compare(a, b);
    if (cmp >= 0) {
        *sign = 1;
        return CMP_Subtract(a, b, dst);
    }
    *sign = -1;
    return CMP_Subtract(b, a, dst);
}
