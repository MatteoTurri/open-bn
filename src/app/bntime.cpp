/*
 * Date and time helpers. A timestamp is a signed 64-bit count of seconds
 * since 1600-01-01 00:00:00; the PS2 system clock runs on JST (city 51).
 */

#include <stdio.h>
#include "systemoptions.h"
#include "bntime.h"

struct TzEntry {
	short zone;              /* offset from GMT in minutes */
	unsigned char city;      /* city id; 128 terminates the table */
};

static TzEntry tzTable[] = {
	{ 270, 0 },
	{ 60, 1 },
	{ 60, 2 },
	{ 60, 3 },
	{ 240, 4 },
	{ 480, 5 },
	{ 570, 6 },
	{ 600, 7 },
	{ 630, 8 },
	{ 60, 9 },
	{ 240, 10 },
	{ 180, 11 },
	{ 360, 12 },
	{ 120, 13 },
	{ 60, 14 },
	{ 60, 15 },
	{ 120, 16 },
	{ -480, 17 },
	{ -420, 18 },
	{ -360, 19 },
	{ -300, 20 },
	{ -240, 21 },
	{ -210, 22 },
	{ -60, 23 },
	{ -240, 24 },
	{ -360, 25 },
	{ 480, 26 },
	{ 60, 27 },
	{ 120, 28 },
	{ 60, 29 },
	{ 60, 30 },
	{ 120, 31 },
	{ 120, 32 },
	{ 720, 33 },
	{ 120, 34 },
	{ 60, 35 },
	{ 240, 36 },
	{ 60, 37 },
	{ 60, 38 },
	{ 120, 39 },
	{ -240, 40 },
	{ -180, 41 },
	{ -60, 42 },
	{ 60, 43 },
	{ 0, 44 },
	{ 330, 45 },
	{ 210, 46 },
	{ 180, 47 },
	{ 0, 48 },
	{ 120, 49 },
	{ 60, 50 },
	{ 540, 51 },
	{ 120, 52 },
	{ 240, 53 },
	{ 300, 54 },
	{ 360, 55 },
	{ 180, 56 },
	{ 300, 57 },
	{ 120, 58 },
	{ 120, 59 },
	{ 60, 60 },
	{ 120, 61 },
	{ 60, 62 },
	{ 60, 63 },
	{ 60, 64 },
	{ -480, 65 },
	{ -420, 66 },
	{ -360, 67 },
	{ -660, 68 },
	{ 60, 69 },
	{ 0, 70 },
	{ 60, 71 },
	{ 345, 72 },
	{ 60, 73 },
	{ 660, 74 },
	{ 720, 75 },
	{ 60, 76 },
	{ 240, 77 },
	{ 300, 78 },
	{ -300, 79 },
	{ 60, 80 },
	{ -60, 81 },
	{ 0, 82 },
	{ -240, 83 },
	{ 240, 84 },
	{ 120, 85 },
	{ 120, 86 },
	{ 180, 87 },
	{ 240, 88 },
	{ 300, 89 },
	{ 360, 90 },
	{ 420, 91 },
	{ 480, 92 },
	{ 540, 93 },
	{ 600, 94 },
	{ 660, 95 },
	{ 720, 96 },
	{ -660, 97 },
	{ 60, 98 },
	{ 180, 99 },
	{ 60, 100 },
	{ 60, 101 },
	{ 120, 102 },
	{ 0, 103 },
	{ 60, 104 },
	{ 60, 105 },
	{ 60, 106 },
	{ 120, 107 },
	{ 60, 108 },
	{ 120, 109 },
	{ 120, 110 },
	{ 240, 111 },
	{ 0, 112 },
	{ -600, 113 },
	{ -540, 114 },
	{ -480, 115 },
	{ -420, 116 },
	{ -360, 117 },
	{ -300, 118 },
	{ 300, 119 },
	{ -240, 120 },
	{ 60, 121 },
	{ 420, 122 },
	{ 480, 123 },
	{ 480, 124 },
	{ 480, 125 },
	{ 480, 126 },
	{ 540, 127 },
	{ 0, 128 },
};

int tzGetIndexFromCityID(unsigned char id)
{
	int i;

	for (i = 0; tzTable[i].city != 128; i++)
		if (tzTable[i].city == id)
			return i;
	return i;
}

short tzGetTimeZone(int index)
{
	return tzTable[index].zone;
}

static char dateBuf[64];
static char timeBuf[64];

/* The \007 sequences are text renderer markup: \007p@0 / \007p00 switch
   the style, \007r0.80 / \007r0.00 scale the glyphs. */
char *format_date(int year, int month, int day, int type)
{
	char *p = 0;

	switch (type) {
	case 0:
		if (year < 0)
			p = "\007p@0----\007p00/\007p@0--\007p00/\007p@0--\007p00";
		else {
			sprintf(dateBuf, "%04d/%02d/%02d", year, month, day);
			p = dateBuf;
		}
		break;
	case 1:
		if (year < 0)
			p = "\007p@0--\007p00/\007p@0--\007p00/\007p@0----\007p00";
		else {
			sprintf(dateBuf, "%02d/%02d/%04d", month, day, year);
			p = dateBuf;
		}
		break;
	case 2:
		if (year < 0)
			p = "\007p@0--\007p00/\007p@0--\007p00/\007p@0----\007p00";
		else {
			sprintf(dateBuf, "%02d/%02d/%04d", day, month, year);
			p = dateBuf;
		}
		break;
	}
	return p;
}

char *format_time(int hour, int min, int sec, int type)
{
	char *p = 0;

	switch (type) {
	case 0:
		if (hour < 0)
			p = "\007p@0--\007p00:\007p@0--\007p00:\007p@0--\007p00";
		else {
			sprintf(timeBuf, "\007p@0%2d\007p00:%02d:%02d", hour, min, sec);
			p = timeBuf;
		}
		break;
	case 1:
		if (hour < 0)
			p = "\007p@0--\007p00:\007p@0--\007p00:\007p@0--\007p00 \007r0.80\007p@A-\007p@M-\007p00\007r0.00";
		else {
			int h = hour % 12;
			sprintf(timeBuf, "\007p@0%2d\007p00:%02d:%02d %s", h ? h : 12, min, sec,
			    hour <= 11 ? "\007r0.80\007p@AA\007p00M\007r0.00" : "\007r0.80\007p@AP\007p00M\007r0.00");
			p = timeBuf;
		}
		break;
	}
	return p;
}

/* Days per month; the second row is for leap years. */
static const int monthDays[2][12] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

/* City 51 is the time zone of the system clock (540 minutes, JST). */
long long _calc_localtime(long long t)
{
	t -= tzGetTimeZone(tzGetIndexFromCityID(51)) * 60;
	t += SysOption.getTimeZone() * 60;
	if (SysOption.getSummerTime())
		t += 3600;
	return t;
}

long long _calc_systemtime(long long t)
{
	if (SysOption.getSummerTime())
		t -= 3600;
	t -= SysOption.getTimeZone() * 60;
	t += tzGetTimeZone(tzGetIndexFromCityID(51)) * 60;
	return t;
}

long long _calc_gm2systemtime(long long t)
{
	return t + tzGetTimeZone(tzGetIndexFromCityID(51)) * 60;
}

long long _calc_system2gmtime(long long t)
{
	return t - tzGetTimeZone(tzGetIndexFromCityID(51)) * 60;
}

void calc_localtime(int *year, int *month, int *day, int *hour, int *min, int *sec)
{
	if (*hour < 0)
		return;
	if (*year < 0)
		return;
	c2t(_calc_localtime(t2c(*year, *month, *day, *hour, *min, *sec)), year, month, day, hour, min, sec);
}

void calc_systemtime(int *year, int *month, int *day, int *hour, int *min, int *sec)
{
	if (*hour < 0)
		return;
	if (*year < 0)
		return;
	c2t(_calc_systemtime(t2c(*year, *month, *day, *hour, *min, *sec)), year, month, day, hour, min, sec);
}

int isLeapYear(int y)
{
	if (y % 400 != 0) {
		if (y % 100 != 0)
			return y % 4 == 0;
		return 0;
	}
	return 1;
}

long long t2c(int year, int month, int day, int hour, int min, int sec)
{
	int y = year - 1600;
	long long days = day - 1;
	int leap = isLeapYear(y);
	int i;

	for (i = 0; i < month - 1; i++)
		days += monthDays[leap][i];
	days += 365 * y + (y + 3) / 4 - (y + 99) / 100 + (y + 399) / 400;
	days = days * 24 + hour;
	days = days * 60 + min;
	return days * 60 + sec;
}

/* Clamps to 2000-01-01 00:00:00 .. 2099-12-31 23:59:59. */
long long clip_time(long long t)
{
	if (t < 12622780800LL)
		t = 12622780800LL;
	else if (t > 15778540799LL)
		t = 15778540799LL;
	return t;
}

/* Clamps to 2000-01-01 00:00:00 .. 2030-12-31 23:59:59. */
long long clip_time_bn(long long t)
{
	if (t < 12622780800LL)
		t = 12622780800LL;
	else if (t > 13601087999LL)
		t = 13601087999LL;
	return t;
}

void c2t(long long t, int *year, int *month, int *day, int *hour, int *min, int *sec)
{
	int leap = 0;
	int ly;
	int i;

	*sec = t % 60;
	t /= 60;
	*min = t % 60;
	t /= 60;
	*hour = t % 24;
	t /= 24;

	*year = 1600;
	if (t > 146096) {
		t -= 146097;
		*year = 1600 + (t / 146097 + 1) * 400;
		t %= 146097;
		leap = 1;
	} else
		leap ^= 1;
	if (t >= 36524 + leap) {
		t -= 36524 + leap;
		*year += (t / 36524 + 1) * 100;
		t %= 36524;
		leap = 1;
	} else
		leap ^= 1;
	if (t >= 1461 - leap) {
		t -= 1461 - leap;
		*year += (t / 1461 + 1) * 4;
		t %= 1461;
		leap = 1;
	} else
		leap ^= 1;
	if (t >= 365 + leap) {
		t -= 365 + leap;
		*year += t / 365 + 1;
		t %= 365;
	}

	ly = isLeapYear(*year);
	for (i = 0; t >= monthDays[ly][i]; i++)
		t -= monthDays[ly][i];
	*month = i + 1;
	*day = t + 1;
}
