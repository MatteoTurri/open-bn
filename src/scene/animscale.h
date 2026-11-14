/*
 * AnimScale: scale animation of an Object.
 */

#ifndef ANIMSCALE_H
#define ANIMSCALE_H

#include "animation.h"

/* Interpolates the scale linearly from m_start to m_target. */
class AnimScale : public Animation {
public:
	/* 0x30 */ vec4 m_start;
	/* 0x40 */ vec4 m_target;

	AnimScale(const vec4 &start, const vec4 &target, float durationMs, float delayMs, int mode);
	virtual ~AnimScale();
	virtual void Do(Object *obj);
	virtual void ResetTarget(const vec4 &target);
}; /* size = 0x50 */

#endif
