/*
 * AnimMoveLine methods.
 */

#include <vector>
#include "gfx/vec.h"
#include "animmoveline.h"
#include "object.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

inline AnimMoveLine::~AnimMoveLine()
{}

inline AnimMoveLine::AnimMoveLine(const vec4 &start, const vec4 &target, float durationMs, float delayMs, int mode,
    float startSlope, float endSlope)
        : Animation(durationMs, delayMs, mode)
{
	m_startPos = start;
	m_targetPos = target;
	m_startSlope = startSlope;
	m_endSlope = endSlope;
}

inline void AnimMoveLine::ResetLine(const vec4 &start, const vec4 &target, float startSlope, float endSlope)
{
	m_startPos = start;
	m_targetPos = target;
	m_startSlope = startSlope;
	m_endSlope = endSlope;
}

inline void AnimMoveLine::ResetAll(const vec4 &start, const vec4 &target, float durationMs, float delayMs, int mode,
    float startSlope, float endSlope)
{
	ResetTimer(durationMs, delayMs, mode);
	ResetLine(start, target, m_startSlope, endSlope);  /* NOTE: bug: keeps m_startSlope, the new one is ignored */
}

inline vec4 AnimMoveLine::GetPosInterpolate(float t)
{
	return m_startPos * (1.0f - t) + m_targetPos * t;
}

inline void AnimMoveLine::ResetTarget(const vec4 &target)
{
	m_targetPos = target;
}

void AnimMoveLine::Do(Object *obj)
{
	if (!obj)
		return;
	float t = DoTimer(m_mode, m_startSlope, m_endSlope, 1.0f);
	if (1.0f <= t) {
		obj->SetPos(m_targetPos, 0);
	} else {
		vec4 result = m_startPos * (1.0f - t) + m_targetPos * t;
		obj->SetPos(result, 0);
	}
}
