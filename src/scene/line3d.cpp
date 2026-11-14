/*
 * Line3d methods: node ring, segment interpolation and drawing.
 */

#include <vector>
#include <string>
#include "gfx/vec.h"
#include "line3d.h"
#include "gfx/gm_graph.h"
#include "gfx/gm_vertex.h"
#include "scene/optid.h"

/* Identity matrix and w = 1 row. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

/* Two vertices of format 7 (position, normal, RGBA color); DrawThis
   rewrites only the positions. */
static gmVertexPNC s_lineVertexBuf[2] = {
	{ -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff },
	{ 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xffffffff },
};

inline Line3d::Line3d(Object *parent, int count) : Object(parent)
{
	node_count = count;
	seg_count = count - 1;
	node_array = 0;
	dirty = 0;
	cur_segment = -1;
	loop = 0;
	vec4 a(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 b(0.0f, 0.0f, 0.0f, 1.0f);
	move = new AnimMoveLine(a, b, 1000.0f, 0.0f, 1, 0.0f, 0.0f);
}

inline Line3d::~Line3d()
{
	if (move)
		delete move;
	if (node_array)
		delete[] node_array;
}

inline void Line3d::InitNodeArray(int count)
{
	if (count < 2)
		return;
	Line3dNode *old = node_array;
	node_count = count;
	if (old)
		delete[] old;
	Line3dNode *newArray = new Line3dNode[node_count];
	node_array = newArray;
	seg_count = loop ? node_count : node_count - 1;
	SetSegment(0);
}

inline void Line3d::SetSegment(int seg)
{
	int idx = seg % seg_count;
	if (cur_segment == idx && dirty == 0)
		return;
	cur_segment = seg % seg_count;
	Line3dNode *a = &node_array[idx];
	int nextIdx = (idx + 1) % node_count;
	Line3dNode *b = &node_array[nextIdx];
	move->ResetLine(a->pos, b->pos, 0.0f, 0.0f);
	dirty = 0;
}

inline void Line3d::SetDirty(int seg)
{
	dirty = 1;
}

inline void Line3d::SetNodePos(const vec4 &pos, int seg)
{
	node_array[(node_count + seg) % node_count].pos = pos;
	/* SetDirty ignores the segment, but the argument is still evaluated. */
	SetDirty((node_count + seg) % node_count);
}

inline vec4 Line3d::GetPosLine(float t, int seg, Object *o)
{
	vec4 result(0.0f, 0.0f, 0.0f, 1.0f);
	if (!node_array)
		return result;
	float w = move->DoFunc(t, seg, 0.0f, 0.0f);
	result = move->GetPosInterpolate(w);
	return result;
}

inline void Line3d::GetPosLine(float &x, float &y, float &z, float t, int seg, Object *o)
{
	if (!node_array)
		return;
	float w = move->DoFunc(t, seg, 0.0f, 0.0f);
	vec4 result = move->GetPosInterpolate(w);
	x = result.x;
	y = result.y;
	z = result.z;
}

inline int Line3d::SetOption(int option, const int &value, unsigned int size)
{
	if (option == OPT_LOOP) {
		loop = 1;
		seg_count = node_count;
		return 1;
	}
	return Object::SetOption(option, value, 1);
}

inline int Line3d::SetOption(int option, const float &value, unsigned int size)
{
	return Object::SetOption(option, value, 1);
}

inline int Line3d::SetOption(int option, const String &value)
{
	return Object::SetOption(option, value);
}

inline int Line3d::GetOption(int option, int &value, unsigned int size) const
{
	if (option == OPT_LOOP) {
		value = loop;
		return 1;
	}
	return Object::GetOption(option, value, 1);
}

inline int Line3d::GetOption(int option, float &value, unsigned int size) const
{
	return Object::GetOption(option, value, 1);
}

inline int Line3d::GetOption(int option, String &value) const
{
	return Object::GetOption(option, value);
}

void Line3d::DrawThis(gm_graph &g)
{
	if (!visible)
		return;
	if (!seg_count)
		return;
	if (!node_count)
		return;
	if (!node_array)
		return;

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE);
	g.BlendFunc(blend_mode);

	for (int i = 0; i < seg_count; i++) {
		s_lineVertexBuf[0].x = node_array[i].pos.x;
		s_lineVertexBuf[0].y = node_array[i].pos.y;
		s_lineVertexBuf[0].z = node_array[i].pos.z;

		int next = i + 1;
		s_lineVertexBuf[1].x = node_array[next % node_count].pos.x;
		s_lineVertexBuf[1].y = node_array[next % node_count].pos.y;
		s_lineVertexBuf[1].z = node_array[next % node_count].pos.z;

		g.SetArrays(s_lineVertexBuf, GM_VERTEX_PNC, 2, 1, 0, 0);
		g.DrawArrays(GM_PRIM_LINE, 0, 2, 1, 0);
	}
}
