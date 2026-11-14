/*
 * The machine-independent time of RCOM: time_mi_loffset and time_mi_cmp. The functions work on a
 * 20-byte scratch: an 8-byte header and the 12-byte time (R_TimeMiPayload).
 */


#include "time_mi.h"

/* dstScratch = src + (seconds, nsec), src being srcScratch + 8 or crypto_epoch when srcScratch is 0;
 * the nanoseconds carry into the seconds as in a timespec normalized to [0, 1e9). The sign of nsec
 * is inverted when src->secHi is negative. */
int time_mi_loffset(void *dstScratch, void *srcScratch, long nsec, long seconds)
{
    int ret;
    int carry;
    int srcSec;
    int nsDelta;
    int secDelta;
    R_TimeMiPayload *dst;
    R_TimeMiPayload *src;

    ret = 0;
    carry = 0;

    if (dstScratch == 0) {
        ret = R_ERROR_NULL_ARG;
    } else {
        dst = &((R_TimeMiScratch *)dstScratch)->time;

        if (srcScratch == 0) {
            src = &crypto_epoch;
        } else {
            src = &((R_TimeMiScratch *)srcScratch)->time;
        }

        nsDelta = nsec;
        secDelta = seconds;

        if (src->secHi < 0) {
            nsDelta = -nsDelta;
        }

        if (nsDelta < 0 && (unsigned int)(-nsDelta) > (unsigned int)src->nsec) {
            secDelta = secDelta - 1;
            dst->nsec = (src->nsec + 1000000000) + nsDelta;
        } else {
            dst->nsec = src->nsec + nsDelta;
            if ((unsigned int)999999999 < (unsigned int)dst->nsec) {
                secDelta = secDelta + 1;
                dst->nsec = dst->nsec + (-1000000000);
            }
        }

        srcSec = src->secLo;
        dst->secLo = src->secLo + secDelta;

        if (secDelta > 0 && (unsigned int)dst->secLo < (unsigned int)srcSec) {
            carry = 1;
        } else if (secDelta < 0 && (unsigned int)dst->secLo > (unsigned int)srcSec) {
            carry = -1;
        }

        dst->secHi = src->secHi + carry;
    }
    return ret;
}

/* Compares two times field by field, the first signed, the other two unsigned; a 0 scratch stands
 * for a zero time. *result gets 1, -1 or 0; returns 0. */
int time_mi_cmp(void *scratch1, void *scratch2, int *result)
{
    int ret = 0;
    R_TimeMiPayload zero = {0};
    R_TimeMiPayload *p1, *p2;
    int cmpResult;

    p1 = (scratch1 == 0) ? &zero : &((R_TimeMiScratch *)scratch1)->time;
    p2 = (scratch2 == 0) ? &zero : &((R_TimeMiScratch *)scratch2)->time;

    if (p1->secHi > p2->secHi) {
        cmpResult = 1;
    } else if (p1->secHi < p2->secHi) {
        cmpResult = -1;
    } else if ((unsigned int)p1->secLo > (unsigned int)p2->secLo) {
        cmpResult = 1;
    } else if ((unsigned int)p1->secLo < (unsigned int)p2->secLo) {
        cmpResult = -1;
    } else if ((unsigned int)p1->nsec > (unsigned int)p2->nsec) {
        cmpResult = 1;
    } else if ((unsigned int)p1->nsec < (unsigned int)p2->nsec) {
        cmpResult = -1;
    } else {
        cmpResult = 0;
    }

    *result = cmpResult;
    return ret;
}
