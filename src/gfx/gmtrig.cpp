/*
 * Sine/cosine lookup table of the gm layer.
 * Angles are fixed point: 0x10000 = full turn, 0x4000 = 90 degrees. The table
 * holds one quarter wave (16385 floats) and is filled once by gmInitSinTable.
 */

#include <math.h>
#include "gmtrig.h"

static float gmSinTable[16385];
static int gmSinTableInit = 0;

/* Folds the angle onto the first quarter wave: sine is odd and
   sin(x) = sin(32768 - x). */
float gmGetTableSin(int angle)
{
	int sign = (unsigned int)angle >> 31;
	int idx;
	float t;

	if (sign)
		idx = (-angle) % 65536;
	else
		idx = angle % 65536;
	if (idx > 32767) {
		sign ^= 1;
		idx -= 32768;
	}
	if (idx >= 16384)
		idx = 32768 - idx;
	t = gmSinTable[idx];
	return sign ? -t : t;
}

/* cos(angle) = sin(angle + 90 degrees) */
float gmGetTableCos(int angle)
{
	int x = angle + 16384;
	int sign = (unsigned int)x >> 31;
	int idx;
	float t;

	if (sign)
		idx = (-x) % 65536;
	else
		idx = x % 65536;
	if (idx > 32767) {
		sign ^= 1;
		idx -= 32768;
	}
	if (idx >= 16384)
		idx = 32768 - idx;
	t = gmSinTable[idx];
	return sign ? -t : t;
}

void gmInitSinTable(void)
{
	int i;

	if (gmSinTableInit)
		return;
	for (i = 0; i < 16385; i++)
		gmSinTable[i] = (float)sin((float)i * 1.5707963267948966 / 16385.0);
	gmSinTableInit = 1;
}
