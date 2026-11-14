/*
 * The Montgomery kernel of the CMP_* numbers: conversions, the coefficient, product and square.
 */

#include "cmp.h"
#include "bsafe.h"


/* out = a * R mod n, R = 2^(32 * n->length): the Montgomery form of a. a < n is checked inline. */
int CMP_ConvertToMont(CMPInt *a, CMPInt *n, CMPInt *out)
{
    CMPInt t;
    int nlen = n->length;
    int alloc;
    int rc;
    int i;

    if (nlen < a->length)
        return 0x108;
    if (nlen == a->length) {
        i = nlen - 1;
        while (i >= 0 && n->value[i] == a->value[i])
            i--;
        if (i < 0)
            return 0x108;
        if (n->value[i] < a->value[i])
            return 0x108;
    }

    alloc = nlen * 2 + 1;
    CMP_Constructor(&t);
    do {
        if ((rc = CMP_reallocNoCopy(alloc, &t)) != 0)
            break;
        if ((rc = CMP_Move(a, &t)) != 0)
            break;
        if ((rc = CMP_ShiftLeftByCMPWords(nlen, &t)) != 0)
            break;
        rc = CMP_ModularReduce(&t, n, out);
    } while (0);
    CMP_Destructor(&t);
    return rc;
}

/* out = mont_in / R mod n: one CMP_MontProduct with 1. */
int CMP_ConvertFromMont(CMPInt *mont_in, CMPInt *n, unsigned long n0, CMPInt *out)
{
    CMPInt one;
    int rc;

    CMP_Constructor(&one);
    do {
        rc = CMP_reallocNoCopy(1, &one);
        if (rc != 0)
            break;
        one.value[0] = 1;
        one.length = 1;
        rc = CMP_MontProduct(mont_in, &one, n, n0, out);
    } while (0);
    CMP_Destructor(&one);
    return rc;
}

/* The Montgomery coefficient n0' with n * n0' == -1 mod 2^32, by bit-by-bit Hensel lifting from a
 * 2-bit guess (odd moduli only). */
int CMP_ComputeMontCoeff(CMPInt *n, int *out)
{
    unsigned long lsw = n->value[0];
    unsigned long bit, mask, inv, prod;
    int i;

    if ((lsw & 1) == 0)
        return 0x107;

    inv = 1;
    bit = 2;
    mask = 3;
    prod = (lsw * inv) & mask;
    if (bit < prod)
        inv += bit;

    for (i = 0x1d; i >= 0; i--) {
        bit <<= 1;
        mask |= bit;
        prod = (lsw * inv) & mask;
        inv += (bit < prod) ? bit : 0;
    }

    *out = ~inv + 1;
    return 0;
}

/* Montgomery square: out = a^2 * R^-1 mod n. The i < j cross products once, doubled with a one-bit
 * shift, plus the squares (CMP_AddInTrace), then the Montgomery reduction. */
int CMP_MontSquare(CMPInt *a, CMPInt *n, unsigned long n0, CMPInt *out)
{
    int nlen = n->length;
    int len2 = nlen * 2;
    unsigned long *ov;
    int alen;
    int cmp;
    int i;
    int rc;

    alen = a->length;
    if (out->allocated < len2 + 3) {
        rc = CMP_reallocNoCopy(len2 + 3, out);
        if (rc != 0)
            return rc;
    }
    ov = out->value;
    T_memset(ov, 0, (len2 + 3) * 4);
    out->length = 1;

    for (i = 0; i < alen - 1; i++) {
        int rest = a->length - 1;
        CMP_VectorMultiply(a->value[i], a, i + 1, rest - i, out, 2 * i + 1);
    }

    if ((rc = CMP_RecomputeLength(len2, out)) != 0)
        return rc;
    if ((rc = CMP_ShiftLeftByBits(1, out)) != 0)
        return rc;
    CMP_AddInTrace(a, out);

    for (i = 0; i < nlen; i++)
        CMP_VectorMultiply(ov[i] * n0, n, 0, nlen, out, i);

    out->length = len2 + 1;
    CMP_ShiftRightByCMPWords(nlen, out);
    CMP_RecomputeLength(nlen, out);
    cmp = CMP_Compare(n, out);
    if (cmp < 0) {
        if ((rc = CMP_SubtractInPlace(n, out)) != 0)
            return rc;
    } else if (cmp == 0) {
        out->value[0] = 0;
        out->length = 1;
    }
    return 0;
}

/* The Montgomery product out = a * b * R^-1 mod n, n0 = -n^-1 mod 2^32: for each word of a, add
 * a[i] * b, then (out[i] * n0) * n, which clears out[i]; then the shift by n->length words and one
 * conditional subtraction. */
int CMP_MontProduct(CMPInt *a, CMPInt *b, CMPInt *n, unsigned long n0, CMPInt *out)
{
    int nlen = n->length;
    int len2 = nlen * 2;
    unsigned long *ov;
    unsigned long *av;
    int alen;
    int i;
    int rc;

    if (out->allocated < len2 + 3) {
        rc = CMP_reallocNoCopy(len2 + 3, out);
        if (rc != 0)
            return rc;
    }
    ov = out->value;
    T_memset(ov, 0, (len2 + 3) * 4);
    out->length = 1;

    alen = a->length;
    av = a->value;
    for (i = 0; i < alen; i++) {
        CMP_VectorMultiply(av[i], b, 0, b->length, out, i);
        CMP_VectorMultiply(ov[i] * n0, n, 0, nlen, out, i);
    }
    for (; i < nlen; i++)
        CMP_VectorMultiply(ov[i] * n0, n, 0, nlen, out, i);

    out->length = len2 + 1;
    CMP_ShiftRightByCMPWords(nlen, out);
    CMP_RecomputeLength(nlen, out);
    if (CMP_Compare(out, n) >= 0) {
        rc = CMP_SubtractInPlace(n, out);
        if (rc != 0)
            return rc;
    }
    return 0;
}
