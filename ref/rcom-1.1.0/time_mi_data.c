/*
 * CRYPTO_day_to_month, the cumulated days per month (row 0 normal year, row 1 leap year), and
 * crypto_epoch.
 */

const long CRYPTO_day_to_month[2][13] = {
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 },
};

/* The zero time, the default source of time_mi_loffset and time_mi_offset when the scratch is 0. */
#include "time_mi.h"

const R_TimeMiPayload crypto_epoch = { 0, 0, 0 };
