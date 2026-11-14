/*
 * The stack diagnostics of RCOM (R_DIAG_*): the lowest stack address seen, and where it was seen.
 */

#include <stdio.h>
#include "bio.h"

#include "r_com.h"


static unsigned int g_low_water_mark = 0;
static unsigned int g_stack_datum = 0;
static int  g_lwm_line;
static int  g_lwm_hit;
static char *g_lwm_file;


int R_DIAG_set_stack_datum(unsigned int datum)
{
    g_stack_datum = datum;
    g_low_water_mark = datum;
    return 0;
}

int R_DIAG_get_stack_datum(void)
{
    return g_stack_datum;
}

int R_DIAG_check_stack(unsigned int value, char *file, int line, int hit)
{
    int ret = 0;

    if (value < g_low_water_mark) {
        g_low_water_mark = value;
        g_lwm_file = file;
        g_lwm_line = line;
        g_lwm_hit = hit;
    }
    return ret;
}

int R_DIAG_get_stack_low_water_mark(int *out_depth, char **out_file, int *out_line, int *out_hit)
{
    if (g_stack_datum == 0) {
        return R_ERROR_NOT_INITIALIZED;
    }

    if (out_depth != 0) {
        *out_depth = g_stack_datum - g_low_water_mark;
    }
    if (out_file != 0) {
        *out_file = g_lwm_file;
    }
    if (out_line != 0) {
        *out_line = g_lwm_line;
    }
    if (out_hit != 0) {
        *out_hit = g_lwm_hit;
    }
    return 0;
}

int R_DIAG_print_stack_depth(void *bio)
{
    int ret = 0;
    char *file;
    int line;
    int hit;
    int depth;

    ret = R_DIAG_get_stack_low_water_mark(&depth, &file, &line, &hit);
    if (ret != 0) {
        return ret;
    }

    BIO_printf(bio, "Peek stack usage = %d bytes [%s:%d, Hit#%d]\n", depth, file, line, hit);
    return ret;
}
