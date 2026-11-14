/*
 * time_mi_export: formats a time as a Gregorian date, with BIGNUM arithmetic for the 64-bit seconds:
 * kind 1 UTCTime "YYMMDDhhmmssZ" (14 bytes with the NUL), 2 GeneralizedTime "YYYYMMDDhhmmss.dZ"
 * (17), 6 "YYYY:MM:DD:hh:mm:ss" (20), 3 the raw big-endian seconds with sign padding; other kinds:
 * R_ERROR_BAD_FORMAT.
 */

#include <stdio.h>
#include "bn.h"

#include "time_mi.h"
#include <string.h>



int time_mi_export(scratch,kind,out,outLen,bufLen)
void *scratch;
long kind;
char *out;
long *outLen;
long bufLen;
	{
	int ret;
	R_TimeMiPayload *t;

	ret = 0;
	t = 0;
	if (out == 0 || scratch == 0)
		{
		ret = R_ERROR_NULL_ARG;
		goto end;
		}
	t = &((R_TimeMiScratch *)scratch)->time;
	if (kind == R_TIME_FMT_UTCTIME || kind == R_TIME_FMT_GENTIME || kind == R_TIME_FMT_COLONS)
		{
		BIGNUM *days,*years,*yearStart,*leapDays;
		long year,month,day,hour,min,sec,dayOfYear;
		long leap,neg;
		R_TimeMiPayload negated;

		day = -1;
		neg = 0;
		days = BN_new();
		if (days == 0) { ret = R_ERROR_ALLOC_FAILURE; goto end; }
		if (t->secHi < 0)
			{
			negated.secHi = ~t->secHi;
			negated.secLo = ~t->secLo;
			negated.nsec = t->nsec;
			t = &negated;
			t->secLo++;
			if (t->secLo == 0) t->secHi++;
			neg = 1;
			}
		BN_set_word(days,t->secHi);
		BN_lshift(days,days,32);
		BN_add_word(days,t->secLo);
		if (neg) days->neg = 1;
		sec = BN_div_word(days,60);
		min = BN_div_word(days,60);
		hour = BN_div_word(days,24);
		years = BN_dup(days);
		if (years == 0) { ret = R_ERROR_ALLOC_FAILURE; goto end; }
		BN_mul_word(years,400);
		BN_div_word(years,146097);
		yearStart = BN_dup(years);
		if (yearStart == 0) { ret = R_ERROR_ALLOC_FAILURE; goto end; }
		BN_add_word(years,1970);
		leapDays = BN_dup(years);
		if (leapDays == 0) { ret = R_ERROR_ALLOC_FAILURE; goto end; }
		if (days->neg == 0) BN_sub_word(leapDays,1);
		BN_mul_word(yearStart,365);
		BN_div_word(leapDays,4);
		BN_add(yearStart,yearStart,leapDays);
		BN_div_word(leapDays,25);
		BN_sub(yearStart,yearStart,leapDays);
		BN_div_word(leapDays,4);
		BN_add(yearStart,yearStart,leapDays);
		BN_sub_word(yearStart,477);
		if (days->neg && years->neg == 0 && !IS_LEAP(years))
			BN_add_word(yearStart,1);
		BN_sub(days,days,yearStart);
		if (days->neg)
			{
			BN_sub_word(years,1);
			BN_add_word(days,IS_LEAP(years) ? 366 : 365);
			}
		dayOfYear = BN_get_word(days);
		leap = 0;
		if (IS_LEAP(years))
			{
			if (dayOfYear >= 366)
				{
				BN_add_word(years,1);
				dayOfYear -= 366;
				}
			else
				leap = 1;
			}
		else
			{
			if (dayOfYear >= 365)
				{
				BN_add_word(years,1);
				dayOfYear -= 365;
				if (IS_LEAP(years)) leap = 1;
				}
			}
		for (month = 1; month < 13; month++)
			{
			if (dayOfYear < CRYPTO_day_to_month[leap][month])
				{
				day = dayOfYear - CRYPTO_day_to_month[leap][month-1];
				break;
				}
			}
		day++;
		if (kind == R_TIME_FMT_UTCTIME)
			{
			year = BN_get_word(years);
			if (years->neg != 0 || year >= 2050 || year < 1950)
				{ ret = R_ERROR_BAD_VALUE; goto end; }
			*outLen = 14;
			if (bufLen < 14) { ret = R_ERROR_BUFFER_TOO_SMALL; goto end; }
			sprintf(out,"%02ld%02ld%02ld%02ld%02ld%02ldZ",
				year % 100,month,day,hour,min,sec);
			}
		else if (kind == R_TIME_FMT_GENTIME)
			{
			year = BN_get_word(years);
			if (years->neg != 0 || year >= 10000)
				{ ret = R_ERROR_BAD_VALUE; goto end; }
			*outLen = 17;
			if (bufLen < 17) { ret = R_ERROR_BUFFER_TOO_SMALL; goto end; }
			sprintf(out,"%04ld%02ld%02ld%02ld%02ld%02ld.%ldZ",
				year,month,day,hour,min,sec,t->nsec / 100000000);
			}
		else if (kind == R_TIME_FMT_COLONS)
			{
			year = BN_get_word(years);
			if (years->neg != 0 || year >= 10000)
				{ ret = R_ERROR_BAD_VALUE; goto end; }
			*outLen = 20;
			if (bufLen < 20) { ret = R_ERROR_BUFFER_TOO_SMALL; goto end; }
			sprintf(out,"%04ld:%02ld:%02ld:%02ld:%02ld:%02ld",
				year,month,day,hour,min,sec);
			}
		BN_free(days);
		BN_free(years);
		BN_free(yearStart);
		BN_free(leapDays);
		}
	else if (kind == R_TIME_FMT_SECONDS)
		{
		long val;
		long one;
		char *src;
		char *dst;
		long i;
		long len;
		long sign;
		long found;
		char pad;

		pad = 0;
		len = 8;
		sign = 1;
		found = 0;
		if (t->secHi < 0)
			{
			sign = -1;
			pad = ~pad;
			}
		val = sign * t->secHi;
		for (i = 3; i >= 0; i--)
			{
			if (((val >> (i*8)) & 0xff) != 0) { found = 1; break; }
			len--;
			}
		if (found == 0)
			{
			val = sign * t->secLo;
			for (i = 3; i >= 0; i--)
				{
				if (((val >> (i*8)) & 0xff) != 0) break;
				len--;
				}
			}
		if (len > bufLen)
			{
			*outLen = len;
			ret = R_ERROR_BUFFER_TOO_SMALL;
			goto end;
			}
		memset(out,pad,bufLen-len);
		*outLen = bufLen;
		one = 1;
		if (*(char *)&one)
			{
			dst = out+bufLen-1;
			src = (char *)&t->secLo;
			for (i = 0; i < sizeof(long); i++)
				{
				*dst-- = *src++;
				if (i == len) break;
				}
			if (i != len)
				{
				src = (char *)t;
				for (i = 0; i < sizeof(long); i++)
					{
					*dst-- = *src++;
					if (i+4 == len) break;
					}
				}
			}
		else
			{
			dst = out+bufLen-1;
			src = (char *)&t->secLo;
			src += 3;
			for (i = 0; i < sizeof(long); i++)
				{
				*dst-- = *src--;
				if (i == len) break;
				}
			if (i != len)
				{
				src = (char *)t;
				src += 3;
				for (i = 0; i < sizeof(long); i++)
					{
					*dst-- = *src--;
					if (i+4 == len) break;
					}
				}
			}
		}
	else
		{
		ret = R_ERROR_BAD_FORMAT;
		goto end;
		}
end:
	return ret;
	}
