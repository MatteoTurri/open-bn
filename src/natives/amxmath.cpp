/*
 * Small/amx natives sin, cos, tan and sqrt. The trigonometric ones use the
 * graphics engine tables, with the angle converted from radians to units of
 * 1/32768 of a half turn.
 */

#include "amx.h"
#include "natives/smallext.h"
#include "script/scenewindow.h"
#include "script/script.h"

#include <math.h>

#include "gfx/gmtrig.h"

static cell AMX_NATIVE_CALL n_sin(AMX *amx, cell *params)
{
	float fA = amx_ctof(params[1]);
	float fR = gmGetTableSin((int)(fA * 32768.0f / 3.14159274f));
	return amx_ftoc(fR);
}

static cell AMX_NATIVE_CALL n_cos(AMX *amx, cell *params)
{
	float fA = amx_ctof(params[1]);
	float fR = gmGetTableCos((int)(fA * 32768.0f / 3.14159274f));
	return amx_ftoc(fR);
}

static cell AMX_NATIVE_CALL n_tan(AMX *amx, cell *params)
{
	float fA = amx_ctof(params[1]);
	int ang = (int)(fA * 32768.0f / 3.14159274f);
	float fR = gmGetTableSin(ang) / gmGetTableCos(ang);
	return amx_ftoc(fR);
}

static cell AMX_NATIVE_CALL n_sqrt(AMX *amx, cell *params)
{
	float fR = sqrtf(amx_ctof(params[1]));
	return amx_ftoc(fR);
}

AMX_NATIVE_INFO smallext_math[] = {
	{ "sin", n_sin },
	{ "cos", n_cos },
	{ "tan", n_tan },
	{ "sqrt", n_sqrt },
	{ 0, 0 },
};
