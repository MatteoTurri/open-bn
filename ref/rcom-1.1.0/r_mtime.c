/*
 * R_mtime_*: the time object of R_time on top of the time_mi_* functions; R_mtime_set_all installs
 * them in R_time. Each one copies the 12-byte time to or from a scratch whose time follows an
 * 8-byte header.
 */

#include "r_com.h"


#include <string.h>

/* The scratch of the time_mi_* functions: an 8-byte header and the time. */
#include "time_mi.h"


void *R_mtime_offset(void *timeObj, void *base, long seconds, long nsec)
{
    R_TimeMiScratch buf = {0};
    int ret;

    if (timeObj == 0) {
        return 0;
    }

    if (base != 0) {
        memcpy(&buf.time, base, 12);
    }

    ret = time_mi_loffset(&buf, &buf, nsec, seconds);
    if (ret == 0) {
        memcpy(timeObj, &buf.time, 12);
        return timeObj;
    } else {
        return 0;
    }
}

int R_mtime_cmp(void *timeObj1, void *timeObj2)
{
    R_TimeMiScratch scratch1 = {0};
    R_TimeMiScratch scratch2 = {0};
    int result;

    if (timeObj1 != 0) {
        memcpy(&scratch1.time, timeObj1, 12);
    }
    if (timeObj2 != 0) {
        memcpy(&scratch2.time, timeObj2, 12);
    }

    time_mi_cmp(&scratch1, &scratch2, &result);
    return result;
}

int R_mtime_import(void *data, unsigned int len, int format, void *timeObj)
{
    R_TimeMiScratch buf;
    int ret;

    if (timeObj == 0) {
        return R_ERROR_NULL_ARG;
    }

    ret = time_mi_import(&buf, format, data, len);
    memcpy(timeObj, &buf.time, 12);
    return ret;
}

int R_mtime_export(void *timeObj, int format, void *data, unsigned int len)
{
    R_TimeMiScratch buf;
    int ret;
    long outLen;

    if (timeObj == 0) {
        return R_ERROR_NULL_ARG;
    }

    memcpy(&buf.time, timeObj, 12);
    ret = time_mi_export(&buf, format, data, &outLen, len);
    return ret;
}

int R_mtime_set_all(void)
{
    R_mtime_set();
    R_time_set_offset_func(R_mtime_offset);
    R_time_set_cmp_func(R_mtime_cmp);
    R_time_set_import_func(R_mtime_import);
    R_time_set_export_func(R_mtime_export);
    return 0;
}
