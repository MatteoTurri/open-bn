/*
 * time_mi_offset.
 */

#include "time_mi.h"

/* As time_mi_loffset, with the offset in milliseconds. */
int time_mi_offset(void *dstScratch, void *srcScratch, long ms)
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

        nsDelta = (ms % 1000) * 1000000;
        secDelta = ms / 1000;

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
