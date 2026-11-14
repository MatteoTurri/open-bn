/*
 * vec2/vec3/vec4/mat4: the vector and matrix types alone. gfx/vec.h adds the
 * inline operations; a file that calls the out-of-line copies of vecmath.cpp
 * includes only this header.
 */

#pragma interface

#ifndef BN_VEC_TYPES_H
#define BN_VEC_TYPES_H

/* A quadword aligned to 16 bytes, like a VU0 vector. */
struct vec4 {
	float x, y, z, w;
	vec4() {}
	vec4(float px, float py, float pz, float pw) : x(px), y(py), z(pz), w(pw) {}
	void Set(float px, float py, float pz, float pw)
	{
		x = px;
		y = py;
		z = pz;
		w = pw;
	}
} __attribute__((aligned(16)));

/* A vec4 without constructor and without the 16-byte alignment: the element
   of aggregate-initialized tables and of unions, and the members that their
   owner sets with Set(). Read as a vec4 through a cast. */
struct fvec4 {
	float x, y, z, w;
	void Set(float px, float py, float pz, float pw)
	{
		x = px;
		y = py;
		z = pz;
		w = pw;
	}
};

/* Like vec2/vec3/vec4, the default constructor leaves the matrix uninitialized. */
struct mat4 {
	vec4 m0, m1, m2, m3;
	mat4() {}
	mat4(const vec4 &a, const vec4 &b, const vec4 &c, const vec4 &d)
	{
		m0 = a;
		m1 = b;
		m2 = c;
		m3 = d;
	}
	mat4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20,
	    float m21, float m22, float m23, float m30, float m31, float m32, float m33)
	{
		m0.Set(m00, m01, m02, m03);
		m1.Set(m10, m11, m12, m13);
		m2.Set(m20, m21, m22, m23);
		m3.Set(m30, m31, m32, m33);
	}
};

struct vec2 {
	float x, y;
	vec2() {}
};

struct vec3 {
	float x, y, z;
	vec3() {}
};

#endif /* BN_VEC_TYPES_H */
