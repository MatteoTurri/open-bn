/*
 * Small/amx natives "float*": the standard Small float library. A float
 * travels in a 32-bit cell by reinterpreting its bits (amx_ctof/amx_ftoc).
 */

#include "amx.h"
#include "natives/smallext.h"
#include "script/scenewindow.h"
#include "script/script.h"

#include <math.h>

#define REAL       float

static cell AMX_NATIVE_CALL n_float(AMX *amx, cell *params)
{
	REAL fValue = (REAL)params[1];
	return amx_ftoc(fValue);
}

static cell AMX_NATIVE_CALL n_floatmul(AMX *amx, cell *params)
{
	REAL fRes = amx_ctof(params[1]) * amx_ctof(params[2]);
	return amx_ftoc(fRes);
}

static cell AMX_NATIVE_CALL n_floatdiv(AMX *amx, cell *params)
{
	REAL fRes = amx_ctof(params[1]) / amx_ctof(params[2]);
	return amx_ftoc(fRes);
}

static cell AMX_NATIVE_CALL n_floatadd(AMX *amx, cell *params)
{
	REAL fRes = amx_ctof(params[1]) + amx_ctof(params[2]);
	return amx_ftoc(fRes);
}

static cell AMX_NATIVE_CALL n_floatsub(AMX *amx, cell *params)
{
	REAL fRes = amx_ctof(params[1]) - amx_ctof(params[2]);
	return amx_ftoc(fRes);
}

static cell AMX_NATIVE_CALL n_floatfract(AMX *amx, cell *params)
{
	REAL fA = amx_ctof(params[1]);
	fA = fA - floorf(fA);
	return amx_ftoc(fA);
}

/* params[2]: 1 = down, 2 = up, anything else = to nearest. */
static cell AMX_NATIVE_CALL n_floatround(AMX *amx, cell *params)
{
	REAL fA = amx_ctof(params[1]);
	REAL fB;

	switch (params[2]) {
	case 1:
		fA = floorf(fA);
		break;
	case 2:
		fB = floorf(fA);
		if (fA >= 0.0F && fA - fB != 0.0F)
			fB = fB + 1.0F;
		fA = fB;
		break;
	default:
		fA = floorf(fA + 0.5F);
		break;
	}
	return (cell)fA;
}

static cell AMX_NATIVE_CALL n_floatcmp(AMX *amx, cell *params)
{
	REAL fA = amx_ctof(params[1]);
	REAL fB = amx_ctof(params[2]);

	if (fA == fB)
		return 0;
	else if (fA > fB)
		return 1;
	else
		return -1;
}

AMX_NATIVE_INFO smallext_float[] = {
	{ "float", n_float },
	{ "floatmul", n_floatmul },
	{ "floatdiv", n_floatdiv },
	{ "floatadd", n_floatadd },
	{ "floatsub", n_floatsub },
	{ "floatfract", n_floatfract },
	{ "floatround", n_floatround },
	{ "floatcmp", n_floatcmp },
	{ 0, 0 },
};
