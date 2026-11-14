/*
 * AnimColor: color animation of an Object.
 */

#ifndef ANIMCOLOR_H
#define ANIMCOLOR_H

#include "animation.h"

/* Interpolates the color linearly from m_start to m_target. */
class AnimColor : public Animation {
public:
	/* 0x30 */ vec4 m_start;
	/* 0x40 */ vec4 m_target;

	AnimColor(const vec4 &start, const vec4 &target, float durationMs, float delayMs, int mode);
	virtual ~AnimColor();
	virtual void Do(Object *obj);
	virtual void ResetTarget(const vec4 &target);
}; /* size = 0x50 */

#endif
