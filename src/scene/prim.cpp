/*
 * Prim: geometric primitives of the 3D scene (square, cube, ball, disk, tubes),
 * generated as interleaved vertex arrays and cached in a gm_graph display list.
 */

#include "prim.h"
#include "gfx/vec.h"
#include "gfx/gm_graph.h"
#include "gfx/gm_vertex.h"
/* Texture manager (C interface) and the scene graph. */
#include "gfx/tm.h"
#include "scenefacade.h"
#include "scene/optid.h"

/* Unit matrix and unit w row: the file-scope objects built at startup. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

inline Prim::Prim(Object *parent) : Object(parent)
{
	option_state.SetLightingMode(1);

	flags = PRIM_REBUILD_LIST;
	unk_254 = 10000;
	display_list = 0;
	tex_handle = 0;
	shape = PRIM_CUBE;
	num_parts = 3;
	cur_div_u = 1;
	cur_div_v = 1;
	face = FACE_FRONT | FACE_BACK;
	fade = 0;
	anti = 0;
	div_level = 1.0f;
	div_u = 1.0f;
	div_v = 1.0f;

	display_list = SceneGetGraph()->CreateList();
}

inline Prim::~Prim()
{
	if (tex_handle)
		tmRelease(tex_handle);
	SceneGetGraph()->DeleteList(display_list);
}

/* Per-shape subdivision table, indexed by shape (0..7). def_* are the
   default subdivisions (used when PRIM_DIVU_SET/PRIM_DIVV_SET are clear), min/max
   clamp the subdivisions computed by DrawThis. */
struct SubdivInfo {
	signed char def_x, def_y;
	signed char min_x, min_y;
	signed char max_x, max_y;
};

SubdivInfo SubdivInfos[PRIM_SHAPE_COUNT] = {
	{ 1, 1, 1, 1, 16, 16 },
	{ 48, 24, 4, 2, 64, 32 },
	{ 1, 1, 1, 1, 16, 16 },
	{ 48, 1, 4, 1, 64, 16 },
	{ 1, 1, 1, 1, 64, 32 },
	{ 48, 1, 4, 1, 64, 32 },
	{ 1, 1, 1, 1, 64, 21 },
	{ 48, 1, 4, 1, 64, 21 },
};

/* PRIM_RECALC_DIV recomputes the subdivisions (clamped by SubdivInfos), PRIM_REBUILD_LIST
   rebuilds the cached display list; the list is then drawn with the passes
   selected by anti/fade/face (color overlay and masking). */
void Prim::DrawThis(gm_graph &g)
{
	if (!visible)
		return;

	/* bit 10 is PRIM_TEX_NONPOW2 */
	g.SetEnableBits(
	    (((unsigned)flags >> 10) & 1) ? (g.fFlagsE | GM_ENABLE_TEXTURE_FIT) : (g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT));
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
	g.SetTexture(tex_handle);
	g.BlendFunc(blend_mode);

	if (flags & PRIM_RECALC_DIV) {
		flags &= ~PRIM_RECALC_DIV;
		int ipx = (int)(div_u * div_level + 0.5f);
		const SubdivInfo *row = &SubdivInfos[shape];
		int ipy = (int)(div_v * div_level + 0.5f);
		if (ipx < row->min_x)
			ipx = row->min_x;
		if (ipx > row->max_x)
			ipx = row->max_x;
		if (ipy < row->min_y)
			ipy = row->min_y;
		if (ipy > row->max_y)
			ipy = row->max_y;
		if (ipx != cur_div_u || ipy != cur_div_v) {
			cur_div_u = ipx;
			cur_div_v = ipy;
			flags |= PRIM_REBUILD_LIST;
		}
	}

	if (flags & PRIM_REBUILD_LIST) {
		flags &= ~PRIM_REBUILD_LIST;
		g.BeginList(display_list);
		DrawShape(g);
		g.EndList(0);
	}

	if (anti) {
		int doit = (anti == ANTI_ALWAYS);
		const vec4 &wc = GetWorldColor();
		if (0.99f < wc.w) {
			g.Color(wc.x, wc.y, wc.z, 1.0f);
			doit = 1;
		}
		if (doit) {
			unsigned int saved = g.fFlagsE;
			unsigned int mask = saved | GM_ENABLE_ANTIALIAS;
			if (face) {
				mask |= GM_ENABLE_CULL_FACE | GM_ENABLE_FRONT_FACE;
				if (face == FACE_BACK)
					mask &= ~GM_ENABLE_FRONT_FACE;
			}
			g.SetEnableBits(mask);
			g.CallList(display_list);
			g.Color(GetWorldColor());
			g.SetEnableBits(saved);
		}
	}

	if (fade) {
		if (fade == FADE_ALWAYS || GetWorldColor().w < 0.99f) {
			g.ColorMask(0);
			g.CallList(display_list);
			g.ColorMask(option_state.GetColorMask());
		}
	}

	if (face) {
		unsigned int baseFlags = g.fFlagsE;
		unsigned int flags4 = baseFlags | GM_ENABLE_CULL_FACE;
		if (face & FACE_BACK) {
			g.SetEnableBits(flags4 & ~GM_ENABLE_FRONT_FACE);
			g.CallList(display_list);
		}
		if (face & FACE_FRONT) {
			g.SetEnableBits(flags4 | GM_ENABLE_FRONT_FACE);
			g.CallList(display_list);
		}
		g.SetEnableBits(baseFlags);
	} else {
		g.CallList(display_list);
	}
}

inline void Prim::DrawShape(gm_graph &g)
{
	switch (shape) {
	case PRIM_CUBE:
		DrawCube(g);
		break;
	case PRIM_BALL:
		DrawBall(g);
		break;
	case PRIM_SQUARE:
		DrawSquare(g);
		break;
	case PRIM_CIRCLE:
		DrawCircle(g);
		break;
	case PRIM_PRISM:
		DrawPrism(g);
		break;
	case PRIM_COLUMN:
		DrawColumn(g);
		break;
	case PRIM_PYRAMID:
		DrawPyramid(g);
		break;
	case PRIM_CONE:
		DrawCone(g);
		break;
	}
}

inline void Prim::DrawCircle(gm_graph &g)
{
	DrawDisk(g, cur_div_u, 1, cur_div_v, 0.5f, 0.0f, 0.0f, 1.0f);
}

inline void Prim::DrawColumn(gm_graph &g)
{
	int n = cur_div_v;
	int base = cur_div_u;
	int top = (n > -1) ? n : n + 3;
	top >>= 2;
	int bottom = n - top * 2;
	if (top <= 0)
		top = 1;
	if (bottom <= 0)
		bottom = 1;

	DrawDisk(g, base, 1, top, 0.5f, 0.5f, 0.0f, 0.25f);
	DrawSmoothTube(g, base, 1, bottom, 0.5f, 0.5f, 0.25f, 0.75f);
	DrawDisk(g, base, 1, top, 0.5f, -0.5f, 0.75f, 1.0f);
}

inline void Prim::DrawPrism(gm_graph &g)
{
	int sides = (cur_div_u + num_parts - 1) / num_parts;

	int n = cur_div_v;
	int top = (n > -1) ? n : n + 3;
	top >>= 2;
	int bottom = n - top * 2;
	if (top <= 0)
		top = 1;
	if (bottom <= 0)
		bottom = 1;

	DrawDisk(g, num_parts, sides, top, 0.5f, 0.5f, 0.0f, 0.25f);
	DrawFlatTube(g, num_parts, sides, bottom, 0.5f, 0.5f, 0.25f, 0.75f);
	DrawDisk(g, num_parts, sides, top, 0.5f, -0.5f, 0.75f, 1.0f);
}

inline void Prim::DrawCone(gm_graph &g)
{
	int n = cur_div_v;
	int base = cur_div_u;
	int top = (n > -1) ? n : n + 3;
	top >>= 2;
	int bottom = n - top;
	if (top <= 0)
		top = 1;
	if (bottom <= 0)
		bottom = 1;

	DrawSmoothTube(g, base, 1, bottom, 0.0f, 0.5f, 0.0f, 0.75f);
	DrawDisk(g, base, 1, top, 0.5f, -0.5f, 0.75f, 1.0f);
}

inline void Prim::DrawPyramid(gm_graph &g)
{
	int sides = (cur_div_u + num_parts - 1) / num_parts;

	int n = cur_div_v;
	int top = (n > -1) ? n : n + 3;
	top >>= 2;
	int bottom = n - top;
	if (top <= 0)
		top = 1;
	if (bottom <= 0)
		bottom = 1;

	DrawFlatTube(g, num_parts, sides, bottom, 0.0f, 0.5f, 0.0f, 0.75f);
	DrawDisk(g, num_parts, sides, top, 0.5f, -0.5f, 0.75f, 1.0f);
}

inline void Prim::SetTexture(int texture, int option, int mode)
{
	if (option == 0 || option == EFF_SLOT_LAYER0) {
		int old = tex_handle;
		flags &= ~PRIM_TEX_NONPOW2;
		int changed;
		if (old == texture)
			goto merge;
		if (old) {
			tmRelease(old);
			tex_handle = 0;
		}
		if (texture == 0)
			goto merge;
		tmDuplicate(texture);
		tex_handle = texture;
		changed = 1;
		goto tail;
merge:
		changed = 0;
tail:
		if (changed) {
			if (tmIsNonPow2(texture))
				flags |= PRIM_TEX_NONPOW2;
		}
		return;
	}
	option_state.SetTexture(texture, option, mode);
}

inline int Prim::SetOption(int option, const float &value, unsigned int size)
{
	switch (option) {
	case OPT_DIVL:
		if (div_level != value) {
			div_level = value;
			flags |= PRIM_RECALC_DIV;
		}
		return 1;
	case OPT_DIVU:
		if (div_u != value) {
			div_u = value;
			flags |= PRIM_DIVU_SET | PRIM_RECALC_DIV;
		}
		return 1;
	case OPT_DIVV:
		if (div_v != value) {
			div_v = value;
			flags |= PRIM_DIVV_SET | PRIM_RECALC_DIV;
		}
		return 1;
	}
	return Object::SetOption(option, value, size);
}

inline int Prim::GetOption(int option, int &value, unsigned int size) const
{
	switch (option) {
	case OPT_TYPE:
		value = shape;
		return 1;
	case OPT_PNUM:
		value = num_parts;
		return 1;
	case OPT_FACE:
		value = face;
		return 1;
	case OPT_FADE:
		value = fade;
		return 1;
	case OPT_ANTI:
		value = anti;
		return 1;
	case OPT_LIGT:
		value = option_state.GetLightingMode() & 1;
		return 1;
	case OPT_HDIV:
		value = (int)div_u;
		return 1;
	case OPT_VDIV:
		value = (int)div_v;
		return 1;
	}
	return Object::GetOption(option, value, size);
}

/* Six DrawPlate calls on the faces of a unit cube centred on the origin. */
void Prim::DrawCube(gm_graph &g)
{
	static vec4 p000(-0.5f, 0.5f, -0.5f, 1.0f);
	static vec4 p001(-0.5f, 0.5f, 0.5f, 1.0f);
	static vec4 p010(0.5f, 0.5f, -0.5f, 1.0f);
	static vec4 p011(0.5f, 0.5f, 0.5f, 1.0f);
	static vec4 p100(-0.5f, -0.5f, -0.5f, 1.0f);
	static vec4 p101(-0.5f, -0.5f, 0.5f, 1.0f);
	static vec4 p110(0.5f, -0.5f, -0.5f, 1.0f);
	static vec4 p111(0.5f, -0.5f, 0.5f, 1.0f);
	static vec4 nyp(0.0f, 1.0f, 0.0f, 1.0f);
	static vec4 nyn(0.0f, -1.0f, 0.0f, 1.0f);
	static vec4 nxp(1.0f, 0.0f, 0.0f, 1.0f);
	static vec4 nxn(-1.0f, 0.0f, 0.0f, 1.0f);
	static vec4 nzp(0.0f, 0.0f, 1.0f, 1.0f);
	static vec4 nzn(0.0f, 0.0f, -1.0f, 1.0f);

	DrawPlate(g, cur_div_u, cur_div_v, p000, p001, p010, p011, nyp);
	DrawPlate(g, cur_div_u, cur_div_v, p101, p100, p111, p110, nyn);
	DrawPlate(g, cur_div_u, cur_div_v, p011, p111, p010, p110, nxp);
	DrawPlate(g, cur_div_u, cur_div_v, p000, p100, p001, p101, nxn);
	DrawPlate(g, cur_div_u, cur_div_v, p001, p101, p011, p111, nzp);
	DrawPlate(g, cur_div_u, cur_div_v, p010, p110, p000, p100, nzn);
}

/* Ball of radius 0.5: (cur_div_u+1) x cur_div_v pairs of vertices, a buffer
   row is 2*(cur_div_u+1) long; the last meridian reuses theta=0 to close the
   surface. The normal is twice the position; u follows theta (around the
   axis), v follows phi (pole to pole). */
void Prim::DrawBall(gm_graph &g)
{
	gmVertexPNT *p;
	int rowlen = (cur_div_u + 1) * 2;
	int nvtx = rowlen * cur_div_v;
	float du = 1.0f / (float)cur_div_u;
	float dv = 1.0f / (float)cur_div_v;
	gmVertexPNT *buf = (gmVertexPNT *)__builtin_alloca(nvtx * sizeof(gmVertexPNT));

	float vv = 0.0f;
	p = buf;
	for (int i = 0; i <= cur_div_u; i++) {
		float theta = vv * 6.2831855f;
		if (i == cur_div_u)
			theta = 0.0f;
		float cx = -cosf(theta) * 0.5f;
		float cz = sinf(theta) * 0.5f;

		float uu = 0.0f;
		float x = 0.0f, y = 0.5f, z = 0.0f;
		/* off: byte offset of row j in the interleaved buffer. */
		int off = 0;
#define BALL_VTX(k) (((gmVertexPNT *)((char *)p + off))[k])
		for (int j = 0; j < cur_div_v; j++) {
			BALL_VTX(0).v = uu;
			uu += dv;
			BALL_VTX(0).x = x;
			BALL_VTX(0).y = y;
			BALL_VTX(0).z = z;
			BALL_VTX(0).u = vv;
			BALL_VTX(0).nx = x + x;
			BALL_VTX(0).ny = y + y;
			BALL_VTX(0).nz = z + z;

			float phi = uu * 3.1415927f;
			float sp = sinf(phi);
			float cp = cosf(phi);
			x = cx * sp;
			y = cp * 0.5f;
			z = cz * sp;

			BALL_VTX(1).x = x;
			BALL_VTX(1).y = y;
			BALL_VTX(1).z = z;
			BALL_VTX(1).nx = x + x;
			BALL_VTX(1).ny = y + y;
			BALL_VTX(1).nz = z + z;
			BALL_VTX(1).u = vv;
			BALL_VTX(1).v = uu;

			off += rowlen * sizeof(gmVertexPNT);
		}
#undef BALL_VTX
		vv += du;
		p += 2;
	}

	g.SetArrays(buf, GM_VERTEX_PNT, nvtx, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, rowlen, cur_div_v, 0);
}

void Prim::DrawSquare(gm_graph &g)
{
	static vec4 c0(-0.5f, 0.0f, -0.5f, 1.0f);
	static vec4 c1(-0.5f, 0.0f, 0.5f, 1.0f);
	static vec4 c2(0.5f, 0.0f, -0.5f, 1.0f);
	static vec4 c3(0.5f, 0.0f, 0.5f, 1.0f);
	static vec4 n(0.0f, 1.0f, 0.0f, 1.0f);

	DrawPlate(g, cur_div_u, cur_div_v, c0, c1, c2, c3, n);
}

/* Grid of (cols+1) x (rows+1) vertices on the quad c0..c3 (c0/c1 one edge, c2/c3
   the opposite one), with a constant normal. */
void Prim::DrawPlate(gm_graph &g, int cols, int rows, const vec4 &c0, const vec4 &c1, const vec4 &c2, const vec4 &c3,
    const vec4 &normal)
{
	float du = 1.0f / (float)cols;
	float dv = 1.0f / (float)rows;
	int m = rows + 1;
	int nvtx = cols * m + m;
	gmVertexPNT *buf = (gmVertexPNT *)__builtin_alloca(nvtx * sizeof(gmVertexPNT));
	gmVertexPNT *p = buf;

	vec4 A, B, P;
	float v = 0.0f;
	for (int j = 0; j <= rows; j++) {
		A = interp(c0, c1, v);
		B = interp(c2, c3, v);
		float u = 0.0f;
		for (int i = 0; i <= cols; i++) {
			P = interp(A, B, u);
			p->nx = normal.x;
			p->ny = normal.y;
			p->nz = normal.z;
			p->u = u;
			p->v = v;
			p->x = P.x;
			p->y = P.y;
			p->z = P.z;
			u += du;
			p++;
		}
		v += dv;
	}

	g.SetArrays(buf, GM_VERTEX_PNT, nvtx, 1, 0, 0);
	g.DrawMesh(GM_PRIM_TRIANGLE_STRIP, 0, cols + 1, rows + 1, 0, 0);
}

/* Disk or annulus: `sectors` angular sectors, `subdiv` subdivisions per
   sector, `rings` radial rings; constant `height`, vStart..vEnd range of the
   v coordinate. With height < 0 the disk faces down: the radius shrinks from
   `radius` and the normal is (0,-1,0). Iteration i == 0 only primes cos/sin
   (a sector uses the previous and the current value); the last sector gets
   one more subdivision to close the ring. */
void Prim::DrawDisk(gm_graph &g, int sectors, int subdiv, int rings, float radius, float height, float vStart,
    float vEnd)
{
	float du = 1.0f / (float)sectors;
	float ic = 1.0f / (float)rings;
	float dr = radius * ic;
	float dv = (vEnd - vStart) * ic;
	int rowlen = (sectors * subdiv + 1) * 2;
	int nvtx = rowlen * rings;
	gmVertexPNT *buf = (gmVertexPNT *)__builtin_alloca(nvtx * sizeof(gmVertexPNT));

	gmVertexPNT *p = buf;
	float v = 0.0f;
	float startRadius = 0.0f;
	float ny = 1.0f;
	if (height < 0.0f) {
		startRadius = radius;
		dr = -dr;
		ny = -1.0f;
	}

	float cprev = 0.0f, sprev = 0.0f;
	for (int i = 0; i <= sectors; i++) {
		float theta = v * 6.2831855f;
		if (i == sectors)
			theta = 0.0f;
		float cold = cprev;
		float sold = sprev;
		cprev = -cosf(theta);
		sprev = sinf(theta);
		if (i == 0) {
			v += du;
			continue;
		}
		int nk = subdiv + (i >= sectors);
		for (int k = 0; k < nk; k++) {
			float bf = (float)subdiv;
			float dc = cprev - cold;
			float ds = sprev - sold;
			float vprev = v - du;
			float t = (float)k / bf;
			float cc = dc * t + cold;
			float ss = ds * t + sold;
			float uu = du * t + vprev;
			float vv = vStart;
			float rad = startRadius;
			if (rings > 0) {
				int step = rowlen * 32;
				int m = rings;
				gmVertexPNT *q = p + 1;
				do {
					q[-1].v = vv;
					vv += dv;
					q[-1].y = height;
					q[-1].nx = 0.0f;
					q[-1].ny = ny;
					q[-1].nz = 0.0f;
					q[-1].u = uu;
					q[0].y = height;
					q[0].nx = 0.0f;
					q[0].ny = ny;
					q[0].nz = 0.0f;
					q[0].u = uu;
					q[-1].x = cc * rad;
					q[-1].z = ss * rad;
					rad += dr;
					q[0].v = vv;
					q[0].x = cc * rad;
					q[0].z = ss * rad;
					q = (gmVertexPNT *)((char *)q + step);
				} while (--m);
			}
			p += 2;
		}
		v += du;
	}

	g.SetArrays(buf, GM_VERTEX_PNT, nvtx, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, rowlen, rings, 0);
}

inline int Prim::GetOption(int option, float &value, unsigned int size) const
{
	switch (option) {
	case OPT_DIVL:
		value = div_level;
		return 1;
	case OPT_DIVU:
		value = div_u;
		return 1;
	case OPT_DIVV:
		value = div_v;
		return 1;
	}
	return Object::GetOption(option, value, size);
}

/* Side of a prism or pyramid: `sectors` angular sectors, subdiv+1
   subdivisions per sector, `rings` rings in height (y from 0.5 to -0.5);
   radius at the first/last ring (a cone if they differ), vStart..vEnd range
   of the v coordinate. The normal is constant per facet, the only structural
   difference from DrawSmoothTube. */
void Prim::DrawFlatTube(gm_graph &g, int sectors, int subdiv, int rings, float radiusFirst, float radiusLast,
    float vStart, float vEnd)
{
	float du = 1.0f / (float)sectors;
	float ic = 1.0f / (float)rings;
	float dv = (vEnd - vStart) * ic;
	float dr = (radiusLast - radiusFirst) * ic;
	int rowlen = (subdiv * sectors + sectors) * 2;
	int nvtx = rowlen * rings;
	gmVertexPNT *buf = (gmVertexPNT *)__builtin_alloca(nvtx * sizeof(gmVertexPNT));

	gmVertexPNT *p = buf;
	float v = 0.0f;

	float cprev = 0.0f, sprev = 0.0f;
	for (int i = 0; i <= sectors; i++) {
		float theta = v * 6.2831855f;
		if (i == sectors)
			theta = 0.0f;
		float cold = cprev;
		float sold = sprev;
		cprev = -cosf(theta);
		sprev = sinf(theta);
		if (i == 0) {
			v += du;
			continue;
		}
		for (int k = 0; k <= subdiv; k++) {
			float bf = (float)subdiv;
			float dc = cprev - cold;
			float ds = sprev - sold;
			float vprev = v - du;
			float t = (float)k / bf;
			float cc = dc * t + cold;
			float ss = ds * t + sold;
			float uu = du * t + vprev;
			float yy = 0.5f;
			float vv = vStart;
			float rad = radiusFirst;
			if (rings > 0) {
				int step = rowlen * 32;
				int m = rings;
				gmVertexPNT *q = p + 1;
				do {
					q[-1].y = yy;
					yy -= ic;
					q[-1].v = vv;
					vv += dv;
					q[-1].nx = cprev;
					q[-1].ny = 0.0f;
					q[-1].nz = sprev;
					q[-1].u = uu;
					q[0].nx = cprev;
					q[0].ny = 0.0f;
					q[0].nz = sprev;
					q[0].u = uu;
					q[-1].x = cc * rad;
					q[-1].z = ss * rad;
					rad += dr;
					q[0].y = yy;
					q[0].v = vv;
					q[0].x = cc * rad;
					q[0].z = ss * rad;
					q = (gmVertexPNT *)((char *)q + step);
				} while (--m);
			}
			p += 2;
		}
		v += du;
	}

	g.SetArrays(buf, GM_VERTEX_PNT, nvtx, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, (subdiv + 1) * 2, sectors * rings, 0);
}

/* Cone or cylinder with smooth normals: the per-vertex normal is
   (cprev, radiusLast-radiusFirst, sprev) normalized, with a constant y component and x/z
   interpolated inside the sector. */
void Prim::DrawSmoothTube(gm_graph &g, int sectors, int subdiv, int rings, float radiusFirst, float radiusLast,
    float vStart, float vEnd)
{
	float v = 0.0f;
	float slope = radiusLast - radiusFirst;
	float ic = 1.0f / (float)rings;
	int rowlen = (sectors * subdiv + 1) * 2;
	int nvtx = rowlen * rings;
	gmVertexPNT *buf = (gmVertexPNT *)__builtin_alloca(nvtx * sizeof(gmVertexPNT));
	gmVertexPNT *p = buf;
	float du = 1.0f / (float)sectors;
	float dv = (vEnd - vStart) * ic;
	float dr = slope * ic;
	float inv = 1.0f / sqrtf(slope * slope + 1.0f);
	float ny = slope * inv;

	float cprev = 0.0f, sprev = 0.0f;
	float nx = 0.0f, nz = 0.0f;
	for (int i = 0; i <= sectors; i++) {
		float theta = v * 6.2831855f;
		if (i == sectors)
			theta = 0.0f;
		float sold = sprev;
		float cold = cprev;
		cprev = -cosf(theta);
		sprev = sinf(theta);
		float nxold = nx;
		float nzold = nz;
		nx = cprev * inv;
		nz = sprev * inv;
		if (i == 0) {
			v += du;
			continue;
		}
		int nk = subdiv + (i >= sectors);
		for (int k = 0; k < nk; k++) {
			float bf = (float)subdiv;
			float dnx = nx - nxold;
			float dnz = nz - nzold;
			float dc = cprev - cold;
			float ds = sprev - sold;
			float vprev = v - du;
			float t = (float)k / bf;
			float nnx = dnx * t + nxold;
			float nnz = dnz * t + nzold;
			float cc = dc * t + cold;
			float ss = ds * t + sold;
			float uu = du * t + vprev;
			float yy = 0.5f;
			float vv = vStart;
			float rad = radiusFirst;
			if (rings > 0) {
				int step = rowlen * 32;
				int m = rings;
				gmVertexPNT *q = p + 1;
				do {
					q[-1].y = yy;
					yy -= ic;
					q[-1].v = vv;
					vv += dv;
					q[-1].nx = nnx;
					q[-1].ny = ny;
					q[-1].nz = nnz;
					q[-1].u = uu;
					q[0].nx = nnx;
					q[0].ny = ny;
					q[0].nz = nnz;
					q[0].u = uu;
					q[-1].x = cc * rad;
					q[-1].z = ss * rad;
					rad += dr;
					q[0].y = yy;
					q[0].v = vv;
					q[0].x = cc * rad;
					q[0].z = ss * rad;
					q = (gmVertexPNT *)((char *)q + step);
				} while (--m);
			}
			p += 2;
		}
		v += du;
	}

	g.SetArrays(buf, GM_VERTEX_PNT, nvtx, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, rowlen, rings, 0);
}

int Prim::SetOption(int option, const int &value, unsigned int size)
{
	int v;
	switch (option) {
	case OPT_TYPE: {
		int type = value;
		if (type != shape) {
			if ((unsigned)type >= PRIM_SHAPE_COUNT)
				return 0;
			shape = type;
			flags |= PRIM_REBUILD_LIST;
			if (!(flags & PRIM_DIVU_SET)) {
				div_u = (float)SubdivInfos[type].def_x;
				flags |= PRIM_REBUILD_LIST | PRIM_RECALC_DIV;
			}
			if (!(flags & PRIM_DIVV_SET)) {
				div_v = (float)SubdivInfos[shape].def_y;
				flags |= PRIM_RECALC_DIV;
			}
		}
		return 1;
	}
	case OPT_PNUM:
		v = value;
		if (v <= 2)
			v = 3;
		if (v > 64)
			v = 64;
		if (v != num_parts) {
			num_parts = v;
			flags |= PRIM_REBUILD_LIST;
		}
		return 1;
	case OPT_FACE:
		face = value;
		return 1;
	case OPT_FADE:
		fade = value;
		return 1;
	case OPT_ANTI:
		anti = value;
		return 1;
	case OPT_LIGT:
		option_state.SetLightingMode(value != 0);
		return 1;
	case OPT_HDIV: {
		float f = (float)value;
		if (div_u != f) {
			div_u = f;
			flags |= PRIM_DIVU_SET | PRIM_RECALC_DIV;
		}
		return 1;
	}
	case OPT_VDIV: {
		float f = (float)value;
		if (div_v != f) {
			div_v = f;
			flags |= PRIM_DIVV_SET | PRIM_RECALC_DIV;
		}
		return 1;
	}
	}
	return Object::SetOption(option, value, size);
}
