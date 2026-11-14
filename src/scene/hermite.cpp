/*
 * Hermite methods.
 */

#include <string>
#include <vector>
#include "gfx/vec.h"
#include "hermite.h"
#include "scene/optid.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

inline Hermite::Hermite(Object *parent, int count) : Object(parent)
{
	node_count = count;
	seg_count = count - 1;
	node_array = 0;
	dirty = 0;
	cur_segment = -1;
	loop_flag = 0;
	predict_mode = 0;
	vec4 a(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 b(0.0f, 0.0f, 0.0f, 1.0f);
	move = new AnimMoveHermite(a, b, a, b, 1000.0f, 0.0f, 1);
	line = new Line3d(this, 0);
	line->visible = 0;
	line->InitNodeArray(2);
}

inline Hermite::~Hermite()
{
	if (move)
		delete move;
	if (line)
		delete line;
	if (node_array)
		delete node_array;  /* NOTE: bug: array from new[] freed with scalar delete */
}

inline void Hermite::InitNodeArray(int count, int loop)
{
	if (count < 2)
		return;
	HermiteNode *old = node_array;
	node_count = count;
	if (old)
		delete old;     /* NOTE: bug: array from new[] freed with scalar delete */
	HermiteNode *newArray = new HermiteNode[node_count];
	node_array = newArray;
	seg_count = predict_mode ? node_count : node_count - 1;
	SetSegment(0);
	loop_flag = loop;
}

inline void Hermite::SetSegment(int seg)
{
	int idx = seg % seg_count;
	if (cur_segment == idx && dirty == 0)
		return;
	cur_segment = seg % seg_count;
	HermiteNode *a = &node_array[idx];
	HermiteNode *b = &node_array[(idx + 1) % node_count];
	int nextIdx = (idx + 1) % node_count;  /* unused, but part of the layout */
	move->ResetHermite(a->pos, b->pos, a->vec, b->vec);
	dirty = 0;
}

inline void Hermite::SetDirty(int seg)
{
	dirty = 1;
	if (loop_flag == 1) {
		PredictorNodeVec(seg);
		PredictorNodeVec(seg - 1);
		PredictorNodeVec(seg + 1);
	}
}

inline void Hermite::SetNodePos(const vec4 &pos, int seg)
{
	node_array[(node_count + seg) % node_count].pos = pos;
	int idx = (node_count + seg) % node_count;
	dirty = 1;
	if (loop_flag == 1) {
		PredictorNodeVec(idx);
		PredictorNodeVec(idx - 1);
		PredictorNodeVec(idx + 1);
	}
}

inline void Hermite::SetNodeVec(const vec4 &vec, int seg)
{
	node_array[(node_count + seg) % node_count].vec = vec;
}

inline vec4 Hermite::GetPosHermite(float t, int seg, Object *obj)
{
	vec4 result(0.0f, 0.0f, 0.0f, 1.0f);
	if (node_array == 0)
		return result;
	float u = move->DoFunc(t, 4, 1.0f, 1.0f);
	result = move->GetPosInterpolate(u);
	if (obj)
		result = GetPos(obj, result);
	return result;
}

/* As above, into x, y, z; leaves them untouched if there are no nodes. */
inline void Hermite::GetPosHermite(float &x, float &y, float &z, float t, int seg, Object *obj)
{
	if (!node_array)
		return;
	float u = move->DoFunc(t, 4, 1.0f, 1.0f);
	vec4 result = move->GetPosInterpolate(u);
	if (obj)
		result = GetPos(obj, result);
	x = result.x;
	y = result.y;
	z = result.z;
}

inline int Hermite::SetOption(int option, const int &value, unsigned int size)
{
	if (option != OPT_DEBG) {
		if (option == OPT_LOOP) {
			predict_mode = value;
			if (predict_mode) {
				seg_count = node_count;
			}
			if (loop_flag == 1) {
				PredictorNodeVec(node_count - 1);
				PredictorNodeVec(0);
			}
			return 1;
		}
	} else {
		line->visible = 1;
	}
	return Object::SetOption(option, value, 1);
}

inline int Hermite::SetOption(int option, const float &value, unsigned int size)
{
	return Object::SetOption(option, value, 1);
}

inline int Hermite::SetOption(int option, const String &value)
{
	return Object::SetOption(option, value);
}

inline int Hermite::GetOption(int option, int &value, unsigned int size) const
{
	if (option == OPT_LOOP) {
		value = predict_mode;
		return 1;
	}
	return Object::GetOption(option, value, 1);
}

inline int Hermite::GetOption(int option, float &value, unsigned int size) const
{
	return Object::GetOption(option, value, 1);
}

inline int Hermite::GetOption(int option, String &value) const
{
	return Object::GetOption(option, value);
}

/*
 * Recomputes the tangent of node `seg` as half the difference between the
 * next and the previous node, times the frame time; on an open curve the end
 * nodes get an almost zero tangent (x 1e-4).
 */
void Hermite::PredictorNodeVec(int seg)
{
	if (!node_array)
		return;

	float dt = Animation::GetFrameTime();

	node_array[(node_count + seg) % node_count].vec =
	    (node_array[(node_count + seg + 1) % node_count].pos - node_array[(node_count + seg - 1) % node_count].pos)
	    * dt * 0.5f;

	if (predict_mode)
		return;

	if (seg % node_count == 0 || seg % node_count == node_count - 1)
		node_array[(node_count + seg) % node_count].vec *= 1.0e-4f;
}

/* Draws each segment as a polyline with steps of 0.1 through the Line3d, then reloads the current segment. */
void Hermite::DrawThis(gm_graph &g)
{
	if (!visible)
		return;
	if (!seg_count)
		return;
	if (!node_count)
		return;
	if (!node_array)
		return;
	if (!debug)
		return;

	int seg = cur_segment;
	for (int i = 0; i < seg_count; i++) {
		float t = 0.0f;
		float step = 0.1f;
		SetSegment(i);
		while (t + step <= 1.0f) {
			line->SetNodePos(GetPosHermite(t, 0, 0), 0);
			line->SetNodePos(GetPosHermite(t + step, 0, 0), 1);
			t += step;
			line->DrawThis(g);
		}
	}
	SetSegment(seg);
}
