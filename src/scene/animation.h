/*
 * Animation: base class of the timed animations of the scene.
 */

#ifndef ANIMATION_H
#define ANIMATION_H

#include "gfx/vec_types.h"

#include <math.h>

class Object;

/* Animation::m_state. */
#define ANIM_STATE_DELAY 0  /* waiting for m_delay */
#define ANIM_STATE_RUN 1
#define ANIM_STATE_DONE 2

/* Animation::m_mode: the DoFunc curves (0 as an override: the curve of m_mode). */
#define ANIM_FUNC_DEFAULT 0
#define ANIM_FUNC_LINEAR 1
#define ANIM_FUNC_SLOWER 2
#define ANIM_FUNC_FASTER 3
#define ANIM_FUNC_INTERPOLATE 4
#define ANIM_FUNC_ANGULER 5
#define ANIM_FUNC_EXPONENT 6

/*
 * Time base of an animation: after a delay it runs for m_totalTime seconds
 * and yields a value going from 0 to 1 along one of the DoFunc curves.
 * Standalone class (not an Object); its vptr follows the data, at 0x24, and
 * derived classes start their fields at 0x30.
 */
class Animation {
public:
	/* 0x00 */ float m_totalTime;  /* duration in seconds, whole frames */
	/* 0x04 */ float m_rate;  /* 1 / m_totalTime */
	/* 0x08 */ float m_currTime;  /* seconds */
	/* 0x0C */ float m_currValue;  /* value, 0 to 1 */
	/* 0x10 */ float m_easeParam;  /* curve parameter (ANIM_FUNC_EXPONENT) */
	/* 0x14 */ float m_delay;  /* start delay in seconds */
	/* 0x18 */ float m_delayElapsed;  /* delay elapsed so far */
	/* 0x1C */ int m_state;  /* ANIM_STATE_* */
	/* 0x20 */ int m_mode;  /* DoFunc curve, ANIM_FUNC_* */

	Animation(float durationMs, float delayMs, int mode);
	Animation();
	virtual ~Animation();

	virtual void Do(Object *obj);
	virtual void ResetTarget(const vec4 &target);

	int GetState();
	void SetCurrTime(float t);
	float GetCurrTime() const;
	float GetCurrValue() const;
	static void SetCurrFrameTime(float dt);
	static float GetFrameTime();
	static void InitFrameTime(float dt);

	inline void ResetTimer(float durationMs, float delayMs, int mode);
	float DoTimer(int modeOverride, float startSlope, float endSlope, float speed);
	float DoFunc(float val, int modeOverride, float startSlope, float endSlope);

	float DoFuncLinear(float val, float dur);
	float DoFuncSlower(float val, float dur);
	float DoFuncFaster(float val, float dur);
	float DoFuncAnguler(float val, float dur);
	float DoFuncExponent(float val, float dur);
	float DoFuncInterpolate(float val, float dur, float startSlope, float endSlope);

	/* Frame time in seconds (1/60 by default), its inverse and the current frame time. */
	static float t_diff;
	static float t_diff_inv;
	static float t_diff_curr;
};

#endif
