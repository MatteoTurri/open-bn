/*
 * Crystal methods: the color buffer passes (refraction, reflection, mirror
 * cube, gloss map, highlight), the shape display lists and the options.
 */

#include <iostream.h>
#include "gfx/vec.h"
#include "crystal.h"
#include "gfx/gm_vertex.h"

#include "gfx/tm.h"
#include "scenefacade.h"
#include "scene/optid.h"

/* Reference size of the screen fade, set by DrawThis from the local scale. */
static float g_crystalFadeRange;

/* Width of the highlight border, in clip coordinates (DrawHilight). */
static float g_crystalHilightWidth;

/* Current fade of the crystal (GetFadeFactor), set by DrawThis. */
static float g_crystalReflectScale;

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

/* Offsets of the shapes: the cube itself and its six neighbours (DrawShape, DefineShape). */
static vec4 g_crystalShapeSize[CRYSTAL_SHAPE_COUNT] = {
	vec4(0.0f, 0.0f, 0.0f, 1.0f),
	vec4(0.0f, 0.5f, 0.0f, 1.0f),
	vec4(0.0f, -0.5f, 0.0f, 1.0f),
	vec4(0.5f, 0.0f, 0.0f, 1.0f),
	vec4(-0.5f, 0.0f, 0.0f, 1.0f),
	vec4(0.0f, 0.0f, 0.5f, 1.0f),
	vec4(0.0f, 0.0f, -0.5f, 1.0f),
};

/* Work rectangles of the color buffer passes (crystal_calc_screen_rect). */
static vec4 g_crystalBlendRect;
static vec4 g_crystalTexCropRect;
static vec4 g_crystalScreenRect;

/* The cube (vertex format 19), copied whole by DefineShape. */
struct crystal_shape {
	gmVertexPNT v[24];
};

/* The 24 vertices of the unit cube, four per face. */
static crystal_shape g_crystalVertexData = { {
    { -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f },
    { -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },
    { 0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f },
    { 0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f },
    { -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f },
    { -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f },
    { 0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f },
    { 0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f },
    { 0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f },
    { -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    { -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f },
    { -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f },
    { -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f },
    { -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
    { -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f },
    { 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f },
    { 0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { 0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f },
    { 0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f },
    { -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f },
    { -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f },
} };
/* Mirror cube faces seen as visible (bit i = face i + 1): set by DrawMirrorCube, read by DrawHilight. */
static int g_crystalMirrorMask = 0;
/* The 12 cube edges: face A, face B, vertex A, vertex B (see DrawHilight). */
/* clang-format off */
static signed char g_crystalEdges[48] = {
	0, 2, 2, 3,
	0, 3, 0, 1,
	0, 4, 1, 3,
	0, 5, 0, 2,
	1, 2, 6, 7,
	1, 3, 4, 5,
	1, 4, 4, 6,
	1, 5, 5, 7,
	2, 4, 3, 6,
	4, 3, 1, 4,
	3, 5, 0, 5,
	5, 2, 2, 7,
};
/* clang-format on */
/* The highlight strip (9 vertices); elements 6-8 double as work variables: previous, current, projected center. */
static gmVertexPT g_crystalHilightVerts[9] = {
	{ 0.0f, 0.0f, 0.0f, 0.5f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 0.5f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 0.5f, 0.5f },
	{ 0.0f, 0.0f, 0.0f, 0.5f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 0.5f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 0.5f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 0.5f, 0.5f },
	{ 0.0f, 0.0f, 0.0f, 0.5f, 0.5f },
	{ 0.0f, 0.0f, 0.0f, 0.5f, 0.0f },
};
/* Crop quads of AddColorBuffer (g_crystalCropQuad) and MixColorBuffer (g_crystalMixQuad). */
static gmVertexPT g_crystalCropQuad[2] = {
	{ -1.0f, 1.0f, 1.0f, 0.0f, 0.0f },
	{ 1.0f, -1.0f, 1.0f, 1.0f, 1.0f },
};
static gmVertexPT g_crystalMixQuad[2] = {
	{ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
	{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f },
};

void Crystal::DefineShape(gm_graph &g, int shape)
{
	crystal_shape buf = g_crystalVertexData;

	const vec4 &sz = g_crystalShapeSize[shape];
	vec4 scaled(sz.x * 2.0f, sz.y * 2.0f, sz.z * 2.0f, sz.w * 2.0f);

	for (int i = 0; i < 24; i++) {
		buf.v[i].x += scaled.x;
		buf.v[i].y += scaled.y;
		buf.v[i].z += scaled.z;
	}

	g.SetArrays(&buf, GM_VERTEX_PNT, 24, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 6, 0);
}

Crystal::Crystal(Object *parent) : Object(parent)
{
	vec4 &c = color;
	int *lists = shape_lists;
	vec4 &cfr0 = refract_color[0];
	vec4 &cfr1 = refract_color[1];
	vec4 &cme0 = reflect_color[0];
	vec4 &cme1 = reflect_color[1];
	vec4 &cmm = mirror_color;
	vec4 &cmg = gloss_color;

	c.x = 0.0f;
	c.y = 0.0f;
	c.z = 0.0f;
	c.w = 0.0f;
	blend_mode = GM_BLEND_ADD;
	flags = 0;

	for (int i = 6; i >= 0; i--)
		lists[i] = 0;

	refract_color[0].x = 1.0f;
	cfr0.y = 1.0f;
	cfr0.z = 1.0f;
	cfr0.w = 1.0f;
	refract_color[1].x = 1.0f;
	cfr1.y = 1.0f;
	cfr1.z = 1.0f;
	cfr1.w = 1.0f;
	refract_factor[0] = 0.2f;
	refract_factor[1] = 0.2f;
	reflect_color[0].x = 1.0f;
	cme0.y = 1.0f;
	cme0.z = 1.0f;
	cme0.w = 1.0f;
	reflect_color[1].x = 1.0f;
	cme1.y = 1.0f;
	cme1.z = 1.0f;
	cme1.w = 1.0f;
	reflect_tex[0] = 0;
	reflect_tex[1] = 0;
	mirror_color.x = 1.0f;
	cmm.y = 1.0f;
	cmm.z = 1.0f;
	cmm.w = 1.0f;
	mirror_tex = 0;
	gloss_color.x = 1.0f;
	cmg.y = 1.0f;
	cmg.z = 1.0f;
	cmg.w = 1.0f;
	gloss_tex = 0;
	fade = 1.0f;
	fade_near = 0.0f;
	hilight_tex = 0;

	gm_graph *g = SceneGetGraph();
	color_buf_a = g->CreateColorBuffer(256, 256, 0);
	color_buf_b = g->CreateColorBuffer(256, 256, 1);

	for (int i = 0; i < CRYSTAL_SHAPE_COUNT; i++) {
		shape_lists[i] = g->CreateList();
		g->BeginList(shape_lists[i]);
		DefineShape(*g, i);
		g->EndList(0);
	}
}

static int crystal_calc_screen_rect(gm_graph &g, float radius, vec4 &blend, vec4 &crop);
static int crystal_solve_quad(float a, float b, float c, float *t0, float *dt);

void Crystal::DrawThis(gm_graph &g)
{
	if (!visible)
		return;

	const vec4 &scale = GetLocalScale();
	g_crystalFadeRange = scale.x;
	if (!crystal_calc_screen_rect(g, scale.x * 0.866f, g_crystalBlendRect, g_crystalTexCropRect))
		return;

	float fade = GetFadeFactor();
	g_crystalReflectScale = fade;
	if (fade < 0.01f)
		return;

	flags = fade < 0.99f ? (flags | CRYSTAL_FADING) : (flags & ~CRYSTAL_FADING);

	g.Color(1.0f, 1.0f, 1.0f, 1.0f);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
	g.TexWrap(GM_WRAP_CLAMP, GM_WRAP_CLAMP);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_CULL_FACE);

	mat4 tmp = multiply(*g.GetMatrix(GM_MATRIX_VIEW), world_mat);
	world_matrix = tmp;
	proj_basis = world_matrix;
	proj_basis.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);

	g.SetColorBuffer(color_buf_a);
	g.Viewport(g_crystalTexCropRect.x * 256.0f, g_crystalTexCropRect.y * 256.0f, g_crystalTexCropRect.z * 256.0f,
	    g_crystalTexCropRect.w * 256.0f);
	g.Clear(0xffff);
	DrawRefraction(g, 0);

	g.SetColorBuffer(color_buf_b);
	g.Clear(0xffff);
	DrawReflection(g, 0);

	g.Viewport(0.0f, 0.0f, 256.0f, 256.0f);
	AddColorBuffer(g, color_buf_a, color_buf_b, false);

	g.SetColorBuffer(color_buf_b);
	g.Viewport(g_crystalTexCropRect.x * 256.0f, g_crystalTexCropRect.y * 256.0f, g_crystalTexCropRect.z * 256.0f,
	    g_crystalTexCropRect.w * 256.0f);
	DrawMirrorCube(g, 0);

	g.Viewport(0.0f, 0.0f, 256.0f, 256.0f);
	AddColorBuffer(g, color_buf_a, color_buf_b, false);

	if (flags & CRYSTAL_FADING) {
		MixColorBuffer(g, color_buf_b, -1, 0);
		g.SetColorBuffer(-1);
		g.Viewport(0.0f, 0.0f, 640.0f, 448.0f);
		DrawRefraction(g, 1);
		g.Color(1.0f, 1.0f, 1.0f, 1.0f - g_crystalReflectScale);
		MixColorBuffer(g, -1, color_buf_b, 1);
	} else {
		g.SetColorBuffer(-1);
		g.Viewport(0.0f, 0.0f, 640.0f, 448.0f);
		DrawRefraction(g, 1);
	}

	g.SetColorBuffer(color_buf_b);
	g.Viewport(g_crystalTexCropRect.x * 256.0f, g_crystalTexCropRect.y * 256.0f, g_crystalTexCropRect.z * 256.0f,
	    g_crystalTexCropRect.w * 256.0f);
	g.Clear(0xffff);
	DrawReflection(g, 1);

	g.Viewport(0.0f, 0.0f, 640.0f, 448.0f);
	AddColorBuffer(g, -1, color_buf_b, true);

	g.Viewport(0.0f, 0.0f, 640.0f, 448.0f);
	DrawGlossMap(g, 1);

	if (color.w != 0.0f)
		DrawHilight(g, 1);

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_CULL_FACE);
	g.SetEnableBits(g.fFlagsE | 0x40);
	g.TexGen(0, GM_TEXGEN_TEXCOORD);
}

inline void Crystal::DrawShape(gm_graph &g, int shape)
{
	if (g.CallList(shape_lists[shape]))
		return;
	g.PushMatrix(GM_MATRIX_MODEL);
	g.Translate(g_crystalShapeSize[shape] + g_crystalShapeSize[shape]);
	g.SetArrays(&g_crystalVertexData, GM_VERTEX_PNT, 24, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 6, 0);
	g.PopMatrix();
}

void Crystal::DrawRefraction(gm_graph &g, int side)
{
	g.FrontFace(side);
	g.Color(refract_color[side]);
	g.SetEnableBits(g.fFlagsE & ~0x40);

	float f = refract_factor[side];

	if (side == 0) {
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE_FIT);
		g.SetTexture(-1);
		g.TexGen(4, GM_TEXGEN_NORMAL);

		mat4 m(f, 0.0f, 0.0f, 0.0f, 0.0f, -f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		g.LoadMatrix(GM_MATRIX_TEXTURE0, m * proj_basis);
	} else {
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT);
		g.SetTexture(color_buf_a);
		g.TexGen(5, GM_TEXGEN_NORMAL);

		float m[4][4];
		m[0][0] = -f;
		m[0][1] = 0.0f;
		m[0][2] = 0.0f;
		m[0][3] = 0.0f;
		m[1][0] = 0.0f;
		m[1][1] = -f;
		m[1][2] = 0.0f;
		m[1][3] = 0.0f;
		m[2][0] = 0.0f;
		m[2][1] = 0.0f;
		m[2][2] = -f;
		m[2][3] = 0.0f;
		m[3][0] = 0.0f;
		m[3][1] = 0.0f;
		m[3][2] = 0.0f;
		m[3][3] = 1.0f;

		g.LoadMatrix(GM_MATRIX_TEXTURE0, *(const mat4 *)m);
		g.TexCrop(g_crystalTexCropRect.x, g_crystalTexCropRect.y, g_crystalTexCropRect.z,
		    g_crystalTexCropRect.w);
	}

	g.SetEnableBits(g.fFlagsE | GM_ENABLE_ANTIALIAS);
	g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGB);
	g.BlendFunc(GM_BLEND_ALPHA);

	DrawShape(g, 0);

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_ANTIALIAS);
	g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGBA);
	g.TexCrop(0.0f, 0.0f, 1.0f, 1.0f);
}

void Crystal::DrawReflection(gm_graph &g, int side)
{
	const int *pt = reflect_tex;
	int tex = pt[side];
	if (!tex)
		return;

	g.FrontFace(side);

	if ((flags & CRYSTAL_FADING) && side == 1) {
		float k = g_crystalReflectScale;
		const vec4 &c = reflect_color[1];
		g.Color(c.x * k, c.y * k, c.z * k, 1.0f);
	} else {
		g.Color(reflect_color[side]);
	}

	g.SetEnableBits(g.fFlagsE | 0x40);
	g.SetTexture(tex);
	g.TexGen(1, GM_TEXGEN_REFLECTION);

	/* Projection matrix of the mirror. */
	static mat4 mirror(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f,
	    1.0f);
	{
		mat4 tmp = multiply(mirror, proj_basis);
		g.LoadMatrix(GM_MATRIX_TEXTURE0, tmp);
	}

	g.SetEnableBits(g.fFlagsE | GM_ENABLE_ANTIALIAS);
	g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGB);
	g.BlendFunc(GM_BLEND_ALPHA);

	DrawShape(g, 0);

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_ANTIALIAS);
	g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGBA);
}

void Crystal::DrawMirrorCube(gm_graph &g, int side)
{
	int tex = mirror_tex;
	if (tex) {
		g.Color(mirror_color);
		g.SetTexture(tex);
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_ANTIALIAS);
		g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGB);
		g.BlendFunc(GM_BLEND_ALPHA);
	}

	mat4 mv = multiply(*g.GetMatrix(GM_MATRIX_VIEW), *g.GetMatrix(GM_MATRIX_MODEL));
	mat4 rot = mv;
	rot.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);

	g_crystalMirrorMask = 0;

	float k = 2.0f / g_crystalFadeRange;

	for (int i = 0; i < 6; i++) {
		vec4 p = mv * g_crystalShapeSize[i + 1];
		vec4 q = rot * g_crystalShapeSize[i + 1];
		if (dot(p, q) <= 0.0f)
			continue;
		g_crystalMirrorMask |= 1 << i;
		if (tex) {
			float t = q.z * k;
			if (t < 0.0f)
				t = -t;
			t = 1.0f - t;
			if (t < 0.0f)
				t = 0.0f;
			if (t > 1.0f)
				t = 1.0f;
			g.Color(t, t, t, 1.0f);
			DrawShape(g, i + 1);
		}
	}

	if (tex) {
		g.Color(1.0f, 1.0f, 1.0f, 1.0f);
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_ANTIALIAS);
		g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGBA);
	}
}

/* Local copies of apply() and project() from vec.h. */
static inline vec4 hi_apply(const mat4 &m, const vec4 &v, float w)
{
	return m.m0 * v.x + m.m1 * v.y + m.m2 * v.z + m.m3 * w;
}

static inline vec4 hi_project(const mat4 &m, const vec4 &v, float w)
{
	vec4 r = hi_apply(m, v, w);
	float rw = r.w;
	if (rw != 0.0f) {
		rw = 1.0f / rw;
		return vec4(r.x * rw, r.y * rw, r.z * rw, rw);
	}
	return r;
}

/* 2D position of a vertex of the highlight strip. */
static inline void hi_setxy(int k, float x, float y)
{
	g_crystalHilightVerts[k].x = x;
	g_crystalHilightVerts[k].y = y;
}

/*
 * Draws a glowing border along the silhouette of the cube: the edges whose two
 * faces differ in visibility (g_crystalMirrorMask) are chained into a strip,
 * offset outwards by g_crystalHilightWidth.
 */
void Crystal::DrawHilight(gm_graph &g, int side)
{
	g.Color(color.x, color.y, color.z, color.w * g_crystalReflectScale);
	g.BlendFunc(blend_mode);

	if (!hilight_tex) {
		g.SetTexture(0);
		DrawShape(g, 0);
		return;
	}

	/* 1) Silhouette edges; `last` starts the chain. */
	int mask = 0;
	int last = -1;
	const signed char *e = g_crystalEdges;
	for (int i = 0; i < 12; i++, e += 4) {
		if (((g_crystalMirrorMask >> e[0]) ^ (g_crystalMirrorMask >> e[1])) & 1) {
			mask |= 1 << i;
			last = i;
		}
	}
	if (last < 0)
		return;

	/* 2) Chain the marked edges sharing the current end point, starting from `last`. */
	int list[16];
	int todo = ~(1 << last) & mask;
	int cur = g_crystalEdges[last * 4 + 3];
	list[0] = g_crystalEdges[last * 4 + 2];
	list[1] = cur;
	int *p = &list[2];
	while (todo) {
		int j;
		for (j = 0; j < 12; j++) {
			if (((todo >> j) & 1) == 0)
				continue;
			if (g_crystalEdges[j * 4 + 2] == cur) {
				todo &= ~(1 << j);
				cur = g_crystalEdges[j * 4 + 3];
				*p++ = cur;
				break;
			}
			if (g_crystalEdges[j * 4 + 3] == cur) {
				cur = g_crystalEdges[j * 4 + 2];
				*p++ = cur;
				todo &= ~(1 << j);
				break;
			}
		}
		if (j == 12)
			break;
	}
	*p++ = list[1];
	int n = p - list;

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_CULL_FACE);
	g.SetTexture(hilight_tex);
	g.TexGen(0, GM_TEXGEN_TEXCOORD);

	const mat4 &mproj = *g.GetMatrix(GM_MATRIX_PROJECTION);
	const mat4 &mview = *g.GetMatrix(GM_MATRIX_VIEW);
	const mat4 &mmodel = *g.GetMatrix(GM_MATRIX_MODEL);
	mat4 mv = mview * mmodel;
	mat4 mvp = mproj * mv;

	g.VertexGen(0);

	gmVertexPT *strip = g_crystalHilightVerts;
	gmVertexPT *vmiter = strip + 2;
	gmVertexPT *vouter = strip + 5;
	vec4 *prev = (vec4 *)&strip[6];
	vec4 *cvert = (vec4 *)&strip[7];
	vec4 *center = (vec4 *)&strip[8];

	static vec4 origin(0.0f, 0.0f, 0.0f, 1.0f);

	*center = hi_project(mvp, origin, 1.0f);
	*prev = hi_project(mvp, *(const vec4 *)&g_crystalVertexData.v[list[0]], 1.0f);

	for (int i = 1; i < n; i++) {
		*cvert = hi_project(mvp, *(const vec4 *)&g_crystalVertexData.v[list[i]], 1.0f);

		/* 2D normal of the projected edge, scaled to the border width and pointing outwards. */
		float nx = prev->y - cvert->y;
		float ny = cvert->x - prev->x;
		float d = sqrtf(nx * nx + ny * ny);
		if (d != 0.0f)
			d = g_crystalHilightWidth / d;
		nx *= d;
		ny *= d;
		if (nx * (prev->x - center->x) + ny * (prev->y - center->y) < 0.0f) {
			nx = -nx;
			ny = -ny;
		}
		vouter->x = cvert->x + nx;
		vouter->y = cvert->y + ny;

		if (i >= 2) {
			/* Miter with the previous edge (vertex 0): the bisector passes through the midpoint of the two offsets. */
			float ex = prev->x + nx;
			float ey = prev->y + ny;
			float mx = (strip->x + ex) * 0.5f;
			float my = (strip->y + ey) * 0.5f;
			float ax = mx - prev->x;
			float ay = my - prev->y;
			vmiter->x = prev->x;
			vmiter->y = prev->y;
			hi_setxy(3, ex, ey);
			hi_setxy(4, ex, ey);
			float e = sqrtf(ax * ax + ay * ay);
			if (e != 0.0f)
				e = g_crystalHilightWidth / e;
			hi_setxy(1, ax * e + vmiter->x, ay * e + vmiter->y);
			g.SetArrays(strip, GM_VERTEX_PT, 9, 1, 0, 0);
			g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 9, 1, 0);
		}

		strip->x = vouter->x;
		strip->y = vouter->y;
		prev->x = cvert->x;
		prev->y = cvert->y;
	}

	g.VertexGen(2);
}

void Crystal::AddColorBuffer(gm_graph &g, int color_buffer, int texture, bool blend)
{
	float x = g_crystalScreenRect.x;
	float z = g_crystalScreenRect.z;
	float y = g_crystalScreenRect.y;
	float xz = x + z;
	float w = g_crystalScreenRect.w;
	float yw = y + w;

	if (x < 0.0f)
		x = 0.0f;
	if (y < 0.0f)
		y = 0.0f;
	if (xz < 0.0f)
		xz = 0.0f;
	if (yw < 0.0f)
		yw = 0.0f;
	if (x > 1.0f)
		x = 1.0f;
	if (y > 1.0f)
		y = 1.0f;
	if (xz > 1.0f)
		xz = 1.0f;
	if (yw > 1.0f)
		yw = 1.0f;

	g_crystalCropQuad[0].u = x;
	g_crystalCropQuad[0].v = y;
	g_crystalCropQuad[1].u = xz;
	g_crystalCropQuad[1].v = yw;

	if (blend) {
		const vec4 &rect = g_crystalBlendRect;
		float bx = rect.x;
		float by = rect.y;
		float bz = rect.z;
		float bw = rect.w;
		x = bx + bz * x;
		y = by + bw * y;
		xz = bx + bz * xz;
		yw = by + bw * yw;
	}

	g_crystalCropQuad[0].x = x * 2.0f + -1.0f;
	g_crystalCropQuad[0].y = 1.0f - y * 2.0f;
	g_crystalCropQuad[1].x = xz * 2.0f + -1.0f;
	g_crystalCropQuad[1].y = 1.0f - yw * 2.0f;

	g.SetColorBuffer(color_buffer);
	g.SetTexture(texture);
	g.TexGen(0, GM_TEXGEN_TEXCOORD);
	g.VertexGen(0);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_CULL_FACE);
	g.BlendFunc(GM_BLEND_ADD | GM_BLEND_FIX_ALPHA);
	g.SetArrays(g_crystalCropQuad, GM_VERTEX_PT, 2, 1, 0, 0);
	g.DrawArrays(GM_PRIM_SPRITE, 0, 2, 1, 0);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_CULL_FACE);
	g.VertexGen(2);
}

void Crystal::MixColorBuffer(gm_graph &g, int a, int b, int c)
{
	float x = g_crystalScreenRect.x;
	float y = g_crystalScreenRect.y;
	float xz = x + g_crystalScreenRect.z;
	float yw = y + g_crystalScreenRect.w;

	if (!c) {
		x -= 0.01f;
		y -= 0.01f;
		xz += 0.02f;
		yw += 0.02f;
	}

	if (x < 0.0f)
		x = 0.0f;
	if (y < 0.0f)
		y = 0.0f;
	if (xz < 0.0f)
		xz = 0.0f;
	if (yw < 0.0f)
		yw = 0.0f;
	if (x > 1.0f)
		x = 1.0f;
	if (y > 1.0f)
		y = 1.0f;
	if (xz > 1.0f)
		xz = 1.0f;
	if (yw > 1.0f)
		yw = 1.0f;

	float bx = g_crystalBlendRect.x;
	float by = g_crystalBlendRect.y;
	float bz = g_crystalBlendRect.z;
	float bw = g_crystalBlendRect.w;
	float u0 = bx + bz * x;
	float v0 = by + bw * y;
	float u1 = bx + bz * xz;
	float v1 = by + bw * yw;

	if (!c) {
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE_FIT);
		g_crystalMixQuad[0].u = u0;
		g_crystalMixQuad[0].v = v0;
		g_crystalMixQuad[1].u = u1;
		g_crystalMixQuad[1].v = v1;
		g_crystalMixQuad[0].x = x * 2.0f + -1.0f;
		g_crystalMixQuad[0].y = 1.0f - y * 2.0f;
		g_crystalMixQuad[1].x = xz * 2.0f + -1.0f;
		g_crystalMixQuad[1].y = 1.0f - yw * 2.0f;
	} else {
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT);
		g_crystalMixQuad[0].u = x;
		g_crystalMixQuad[0].v = y;
		g_crystalMixQuad[1].u = xz;
		g_crystalMixQuad[1].v = yw;
		g_crystalMixQuad[0].x = u0 * 2.0f + -1.0f;
		g_crystalMixQuad[0].y = 1.0f - v0 * 2.0f;
		g_crystalMixQuad[1].x = u1 * 2.0f + -1.0f;
		g_crystalMixQuad[1].y = 1.0f - v1 * 2.0f;
	}

	g.VertexGen(0);
	g.SetColorBuffer(a);
	g.SetTexture(b);
	g.TexGen(0, GM_TEXGEN_TEXCOORD);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_CULL_FACE);
	g.BlendFunc(c != 0);
	g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGB);
	g.SetArrays(g_crystalMixQuad, GM_VERTEX_PT, 2, 1, 0, 0);
	g.DrawArrays(GM_PRIM_SPRITE, 0, 2, 1, 0);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_CULL_FACE);
	g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGBA);
	g.VertexGen(2);
}

void Crystal::SetTexture(int texture, int option, int mode)
{
	switch (option) {
	case 0:
		if (reflect_tex[0] == texture)
			return;
		if (reflect_tex[0]) {
			tmRelease(reflect_tex[0]);
			reflect_tex[0] = 0;
		}
		if (texture) {
			tmDuplicate(texture);
			reflect_tex[0] = texture;
		}
		break;
	case 1:
		if (reflect_tex[1] == texture)
			return;
		if (reflect_tex[1]) {
			tmRelease(reflect_tex[1]);
			reflect_tex[1] = 0;
		}
		if (texture) {
			tmDuplicate(texture);
			reflect_tex[1] = texture;
		}
		break;
	case 2:
		if (mirror_tex == texture)
			return;
		if (mirror_tex) {
			tmRelease(mirror_tex);
			mirror_tex = 0;
		}
		if (texture) {
			tmDuplicate(texture);
			mirror_tex = texture;
		}
		break;
	case 3:
		if (gloss_tex == texture)
			return;
		if (gloss_tex) {
			tmRelease(gloss_tex);
			gloss_tex = 0;
		}
		if (texture) {
			tmDuplicate(texture);
			gloss_tex = texture;
		}
		break;
	case 4:
		if (hilight_tex == texture)
			return;
		if (hilight_tex) {
			tmRelease(hilight_tex);
			hilight_tex = 0;
		}
		if (texture) {
			tmDuplicate(texture);
			hilight_tex = texture;
		}
		break;
	}
}

int Crystal::SetOption(int option, const void *data, unsigned int size)
{
	const vec4 &v = *(const vec4 *)data;
	switch (option) {
	case OPT_RFC1:
	case OPT_RFC0:
	case OPT_RFC: {
		int ch = (unsigned)option >> 24;
		if (ch != '1')
			refract_color[0] = v;
		if (ch != '0')
			refract_color[1] = v;
		return 1;
	}
	case OPT_EMC1:
	case OPT_EMC0:
	case OPT_EMC: {
		int ch = (unsigned)option >> 24;
		if (ch != '1')
			reflect_color[0] = v;
		if (ch != '0')
			reflect_color[1] = v;
		return 1;
	}
	case OPT_MMC:
		mirror_color = v;
		return 1;
	case OPT_GMC:
		gloss_color = v;
		return 1;
	}
	return Object::SetOption(option, data, 0);
}

int Crystal::GetOption(int option, void *data, unsigned int size) const
{
	vec4 &v = *(vec4 *)data;
	switch (option) {
	case OPT_RFC:
		if (size < 16)
			return 0;
		v = refract_color[0];
		return 1;
	case OPT_EMC:
		if (size < 16)
			return 0;
		v = reflect_color[0];
		return 1;
	case OPT_MMC:
		if (size < 16)
			return 0;
		v = mirror_color;
		return 1;
	case OPT_GMC:
		if (size < 16)
			return 0;
		v = gloss_color;
		return 1;
	}
	return Object::GetOption(option, data, size);
}

/* Minimum size of the rectangle in render target units (256 x 256); 0.5714286 = 0.4 / 0.7, 0.7 = 448 / 640. */
#define CRYSTAL_MIN_W 0.4f
#define CRYSTAL_MIN_H 0.5714286f

/*
 * Screen rectangle covered by a sphere of `radius` around the crystal
 * (`blend`) and the matching texture crop of the color buffers (`crop`);
 * returns 0 if the crystal is off screen.
 */
static int crystal_calc_screen_rect(gm_graph &g, float radius, vec4 &blend, vec4 &crop)
{
	vec4 c = apply(*g.GetMatrix(GM_MATRIX_VIEW), g.GetMatrix(GM_MATRIX_MODEL)->m3, 1.0f);
	float x, w, y, h;

	if (!crystal_solve_quad(c.z, c.x, radius, &x, &w))
		return 0;
	if (!crystal_solve_quad(c.z, -c.y, radius, &y, &h))
		return 0;

	const mat4 &vp = *g.GetMatrix(GM_MATRIX_PROJECTION);
	float sx = vp.m0.x * 0.5f;
	float sy = vp.m1.y * 0.5f;
	x = x * sx + 0.5f;
	w = w * sx;
	y = y * sy + 0.5f;
	h = h * sy;

	float bx = x, by = y, bw = w, bh = h;
	if (bw < CRYSTAL_MIN_W)
		bw = CRYSTAL_MIN_W;
	if (bh < CRYSTAL_MIN_H)
		bh = CRYSTAL_MIN_H;
	if (bx < 0.0f) {
		bw += bx;
		bx = 0.0f;
		if (bw < CRYSTAL_MIN_W)
			bw = CRYSTAL_MIN_W;
	}
	if (by < 0.0f) {
		bh += by;
		by = 0.0f;
		if (bh < CRYSTAL_MIN_H)
			bh = CRYSTAL_MIN_H;
	}
	if (1.0f - bw < bx) {
		if (1.0f - CRYSTAL_MIN_W < bx)
			bx = 1.0f - CRYSTAL_MIN_W;
		bw = 1.0f - bx;
	}
	if (1.0f - bh < by) {
		if (1.0f - CRYSTAL_MIN_H < by)
			by = 1.0f - CRYSTAL_MIN_H;
		bh = 1.0f - by;
	}

	blend.x = bx;
	blend.y = by;
	blend.z = bw;
	blend.w = bh;
	crop.z = 1.0f / bw;
	crop.w = 1.0f / bh;
	crop.x = -blend.x * crop.z;
	crop.y = -blend.y * crop.w;

	g_crystalHilightWidth = h * 0.5f;
	g_crystalScreenRect.x = (x - bx) / bw;
	g_crystalScreenRect.y = (y - by) / bh;
	g_crystalScreenRect.z = w / bw;
	g_crystalScreenRect.w = h / bh;
	return 1;
}

Crystal::~Crystal()
{
	if (reflect_tex[0])
		tmRelease(reflect_tex[0]);
	if (reflect_tex[1])
		tmRelease(reflect_tex[1]);
	if (mirror_tex)
		tmRelease(mirror_tex);
	if (gloss_tex)
		tmRelease(gloss_tex);
	if (hilight_tex)
		tmRelease(hilight_tex);

	gm_graph *mgr = SceneGetGraph();
	mgr->DeleteColorBuffer(color_buf_a);
	mgr->DeleteColorBuffer(color_buf_b);

	int idx = 0;
	int cnt = 6;
	do {
		mgr->DeleteList(shape_lists[idx]);
		idx++;
	} while (--cnt >= 0);
}

inline void Crystal::DrawGlossMap(gm_graph &g, int side)
{
	if (!gloss_tex)
		return;

	int tex = gloss_tex;
	/* The alpha is the current fade. */
	g.Color(1.0f, 1.0f, 1.0f, g_crystalReflectScale);
	g.BlendFunc(7);
	g.SetTexture(tex);
	g.TexGen(0, GM_TEXGEN_TEXCOORD);

	DrawShape(g, 0);
}

float Crystal::GetFadeFactor()
{
	float f = fade;
	if (scene)
		f *= scene->GetColor().w;
	float t = (-zsort_value - fade_near) / (g_crystalFadeRange * 0.866f) - 1.0f;
	if (t < 1.0f)
		f *= t;
	if (f < 0.0f)
		f = 0.0f;
	if (f > 1.0f)
		f = 1.0f;
	return f;
}

int Crystal::SetOption(int option, const int &value, unsigned int size)
{
	if (option == OPT_ZBUF) {
		int newFlags = value ? (flags | CRYSTAL_ZBUF) : (flags & ~CRYSTAL_ZBUF);
		if (newFlags == flags)
			return 1;
		flags = newFlags;

		gm_graph *g = SceneGetGraph();
		g->DeleteColorBuffer(color_buf_a);
		g->DeleteColorBuffer(color_buf_b);

		if (!value) {
			color_buf_a = g->CreateColorBuffer(256, 256, 0);
			color_buf_b = g->CreateColorBuffer(256, 256, 1);
		} else {
			color_buf_a = g->CreateColorBuffer(256, 256, 1);
			color_buf_b = g->CreateColorBuffer(256, 256, 1);
			tmSetBaseStatic(color_buf_a, 8960, 0);
			tmSetBaseStatic(color_buf_b, 9984, 0);
		}
		return 1;
	}
	return Object::SetOption(option, value, 1);
}

int Crystal::SetOption(int option, const float &value, unsigned int size)
{
	switch (option) {
	case OPT_RFF1:
	case OPT_RFF:
	case OPT_RFF0: {
		int ch = (unsigned)option >> 24;
		if (ch != '1')
			refract_factor[0] = value;
		if (ch != '0')
			refract_factor[1] = value;
		return 1;
	}
	case OPT_FADE:
		fade = value;
		return 1;
	case OPT_NEAR:
		fade_near = value;
		return 1;
	}
	return Object::SetOption(option, value, 1);
}

int Crystal::GetOption(int option, int &value, unsigned int size) const
{
	switch (option) {
	case OPT_EMT:
		value = reflect_tex[0];
		return 1;
	case OPT_MMT:
		value = mirror_tex;
		return 1;
	case OPT_GMT:
		value = gloss_tex;
		return 1;
	}
	return Object::GetOption(option, value, 1);
}

int Crystal::GetOption(int option, float &value, unsigned int size) const
{
	if (option == OPT_RFF) {
		value = refract_factor[0];
		return 1;
	}
	return Object::GetOption(option, value, 1);
}

/* Intersection of the sphere of radius `c` with the projection plane: *t0 = lower edge, *dt = width; 0 if none. */
static int crystal_solve_quad(float a, float b, float c, float *t0, float *dt)
{
	float ab = a * b;
	float aa = a * a;
	float cc = c * c;
	float qa = aa - cc;
	float qb = b * b - cc;
	float d;

	if (qa <= 0.0f)
		return 0;
	d = ab * ab - qa * qb;
	if (d <= 0.0f)
		return 0;
	d = sqrtf(d);
	qa = 1.0f / qa;
	*t0 = (-ab - d) * qa;
	*dt = (d + d) * qa;
	return 1;
}
