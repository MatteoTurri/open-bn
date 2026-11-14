/*
 * Date and time helpers (bntime.cpp). A timestamp is a signed 64-bit count of
 * seconds since 1600-01-01 00:00:00.
 */

#ifndef BNTIME_H
#define BNTIME_H

int tzGetIndexFromCityID(unsigned char id);
short tzGetTimeZone(int index);
char *format_date(int year, int month, int day, int type);
char *format_time(int hour, int min, int sec, int type);
long long _calc_localtime(long long t);
long long _calc_systemtime(long long t);
long long _calc_gm2systemtime(long long t);
long long _calc_system2gmtime(long long t);
void calc_localtime(int *year, int *month, int *day, int *hour, int *min, int *sec);
void calc_systemtime(int *year, int *month, int *day, int *hour, int *min, int *sec);
int isLeapYear(int y);
long long t2c(int year, int month, int day, int hour, int min, int sec);
long long clip_time(long long t);
long long clip_time_bn(long long t);
void c2t(long long t, int *year, int *month, int *day, int *hour, int *min, int *sec);

#endif
