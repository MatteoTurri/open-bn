/*
 * HTTP dates: parsing of the RFC 1123, RFC 850 and asctime formats into
 * seconds since the epoch, and formatting as RFC 1123.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "http/scehttp.h"

/* Days per month, [leap year][month - 1]. */
static char g_days_in_month[2][12] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

static const char *g_wdayName[7] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat",
};
/* clang-format off */
static const char *g_monthName[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};
/* clang-format on */

/* Returns 0..6 for an abbreviated weekday name (Sun = 0), -1 if none. */
static int http_parse_weekday(const char *str)
{
	switch (str[0]) {
	case 'M':
		if (str[1] != 'o')
			goto bad;
		if (str[2] != 'n')
			goto bad;
		return 1;
	case 'T':
		if (str[1] == 'u' && str[2] == 'e')
			return 2;
		if (str[1] != 'h')
			goto bad;
		if (str[2] != 'u')
			goto bad;
		return 4;
	case 'W':
		if (str[1] != 'e')
			goto bad;
		if (str[2] != 'd')
			goto bad;
		return 3;
	case 'F':
		if (str[1] != 'r')
			goto bad;
		if (str[2] != 'i')
			goto bad;
		return 5;
	case 'S':
		if (str[1] == 'u' && str[2] == 'n')
			return 0;
		if (str[1] != 'a')
			goto bad;
		if (str[2] != 't')
			goto bad;
		return 6;
	}
bad:
	return -1;
}

/* Returns 0..6 for a full weekday name (RFC 850), -1 if none. */
static int http_parse_weekday_long(const char *str, int len)
{
	switch (str[0]) {
	case 'M':
		if (len != 6)
			goto bad;
		if (strncmp(str + 1, "onday", 5) != 0)
			goto bad;
		return 1;
	case 'T':
		if (len == 7 && strncmp(str + 1, "uesday", 6) == 0)
			return 2;
		if (len != 8)
			goto bad;
		if (strncmp(str + 1, "hursday", 7) != 0)
			goto bad;
		return 4;
	case 'W':
		if (len != 9)
			goto bad;
		if (strncmp(str + 1, "ednesday", 8) != 0)
			goto bad;
		return 3;
	case 'F':
		if (len != 6)
			goto bad;
		if (strncmp(str + 1, "riday", 5) != 0)
			goto bad;
		return 5;
	case 'S':
		if (len == 6 && strncmp(str + 1, "unday", 5) == 0)
			return 0;
		if (len != 8)
			goto bad;
		if (strncmp(str + 1, "aturday", 7) != 0)
			goto bad;
		return 6;
	}
bad:
	return -1;
}

/* Returns 1..12 for an abbreviated month name, -1 if none. */
static int http_parse_month(const char *str)
{
	switch (str[0]) {
	case 'J':
		if (str[1] == 'a' && str[2] == 'n')
			return 1;
		if (str[1] != 'u')
			goto bad;
		if (str[2] == 'n')
			return 6;
		if (str[2] != 'l')
			goto bad;
		return 7;
	case 'F':
		if (str[1] != 'e')
			goto bad;
		if (str[2] != 'b')
			goto bad;
		return 2;
	case 'M':
		if (str[1] != 'a')
			goto bad;
		if (str[2] == 'r')
			return 3;
		if (str[2] != 'y')
			goto bad;
		return 5;
	case 'A':
		if (str[1] == 'p' && str[2] == 'r')
			return 4;
		if (str[1] != 'u')
			goto bad;
		if (str[2] != 'g')
			goto bad;
		return 8;
	case 'S':
		if (str[1] != 'e')
			goto bad;
		if (str[2] != 'p')
			goto bad;
		return 9;
	case 'O':
		if (str[1] != 'c')
			goto bad;
		if (str[2] != 't')
			goto bad;
		return 10;
	case 'N':
		if (str[1] != 'o')
			goto bad;
		if (str[2] != 'v')
			goto bad;
		return 11;
	case 'D':
		if (str[1] != 'e')
			goto bad;
		if (str[2] != 'c')
			goto bad;
		return 12;
	}
bad:
	return -1;
}

/* Parse a fixed-width decimal field of 2 or 4 digits; -1 if a byte is not
   a digit. */
static int http_parse_2digits(const char *p)
{
	if (!isdigit(p[0]))
		goto bad;
	if (!isdigit(p[1]))
		goto bad;
	return (p[0] - '0') * 10 + (p[1] - '0');
bad:
	return -1;
}

static int http_parse_4digits(const char *p)
{
	if (!isdigit(p[0]))
		goto bad;
	if (!isdigit(p[1]))
		goto bad;
	if (!isdigit(p[2]))
		goto bad;
	if (!isdigit(p[3]))
		goto bad;
	return (p[0] - '0') * 1000 + (p[1] - '0') * 100 + (p[2] - '0') * 10 + (p[3] - '0');
bad:
	return -1;
}

/* Returns the seconds since 1970-01-01 of a UTC date, computed from the
   day count of the proleptic Gregorian calendar. */
static long long http_make_time(int year, int month, int day, int hour, int min, int sec)
{
	long long days;

	month -= 2;
	if (month <= 0) {
		month += 12;
		year--;
	}
	days = year / 4 - year / 100 + year / 400 + 367 * month / 12 + day;
	days += 365 * year;
	days -= 719499;
	return ((days * 24 + hour) * 60 + min) * 60 + sec;
}

/* Parses "Wdy, DD Mon YYYY HH:MM:SS GMT" (RFC 1123), or the RFC 850 form
   with the full weekday name and '-' between the date fields. Returns 0, or
   -1 on error. */
static int http_parse_date_fmt2(const char *str, int len, long long *out)
{
	const char *end;
	char sep1, sep2;
	int day, month, year, hour, min, sec, leap;

	end = sceHTTPLibSkipToken((char *)str);
	if (end - str == 3) {
		if (http_parse_weekday(str) >= 0)
			goto cont;
		goto bad;
	}
	if (http_parse_weekday_long(str, end - str) < 0)
		goto bad;
cont:
	len -= end - str;
	if (len < 26)
		goto bad;
	str = end;
	if (*str++ != ',')
		goto bad;
	sep1 = *str++;
	if (sep1 != ' ')
		goto bad;
	day = http_parse_2digits(str);
	if (day <= 0)
		goto bad;
	str += 2;
	if (*str != sep1 && *str != '-')
		return -1;
	sep2 = *str++;
	month = http_parse_month(str);
	if (month <= 0)
		goto bad;
	if (month >= 13)
		goto bad;
	str += 3;
	if (*str++ != sep2)
		goto bad;
	year = http_parse_4digits(str);
	if (year < 1970)
		goto bad;
	str += 4;
	if (*str++ != ' ')
		goto bad;
	hour = http_parse_2digits(str);
	if (hour < 0)
		goto bad;
	if (hour >= 24)
		goto bad;
	str += 2;
	sep2 = *str++;
	if (sep2 != ':')
		goto bad;
	min = http_parse_2digits(str);
	if (min < 0)
		goto bad;
	if (min >= 60)
		goto bad;
	str += 2;
	if (*str++ != sep2)
		goto bad;
	sec = http_parse_2digits(str);
	if (sec < 0)
		goto bad;
	if (sec >= 60)
		goto bad;
	if (sceHTTPLibStrsub(str + 2, " GMT") != 0)
		goto bad;
	if (year % 4 != 0)
		goto notleap;
	if (year % 100 != 0)
		goto isleap;
	if (year % 400 != 0)
		goto notleap;
isleap:
	leap = 1;
	goto gotleap;
notleap:
	leap = 0;
gotleap:
	if (g_days_in_month[leap][month - 1] < day)
		goto bad;
	*out = http_make_time(year, month, day, hour, min, sec);
	return 0;
bad:
	return -1;
}

/* Parses an asctime-like date, "Wdy Mon DD YYYY HH:MM:SS". Returns 0, or
   -1 on error. */
static int http_parse_date_fmt1(const char *str, int len, long long *out)
{
	const char *end;
	char sep, c;
	int day, month, year, hour, min, sec, leap;

	end = sceHTTPLibSkipToken((char *)str);
	if (end - str == 3) {
		if (http_parse_weekday(str) >= 0)
			goto cont;
		goto bad;
	}
	if (http_parse_weekday_long(str, end - str) < 0)
		goto bad;
cont:
	len -= end - str;
	if (len < 21)
		goto bad;
	str = end;
	sep = *str++;
	if (sep != ' ')
		goto bad;
	month = http_parse_month(str);
	if (month <= 0)
		goto bad;
	if (month >= 13)
		goto bad;
	str += 3;
	c = *str++;
	if (c != sep)
		goto bad;
	sep = c;
	day = http_parse_2digits(str);
	if (day <= 0)
		goto bad;
	str += 2;
	c = *str++;
	if (c != sep)
		goto bad;
	sep = c;
	year = http_parse_4digits(str);
	if (year < 1970)
		goto bad;
	str += 4;
	if (*str++ != sep)
		goto bad;
	hour = http_parse_2digits(str);
	if (hour < 0)
		goto bad;
	if (hour >= 24)
		goto bad;
	str += 2;
	sep = *str++;
	if (sep != ':')
		goto bad;
	min = http_parse_2digits(str);
	if (min < 0)
		goto bad;
	if (min >= 60)
		goto bad;
	str += 2;
	if (*str++ != sep)
		goto bad;
	sec = http_parse_2digits(str);
	if (sec < 0)
		goto bad;
	if (sec >= 60)
		goto bad;
	if (year % 4 != 0)
		goto notleap;
	if (year % 100 != 0)
		goto isleap;
	if (year % 400 != 0)
		goto notleap;
isleap:
	leap = 1;
	goto gotleap;
notleap:
	leap = 0;
gotleap:
	if (g_days_in_month[leap][month - 1] < day)
		goto bad;
	*out = http_make_time(year, month, day, hour, min, sec);
	return 0;
bad:
	return -1;
}

/* Stores in *out the time of an HTTP date; returns 0, or -1 on error. */
int sceHTTPLibDateToTime(const char *str, int len, long long *out)
{
	if (len < 3)
		return -1;
	if (str[3] == ' ')
		str = http_parse_date_fmt1(str, len, out);  /* redundant as written */
	else
		str = http_parse_date_fmt2(str, len, out);  /* redundant as written */
	return str;
}

/* Formats t (seconds since the epoch, UTC) as an RFC 1123 date in a new
   32-byte buffer. */
char *sceHTTPLibTimeToDate(long long t)
{
	char *buf;
	long long sec, min, hour;
	int wday, years, yday, mon, leap;

	buf = xmalloc(32);
	if (buf == 0)
		return 0;

	sec = t % 60;
	t /= 60;
	min = t % 60;
	t /= 60;
	hour = t % 24;
	t /= 24;
	wday = (t + 4) % 7;
	years = t / 365;
	if ((long long)(years * 365 + (years + 1) / 4) > t)
		years--;
	yday = (int)t - (years * 365 + (years + 1) / 4);

	leap = years % 4 == 2;
	mon = 0;
	yday -= g_days_in_month[leap][0];
	if (yday >= 0) {
		do {
			mon++;
			if (mon >= 12)
				break;
			yday -= g_days_in_month[leap][mon];
		} while (yday >= 0);
	}

	yday += g_days_in_month[leap][mon] + 1;
	sprintf(buf, "%s, %02d %s %4d %02d:%02d:%02d GMT", g_wdayName[wday], yday, g_monthName[mon], years + 1970,
	    (int)hour, (int)min, (int)sec);
	buf[29] = 0;
	return buf;
}
