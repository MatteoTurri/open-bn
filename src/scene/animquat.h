/*
 * AnimQuat: rotation animation of an Object between two quaternions.
 */

#ifndef ANIMQUAT_H
#define ANIMQUAT_H

#include "animation.h"

/* Spherical interpolation from m_start to m_target (quaternions). */
class AnimQuat : public Animation {
public:
	/* 0x30 */ vec4 m_start;
	/* 0x40 */ vec4 m_target;

	AnimQuat(const vec4 &start, const vec4 &target, float durationMs, float delayMs, int mode);
	virtual ~AnimQuat();
	virtual void Do(Object *obj);
	virtual void ResetTarget(const vec4 &target);
}; /* size = 0x50 */

#endif
