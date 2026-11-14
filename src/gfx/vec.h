/*
 * The inline operations on vec2/vec3/vec4/mat4 (gfx/vec_types.h), shared by
 * the scene and graphics code. vecmath.cpp defines VECMATH_IMPLEMENTATION and
 * provides the out-of-line copies.
 *
 * The header does not include <iostream.h>: the stream operators are defined
 * only when the includer has included it first.
 */

#pragma interface

#ifndef BN_VEC_H
#define BN_VEC_H

#include <math.h>
#include "vec_types.h"
#include "gmtrig.h"

vec4 apply(const mat4 &m, const vec4 &v, float w);
mat4 multiply(const mat4 &a, const mat4 &b);

#ifndef VECMATH_IMPLEMENTATION
#ifdef _IOSTREAM_H
inline ostream &operator<<(ostream &os, const vec2 &v)
{
	return os << v.x << " " << v.y;
}

inline ostream &operator<<(ostream &os, const vec3 &v)
{
	return os << v.x << " " << v.y << " " << v.z;
}

inline ostream &operator<<(ostream &os, const vec4 &v)
{
	return os << v.x << " " << v.y << " " << v.z << " " << v.w;
}

inline ostream &operator<<(ostream &os, const mat4 &m)
{
	return os << m.m0 << "\n" << m.m1 << "\n" << m.m2 << "\n" << m.m3;
}
#endif /* _IOSTREAM_H */

inline vec4 operator+(const vec4 &a, const vec4 &b)
{
	return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

inline vec4 operator-(const vec4 &a, const vec4 &b)
{
	return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

inline vec4 operator-(const vec4 &v)
{
	return vec4(-v.x, -v.y, -v.z, -v.w);
}

inline vec4 operator*(const vec4 &a, const vec4 &b)
{
	return vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

inline vec4 operator*(const vec4 &v, float s)
{
	return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

inline vec4 operator*(float s, const vec4 &v)
{
	return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

inline float dot(const vec4 &a, const vec4 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec4 cross(const vec4 &a, const vec4 &b)
{
	return vec4(a.y * b.z - b.y * a.z, a.z * b.x - b.z * a.x, a.x * b.y - b.x * a.y, a.w);
}

inline float length(const vec4 &v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline vec4 operator/(const vec4 &v, float s)
{
	if (s == 0.0f)
		return v;
	s = 1.0 / s;
	return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

inline vec4 &operator+=(vec4 &a, const vec4 &b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	a.w += b.w;
	return a;
}

inline vec4 &operator-=(vec4 &a, const vec4 &b)
{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	a.w -= b.w;
	return a;
}

inline vec4 &operator*=(vec4 &a, const vec4 &b)
{
	a.x *= b.x;
	a.y *= b.y;
	a.z *= b.z;
	a.w *= b.w;
	return a;
}

inline vec4 &operator*=(vec4 &a, float s)
{
	a.x *= s;
	a.y *= s;
	a.z *= s;
	a.w *= s;
	return a;
}

inline vec4 &operator/=(vec4 &a, float s)
{
	if (s == 0.0f)
		return a;
	s = 1.0 / s;
	a.x *= s;
	a.y *= s;
	a.z *= s;
	a.w *= s;
	return a;
}

inline vec4 normalize(const vec4 &v)
{
	float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	if (len == 0.0f)
		return v;
	len = 1.0 / len;
	return vec4(v.x * len, v.y * len, v.z * len, v.w);
}

inline vec4 interp(const vec4 &a, const vec4 &b, float t)
{
	return vec4((b.x - a.x) * t + a.x, (b.y - a.y) * t + a.y, (b.z - a.z) * t + a.z, a.w);
}

inline vec4 clamp(const vec4 &v, float lo, float hi)
{
	return vec4(v.x < lo ? lo : (hi < v.x ? hi : v.x), v.y < lo ? lo : (hi < v.y ? hi : v.y),
	    v.z < lo ? lo : (hi < v.z ? hi : v.z), v.w);
}

inline vec4 min(const vec4 &v, float s)
{
	return vec4(s < v.x ? s : v.x, s < v.y ? s : v.y, s < v.z ? s : v.z, v.w);
}

inline vec4 max(const vec4 &v, float s)
{
	return vec4(v.x < s ? s : v.x, v.y < s ? s : v.y, v.z < s ? s : v.z, v.w);
}

inline float dot4(const vec4 &a, const vec4 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline float length4(const vec4 &v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

inline vec4 normalize4(const vec4 &v)
{
	float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
	if (len == 0.0f)
		return v;
	len = 1.0 / len;
	return vec4(v.x * len, v.y * len, v.z * len, v.w * len);
}

inline vec4 interp4(const vec4 &a, const vec4 &b, float t)
{
	return vec4((b.x - a.x) * t + a.x, (b.y - a.y) * t + a.y, (b.z - a.z) * t + a.z, (b.w - a.w) * t + a.w);
}

inline vec4 clamp4(const vec4 &v, float lo, float hi)
{
	return vec4(v.x < lo ? lo : (hi < v.x ? hi : v.x), v.y < lo ? lo : (hi < v.y ? hi : v.y),
	    v.z < lo ? lo : (hi < v.z ? hi : v.z), v.w < lo ? lo : (hi < v.w ? hi : v.w));
}

inline vec4 min4(const vec4 &v, float s)
{
	return vec4(s < v.x ? s : v.x, s < v.y ? s : v.y, s < v.z ? s : v.z, s < v.w ? s : v.w);
}

inline vec4 max4(const vec4 &v, float s)
{
	return vec4(v.x < s ? s : v.x, v.y < s ? s : v.y, v.z < s ? s : v.z, v.w < s ? s : v.w);
}

inline vec4 apply4(const mat4 &m, const vec4 &v)
{
	return apply(m, v, v.w);
}

inline vec4 operator*(const mat4 &m, const vec4 &v)
{
	return apply(m, v, 1.0f);
}

inline mat4 operator*(const mat4 &a, const mat4 &b)
{
	return multiply(a, b);
}

/* Inverse of a translate-rotate-scale matrix: transposed rotation scaled by
   1/|row|^2, translation = -t projected on the rows. */
inline mat4 inverse_trs(const mat4 &m)
{
	float ix = 1.0f / dot(m.m0, m.m0);
	float iy = 1.0f / dot(m.m1, m.m1);
	float iz = 1.0f / dot(m.m2, m.m2);
	float tx = -m.m3.x;
	float ty = -m.m3.y;
	float tz = -m.m3.z;
	return mat4(m.m0.x * ix, m.m1.x * iy, m.m2.x * iz, 0.0f, m.m0.y * ix, m.m1.y * iy, m.m2.y * iz, 0.0f,
	    m.m0.z * ix, m.m1.z * iy, m.m2.z * iz, 0.0f, (m.m0.x * tx + m.m0.y * ty + m.m0.z * tz) * ix,
	    (m.m1.x * tx + m.m1.y * ty + m.m1.z * tz) * iy, (m.m2.x * tx + m.m2.y * ty + m.m2.z * tz) * iz, 1.0f);
}

/* Inverse of a translate-rotate matrix (orthonormal rotation): transposed
   rotation and projected translation, no rescaling. */
inline mat4 inverse_tr(const mat4 &m)
{
	float tx = -m.m3.x;
	float ty = -m.m3.y;
	float tz = -m.m3.z;
	return mat4(m.m0.x, m.m1.x, m.m2.x, 0.0f, m.m0.y, m.m1.y, m.m2.y, 0.0f, m.m0.z, m.m1.z, m.m2.z, 0.0f,
	    m.m0.x * tx + m.m0.y * ty + m.m0.z * tz, m.m1.x * tx + m.m1.y * ty + m.m1.z * tz,
	    m.m2.x * tx + m.m2.y * ty + m.m2.z * tz, 1.0f);
}

#endif /* VECMATH_IMPLEMENTATION */


#ifndef VECMATH_IMPLEMENTATION
inline mat4 mat4_rotate_x(float angle)
{
	int i = (int)(angle * 32768.0f / 3.14159274f);
	float c = gmGetTableCos(i);
	float s = gmGetTableSin(i);
	return mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, s, 0.0f, 0.0f, -s, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

inline mat4 mat4_rotate_y(float angle)
{
	int i = (int)(angle * 32768.0f / 3.14159274f);
	float c = gmGetTableCos(i);
	float s = gmGetTableSin(i);
	return mat4(c, 0.0f, -s, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, s, 0.0f, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

inline mat4 mat4_rotate_z(float angle)
{
	int i = (int)(angle * 32768.0f / 3.14159274f);
	float c = gmGetTableCos(i);
	float s = gmGetTableSin(i);
	return mat4(c, s, 0.0f, 0.0f, -s, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

#endif /* VECMATH_IMPLEMENTATION */

#endif /* BN_VEC_H */
