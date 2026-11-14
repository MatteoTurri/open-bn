/*
 * Mesh, Mesh::item_info and Mesh::shape_info methods: options, morph
 * weights, surface evaluation (bilinear patch or cubic B-spline) and drawing.
 */

#pragma implementation "mesh.h"
#include <vector>
#include "gfx/vec.h"
#include "mesh.h"
#include <math.h>
#include "gfx/tm.h"
#include "scene/optid.h"
#include "script/parseresult.h"

/* Identity matrix and w = 1 row. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

/* Set at the start of DrawThis: HasAutoCrop() and the Object debug. */
static int s_meshAutoCrop = 0;   /* read by shape_info::draw */

/* An fvec4 option of item_info, read or written as a vec4. */
static inline vec4 &as_vec4(fvec4 &v)
{
	return *(vec4 *)&v;
}

static inline const vec4 &as_vec4(const fvec4 &v)
{
	return *(const vec4 *)&v;
}
static int s_meshDrawFlag = 0;   /* read by shape_info::fixup_disp_lists */

/* Default shape: a quad of 4 points (x, y, z). */
static float kDefaultQuad[12] = {
	-0.5f,
	0.5f,
	0.0f,
	0.5f,
	0.5f,
	0.0f,
	-0.5f,
	-0.5f,
	0.0f,
	0.5f,
	-0.5f,
	0.0f,
};

Mesh::Mesh(Object *parent) : Object(parent)
{
	/* EffOptionState::always_mask */
	int *os = (int *)&option_state;
	os[4] |= EFF_TEXTURE_CROP | EFF_DIFFUSE;
	flags = 0;
	tex_handle = 0;
	items.resize(1);
	items[0].set_master_shape(&shape);
	shape.set_positions(kDefaultQuad, 12, 0, 0);
	pmod = 0;
	pidx = 0;
	face = 0;
	fade = 0;
	anti = 0;
}

Mesh::~Mesh()
{
	if (tex_handle)
		tmRelease(tex_handle);
}

void Mesh::DrawThis(gm_graph &g)
{
	s_meshAutoCrop = option_state.HasAutoCrop();
	s_meshDrawFlag = debug;

	/* bit 9 is MESH_TEX_NONPOW2 */
	g.SetEnableBits(
	    ((unsigned)flags >> 9) & 1 ? (g.fFlagsE | GM_ENABLE_TEXTURE_FIT) : (g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT));
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
	g.SetTexture(tex_handle);
	g.SetVertexShader(-3);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_VERTEX_BLEND);

	if (flags & MESH_SHAPE_CHANGED)
		fixup_shape_sharing();

	if (anti) {
		int doit = (anti == ANTI_ALWAYS);
		const vec4 &c = GetWorldColor();
		if (c.w > 0.99f) {
			g.Color(c.x, c.y, c.z, 1.0f);
			doit = 1;
		}
		if (doit) {
			int save = g.fFlagsE;
			int e = save | GM_ENABLE_ANTIALIAS;
			if (face) {
				e |= GM_ENABLE_CULL_FACE | GM_ENABLE_FRONT_FACE;
				if (face == FACE_BACK)
					e &= ~GM_ENABLE_FRONT_FACE;
			}
			g.SetEnableBits(e);
			DrawItems(g);
			g.Color(GetWorldColor());
			g.SetEnableBits(save);
		}
	}

	if (fade && (fade == FADE_ALWAYS || GetWorldColor().w < 0.99f)) {
		g.ColorMask(0);
		DrawItems(g);
		g.ColorMask(option_state.GetColorMask());
	}

	if (face) {
		int save = g.fFlagsE;
		int e2 = save | GM_ENABLE_CULL_FACE;
		if (face & FACE_BACK) {
			g.SetEnableBits(e2 & ~GM_ENABLE_FRONT_FACE);
			DrawItems(g);
		}
		if (face & FACE_FRONT) {
			g.SetEnableBits(save | GM_ENABLE_CULL_FACE | GM_ENABLE_FRONT_FACE);
			DrawItems(g);
		}
		g.SetEnableBits(save);
	} else {
		DrawItems(g);
	}

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_VERTEX_BLEND);
	g.PatchClip(0.0f, 0.0f, 1.0f, 1.0f);
	g.PatchCrop(0.0f, 0.0f, 1.0f, 1.0f);
}

int Mesh::SetOption(int option, const int &value, unsigned int size)
{
	switch (option) {
	case OPT_NUM: {
		int n = value;
		if (n < 0)
			n = 0;
		if (n > 256)
			n = 256;
		int cur = items.end() - items.begin();
		if (n == cur)
			return 1;
		if (n < cur) {
			items.resize(n);
		} else if (cur == 0) {
			items.resize(n);
			for (int i = 0; i < n; i++)
				items[i].set_master_shape(&shape);
		} else {
			item_info temp(items.back());
			temp.free_shape();
			for (int i = cur; i < n; i++)
				items.push_back(temp);
		}
		return 1;
	}
	case OPT_PMOD:
		pmod = value;
		return 1;
	case OPT_PIDX:
		pidx = value;
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
	case OPT_DISP:
	case OPT_ALPH:
	case OPT_MMOD:
		/* Applied to every item. */
		for (int i = 0; i < (int)(items.end() - items.begin()); i++)
			SetItemOption(option, i, value, size);
		return 1;
	case OPT_TYPE:
		shape.set_type(value);
		flags |= MESH_SHAPE_CHANGED;
		return 1;
	case OPT_PNUM: {
		int u = value;
		/* 'PNUM' is a (u, v) pair: with size >= 2 value points to two ints. */
		int v = (size >= 2) ? (&value)[1] : value;
		shape.set_point_count(u, v);
		flags |= MESH_SHAPE_CHANGED;
		return 1;
	}
	case OPT_MNUM:
		shape.set_morph_count(value);
		flags |= MESH_SHAPE_CHANGED;
		return 1;
	case OPT_DEBG:
		debug = (value != 0);
		for (int i = 0; i < (int)(items.end() - items.begin()); i++)
			items[i].get_shape()->flags |= MESH_LISTS_DIRTY;
		return 1;
	default:
		return Object::SetOption(option, value, size);
	}
}

int Mesh::SetOption(int option, const float &value, unsigned int size)
{
	switch (option) {
	case OPT_DIVL:
	case OPT_MTIM: {
		/* Applied to every item. */
		for (int i = 0; i < (int)(items.end() - items.begin()); i++)
			SetItemOption(option, i, value, size);
		return 1;
	}
	case OPT_SUV:
	case OPT_MTRG:
	case OPT_CLIP:
	case OPT_PPOS:
	case OPT_DIVU:
	case OPT_DIVV:
	case OPT_MWGT:
	case OPT_CROP:
	case OPT_SCAL:
	case OPT_DIFF:
	case OPT_POS:
	case OPT_ROT:
		return SetOption(option, &value, size * 4);
	default:
		return Object::SetOption(option, value, size);
	}
}

int Mesh::SetOption(int option, const void *data, unsigned int size)
{
	unsigned int n = size / 4;
	switch (option) {
	case OPT_SCAL:
	case OPT_SUV:
	case OPT_POS:
	case OPT_ROT:
	case OPT_DIFF:
	case OPT_MTRG:
	case OPT_CLIP:
	case OPT_CROP:
	case OPT_DIVU:
	case OPT_MWGT:
	case OPT_DIVV:
		for (int i = 0; i < (int)(items.end() - items.begin()); i++)
			SetItemOption(option, i, data, size);
		return 1;
	case OPT_PPOS:
		shape.set_positions((const float *)data, n, pidx, pmod);
		flags |= MESH_SHAPE_CHANGED;
		return 1;
	default:
		return Object::SetOption(option, data, size);
	}
}

int Mesh::SetItemOption(int option, int item, const int &value, unsigned int size)
{
	if (item < 0)
		return 0;
	int count = items.end() - items.begin();
	if (item >= count)
		return 0;
	item_info &it = items[item];
	switch (option) {
	case OPT_DISP:
		it.disp = value;
		return 1;
	case OPT_ALPH:
		it.blend = value;
		return 1;
	case OPT_MMOD:
		it.morph_mode = value;
		return 1;
	case OPT_TYPE:
		if (flags & MESH_SHAPE_CHANGED)
			fixup_shape_sharing();
		it.alloc_shape()->set_type(value);
		flags |= MESH_ITEM_SHAPES;
		return 1;
	case OPT_PNUM: {
		if (flags & MESH_SHAPE_CHANGED)
			fixup_shape_sharing();
		int u = value;
		/* With size >= 2 the second int follows value. */
		int v = (size >= 2) ? (&value)[1] : value;
		it.alloc_shape()->set_point_count(u, v);
		flags |= MESH_ITEM_SHAPES;
		return 1;
	}
	case OPT_MNUM:
		if (flags & MESH_SHAPE_CHANGED)
			fixup_shape_sharing();
		it.alloc_shape()->set_morph_count(value);
		flags |= MESH_ITEM_SHAPES;
		return 1;
	default:
		return Object::SetItemOption(option, item, value, size);
	}
}

int Mesh::SetItemOption(int option, int item, const float &value, unsigned int size)
{
	if (item < 0)
		return 0;
	int count = items.end() - items.begin();
	if (item >= count)
		return 0;
	item_info &it = items[item];
	switch (option) {
	case OPT_DIVL:
		it.div_level = value;
		return 1;
	case OPT_MTIM:
		it.flags |= MESH_ITEM_MORPH_DIRTY;
		it.morph_time = value;
		if (it.morph_mode == 0)
			it.morph_mode = MORPH_LINEAR;
		return 1;
	case OPT_PPOS:
	case OPT_DIVU:
	case OPT_CLIP:
	case OPT_DIFF:
	case OPT_SCAL:
	case OPT_POS:
	case OPT_DIVV:
	case OPT_MWGT:
	case OPT_CROP:
	case OPT_MTRG:
	case OPT_ROT:
		return SetItemOption(option, item, &value, size * 4);
	default:
		return Object::SetItemOption(option, item, value, size);
	}
}

int Mesh::SetItemOption(int option, int item, const void *data, unsigned int size)
{
	if (item < 0)
		return 0;
	int count = items.end() - items.begin();
	if (item >= count)
		return 0;
	unsigned int n = size / 4;
	item_info &it = items[item];
	const float *f = (const float *)data;
	switch (option) {
	case OPT_SUV:
		ParseFloatArray(&it.surf_u, 2, f, n);
		return 1;
	case OPT_DIFF:
		ParseVec4(as_vec4(it.diffuse), f, n);
		return 1;
	case OPT_CLIP:
		ParseVec4(as_vec4(it.clip), f, n);
		return 1;
	case OPT_POS:
		ParseVec4(as_vec4(it.pos), f, n);
		it.flags |= ITEM_POS;
		return 1;
	case OPT_ROT:
		ParseVec4(as_vec4(it.rot), f, n);
		it.flags |= ITEM_ROT;
		return 1;
	case OPT_SCAL:
		ParseVec4(as_vec4(it.scale), f, n);
		it.flags |= ITEM_SCAL;
		return 1;
	case OPT_DIVU:
		it.div_u.resize(n);
		ParseFloatArray(it.div_u, f, n);
		return 1;
	case OPT_DIVV:
		it.div_v.resize(n);
		ParseFloatArray(it.div_v, f, n);
		return 1;
	case OPT_MWGT:
		it.flags &= ~MESH_ITEM_MORPH_DIRTY;
		ParseFloatArray(it.weights, f, n);
		if (it.morph_mode)
			it.morph_mode = 0;
		return 1;
	case OPT_CROP:
		ParseVec4(as_vec4(it.crop), f, n);
		return 1;
	case OPT_MTRG:
		ParseVec4(as_vec4(it.morph_range), f, n);
		return 1;
	case OPT_PPOS:
		if (flags & MESH_SHAPE_CHANGED)
			fixup_shape_sharing();
		it.alloc_shape()->set_positions(f, n, pidx, pmod);
		flags |= MESH_ITEM_SHAPES;
		return 1;
	default:
		return Object::SetItemOption(option, item, data, size);
	}
}

int Mesh::GetOption(int option, int &value, unsigned int size) const
{
	switch (option) {
	case OPT_NUM:
		value = items.end() - items.begin();
		return 1;
	case OPT_PMOD:
		value = pmod;
		return 1;
	case OPT_PIDX:
		value = pidx;
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
	case OPT_TYPE:
	case OPT_MNUM:
	case OPT_DISP:
	case OPT_PNUM:
	case OPT_ALPH:
	case OPT_MMOD:
		return GetItemOption(option, 0, value, size);
	default:
		return Object::GetOption(option, value, size);
	}
}

int Mesh::GetOption(int option, float &value, unsigned int size) const
{
	switch (option) {
	case OPT_MTIM:
	case OPT_DIVL:
		return GetItemOption(option, 0, value, size);
	case OPT_PPOS:
	case OPT_MWGT:
	case OPT_DIVU:
	case OPT_DIVV:
	case OPT_SPOS:
	case OPT_CROP:
	case OPT_SNOR:
	case OPT_CLIP:
	case OPT_SDV:
	case OPT_MTRG:
	case OPT_SCAL:
	case OPT_DIFF:
	case OPT_ROT:
	case OPT_SDU:
	case OPT_POS:
		/* Forwards to the void * variant with size in bytes. */
		return GetOption(option, &value, size * 4);
	default:
		return Object::GetOption(option, value, size);
	}
}

int Mesh::GetItemOption(int option, int item, int &value, unsigned int size) const
{
	if (item < 0)
		return 0;
	int count = items.end() - items.begin();
	if (item >= count)
		return 0;
	const item_info &it = items[item];
	switch (option) {
	case OPT_DISP:
		value = it.disp;
		return 1;
	case OPT_ALPH:
		value = it.blend;
		return 1;
	case OPT_MMOD:
		value = it.morph_mode;
		return 1;
	case OPT_TYPE:
		value = it.get_shape()->type;
		return 1;
	case OPT_PNUM:
		/* (count_u, count_v): with size >= 2 value points to two ints. */
		value = it.get_shape()->count_u;
		if (size >= 2)
			(&value)[1] = it.get_shape()->count_v;
		return 1;
	case OPT_MNUM:
		value = it.get_shape()->morph_count;
		return 1;
	default:
		return Object::GetItemOption(option, item, value, size);
	}
}

int Mesh::GetItemOption(int option, int item, float &value, unsigned int size) const
{
	if (item < 0)
		return 0;
	int count = items.end() - items.begin();
	if (item >= count)
		return 0;
	const item_info &it = items[item];
	switch (option) {
	case OPT_DIVL:
		value = it.div_level;
		return 1;
	case OPT_MTIM:
		value = it.morph_time;
		return 1;
	case OPT_PPOS:
	case OPT_MWGT:
	case OPT_DIVU:
	case OPT_CROP:
	case OPT_SDV:
	case OPT_MTRG:
	case OPT_SCAL:
	case OPT_ROT:
	case OPT_DIVV:
	case OPT_SPOS:
	case OPT_SNOR:
	case OPT_CLIP:
	case OPT_DIFF:
	case OPT_SDU:
	case OPT_POS:
		return GetItemOption(option, item, &value, size * 4);
	default:
		return Object::GetItemOption(option, item, value, size);
	}
}


int Mesh::GetItemOption(int option, int item, void *data, unsigned int size) const
{
	static vec4 s_defA(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */
	static vec4 s_defB(1.0f, 1.0f, 1.0f, 1.0f);  /* unused, but part of the layout */

	if (item < 0)
		return 0;
	int count = items.end() - items.begin();
	if (item >= count)
		return 0;
	unsigned int n = size / 4;
	const item_info &it = items[item];
	float *f = (float *)data;
	switch (option) {
	case OPT_SPOS: {
		vec4 tmp = it.calc_surface(0);
		ResultVec4(tmp, f, n);
		return 1;
	}
	case OPT_SNOR: {
		vec4 tmp = cross(it.calc_surface(2), it.calc_surface(1));
		ResultVec4(tmp, f, n);
		return 1;
	}
	case OPT_SDU: {
		vec4 tmp = it.calc_surface(1);
		ResultVec4(tmp, f, n);
		return 1;
	}
	case OPT_SDV: {
		vec4 tmp = it.calc_surface(2);
		ResultVec4(tmp, f, n);
		return 1;
	}
	case OPT_CLIP:
		ResultVec4(as_vec4(it.clip), f, n);
		return 1;
	case OPT_ROT:
		ResultVec4(as_vec4(it.rot), f, n);
		return 1;
	case OPT_DIVU:
		ResultFloatArray(it.div_u, f, n);
		return 1;
	case OPT_DIVV:
		ResultFloatArray(it.div_v, f, n);
		return 1;
	case OPT_MWGT:
		/* NOTE: bug: the weights are recomputed on a temporary copy, so the stale
		   weights of the item are returned. */
		if (it.flags & MESH_ITEM_MORPH_DIRTY) {
			item_info tmp(it);
			tmp.set_morph_weights();
			tmp.flags &= ~MESH_ITEM_MORPH_DIRTY;
		}
		ResultFloatArray(it.weights, f, n);
		return 1;
	case OPT_POS:
		ResultVec4(as_vec4(it.pos), f, n);
		return 1;
	case OPT_DIFF:
		ResultVec4(as_vec4(it.diffuse), f, n);
		return 1;
	case OPT_SCAL:
		ResultVec4(as_vec4(it.scale), f, n);
		return 1;
	case OPT_CROP:
		ResultVec4(as_vec4(it.crop), f, n);
		return 1;
	case OPT_MTRG:
		ResultVec4(as_vec4(it.morph_range), f, n);
		return 1;
	case OPT_PPOS:
		it.get_shape()->get_positions(f, n, pidx);
		return 1;
	default:
		return Object::GetItemOption(option, item, data, size);
	}
}

inline void Mesh::DrawItems(gm_graph &g)
{
	for (int i = 0; i < (int)(items.end() - items.begin()); i++)
		items[i].draw(g);
}

inline void Mesh::SetTexture(int texture, int option, int mode)
{
	if (option == 0 || option == EFF_SLOT_LAYER0) {
		int old = tex_handle;
		flags &= ~MESH_TEX_NONPOW2;
		int changed;
		if (old == texture)
			goto same;
		if (old) {
			tmRelease(old);
			tex_handle = 0;
		}
		if (!texture)
			goto same;
		tmDuplicate(texture);
		tex_handle = texture;
		changed = 1;
		goto checkpow2;
same:
		changed = 0;
checkpow2:
		if (changed) {
			if (tmIsNonPow2(texture))
				flags |= MESH_TEX_NONPOW2;
		}
		return;
	}
	option_state.SetTexture(texture, option, mode);
}

inline int Mesh::GetOption(int option, void *data, unsigned int size) const
{
	switch (option) {
	case OPT_PPOS:
	case OPT_MWGT:
	case OPT_DIVU:
	case OPT_DIVV:
	case OPT_SPOS:
	case OPT_CROP:
	case OPT_SNOR:
	case OPT_CLIP:
	case OPT_SDV:
	case OPT_MTRG:
	case OPT_SCAL:
	case OPT_DIFF:
	case OPT_ROT:
	case OPT_SDU:
	case OPT_POS:
		return GetItemOption(option, 0, data, size);
	default:
		return Object::GetOption(option, data, size);
	}
}

inline void Mesh::fixup_shape_sharing()
{
	if (flags & MESH_ITEM_SHAPES) {
		for (int i = 0; i < (int)(items.end() - items.begin()); i++)
			items[i].free_shape();
	}
	flags &= ~(MESH_SHAPE_CHANGED | MESH_ITEM_SHAPES);
}

inline void Mesh::item_info::set_master_shape(shape_info *s)
{
	master_shape = s;
}

inline Mesh::shape_info *Mesh::item_info::get_shape() const
{
	return own_shape ? own_shape : master_shape;
}

inline Mesh::shape_info *Mesh::item_info::alloc_shape()
{
	if (own_shape)
		return own_shape;
	own_shape = new shape_info();
	if (master_shape)
		*own_shape = *master_shape;
	return own_shape;
}

inline void Mesh::item_info::free_shape()
{
	if (own_shape)
		delete own_shape;
	own_shape = 0;
}

inline void Mesh::shape_info::set_type(int t)
{
	if (t != type) {
		type = t;
		flags |= MESH_LISTS_DIRTY | MESH_ARRAYS_DIRTY;
	}
}

inline void Mesh::shape_info::set_point_count(int u, int v)
{
	if (u < 2)
		u = 2;
	if (u > 256)
		u = 256;
	if (v < 2)
		v = 2;
	if (v > 256)
		v = 256;
	if (u != count_u || v != count_v) {
		count_u = u;
		count_v = v;
		flags |= MESH_LISTS_DIRTY | MESH_ARRAYS_DIRTY;
	}
}

inline void Mesh::shape_info::set_morph_count(int n)
{
	if (n < 1)
		n = 1;
	if (n > 16)
		n = 16;
	if (n != morph_count) {
		morph_count = n;
		flags |= MESH_LISTS_DIRTY | MESH_ARRAYS_DIRTY;
	}
}

inline void Mesh::shape_info::get_positions(float *dst, int count, int start)
{
	count = count / 3;
	if (start < 0) {
		count += start;
		dst += (-start) * 3;
		start = 0;
	}
	int avail = (int)(positions.size() / 3u) - start;
	if (avail < count)
		count = avail;
	if (count > 0) {
		const float *s = positions.begin() + start * 3;
		for (int i = 0; i < count; i++) {
			dst[0] = s[0];
			dst[1] = s[1];
			dst[2] = s[2];
			dst += 3;
			s += 3;
		}
	}
}

Mesh::item_info::item_info()
{
	flags = 0;
	master_shape = 0;
	own_shape = 0;
	disp = DISP_VISIBLE;
	blend = GM_BLEND_ALPHA;
	div_u.push_back(4.0f);
	div_v.push_back(4.0f);
	morph_mode = MORPH_LINEAR;
	morph_time = 0.0f;
	div_level = 1.0f;
	morph_range.Set(0.0f, 0.0f, 0.0f, 1.0f);
	diffuse.Set(1.0f, 1.0f, 1.0f, 1.0f);
	clip.Set(0.0f, 0.0f, 1.0f, 1.0f);
	crop.Set(0.0f, 0.0f, 1.0f, 1.0f);
	pos.Set(0.0f, 0.0f, 0.0f, 1.0f);
	rot.Set(0.0f, 0.0f, 0.0f, 1.0f);
	scale.Set(1.0f, 1.0f, 1.0f, 1.0f);
	surf_u = 0.0f;
	surf_v = 0.0f;
}

Mesh::item_info::~item_info()
{
	if (own_shape)
		delete own_shape;
}

Mesh::item_info::item_info(const item_info &o)
{
	own_shape = 0;
	*this = o;
}

Mesh::item_info &Mesh::item_info::operator=(const item_info &o)
{
	if (this == &o)
		return *this;
	flags = o.flags;
	master_shape = o.master_shape;
	if (own_shape)
		delete own_shape;
	own_shape = 0;
	if (o.own_shape)
		own_shape = new shape_info(*o.own_shape);
	disp = o.disp;
	blend = o.blend;
	div_u = o.div_u;
	div_v = o.div_v;
	div_level = o.div_level;
	morph_mode = o.morph_mode;
	morph_time = o.morph_time;
	weights = o.weights;
	morph_range = o.morph_range;
	diffuse = o.diffuse;
	clip = o.clip;
	crop = o.crop;
	pos = o.pos;
	rot = o.rot;
	scale = o.scale;
	return *this;
}

void Mesh::item_info::draw(gm_graph &g)
{
	shape_info *shape = get_shape();
	if (!shape)
		return;
	if (!(disp & DISP_VISIBLE))
		return;
	if (!set_morph_weights())
		return;

	flags &= ~MESH_ITEM_MORPH_DIRTY;
	int n = (int)(weights.end() - weights.begin());
	if (n > 0)
		g.SetMorphWeights(weights.begin(), n);

	g.BlendFunc(blend);
	g.Diffuse(as_vec4(diffuse));
	g.PatchLevel(div_level);

	if (flags & ITEM_TRANSFORM) {
		mat4 m = s_unitMatrix;
		if (flags & ITEM_ROT) {
			const fvec4 &r = rot;
			m = mat4_rotate_y(r.y) * mat4_rotate_x(r.x) * mat4_rotate_z(r.z);
		}
		if (flags & ITEM_POS)
			m.m3.Set(pos.x, pos.y, pos.z, 1.0f);
		if (flags & ITEM_SCAL) {
			m.m0 *= as_vec4(scale);
			m.m1 *= as_vec4(scale);
			m.m2 *= as_vec4(scale);
		}
		mat4 mm = *g.GetMatrix(GM_MATRIX_MODEL) * m;
		g.PushMatrix(GM_MATRIX_MODEL, mm);
		shape->draw(g, *this);
		g.PopMatrix(GM_MATRIX_MODEL);
	} else {
		shape->draw(g, *this);
	}
}

/*
 * Recomputes `weights` (one per morph target) from the time morph_time and
 * the range in morph_range ('MTRG'): t = (time - morph_range.x) * morph_range.w + morph_range.z, with
 * morph_range.y an upper limit (returns 0 when exceeded, if positive). morph_mode is
 * the mode: the low 4 bits pick the interpolation kernel, MORPH_CYCLIC makes
 * the sequence cyclic (otherwise the end targets absorb the weight of the
 * missing neighbor and there are morph_count - 1 segments).
 *   1 = linear, 2 taps; 2 = quadratic B-spline, 3 taps (half-step offset);
 *   3 = cubic B-spline, 4 taps; 6 = quadratic Bezier on pairs (index rounded
 *   to a multiple of 2); 7 = cubic Bezier on triples (multiple of 3);
 *   anything else = all the weight on target 0.
 */
int Mesh::item_info::set_morph_weights()
{
	shape_info *shape = get_shape();
	if (!shape)
		return 0;

	float t = morph_time - morph_range.x;
	if (t < 0.0f)
		return 0;
	t = t * morph_range.w + morph_range.z;
	if (morph_range.y > 0.0f && t > morph_range.y)
		return 0;

	int mc = shape->morph_count;
	if (mc < 2)
		return 1;

	int old = weights.end() - weights.begin();
	if (old < mc) {
		weights.resize(mc);
		if (old == 0)
			weights[0] = 1.0f;
	}
	if (morph_mode) {
		for (int i = 0; i < mc; i++)
			weights[i] = 0.0f;

		int wrap = morph_mode & MORPH_CYCLIC;
		float fl = floorf(t);
		int idx = (int)fl;
		int seg = mc - (wrap ? 0 : 1);
		t = t - fl;
		idx = idx % seg;
		if (idx < 0)
			idx += seg;

		switch (morph_mode & MORPH_KERNEL_MASK) {
		case MORPH_LINEAR:
			weights[idx] = 1.0f - t;
			weights[(idx + 1) % mc] = t;
			break;
		case MORPH_QUAD_BSPLINE: {
			t = t + 0.5f;
			if (t > 1.0f) {
				t = t - 1.0f;
				idx = (idx + 1) % mc;
			}
			float u = 1.0f - t;
			float p = u * t;
			float prev = 0.0f;
			float c = t * t;
			float a = p + p;
			float next = 0.0f;
			float b = u * u;
			float mid = a;
			if (!wrap && idx == 0) {
				mid = mid + b;
			} else {
				prev = b * 0.5f;
				mid = a + prev;
			}
			if (!wrap && idx == mc - 1) {
				mid = mid + c;
			} else {
				mid = mid + c * 0.5f;
				next = c * 0.5f;
			}
			weights[(idx + mc - 1) % mc] = prev;
			weights[idx] = mid;
			weights[(idx + 1) % mc] = next;
			break;
		}
		case MORPH_CUBIC_BSPLINE: {
			float u = 1.0f - t;
			float u2 = u * u;
			float p = u * t;
			float q = u2 * t;
			p = p * t;
			float u3 = u2 * u;
			q = q * 3.0f;
			p = p * 3.0f;
			float m1 = q * 4.0f;
			float m2 = p + p;
			float m3 = q + q;
			float m4 = p * 4.0f;
			float t2 = t * t;
			float mid = m1 + m2;
			float nxt = m3 + m4;
			float prev = 0.0f;
			float t3 = t2 * t;
			float last = 0.0f;
			if (!wrap && idx == 0) {
				mid = mid + u3 * 6.0f;
			} else {
				prev = u3;
				mid = mid + prev * 4.0f;
				nxt = nxt + prev;
			}
			if (!wrap && idx == mc - 2) {
				nxt = nxt + t3 * 6.0f;
			} else {
				nxt = nxt + t3 * 4.0f;
				mid = mid + t3;
				last = t3;
			}
			weights[(idx + mc - 1) % mc] = prev * (1.0f / 6.0f);
			weights[idx] = mid * (1.0f / 6.0f);
			weights[(idx + 1) % mc] = nxt * (1.0f / 6.0f);
			weights[(idx + 2) % mc] = last * (1.0f / 6.0f);
			break;
		}
		case MORPH_QUAD_BEZIER: {
			int r = idx % 2;
			idx = idx - r;
			t = (t + (float)r) * 0.5f;
			float u = 1.0f - t;
			weights[idx] = u * u;
			weights[(idx + 1) % mc] = u * t + u * t;
			weights[(idx + 2) % mc] = t * t;
			break;
		}
		case MORPH_CUBIC_BEZIER: {
			int r = idx % 3;
			idx = idx - r;
			t = (t + (float)r) * (1.0f / 3.0f);
			float u = 1.0f - t;
			float u2 = u * u;
			weights[idx] = u2 * u;
			weights[(idx + 1) % mc] = u2 * t * 3.0f;
			weights[(idx + 2) % mc] = u * t * t * 3.0f;
			weights[(idx + 3) % mc] = t * t * t;
			break;
		}
		default:
			weights[0] = 1.0f;
			break;
		}
	}
	return 1;
}

/* Blend of the morph targets weighted by `weights`; recomputes the weights
   first when MESH_ITEM_MORPH_DIRTY is set. */
vec4 Mesh::item_info::calc_surface(int idx) const
{
	if (flags & MESH_ITEM_MORPH_DIRTY) {
		const_cast<item_info *>(this)->set_morph_weights();
		flags = flags & ~MESH_ITEM_MORPH_DIRTY;
	}

	shape_info *shape = get_shape();
	if (shape->flags & MESH_ARRAYS_DIRTY) {
		if (!shape->fixup_arrays())
			return vec4(0.0f, 0.0f, 0.0f, 0.0f);
		shape->flags &= ~MESH_ARRAYS_DIRTY;
	}

	int count = shape->morph_count;
	int rawCount = (int)(weights.end() - weights.begin());
	if (rawCount < count)
		count = rawCount;

	if (count < 2)
		return shape->calc_surface(surf_u, surf_v, idx, 0);

	vec4 result(0.0f, 0.0f, 0.0f, 0.0f);
	for (int i = 0; i < count; i++) {
		float w = weights[i];
		if (w == 0.0f)
			continue;
		result += shape->calc_surface(surf_u, surf_v, idx, i) * w;
	}
	return result;
}

Mesh::shape_info::shape_info()
{
	flags = MESH_LISTS_DIRTY | MESH_ARRAYS_DIRTY;
	type = MESH_TYPE_BILINEAR;
	count_u = 2;
	count_v = 2;
	morph_count = 1;
}

Mesh::shape_info::~shape_info()
{
	gm_graph *g = SceneGetGraph();
	for (int i = 0; i < (int)(lists.end() - lists.begin()); i++)
		g->DeleteList(lists[i]);
}

Mesh::shape_info::shape_info(const shape_info &o)
{
	int f = o.flags;
	*this = o;
	flags = f;
	gm_graph *g = SceneGetGraph();
	int n = o.lists.end() - o.lists.begin();
	lists.resize(n);
	for (int i = 0; i < n; i++)
		lists[i] = g->DuplicateList(o.lists[i]);
}

Mesh::shape_info &Mesh::shape_info::operator=(const shape_info &o)
{
	if (this != &o) {
		flags = o.flags | MESH_LISTS_DIRTY;
		type = o.type;
		gm_graph *g = SceneGetGraph();
		for (int i = 0; i < (int)(lists.end() - lists.begin()); i++)
			g->DeleteList(lists[i]);
		lists.erase(lists.begin(), lists.end());
		count_u = o.count_u;
		count_v = o.count_v;
		morph_count = o.morph_count;
		control_grid = o.control_grid;
		positions = o.positions;
	}
	return *this;
}

/* Clamps a grid index to [0, hi]. */
static inline int clampi(int v, int hi)
{
	if (v <= 0)
		return 0;
	if (v >= hi)
		return hi;
	return v;
}

/* Clamps a patch coordinate to [0, hi]. */
static inline float clampf(float v, float hi)
{
	if (v <= 0.0f)
		return 0.0f;
	if (v >= hi)
		return hi;
	return v;
}

void Mesh::shape_info::draw(gm_graph &g, const item_info &it)
{
	if (flags & MESH_ARRAYS_DIRTY) {
		if (!fixup_arrays())
			return;
		flags &= ~MESH_ARRAYS_DIRTY;
	}
	if (flags & MESH_LISTS_DIRTY) {
		if (!fixup_disp_lists())
			return;
		flags &= ~MESH_LISTS_DIRTY;
	}

	int fl = it.disp;
	float x = it.clip.x;
	float y = it.clip.y;
	float w = it.clip.z;
	float h = it.clip.w;
	if (w <= 0.0f) {
		if (w == 0.0f)
			return;
		x += w;
		w = -w;
		fl ^= DISP_MIRROR_U;
	}
	if (h <= 0.0f) {
		if (h == 0.0f)
			return;
		y += h;
		h = -h;
		fl ^= DISP_MIRROR_V;
	}
	float x1 = x + w;
	float y1 = y + h;
	if (!s_meshAutoCrop) {
		if (!(fl & (DISP_MIRROR_U | DISP_MIRROR_V))) {
			g.TexCrop(as_vec4(it.crop));
		} else {
			float cx = it.crop.x;
			float cy = it.crop.y;
			float cw = it.crop.z;
			float ch = it.crop.w;
			if (fl & DISP_MIRROR_U) {
				cx += cw;
				cw = -cw;
			}
			if (fl & DISP_MIRROR_V) {
				cy += ch;
				ch = -ch;
			}
			g.TexCrop(cx, cy, cw, ch);
		}
	}
	int row0 = (int)floorf(y);
	int row1 = (int)floorf(y1);
	int col0 = (int)floorf(x);
	int col1 = (int)floorf(x1);
	if (!(type & MESH_TYPE_WRAP_U)) {
		col0 = clampi(col0, count_u - 1);
		col1 = clampi(col1, count_u - 1);
	}
	if (!(type & MESH_TYPE_WRAP_V)) {
		row0 = clampi(row0, count_v - 1);
		row1 = clampi(row1, count_v - 1);
	}
	w = 1.0f / w;
	h = 1.0f / h;
	for (int row = row0; row <= row1; row++) {
		const vector<float> &sv = it.div_v;
		float fr = (float)row;
		float t0 = clampf(y - fr, 1.0f);
		float t1 = clampf(y1 - fr, 1.0f);
		if (t0 == t1)
			continue;
		float v0 = (fr + t0 - y) * h;
		float v1 = (fr + t1 - y) * h;
		int rr = row % count_v;
		if (rr < 0)
			rr += count_v;
		float sy;
		if (rr < (int)(it.div_v.end() - it.div_v.begin()))
			sy = it.div_v[rr];
		else
			sy = sv.back();
		rr *= count_u;
		for (int col = col0; col <= col1; col++) {
			const vector<float> &su = it.div_u;
			float fc = (float)col;
			float u0 = clampf(x - fc, 1.0f);
			float u1 = clampf(x1 - fc, 1.0f);
			if (u0 == u1)
				continue;
			float uu0 = (fc + u0 - x) * w;
			float uu1 = (fc + u1 - x) * w;
			g.PatchClip(u0, t0, u1 - u0, t1 - t0);
			g.PatchCrop(uu0, v0, uu1 - uu0, v1 - v0);
			int cc = col % count_u;
			if (cc < 0)
				cc += count_u;
			float sx;
			if (cc < (int)(it.div_u.end() - it.div_u.begin()))
				sx = it.div_u[cc];
			else
				sx = su.back();
			g.PatchSubdiv(sx, sy);
			g.CallList(lists[rr + cc]);
		}
	}
}

/*
 * Rebuilds `control_grid`, the control grid of (count_u + 3) x (count_v + 3) points per
 * morph target, from `positions` (count_u x count_v). The grid has one guard
 * row/column before and two after, filled by:
 *   - MESH_TYPE_WRAP_U / MESH_TYPE_WRAP_V: wrap-around (closed surface):
 *     g[-1] = g[count-1], g[count] = g[0], g[count+1] = g[1];
 *   - otherwise, if the type is not MESH_TYPE_BILINEAR: linear extrapolation:
 *     g[-1] = 2*g[0] - g[1], g[count] = 2*g[count-1] - g[count-2].
 * For MESH_TYPE_BILINEAR (open patch) the guards are left as they are.
 */
int Mesh::shape_info::fixup_arrays()
{
	int cnt = (int)(positions.size() / 3);
	int block = count_u * count_v;
	int n = cnt / block;
	if (n <= 0)
		return 0;
	int stride = (count_u + 3) * (count_v + 3);
	if (n < morph_count)
		n = 1;
	control_grid.resize(stride * n * 3);

	const float *src = positions.begin();
	float *base = control_grid.begin() + (count_u + 4) * 3;
	for (int m = 0; m < n; m++) {
		const float *s = src;
		float *d = base;
		for (int j = 0; j < count_v; j++) {
			for (int i = 0; i < count_u; i++) {
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				s += 3;
				d += 3;
			}
			d += 9;
		}

		float *p = base;
		int rowStride = (count_u + 3) * 3;
		int open = (type == MESH_TYPE_BILINEAR);
		float *e = base + count_u * 3;
		if (type & MESH_TYPE_WRAP_U) {
			for (int j = 0; j < count_v; j++) {
				float *a = p + 3;
				float *b = e + 3;
				a[-6] = b[-6];
				a[-5] = b[-5];
				a[-4] = b[-4];
				b[-3] = a[-3];
				b[-2] = a[-2];
				b[-1] = a[-1];
				b[0] = a[0];
				b[1] = a[1];
				b[2] = a[2];
				p += rowStride;
				e += rowStride;
			}
		} else if (!open) {
			for (int j = 0; j < count_v; j++) {
				float *r = p + 3;
				r[-6] = r[-3] + r[-3] - r[0];
				r[-5] = r[-2] + r[-2] - r[1];
				r[-4] = r[-1] + r[-1] - r[2];
				float *q = e - 6;
				q[6] = q[3] + q[3] - q[0];
				q[7] = q[4] + q[4] - q[1];
				q[8] = q[5] + q[5] - q[2];
				p += rowStride;
				e += rowStride;
			}
		}

		float *u = base - 3;
		float *w = u + rowStride * count_v;
		if (type & MESH_TYPE_WRAP_V) {
			for (int c = 0; c < count_u + 3; c++) {
				float *pu = u - rowStride;
				float *pw = w - rowStride;
				float *nu = u + rowStride;
				float *nw = w + rowStride;
				pu[0] = pw[0];
				pu[1] = pw[1];
				pu[2] = pw[2];
				w[0] = u[0];
				w[1] = u[1];
				w[2] = u[2];
				nw[0] = nu[0];
				nw[1] = nu[1];
				nw[2] = nu[2];
				u += 3;
				w += 3;
			}
		} else if (!open) {
			for (int c = 0; c < count_u + 3; c++) {
				float *pu = u - rowStride;
				float *nu = u + rowStride;
				float *lw = w - rowStride;
				float *pw = w - 2 * rowStride;
				pu[0] = u[0] + u[0] - nu[0];
				pu[1] = u[1] + u[1] - nu[1];
				pu[2] = u[2] + u[2] - nu[2];
				w[0] = lw[0] + lw[0] - pw[0];
				w[1] = lw[1] + lw[1] - pw[1];
				w[2] = lw[2] + lw[2] - pw[2];
				u += 3;
				w += 3;
			}
		}

		src += block * 3;
		base += stride * 3;
	}
	return 1;
}

int Mesh::shape_info::fixup_disp_lists()
{
	int cnt = (int)(control_grid.size() / 3);
	int block = (count_u + 3) * (count_v + 3);
	int n = cnt / block;
	if (n <= 0)
		return 0;
	if (n < morph_count)
		n = 1;

	gm_graph *g = SceneGetGraph();
	for (int k = 0; k < (int)(lists.end() - lists.begin()); k++)
		g->DeleteList(lists[k]);
	lists.erase(lists.begin(), lists.end());
	g->SetArrays(control_grid.begin(), GM_VERTEX_POSITION, block, n, 0, 0);

	int basis;
	int step;
	int base;
	if ((unsigned char)type == MESH_TYPE_BILINEAR) {
		basis = 0;
		step = 2;
		base = count_u + 4;
	} else {
		base = 0;
		basis = 15;
		step = 4;
	}
	int mode = s_meshDrawFlag ? 2 : 4;
	int stride = count_u + 3;

	for (int j = 0; j < count_v; j++) {
		if (j < count_v - 1 || (type & MESH_TYPE_WRAP_V)) {
			for (int i = 0; i < count_u; i++) {
				int handle = 0;
				if (i < count_u - 1 || (type & MESH_TYPE_WRAP_U)) {
					handle = g->CreateList();
					if (handle) {
						g->BeginList(handle);
						g->DrawRectPatch(mode, basis, base, step, step, stride, 0);
						g->EndList(0);
					}
				}
				lists.push_back(handle);
				base++;
			}
			base += 3;
		} else {
			for (int i = 0; i <= count_u; i++) {
				lists.push_back(0);
				base++;
			}
			base += 3;
		}
	}
	return 1;
}

void Mesh::shape_info::set_positions(const float *src, int count, int start, int repeat)
{
	count = count / 3;
	if (start < 0) {
		count += start;
		src += (-start) * 3;
		start = 0;
	}
	if (count <= 0)
		return;
	int total = start + count;
	int block = count_u * count_v;
	total = (total + block - 1) / block * block;
	if (positions.end() - positions.begin() < total * 3)
		positions.resize(total * 3);
	const float *se = src + count * 3;
	float *d = positions.begin() + start * 3;
	float x = 0.0f, y = 0.0f, z = 0.0f;
	for (int i = start; i < total; i++) {
		if (src >= se) {
			if (!repeat)
				break;
		} else {
			x = src[0];
			y = src[1];
			z = src[2];
			src += 3;
		}
		d[0] = x;
		d[1] = y;
		d[2] = z;
		d += 3;
	}
	flags |= MESH_LISTS_DIRTY | MESH_ARRAYS_DIRTY;
}

#define DERIV_EPS 1e-6f

/*
 * Returns the surface point (idx 0) or a derivative (bit 0 = d/du, bit 1 =
 * d/dv) on morph target `morph`, from the control grid of fixup_arrays.
 * MESH_TYPE_BILINEAR is a bilinear patch (2x2 points), otherwise a uniform cubic
 * B-spline (4x4). The weights are computed in double precision.
 */
vec4 Mesh::shape_info::calc_surface(float u, float v, int idx, int morph) const
{
	if (flags & MESH_ARRAYS_DIRTY)
		return vec4(0.0f, 0.0f, 0.0f, 0.0f);

	float fu = floorf(u);
	u -= fu;
	if (!(type & MESH_TYPE_WRAP_U)) {
		float last = count_u - 1;
		if (fu < 0.0f) {
			u = 0.0f;
			fu = 0.0f;
		}
		if (last <= fu) {
			fu = last - 1.0f;
			u = 1.0f;
		}
	}
	float fv = floorf(v);
	v -= fv;
	if (!(type & MESH_TYPE_WRAP_V)) {
		float last = count_v - 1;
		if (fv < 0.0f) {
			v = 0.0f;
			fv = 0.0f;
		}
		if (last <= fv) {
			fv = last - 1.0f;
			v = 1.0f;
		}
	}

	int iu = (int)fu % count_u;
	int iv = (int)fv % count_v;
	if (iu < 0)
		iu += count_u;
	if (iv < 0)
		iv += count_v;

	vec4 result(0.0f, 0.0f, 0.0f, 0.0f);

	if ((type & 0xff) == MESH_TYPE_BILINEAR) {
		float wu[2], wv[2];
		if ((idx & 1) == 0) {
			wu[0] = 1.0f - u;
			wu[1] = u;
		} else {
			wu[0] = -1.0f;
			wu[1] = 1.0f;
		}
		if ((idx & 2) == 0) {
			wv[0] = 1.0f - v;
			wv[1] = v;
		} else {
			wv[0] = -1.0f;
			wv[1] = 1.0f;
		}
		const float *p = control_grid.begin()
		    + ((count_u + 3) * (count_v + 3) * morph + (count_u + 3) * iv + iu + count_u + 4) * 3;
		int skip = (count_u + 1) * 3;
		for (int j = 0; j < 2; j++) {
			for (int i = 0; i < 2; i++) {
				float w = wu[i] * wv[j];
				result.x += p[0] * w;
				result.y += p[1] * w;
				result.z += p[2] * w;
				p += 3;
			}
			p += skip;
		}
	} else {
		float wu[4], wv[4];
		float u2 = u * u;
		if ((idx & 1) == 0) {
			float t3 = u2 * u;
			wu[0] = -t3 * (1.0 / 6.0) + u2 * 0.5 + u * -0.5 + 1.0 / 6.0;
			wu[1] = t3 * 0.5 + u2 * -1.0 + 2.0 / 3.0;
			wu[2] = t3 * -0.5 + u2 * 0.5 + u * 0.5 + 1.0 / 6.0;
			wu[3] = t3 * (1.0 / 6.0);
		} else {
			wu[0] = u2 * -0.5 + u + (-0.5 + DERIV_EPS);
			wu[1] = u2 * 1.5 + u * -2.0 + DERIV_EPS;
			wu[2] = u2 * -1.5 + u + (0.5 + DERIV_EPS);
			wu[3] = u2 * 0.5 + DERIV_EPS;
		}
		float v2 = v * v;
		if ((idx & 2) == 0) {
			float t3 = v2 * v;
			wv[0] = -t3 * (1.0 / 6.0) + v2 * 0.5 + v * -0.5 + 1.0 / 6.0;
			wv[1] = t3 * 0.5 + v2 * -1.0 + 2.0 / 3.0;
			wv[2] = t3 * -0.5 + v2 * 0.5 + v * 0.5 + 1.0 / 6.0;
			wv[3] = t3 * (1.0 / 6.0);
		} else {
			wv[0] = v2 * -0.5 + v + (-0.5 + DERIV_EPS);
			wv[1] = v2 * 1.5 + v * -2.0 + DERIV_EPS;
			wv[2] = v2 * -1.5 + v + (0.5 + DERIV_EPS);
			wv[3] = v2 * 0.5 + DERIV_EPS;
		}
		const float *p =
		    control_grid.begin() + ((count_u + 3) * (count_v + 3) * morph + (count_u + 3) * iv + iu) * 3;
		int skip = (count_u - 1) * 3;
		for (int j = 0; j < 4; j++) {
			for (int i = 0; i < 4; i++) {
				float w = wu[i] * wv[j];
				result.x += p[0] * w;
				result.y += p[1] * w;
				result.z += p[2] * w;
				p += 3;
			}
			p += skip;
		}
	}
	return result;
}
