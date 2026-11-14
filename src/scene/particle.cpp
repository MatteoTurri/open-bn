/*
 * Particle, Particle::item_info and Particle::shape_info methods: options,
 * morph weights, vertex array building and drawing.
 */

#pragma implementation
#include <vector>
#include "gfx/vec.h"
#include "particle.h"
#include "scene/optid.h"
#include "gfx/tm.h"
#include "script/parseresult.h"
#include <math.h>

/* Identity matrix and w = 1 row; item_info::draw starts from the matrix. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

/* Auto-crop flag: set by DrawThis, read by item_info::draw. */
static int g_particleAutoCrop = 0;

static float s_origin[3] = { 0.0f, 0.0f, 0.0f };

/* An fvec4 of item_info::vec_options, read or written as a vec4. */
static inline vec4 &as_vec4(fvec4 &v)
{
	return *(vec4 *)&v;
}

static inline const vec4 &as_vec4(const fvec4 &v)
{
	return *(const vec4 *)&v;
}

Particle::Particle(Object *parent) : Object(parent)
{
	EffOptionState &opt = option_state;
	opt.SetLayerMode(1, 0xffff);
	opt.always_mask |= EFF_TEXTURE_CROP | EFF_DIFFUSE;
	flags = 0;
	tex_handle = 0;
	items.resize(1);
	items[0].set_master_shape(&shape);
	shape.set_positions(s_origin, 3, 0, 0);
	pmod = 0;
	pidx = 0;
}

Particle::~Particle()
{
	if (tex_handle)
		tmRelease(tex_handle);
}

void Particle::DrawThis(gm_graph &g)
{
	g_particleAutoCrop = option_state.HasAutoCrop();

	int bits;
	if (((unsigned)flags >> 9) & 1) /* PARTICLE_TEX_NONPOW2 */
		bits = g.fFlagsE | GM_ENABLE_TEXTURE_FIT;
	else
		bits = g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT;
	g.SetEnableBits(bits);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
	g.SetTexture(tex_handle);
	g.SetVertexShader(-2);

	const float *m = &world_mat.m0.x;
	float scale = sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);

	if (flags & PARTICLE_SHAPE_CHANGED)
		fixup_shape_sharing();

	for (int i = 0; i < items.end() - items.begin(); i++)
		items.begin()[i].draw(g, scale);

	g.ParticleSize(1.0f);
	g.ParticleCrop(0.0f, 0.0f, 1.0f, 1.0f);
}

/* 'NUM' resizes items: growing from empty attaches every item to shape,
   growing otherwise copies the last item without its own shape. */
int Particle::SetOption(int option, const int &value, unsigned int size)
{
	switch (option) {
	case OPT_NUM: {
		int n = value;
		if (n < 0)
			n = 0;
		if (n > 256)
			n = 256;
		int count = items.end() - items.begin();
		if (n == count)
			return 1;
		if (n < count) {
			item_info temp;
			items.resize(n, temp);
		} else if (count == 0) {
			{
				item_info temp;
				items.resize(n, temp);
			}
			for (int i = 0; i < n; i++)
				items[i].set_master_shape(&shape);
		} else {
			item_info temp(items.end()[-1]);
			temp.free_shape();
			for (int i = count; i < n; i++)
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
	case OPT_DISP:
	case OPT_ALPH:
	case OPT_MMOD:
		for (int i = 0; i < (int)items.size(); i++)
			SetItemOption(option, i, value, size);
		return 1;
	case OPT_TYPE:
		shape.set_type(value);
		flags |= PARTICLE_SHAPE_CHANGED;
		/* NOTE: bug: missing break, 'TYPE' also sets the point count */
	case OPT_PNUM:
		shape.set_point_count(value);
		flags |= PARTICLE_SHAPE_CHANGED;
		return 1;
	case OPT_MNUM:
		shape.set_morph_count(value);
		flags |= PARTICLE_SHAPE_CHANGED;
		return 1;
	case OPT_LAY1:
	case OPT_LAY2:
	case OPT_LAY3:
		return 0;
	}
	return Object::SetOption(option, value, size);
}

/* 'MTIM' and 'SIZE' apply to every item; the other float options go to
   the void * overload. */
int Particle::SetOption(int option, const float &value, unsigned int size)
{
	switch (option) {
	case OPT_MTIM:
	case OPT_SIZE:
		for (int i = 0; i < (int)items.size(); i++)
			SetItemOption(option, i, value, size);
		return 1;
	case OPT_PAXS:
	case OPT_PTEX:
	case OPT_PSIZ:
	case OPT_MWGT:
	case OPT_PNOR:
	case OPT_PPOS:
	case OPT_CROP:
	case OPT_DIFF:
	case OPT_SCAL:
	case OPT_PCOL:
	case OPT_MTRG:
	case OPT_ROT:
	case OPT_POS:
		return SetOption(option, &value, size * 4);
	}
	return Object::SetOption(option, value, size);
}

/* Vec4 options apply to every item. 'PPOS'/'PNOR'/'PSIZ'/'PCOL'/'PTEX'/'PAXS'
   write shape with pidx as start and pmod as clamp; only
   'PPOS'/'PCOL'/'PAXS' default the count to the whole shape. */
int Particle::SetOption(int option, const void *data, unsigned int size)
{
	int n = size / 4;

	switch (option) {
	case OPT_CROP:
	case OPT_DIFF:
	case OPT_SCAL:
	case OPT_MTRG:
	case OPT_POS:
	case OPT_ROT:
	case OPT_MWGT:
		for (int i = 0; i < (int)items.size(); i++)
			SetItemOption(option, i, data, size);
		return 1;
	case OPT_PPOS:
		if (n == 0)
			n = shape.point_count * 3;
		shape.set_positions((const float *)data, n, pidx, pmod);
		flags |= PARTICLE_SHAPE_CHANGED;
		return 1;
	case OPT_PNOR:
		shape.set_normals((const float *)data, n, pidx, pmod);
		flags |= PARTICLE_SHAPE_CHANGED;
		return 1;
	case OPT_PSIZ:
		shape.set_pointsizes((const float *)data, n, pidx, pmod);
		flags |= PARTICLE_SHAPE_CHANGED;
		return 1;
	case OPT_PCOL:
		if (n == 0)
			n = shape.point_count * 4;
		shape.set_colors((const float *)data, n, pidx, pmod);
		flags |= PARTICLE_SHAPE_CHANGED;
		return 1;
	case OPT_PTEX:
		shape.set_texcoords((const float *)data, n, pidx, pmod);
		flags |= PARTICLE_SHAPE_CHANGED;
		return 1;
	case OPT_PAXS:
		if (n == 0)
			n = shape.point_count * 6;
		shape.set_axes((const float *)data, n, pidx, pmod);
		flags |= PARTICLE_SHAPE_CHANGED;
		return 1;
	}
	return Object::SetOption(option, data, size);
}

/* 'TYPE'/'PNUM'/'MNUM' give the item its own shape before changing it. */
int Particle::SetItemOption(int option, int item, const int &value, unsigned int size)
{
	if (item < 0 || item >= (int)items.size())
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
	case OPT_TYPE: {
		if (flags & PARTICLE_SHAPE_CHANGED)
			fixup_shape_sharing();
		shape_info *s = it.alloc_shape();
		s->set_type(value);
		flags |= PARTICLE_ITEM_SHAPES;
		return 1;
	}
	case OPT_PNUM: {
		if (flags & PARTICLE_SHAPE_CHANGED)
			fixup_shape_sharing();
		shape_info *s = it.alloc_shape();
		s->set_point_count(value);
		flags |= PARTICLE_ITEM_SHAPES;
		return 1;
	}
	case OPT_MNUM: {
		if (flags & PARTICLE_SHAPE_CHANGED)
			fixup_shape_sharing();
		shape_info *s = it.alloc_shape();
		s->set_morph_count(value);
		flags |= PARTICLE_ITEM_SHAPES;
		return 1;
	}
	}
	return Object::SetItemOption(option, item, value, size);
}

/* 'SIZE' gives the item its own shape when the current one has type PARTICLE_QUADS. */
int Particle::SetItemOption(int option, int item, const float &value, unsigned int size)
{
	if (item < 0 || item >= (int)items.size())
		return 0;
	item_info &it = items[item];

	switch (option) {
	case OPT_MTIM:
		it.morph_time = value;
		if (!it.morph_mode)
			it.morph_mode = MORPH_LINEAR;
		return 1;
	case OPT_SIZE: {
		it.size = value;
		if (it.get_shape()->type == PARTICLE_QUADS) {
			if (flags & PARTICLE_SHAPE_CHANGED)
				fixup_shape_sharing();
			it.alloc_shape()->set_size(value);
			flags |= PARTICLE_ITEM_SHAPES;
		}
		return 1;
	}
	case OPT_PAXS:
	case OPT_PTEX:
	case OPT_PNOR:
	case OPT_DIFF:
	case OPT_SCAL:
	case OPT_ROT:
	case OPT_PSIZ:
	case OPT_MWGT:
	case OPT_PPOS:
	case OPT_CROP:
	case OPT_PCOL:
	case OPT_MTRG:
	case OPT_POS:
		return SetItemOption(option, item, &value, size * 4);
	}
	return Object::SetItemOption(option, item, value, size);
}

/* 'MTRG'/'DIFF'/'CROP'/'POS'/'ROT'/'SCAL' set a vec4 of vec_options (the last
   three also set ITEM_POS/ITEM_ROT/ITEM_SCAL); 'MWGT' sets the morph weights; the shape
   options give the item its own shape first. */
int Particle::SetItemOption(int option, int item, const void *data, unsigned int size)
{
	if (item < 0 || item >= (int)items.size())
		return 0;
	unsigned int n = size / 4;
	item_info &it = items[item];
	const float *f = (const float *)data;

	switch (option) {
	case OPT_DIFF:
		ParseVec4(as_vec4(it.vec_options.q.diffuse), f, n);
		return 1;
	case OPT_CROP:
		ParseVec4(as_vec4(it.vec_options.q.crop), f, n);
		return 1;
	case OPT_POS:
		ParseVec4(as_vec4(it.vec_options.q.pos), f, n);
		it.flags |= ITEM_POS;
		return 1;
	case OPT_ROT:
		ParseVec4(as_vec4(it.vec_options.q.rot), f, n);
		it.flags |= ITEM_ROT;
		return 1;
	case OPT_SCAL:
		ParseVec4(as_vec4(it.vec_options.q.scale), f, n);
		it.flags |= ITEM_SCAL;
		return 1;
	case OPT_MWGT:
		ParseFloatArray(it.weights, f, n);
		if (it.morph_mode)
			it.morph_mode = 0;
		return 1;
	case OPT_MTRG:
		ParseVec4(as_vec4(it.vec_options.q.morph_range), f, n);
		return 1;
	case OPT_PPOS:
		if (flags & PARTICLE_SHAPE_CHANGED)
			fixup_shape_sharing();
		it.alloc_shape()->set_positions(f, n, pidx, pmod);
		flags |= PARTICLE_ITEM_SHAPES;
		return 1;
	case OPT_PNOR:
		if (flags & PARTICLE_SHAPE_CHANGED)
			fixup_shape_sharing();
		it.alloc_shape()->set_normals(f, n, pidx, pmod);
		flags |= PARTICLE_ITEM_SHAPES;
		return 1;
	case OPT_PSIZ:
		if (flags & PARTICLE_SHAPE_CHANGED)
			fixup_shape_sharing();
		it.alloc_shape()->set_pointsizes(f, n, pidx, pmod);
		flags |= PARTICLE_ITEM_SHAPES;
		return 1;
	case OPT_PCOL:
		if (flags & PARTICLE_SHAPE_CHANGED)
			fixup_shape_sharing();
		it.alloc_shape()->set_colors(f, n, pidx, pmod);
		flags |= PARTICLE_ITEM_SHAPES;
		return 1;
	case OPT_PTEX:
		if (flags & PARTICLE_SHAPE_CHANGED)
			fixup_shape_sharing();
		it.alloc_shape()->set_texcoords(f, n, pidx, pmod);
		flags |= PARTICLE_ITEM_SHAPES;
		return 1;
	case OPT_PAXS:
		if (flags & PARTICLE_SHAPE_CHANGED)
			fixup_shape_sharing();
		it.alloc_shape()->set_axes(f, n, pidx, pmod);
		flags |= PARTICLE_ITEM_SHAPES;
		return 1;
	}
	return Object::SetItemOption(option, item, data, size);
}

int Particle::GetItemOption(int option, int item, int &value, unsigned int size) const
{
	if (item < 0 || item >= (int)items.size())
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
		value = it.get_shape()->point_count;
		return 1;
	case OPT_MNUM:
		value = it.get_shape()->morph_count;
		return 1;
	}
	return Object::GetItemOption(option, item, value, size);
}

int Particle::GetItemOption(int option, int item, float &value, unsigned int size) const
{
	if (item < 0 || item >= (int)items.size())
		return 0;
	const item_info &it = items[item];

	switch (option) {
	case OPT_MTIM:
		value = it.morph_time;
		return 1;
	case OPT_SIZE:
		value = it.size;
		return 1;
	case OPT_PAXS:
	case OPT_PTEX:
	case OPT_PNOR:
	case OPT_DIFF:
	case OPT_SCAL:
	case OPT_ROT:
	case OPT_PSIZ:
	case OPT_MWGT:
	case OPT_PPOS:
	case OPT_CROP:
	case OPT_PCOL:
	case OPT_MTRG:
	case OPT_POS:
		return GetItemOption(option, item, &value, size * 4);
	}
	return Object::GetItemOption(option, item, value, size);
}

int Particle::GetItemOption(int option, int item, void *data, unsigned int size) const
{
	static vec4 s_defaultA(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */
	static vec4 s_defaultB(1.0f, 1.0f, 1.0f, 1.0f);  /* unused, but part of the layout */

	if (item < 0 || item >= (int)items.size())
		return 0;
	const item_info &it = items[item];
	int n = size / 4;

	switch (option) {
	case OPT_DIFF:
		ResultVec4(as_vec4(it.vec_options.q.diffuse), (float *)data, n);
		return 1;
	case OPT_CROP:
		ResultVec4(as_vec4(it.vec_options.q.crop), (float *)data, n);
		return 1;
	case OPT_POS:
		ResultVec4(as_vec4(it.vec_options.q.pos), (float *)data, n);
		return 1;
	case OPT_ROT:
		ResultVec4(as_vec4(it.vec_options.q.rot), (float *)data, n);
		return 1;
	case OPT_SCAL:
		ResultVec4(as_vec4(it.vec_options.q.scale), (float *)data, n);
		return 1;
	case OPT_MWGT:
		ResultFloatArray(it.weights, (float *)data, n);
		return 1;
	case OPT_MTRG:
		ResultVec4(as_vec4(it.vec_options.q.morph_range), (float *)data, n);
		return 1;
	case OPT_PPOS:
		it.get_shape()->get_positions((float *)data, n, pidx);
		return 1;
	case OPT_PNOR:
		it.get_shape()->get_normals((float *)data, n, pidx);
		return 1;
	case OPT_PSIZ:
		it.get_shape()->get_pointsizes((float *)data, n, pidx);
		return 1;
	case OPT_PCOL:
		it.get_shape()->get_colors((float *)data, n, pidx);
		return 1;
	case OPT_PTEX:
		it.get_shape()->get_texcoords((float *)data, n, pidx);
		return 1;
	case OPT_PAXS:
		it.get_shape()->get_axes((float *)data, n, pidx);
		return 1;
	}
	return Object::GetItemOption(option, item, data, size);
}

void Particle::SetTexture(int texture, int option, int mode)
{
	if (option == 0 || option == EFF_SLOT_LAYER0) {
		int old = tex_handle;
		flags &= ~PARTICLE_TEX_NONPOW2;
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
				flags |= PARTICLE_TEX_NONPOW2;
		}
		return;
	}
	option_state.SetTexture(texture, option, mode);
}

int Particle::GetOption(int option, int &value, unsigned int size) const
{
	switch (option) {
	case OPT_NUM:
		value = items.size();
		return 1;
	case OPT_PMOD:
		value = pmod;
		return 1;
	case OPT_PIDX:
		value = pidx;
		return 1;
	case OPT_ALPH:
	case OPT_PNUM:
	case OPT_DISP:
	case OPT_MNUM:
	case OPT_MMOD:
	case OPT_TYPE:
		return GetItemOption(option, 0, value, size);
	}
	return Object::GetOption(option, value, size);
}

/* Options not handled here go to GetItemOption(option, 0, ...) or to the
   void * overload. */

int Particle::GetOption(int option, float &value, unsigned int size) const
{
	switch (option) {
	case OPT_MTIM:
	case OPT_SIZE:
		return GetItemOption(option, 0, value, size);
	case OPT_PAXS:
	case OPT_PTEX:
	case OPT_PSIZ:
	case OPT_MWGT:
	case OPT_PNOR:
	case OPT_PPOS:
	case OPT_CROP:
	case OPT_DIFF:
	case OPT_SCAL:
	case OPT_PCOL:
	case OPT_MTRG:
	case OPT_ROT:
	case OPT_POS:
		return GetOption(option, &value, size * 4);
	}
	return Object::GetOption(option, value, size);
}

/* All the handled options read item 0. */
int Particle::GetOption(int option, void *data, unsigned int size) const
{
	switch (option) {
	case OPT_CROP:
	case OPT_DIFF:
	case OPT_POS:
	case OPT_ROT:
	case OPT_PAXS:
	case OPT_PNOR:
	case OPT_PPOS:
	case OPT_PTEX:
	case OPT_MWGT:
	case OPT_PSIZ:
	case OPT_SCAL:
	case OPT_PCOL:
	case OPT_MTRG:
		return GetItemOption(option, 0, data, size);
	}
	return Object::GetOption(option, data, size);
}

void Particle::fixup_shape_sharing()
{
	if (flags & PARTICLE_ITEM_SHAPES) {
		for (int i = 0; i < items.end() - items.begin(); i++)
			items[i].free_shape();
	}
	flags &= ~(PARTICLE_SHAPE_CHANGED | PARTICLE_ITEM_SHAPES);
}

inline Particle::item_info::~item_info()
{
	if (own_shape)
		delete own_shape;
}

Particle::item_info::item_info(const item_info &o)
{
	own_shape = 0;
	*this = o;
}

void Particle::item_info::set_master_shape(shape_info *s)
{
	master_shape = s;
}

Particle::shape_info *Particle::item_info::get_shape() const
{
	return own_shape ? own_shape : master_shape;
}

Particle::shape_info *Particle::item_info::alloc_shape()
{
	if (own_shape)
		return own_shape;
	own_shape = new shape_info();
	if (master_shape)
		*own_shape = *master_shape;
	own_shape->size = size;
	return own_shape;
}

void Particle::item_info::free_shape()
{
	if (own_shape)
		delete own_shape;
	own_shape = 0;
}

Particle::item_info::item_info()
{
	flags = 0;
	master_shape = 0;
	own_shape = 0;
	size = 1.0f;
	disp = DISP_VISIBLE;
	blend = GM_BLEND_ALPHA;
	morph_mode = MORPH_LINEAR;
	morph_time = 0.0f;
	vec_options.q.morph_range.Set(0.0f, 0.0f, 0.0f, 1.0f);
	vec_options.q.diffuse.Set(1.0f, 1.0f, 1.0f, 1.0f);
	vec_options.q.crop.Set(0.0f, 0.0f, 1.0f, 1.0f);
	vec_options.q.pos.Set(0.0f, 0.0f, 0.0f, 1.0f);
	vec_options.q.rot.Set(0.0f, 0.0f, 0.0f, 1.0f);
	vec_options.q.scale.Set(1.0f, 1.0f, 1.0f, 1.0f);
}

Particle::item_info &Particle::item_info::operator=(const item_info &o)
{
	if (this != &o)
		goto work;
	return *this;
work:
	flags = o.flags;
	master_shape = o.master_shape;
	if (own_shape)
		delete own_shape;
	own_shape = 0;
	if (o.own_shape)
		own_shape = new shape_info(*o.own_shape);
	size = o.size;
	disp = o.disp;
	blend = o.blend;
	morph_mode = o.morph_mode;
	morph_time = o.morph_time;
	weights = o.weights;
	vec_options.q.morph_range = o.vec_options.q.morph_range;
	vec_options.q.diffuse = o.vec_options.q.diffuse;
	vec_options.q.crop = o.vec_options.q.crop;
	vec_options.q.pos = o.vec_options.q.pos;
	vec_options.q.rot = o.vec_options.q.rot;
	vec_options.q.scale = o.vec_options.q.scale;
	return *this;
}

/* Draws the item: morph weights, blend, diffuse, local transform (YXZ
   rotation, translation, scale), texture crop and point size, then the shape. */
void Particle::item_info::draw(gm_graph &g, float scale)
{
	shape_info *shape = get_shape();
	if (!shape)
		return;
	if (!(disp & DISP_VISIBLE))
		return;
	if (!set_morph_weights(g))
		return;

	g.BlendFunc(blend);
	g.Diffuse(as_vec4(vec_options.q.diffuse));

	if (flags & ITEM_TRANSFORM) {
		mat4 m = s_unitMatrix;
		if (flags & ITEM_ROT) {
			const vec4 &r = as_vec4(vec_options.q.rot);
			m = mat4_rotate_y(r.y) * mat4_rotate_x(r.x) * mat4_rotate_z(r.z);
		}
		if (flags & ITEM_POS)
			m.m3.Set(vec_options.v[12], vec_options.v[13], vec_options.v[14], 1.0f);
		if (flags & ITEM_SCAL) {
			m.m0 *= as_vec4(vec_options.q.scale);
			m.m1 *= as_vec4(vec_options.q.scale);
			m.m2 *= as_vec4(vec_options.q.scale);
		}
		mat4 mm = *g.GetMatrix(GM_MATRIX_MODEL) * m;
		g.PushMatrix(GM_MATRIX_MODEL, mm);
	}

	if (shape->type == PARTICLE_QUADS) {
		if (!g_particleAutoCrop)
			g.TexCrop(as_vec4(vec_options.q.crop));
	} else {
		if (!(shape->format & GM_VERTEX_POINTSIZE))
			scale = scale * 0.1f;
		if (!g_particleAutoCrop)
			g.TexCrop(as_vec4(vec_options.q.crop));
		g.ParticleSize(size * scale);
	}

	shape->draw(g);

	if (flags & ITEM_TRANSFORM)
		g.PopMatrix(GM_MATRIX_MODEL);
}

/* Same kernels as Mesh::item_info::set_morph_weights (time morph_time, range in
   vec_options.v[0..3], mode morph_mode), then passes the weights to gm_graph. */
int Particle::item_info::set_morph_weights(gm_graph &g)
{
	shape_info *shape = get_shape();
	if (!shape)
		return 0;

	float t = morph_time - vec_options.v[0];
	if (t < 0.0f)
		return 0;
	t = t * vec_options.v[3] + vec_options.v[2];
	if (vec_options.v[1] > 0.0f && t > vec_options.v[1])
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
	g.SetMorphWeights(weights.begin(), mc);
	return 1;
}

Particle::shape_info::shape_info()
{
	flags = 0;
	type = PARTICLE_QUADS;
	point_count = 1;
	morph_count = 1;
	format = 0;
	morph_format = 0;
	size = 1.0f;
	list = SceneGetGraph()->CreateList();
}

Particle::shape_info::~shape_info()
{
	SceneGetGraph()->DeleteList(list);
}

Particle::shape_info::shape_info(const shape_info &o)
{
	int f = o.flags;
	list = 0;
	*this = o;
	flags = f;
	list = SceneGetGraph()->DuplicateList(o.list);
}

Particle::shape_info &Particle::shape_info::operator=(const shape_info &o)
{
	if (this == &o)
		return *this;
	flags = o.flags | PARTICLE_LIST_DIRTY;
	type = o.type;
	size = o.size;
	SceneGetGraph()->ClearList(list);
	point_count = o.point_count;
	morph_count = o.morph_count;
	format = o.format;
	morph_format = o.morph_format;
	vertices = o.vertices;
	positions = o.positions;
	normals = o.normals;
	pointsizes = o.pointsizes;
	colors = o.colors;
	texcoords = o.texcoords;
	axes = o.axes;
	return *this;
}

void Particle::shape_info::draw_shape(gm_graph &g)
{
	int n = vertices.size();
	if (n == 0)
		return;
	int m = morph_format ? morph_count : 1;
	switch (type) {
	case PARTICLE_QUADS:
		g.SetArrays(vertices.begin(), format, point_count * 4, m, morph_format, 0);
		g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, point_count, 0);
		break;
	case PARTICLE_POINTS:
		g.SetArrays(vertices.begin(), format, point_count, m, morph_format, 0);
		g.DrawParticle(GM_PRIM_POINT, 0, point_count, 1, 0);
		break;
	case PARTICLE_LINES:
		g.SetArrays(vertices.begin(), format, point_count, m, morph_format, 0);
		g.DrawParticle(GM_PRIM_LINE, 0, point_count, 1, 0);
		break;
	}
}

/*
 * Rebuilds the interleaved `vertices` array. For PARTICLE_QUADS every point becomes a
 * quad of 4 vertices offset along the `right`/`up` axes (from `axes` when it
 * is long enough, otherwise (0.05, 0, 0) and (0, 0.05, 0)); for other types
 * each point gives one vertex. format/morph_format are the layout masks passed to
 * gm_graph::GetStride.
 */
void Particle::shape_info::fixup_arrays()
{
	format = 0;
	morph_format = 0;
	vertices.erase(vertices.begin(), vertices.end());

	int n = point_count * morph_count;
	if (n == 0)
		return;

	if (type == PARTICLE_QUADS) {
		/* Quads: 4 vertices per point. */
		int fmt = GM_VERTEX_POSITION;
		if ((int)positions.size() < point_count * 3)
			return;

		int use_tex = 0;
		int use_axes = 0;
		if ((int)normals.size() >= point_count * 3)
			fmt |= GM_VERTEX_NORMAL;
		if ((int)colors.size() >= point_count)
			fmt |= GM_VERTEX_COLOR;
		if ((int)texcoords.size() >= point_count * 2)
			use_tex = 1;
		if ((int)axes.size() >= point_count * 6)
			use_axes = 1;
		int use_axes2 = 0;
		int use_tex2 = 0;
		int morphFmt = 0;
		format = fmt | GM_VERTEX_TEXCOORD;
		int stride = gm_graph::GetStride(format);

		if (morph_count >= 2) {
			if ((int)positions.size() >= n * 3)
				morphFmt |= GM_VERTEX_POSITION;
			if ((int)normals.size() >= n * 3)
				morphFmt |= GM_VERTEX_NORMAL;
			if ((int)colors.size() >= n)
				morphFmt |= GM_VERTEX_COLOR;
			if ((int)texcoords.size() >= n * 2) {
				morphFmt |= GM_VERTEX_TEXCOORD;
				use_tex2 = 1;
			}
			if ((int)axes.size() >= n * 6)
				use_axes2 = 1;
			morph_format = morphFmt;
			stride += gm_graph::GetStride(morphFmt) * (morph_count - 1);
		}

		vertices.resize(stride * point_count);

		int scale_by_size = 1;
		const float *pp = positions.begin();
		const float *pn = normals.begin();
		const float *pcol = (const float *)colors.begin();
		const float *pt = texcoords.begin();
		const float *pa = axes.begin();
		float *dst = vertices.begin();
		vec4 base;
		vec4 right(0.05f, 0.0f, 0.0f, 1.0f);
		vec4 up(0.0f, 0.05f, 0.0f, 1.0f);
		float col = 0.0f;
		if (size == 1.0f)
			scale_by_size = 0;

		for (int m = 0; m < morph_count; m++) {
			for (int i = 0; i < point_count; i++) {
				if (fmt & GM_VERTEX_POSITION) {
					base.Set(pp[0], pp[1], pp[2], 1.0f);
					pp += 3;
				}
				if (use_axes) {
					right.Set(pa[0], pa[1], pa[2], 1.0f);
					up.Set(pa[3], pa[4], pa[5], 1.0f);
					pa += 6;
				}
				if (fmt & GM_VERTEX_NORMAL)
					pn += 3;
				if (fmt & GM_VERTEX_COLOR) {
					col = pcol[0];
					pcol += 1;
				}
				if (use_tex)
					pt += 2;
				for (int k = 0; k < 4; k++) {
					float sy = -1.0f;
					if (k & 2)
						sy = 1.0f;
					float sx = 1.0f;
					if (k & 1)
						sx = -1.0f;
					if (scale_by_size) {
						sy *= size;
						sx *= size;
					}
					dst[0] = base.x + right.x * sy + up.x * sx;
					dst[1] = base.y + right.y * sy + up.y * sx;
					dst[2] = base.z + right.z * sy + up.z * sx;
					dst += 3;
					if (fmt & GM_VERTEX_NORMAL) {
						dst[0] = pn[-3];
						dst[1] = pn[-2];
						dst[2] = pn[-1];
						dst += 3;
					}
					if (fmt & GM_VERTEX_COLOR) {
						dst[0] = col;
						dst += 1;
					}
					if (k & 2)
						dst[0] = 1.0f;
					else
						dst[0] = 0.0f;
					if (k & 1)
						dst[1] = 1.0f;
					else
						dst[1] = 0.0f;
					if (use_tex) {
						dst[0] += pt[-2];
						dst[1] += pt[-1];
					}
					dst += 2;
				}
			}
			fmt = morphFmt;
			use_axes = use_axes2;
			use_tex = use_tex2;
		}
		return;
	}

	/* One vertex per point. */
	int fmt = GM_VERTEX_POSITION;
	if ((int)positions.size() < point_count * 3)
		return;
	if ((int)normals.size() >= point_count * 3)
		fmt |= GM_VERTEX_NORMAL;
	if ((int)pointsizes.size() >= point_count)
		fmt |= GM_VERTEX_POINTSIZE;
	if ((int)colors.size() >= point_count)
		fmt |= GM_VERTEX_COLOR;
	if ((int)texcoords.size() >= point_count * 2)
		fmt |= GM_VERTEX_TEXCOORD;
	format = fmt;
	int stride = gm_graph::GetStride(fmt);

	int morphFmt = 0;
	if (morph_count >= 2) {
		if ((int)positions.size() >= n * 3)
			morphFmt |= GM_VERTEX_POSITION;
		if ((int)normals.size() >= n * 3)
			morphFmt |= GM_VERTEX_NORMAL;
		if ((int)pointsizes.size() >= n)
			morphFmt |= GM_VERTEX_POINTSIZE;
		if ((int)colors.size() >= n)
			morphFmt |= GM_VERTEX_COLOR;
		if ((int)texcoords.size() >= n * 2)
			morphFmt |= GM_VERTEX_TEXCOORD;
		morph_format = morphFmt;
		stride += gm_graph::GetStride(morphFmt) * (morph_count - 1);
	}

	vertices.resize(stride * point_count / 4);

	const float *pp = positions.begin();
	const float *pn = normals.begin();
	const float *ps = pointsizes.begin();
	const float *pcol = (const float *)colors.begin();
	const float *pt = texcoords.begin();
	float *dst = vertices.begin();
	for (int m = 0; m < morph_count; m++) {
		for (int i = 0; i < point_count; i++) {
			if (fmt & GM_VERTEX_POSITION) {
				dst[0] = pp[0];
				dst[1] = pp[1];
				dst[2] = pp[2];
				pp += 3;
				dst += 3;
			}
			if (fmt & GM_VERTEX_NORMAL) {
				dst[0] = pn[0];
				dst[1] = pn[1];
				dst[2] = pn[2];
				pn += 3;
				dst += 3;
			}
			if (fmt & GM_VERTEX_POINTSIZE) {
				dst[0] = ps[0];
				ps += 1;
				dst += 1;
			}
			if (fmt & GM_VERTEX_COLOR) {
				dst[0] = pcol[0];
				pcol += 1;
				dst += 1;
			}
			if (fmt & GM_VERTEX_TEXCOORD) {
				dst[0] = pt[0];
				dst[1] = pt[1];
				pt += 2;
				dst += 2;
			}
		}
		fmt = morphFmt;
	}
}

static inline int clampByte(float f)
{
	int v = (int)f;
	if (v < 0)
		v = 0;
	if (v > 255)
		v = 255;
	return v;
}

void Particle::shape_info::set_colors(const float *src, int count, int start, int clamp)
{
	count /= 4;
	if (start < 0) {
		count += start;
		/* NOTE: bug: skips 3 floats per point instead of 4. */
		src += (-start) * 3;
		start = 0;
	}
	if (count > 0) {
		int total = start + count;
		if (point_count >= 2)
			total = (total + point_count - 1) / point_count * point_count;
		if ((int)colors.size() < total)
			colors.resize(total);
		const float *end = src + count * 4;
		int *d = colors.begin() + start;
		int color = -1;
		for (int i = start; i < total; i++) {
			if (src >= end) {
				if (!clamp)
					break;
			} else {
				int r = clampByte(src[0] * 255.0f);
				int g = clampByte(src[1] * 255.0f);
				int b = clampByte(src[2] * 255.0f);
				int a = clampByte(src[3] * 255.0f);
				src += 4;
				color = (r & 0xff) | ((g & 0xff) << 8) | ((b & 0xff) << 16) | ((a & 0xff) << 24);
			}
			*d++ = color;
		}
		flags |= PARTICLE_LIST_DIRTY | PARTICLE_ARRAYS_DIRTY;
	}
}

void Particle::shape_info::get_colors(float *dst, int count, int start)
{
	count /= 4;
	if (start < 0) {
		dst += (-start) * 4;
		count += start;
		start = 0;
	}
	int avail = (int)colors.size() - start;
	if (avail < count)
		count = avail;
	if (count > 0) {
		const int *s = colors.begin() + start;
		for (int i = 0; i < count; i++) {
			/* Components are unsigned bytes. */
			unsigned c = (unsigned)*s;
			dst[0] = (float)(c & 0xff) / 255.0f;
			dst[1] = (float)((c >> 8) & 0xff) / 255.0f;
			dst[2] = (float)((c >> 16) & 0xff) / 255.0f;
			dst[3] = (float)((c >> 24) & 0xff) / 255.0f;
			s++;
			dst += 4;
		}
	}
}

void Particle::shape_info::set_points(vector<float> &dst, const float *src, int count, int start, int clamp, int comps)
{
	count /= comps;
	if (start < 0) {
		count += start;
		src += (-start) * comps;
		start = 0;
	}
	if (count > 0) {
		int total = start + count;
		if (point_count >= 2)
			total = (total + point_count - 1) / point_count * point_count;
		if ((int)dst.size() < total * comps)
			dst.resize(total * comps);
		float *d = dst.begin() + start * comps;
		const float *end = src + count * comps;
		for (int i = start; i < total; i++) {
			if (src >= end) {
				if (!clamp)
					break;
				src = end - comps;
			}
			for (int c = 0; c < comps; c++)
				*d++ = *src++;
		}
		flags |= PARTICLE_LIST_DIRTY | PARTICLE_ARRAYS_DIRTY;
	}
}

void Particle::shape_info::draw(gm_graph &g)
{
	if (flags & PARTICLE_ARRAYS_DIRTY) {
		fixup_arrays();
		flags &= ~PARTICLE_ARRAYS_DIRTY;
	}
	if (flags & PARTICLE_LIST_DIRTY) {
		g.BeginList(list);
		draw_shape(g);
		g.EndList(0);
		flags &= ~PARTICLE_LIST_DIRTY;
	}
	if (!g.CallList(list))
		draw_shape(g);
}

void Particle::shape_info::set_type(int t)
{
	if (t != type) {
		type = t;
		flags |= PARTICLE_LIST_DIRTY | PARTICLE_ARRAYS_DIRTY;
	}
}

void Particle::shape_info::set_size(float s)
{
	if (s != size) {
		size = s;
		flags |= PARTICLE_LIST_DIRTY | PARTICLE_ARRAYS_DIRTY;
	}
}

void Particle::shape_info::set_point_count(int n)
{
	if (n < 0)
		n = 0;
	if (n > 4096)
		n = 4096;
	if (n != point_count) {
		point_count = n;
		flags |= PARTICLE_LIST_DIRTY | PARTICLE_ARRAYS_DIRTY;
	}
}

void Particle::shape_info::set_morph_count(int n)
{
	if (n < 1)
		n = 1;
	if (n > 16)
		n = 16;
	if (n != morph_count) {
		morph_count = n;
		flags |= PARTICLE_LIST_DIRTY | PARTICLE_ARRAYS_DIRTY;
	}
}

void Particle::shape_info::set_positions(const float *src, int count, int start, int clamp)
{
	set_points(positions, src, count, start, clamp, 3);
}

void Particle::shape_info::set_normals(const float *src, int count, int start, int clamp)
{
	set_points(normals, src, count, start, clamp, 3);
}

void Particle::shape_info::set_pointsizes(const float *src, int count, int start, int clamp)
{
	set_points(pointsizes, src, count, start, clamp, 1);
}

void Particle::shape_info::set_texcoords(const float *src, int count, int start, int clamp)
{
	set_points(texcoords, src, count, start, clamp, 2);
}

void Particle::shape_info::set_axes(const float *src, int count, int start, int clamp)
{
	set_points(axes, src, count, start, clamp, 6);
}

void Particle::shape_info::get_positions(float *dst, int count, int start)
{
	get_points(positions, dst, count, start, 3);
}

void Particle::shape_info::get_normals(float *dst, int count, int start)
{
	get_points(normals, dst, count, start, 3);
}

void Particle::shape_info::get_pointsizes(float *dst, int count, int start)
{
	get_points(pointsizes, dst, count, start, 1);
}

void Particle::shape_info::get_texcoords(float *dst, int count, int start)
{
	get_points(texcoords, dst, count, start, 2);
}

/* NOTE: bug: reads `positions` instead of `axes`. */
void Particle::shape_info::get_axes(float *dst, int count, int start)
{
	get_points(positions, dst, count, start, 6);
}

void Particle::shape_info::get_points(const vector<float> &src, float *dst, int count, int start, int comps)
{
	int n = count / comps;
	if (start < 0) {
		n += start;
		dst += (-start) * comps;
		start = 0;
	}
	int avail = (int)(src.size() / (unsigned)comps) - start;
	if (avail < n)
		n = avail;
	if (n > 0) {
		const float *s = src.begin() + start * comps;
		for (int i = 0; i < n; i++) {
			for (int c = 0; c < comps; c++)
				*dst++ = *s++;
		}
	}
}
