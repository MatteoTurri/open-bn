/*
 * The out-of-line vec4/mat4 functions of vecmath.cpp: arithmetic (also inline in
 * vec.h), projections, inverses, transform matrices and quaternions.
 */

#ifndef VECMATH_H
#define VECMATH_H

#include "vec_types.h"

vec4 operator+(const vec4 &a, const vec4 &b);
vec4 operator-(const vec4 &a, const vec4 &b);
vec4 operator-(const vec4 &v);
vec4 operator*(const vec4 &a, const vec4 &b);
vec4 operator*(const vec4 &v, float s);
vec4 operator*(float s, const vec4 &v);
vec4 operator/(const vec4 &v, float s);
vec4 &operator+=(vec4 &a, const vec4 &b);
vec4 &operator-=(vec4 &a, const vec4 &b);
vec4 &operator*=(vec4 &a, const vec4 &b);
vec4 &operator*=(vec4 &a, float s);
vec4 &operator/=(vec4 &a, float s);
float dot(const vec4 &a, const vec4 &b);
vec4 cross(const vec4 &a, const vec4 &b);
float length(const vec4 &v);
vec4 normalize(const vec4 &v);
vec4 interp(const vec4 &a, const vec4 &b, float t);
vec4 clamp(const vec4 &v, float lo, float hi);
vec4 min(const vec4 &v, float s);
vec4 max(const vec4 &v, float s);
float dot4(const vec4 &a, const vec4 &b);
float length4(const vec4 &v);
vec4 normalize4(const vec4 &v);
vec4 interp4(const vec4 &a, const vec4 &b, float t);
vec4 clamp4(const vec4 &v, float lo, float hi);
vec4 min4(const vec4 &v, float s);
vec4 max4(const vec4 &v, float s);
mat4 operator+(const mat4 &a, const mat4 &b);
mat4 operator-(const mat4 &a, const mat4 &b);
mat4 operator-(const mat4 &m);
mat4 operator*(const mat4 &m, float s);
mat4 operator*(float s, const mat4 &m);
mat4 operator/(const mat4 &m, float s);
mat4 operator*(const mat4 &a, const mat4 &b);
vec4 operator*(const mat4 &m, const vec4 &v);
mat4 &operator+=(mat4 &a, const mat4 &b);
mat4 &operator-=(mat4 &a, const mat4 &b);
mat4 &operator*=(mat4 &a, const mat4 &b);
mat4 &operator*=(mat4 &a, float s);
mat4 &operator/=(mat4 &a, float s);
vec4 apply4(const mat4 &m, const vec4 &v);
vec4 project(const mat4 &m, const vec4 &v, float w);
vec4 project4(const mat4 &m, const vec4 &v);
mat4 normalize(const mat4 &m);
mat4 inverse(const mat4 &m);
mat4 inverse_trs(const mat4 &m);
mat4 inverse_tr(const mat4 &m);
mat4 transpose(const mat4 &m);
const mat4 &mat4_identity();
mat4 mat4_translate(const vec4 &v);
mat4 mat4_translate(float x, float y, float z);
mat4 mat4_rotate(float angle, float x, float y, float z);
mat4 mat4_rotate_x(float angle);
mat4 mat4_rotate_y(float angle);
mat4 mat4_rotate_z(float angle);
mat4 mat4_rotate(float angle, const vec4 &axis);
mat4 mat4_rotate_xyz(const vec4 &v);
mat4 mat4_rotate_yxz(const vec4 &v);
mat4 mat4_rotate_zyx(const vec4 &v);
mat4 mat4_rotate_xyz(float x, float y, float z);
mat4 mat4_rotate_yxz(float x, float y, float z);
mat4 mat4_rotate_zyx(float x, float y, float z);
mat4 mat4_scale(const vec4 &v);
mat4 mat4_scale(float x, float y, float z);
mat4 mat4_viewport(float x, float y, float w, float h, float n, float f);
mat4 mat4_perspective(float fov, float aspect, float n, float f);
mat4 mat4_perspective_tan(float t, float aspect, float n, float f);
mat4 mat4_frustum(float l, float r, float b, float t, float n, float f);
mat4 mat4_ortho(float l, float r, float b, float t, float n, float f);
mat4 mat4_look_at(const vec4 &eye, const vec4 &center, const vec4 &up);
mat4 mat4_look_at(float ex, float ey, float ez, float cx, float cy, float cz, float ux, float uy, float uz);
mat4 mat4_quat(const vec4 &q);
mat4 mat4_quat(float x, float y, float z, float w);
vec4 quat_slerp(const vec4 &a, const vec4 &b, float t);
vec4 quat_from_mat4(const mat4 &m);
vec4 quat_from_rotzyx(const vec4 &v);
vec4 quat_from_rotyxz(const vec4 &v);
vec4 quat_from_rotate(float angle, const vec4 &axis);
vec4 quat_to_rotzyx(const vec4 &q);
vec4 quat_to_rotyxz(const vec4 &q);
vec4 quat_normalize(const vec4 &q);
vec4 quat_multiply(const vec4 &a, const vec4 &b);

#endif
