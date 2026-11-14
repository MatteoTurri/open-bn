/*
 * The common services of RCOM: memory allocation (R_malloc and the hooks
 * behind it), locks and thread ids, the stack depth diagnostics, the time
 * object (R_time_*), and the error string tables of the RCOM libraries.
 */

#ifndef RCOM_R_COM_H
#define RCOM_R_COM_H

/* The error codes of the common services. */
#define R_ERROR_ALLOC_FAILURE 10005    /* an allocation failed */
#define R_ERROR_BUFFER_TOO_SMALL 10016 /* the output buffer is too small */
#define R_ERROR_NULL_ARG 10017         /* a required pointer argument is 0 */
#define R_ERROR_BAD_VALUE 10019        /* a value cannot be obtained or represented */
#define R_ERROR_BAD_FORMAT 10020       /* an unknown format, or input not in the format */
#define R_ERROR_BAD_DATA 10022         /* invalid input data */
#define R_ERROR_NOT_INITIALIZED 10077  /* the stack datum was never set */

typedef char *(*R_MallocFunc)(unsigned int);
typedef char *(*R_ReallocFunc)(char *, unsigned int, unsigned int);
typedef void (*R_FreeFunc)(void *);

typedef int (*R_LockCb)(int mode, int type, char *file, int line);
typedef int (*R_LockedAddCb)(int *pointer, int amount, int type, char *file, int line);
typedef unsigned long (*R_ThreadIdCb)(void);

/* The time functions an application installs (R_mtime_set_all installs the R_mtime_* ones). */
typedef void *(*R_TimeFunc)(void *timeObj);
typedef int (*R_TimeCmpFunc)(void *timeObj1, void *timeObj2);
typedef void *(*R_TimeOffsetFunc)(void *timeObj, void *base, long seconds, long nsec);
typedef int (*R_TimeImportFunc)(void *data, unsigned int len, int format, void *timeObj);
typedef int (*R_TimeExportFunc)(void *timeObj, int format, void *data, unsigned int len);

/* r_mem.c */
int R_mem_ctrl(int mode);
void R_set_mem_functions(R_MallocFunc m, R_ReallocFunc r, R_FreeFunc f);
void R_set_locked_mem_functions(R_MallocFunc m, R_FreeFunc f);
void R_get_mem_functions(R_MallocFunc *m, R_ReallocFunc *r, R_FreeFunc *f);
void R_get_locked_mem_functions(R_MallocFunc *m, R_FreeFunc *f);
char *R_malloc_locked(unsigned int num);
void R_free_locked(void *ptr);
char *R_malloc(unsigned int num);
char *R_realloc(char *ptr, unsigned int newsize, unsigned int oldsize);
void R_free(void *ptr);
char *R_remalloc(char *a, unsigned int n);

/* r_lock.c */
int R_lockid_new(char *name);
void R_lockids_free(void);
int R_lock_num(void);
char *R_lock_get_name(int type);
R_LockCb R_lock_get_cb(void);
R_LockedAddCb R_locked_add_get_cb(void);
int R_lock_set_cb(R_LockCb cb);
int R_locked_add_set_cb(R_LockedAddCb cb);
int R_lock_ctrl(int mode, int type, char *file, int line);
int R_locked_add(int *pointer, int amount, int type, char *file, int line);
R_ThreadIdCb R_thread_get_id_cb(void);
int R_thread_id_set_cb(R_ThreadIdCb cb);
unsigned long R_thread_id(void);

/* r_diag.c */
int R_DIAG_set_stack_datum(unsigned int datum);
int R_DIAG_get_stack_datum(void);
int R_DIAG_check_stack(unsigned int value, char *file, int line, int hit);
int R_DIAG_get_stack_low_water_mark(int *out_depth, char **out_file, int *out_line, int *out_hit);
int R_DIAG_print_stack_depth(void *bio);

/* The formats of R_time_import and R_time_export. */
#define R_TIME_FMT_UTCTIME 1  /* "YYMMDDhhmmssZ" */
#define R_TIME_FMT_GENTIME 2  /* "YYYYMMDDhhmmss.dZ" */
#define R_TIME_FMT_SECONDS 3  /* the raw big-endian seconds */
#define R_TIME_FMT_COLONS 6   /* "YYYY:MM:DD:hh:mm:ss" */

/* r_time.c */
void *R_time_new(void);
void R_time_free(void *obj);
void *R_time_offset(void *timeObj, void *base, long seconds, long nsec);
R_TimeOffsetFunc R_time_get_offset_func(void);
void R_time_set_offset_func(R_TimeOffsetFunc f);
int R_time_from_int(void *timeObj, int value);
int R_time_to_int(void *timeObj);
int R_time_cmp(void *timeObj1, void *timeObj2);
R_TimeCmpFunc R_time_get_cmp_func(void);
void R_time_set_cmp_func(R_TimeCmpFunc f);
int R_time_import(void *data, unsigned int len, int format, void *timeObj);
R_TimeImportFunc R_time_get_import_func(void);
void R_time_set_import_func(R_TimeImportFunc f);
int R_time_export(void *timeObj, int format, void *data, unsigned int len);
R_TimeExportFunc R_time_get_export_func(void);
void R_time_set_export_func(R_TimeExportFunc f);

/* r_time_func.c */
int R_time_size(void);
void *R_time(void *timeObj);
R_TimeFunc R_time_get_func(void);
void R_time_set_func(R_TimeFunc f);

/* The error string tables (hmac_err.c, proxy_err.c, sio_err.c, x509tc_err.c) */
void ERR_load_HMAC_strings(void);
void ERR_load_PROXY_strings(void);
void ERR_load_SIO_strings(void);
void ERR_load_X509TC_strings(void);

#endif
