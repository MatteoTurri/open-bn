/*
 * The multiply-accumulate word kernels of the CMP_* numbers.
 */

#include "cmp.h"


/* dst[dstStart..dstStart+count) += src[srcStart..srcStart+count) * word with a running carry, then
 * the carry ripples through the 0xFFFFFFFF words past the end. Each 32 x 32 product is built from
 * four 16 x 16 products. */
void CMP_VectorMultiply(unsigned long word, CMPInt *src, int srcStart,
                       int count, CMPInt *dst, int dstStart)
{
    unsigned long *sv;
    unsigned long *dv;
    unsigned long carry = 0;
    unsigned long hi, lo, t;
    int i;

    sv = src->value + srcStart;
    dv = dst->value + dstStart;
    for (i = 0; i < count; i++) {
        unsigned long wordLo = word & 0xffff;
        unsigned long wordHi = word >> 16;
        unsigned long sw = sv[i];
        unsigned long sHi = sw >> 16;
        unsigned long sLo = sw & 0xffff;
        unsigned long lohi = wordLo * sHi;
        unsigned long hilo = sLo * wordHi;
        unsigned long mid = hilo + lohi;
        unsigned long lolo = wordLo * sLo;
        hi = wordHi * sHi;
        if (mid < lohi)
            hi += 0x10000;
        hi += mid >> 16;
        mid <<= 16;
        lo = lolo + mid;
        if (lo < mid)
            hi++;
        t = dv[i];
        lo += t;
        if (lo < t)
            hi++;
        lo += carry;
        if (lo < carry)
            hi++;
        dv[i] = lo;
        carry = hi;
    }

    dv[i] += carry;
    if (dv[i] < carry) {
        i++;
        while (dv[i] == (unsigned long)-1) {
            dv[i] = 0;
            i++;
        }
        dv[i]++;
    }
}

/* dst += the squares of the words of a, a->value[i]^2 at dst->value[2i] and [2i + 1], with a running
 * carry: the diagonal pass of CMP_MontSquare. Same 16 x 16 products as CMP_VectorMultiply. */
void CMP_AddInTrace(CMPInt *a, CMPInt *dst)
{
    unsigned long *base;
    unsigned long *av;
    unsigned long carry = 0;
    unsigned long hi, lo, t, u;
    int i;
    int j = 0;

    av = a->value;
    base = dst->value;
    for (i = 0; i < a->length; i++, j += 2) {
        unsigned long sw = av[i];
        unsigned long sLo = sw & 0xffff;
        unsigned long sHi = sw >> 16;
        unsigned long cross = sLo * sHi;
        unsigned long mid = cross << 1;
        unsigned long sqLo = sLo * sLo;
        hi = sHi * sHi;
        if (mid < cross)
            hi += 0x10000;
        hi += mid >> 16;
        mid <<= 16;
        lo = sqLo + mid;
        if (lo < mid)
            hi++;
        t = base[j];
        lo += t;
        if (lo < t)
            hi++;
        lo += carry;
        if (lo < carry)
            hi++;
        base[j] = lo;
        u = base[j + 1];
        hi += u;
        base[j + 1] = hi;
        carry = (hi < u);
    }
    if (carry) {
        while (base[j] == (unsigned long)-1) {
            base[j] = 0;
            j++;
        }
        base[j]++;
    }
}
