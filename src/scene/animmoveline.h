/*
 * AnimMoveLine: linear position animation (used by Line3d).
 */

#ifndef ANIMMOVELINE_H
#define ANIMMOVELINE_H

#include "animation.h"

class Object;

/* Moves an Object from m_startPos to m_targetPos (96 bytes). */
class AnimMoveLine : public Animation {
public:
	/* 0x30 */ vec4 m_startPos;
	/* 0x40 */ vec4 m_targetPos;
	/* 0x50 */ float m_startSlope;  /* curve 4 */
	/* 0x54 */ float m_endSlope;

	AnimMoveLine(const vec4 &start, const vec4 &target, float durationMs, float delayMs, int mode, float startSlope,
	    float endSlope);
	virtual ~AnimMoveLine();

	void Do(Object *obj);
	vec4 GetPosInterpolate(float t);
	void ResetLine(const vec4 &start, const vec4 &target, float startSlope, float endSlope);
	void ResetAll(const vec4 &start, const vec4 &target, float durationMs, float delayMs, int mode,
	    float startSlope, float endSlope);
	void ResetTarget(const vec4 &target);
}; /* size = 0x60 */

#endif
