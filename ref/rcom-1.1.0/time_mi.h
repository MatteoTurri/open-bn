/*
 * The machine-independent time of RCOM (time_mi_*, R_mtime_*): a time value and the scratch
 * object the time_mi_* functions work on.
 */
#ifndef RCOM_TIME_MI_H
#define RCOM_TIME_MI_H

#include "r_com.h"

/* A time (R_time_size() == 12): seconds since the epoch as a signed 64-bit pair, and nanoseconds. */
typedef struct R_TimeMiPayload {
    /* 0x00 */ long secHi;
    /* 0x04 */ long secLo;
    /* 0x08 */ unsigned long nsec;
} R_TimeMiPayload; /* size = 0x0C */

/* The scratch object of the time_mi_* functions: a header R_mtime_* never touches, then the time. */
typedef struct R_TimeMiScratch {
    /* 0x00 */ int pad_00[2];  /* unused, but part of the layout */
    /* 0x08 */ R_TimeMiPayload time;
} R_TimeMiScratch; /* size = 0x14 */

/* r_mtime_set.c */
void *R_mtime(void *timeObj);
int R_mtime_set(void);

/* r_mtime.c */
int R_mtime_cmp(void *timeObj1, void *timeObj2);
int R_mtime_export(void *timeObj, int format, void *data, unsigned int len);
int R_mtime_import(void *data, unsigned int len, int format, void *timeObj);
void *R_mtime_offset(void *timeObj, void *base, long seconds, long nsec);
int R_mtime_set_all(void);

/* Whether the year in the BIGNUM y is a leap year. */
#define IS_LEAP(y) (BN_mod_word(y,400) == 0 \
	|| (BN_mod_word(y,4) == 0 && BN_mod_word(y,100) != 0))

/* time_mi_data.c */
extern const long CRYPTO_day_to_month[2][13];   /* days before each month, [leap year][month] */
extern const R_TimeMiPayload crypto_epoch;   /* the zero time */

/* time_mi.c */
int time_mi_cmp(void *scratch1, void *scratch2, int *result);
int time_mi_loffset(void *dstScratch, void *srcScratch, long nsec, long seconds);

/* time_mi_export.c */
int time_mi_export(void *scratch, long kind, char *out, long *outLen, long bufLen);

/* time_mi_import.c */
int time_mi_import(void *scratch, long kind, char *in, long inLen);

/* time_mi_offset.c */
int time_mi_offset(void *dstScratch, void *srcScratch, long ms);

/* time_mi_time.c */
int time_mi_time(void *scratch);

#endif
