/*
 * AnimRotate: rotation animation of an Object given as Euler angles.
 */

#include <iostream.h>
#include "gfx/vec.h"
#include "animation.h"
#include <vector>
#include "object.h"
#include <math.h>
#include "animrotate.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

/* Local copies of the quat_from_rot* functions of vecmath.cpp. */
static inline vec4 l_quat_from_rotzyx(const vec4 &v)
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

static inline vec4 l_quat_from_rotyxz(const vec4 &v)
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

AnimRotate::AnimRotate(const vec4 &a, const vec4 &b, float durationMs, float delayMs, int mode, int order)
        : Animation(durationMs, delayMs, mode)
{
	m_order = order;
	switch (order) {
	case 1:
		m_start = normalize4(l_quat_from_rotzyx(a));
		m_target = normalize4(l_quat_from_rotzyx(b));
		break;
	case 0:
	default:
		m_start = normalize4(l_quat_from_rotyxz(a));
		m_target = normalize4(l_quat_from_rotyxz(b));
		break;
	}
}

inline AnimRotate::~AnimRotate()
{}

/* Local copy of quat_slerp() from vecmath.cpp. */
static inline vec4 l_quat_slerp(const vec4 &a, const vec4 &b, float t)
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

void AnimRotate::Do(Object *obj)
{
	if (!obj)
		return;
	float t = DoTimer(m_mode, 0.0f, 0.0f, 1.0f);
	if (1.0f <= t) {
		obj->SetQuat(m_target, 0);
		return;
	}
	vec4 q = normalize4(l_quat_slerp(m_start, m_target, t));
	obj->SetQuat(q, 0);
}

void AnimRotate::ResetTarget(const vec4 &v)
{
	switch (m_order) {
	case 1:
		m_target = normalize4(l_quat_from_rotzyx(v));
		break;
	case 0:
	default:
		m_target = normalize4(l_quat_from_rotyxz(v));
		break;
	}
}
