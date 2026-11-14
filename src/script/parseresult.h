/*
 * Marshalling between script arrays and engine values (vectors, vec4).
 */

#ifndef PARSERESULT_H
#define PARSERESULT_H

#include <vector>
#include "gfx/vec.h"
using std::vector;

void ParseFloatArray(vector<float> &v, const float *src, int n);
void ParseIntArray(vector<int> &v, const int *src, int n);
void ParseVec4(vec4 &dst, const float *src, int n);
void ResultVec4(const vec4 &src, float *dst, int n);
void ParseFloatArray(float *dst, int dstCount, const float *src, int srcCount);
void ResultFloatArray(const std::vector<float> &src, float *dst, int dstCount);
void ResultFloatArray(const float *src, int srcCount, float *dst, int dstCount);
void ParseIntArray(int *dst, int dstCount, const int *src, int srcCount);
void ResultIntArray(const std::vector<int> &src, int *dst, int dstCount);
void ResultIntArray(const int *src, int srcCount, int *dst, int dstCount);

#endif
