/*
 * AnimScale: scale animation of an Object.
 */

#include <vector>
#include "gfx/vec.h"
#include "animation.h"
#include "object.h"
#include "animscale.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

inline AnimScale::~AnimScale()
{}

inline AnimScale::AnimScale(const vec4 &start, const vec4 &target, float durationMs, float delayMs, int mode)
        : Animation(durationMs, delayMs, mode)
{
	m_start = start;
	m_target = target;
}

inline void AnimScale::ResetTarget(const vec4 &target)
{
	m_target = target;
}

void AnimScale::Do(Object *obj)
{
	if (!obj)
		return;
	float t = DoTimer(m_mode, 0.0f, 0.0f, 1.0f);
	if (t >= 1.0f) {
		obj->SetScale(m_target);
		return;
	}
	obj->SetScale(m_start * (1.0f - t) + m_target * t);
}
