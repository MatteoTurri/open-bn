/*
 * AnimQuat: rotation animation of an Object between two quaternions.
 */

#include <iostream.h>
#include "gfx/vec.h"
#include "animation.h"
#include <vector>
#include "object.h"
#include <math.h>
#include "animquat.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

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

AnimQuat::~AnimQuat()
{}

AnimQuat::AnimQuat(const vec4 &start, const vec4 &target, float durationMs, float delayMs, int mode)
        : Animation(durationMs, delayMs, mode)
{
	m_start = start;
	m_target = target;
}

void AnimQuat::ResetTarget(const vec4 &target)
{
	m_target = target;
}

void AnimQuat::Do(Object *obj)
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
