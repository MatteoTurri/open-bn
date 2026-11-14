/*
 * time_mi_time.
 */

#include <sys/time.h>

#include "time_mi.h"

/* Fills the scratch with the current time (gettimeofday): R_ERROR_BAD_VALUE when it fails, R_ERROR_NULL_ARG when the
 * scratch is 0. */
int time_mi_time(void *scratch)
{
    struct timeval tv;
    int ret;
    R_TimeMiScratch *p;

    ret = 0;
    p = scratch;

    if (scratch == 0) {
        ret = R_ERROR_NULL_ARG;
    } else {
        if (gettimeofday(&tv, 0) == -1) {
            ret = R_ERROR_BAD_VALUE;
        } else {
            p->time.secLo = tv.tv_sec;
            p->time.nsec = tv.tv_usec * 1000;
            p->time.secHi = 0;
        }
    }
    return ret;
}
