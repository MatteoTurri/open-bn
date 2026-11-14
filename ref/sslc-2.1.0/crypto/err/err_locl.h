/* crypto/err/err_locl.h */
/* SSL-C: helpers of the error queue shared by the err units (static in
 * SSLeay's err.c) */

#ifndef HEADER_ERR_LOCL_H
#define HEADER_ERR_LOCL_H

#include "err.h"
#include "lhash.h"

/* The two hash tables of err_tbl.c: the error strings and the per-thread states. */
extern LHASH *err_error_hash;
extern LHASH *err_thread_hash;

#define err_clear_data(p,i) \
	if (((p)->err_data[i] != NULL) && \
		(p)->err_data_flags[i] & ERR_TXT_MALLOCED) \
		{  \
		Free((p)->err_data[i]); \
		(p)->err_data[i]=NULL; \
		} \
	(p)->err_data_flags[i]=0;

#ifndef NOPROTO
unsigned long err_get_error_values(int inc,char **file,int *line,
	char **data,int *flags);
void ERR_STATE_free(ERR_STATE *s);
#else
unsigned long err_get_error_values();
void ERR_STATE_free();
#endif

#endif
