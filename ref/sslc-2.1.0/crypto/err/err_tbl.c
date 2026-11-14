/*
 * The two tables of err.c and their getters, ERR_get_string_table and ERR_get_err_state_table.
 */

#include <stdio.h>
#include "lhash.h"
#include "crypto.h"
#include "cryptlib.h"
#include "buffer.h"
#include "err.h"
#include "err_locl.h"

LHASH *err_error_hash=NULL;
LHASH *err_thread_hash=NULL;

LHASH *ERR_get_string_table()
	{
	return(err_error_hash);
	}

LHASH *ERR_get_err_state_table()
	{
	return(err_thread_hash);
	}
