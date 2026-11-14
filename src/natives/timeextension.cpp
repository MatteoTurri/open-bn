/*
 * Small/amx natives "time"/"date"/"tickcount" and the frame tick counter.
 * TimeExtension_Run is called once per tick; tickcount converts the count
 * to milliseconds dividing by SystemOptions::m_frameRate (60 by default).
 */

#include <time.h>
#include "amx.h"
#include "natives/smallext.h"
#include "script/scenewindow.h"
#include "script/script.h"

#include "app/systemoptions.h"
#include "timeextension.h"

static int s_timeTick;

/* Moves an RTC time to the configured local time (time zone and summer time). */
static void AdjustToLocalTime(time_t *t)
{
	*t += SysOption.getTimeZone() * 60;
	*t += SysOption.getSummerTime() * 3600;
}

/*
 * time(&h, &m, &s) / date(&y, &mo, &d) [, utc]: as amxtime.c, but local time
 * comes from gmtime() on an adjusted instant; an optional fourth argument
 * equal to 1 asks for UTC.
 */
static cell AMX_NATIVE_CALL n_time(AMX *amx, cell *params)
{
	time_t sec1970;
	struct tm gtm;
	cell *cptr;

	time(&sec1970);
	if (params[0] != 4 * (int)sizeof(cell) || params[4] != 1)
		AdjustToLocalTime(&sec1970);
	gtm = *gmtime(&sec1970);

	if (amx_GetAddr(amx, params[1], &cptr) == AMX_ERR_NONE)
		*cptr = gtm.tm_hour;
	if (amx_GetAddr(amx, params[2], &cptr) == AMX_ERR_NONE)
		*cptr = gtm.tm_min;
	if (amx_GetAddr(amx, params[3], &cptr) == AMX_ERR_NONE)
		*cptr = gtm.tm_sec;
	return 0;
}

static cell AMX_NATIVE_CALL n_date(AMX *amx, cell *params)
{
	time_t sec1970;
	struct tm gtm;
	cell *cptr;

	time(&sec1970);
	if (params[0] != 4 * (int)sizeof(cell) || params[4] != 1)
		AdjustToLocalTime(&sec1970);
	gtm = *gmtime(&sec1970);

	if (amx_GetAddr(amx, params[1], &cptr) == AMX_ERR_NONE)
		*cptr = gtm.tm_year + 1900;
	if (amx_GetAddr(amx, params[2], &cptr) == AMX_ERR_NONE)
		*cptr = gtm.tm_mon + 1;
	if (amx_GetAddr(amx, params[3], &cptr) == AMX_ERR_NONE)
		*cptr = gtm.tm_mday;
	return 0;
}

void TimeExtension_Run()
{
	s_timeTick++;
}

int tickcount()
{
	return s_timeTick * 1000 / SysOption.m_frameRate;
}

static cell AMX_NATIVE_CALL n_tickcount(AMX *amx, cell *params)
{
	return tickcount();
}

AMX_NATIVE_INFO smallext_time[] = {
	{ "time", n_time },
	{ "date", n_date },
	{ "tickcount", n_tickcount },
	{ 0, 0 },
};
