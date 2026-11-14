/*
 * Small/amx natives "long*": 64-bit integers. A value takes two consecutive
 * cells of AMX memory, low cell first, and a native gets its AMX address.
 */

#include "amx.h"
#include "natives/smallext.h"
#include "script/scenewindow.h"
#include "script/script.h"

typedef union {
	cell c[2];
	long long ll;
} amxlong;

/* Reads the 64-bit value at amx_addr into *out; 0 if the address is invalid. */
static amxlong *GetLong(amxlong *out, AMX *amx, cell amx_addr)
{
	amxlong u;
	cell *cptr;
	cell *d;

	if (amx_GetAddr(amx, amx_addr, &cptr) == AMX_ERR_NONE) {
		d = u.c;
		d[0] = *cptr;
		d[1] = *++cptr;
		*out = u;
	} else {
		amxlong zero;

		zero.ll = 0;
		*out = zero;
	}
	return out;
}

/* Stores value at amx_addr, if the address is valid. */
static void SetLong(AMX *amx, cell amx_addr, long long value)
{
	amxlong u;
	cell *cptr;

	u.ll = value;
	if (amx_GetAddr(amx, amx_addr, &cptr) == AMX_ERR_NONE) {
		cptr[0] = u.c[0];
		cptr[1] = u.c[1];
	}
}

static cell AMX_NATIVE_CALL n_longmul(AMX *amx, cell *params)
{
	amxlong a, b;

	GetLong(&a, amx, params[2]);
	GetLong(&b, amx, params[3]);

	SetLong(amx, params[1], a.ll * b.ll);
	return 0;
}

static cell AMX_NATIVE_CALL n_longdiv(AMX *amx, cell *params)
{
	amxlong a, b;

	GetLong(&a, amx, params[2]);
	GetLong(&b, amx, params[3]);

	SetLong(amx, params[1], a.ll / b.ll);
	return 0;
}

static cell AMX_NATIVE_CALL n_longadd(AMX *amx, cell *params)
{
	amxlong a, b;

	GetLong(&a, amx, params[2]);
	GetLong(&b, amx, params[3]);

	SetLong(amx, params[1], a.ll + b.ll);
	return 0;
}

static cell AMX_NATIVE_CALL n_longsub(AMX *amx, cell *params)
{
	amxlong a, b;

	GetLong(&a, amx, params[2]);
	GetLong(&b, amx, params[3]);

	SetLong(amx, params[1], a.ll - b.ll);
	return 0;
}

static cell AMX_NATIVE_CALL n_longcmp(AMX *amx, cell *params)
{
	amxlong a, b;
	long long d;

	GetLong(&a, amx, params[1]);
	GetLong(&b, amx, params[2]);
	d = a.ll - b.ll;

	if (d > 0)
		return 1;
	else if (d < 0)
		return -1;
	else
		return 0;
}

AMX_NATIVE_INFO smallext_long[] = {
	{ "longadd", n_longadd },
	{ "longsub", n_longsub },
	{ "longmul", n_longmul },
	{ "longdiv", n_longdiv },
	{ "longcmp", n_longcmp },
	{ 0, 0 },
};
