/*
 * AnimMoveHermite methods.
 */

#include <vector>
#include "gfx/vec.h"
#include "animmovehermite.h"
#include "object.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */
/* Hermite basis matrix. */
mat4 AnimMoveHermite::hermite_mat(2.0f, -2.0f, 1.0f, 1.0f, -3.0f, 3.0f, -2.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f);

/* m * (v.x, v.y, v.z, w): local copy of apply() from vec.h. */
static inline vec4 h_apply(const mat4 &m, const vec4 &v, float w)
{
	return m.m0 * v.x + m.m1 * v.y + m.m2 * v.z + m.m3 * w;
}

void AnimMoveHermite::ResetHermite(const vec4 &posA, const vec4 &posB, const vec4 &vecA, const vec4 &vecB)
{
	m_posA = posA;
	m_posB = posB;
	m_vecA = vecA;
	m_vecB = vecB;
	m_curPosA = posA;
	m_curPosB = posB;
	m_curVecA = vecA * t_diff_inv;
	m_curVecB = vecB * t_diff_inv;
}

/* Hermite weights H * [t^3, t^2, t, 1] applied to the working control points. */
vec4 AnimMoveHermite::GetPosInterpolate(float t)
{
	float t2 = t * t;
	vec4 T(t2 * t, t2, t, 1.0f);
	vec4 w = h_apply(hermite_mat, T, 1.0f);
	return m_curPosA * w.x + m_curPosB * w.y + m_curVecA * w.z + m_curVecB * w.w;
}

AnimMoveHermite::~AnimMoveHermite()
{}

AnimMoveHermite::AnimMoveHermite(const vec4 &posA, const vec4 &posB, const vec4 &vecA, const vec4 &vecB,
    float durationMs, float delayMs, int mode)
        : Animation(durationMs, delayMs, mode)
{
	ResetHermite(posA, posB, vecA, vecB);
}

void AnimMoveHermite::ResetAll(const vec4 &posA, const vec4 &posB, const vec4 &vecA, const vec4 &vecB, float durationMs,
    float delayMs, int mode)
{
	ResetTimer(durationMs, delayMs, mode);
	ResetHermite(posA, posB, vecA, vecB);
}

void AnimMoveHermite::Do(Object *obj)
{
	if (!obj)
		return;
	float t = DoTimer(m_mode, 1.0f, 1.0f, 1.0f);
	if (1.0f <= t) {
		obj->SetPos(m_posB, 0);
	} else {
		vec4 result = GetPosInterpolate(t);
		obj->SetPos(result, 0);
	}
}

void AnimMoveHermite::ResetTarget(const vec4 &target)
{
	m_posB = target;
	m_curPosB = target;
}
