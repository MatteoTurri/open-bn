/*
 * time_mi_import: the inverse of time_mi_export: reads a UTCTime (kind 1), a GeneralizedTime (2),
 * "YYYY:MM:DD:hh:mm:ss" (6) or raw big-endian seconds (3) into the time of the scratch. The fields
 * are read with a template of ten {value, width, separator} records.
 */

#include <ctype.h>

#include <stdio.h>
#include "bn.h"

#include "time_mi.h"

typedef struct
	{
	long value;
	long width;
	char sep;
	} R_TimeField;



static long time_mi_scan();

int time_mi_import(scratch,kind,in,inLen)
void *scratch;
long kind;
char *in;
long inLen;
	{
	long frac;
	long tzHour;
	long tzMin;
	long tzOff;
	char sign;
	char zone;
	int ret;
	R_TimeMiPayload *t;

	frac = 0;
	tzHour = 0;
	tzMin = 0;
	tzOff = 0;
	sign = 0;
	zone = 0;
	ret = 0;
	if (in == 0 || scratch == 0)
		{
		ret = R_ERROR_NULL_ARG;
		goto end;
		}
	t = &((R_TimeMiScratch *)scratch)->time;
	if (kind == R_TIME_FMT_UTCTIME || kind == R_TIME_FMT_GENTIME || kind == R_TIME_FMT_COLONS)
		{
		BIGNUM *seconds,*leapDays;
		long leap = 0;
		long adjust = 0;
		long year,month,day,hour,min,sec,fieldCount;
		R_TimeField fields[10] = {
			{0,2,0},{0,2,0},{0,2,0},{0,2,0},{0,2,0},
			{0,2,0},{0,2,0},{0,2,0},{0,2,0},{0,0,0}};
		BIGNUM bn;
		long nbytes;
		unsigned char buf[8];

		seconds = BN_new();
		if (seconds == 0) { ret = R_ERROR_ALLOC_FAILURE; goto end; }
		if (kind == R_TIME_FMT_GENTIME)
			{
			fields[0].width = 4;
			fields[6].width = 1;
			}
		else if (kind == R_TIME_FMT_COLONS)
			{
			fields[0].width = 4;
			fields[1].sep = fields[2].sep = fields[3].sep
				= fields[4].sep = fields[5].sep = ':';
			}
		fieldCount = time_mi_scan(in,inLen,fields,10);
		year = fields[0].value;
		month = fields[1].value;
		day = fields[2].value;
		hour = fields[3].value;
		min = fields[4].value;
		sec = fields[5].value;
		zone = fields[6].sep;
		if (kind == R_TIME_FMT_UTCTIME)
			{
			if (fieldCount < 5) { ret = R_ERROR_BAD_DATA; goto end; }
			if (year < 50) year += 100;
			year += 1900;
			if (fieldCount == 5)
				{
				sec = 0;
				zone = fields[5].sep;
				}
			else if (fieldCount == 7)
				{
				sign = fields[5].sep;
				tzHour = fields[5].value;
				tzMin = fields[6].value;
				}
			else if (fieldCount == 8)
				{
				sec = fields[5].value;
				sign = fields[6].sep;
				tzHour = fields[6].value;
				tzMin = fields[7].value;
				}
			BN_set_word(seconds,year);
			}
		else if (kind == R_TIME_FMT_COLONS)
			{
			if (fieldCount != 6) { ret = R_ERROR_BAD_FORMAT; goto end; }
			zone = 90;
			BN_set_word(seconds,year);
			}
		else
			{
			if (fieldCount < 7) { ret = R_ERROR_BAD_FORMAT; goto end; }
			frac = fields[6].value;
			zone = fields[7].sep;
			if (fieldCount == 9)
				{
				sign = fields[7].sep;
				tzHour = fields[7].value;
				tzMin = fields[8].value;
				}
			BN_set_word(seconds,year);
			}
		leap = 0;
		if (IS_LEAP(seconds))
			leap = 1;
		else if (seconds->neg == 0)
			adjust = 1;
		if (!(month < 13
			&& CRYPTO_day_to_month[leap][month-1] + day
				<= CRYPTO_day_to_month[leap][month]
			&& hour < 24 && min < 60 && sec < 61
			&& tzHour < 13 && tzMin < 60
			&& (sign == '+' || sign == '-' || toupper(zone) == 'Z')))
			{
			ret = R_ERROR_BAD_DATA;
			goto end;
			}
		if (sec == 60) sec = 59;
		leapDays = BN_dup(seconds);
		if (leapDays == 0) { ret = R_ERROR_ALLOC_FAILURE; goto end; }
		BN_sub_word(seconds,1970);
		if (seconds->neg == 0) BN_sub_word(leapDays,1);
		BN_mul_word(seconds,365);
		BN_div_word(leapDays,4);
		BN_add(seconds,seconds,leapDays);
		BN_div_word(leapDays,25);
		BN_sub(seconds,seconds,leapDays);
		BN_div_word(leapDays,4);
		BN_add(seconds,seconds,leapDays);
		BN_sub_word(seconds,477);
		if (adjust && seconds->neg) BN_add_word(seconds,1);
		BN_add_word(seconds,CRYPTO_day_to_month[leap][month-1]);
		BN_add_word(seconds,day-1);
		BN_mul_word(seconds,24);
		BN_add_word(seconds,hour);
		BN_mul_word(seconds,60);
		BN_add_word(seconds,min);
		BN_mul_word(seconds,60);
		BN_add_word(seconds,sec);
		BN_init(&bn);
		BN_copy(&bn,seconds);
		BN_mask_bits(&bn,64);
		buf[0] = buf[1] = buf[2] = buf[3] = 0;
		buf[4] = buf[5] = buf[6] = buf[7] = 0;
		nbytes = (BN_num_bits(&bn)+7)/8;
		BN_bn2bin(&bn,&buf[sizeof(buf)-nbytes]);
		BN_free(&bn);
		t->secLo = buf[7] | (buf[6]<<8) | (buf[5]<<16) | (buf[4]<<24);
		t->secHi = buf[3] | (buf[2]<<8) | (buf[1]<<16) | (buf[0]<<24);
		if (seconds->neg)
			{
			t->secLo = ~t->secLo;
			t->secHi = ~t->secHi;
			t->secLo++;
			if (t->secLo == 0) t->secHi++;
			}
		t->nsec = frac * 100000000;
		BN_free(seconds);
		BN_free(leapDays);
		tzOff = (tzHour*60+tzMin)*60;
		if (sign == '+') tzOff = -tzOff;
		time_mi_offset(scratch,scratch,tzOff*1000);
		}
	else if (kind == R_TIME_FMT_SECONDS)
		{
		long one;
		char *src;
		char *dst;
		long i;

		if (inLen > 2*sizeof(long)) { ret = R_ERROR_BAD_DATA; goto end; }
		t->secHi = t->secLo = t->nsec = 0;
		one = 1;
		if (*(char *)&one)
			{
			src = in+inLen-1;
			dst = (char *)&t->secLo;
			for (i = 0; i < sizeof(long); i++)
				{
				*dst++ = *src--;
				if (src < in) break;
				}
			if (src >= in)
				{
				dst = (char *)t;
				for (i = 0; i < sizeof(long); i++)
					{
					*dst++ = *src--;
					if (src < in) break;
					}
				}
			}
		else
			{
			src = in+inLen-1;
			dst = (char *)&t->secLo;
			dst += 3;
			for (i = 0; i < sizeof(long); i++)
				{
				*dst-- = *src--;
				if (src < in) break;
				}
			if (src >= in)
				{
				dst = (char *)t;
				dst += 3;
				for (i = 0; i < sizeof(long); i++)
					{
					*dst-- = *src--;
					if (src < in) break;
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

/* Reads at most maxf numeric fields from s: each may start with a separator (which must be the one
 * the field prescribes, if any) and has up to `width` decimal digits. Returns the number of fields
 * read. */
static long time_mi_scan(s,len,f,maxf)
char *s;
long len;
R_TimeField *f;
long maxf;
	{
	long i;
	long pos;
	long n;
	char c;

	i = pos = 0;
	for (; i < maxf; i++)
		{
		if (pos >= len)
			break;
		if ((c = s[pos++]) == 0)
			break;
		if (!isdigit(c))
			{
			if (f[i].sep != 0 && f[i].sep != c)
				goto done;
			else
				{
				f[i].sep = c;
				f[i].value = 0;
				n = 0;
				}
			}
		else
			{
			f[i].value = c - '0';
			n = 1;
			}
		while (n < f[i].width)
			{
			if (pos >= len || (c = s[pos++]) == 0 || !isdigit(c))
				goto done;
			f[i].value = f[i].value * 10;
			f[i].value = f[i].value + (c - '0');
			n++;
			}
		}
done:
	return i;
	}
