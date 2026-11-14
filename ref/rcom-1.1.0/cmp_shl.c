/*
 * CMP_* subtract in place, signed subtract, shifts by words and by bits, length and bit tests.
 */

#include "cmp.h"
#include "bsafe.h"


/* b -= a in place; b must be at least as long as a (0x109 otherwise). The borrow ripples word by
 * word, then the zero top words are trimmed. */
int CMP_SubtractInPlace(CMPInt *a, CMPInt *b)
{
    int blen = b->length;
    int alen = a->length;
    unsigned long *bv = b->value;
    unsigned long *av = a->value;
    int borrow = 0;
    int i, j;

    if (blen < alen)
        return 0x109;

    for (i = 0; i < alen; i++) {
        borrow = bv[i];
        bv[i] = borrow - av[i];
        borrow = (borrow < bv[i]);
        if (borrow) {
            for (j = i + 1; j < blen; j++) {
                borrow = bv[j];
                bv[j]--;
                borrow = (borrow < bv[j]);
                if (!borrow)
                    break;
            }
        }
    }

    if (borrow)
        return 0x109;

    while (b->value[b->length - 1] == 0 && b->length >= 2)
        b->length--;
    return 0;
}

/* Signed subtract: dst = |a| - |b| or |a| + |b| according to signA and signB; *sign_out gets the sign
 * of the larger operand. */
int CMP_SubtractSigned(CMPInt *a, int signA, CMPInt *b, int signB, CMPInt *dst, int *sign_out)
{
    int cmp = CMP_Compare(a, b);

    if (cmp >= 0) {
        *sign_out = signA;
        if (signA == signB)
            return CMP_Subtract(a, b, dst);
        return CMP_Add(a, b, dst);
    }

    *sign_out = -signB;
    if (signA == signB)
        return CMP_Subtract(b, a, dst);
    return CMP_Add(a, b, dst);
}

/* Shifts left by n whole words in place: with room, the words move up and the n low words are
 * cleared; otherwise the value is saved, the buffer reallocated without copy, and the saved value
 * copied back above the cleared words. Then the zero top words are trimmed. */
int CMP_ShiftLeftByCMPWords(int n, CMPInt *x)
{
    int oldlen;
    int newlen;

    if (n <= 0)
        return 0;

    oldlen = x->length;
    newlen = oldlen + n;
    if (x->allocated < newlen) {
        CMPInt tmp;
        int rc;

        do {
            CMP_Constructor(&tmp);
            if ((rc = CMP_Move(x, &tmp)) != 0)
                break;
            if ((rc = CMP_reallocNoCopy(newlen + 1, x)) != 0)
                break;
            T_memset(x->value, 0, n * 4);
            T_memcpy((unsigned char *)x->value + n * 4, tmp.value, oldlen * 4);
            while (x->value[newlen - 1] == 0 && newlen >= 2)
                newlen--;
            x->length = newlen;
        } while (0);
        CMP_Destructor(&tmp);
        return rc;
    }
    T_memmove((unsigned char *)x->value + n * 4, x->value, oldlen * 4);
    T_memset(x->value, 0, n * 4);
    while (x->value[newlen - 1] == 0 && newlen >= 2)
        newlen--;
    x->length = newlen;
    return 0;
}

/* Shifts left by n bits in place: whole words through CMP_ShiftLeftByCMPWords, then the remaining
 * 0-31 bits, growing by one word when the top word carries out. */
int CMP_ShiftLeftByBits(int bits, CMPInt *x)
{
    int invShift;
    int i;
    unsigned long carry;
    unsigned long *v;

    if (bits <= 0)
        return 0;

    if (bits >= 32) {
        int wordShift = bits / 32;
        int rc = CMP_ShiftLeftByCMPWords(wordShift, x);
        if (rc != 0)
            return rc;
        bits -= wordShift * 32;
        if (bits == 0)
            return 0;
    }

    i = x->length;
    invShift = 32 - bits;
    carry = x->value[i - 1] >> invShift;
    if (carry != 0) {
        if (x->allocated < i + 1) {
            int rc = CMP_realloc(i + 2, x);
            if (rc != 0)
                return rc;
        }
        x->value[i] = carry;
        x->length = x->length + 1;
    }

    i--;
    v = x->value;
    if (i == 0) {
        v[0] = v[0] << bits;
        return 0;
    }
    v[i] = v[i] << bits;
    while (i-- > 0) {
        v[i + 1] |= v[i] >> invShift;
        v[i] = v[i] << bits;
    }
    return 0;
}

/* Shifts right by n whole words in place; an emptied value becomes one zero word. */
int CMP_ShiftRightByCMPWords(int n, CMPInt *x)
{
    int newlen = x->length - n;
    x->length = newlen;
    if (newlen > 0) {
        unsigned long *v = x->value;
        T_memmove(v, v + n, newlen * 4);
        return 0;
    }
    x->length = 1;
    x->value[0] = 0;
    return 0;
}

/* Shifts right by n bits in place: whole words through CMP_ShiftRightByCMPWords, then the remaining
 * 0-31 bits. */
int CMP_ShiftRightByBits(int bits, CMPInt *x)
{
    int remBits;
    int invShift;
    int top;
    unsigned long *p;

    if (bits <= 0)
        return 0;

    if (bits >= 32) {
        int wordShift = bits / 32;
        int rc = CMP_ShiftRightByCMPWords(wordShift, x);
        if (rc != 0)
            return rc;
        bits -= wordShift * 32;
        if (bits == 0)
            return 0;
    }

    remBits = bits;
    invShift = 32 - remBits;
    top = x->length - 1;
    p = x->value;

    if (top > 0) {
        unsigned long *q = p;
        int n = top;
        do {
            n--;
            q[0] = (q[0] >> remBits) | (q[1] << invShift);
            q++;
        } while (n != 0);
    }

    p[top] >>= remBits;
    if (p[top] == 0) {
        if (top != 0)
            x->length--;
    }
    return 0;
}

/* Trims x->value from word hi down to the first nonzero word and sets the length. */
int CMP_RecomputeLength(int hi, CMPInt *x)
{
    unsigned long *value = x->value;

    while (value[hi] == 0 && hi > 0)
        hi--;
    x->length = hi + 1;
    return 0;
}

/* The bit position (1..32) of the top bit of the top word, 0x20 when the top word is zero. */
int CMP_GetOffsetOfMSB(CMPInt *x, int *out)
{
    unsigned long msw = x->value[x->length - 1];
    int n = 0;
    if (msw != 0) {
        while ((msw & 0x80000000UL) == 0) {
            msw <<= 1;
            n++;
        }
        *out = n;
        return 0;
    }
    *out = 0x20;
    return 0;
}

/* Shifts right by one bit in place. */
int CMP_ShiftRight1Bit(CMPInt *x)
{
    unsigned long *v = x->value;
    int top = x->length - 1;

    if (top > 0) {
        unsigned long *p = v;
        int n = top;
        do {
            p[0] = (p[0] >> 1) | (p[1] << 31);
            p++;
        } while (--n != 0);
    }

    v[top] >>= 1;
    if (v[top] == 0) {
        if (top != 0)
            x->length--;
    }
    return 0;
}

/* As CMP_ShiftRight1Bit, into dst instead of in place. */
int CMP_ShiftRight1BitNotInPlace(CMPInt *src, CMPInt *dst)
{
    int len = src->length;
    unsigned long *sv;
    unsigned long *dv;
    int n;
    int i;

    if (dst->allocated < len) {
        int rc = CMP_reallocNoCopy(len, dst);
        if (rc != 0)
            return rc;
    }

    sv = src->value;
    dv = dst->value;
    n = src->length - 1;
    dst->length = len;

    for (i = 0; i < n; i++) {
        dv[i] = (sv[i] >> 1) | (sv[i + 1] << 31);
    }

    dv[n] = sv[n] >> 1;
    if (dv[n] == 0 && n != 0)
        dst->length--;
    return 0;
}

int CMP_IsCMPEven(CMPInt *x)
{
    return (x->value[0] ^ 1) & 1;
}

int CMP_IsCMPOdd(CMPInt *x)
{
    return x->value[0] & 1;
}
