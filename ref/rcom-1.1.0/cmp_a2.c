/*
 * The CMP_* big numbers of BSAFE: conversion from and to octet strings, word and power-of-two
 * setters, and the bit and parity tests. A CMPInt is {allocated, length, value}, the words least
 * significant first; the functions return a status and their result through out parameters.
 */

#include "cmp.h"
#include "bsafe.h"


/* Big-endian octet string to CMPInt: reads the buffer from its last byte, one word per 4 bytes,
 * then the partial top word, and trims the zero top words. 0x102: empty input. */
int CMP_OctetStringToCMPInt(const unsigned char *buf, unsigned int len, CMPInt *x)
{
    int words;
    int fullWords;
    const unsigned char *p;
    unsigned long *v;
    int i, j;
    int rem;

    if (len == 0)
        return 0x102;

    i = (len + 3) / 4;
    words = i;
    if (words == 0)
        words = 1;
    fullWords = len / 4;
    if (x->allocated < words) {
        int rc2 = CMP_reallocNoCopy(words + 1, x);
        if (rc2 != 0)
            return rc2;
    }
    p = buf + len - 1;
    x->length = words;
    v = x->value;
    for (i = 0; i < fullWords; i++) {
        *v = p[0];
        for (j = 1; j < 4; j++)
            *v |= (unsigned long)p[-j] << (j * 8);
        p -= 4;
        v++;
    }
    if (fullWords * 4 < (int)len) {
        rem = (int)len - fullWords * 4;
        *v = p[0];
        for (j = 1; j < rem; j++)
            *v |= (unsigned long)p[-j] << (j * 8);
    }
    while (x->value[x->length - 1] == 0 && x->length >= 2)
        x->length--;
    return 0;
}

/* As CMP_CMPIntToOctetString with a fixed length: a shorter integer is padded with leading zeros.
 * 268 when the buffer or fixedLen is too small. */
int CMP_CMPIntToFixedLenOctetStr(CMPInt *x, unsigned int fixedLen,
                                 unsigned int cap, int *outLen,
                                 unsigned char *buf)
{
    int nbytes;
    int i;

    if (cap < fixedLen)
        return 0x10c;

    nbytes = (CMP_BitLengthOfCMPInt(x) + 7) / 8;
    if (fixedLen < nbytes)
        return 0x10c;

    i = 0;
    if (nbytes < fixedLen) {
        for (i = 0; i < (int)(fixedLen - nbytes); i++)
            buf[i] = 0;
        *outLen = fixedLen;
        return CMP_CMPIntToOctetString(x, cap, &nbytes, buf + i);
    }
    return CMP_CMPIntToOctetString(x, cap, outLen, buf);
}

/* Big-endian serialization of the whole integer without the leading zero bytes of the top word.
 * 0x105: buffer too small; 0x104: zero top word with length > 1 (not normalized). */
int CMP_CMPIntToOctetString(CMPInt *x, unsigned int cap, int *outLen,
                            unsigned char *buf)
{
    int len = x->length;
    int nbytes = len * 4;
    unsigned long *p;
    unsigned long w;
    unsigned char byte;
    int i;
    int j;

    *outLen = nbytes;
    i = 3;
    p = (unsigned long *)((unsigned char *)x->value + (nbytes - 4));
    w = *p;
    p--;
    byte = (unsigned char)(w >> 24);
    if (byte == 0) {
        do {
            i--;
            byte = (unsigned char)(w >> (i * 8));
            (*outLen)--;
        } while (i > 0 && byte == 0);
        if (byte == 0) {
            if (*outLen == 1) {
                if (cap == 0)
                    return 0x105;
                *buf = 0;
                return 0;
            }
            return 0x104;
        }
    }
    if (cap < *outLen)
        return 0x105;
    *buf++ = byte;
    for (j = i - 1; j >= 0; j--)
        *buf++ = (unsigned char)(w >> (j * 8));
    for (i = 1; i < len; i++) {
        w = *p--;
        *buf++ = (unsigned char)(w >> 24);
        *buf++ = (unsigned char)(w >> 16);
        *buf++ = (unsigned char)(w >> 8);
        *buf++ = (unsigned char)w;
    }
    return 0;
}

/* Sets x to one word (length 1, growing to at least 2 words when empty). */
int CMP_CMPWordToCMPInt(unsigned long word, CMPInt *x)
{
    if (x->allocated <= 0) {
        int rc = CMP_reallocNoCopy(2, x);
        if (rc != 0)
            return rc;
    }
    x->value[0] = word;
    x->length = 1;
    return 0;
}

/* Sets x = 2^bits, growing and clearing as needed. 0x104 for bits < 0. */
int CMP_PowerOfTwo(int bits, CMPInt *out)
{
    int wordIndex, bitOffset;

    if (bits < 0)
        return 0x104;

    wordIndex = bits / 32;
    bitOffset = bits % 32;

    if (out->allocated < wordIndex + 1) {
        int rc = CMP_reallocNoCopy(wordIndex + 2, out);
        if (rc != 0)
            return rc;
    }

    out->length = wordIndex + 1;
    T_memset(out->value, 0, wordIndex * 4);
    out->value[wordIndex] = 1UL << bitOffset;
    return 0;
}
