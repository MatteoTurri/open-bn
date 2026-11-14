/*
 * Prim: geometric primitive object of the 3D scene (cube, ball, cone...).
 */

#ifndef PRIM_H
#define PRIM_H

#include <vector>
#include "object.h"

class Object;
class gm_graph;
struct vec4;

/* Prim::flags. */
#define PRIM_REBUILD_LIST 0x001  /* rebuild the display list */
#define PRIM_RECALC_DIV   0x002  /* recompute the subdivisions */
#define PRIM_DIVU_SET     0x100  /* div_u set explicitly, not taken from the shape defaults */
#define PRIM_DIVV_SET     0x200  /* div_v set explicitly */
#define PRIM_TEX_NONPOW2  0x400  /* the texture is not a power of two */

/* Prim::shape, option "TYPE". */
enum {
	PRIM_CUBE,
	PRIM_BALL,
	PRIM_SQUARE,
	PRIM_CIRCLE,
	PRIM_PRISM,
	PRIM_COLUMN,
	PRIM_PYRAMID,
	PRIM_CONE,
	PRIM_SHAPE_COUNT
};

class Prim : public Object {
public:
	/* 0x250 */ int flags; /* PRIM_* */
	/* 0x254 */ int unk_254;
	/* 0x258 */ int display_list; /* of the shape (gm_graph::CreateList) */
	/* 0x25C */ int tex_handle; /* released with tmRelease */
	/* 0x260 */ int shape; /* option "TYPE", PRIM_CUBE..PRIM_CONE */
	/* 0x264 */ int num_parts; /* option "PNUM" */
	/* 0x268 */ float div_level; /* option "DIVL": subdivision scale */
	/* 0x26C */ float div_u; /* option "DIVU": base subdivisions along u */
	/* 0x270 */ float div_v; /* option "DIVV": base subdivisions along v */
	/* 0x274 */ int cur_div_u; /* current subdivisions along u */
	/* 0x278 */ int cur_div_v; /* current subdivisions along v */
	/* 0x27C */ int face; /* option "FACE": FACE_FRONT, FACE_BACK */
	/* 0x280 */ int fade; /* option "FADE": FADE_ALWAYS, else when translucent: Z-only pass */
	/* 0x284 */ int anti; /* option "ANTI": ANTI_ALWAYS, else when opaque: antialiased pass */
	Prim(Object *parent);
	virtual ~Prim();

	virtual void DrawThis(gm_graph &g);
	virtual void SetTexture(int texture, int option, int mode);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;

	void DrawShape(gm_graph &g);
	void DrawCircle(gm_graph &g);
	void DrawSquare(gm_graph &g);
	void DrawCube(gm_graph &g);
	void DrawBall(gm_graph &g);
	void DrawColumn(gm_graph &g);
	void DrawPrism(gm_graph &g);
	void DrawPyramid(gm_graph &g);
	void DrawCone(gm_graph &g);
	void DrawDisk(gm_graph &g, int sectors, int subdiv, int rings, float radius, float height, float vStart,
	    float vEnd);
	void DrawFlatTube(gm_graph &g, int sectors, int subdiv, int rings, float radiusFirst, float radiusLast,
	    float vStart, float vEnd);
	void DrawSmoothTube(gm_graph &g, int sectors, int subdiv, int rings, float radiusFirst, float radiusLast,
	    float vStart, float vEnd);
	void DrawPlate(gm_graph &g, int cols, int rows, const vec4 &c0, const vec4 &c1, const vec4 &c2, const vec4 &c3,
	    const vec4 &normal);
};

#endif
