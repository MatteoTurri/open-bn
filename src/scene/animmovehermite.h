/*
 * AnimMoveHermite: cubic Hermite position animation (used by Hermite).
 */

#ifndef ANIMMOVEHERMITE_H
#define ANIMMOVEHERMITE_H

#include "animation.h"

class Object;

/*
 * Moves an Object along the Hermite curve from m_posA to m_posB with the
 * tangents m_vecA and m_vecB (192 bytes).
 */
class AnimMoveHermite : public Animation {
public:
	/* 0x30 */ vec4 m_posA;
	/* 0x40 */ vec4 m_posB;
	/* 0x50 */ vec4 m_vecA;
	/* 0x60 */ vec4 m_vecB;
	/* Working copy of the control points, tangents multiplied by Animation::t_diff_inv. */
	/* 0x70 */ vec4 m_curPosA;
	/* 0x80 */ vec4 m_curPosB;
	/* 0x90 */ vec4 m_curVecA;
	/* 0xA0 */ vec4 m_curVecB;
	/* 0xB0 */ int pad_B0[4];

	AnimMoveHermite(const vec4 &posA, const vec4 &posB, const vec4 &vecA, const vec4 &vecB, float durationMs,
	    float delayMs, int mode);
	virtual ~AnimMoveHermite();

	void Do(Object *obj);
	vec4 GetPosInterpolate(float t);
	void ResetHermite(const vec4 &posA, const vec4 &posB, const vec4 &vecA, const vec4 &vecB);
	void ResetAll(const vec4 &posA, const vec4 &posB, const vec4 &vecA, const vec4 &vecB, float durationMs,
	    float delayMs, int mode);
	void ResetTarget(const vec4 &target);

	static mat4 hermite_mat;
}; /* size = 0xC0 */

#endif
