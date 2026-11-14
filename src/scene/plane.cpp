/*
 * Plane methods: display list building per group of 16 items, the quad
 * styles (flat, gouraud, edge-faded, 9-slice) and the option dispatchers.
 */

#include <vector>
#include "gfx/vec.h"
#include "plane.h"

#include "gfx/gm_graph.h"
#include "gfx/gm_vertex.h"

#include "scenefacade.h"
#include "gfx/tm.h"

#include "scene/optid.h"
#include <math.h>
#include <string.h>
#include "script/parseresult.h"

/* Rotation from three angles, composed Y * X * Z. */
static inline mat4 planeRotYXZ(const vec4 &r)
{
	return mat4_rotate_y(r.y) * mat4_rotate_x(r.x) * mat4_rotate_z(r.z);
}

/* World matrix of an item: the current matrix times the item matrix. */
static inline mat4 planeWorldMatrix(gm_graph &g, const mat4 &m)
{
	return multiply(*g.GetMatrix(GM_MATRIX_MODEL), m);
}

/* Identity matrix and w = 1 row. */
static mat4 s_planeUnit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
static vec4 s_planeRow3(0.0f, 0.0f, 0.0f, 1.0f);

static inline int Clamp(int v, int lo, int hi)
{
	if (v < lo)
		v = lo;
	if (v > hi)
		v = hi;
	return v;
}

/* Set by DrawThis to the auto-crop state (0 or PLANE_AUTO_CROP); BuildShape skips
   the texture crop when it is set. s_shapesBuilt guards the one-time setup
   of the shared vertex buffers. */
static int s_planeAutoCrop = 0;
static int s_shapesBuilt = 0;

/* Each quad style has a default buffer and a scratch buffer used for items
   with a custom quad (PLANE_ITEM_QUAD). */
static gmVertexPNCT s_flatQuadDefault[4] = {
	{ -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 0.0f },
	{ -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 1.0f },
	{ 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 0.0f },
	{ 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 1.0f },
};
static gmVertexPNCT s_edgeFadedTemplate[272];
static gmVertexPNCT s_flatQuadScratch[4];

static gmVertexPNCT s_gouraudQuadDefault[4] = {
	{ -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 0.0f },
	{ -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 1.0f },
	{ 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 0.0f },
	{ 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 1.0f },
};
static gmVertexPNCT s_gouraudQuadScratch[4];

/* Template of s_marginedQuadOutput: three strips of 8 vertices (left,
   middle and right columns of the 9-slice). */
static gmVertexPNCT s_marginedQuadDefault[24] = {
	{ -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 0.0f },
	{ -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 0.0f },
	{ -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 0.0f },
	{ -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 0.0f },
	{ -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 1.0f },
	{ -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 1.0f },
	{ -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 1.0f },
	{ -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 1.0f },
	{ -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 0.0f },
	{ 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 0.0f },
	{ -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 0.0f },
	{ 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 0.0f },
	{ -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 1.0f },
	{ 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 1.0f },
	{ -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 1.0f },
	{ 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 1.0f },
	{ 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 0.0f },
	{ 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 0.0f },
	{ 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 0.0f },
	{ 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 0.0f },
	{ 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 1.0f },
	{ 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 1.0f },
	{ 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 1.0f },
	{ 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 1.0f },
};
static char s_gouraudColorSel[4] = { 0, 2, 1, 3 };

static gmVertexPNCT s_marginedQuadOutput[24];
static gmVertexPNCT s_edgeFadedOutput[272];

/* One vertex of the faded quad: position in [-0.5, 0.5], uv from the
   position, alpha from the ring ramp. */
#define PLANE_FADED_VERTEX(p, px, py, pa)		\
	{						\
		(p)->x = (px);				\
		(p)->y = (py);				\
		(p)->color.c[3] = (pa);		\
		(p)->u = (px) + 0.5f;			\
		(p)->v = 0.5f - (py);			\
	}

/* Static helpers used before their definition. */
static void BuildArcTable(const float *src, int n, int m, float *dc, float *ds);
static void BuildFadedShape(gmVertexPNCT *buf, int rings, int seg, float fade);

Plane::Plane(Object *parent) : Object(parent)
{
	int *os = (int *)&option_state;
	os[4] |= EFF_DIFFUSE | EFF_TEXTURE_CROP;

	flags = PLANE_REBUILD_ALL;
	tex_handle = 0;

	vec4 &ra = def_diffuse;
	def_diffuse.x = 1.0f;
	ra.y = 1.0f;
	ra.z = 1.0f;
	ra.w = 1.0f;

	vec4 &rb = def_crop;
	def_crop.x = 0.0f;
	rb.y = 0.0f;
	rb.z = 1.0f;
	rb.w = 1.0f;

	items.resize(1);
	dirty.resize(1);

	margins[0] = margins[1] = 0.0f;
	margins[2] = margins[3] = 0.0f;
	margins_uv[0] = margins_uv[1] = 0.0f;
	margins_uv[2] = margins_uv[3] = 0.0f;

	if (!s_shapesBuilt) {
		s_shapesBuilt = 1;
		BuildFadedShape(s_edgeFadedTemplate, 2, 8, 0.1f);
		memcpy(s_flatQuadScratch, s_flatQuadDefault, sizeof(s_flatQuadDefault));
		memcpy(s_gouraudQuadScratch, s_gouraudQuadDefault, sizeof(s_gouraudQuadDefault));
		memcpy(s_marginedQuadOutput, s_marginedQuadDefault, sizeof(s_marginedQuadDefault));
		memcpy(s_edgeFadedOutput, s_edgeFadedTemplate, sizeof(s_edgeFadedTemplate));
	}
}

Plane::~Plane()
{
	if (tex_handle)
		tmRelease(tex_handle);
	gm_graph *mgr = SceneGetGraph();
	int i = 0;
	while (i < items.end() - items.begin()) {
		mgr->DeleteList(items[i].display_list);
		i += items[i].list_count;
	}
}

/* Rebuilds the display lists of the dirty groups of PLANE_GROUP_SIZE items (all of them
   when the auto-crop state changes or PLANE_REBUILD_ALL), draws every list and
   refreshes the debug texture when requested. */
void Plane::DrawThis(gm_graph &g)
{
	int crop = option_state.HasAutoCrop() ? PLANE_AUTO_CROP : 0;
	s_planeAutoCrop = crop;
	int d = (flags ^ crop) & PLANE_AUTO_CROP;
	if (d)
		flags = (flags ^ d) | PLANE_REBUILD_ALL;

	/* bit 8 is PLANE_TEX_NONPOW2 */
	g.SetEnableBits(
	    ((unsigned)flags >> 8) & 1 ? (g.fFlagsE | GM_ENABLE_TEXTURE_FIT) : (g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT));
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
	g.SetTexture(tex_handle);

	if (flags & (PLANE_LISTS_DIRTY | PLANE_REBUILD_ALL)) {
		int i = 0;
		int n = items.end() - items.begin();
		int force = flags & PLANE_REBUILD_ALL;
		while (n >= PLANE_GROUP_SIZE) {
			if (dirty[i] | force) {
				BuildShape(g, i * PLANE_GROUP_SIZE, PLANE_GROUP_SIZE);
				dirty[i] = false;
			}
			n -= PLANE_GROUP_SIZE;
			i++;
		}
		if (n > 0) {
			if (dirty[i] | force) {
				BuildShape(g, i * PLANE_GROUP_SIZE, n);
				dirty[i] = false;
			}
		}
		flags &= ~(PLANE_LISTS_DIRTY | PLANE_REBUILD_ALL);
	}

	for (int k = 0; k < (int)(items.end() - items.begin()); k += items[k].list_count)
		DrawItem(g, items[k]);

	if (flags < 0) /* PLANE_DEBUG_TEXTURE */
		UpdateDebugTexture(g);
}

/* Rebuilds the display lists of items [start, start + count): a list stays
   open across items until one with ITEM_TRANSFORM (its own transform) forces a new
   one, and list_count of the first item records how many items the list covers.
   Texture crop, diffuse and blend state are emitted only when they change. */
void Plane::BuildShape(gm_graph &g, int start, int count)
{
	int texApplied = 1;
	int difApplied = 1;
	int lastBlend = -1;
	int pending = 0;
	int listIdx = -1;

	if (count > 0) {
		int off = start * 128;
		int i = start;
		int n = count;
		do {
			item_info &it = *(item_info *)((char *)items.begin() + off);
			int f = it.flags;
			if (f & ITEM_TRANSFORM)
				pending = 2;
			if (pending > 0) {
				if (listIdx >= 0) {
					g.EndList(0);
					items[listIdx].list_count = i - listIdx;
					listIdx = -1;
				}
				pending--;
			}
			if (listIdx < 0) {
				if (it.display_list == 0)
					it.display_list = g.CreateList();
				g.BeginList(it.display_list);
				listIdx = i;
			} else if (it.display_list) {
				g.DeleteList(it.display_list);
				it.display_list = 0;
			}
			int o = it.disp;
			if (o & DISP_VISIBLE) {
				if (!s_planeAutoCrop) {
					if (!(o & (DISP_MIRROR_U | DISP_MIRROR_V))) {
						if (f & PLANE_ITEM_CROP) {
							g.TexCrop(it.crop);
							texApplied = 1;
						} else if (texApplied) {
							g.TexCrop(def_crop);
							texApplied = 0;
						}
					} else {
						vec4 tmp = (f & PLANE_ITEM_CROP) ? it.crop : def_crop;
						if (o & DISP_MIRROR_U) {
							tmp.x = tmp.x + tmp.z;
							tmp.z = -tmp.z;
						}
						if (o & DISP_MIRROR_V) {
							tmp.y = tmp.y + tmp.w;
							tmp.w = -tmp.w;
						}
						g.TexCrop(tmp);
						texApplied = 1;
					}
				}
				if (f & PLANE_ITEM_DIFFUSE) {
					g.Diffuse(it.diffuse);
					difApplied = 1;
				} else if (difApplied) {
					g.Diffuse(def_diffuse);
					difApplied = 0;
				}
				int bl = it.blend;
				if (lastBlend != bl) {
					g.BlendFunc(bl);
					lastBlend = bl;
				}
				DrawItemShape(g, it);
			}
			off += 128;
			i++;
		} while (--n);
	}
	if (listIdx >= 0) {
		g.EndList(0);
		items[listIdx].list_count = start + count - listIdx;
	}
}

/* Draws the display list of an item, inside its own transform (rotation,
   position, scale) when ITEM_TRANSFORM is set. */
void Plane::DrawItem(gm_graph &g, item_info &item)
{
	if (item.flags & ITEM_TRANSFORM) {
		mat4 m = s_planeUnit;
		if (item.flags & ITEM_ROT)
			m = planeRotYXZ(item.rot);
		if (item.flags & ITEM_POS)
			m.m3.Set(item.pos.x, item.pos.y, item.pos.z, 1.0f);
		if (item.flags & ITEM_SCAL) {
			m.m0 *= item.scale;
			m.m1 *= item.scale;
			m.m2 *= item.scale;
		}
		mat4 w = planeWorldMatrix(g, m);
		g.PushMatrix(GM_MATRIX_MODEL, w);
		g.CallList(item.display_list);
		g.PopMatrix(GM_MATRIX_MODEL);
		return;
	}
	g.CallList(item.display_list);
}

void Plane::DrawItemShape(gm_graph &g, item_info &item)
{
	if ((flags & PLANE_STYLE_MASK) == 0) {
		DrawFlatQuad(g, item);
	} else if (flags & PLANE_MARGINS) {
		DrawMarginedQuad(g, item);
	} else if (flags & PLANE_EDGE_FADE) {
		DrawEdgeFadedQuad(g, item);
	} else if (flags & PLANE_GOURAUD) {
		DrawGouraudQuad(g, item);
	}
}

/* Flat quad, scaled into the item quad when PLANE_ITEM_QUAD is set. */
void Plane::DrawFlatQuad(gm_graph &g, item_info &item)
{
	gmVertexPNCT *v = s_flatQuadDefault;
	if (item.flags & PLANE_ITEM_QUAD) {
		v = s_flatQuadScratch;
		int i;
		for (i = 0; i < 4; i++) {
			v[i].x = item.quad.z * ((i & 2) ? 0.5f : -0.5f) + item.quad.x;
			v[i].y = item.quad.w * ((i & 1) ? -0.5f : 0.5f) + item.quad.y;
		}
	}
	g.SetArrays(v, GM_VERTEX_PNCT, 4, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);
}

/* Edge-faded quad: 272 vertices (center, corners, edges) built once by
   BuildFadedShape, scaled into the item quad when PLANE_ITEM_QUAD is set. */
void Plane::DrawEdgeFadedQuad(gm_graph &g, item_info &item)
{
	gmVertexPNCT *buf = s_edgeFadedTemplate;
	int n = 8;
	if (item.flags & PLANE_ITEM_QUAD) {
		gmVertexPNCT *dst = s_edgeFadedOutput;
		int i;
		for (i = 0; i < 272; i++) {
			dst->x = buf->x * item.quad.z + item.quad.x;
			dst->y = buf->y * item.quad.w + item.quad.y;
			buf++;
			dst++;
		}
		buf = s_edgeFadedOutput;
	}
	g.SetArrays(buf, GM_VERTEX_PNCT, 272, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, n * 2, 1, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, n * 2, 8, 2, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, n * 4, 20, 12, 0);
}

void Plane::SetTexture(int texture, int option, int mode)
{
	if (option == 0 || option == EFF_SLOT_LAYER0) {
		int old = tex_handle;
		if (texture == old)
			return;
		flags &= ~PLANE_TEX_NONPOW2;
		int changed;
		if (old == texture) {
			changed = 0;
		} else {
			if (old) {
				tmRelease(old);
				tex_handle = 0;
			}
			if (texture) {
				tmDuplicate(texture);
				tex_handle = texture;
				changed = 1;
			} else {
				changed = 0;
			}
		}
		if (changed) {
			if (tmIsNonPow2(texture))
				flags |= PLANE_TEX_NONPOW2;
		}
		flags |= PLANE_REBUILD_ALL;
		return;
	}
	option_state.SetTexture(texture, option, mode);
}

/* The vec4 options are forwarded to the void * overload. */
int Plane::SetOption(int option, const float &value, unsigned int size)
{
	switch (option) {
	case OPT_SCAL:
	case OPT_POS:
	case OPT_MGN:
	case OPT_ROT:
	case OPT_DIFF:
	case OPT_CROP:
	case OPT_CLIP:
	case OPT_COLR:
	case OPT_MGNT:
		return SetOption(option, &value, size * 4);
	}
	return Object::SetOption(option, value, size);
}

/* Flat quad with one color per corner from corner_colors, mirrored by the 'DISP'
   bits 0x10/0x20. */
void Plane::DrawGouraudQuad(gm_graph &g, item_info &item)
{
	gmVertexPNCT *v = s_gouraudQuadDefault;
	if (item.flags & PLANE_ITEM_QUAD) {
		v = s_gouraudQuadScratch;
		int i;
		for (i = 0; i < 4; i++) {
			v[i].x = item.quad.z * ((i & 2) ? 0.5f : -0.5f) + item.quad.x;
			v[i].y = item.quad.w * ((i & 1) ? -0.5f : 0.5f) + item.quad.y;
		}
	}
	int flip = item.disp;
	int i;
	for (i = 0; i < 4; i++) {
		int sel = s_gouraudColorSel[i];
		if (flip & DISP_MIRROR_U)
			sel ^= 2;
		if (flip & DISP_MIRROR_V)
			sel ^= 1;
		v[i].color.rgba = corner_colors[sel];
	}
	g.SetArrays(v, GM_VERTEX_PNCT, 4, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);
}

/* 9-slice quad: 4 x 4 grid from the position margins margins and the uv
   margins margins_uv, drawn as 3 strips of 8 vertices. */
void Plane::DrawMarginedQuad(gm_graph &g, item_info &item)
{
	float colX[4] = { -0.5f, margins[0] + -0.5f, 0.5f - margins[1], 0.5f };
	float rowY[4] = { 0.5f, 0.5f - margins[2], margins[3] + -0.5f, -0.5f };
	float colU[4] = { 0.0f, margins_uv[0] + 0.0f, 1.0f - margins_uv[1], 1.0f };
	float rowV[4] = { 0.0f, margins_uv[2] + 0.0f, 1.0f - margins_uv[3], 1.0f };

	gmVertexPNCT *base = s_marginedQuadOutput;
	int n = 0;
	int row;
	for (row = 0; row < 3; row++) {
		int col;
		for (col = 0; col < 8; col++) {
			gmVertexPNCT *out = &base[n];
			int cx = row + (col & 1);
			int cy = col / 2;
			out->x = colX[cx];
			out->y = rowY[cy];
			out->u = colU[cx];
			out->v = rowV[cy];
			if (item.flags & PLANE_ITEM_QUAD) {
				out->x = out->x * item.quad.z + item.quad.x;
				out->y = out->y * item.quad.w + item.quad.y;
			}
			n++;
		}
	}
	g.SetArrays(base, GM_VERTEX_PNCT, 24, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 8, 3, 0);
}

/* 'NUM' resizes the item list (0..4096), marking the changed groups dirty
   and copying the last item into the new ones. */
int Plane::SetOption(int option, const int &value, unsigned int size)
{
	switch (option) {
	case OPT_NUM: {
		int n = Clamp(value, 0, 4096);
		int cur = items.end() - items.begin();
		if (n == cur)
			return 1;
		flags |= PLANE_LISTS_DIRTY;
		if (n < cur) {
			gm_graph *g = SceneGetGraph();
			for (int i = n; i < cur; i++)
				g->DeleteList(items[i].display_list);
			items.resize(n);
			dirty.resize((n + PLANE_GROUP_SIZE - 1) / PLANE_GROUP_SIZE, false);
			if ((n & (PLANE_GROUP_SIZE - 1)) != 0)
				dirty[n / PLANE_GROUP_SIZE] = true;
		} else {
			items.resize(n);
			dirty.resize((n + PLANE_GROUP_SIZE - 1) / PLANE_GROUP_SIZE, false);
			int first = cur / PLANE_GROUP_SIZE, last = (n - 1) / PLANE_GROUP_SIZE;
			for (int w = first; w <= last; w++)
				dirty[w] = true;
			if (cur > 0) {
				item_info &src = items[cur - 1];
				for (int i = cur; i < n; i++)
					items[i] = src;
			}
		}
		return 1;
	}
	case OPT_FDEG: {
		int nf = value ? (flags | PLANE_EDGE_FADE) : (flags & ~PLANE_EDGE_FADE);
		if (nf == flags)
			return 1;
		flags = nf | PLANE_REBUILD_ALL;
		return 1;
	}
	case OPT_ALPH:
	case OPT_DISP:
		for (int i = 0; i < (int)(items.end() - items.begin()); i++)
			SetItemOption(option, i, value, size);
		return 1;
	case OPT_COL4:
		return SetOption(OPT_COL4, &value, size * 4);
	case OPT_DBG:
		if (tex_handle)
			tmRelease(tex_handle);
		tex_handle = 0;
		flags |= PLANE_DEBUG_TEXTURE;
		return 1;
	}
	return Object::SetOption(option, value, size);
}

int Plane::SetOption(int option, const String &value)
{
	return Object::SetOption(option, value);
}

int Plane::SetOption(int option, const void *data, unsigned int size)
{
	int n = size >> 2;
	switch (option) {
	case OPT_COL4:
		if (n == 0)
			n = 4;
		ParseIntArray(corner_colors, 4, (const int *)data, n);
		flags |= PLANE_GOURAUD | PLANE_REBUILD_ALL;
		return 1;
	case OPT_MGN:
		if (n == 0)
			n = 4;
		ParseFloatArray(margins, 4, (const float *)data, n);
		flags |= PLANE_MARGINS | PLANE_REBUILD_ALL;
		return 1;
	case OPT_MGNT:
		if (n == 0)
			n = 4;
		ParseFloatArray(margins_uv, 4, (const float *)data, n);
		flags |= PLANE_MARGINS | PLANE_REBUILD_ALL;
		return 1;
	case OPT_DIFF:
	case OPT_COLR: {
		ParseVec4(def_diffuse, (const float *)data, n);
		int i;
		for (i = 0; i < items.end() - items.begin(); i++)
			items[i].flags &= ~PLANE_ITEM_DIFFUSE;
		flags |= PLANE_REBUILD_ALL;
		return 1;
	}
	case OPT_CROP: {
		ParseVec4(def_crop, (const float *)data, n);
		int i;
		for (i = 0; i < items.end() - items.begin(); i++)
			items[i].flags &= ~PLANE_ITEM_CROP;
		flags |= PLANE_REBUILD_ALL;
		return 1;
	}
	case OPT_POS:
	case OPT_ROT:
	case OPT_SCAL:
	case OPT_CLIP: {
		int i;
		for (i = 0; i < items.end() - items.begin(); i++)
			SetItemOption(option, i, data, size);
		return 1;
	}
	}
	return Object::SetOption(option, data, size);
}

int Plane::SetItemOption(int option, int item, const int &value, unsigned int size)
{
	if (item < 0 || item >= items.end() - items.begin())
		return 0;
	{
		item_info &it = items[item];
		switch (option) {
		case OPT_DISP:
			if (it.disp != value) {
				it.disp = value;
				flags |= PLANE_LISTS_DIRTY;
				dirty[item / PLANE_GROUP_SIZE] = true;
			}
			return 1;
		case OPT_ALPH:
			if (it.blend != value) {
				it.blend = value;
				flags |= PLANE_LISTS_DIRTY;
				dirty[item / PLANE_GROUP_SIZE] = true;
			}
			return 1;
		}
		return Object::SetItemOption(option, item, value, size);
	}
}

int Plane::SetItemOption(int option, int item, const float &value, unsigned int size)
{
	switch (option) {
	case OPT_SCAL:
	case OPT_ROT:
	case OPT_DIFF:
	case OPT_POS:
	case OPT_CROP:
	case OPT_CLIP:
	case OPT_COLR:
		return SetItemOption(option, item, &value, size * 4);
	}
	return Object::SetItemOption(option, item, value, size);
}

/* Per-item vec4 options; each marks the item group dirty. 'POS' and 'ROT'
   set their flags bit only when non-zero, 'SCAL' only when not 1. */
int Plane::SetItemOption(int option, int item, const void *data, unsigned int size)
{
	if (item < 0 || item >= (int)(items.end() - items.begin()))
		return 0;
	const float *f = (const float *)data;
	{
		item_info *p = items.begin() + item;
		int n = size >> 2;
		switch (option) {
		case OPT_DIFF:
		case OPT_COLR:
			ParseVec4(p->diffuse, f, n);
			p->flags |= PLANE_ITEM_DIFFUSE;
			flags |= PLANE_LISTS_DIRTY;
			dirty[item / PLANE_GROUP_SIZE] = true;
			return 1;
		case OPT_CLIP:
			ParseVec4(p->quad, f, n);
			p->quad.x = p->quad.x + p->quad.z * 0.5f - 0.5f;
			p->quad.y = -p->quad.y - p->quad.w * 0.5f + 0.5f;
			p->flags |= PLANE_ITEM_QUAD;
			flags |= PLANE_LISTS_DIRTY;
			dirty[item / PLANE_GROUP_SIZE] = true;
			return 1;
		case OPT_CROP:
			ParseVec4(p->crop, f, n);
			p->flags |= PLANE_ITEM_CROP;
			flags |= PLANE_LISTS_DIRTY;
			dirty[item / PLANE_GROUP_SIZE] = true;
			return 1;
		case OPT_POS:
			ParseVec4(p->pos, f, n);
			if (p->pos.z != 0.0f || p->pos.y != 0.0f || p->pos.x != 0.0f)
				p->flags |= ITEM_POS;
			else
				p->flags &= ~ITEM_POS;
			flags |= PLANE_LISTS_DIRTY;
			dirty[item / PLANE_GROUP_SIZE] = true;
			return 1;
		case OPT_ROT:
			ParseVec4(p->rot, f, n);
			if (p->rot.z != 0.0f || p->rot.y != 0.0f || p->rot.x != 0.0f)
				p->flags |= ITEM_ROT;
			else
				p->flags &= ~ITEM_ROT;
			flags |= PLANE_LISTS_DIRTY;
			dirty[item / PLANE_GROUP_SIZE] = true;
			return 1;
		case OPT_SCAL:
			ParseVec4(p->scale, f, n);
			if (p->scale.x != 1.0f || p->scale.y != 1.0f || p->scale.z != 1.0f)
				p->flags |= ITEM_SCAL;
			else
				p->flags &= ~ITEM_SCAL;
			flags |= PLANE_LISTS_DIRTY;
			dirty[item / PLANE_GROUP_SIZE] = true;
			return 1;
		default:
			goto other;
		}
	}
other:
	return Object::SetItemOption(option, item, data, size);
}

int Plane::GetOption(int option, int &value, unsigned int size) const
{
	switch (option) {
	case OPT_NUM:
		value = items.end() - items.begin();
		return 1;
	case OPT_FDEG:
		value = (unsigned(flags) >> 10) & 1; /* PLANE_EDGE_FADE */
		return 1;
	case OPT_ALPH:
	case OPT_DISP:
		return GetItemOption(option, 0, value, size);
	case OPT_COL4:
		return GetOption(OPT_COL4, &value, size * 4);
	}
	return Object::GetOption(option, value, size);
}

int Plane::GetOption(int option, float &value, unsigned int size) const
{
	switch (option) {
	case OPT_SCAL:
	case OPT_POS:
	case OPT_MGN:
	case OPT_ROT:
	case OPT_DIFF:
	case OPT_CROP:
	case OPT_CLIP:
	case OPT_COLR:
	case OPT_MGNT:
		return GetOption(option, &value, size * 4);
	}
	return Object::GetOption(option, value, size);
}

int Plane::GetOption(int option, String &value) const
{
	return Object::GetOption(option, value);
}

int Plane::GetOption(int option, void *data, unsigned int size) const
{
	int n = size >> 2;
	switch (option) {
	case OPT_COL4:
		if (n == 0)
			n = 4;
		ResultIntArray(corner_colors, 4, (int *)data, n);
		return 1;
	case OPT_MGN:
		if (n == 0)
			n = 4;
		ResultFloatArray(margins, 4, (float *)data, n);
		return 1;
	case OPT_MGNT:
		if (n == 0)
			n = 4;
		ResultFloatArray(margins_uv, 4, (float *)data, n);
		return 1;
	case OPT_SCAL:
	case OPT_POS:
	case OPT_ROT:
	case OPT_DIFF:
	case OPT_CROP:
	case OPT_CLIP:
	case OPT_COLR:
		return GetItemOption(option, 0, data, size);
	}
	return Object::GetOption(option, data, size);
}

int Plane::GetItemOption(int option, int item, int &value, unsigned int size) const
{
	if (item < 0 || item >= (int)(items.end() - items.begin()))
		return 0;
	{
		const item_info *p = items.begin() + item;
		int v;
		switch (option) {
		case OPT_DISP:
			v = p->disp;
			value = v;
			return 1;
		case OPT_ALPH:
			v = p->blend;
			value = v;
			return 1;
		default:
			goto other;
		}
	}
other:
	return Object::GetItemOption(option, item, value, size);
}

/* Float options forward to the void * overload. */
int Plane::GetItemOption(int option, int item, float &value, unsigned int size) const
{
	switch (option) {
	case OPT_SCAL:
	case OPT_ROT:
	case OPT_DIFF:
	case OPT_POS:
	case OPT_CROP:
	case OPT_CLIP:
	case OPT_COLR:
		return GetItemOption(option, item, &value, size * 4);
	}
	return Object::GetItemOption(option, item, value, size);
}

int Plane::GetItemOption(int option, int item, void *data, unsigned int size) const
{
	if (item < 0 || item >= (int)(items.end() - items.begin()))
		return 0;
	float *f = (float *)data;
	{
		const item_info *p = items.begin() + item;
		int n = size >> 2;
		switch (option) {
		case OPT_DIFF:
		case OPT_COLR:
			if (p->flags & PLANE_ITEM_DIFFUSE)
				ResultVec4(p->diffuse, f, n);
			else
				ResultVec4(def_diffuse, f, n);
			return 1;
		case OPT_CROP:
			if (p->flags & PLANE_ITEM_CROP)
				ResultVec4(p->crop, f, n);
			else
				ResultVec4(def_crop, f, n);
			return 1;
		case OPT_CLIP: {
			vec4 tmp;
			tmp.x = p->quad.x - p->quad.z * 0.5f + 0.5f;
			tmp.y = -p->quad.y - p->quad.w * 0.5f + 0.5f;
			tmp.z = p->quad.z;
			tmp.w = p->quad.w;
			ResultVec4(tmp, f, n);
			return 1;
		}
		case OPT_ROT:
			ResultVec4(p->rot, f, n);
			return 1;
		case OPT_SCAL:
			ResultVec4(p->scale, f, n);
			return 1;
		case OPT_POS:
			ResultVec4(p->pos, f, n);
			return 1;
		default:
			goto other;
		}
	}
other:
	return Object::GetItemOption(option, item, data, size);
}

/* Creates the debug texture on first use (512 x 1, two-color CLUT), then
   uploads the current packet buffer contents into it. */
void Plane::UpdateDebugTexture(gm_graph &g)
{
	unsigned int clut[256];
	char dbuf[256];
	char hbuf[256];

	int tex = tex_handle;
	if (!tex) {
		clut[0] = 0x80000033;
		clut[1] = 0x80000066;
		int mgr = g.GetTextureManager();
		tex = tmCreateTexture(mgr, 512, 1, TM_FLUSH | PS2_GS_PSMT8);
		int cl = tmCreateClut(mgr, 19, 0);
		tmSetClut(tex, cl);
		tmUpdateClut(cl, 1, clut);
		tmRelease(cl);
		tex_handle = tex;
		if (!tex)
			return;
	}
	g.fPacket.render_dbuf(dbuf, 256);
	g.fPacket.render_heap(hbuf, 256);
	int w = tmGetWidth(tex);
	int h = tmGetHeight(tex);
	int fmt = tmGetPixelFormat(tex);
	tmUpdateTexture(tex, w, h, fmt, dbuf);
}

Plane::item_info::item_info()
{
	list_count = 1;
	disp = DISP_VISIBLE;
	blend = GM_BLEND_ALPHA;
	flags = 0;
	display_list = 0;
	diffuse.Set(1.0f, 1.0f, 1.0f, 1.0f);
	quad.Set(0.0f, 0.0f, 1.0f, 1.0f);
	crop.Set(0.0f, 0.0f, 1.0f, 1.0f);
	pos.Set(0.0f, 0.0f, 0.0f, 1.0f);
	rot.Set(0.0f, 0.0f, 0.0f, 1.0f);
	scale.Set(1.0f, 1.0f, 1.0f, 1.0f);
}

/* Does not copy display_list and list_count. */
Plane::item_info &Plane::item_info::operator=(const item_info &o)
{
	flags = o.flags;
	disp = o.disp;
	blend = o.blend;
	pos = o.pos;
	rot = o.rot;
	scale = o.scale;
	quad = o.quad;
	crop = o.crop;
	diffuse = o.diffuse;
	return *this;
}

/* Builds the edge-faded quad template: `rings` fade rings across a border
   of width `fade`, `seg` steps per corner arc. */
static void BuildFadedShape(gmVertexPNCT *buf, int rings, int seg, float fade)
{
	int alpha[rings + 2];
	float radius[rings + 2];

	for (int i = 0; i < rings + 2; i++) {
		float t = (float)i / (rings + 1);
		alpha[i] = (int)(sinf(t * 1.5707964f) * 255.0f);
		radius[i] = fade * (1.0f - t);
	}

	/* Vertices: 8 per ring on the edges, ncol x nrow per corner. */
	int ncol = seg + 2;
	int nrow = (rings + 1) * 2;
	int total = (rings + 2) * 8 + ncol * nrow * 4;
	int npts = ncol * nrow;
	int n = (rings + 2) * 2;
	float dc[npts];
	float ds[npts];

	for (int i = 0; i < total; i++) {
		buf[i].z = 0.0f;
		buf[i].color.c[0] = 255;
		buf[i].color.c[1] = 255;
		buf[i].color.c[2] = 255;
		buf[i].nx = 0.0f;
		buf[i].ny = 0.0f;
		buf[i].nz = 1.0f;
	}

	gmVertexPNCT *s = buf;

	/* top and bottom edges */
	{
		gmVertexPNCT *p = s;
		gmVertexPNCT *q = s + n * 2;
		for (int i = 0; i < rings + 2; i++) {
			float t = i * fade / (rings + 1);
			PLANE_FADED_VERTEX(p, -0.5f + t, 0.5f - fade, alpha[i])
			p++;
			q--;
			PLANE_FADED_VERTEX(q, 0.5f - t, -0.5f + fade, alpha[i])
			PLANE_FADED_VERTEX(p, -0.5f + t, -0.5f + fade, alpha[i])
			p++;
			q--;
			PLANE_FADED_VERTEX(q, 0.5f - t, 0.5f - fade, alpha[i])
		}
	}
	s = buf + n * 2;

	/* left and right edges */
	{
		gmVertexPNCT *p = s;
		gmVertexPNCT *q = s + n;
		for (int i = 0; i < rings + 2; i++) {
			float t = i * fade / (rings + 1);
			PLANE_FADED_VERTEX(p, 0.5f - fade, 0.5f - t, alpha[i])
			p++;
			PLANE_FADED_VERTEX(q, -0.5f + fade, -0.5f + t, alpha[i])
			q++;
			PLANE_FADED_VERTEX(p, -0.5f + fade, 0.5f - t, alpha[i])
			p++;
			PLANE_FADED_VERTEX(q, 0.5f - fade, -0.5f + t, alpha[i])
			q++;
		}
	}
	s += n * 2;

	/* the four corners: quarter rings around the inner square */
	BuildArcTable(radius, rings, seg, dc, ds);
	const float *pc = dc;
	const float *ps = ds;
	gmVertexPNCT *p = s;
	gmVertexPNCT *q = s + npts * 2;
	gmVertexPNCT *r = s + npts * 3;
	gmVertexPNCT *w = s + npts * 3;
	for (int i = 0; i < rings + 1; i++) {
		for (int j = 0; j < seg + 2; j++) {
			float c0 = *pc++;
			float s0 = *ps++;
			float c1 = *pc++;
			float s1 = *ps++;
			PLANE_FADED_VERTEX(p, -0.5f + fade - c0, 0.5f - fade + s0, alpha[i])
			p++;
			PLANE_FADED_VERTEX(p, -0.5f + fade - c1, 0.5f - fade + s1, alpha[i + 1])
			p++;
			PLANE_FADED_VERTEX(w, 0.5f - fade + c0, -0.5f + fade - s0, alpha[i])
			w++;
			PLANE_FADED_VERTEX(w, 0.5f - fade + c1, -0.5f + fade - s1, alpha[i + 1])
			w++;
			q--;
			PLANE_FADED_VERTEX(q, -0.5f + fade - c1, -0.5f + fade - s1, alpha[i + 1])
			q--;
			PLANE_FADED_VERTEX(q, -0.5f + fade - c0, -0.5f + fade - s0, alpha[i])
			r--;
			PLANE_FADED_VERTEX(r, 0.5f - fade + c1, 0.5f - fade + s1, alpha[i + 1])
			r--;
			PLANE_FADED_VERTEX(r, 0.5f - fade + c0, 0.5f - fade + s0, alpha[i])
		}
	}
}

/* Sweeps a quarter turn: for each of the n + 1 pairs in `src` writes m + 2
   columns into the cosine and sine tables; column 0 is the value itself,
   columns 1..m are scaled by cos/sin of (j << 14) / (m + 1) (16384 = 90
   degrees), the last column is the value fully on the sine. */
static void BuildArcTable(const float *src, int n, int m, float *dc, float *ds)
{
	float *pc;
	float *ps;
	int i, j;

	pc = dc;
	ps = ds;
	for (i = 0; i < n + 1; i++) {
		pc[0] = src[i];
		pc[1] = src[i + 1];
		pc += (m + 2) * 2;
		ps[0] = 0.0f;
		ps[1] = 0.0f;
		ps += (m + 2) * 2;
	}

	dc += 2;
	ds += 2;
	for (j = 1; j < m + 1; j++) {
		pc = dc;
		ps = ds;
		for (i = 0; i < n + 1; i++) {
			int a = (j << 14) / (m + 1);

			pc[0] = src[i] * gmGetTableCos(a);
			pc[1] = src[i + 1] * gmGetTableCos(a);
			ps[0] = src[i] * gmGetTableSin(a);
			ps[1] = src[i + 1] * gmGetTableSin(a);
			pc += (m + 2) * 2;
			ps += (m + 2) * 2;
		}
		dc += 2;
		ds += 2;
	}

	pc = dc;
	ps = ds;
	for (i = 0; i < n + 1; i++) {
		pc[0] = 0.0f;
		pc[1] = 0.0f;
		pc += (m + 2) * 2;
		ps[0] = src[i];
		ps[1] = src[i + 1];
		ps += (m + 2) * 2;
	}
}
