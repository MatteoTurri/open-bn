/*
 * Out-of-line vec4/mat4 functions: arithmetic, inverses, transforms,
 * projections and quaternions.
 */

#include <iostream.h>
#define VECMATH_IMPLEMENTATION
#include "vec.h"
#include "vecmath.h"

ostream &operator<<(ostream &os, const vec2 &v)
{
	return os << v.x << " " << v.y;
}

ostream &operator<<(ostream &os, const vec3 &v)
{
	return os << v.x << " " << v.y << " " << v.z;
}

ostream &operator<<(ostream &os, const vec4 &v)
{
	return os << v.x << " " << v.y << " " << v.z << " " << v.w;
}

ostream &operator<<(ostream &os, const mat4 &m)
{
	return os << m.m0 << "\n" << m.m1 << "\n" << m.m2 << "\n" << m.m3;
}

vec4 operator+(const vec4 &a, const vec4 &b)
{
	return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

vec4 operator-(const vec4 &a, const vec4 &b)
{
	return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

vec4 operator-(const vec4 &v)
{
	return vec4(-v.x, -v.y, -v.z, -v.w);
}

vec4 operator*(const vec4 &a, const vec4 &b)
{
	return vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

vec4 operator*(const vec4 &v, float s)
{
	return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

vec4 operator*(float s, const vec4 &v)
{
	return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

vec4 operator/(const vec4 &v, float s)
{
	if (s == 0.0f)
		return v;
	s = 1.0 / s;
	return vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

vec4 &operator+=(vec4 &a, const vec4 &b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	a.w += b.w;
	return a;
}

vec4 &operator-=(vec4 &a, const vec4 &b)
{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	a.w -= b.w;
	return a;
}

vec4 &operator*=(vec4 &a, const vec4 &b)
{
	a.x *= b.x;
	a.y *= b.y;
	a.z *= b.z;
	a.w *= b.w;
	return a;
}

vec4 &operator*=(vec4 &a, float s)
{
	a.x *= s;
	a.y *= s;
	a.z *= s;
	a.w *= s;
	return a;
}

vec4 &operator/=(vec4 &a, float s)
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

float dot(const vec4 &a, const vec4 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec4 cross(const vec4 &a, const vec4 &b)
{
	return vec4(a.y * b.z - b.y * a.z, a.z * b.x - b.z * a.x, a.x * b.y - b.x * a.y, a.w);
}

float length(const vec4 &v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec4 normalize(const vec4 &v)
{
	float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	if (len == 0.0f)
		return v;
	len = 1.0 / len;
	return vec4(v.x * len, v.y * len, v.z * len, v.w);
}

vec4 interp(const vec4 &a, const vec4 &b, float t)
{
	return vec4((b.x - a.x) * t + a.x, (b.y - a.y) * t + a.y, (b.z - a.z) * t + a.z, a.w);
}

vec4 clamp(const vec4 &v, float lo, float hi)
{
	return vec4(v.x < lo ? lo : (hi < v.x ? hi : v.x), v.y < lo ? lo : (hi < v.y ? hi : v.y),
	    v.z < lo ? lo : (hi < v.z ? hi : v.z), v.w);
}

vec4 min(const vec4 &v, float s)
{
	return vec4(s < v.x ? s : v.x, s < v.y ? s : v.y, s < v.z ? s : v.z, v.w);
}

vec4 max(const vec4 &v, float s)
{
	return vec4(v.x < s ? s : v.x, v.y < s ? s : v.y, v.z < s ? s : v.z, v.w);
}

float dot4(const vec4 &a, const vec4 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float length4(const vec4 &v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

vec4 normalize4(const vec4 &v)
{
	float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
	if (len == 0.0f)
		return v;
	len = 1.0 / len;
	return vec4(v.x * len, v.y * len, v.z * len, v.w * len);
}

vec4 interp4(const vec4 &a, const vec4 &b, float t)
{
	return vec4((b.x - a.x) * t + a.x, (b.y - a.y) * t + a.y, (b.z - a.z) * t + a.z, (b.w - a.w) * t + a.w);
}

vec4 clamp4(const vec4 &v, float lo, float hi)
{
	return vec4(v.x < lo ? lo : (hi < v.x ? hi : v.x), v.y < lo ? lo : (hi < v.y ? hi : v.y),
	    v.z < lo ? lo : (hi < v.z ? hi : v.z), v.w < lo ? lo : (hi < v.w ? hi : v.w));
}

vec4 min4(const vec4 &v, float s)
{
	return vec4(s < v.x ? s : v.x, s < v.y ? s : v.y, s < v.z ? s : v.z, s < v.w ? s : v.w);
}

vec4 max4(const vec4 &v, float s)
{
	return vec4(v.x < s ? s : v.x, v.y < s ? s : v.y, v.z < s ? s : v.z, v.w < s ? s : v.w);
}

mat4 operator+(const mat4 &a, const mat4 &b)
{
	return mat4(a.m0 + b.m0, a.m1 + b.m1, a.m2 + b.m2, a.m3 + b.m3);
}

mat4 operator-(const mat4 &a, const mat4 &b)
{
	return mat4(a.m0 - b.m0, a.m1 - b.m1, a.m2 - b.m2, a.m3 - b.m3);
}

mat4 operator-(const mat4 &m)
{
	return mat4(-m.m0, -m.m1, -m.m2, -m.m3);
}

mat4 operator*(const mat4 &m, float s)
{
	return mat4(m.m0 * s, m.m1 * s, m.m2 * s, m.m3 * s);
}

mat4 operator*(float s, const mat4 &m)
{
	return mat4(m.m0 * s, m.m1 * s, m.m2 * s, m.m3 * s);
}

mat4 operator/(const mat4 &m, float s)
{
	if (s == 0.0f)
		return m;
	s = 1.0 / s;
	return mat4(m.m0 * s, m.m1 * s, m.m2 * s, m.m3 * s);
}

mat4 operator*(const mat4 &a, const mat4 &b)
{
	return multiply(a, b);
}

vec4 operator*(const mat4 &m, const vec4 &v)
{
	return apply(m, v, 1.0f);
}

mat4 &operator+=(mat4 &a, const mat4 &b)
{
	a.m0 += b.m0;
	a.m1 += b.m1;
	a.m2 += b.m2;
	a.m3 += b.m3;
	return a;
}

mat4 &operator-=(mat4 &a, const mat4 &b)
{
	a.m0 -= b.m0;
	a.m1 -= b.m1;
	a.m2 -= b.m2;
	a.m3 -= b.m3;
	return a;
}

mat4 &operator*=(mat4 &a, const mat4 &b)
{
	a = multiply(a, b);
	return a;
}

mat4 &operator*=(mat4 &a, float s)
{
	a.m0 *= s;
	a.m1 *= s;
	a.m2 *= s;
	a.m3 *= s;
	return a;
}

mat4 &operator/=(mat4 &a, float s)
{
	if (s == 0.0f)
		return a;
	s = 1.0 / s;
	a.m0 *= s;
	a.m1 *= s;
	a.m2 *= s;
	a.m3 *= s;
	return a;
}

vec4 apply4(const mat4 &m, const vec4 &v)
{
	return apply(m, v, v.w);
}

vec4 project(const mat4 &m, const vec4 &v, float w)
{
	vec4 r = apply(m, v, w);
	float rw = r.w;
	if (rw != 0.0f) {
		rw = 1.0f / rw;
		return vec4(r.x * rw, r.y * rw, r.z * rw, rw);
	}
	return r;
}

vec4 project4(const mat4 &m, const vec4 &v)
{
	vec4 r = apply(m, v, v.w);
	float rw = r.w;
	if (rw != 0.0f) {
		rw = 1.0f / rw;
		return vec4(r.x * rw, r.y * rw, r.z * rw, rw);
	}
	return r;
}

vec4 apply(const mat4 &m, const vec4 &v, float w)
{
	return m.m0 * v.x + m.m1 * v.y + m.m2 * v.z + m.m3 * w;
}

mat4 multiply(const mat4 &a, const mat4 &b)
{
	return mat4(apply4(a, b.m0), apply4(a, b.m1), apply4(a, b.m2), apply4(a, b.m3));
}

static mat4 identity_matrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 identity_row3(0.0f, 0.0f, 0.0f, 1.0f);

mat4 normalize(const mat4 &m)
{
	return mat4(normalize(m.m0), normalize(m.m1), normalize(m.m2), identity_row3);
}

mat4 inverse(const mat4 &m)
{
	float a[4][4], b[4][4];
	int i, j, k;
	float piv = 0.0f;
	const float *src = (const float *)&m;
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++) {
			a[i][j] = *(src + j + i * 4);
			b[i][j] = (i == j) ? 1.0f : 0.0f;
		}
	for (i = 0; i < 4; i++) {
		for (j = i; j < 4; j++) {
			piv = a[j][i];
			if (piv != 0.0)
				break;
		}
		if (j == 4)
			return mat4_identity();
		if (j != i)
			for (k = 0; k < 4; k++) {
				float t;
				t = a[i][k];
				a[i][k] = a[j][k];
				a[j][k] = t;
				t = b[i][k];
				b[i][k] = b[j][k];
				b[j][k] = t;
			}
		for (j = 0; j < 4; j++) {
			if (j == i)
				a[i][j] = 1.0f;
			if (i < j)
				a[i][j] /= piv;
			b[i][j] /= piv;
		}
		for (j = 0; j < 4; j++)
			if (j != i) {
				piv = a[j][i];
				for (k = 0; k < 4; k++) {
					if (k == i)
						a[j][i] = 0.0f;
					if (i < k)
						a[j][k] -= a[i][k] * piv;
					b[j][k] -= b[i][k] * piv;
				}
			}
	}
	float c[4][4];
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++) {
			float *dst = (float *)c;
			*(dst + j + i * 4) = b[i][j];
		}
	return *(mat4 *)c;
}

mat4 inverse_trs(const mat4 &m)
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

mat4 inverse_tr(const mat4 &m)
{
	float tx = -m.m3.x;
	float ty = -m.m3.y;
	float tz = -m.m3.z;
	return mat4(m.m0.x, m.m1.x, m.m2.x, 0.0f, m.m0.y, m.m1.y, m.m2.y, 0.0f, m.m0.z, m.m1.z, m.m2.z, 0.0f,
	    m.m0.x * tx + m.m0.y * ty + m.m0.z * tz, m.m1.x * tx + m.m1.y * ty + m.m1.z * tz,
	    m.m2.x * tx + m.m2.y * ty + m.m2.z * tz, 1.0f);
}

mat4 transpose(const mat4 &m)
{
	return mat4(m.m0.x, m.m1.x, m.m2.x, m.m3.x, m.m0.y, m.m1.y, m.m2.y, m.m3.y, m.m0.z, m.m1.z, m.m2.z, m.m3.z,
	    m.m0.w, m.m1.w, m.m2.w, m.m3.w);
}

const mat4 &mat4_identity()
{
	return identity_matrix;
}

mat4 mat4_translate(const vec4 &v)
{
	return mat4_translate(v.x, v.y, v.z);
}

mat4 mat4_translate(float x, float y, float z)
{
	return mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, x, y, z, 1.0f);
}

mat4 mat4_rotate(float angle, float x, float y, float z)
{
	return mat4_rotate(angle, vec4(x, y, z, 1.0f));
}

/* out-of-line copies of the mat4_rotate_x/y/z inlines of vec.h */
mat4 mat4_rotate_x(float angle)
{
	int i = (int)(angle * 32768.0f / 3.14159274f);
	float c = gmGetTableCos(i);
	float s = gmGetTableSin(i);
	return mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, s, 0.0f, 0.0f, -s, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

mat4 mat4_rotate_y(float angle)
{
	int i = (int)(angle * 32768.0f / 3.14159274f);
	float c = gmGetTableCos(i);
	float s = gmGetTableSin(i);
	return mat4(c, 0.0f, -s, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, s, 0.0f, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

mat4 mat4_rotate_z(float angle)
{
	int i = (int)(angle * 32768.0f / 3.14159274f);
	float c = gmGetTableCos(i);
	float s = gmGetTableSin(i);
	return mat4(c, s, 0.0f, 0.0f, -s, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

mat4 mat4_rotate(float angle, const vec4 &axis)
{
	float half = angle * 0.5f;
	int i;
	vec4 q = normalize(axis) * gmGetTableSin(i = (int)(half * 32768.0f / 3.14159274f));
	float x = q.x, y = q.y, z = q.z, w = gmGetTableCos(i);
	float x2 = x + x;
	float w2 = w + w;
	float z2 = z + z;
	float y2 = y + y;
	return mat4(1.0f - y2 * y - z2 * z, x2 * y + w2 * z, z2 * x - w2 * y, 0.0f, x2 * y - w2 * z,
	    1.0f - x2 * x - z2 * z, y2 * z + w2 * x, 0.0f, z2 * x + w2 * y, y2 * z - w2 * x, 1.0f - x2 * x - y2 * y,
	    0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

mat4 mat4_rotate_xyz(const vec4 &v)
{
	return mat4_rotate_x(v.x) * mat4_rotate_y(v.y) * mat4_rotate_z(v.z);
}

mat4 mat4_rotate_yxz(const vec4 &v)
{
	return mat4_rotate_y(v.y) * mat4_rotate_x(v.x) * mat4_rotate_z(v.z);
}

mat4 mat4_rotate_zyx(const vec4 &v)
{
	return mat4_rotate_z(v.z) * mat4_rotate_y(v.y) * mat4_rotate_x(v.x);
}

mat4 mat4_rotate_xyz(float x, float y, float z)
{
	return mat4_rotate_x(x) * mat4_rotate_y(y) * mat4_rotate_z(z);
}

mat4 mat4_rotate_yxz(float x, float y, float z)
{
	return mat4_rotate_y(y) * mat4_rotate_x(x) * mat4_rotate_z(z);
}

mat4 mat4_rotate_zyx(float x, float y, float z)
{
	return mat4_rotate_z(z) * mat4_rotate_y(y) * mat4_rotate_x(x);
}

mat4 mat4_scale(const vec4 &v)
{
	return mat4_scale(v.x, v.y, v.z);
}

mat4 mat4_scale(float x, float y, float z)
{
	return mat4(x, 0.0f, 0.0f, 0.0f, 0.0f, y, 0.0f, 0.0f, 0.0f, 0.0f, z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

mat4 mat4_viewport(float x, float y, float w, float h, float n, float f)
{
	h = h * -0.5f;
	float hw = w * 0.5f;
	return mat4(hw, 0.0f, 0.0f, 0.0f, 0.0f, h, 0.0f, 0.0f, 0.0f, 0.0f, (f - n) * 0.5f, 0.0f, x + hw, y - h,
	    (f + n) * 0.5f, 1.0f);
}

mat4 mat4_perspective(float fov, float aspect, float n, float f)
{
	float cot = 1.0f / tanf(fov * 0.5f);
	return mat4(cot / aspect, 0.0f, 0.0f, 0.0f, 0.0f, cot, 0.0f, 0.0f, 0.0f, 0.0f, (f + n) / (n - f), -1.0f, 0.0f,
	    0.0f, 2.0f * f * n / (n - f), 0.0f);
}

mat4 mat4_perspective_tan(float t, float aspect, float n, float f)
{
	float cot = 1.0f / t;
	return mat4(cot / aspect, 0.0f, 0.0f, 0.0f, 0.0f, cot, 0.0f, 0.0f, 0.0f, 0.0f, (f + n) / (n - f), -1.0f, 0.0f,
	    0.0f, 2.0f * f * n / (n - f), 0.0f);
}

mat4 mat4_frustum(float l, float r, float b, float t, float n, float f)
{
	float ix = 1.0f / (r - l);
	float iy = 1.0f / (t - b);
	float n2 = 2.0f * n;
	float iz = 1.0f / (f - n);
	return mat4(n2 * ix, 0.0f, 0.0f, 0.0f, 0.0f, n2 * iy, 0.0f, 0.0f, (r + l) * ix, (t + b) * iy, -(f + n) * iz,
	    -1.0f, 0.0f, 0.0f, -2.0f * f * n * iz, 0.0f);
}

mat4 mat4_ortho(float l, float r, float b, float t, float n, float f)
{
	float ix = 1.0f / (r - l);
	float iy = 1.0f / (t - b);
	float iz = 1.0f / (f - n);
	return mat4(2.0f * ix, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f * iy, 0.0f, 0.0f, 0.0f, 0.0f, -2.0f * iz, 0.0f,
	    -(r + l) * ix, -(t + b) * iy, -(f + n) * iz, 1.0f);
}

mat4 mat4_look_at(const vec4 &eye, const vec4 &center, const vec4 &up)
{
	vec4 f = normalize(center - eye);
	vec4 u = normalize(up);
	vec4 s = normalize(cross(f, u));
	u = cross(s, f);
	return mat4(s.x, u.x, -f.x, 0.0f, s.y, u.y, -f.y, 0.0f, s.z, u.z, -f.z, 0.0f, -dot(eye, s), -dot(eye, u),
	    dot(eye, f), 1.0f);
}

mat4 mat4_look_at(float ex, float ey, float ez, float cx, float cy, float cz, float ux, float uy, float uz)
{
	return mat4_look_at(vec4(ex, ey, ez, 1.0f), vec4(cx, cy, cz, 1.0f), vec4(ux, uy, uz, 1.0f));
}

mat4 mat4_quat(const vec4 &q)
{
	return mat4_quat(q.x, q.y, q.z, q.w);
}

mat4 mat4_quat(float x, float y, float z, float w)
{
	return mat4(1.0f - 2.0f * y * y - 2.0f * z * z, 2.0f * x * y + 2.0f * w * z, 2.0f * z * x - 2.0f * w * y, 0.0f,
	    2.0f * x * y - 2.0f * w * z, 1.0f - 2.0f * x * x - 2.0f * z * z, 2.0f * y * z + 2.0f * w * x, 0.0f,
	    2.0f * z * x + 2.0f * w * y, 2.0f * y * z - 2.0f * w * x, 1.0f - 2.0f * x * x - 2.0f * y * y, 0.0f, 0.0f,
	    0.0f, 0.0f, 1.0f);
}

vec4 quat_slerp(const vec4 &a, const vec4 &b, float t)
{
	float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	int flip = 0;
	if (dot < 0.0f) {
		dot = -dot;
		flip = 1;
	}
	float angle = acosf(dot);
	float sn = gmGetTableSin((int)(angle * 32768.0f / 3.14159274f));
	if (sn == 0.0f)
		return b;
	float ra = gmGetTableSin((int)((1.0f - t) * angle * 32768.0f / 3.14159274f)) / sn;
	float rb = gmGetTableSin((int)(t * angle * 32768.0f / 3.14159274f)) / sn;
	if (flip)
		rb = -rb;
	return vec4(a.x * ra + b.x * rb, a.y * ra + b.y * rb, a.z * ra + b.z * rb, a.w * ra + b.w * rb);
}

vec4 quat_from_mat4(const mat4 &m) return q;
{
	float m00 = m.m0.x;
	float m11 = m.m1.y;
	float m22 = m.m2.z;
	float trace = m00 + m11 + m22;
	if (trace + 1.0f > 0.01f) {
		float s = sqrtf(trace + 1.0f);
		float h = s * 0.5f;
		float d = 0.25f / h;
		float t1 = m.m1.z - m.m2.y;
		float t2 = m.m2.x - m.m0.z;
		float t3 = m.m0.y - m.m1.x;
		q.w = h;
		q.x = t1 * d;
		q.y = t2 * d;
		q.z = t3 * d;
	} else if (m00 > m11) {
		if (!(m00 > m22))
			goto lastcase;
		float s = sqrtf(m00 + 1.0f - m11 - m22);
		float h = s * 0.5f;
		float d = 0.25f / h;
		float t1 = m.m1.x + m.m0.y;
		float t2 = m.m2.x + m.m0.z;
		float t3 = m.m1.z - m.m2.y;
		q.x = h;
		q.y = t1 * d;
		q.z = t2 * d;
		q.w = t3 * d;
	} else if (m11 > m22) {
		float s = sqrtf(m11 + 1.0f - m22 - m00);
		float h = s * 0.5f;
		float d = 0.25f / h;
		float t1 = m.m1.x + m.m0.y;
		float t2 = m.m2.y + m.m1.z;
		float t3 = m.m2.x - m.m0.z;
		q.y = h;
		q.x = t1 * d;
		q.z = t2 * d;
		q.w = t3 * d;
	} else {
lastcase:
		float s = sqrtf(m.m2.z + 1.0f - m.m0.x - m.m1.y);
		float h = s * 0.5f;
		float d = 0.25f / h;
		float t1 = m.m2.x + m.m0.z;
		float t2 = m.m2.y + m.m1.z;
		float t3 = m.m0.y - m.m1.x;
		q.z = h;
		q.x = t1 * d;
		q.y = t2 * d;
		q.w = t3 * d;
	}
}

vec4 quat_from_rotzyx(const vec4 &v)
{
	float hx = v.x * 0.5f;
	float hy = v.y * 0.5f;
	float hz = v.z * 0.5f;
	int ix = (int)(hx * 32768.0f / 3.14159274f);
	float cx = gmGetTableCos(ix);
	float sx = gmGetTableSin(ix);
	int iy = (int)(hy * 32768.0f / 3.14159274f);
	float cy = gmGetTableCos(iy);
	float sy = gmGetTableSin(iy);
	int iz = (int)(hz * 32768.0f / 3.14159274f);
	float cz = gmGetTableCos(iz);
	float sz = gmGetTableSin(iz);
	return vec4(cz * cy * sx - cx * sz * sy, cx * cz * sy + cy * sz * sx, cx * cy * sz - cz * sy * sx,
	    cz * cy * cx + sz * sy * sx);
}

vec4 quat_from_rotyxz(const vec4 &v)
{
	float hx = v.x * 0.5f;
	float hy = v.y * 0.5f;
	float hz = v.z * 0.5f;
	int ix = (int)(hx * 32768.0f / 3.14159274f);
	float cx = gmGetTableCos(ix);
	float sx = gmGetTableSin(ix);
	int iy = (int)(hy * 32768.0f / 3.14159274f);
	float cy = gmGetTableCos(iy);
	float sy = gmGetTableSin(iy);
	int iz = (int)(hz * 32768.0f / 3.14159274f);
	float cz = gmGetTableCos(iz);
	float sz = gmGetTableSin(iz);
	return vec4(cy * cz * sx + sy * cx * sz, sy * cx * cz - cy * sx * sz, cy * cx * sz - sy * cz * sx,
	    cy * cx * cz + sy * sx * sz);
}

vec4 quat_from_rotate(float angle, const vec4 &axis)
{
	float half = angle * 0.5f;
	int i;
	vec4 q = normalize(axis) * gmGetTableSin(i = (int)(half * 32768.0f / 3.14159274f));
	q.w = gmGetTableCos(i);
	return q;
}

vec4 quat_to_rotzyx(const vec4 &q)
{
	float qx = q.x, qy = q.y, qz = q.z, qw = q.w;
	float sp = 2.0f * qw * qy - 2.0f * qz * qx;
	float ey, rn, rd, yn, yd;
	if (sp <= 0.99995 && sp >= -0.99995) {
		yn = 2.0f * qx * qy + 2.0f * qw * qz;
		yd = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
		rd = 1.0f - 2.0f * qx * qx - 2.0f * qy * qy;
		rn = 2.0f * qy * qz + 2.0f * qw * qx;
		ey = asinf(sp);
	} else {
		rn = 0.0f;
		yd = 1.0f - 2.0f * qx * qx - 2.0f * qz * qz;
		rd = 1.0f;
		yn = 2.0f * qw * qz - 2.0f * qx * qy;
		ey = atan2f(sp, 1.0f - 2.0f * qx * qx - 2.0f * qy * qy);
	}
	float ex = atan2f(rn, rd);
	return vec4(ex, ey, atan2f(yn, yd), 1.0f);
}

vec4 quat_to_rotyxz(const vec4 &q)
{
	float qx = q.x, qy = q.y, qz = q.z, qw = q.w;
	float sp = 2.0f * qw * qx - 2.0f * qy * qz;
	float ex, zn, zd, yn, yd;
	if (sp <= 0.99995 && sp >= -0.99995) {
		zd = 1.0f - 2.0f * qx * qx - 2.0f * qz * qz;
		zn = 2.0f * qx * qy + 2.0f * qw * qz;
		yn = 2.0f * qz * qx + 2.0f * qw * qy;
		yd = 1.0f - 2.0f * qx * qx - 2.0f * qy * qy;
		ex = asinf(sp);
	} else {
		zn = 0.0f;
		yd = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
		zd = 1.0f;
		yn = 2.0f * qw * qy - 2.0f * qz * qx;
		ex = atan2f(sp, 1.0f - 2.0f * qx * qx - 2.0f * qz * qz);
	}
	return vec4(ex, atan2f(yn, yd), atan2f(zn, zd), 1.0f);
}

vec4 quat_normalize(const vec4 &q)
{
	float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
	if (len == 0.0f)
		return q;
	len = 1.0 / len;
	return vec4(q.x * len, q.y * len, q.z * len, q.w * len);
}

vec4 quat_multiply(const vec4 &a, const vec4 &b)
{
	float ax = a.x, ay = a.y, az = a.z, aw = a.w;
	float bx = b.x, by = b.y, bz = b.z, bw = b.w;
	return vec4(aw * bx + bw * ax + ay * bz - az * by, aw * by + bw * ay + az * bx - ax * bz,
	    aw * bz + bw * az + ax * by - ay * bx, aw * bw - ax * bx - ay * by - az * bz);
}
