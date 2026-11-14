/*
 * The time object of RCOM (R_time_*): 12 opaque bytes whose functions an application installs
 * (R_time_set_*_func). Each entry point calls the installed function, or returns 0 when there is
 * none. R_time_from_int and R_time_to_int convert an int through a 4-byte big-endian buffer and
 * format 3.
 */


#include "r_com.h"

static R_TimeOffsetFunc g_offsetFunc = 0;
static R_TimeCmpFunc    g_cmpFunc = 0;
static R_TimeImportFunc g_importFunc = 0;
static R_TimeExportFunc g_exportFunc = 0;


void *R_time_new(void)
{
    void *obj = 0;
    void *status = 0;

    if ((obj = R_malloc(12))) {
        status = R_time(obj);
        if (status == 0) {
            R_free(obj);
        }
    }
    return status;
}

void R_time_free(void *obj)
{
    R_free(obj);
}

void *R_time_offset(void *timeObj, void *base, long seconds, long nsec)
{
    if (g_offsetFunc != 0)
        return (*g_offsetFunc)(timeObj, base, seconds, nsec);
    else
        return 0;
}

R_TimeOffsetFunc R_time_get_offset_func(void)
{
    return g_offsetFunc;
}

void R_time_set_offset_func(R_TimeOffsetFunc f)
{
    g_offsetFunc = f;
}

int R_time_from_int(void *timeObj, int value)
{
    unsigned long buf;
    unsigned char *p;

    p = (unsigned char *)&value;
    buf  = ((unsigned long)(*(p++))) << 24;
    buf |= ((unsigned long)(*(p++))) << 16;
    buf |= ((unsigned long)(*(p++))) << 8;
    buf |= ((unsigned long)(*(p++)));
    return R_time_import(&buf, 4, R_TIME_FMT_SECONDS, timeObj);
}

int R_time_to_int(void *timeObj)
{
    unsigned long buf;
    int result;
    unsigned char *p;

    p = (unsigned char *)&buf;
    if (R_time_export(timeObj, R_TIME_FMT_SECONDS, &buf, 4) == 0) {
        result  = ((int)(*(p++))) << 24;
        result |= ((int)(*(p++))) << 16;
        result |= ((int)(*(p++))) << 8;
        result |= ((int)(*(p++)));
    } else {
        result = -1;
    }
    return result;
}

int R_time_cmp(void *timeObj1, void *timeObj2)
{
    if (g_cmpFunc != 0)
        return (*g_cmpFunc)(timeObj1, timeObj2);
    else
        return 0;
}

R_TimeCmpFunc R_time_get_cmp_func(void)
{
    return g_cmpFunc;
}

void R_time_set_cmp_func(R_TimeCmpFunc f)
{
    g_cmpFunc = f;
}

int R_time_import(void *data, unsigned int len, int format, void *timeObj)
{
    if (g_importFunc != 0)
        return (*g_importFunc)(data, len, format, timeObj);
    else
        return 0;
}

R_TimeImportFunc R_time_get_import_func(void)
{
    return g_importFunc;
}

void R_time_set_import_func(R_TimeImportFunc f)
{
    g_importFunc = f;
}

int R_time_export(void *timeObj, int format, void *data, unsigned int len)
{
    if (g_exportFunc != 0)
        return (*g_exportFunc)(timeObj, format, data, len);
    else
        return 0;
}

R_TimeExportFunc R_time_get_export_func(void)
{
    return g_exportFunc;
}

void R_time_set_export_func(R_TimeExportFunc f)
{
    g_exportFunc = f;
}

