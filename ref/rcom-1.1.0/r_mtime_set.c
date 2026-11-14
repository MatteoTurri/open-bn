/*
 * R_mtime and R_mtime_set.
 */

#include "r_com.h"


#include <string.h>

/* The scratch of the time_mi_* functions: an 8-byte header and the time. */
#include "time_mi.h"


void *R_mtime(void *timeObj)
{
    R_TimeMiScratch buf;
    int ret = 0;

    if (timeObj == 0) {
        return 0;
    }

    ret = time_mi_time(&buf);
    if (ret == 0) {
        memcpy(timeObj, &buf.time, 12);
        return timeObj;
    } else {
        return 0;
    }
}

int R_mtime_set(void)
{
    R_time_set_func(R_mtime);
    return 0;
}
