/*
 * AnimRotate: rotation animation of an Object given as Euler angles.
 */

#ifndef ANIMROTATE_H
#define ANIMROTATE_H

#include "animation.h"

/* Spherical interpolation between the quaternions of two sets of Euler angles. */
class AnimRotate : public Animation {
public:
	/* 0x30 */ vec4 m_start;
	/* 0x40 */ vec4 m_target;
	/* 0x50 */ int m_order;  /* rotation order: 0 = YXZ, 1 = ZYX */

	AnimRotate(const vec4 &start, const vec4 &target, float durationMs, float delayMs, int mode, int order);
	virtual ~AnimRotate();
	virtual void Do(Object *obj);
	virtual void ResetTarget(const vec4 &target);
}; /* size = 0x60 */

#endif
