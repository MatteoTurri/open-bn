/*
 * Hermite: cubic spline object of the scene (the cubic twin of Line3d).
 */

#ifndef HERMITE_H
#define HERMITE_H

#include <string>
#include <vector>

#include "gfx/vec_types.h"
#include "object.h"
#include "animmovehermite.h"
#include "line3d.h"

/* Spline node: position and tangent. */
struct HermiteNode {
	/* 0x00 */ vec4 pos;
	/* 0x10 */ vec4 vec;
};

class gm_graph;

/*
 * Cubic spline through an array of nodes; the current segment is interpolated
 * by an AnimMoveHermite and drawn with a Line3d.
 */
class Hermite : public Object {
public:
	/* 0x250 */ AnimMoveHermite *move;
	/* 0x254 */ HermiteNode *node_array;
	/* 0x258 */ int node_count;
	/* 0x25C */ int dirty;  /* segment must be reloaded */
	/* 0x260 */ int seg_count;
	/* 0x264 */ int cur_segment;  /* segment loaded in move */
	/* 0x268 */ Line3d *line;
	/* 0x26C */ int loop_flag;  /* 1 = recompute the tangents when a node moves */
	/* 0x270 */ int predict_mode;  /* option LOOP: closed curve */

	Hermite(Object *parent, int count);
	virtual ~Hermite();

	void PredictorNodeVec(int seg);
	void InitNodeArray(int count, int loop);
	void SetSegment(int seg);

	void SetDirty(int seg);
	void SetNodePos(const vec4 &pos, int seg);
	void SetNodeVec(const vec4 &vec, int seg);
	vec4 GetPosHermite(float t, int seg, Object *obj);
	void GetPosHermite(float &x, float &y, float &z, float t, int seg, Object *obj);

	virtual void DrawThis(gm_graph &g);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const String &value);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, String &value) const;
};

#endif
