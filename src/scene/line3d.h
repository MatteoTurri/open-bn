/*
 * Line3d: Object holding a ring of nodes joined by straight segments, with
 * an AnimMoveLine interpolator; the linear counterpart of Hermite.
 */

#ifndef LINE3D_H
#define LINE3D_H

#include <string>

#include "gfx/vec_types.h"

#include "object.h"

#include "animmoveline.h"

struct Line3dNode {
	vec4 pos;
};

class gm_graph;

class Line3d : public Object {
public:
	/* 0x250 */ AnimMoveLine *move;  /* linear interpolator */
	/* 0x254 */ Line3dNode *node_array;  /* node ring */
	/* 0x258 */ int node_count;
	/* 0x25C */ int dirty;
	/* 0x260 */ int seg_count;
	/* 0x264 */ int cur_segment;  /* segment loaded into move, -1 for none */
	/* 0x268 */ int loop;  /* closed loop, option 'LOOP' */

	Line3d(Object *parent, int count);
	virtual ~Line3d();

	void InitNodeArray(int count);
	void SetSegment(int seg);
	void SetDirty(int seg);
	void SetNodePos(const vec4 &pos, int seg);
	vec4 GetPosLine(float t, int seg, Object *o);
	void GetPosLine(float &x, float &y, float &z, float t, int seg, Object *o);

	virtual void DrawThis(gm_graph &g);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const String &value);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, String &value) const;
};

#endif
