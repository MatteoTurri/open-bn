/*
 * The basic CMP_* operations: swap, move, compare, add and subtract (also of one word), bit length
 * and bit setters.
 */
#include "cmp.h"
#include "bsafe.h"


/* Swaps the headers of two CMPInts (the buffers change owner) through a CMPInt temporary. */
int CMP_SwapContents(CMPInt *a, CMPInt *b)
{
    CMPInt t;

    t.value = a->value;
    a->value = b->value;
    t.allocated = a->allocated;
    a->allocated = b->allocated;
    t.length = a->length;
    a->length = b->length;
    b->value = t.value;
    b->allocated = t.allocated;
    b->length = t.length;
    return 0;
}

/* Copies src into dst (source first); dst grows as needed and does not shrink. */
int CMP_Move(CMPInt *src, CMPInt *dst)
{
    int len = src->length;
    if (dst->allocated < len) {
        int rc = CMP_reallocNoCopy(len, dst);
        if (rc != 0)
            return rc;
    }
    T_memcpy(dst->value, src->value, len * 4);
    dst->length = len;
    return 0;
}

/* Compares two magnitudes: 1 if a > b, -1 if a < b, 0 if equal; the length first, then the words
 * from the most significant. */
int CMP_Compare(CMPInt *a, CMPInt *b)
{
    int len = a->length;
    int blen = b->length;
    unsigned long *av = a->value;
    unsigned long *bv = b->value;
    int i;

    if (len == blen) {
        i = len - 1;
        while (i >= 0 && av[i] == bv[i])
            i--;
        if (i < 0)
            return 0;
        if (av[i] > bv[i])
            return 1;
        return -1;
    }
    if (len > blen)
        return 1;
    return -1;
}

/* dst = a + b: adds the longer operand and the shorter one word by word with the carry; a final
 * carry becomes a new word 1. The first reallocation does not keep the contents, the second one
 * (for the carry word) does. */
int CMP_Add(CMPInt *a, CMPInt *b, CMPInt *dst)
{
    int bigLen;
    unsigned long *bigV;
    int smallLen;
    unsigned long *smallV;
    unsigned long *out;
    unsigned long carry;
    int i;
    int rc;
    if (a->length >= b->length) {
        bigLen = a->length;
        smallLen = b->length;
        bigV = a->value;
        smallV = b->value;
    } else {
        bigLen = b->length;
        smallLen = a->length;
        bigV = b->value;
        smallV = a->value;
    }

    if (dst->allocated < bigLen) {
        rc = CMP_reallocNoCopy(bigLen + 1, dst);
        if (rc != 0)
            return rc;
    }

    out = dst->value;
    carry = 0;
    i = 0;
    while (i < smallLen) {
        unsigned long t = *smallV + *bigV;
        unsigned long s = t + carry;
        *out = s;
        if (t < *smallV || s < t)
            carry = 1;
        else
            carry = 0;
        i++;
        bigV++;
        smallV++;
        out++;
    }

    for (; i < bigLen; i++) {
        unsigned long v = *bigV + carry;
        *out = v;
        out++;
        carry = (v < *bigV);
        bigV++;
    }
    dst->length = i;

    if (carry == 0)
        return 0;

    i = i + 1;
    if (dst->allocated < i) {
        rc = CMP_realloc(i, dst);
        if (rc != 0)
            return rc;
    }
    dst->length = i;
    dst->value[bigLen] = 1;
    return 0;
}

/* x += word, rippling the carry through the higher words and growing by one word past the top. */
int CMP_AddCMPWord(unsigned long word, CMPInt *x)
{
    unsigned long *v = x->value;
    int i, len;

    v[0] += word;
    if (v[0] >= word)
        return 0;

    len = x->length;
    for (i = 1; i < len; i++) {
        if (++v[i] != 0)
            return 0;
    }

    len = len + 1;
    if (x->allocated < len) {
        int rc = CMP_realloc(len + 1, x);
        if (rc != 0)
            return rc;
    }
    x->value[i] = 1;
    x->length = len;
    return 0;
}

/* dst = a - b with a >= b (265 when a is shorter than b or a borrow is left). The first word is
 * unrolled, then the words of b; the tail of a is copied when there is no borrow, otherwise the
 * borrow is rippled and the length trimmed. Equal lengths go through CMP_RecomputeLength. */
int CMP_Subtract(CMPInt *a, CMPInt *b, CMPInt *dst)
{
    int alen = a->length;
    int blen = b->length;
    unsigned long *bv = b->value;
    unsigned long *av = a->value;
    unsigned long *out;
    unsigned long borrow;
    int i;
    int rc;

    if (alen < blen)
        return 0x109;

    if (dst->allocated < alen) {
        rc = CMP_reallocNoCopy(alen + 1, dst);
        if (rc != 0)
            return rc;
    }

    out = dst->value;
    out[0] = av[0] - bv[0];
    borrow = (av[0] < out[0]);

    for (i = 1; i < blen; i++) {
        out[i] = av[i] - bv[i];
        if (!borrow)
            borrow = (av[i] < out[i]);
        else {
            out[i]--;
            borrow = (av[i] <= out[i]);
        }
    }

    if (i == alen) {
        if (borrow)
            return 0x109;
        return CMP_RecomputeLength(alen - 1, dst);
    }

    if (!borrow) {
        T_memcpy(&out[i], &av[i], (alen - i) * 4);
        dst->length = alen;
    } else {
        int j;
        for (j = i; j < alen; j++) {
            out[j] = av[j] - borrow;
            borrow = (av[j] < out[j]);
        }
        if (borrow)
            return 0x109;
        for (;;) {
            if (dst->value[alen - 1] != 0)
                break;
            if (alen < 2)
                break;
            alen--;
        }
        dst->length = alen;
    }
    return 0;
}

/* Subtracts one word in place, rippling the borrow; 0x109 when it goes past the top. */
int CMP_SubtractCMPWord(unsigned long word, CMPInt *x)
{
    unsigned long *v = x->value;
    unsigned long w0 = v[0];

    if (w0 >= word) {
        v[0] = w0 - word;
        return 0;
    }

    {
        int top;
        int i;

        v[0] = w0 - word;

        top = x->length - 1;
        i = 1;

        if (top > 0) {
            unsigned long w1 = v[1] - 1;
            v[1] = w1;
            if (w1 == (unsigned long)-1) {
                unsigned long w;
                do {
                    i++;
                    if (top < i)
                        break;
                    w = v[i] - 1;
                    v[i] = w;
                } while (w == (unsigned long)-1);
            }
        }

        if (i < top)
            return 0;
        if (top < i)
            return 0x109;
        if (v[i] == 0)
            x->length--;
        return 0;
    }
}

int CMP_BitLengthOfCMPInt(CMPInt *x)
{
    unsigned long *w = x->value;
    int len;
    int idx;
    unsigned long msw;
    int bits;

    if (w != 0) {
        len = x->length;
        idx = len - 1;
        msw = w[idx] >> 1;
        bits = 1;
        while (msw != 0) {
            bits++;
            msw >>= 1;
        }
        return (idx << 5) + bits;
    }
    return 0;
}

int CMP_GetBit(int bitIndex, CMPInt *x, int *out)
{
    int wordIndex = bitIndex / 32;
    int bitOffset = bitIndex % 32;
    if (wordIndex < x->length) {
        *out = (x->value[wordIndex] >> bitOffset) & 1;
        return 0;
    }
    *out = 0;
    return 0;
}

int CMP_ClearBit(int bitIndex, CMPInt *x)
{
    int bitOffset = bitIndex % 32;
    int wordIndex = bitIndex / 32;

    if (wordIndex >= x->length)
        return 0;
    x->value[wordIndex] &= ~(1UL << bitOffset);
    CMP_RecomputeLength(x->length - 1, x);
    return 0;
}

/* Sets bit bitIndex, growing the buffer when the word is beyond the length (the counterpart of
 * CMP_ClearBit). */
int CMP_SetBit(int bitIndex, CMPInt *x)
{
    int wordIndex = bitIndex / 32;
    int bitOffset = bitIndex % 32;

    if (wordIndex < x->length) {
        x->value[wordIndex] |= (1UL << bitOffset);
        return 0;
    }

    if (x->allocated < wordIndex + 1) {
        int rc = CMP_realloc(wordIndex + 2, x);
        if (rc != 0)
            return rc;
    }

    T_memset(x->value + x->length, 0, (x->allocated - x->length) * 4);
    x->value[wordIndex] = 1UL << bitOffset;
    CMP_RecomputeLength(wordIndex, x);
    return 0;
}
