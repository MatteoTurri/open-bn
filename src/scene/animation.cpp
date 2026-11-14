/*
 * Animation: timer, frame time and interpolation curves shared by all the
 * animations of the scene.
 */

#include <iostream.h>
#include "gfx/vec.h"
#include "animation.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

float Animation::t_diff = 1.0f / 60.0f;
float Animation::t_diff_inv = 60.0f;
float Animation::t_diff_curr = 1.0f / 60.0f;

int Animation::GetState()
{
	return m_state;
}

void Animation::InitFrameTime(float dt)
{
	float t = dt * 0.001f;
	float inv = 1.0f / t;
	t_diff = t;
	t_diff_curr = t;
	t_diff_inv = inv;
}

void Animation::SetCurrFrameTime(float dt)
{
	t_diff_curr = dt;
}

float Animation::GetFrameTime()
{
	return t_diff;
}

/*
 * Restarts the timer: a duration of `durationMs`, rounded down to whole frames
 * (at least one), after a delay of `delayMs`; `mode` is the curve.
 */
void Animation::ResetTimer(float durationMs, float delayMs, int mode)
{
	m_totalTime = durationMs * 0.001f;
	float delay = delayMs * 0.001f;
	m_currTime = 0.0f;
	m_delayElapsed = 0.0f;
	m_delay = delay;
	if (delay < t_diff)
		m_state = ANIM_STATE_RUN;
	int frameCount = (int)(m_totalTime * t_diff_inv);
	float wholeFrames = (float)frameCount * t_diff;
	m_currValue = 0.0f;
	m_mode = mode;

	float root = wholeFrames * 0.999999;
	m_totalTime = root;
	if (root < t_diff)
		m_totalTime = t_diff;
	m_rate = 1.0f / m_totalTime;

	if (m_mode == ANIM_FUNC_EXPONENT) {
		if (frameCount < 5) {
			switch (frameCount) {
			case 1:
				m_easeParam = 0.9615384936332703f;
				break;
			case 2:
				m_easeParam = 0.78125f;
				break;
			case 3:
				m_easeParam = 0.6369426250457764f;
				break;
			case 4:
				m_easeParam = 0.5291005373001099f;
				break;
			case 0:
			default:
				m_easeParam = 1.0f;
				break;
			}
		} else {
			m_easeParam = 1.0f / ((frameCount - 5) * 0.33333298563957214f + 2.200000047683716f);
		}
	}
}

void Animation::SetCurrTime(float t)
{
	m_currTime = t * 0.001f;
}

float Animation::GetCurrTime() const
{
	return m_currTime / 0.001f;
}

float Animation::GetCurrValue() const
{
	return m_currValue;
}

Animation::Animation()
{
	m_state = ANIM_STATE_DELAY;
}

Animation::Animation(float durationMs, float delayMs, int mode)
{
	m_state = ANIM_STATE_DELAY;
	ResetTimer(durationMs, delayMs, mode);
}

Animation::~Animation()
{}

void Animation::Do(Object *obj)
{
	if (obj)
		DoTimer(m_mode, 0.0f, 0.0f, 1.0f);
}

void Animation::ResetTarget(const vec4 &target)
{}

float Animation::DoFunc(float val, int modeOverride, float startSlope, float endSlope)
{
	int idx = m_mode;
	if (modeOverride)
		idx = modeOverride;
	switch (idx) {
	case ANIM_FUNC_SLOWER:
		return DoFuncSlower(val, m_totalTime);
	case ANIM_FUNC_FASTER:
		return DoFuncFaster(val, m_totalTime);
	case ANIM_FUNC_INTERPOLATE:
		return DoFuncInterpolate(val, m_totalTime, startSlope, endSlope);
	case ANIM_FUNC_ANGULER:
		return DoFuncAnguler(val, m_totalTime);
	case ANIM_FUNC_EXPONENT:
		return DoFuncExponent(val, m_totalTime);
	case ANIM_FUNC_LINEAR:
	default:
		return DoFuncLinear(val, m_totalTime);
	}
}

float Animation::DoFuncLinear(float val, float dur)
{
	if (val >= 1.0f)
		return 1.0f;
	if (val <= 0.0f)
		return 0.0f;
	return val;
}

float Animation::DoFuncSlower(float val, float dur)
{
	if (val >= 1.0f)
		return 1.0f;
	if (val <= 0.0f)
		return 0.0f;
	return sqrtf(val);
}

float Animation::DoFuncFaster(float val, float dur)
{
	if (val >= 1.0f)
		return 1.0f;
	if (val <= 0.0f)
		return 0.0f;
	return val * val;
}

float Animation::DoFuncAnguler(float val, float dur)
{
	if (val >= 1.0f)
		return 1.0f;
	if (val <= 0.0f)
		return 0.0f;
	float x = (val - 0.5f) * 3.1459200382232666f;
	int i = (int)(x * 32768.0f / 3.14159274f);
	float s = gmGetTableSin(i);
	return (s + 1.0f) * 0.5f;
}

float Animation::DoFuncExponent(float val, float dur)
{
	float ratio = 1.0f / (dur * 3.0f + 1.0f);
	float r;

	if (val < ratio) {
		float root = dur * 0.5;
		if (1.0f < root)
			root = 1.0f;
		float t = val * ((1.0f - root) / ratio);
		r = m_currValue + (1.0f - m_currValue) * (m_easeParam * (root + t));
	} else {
		r = m_currValue + (1.0f - m_currValue) * m_easeParam;
	}
	return r;
}

float Animation::DoFuncInterpolate(float val, float dur, float startSlope, float endSlope)
{
	if (val >= 1.0f)
		return 1.0f;
	if (val <= 0.0f)
		return 0.0f;
	return val
	    * (val
	            * (val * ((startSlope + endSlope) * dur - 2.0f)
	                - ((startSlope + startSlope + endSlope) * dur - 3.0f))
	        + startSlope * dur);
}

/* Advances the timer by one frame (scaled by `speed`) and returns the current value. */
float Animation::DoTimer(int modeOverride, float startSlope, float endSlope, float speed)
{
	switch (m_state) {
	case ANIM_STATE_DELAY:
		m_delayElapsed += t_diff_curr * speed;
		if (m_delay <= m_delayElapsed)
			m_state = ANIM_STATE_RUN;
		m_currValue = 0.0f;
		break;
	case ANIM_STATE_RUN:
		m_currTime += t_diff_curr * speed;
		m_currValue = DoFunc(m_currTime * m_rate, modeOverride, startSlope, endSlope);
		if (m_currValue >= 1.0f) {
			m_state = ANIM_STATE_DONE;
			m_currValue = 1.0f;
		}
		break;
	case ANIM_STATE_DONE:
		m_currTime = m_totalTime;
		m_currValue = 1.0f;
		break;
	}
	return m_currValue;
}
