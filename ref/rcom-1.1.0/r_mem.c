/*
 * The memory functions of RCOM: R_malloc, R_realloc, R_free and their locked forms, and the
 * functions an application installs behind them (R_mem_ctrl, R_set/get[_locked]_mem_functions).
 */


#include "r_com.h"


/* The mode and the five hooks, which start as the functions of the C library. */
#include <stdlib.h>

static int mh_mode = 0;
static R_MallocFunc malloc_locked_func = (R_MallocFunc)malloc;
static R_FreeFunc free_locked_func = (R_FreeFunc)free;
static R_MallocFunc malloc_func = (R_MallocFunc)malloc;
static R_ReallocFunc realloc_func = (R_ReallocFunc)realloc;
static R_FreeFunc free_func = (R_FreeFunc)free;

/* Case 0 clears bit 0, 1 sets it, 2 sets bit 1 when bit 0 is set, 3 clears bit 1, 8 rereads the
 * mode; the others change nothing. Returns the mode read at the start. */
int R_mem_ctrl(int mode)
{
    int ret;

    ret = mh_mode;

    switch (mode) {
    case 1:
        mh_mode = mh_mode | 1;
        break;
    case 0:
        mh_mode = mh_mode & ~1;
        break;
    case 3:
        mh_mode = mh_mode & ~2;
        break;
    case 2:
        if (mh_mode & 1) {
            mh_mode = mh_mode | 2;
        }
        break;
    case 8:
        ret = mh_mode;
        break;
    default:
        break;
    }
    return ret;
}

void R_set_mem_functions(R_MallocFunc m, R_ReallocFunc r, R_FreeFunc f)
{
    if (m == 0 || r == 0 || f == 0) {
        return;
    }
    malloc_func = m;
    realloc_func = r;
    free_func = f;
    malloc_locked_func = m;
    free_locked_func = f;
}

void R_set_locked_mem_functions(R_MallocFunc m, R_FreeFunc f)
{
    if (m == 0 || f == 0) {
        return;
    }
    malloc_locked_func = m;
    free_locked_func = f;
}

void R_get_mem_functions(R_MallocFunc *m, R_ReallocFunc *r, R_FreeFunc *f)
{
    if (m != 0) {
        *m = malloc_func;
    }
    if (r != 0) {
        *r = realloc_func;
    }
    if (f != 0) {
        *f = free_func;
    }
}

void R_get_locked_mem_functions(R_MallocFunc *m, R_FreeFunc *f)
{
    if (m != 0) {
        *m = malloc_locked_func;
    }
    if (f != 0) {
        *f = free_locked_func;
    }
}

char *R_malloc_locked(unsigned int num)
{
    return (*malloc_locked_func)(num);
}

void R_free_locked(void *ptr)
{
    (*free_locked_func)(ptr);
}

char *R_malloc(unsigned int num)
{
    return (*malloc_func)(num);
}

/* (ptr, new size, old size) */
char *R_realloc(char *ptr, unsigned int newsize, unsigned int oldsize)
{
    return (*realloc_func)(ptr, newsize, oldsize);
}

void R_free(void *ptr)
{
    (*free_func)(ptr);
}

char *R_remalloc(char *a, unsigned int n)
{
    if (a != 0) {
        R_free(a);
    }
    a = R_malloc(n);
    return a;
}
