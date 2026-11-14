/*
 * R_time_size, R_time, R_time_get_func and R_time_set_func.
 */


#include "r_com.h"


static R_TimeFunc       g_timeFunc = 0;

int R_time_size(void)
{
    return 12;
}

void *R_time(void *timeObj)
{
    if (g_timeFunc != 0)
        return (*g_timeFunc)(timeObj);
    else
        return 0;
}

R_TimeFunc R_time_get_func(void)
{
    return g_timeFunc;
}

void R_time_set_func(R_TimeFunc f)
{
    g_timeFunc = f;
}
