/*
 * Script <-> engine marshalling: Parse* copies values from a script array into
 * an engine object, Result* copies them back; each for raw arrays and vectors.
 */
#include "parseresult.h"

/* Unit matrix and unit w row: the file-scope objects built at startup. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

/* Grows v to at least max(n, 1) elements and copies them from src. */
void ParseFloatArray(vector<float> &v, const float *src, int n)
{
	int positive = n > 0;
	int count = positive ? n : 1;
	int i;

	if ((int)v.size() < count)
		v.resize(count);
	for (i = 0; i < count; i++)
		v[i] = src[i];
}

void ParseIntArray(vector<int> &v, const int *src, int n)
{
	int positive = n > 0;
	int count = positive ? n : 1;
	int i;

	if ((int)v.size() < count)
		v.resize(count);
	for (i = 0; i < count; i++)
		v[i] = src[i];
}

static inline int clamp_lo(int v)
{
	if (v <= 0)
		v = 1;
	return v;
}

static inline int min_i(int a, int b)
{
	if (a < b)
		return a;
	else
		return b;
}

void ParseVec4(vec4 &dst, const float *src, int n)
{
	int count = clamp_lo(n);

	count = count < 5 ? count : 4;

	switch (count) {
	case 4:
		dst.w = src[3];
	case 3:
		dst.z = src[2];
	case 2:
		dst.y = src[1];
	case 1:
		dst.x = src[0];
	}
}

void ResultVec4(const vec4 &src, float *dst, int n)
{
	int count = clamp_lo(n);

	count = count < 5 ? count : 4;

	switch (count) {
	case 4:
		dst[3] = src.w;
	case 3:
		dst[2] = src.z;
	case 2:
		dst[1] = src.y;
	case 1:
		dst[0] = src.x;
	}
}

void ParseFloatArray(float *dst, int dstCount, const float *src, int srcCount)
{
	int n = clamp_lo(srcCount);
	int i;

	if (dstCount < n)
		n = dstCount;
	for (i = 0; i < n; i++)
		*dst++ = *src++;
}

void ResultFloatArray(const std::vector<float> &src, float *dst, int dstCount)
{
	int n = min_i((int)src.size(), dstCount);
	int i;

	for (i = 0; i < n; i++)
		*dst++ = src[i];
}

void ResultFloatArray(const float *src, int srcCount, float *dst, int dstCount)
{
	int n = min_i(srcCount, dstCount);
	int i;

	for (i = 0; i < n; i++)
		*dst++ = *src++;
}

void ParseIntArray(int *dst, int dstCount, const int *src, int srcCount)
{
	int n = clamp_lo(srcCount);
	int i;

	if (dstCount < n)
		n = dstCount;
	for (i = 0; i < n; i++)
		*dst++ = *src++;
}

void ResultIntArray(const std::vector<int> &src, int *dst, int dstCount)
{
	int n = min_i((int)src.size(), dstCount);
	int i;

	for (i = 0; i < n; i++)
		*dst++ = src[i];
}

void ResultIntArray(const int *src, int srcCount, int *dst, int dstCount)
{
	int n = min_i(srcCount, dstCount);
	int i;

	for (i = 0; i < n; i++)
		*dst++ = *src++;
}
