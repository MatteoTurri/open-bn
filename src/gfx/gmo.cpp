/*
 * GMO model loader: chunk parsing, copy, update, skinning and drawing of the
 * gmo_* classes declared in gmo.h. Large functions come first, then the small
 * accessors of every class, then the file-local helpers.
 */

#include <stdio.h>
#include <malloc.h>
#define GMO_IMPLEMENTATION
#include "gmo.h"
#include "vec.h"
#include "vecmath.h"
#include "gfx/gm_graph.h"

static GmoArrays *gmo_load_arrays(GmoChunk *chunk);
static GmoFCurve *gmo_load_fcurve(GmoChunk *chunk);

/* Identity, used by gmo_bone::activate when a blend index does not resolve to a bone. */
static mat4 gmo_null_matrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 gmo_null_row3(0.0f, 0.0f, 0.0f, 1.0f);

static void gmo_apply_curve(GmoFCurve *blob, float frame, float *out);

static int gmo_read_file(const char *name, void **buf, int *len);

static void gmo_lerp(float *dst, const float *src, int n, float t);

/* Bodies of gmo_bone::restore and gmo_material::restore, also expanded in
   gmo_motion::stop. */
static inline void gmo_restore_time(gmo_model *m, float t)
{
	if (t <= 0.0f)
		t = 0.001f;
	if (m->transition_time < t)
		m->transition_time = t;
}

static inline void gmo_restore_bone(gmo_bone *b, float t)
{
	if (b->src_bone) {
		b->flags &= ~GMO_BONE_HELD(GMO_BONE_FIELDS);
		if (t <= 0.0f)
			t = 0.001f;
		b->transition_time = t;
		gmo_restore_time(b->model, t);
		b->visible_parts = b->src_bone->visible_parts;
	}
}

static inline void gmo_restore_material(gmo_material *m, float t)
{
	if (m->src_material) {
		m->transition_flags &= 0xff00ffff;
		if (t <= 0.0f)
			t = 0.001f;
		m->transition_time = t;
		gmo_restore_time(m->model, t);
	}
}

/* Bounds-checked element access: 0 when the index is out of range. */
static inline mat4 gmo_multiply(const mat4 &a, const mat4 &b)
{
	return multiply(a, b);
}

static inline mat4 gmo_quat_matrix(const vec4 &q)
{
	return mat4_quat(q.x, q.y, q.z, q.w);
}

/* Rows 0-2 normalized, row 3 = (0, 0, 0, 1). */
static inline mat4 gmo_normalize(const mat4 &m)
{
	return mat4(normalize(m.m0), normalize(m.m1), normalize(m.m2), gmo_null_row3);
}

/* Rotation matrix of the quaternion (x, y, z, w), same formula as mat4_quat. */
static inline mat4 gmo_quat_to_matrix(float x, float y, float z, float w)
{
	float x2 = x + x;
	float w2 = w + w;
	float z2 = z + z;
	float y2 = y + y;
	return mat4(1.0f - y2 * y - z2 * z, x2 * y + w2 * z, z2 * x - w2 * y, 0.0f, x2 * y - w2 * z,
	    1.0f - x2 * x - z2 * z, y2 * z + w2 * x, 0.0f, z2 * x + w2 * y, y2 * z - w2 * x, 1.0f - x2 * x - y2 * y,
	    0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

/* Rotation by `angle` around `axis` (normalized here). */
static inline mat4 gmo_rotate_matrix(float angle, const vec4 &axis)
{
	float half = angle * 0.5f;
	vec4 v = normalize(axis) * sinf(half);
	return gmo_quat_to_matrix(v.x, v.y, v.z, cosf(half));
}

static inline gmo_bone *gmo_lookup_bone(const gmo_model *m, int i)
{
	return (0 <= i && i < (int)m->bones.size()) ? m->bones[i] : 0;
}

static inline gmo_part *gmo_lookup_part(const gmo_model *m, int i)
{
	return (0 <= i && i < (int)m->parts.size()) ? m->parts[i] : 0;
}

static inline gmo_motion *gmo_lookup_motion(const gmo_model *m, int i)
{
	return (0 <= i && i < (int)m->motions.size()) ? m->motions[i] : 0;
}

static inline gmo_material *gmo_lookup_material(const gmo_model *m, int i)
{
	return (0 <= i && i < (int)m->materials.size()) ? m->materials[i] : 0;
}

static inline GmoArrays *gmo_lookup_arrays(const gmo_part *p, int i)
{
	return (0 <= i && i < (int)p->vertex_arrays.size()) ? p->vertex_arrays[i] : 0;
}

/* The sibling after a chunk and its first sub-chunk. */
static inline GmoChunk *gmo_next_chunk(GmoChunk *c)
{
	return (GmoChunk *)((char *)c + c->size);
}

static inline GmoChunk *gmo_first_child(GmoChunk *c)
{
	return (GmoChunk *)((char *)c + c->first_child);
}

gmo_model::gmo_model(gm_graph &g)
{
	int none = -1;

	state = 0;
	graph = &g;
	option_flags = 0;
	world_matrix.m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	world_matrix.m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	world_matrix.m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	world_matrix.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	for (int i = 3; i >= 0; i--)
		current_motion[i] = none;
	transition_time = 0.0f;
	global_material = new gmo_material(this, true);
	bounding_box = 0;
}

gmo_model::~gmo_model()
{
	if (global_material)
		delete global_material;
	set_bounding_box(0);
	unload();
}

/* Deep copy of the model. */
void gmo_model::copy(const gmo_model &src)
{
	state = src.state;
	graph = src.graph;
	global_material = new gmo_material(this, false);
	global_material->copy(*src.global_material);
	set_bounding_box(src.bounding_box);
	option_flags = src.option_flags;
	world_matrix = src.world_matrix;
	for (int i = 0; i < 4; i++)
		current_motion[i] = src.current_motion[i];
	transition_time = src.transition_time;
	unload();
	for (int i = 0; i < (int)src.bones.size(); i++) {
		gmo_bone *p = new gmo_bone(this);
		p->copy(*((0 <= i && i < (int)src.bones.size()) ? src.bones[i] : 0));
		bones.push_back(p);
	}
	for (int i = 0; i < (int)src.parts.size(); i++) {
		gmo_part *p = new gmo_part(this);
		p->copy(*((0 <= i && i < (int)src.parts.size()) ? src.parts[i] : 0));
		parts.push_back(p);
	}
	for (int i = 0; i < (int)src.materials.size(); i++) {
		gmo_material *p = new gmo_material(this, false);
		p->copy(*((0 <= i && i < (int)src.materials.size()) ? src.materials[i] : 0));
		materials.push_back(p);
	}
	for (int i = 0; i < (int)src.textures.size(); i++) {
		gmo_texture *p = new gmo_texture(this);
		p->copy(*((0 <= i && i < (int)src.textures.size()) ? src.textures[i] : 0));
		textures.push_back(p);
	}
	for (int i = 0; i < (int)src.motions.size(); i++) {
		gmo_motion *p = new gmo_motion(this);
		p->copy(*((0 <= i && i < (int)src.motions.size()) ? src.motions[i] : 0));
		motions.push_back(p);
	}
}

/* Deletes the contents of the five vectors and empties them. */
void gmo_model::unload()
{
	for (int i = 0; i < (int)bones.size(); i++)
		if (bones[i])
			delete bones[i];
	for (int i = 0; i < (int)parts.size(); i++)
		if (parts[i])
			delete parts[i];
	for (int i = 0; i < (int)materials.size(); i++)
		if (materials[i])
			delete materials[i];
	for (int i = 0; i < (int)textures.size(); i++)
		if (textures[i])
			delete textures[i];
	for (int i = 0; i < (int)motions.size(); i++)
		if (motions[i])
			delete motions[i];
	bones.erase(bones.begin(), bones.end());
	parts.erase(parts.begin(), parts.end());
	materials.erase(materials.begin(), materials.end());
	textures.erase(textures.begin(), textures.end());
	motions.erase(motions.begin(), motions.end());
}

/* Sub-chunks: bones, parts, materials, textures, motions and the bounding box. */
int gmo_model::load(GmoChunk *chunk)
{
	char *cursor = (char *)chunk;

	unload();

	char *end = cursor + chunk->size;
	char *first = (chunk->tag & GMO_CHUNK_CONTAINER) ? cursor + chunk->first_child : end;

	cursor = first;
	while (cursor < end) {
		GmoChunk *c = (GmoChunk *)cursor;
		switch (c->tag & ~GMO_CHUNK_CONTAINER) {
		case GMO_BLOCK_BONE: {
			gmo_bone *p = new gmo_bone(this);
			p->load(c);
			bones.push_back(p);
			break;
		}
		case GMO_BLOCK_PART: {
			gmo_part *p = new gmo_part(this);
			p->load(c);
			parts.push_back(p);
			break;
		}
		case GMO_BLOCK_MATERIAL: {
			gmo_material *p = new gmo_material(this, false);
			p->load(c);
			materials.push_back(p);
			break;
		}
		case GMO_BLOCK_TEXTURE: {
			gmo_texture *p = new gmo_texture(this);
			p->load(c);
			textures.push_back(p);
			break;
		}
		case GMO_BLOCK_MOTION: {
			gmo_motion *p = new gmo_motion(this);
			p->load(c);
			motions.push_back(p);
			break;
		}
		case GMO_CMD_BOUNDING_BOX:
			set_bounding_box(
			    (const GmoBoundingBox *)((c->tag & GMO_CHUNK_CONTAINER) ? cursor + 16 : cursor + 8));
			break;
		}
		cursor += c->size;
	}
	reconfigure();
	return 1;
}

void gmo_model::reconfigure()
{
	for (int i = 0; i < (int)bones.size(); i++)
		bones[i]->reconfigure();
	for (int i = 0; i < (int)parts.size(); i++)
		parts[i]->reconfigure();
	for (int i = 0; i < (int)materials.size(); i++)
		materials[i]->reconfigure();
	for (int i = 0; i < (int)textures.size(); i++)
		textures[i]->reconfigure();
	for (int i = 0; i < (int)motions.size(); i++)
		motions[i]->reconfigure();
}

/* Advances the 4 active motions, then the pending transition time (transition_time),
   then bones and materials. */
void gmo_model::update(float dt)
{
	for (int i = 0; i < 4; i++) {
		gmo_motion *m = gmo_lookup_motion(this, current_motion[i]);
		if (m)
			m->update(dt);
	}
	if (transition_time == 0.0f)
		return;
	transition_time -= dt;
	if (transition_time < 0.0f)
		transition_time = 0.0f;
	for (int i = 0; i < (int)bones.size(); i++)
		bones[i]->update(dt);
	for (int i = 0; i < (int)materials.size(); i++)
		materials[i]->update(dt);
}

void gmo_model::set_graph(gm_graph &g)
{
	graph = &g;
}

gm_graph &gmo_model::get_graph() const
{
	return *graph;
}

gmo_material &gmo_model::get_global_material() const
{
	return *global_material;
}

GmoBoundingBox &gmo_model::get_bounding_box() const
{
	return *bounding_box;
}

void gmo_model::set_world_matrix(const mat4 &m)
{
	world_matrix = m;
}

mat4 &gmo_model::get_world_matrix() const
{
	return ((gmo_model *)this)->world_matrix;
}

int gmo_model::get_current_motion(int i)
{
	return current_motion[i];
}

void gmo_model::set_option_flags(int flags)
{
	option_flags |= flags;
}

void gmo_model::clear_option_flags(int flags)
{
	option_flags &= ~flags;
}

int gmo_model::get_option_flags() const
{
	return option_flags;
}

gmo_bone *gmo_model::get_bone(int i) const
{
	return (0 <= i && i < (int)bones.size()) ? bones[i] : 0;
}

gmo_part *gmo_model::get_part(int i) const
{
	return (0 <= i && i < (int)parts.size()) ? parts[i] : 0;
}

gmo_material *gmo_model::get_material(int i) const
{
	return (0 <= i && i < (int)materials.size()) ? materials[i] : 0;
}

gmo_texture *gmo_model::get_texture(int i) const
{
	return (0 <= i && i < (int)textures.size()) ? textures[i] : 0;
}

gmo_motion *gmo_model::get_motion(int i) const
{
	return (0 <= i && i < (int)motions.size()) ? motions[i] : 0;
}

int gmo_model::get_bone_count() const
{
	return bones.size();
}

int gmo_model::get_part_count() const
{
	return parts.size();
}

int gmo_model::get_material_count() const
{
	return materials.size();
}

int gmo_model::get_texture_count() const
{
	return textures.size();
}

int gmo_model::get_motion_count() const
{
	return motions.size();
}

/* Reads the file with gmo_read_file and loads it from memory. */
int gmo_model::load_from_file(const char *name, int index)
{
	void *buf;
	int len;

	if (!gmo_read_file(name, &buf, &len)) {
		printf("cannot load \"%s\" ( file not found )\n", name);
		return 0;
	}
	int r = load_from_file(buf, len, index, name);
	if (buf)
		delete[] (char *)buf;
	return r;
}

/* Checks the header (0x2e474d4f = '.GMO', 0x312e3030 = '1.00', compared as
   ints) and loads the index-th GMO_BLOCK_MODEL chunk. */
int gmo_model::load_from_file(const void *data, int size, int index, const char *name)
{
	if (!name || !*name)
		name = "????";
	if (*(const int *)data != 0x2e474d4f) {
		printf("cannot load \"%s\" ( wrong signature )\n", name);
		return 0;
	}
	if (((const int *)data)[1] != 0x312e3030) {
		printf("cannot load \"%s\" ( wrong version )\n", name);
		return 0;
	}

	char *cursor = (char *)data + 16;
	char *end = cursor + ((GmoChunk *)cursor)->size;

	if (((GmoChunk *)cursor)->tag & GMO_CHUNK_CONTAINER)
		cursor += ((GmoChunk *)cursor)->first_child;
	else
		cursor = end;
	while (cursor < end) {
		GmoChunk *k = (GmoChunk *)cursor;
		if ((k->tag & ~GMO_CHUNK_CONTAINER) == GMO_BLOCK_MODEL) {
			if (--index < 0) {
				printf("load \"%s\" ( %d bytes )\n", name, k->size);
				return load(k);
			}
		}
		cursor += k->size;
	}
	printf("cannot load \"%s\" ( wrong index )\n", name);
	return 0;
}

void gmo_model::calc()
{
	if (state & GMO_STATE_RECONFIGURE) {
		state &= ~GMO_STATE_RECONFIGURE;
		reconfigure();
	}
	world_matrix = *graph->GetMatrix(GM_MATRIX_MODEL);
	for (int i = 0; i < (int)bones.size(); i++)
		bones[i]->calc();
}

void gmo_model::draw()
{
	gm_graph *g = graph;

	current_bone = 0;
	active_bone = 0;
	active_material = -2;
	pending_flags = -1;
	pending_mask = -1;
	enable_bits = g->fFlagsE;
	g->PushMatrix(GM_MATRIX_MODEL);
	for (int i = 0; i < (int)bones.size(); i++)
		bones[i]->draw();
	g->PopMatrix(GM_MATRIX_MODEL);
	g->SetEnableBits(enable_bits);
}

void gmo_model::set_bounding_box(const GmoBoundingBox *box)
{
	if (box) {
		if (!bounding_box)
			bounding_box = new GmoBoundingBox;
		*bounding_box = *box;
	} else {
		delete bounding_box;
		bounding_box = 0;
	}
}

/* Stops the motion in the slot, stores the new index and starts the new motion. */
void gmo_model::set_current_motion(int slot, int idx, float t, float u)
{
	if ((unsigned)slot < 4) {
		int cur = current_motion[slot];
		gmo_motion *m = (0 <= cur && cur < (int)motions.size()) ? motions[cur] : 0;
		if (m)
			m->stop(u);
		current_motion[slot] = idx;
		gmo_motion *m2 = (0 <= idx && idx < (int)motions.size()) ? motions[idx] : 0;
		if (m2)
			m2->start(t, u);
	}
}

void gmo_model::set_mapper(int index, const mat4 *m)
{
	if ((unsigned)index < 4) {
		int bit = GMO_STATE_MAPPER0 << index;
		if (m) {
			state |= bit;
			mapper[index] = *m;
		} else {
			state &= ~bit;
		}
	}
}

/* Clamps t to at least 0.001 and keeps the longest pending transition time
   in transition_time. */
void gmo_model::restore(float t)
{
	gmo_restore_time(this, t);
}

void gmo_model::notify_configure()
{
	state |= GMO_STATE_RECONFIGURE;
}

gmo_bone::gmo_bone(gmo_model *m)
{
	model = m;
	flags = 0;
	src_bone = 0;
	parent = -1;
	visible_parts = 0;
	transition_time = 0.0f;
	bounding_box = 0;
	mult_matrix.m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	mult_matrix.m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	mult_matrix.m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	mult_matrix.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	translate.Set(0.0f, 0.0f, 0.0f, 0.0f);
	rotate.Set(0.0f, 0.0f, 0.0f, 0.0f);
	scale.Set(1.0f, 1.0f, 1.0f, 1.0f);
	inherited_scale.Set(1.0f, 1.0f, 1.0f, 1.0f);
	world_mat.m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	world_mat.m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	world_mat.m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	world_mat.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	scaled_world_mat.m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	scaled_world_mat.m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	scaled_world_mat.m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	scaled_world_mat.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	world_offset.m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	world_offset.m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	world_offset.m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	world_offset.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	local_offset.m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	local_offset.m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	local_offset.m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	local_offset.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
}

gmo_bone::~gmo_bone()
{
	set_bounding_box(0);
}

void gmo_bone::copy(const gmo_bone &src)
{
	src_bone = &src;
	flags = src.flags;
	parent = src.parent;
	visible_parts = src.visible_parts;
	transition_time = src.transition_time;
	set_bounding_box(src.bounding_box);
	mult_matrix = src.mult_matrix;
	translate = src.translate;
	rotate = src.rotate;
	scale = src.scale;
	inherited_scale = src.inherited_scale;
	world_mat = src.world_mat;
	scaled_world_mat = src.scaled_world_mat;
	world_offset = src.world_offset;
	local_offset = src.local_offset;
	part_indices = src.part_indices;
	morph_weights = src.morph_weights;
	blend_bones = src.blend_bones;
	blend_offsets = src.blend_offsets;
}

/* Sub-chunks: the bone commands. GMO_CMD_BLEND_BONES fills blend indices and matrices (two parallel
   vectors), the two ROTATE_* build the quaternion from half angles, SCALE and SCALE_INHERIT clear and set
   flag 0x10. */
int gmo_bone::load(GmoChunk *chunk)
{
	char *cursor = (char *)chunk;
	int found = 0;
	int sawMorph = 0;
	char *end = cursor + chunk->size;
	char *first = (chunk->tag & GMO_CHUNK_CONTAINER) ? cursor + chunk->first_child : end;

	cursor = first;
	while (cursor < end) {
		GmoChunk *c = (GmoChunk *)cursor;
		char *data = (c->tag & GMO_CHUNK_CONTAINER) ? cursor + 0x10 : cursor + 8;
		switch (c->tag & ~GMO_CHUNK_CONTAINER) {
		case GMO_CMD_BOUNDING_BOX:
			set_bounding_box((const GmoBoundingBox *)data);
			break;
		case GMO_CMD_PARENT_BONE:
			parent = *(const short *)data;
			break;
		case GMO_CMD_VISIBILITY:
			visible_parts = *(const int *)data;
			found = 1;
			break;
		case GMO_CMD_MORPH_WEIGHTS:
			set_morph_weights((const float *)(data + 4), *(const int *)data);
			break;
		case GMO_CMD_BLEND_BONES: {
			int n = *(const int *)data;
			blend_bones.resize(n);
			for (int i = 0; i < n; i++) {
				const int *idx = (const int *)(data + 4);
				blend_bones[i] = (short)idx[i];
			}
			const mat4 *mats = (const mat4 *)(data + 4 + *(const int *)data * 4);
			if (cursor + c->size < (char *)(mats + n))
				break;
			blend_offsets.resize(n);
			for (int i = 0; i < n; i++)
				blend_offsets[i] = *mats++;
			break;
		}
		case GMO_CMD_MULT_MATRIX:
			set_mult_matrix(*(const mat4 *)data);
			break;
		case GMO_CMD_TRANSLATE:
			set_translate(*(const vec4 *)data);
			break;
		case GMO_CMD_ROTATE_ZYX: {
			float hx = ((const float *)data)[0] * 0.5f;
			float hy = ((const float *)data)[1] * 0.5f;
			float hz = ((const float *)data)[2] * 0.5f;
			float cx = cosf(hx);
			float sx = sinf(hx);
			float cy = cosf(hy);
			float sy = sinf(hy);
			float cz = cosf(hz);
			float sz = sinf(hz);
			vec4 q;
			q.x = cz * cy * sx - cx * sz * sy;
			q.y = cx * cz * sy + cy * sz * sx;
			q.z = cx * cy * sz - cz * sy * sx;
			q.w = cz * cy * cx + sz * sy * sx;
			set_rotate(q);
			break;
		}
		case GMO_CMD_ROTATE_YXZ: {
			float hx = ((const float *)data)[0] * 0.5f;
			float hy = ((const float *)data)[1] * 0.5f;
			float hz = ((const float *)data)[2] * 0.5f;
			float cx = cosf(hx);
			float sx = sinf(hx);
			float cy = cosf(hy);
			float sy = sinf(hy);
			float cz = cosf(hz);
			float sz = sinf(hz);
			vec4 q;
			q.x = cy * cz * sx + sy * cx * sz;
			q.y = sy * cx * cz - cy * sx * sz;
			q.z = cy * cx * sz - sy * cz * sx;
			q.w = cy * cx * cz + sy * sx * sz;
			set_rotate(q);
			break;
		}
		case GMO_CMD_ROTATE:
			set_rotate(*(const vec4 *)data);
			break;
		case GMO_CMD_SCALE:
			set_scale(*(const vec4 *)data);
			flags &= ~GMO_BONE_SCALE_INHERIT;
			break;
		case GMO_CMD_SCALE_INHERIT:
			set_scale(*(const vec4 *)data);
			flags |= GMO_BONE_SCALE_INHERIT;
			break;
		case GMO_CMD_DRAW_PART:
			part_indices.push_back(*(const short *)data);
			sawMorph = 1;
			break;
		}
		cursor += c->size;
	}

	if (!found)
		visible_parts = sawMorph ? -1 : 0;
	return 1;
}

/* Spherical linear interpolation of two quaternions, along the shorter path. */
static inline vec4 gmo_slerp(const vec4 *r, const vec4 *sr, float t)
{
	float dot = r->x * sr->x + r->y * sr->y + r->z * sr->z + r->w * sr->w;
	int neg = 0;

	if (dot < 0.0f) {
		dot = -dot;
		neg = 1;
	}
	float ang = acosf(dot);
	float sn = sinf(ang);
	if (sn == 0.0f)
		return *sr;
	float a = sinf((1.0f - t) * ang) / sn;
	float b = sinf(t * ang) / sn;
	if (neg)
		b = -b;
	float qx = r->x * a + sr->x * b;
	float qy = r->y * a + sr->y * b;
	float qz = r->z * a + sr->z * b;
	float qw = r->w * a + sr->w * b;
	return vec4(qx, qy, qz, qw);
}

/* transition_time is the remaining transition time; GMO_BONE_CHANGED marks the fields
   that differ from src_bone, GMO_BONE_HELD the ones a motion holds (not restored). Rotation is interpolated with
   gmo_slerp, the rest with gmo_lerp. */
void gmo_bone::update(float dt)
{
	if (transition_time == 0.0f)
		return;

	int done = ~(flags >> 8) & GMO_BONE_CHANGED(GMO_BONE_FIELDS);
	int mask = flags & done;

	if (!mask) {
		transition_time = 0.0f;
		return;
	}

	float t = dt / transition_time;
	transition_time -= dt;
	if (transition_time < 0.0f) {
		transition_time = 0.0f;
		flags &= (flags >> 8) | ~GMO_BONE_CHANGED(GMO_BONE_FIELDS);
		t = 1.0f;
	}

	if (mask & GMO_BONE_CHANGED(GMO_BONE_MATRIX))
		gmo_lerp(&mult_matrix.m0.x, &src_bone->mult_matrix.m0.x, 16, t);
	if (mask & GMO_BONE_CHANGED(GMO_BONE_TRANSLATE))
		gmo_lerp(&translate.x, &src_bone->translate.x, 3, t);
	if (mask & GMO_BONE_CHANGED(GMO_BONE_ROTATE))
		rotate = gmo_slerp(&rotate, &src_bone->rotate, t);
	if (mask & GMO_BONE_CHANGED(GMO_BONE_SCALE))
		gmo_lerp(&scale.x, &src_bone->scale.x, 3, t);
	if (mask & GMO_BONE_CHANGED(GMO_BONE_MORPH))
		gmo_lerp(&morph_weights[0], &src_bone->morph_weights[0], src_bone->morph_weights.size(), t);
}

/* World matrix of the bone: the parent's (or the model's world_matrix),
   times mult_matrix if GMO_BONE_MATRIX is set, times the local rotation,
   translation and scale. GMO_BONE_INHERITED_SCALE makes the scale inherited, kept in inherited_scale. */
void gmo_bone::calc()
{
	flags |= GMO_BONE_OFFSET_DIRTY | GMO_BONE_SCALED_DIRTY;

	gmo_bone *p = gmo_lookup_bone(model, parent);
	int inh = 0;

	if (!p) {
		world_mat = model->world_matrix;
		flags &= ~GMO_BONE_INHERITED_SCALE;
	} else {
		world_mat = p->world_mat;
		inh = p->flags & GMO_BONE_INHERITED_SCALE;
		flags |= inh;
		if (inh)
			inherited_scale = p->inherited_scale;
	}

	if (flags & GMO_BONE_MATRIX)
		world_mat = gmo_multiply(world_mat, mult_matrix);

	if (!(flags & (GMO_BONE_TRANSLATE | GMO_BONE_ROTATE | GMO_BONE_SCALE)))
		return;

	mat4 m;

	if (flags & GMO_BONE_ROTATE)
		m = gmo_quat_matrix(rotate);
	else
		m = gmo_null_matrix;

	if (flags & GMO_BONE_TRANSLATE) {
		if (!inh) {
			m.m3.x = translate.x;
			m.m3.y = translate.y;
			m.m3.z = translate.z;
		} else {
			m.m3.x = translate.x * inherited_scale.x;
			m.m3.y = translate.y * inherited_scale.y;
			m.m3.z = translate.z * inherited_scale.z;
		}
	}

	if (flags & GMO_BONE_SCALE) {
		if (flags & GMO_BONE_SCALE_INHERIT) {
			if (!inh) {
				inherited_scale.x = scale.x;
				inherited_scale.y = scale.y;
				inherited_scale.z = scale.z;
			} else {
				inherited_scale.x *= scale.x;
				inherited_scale.y *= scale.y;
				inherited_scale.z *= scale.z;
			}
			flags |= GMO_BONE_INHERITED_SCALE;
		} else {
			m.m0 *= scale.x;
			m.m1 *= scale.y;
			m.m2 *= scale.z;
		}
	}

	world_mat = gmo_multiply(world_mat, m);
}

/* Loads the bone matrix into gm_graph: a single LoadMatrix without blend
   indices, otherwise an array of get_world_offset() of 1-4 bones. */
void gmo_bone::activate()
{
	const mat4 *bufs[8];
	int n = (int)blend_bones.size();
	gm_graph *g = model->graph;

	if (n == 0) {
		g->LoadMatrix(GM_MATRIX_MODEL, get_world_matrix());
		return;
	}
	if (n >= 5)
		return;
	for (int i = 0; i < n; i++) {
		int idx = blend_bones[i];
		gmo_bone *b = gmo_lookup_bone(model, idx);
		bool missing = (b == 0);
		bufs[i] = missing ? &gmo_null_matrix : &b->get_world_offset();
	}
	g->SetBlendMatrices(bufs, n);
}

/* Publishes the morph weights and blend matrices, then draws the parts listed
   in part_indices whose bit is set in visible_parts. The model's enable_bits is saved and
   restored around it. */
void gmo_bone::draw()
{
	if (!visible_parts)
		return;

	int f = model->state;
	if (f & GMO_STATE_MAPPERS) {
		if (f & GMO_STATE_MAPPER0)
			model->pending_flags |= 0x1000000;
		if (f & (GMO_STATE_MAPPER0 << 1))
			model->pending_mask |= 0x100;
		if (f & (GMO_STATE_MAPPER0 << 2))
			model->pending_mask |= 0x40000;
		if (f & (GMO_STATE_MAPPER0 << 3))
			model->pending_mask |= 0x10000000;
		model->active_material = -2;
	}

	int n = (int)morph_weights.size();
	gm_graph *g = model->graph;
	int saved = model->enable_bits;
	if (n > 0) {
		g->SetMorphWeights(&morph_weights[0], n);
		model->enable_bits = saved | GM_ENABLE_VERTEX_BLEND;
	}

	n = (int)blend_offsets.size();
	if (n > 0) {
		for (int i = 0; i < n; i++) {
			gmo_bone *b = gmo_lookup_bone(model, blend_bones[i]);
			if (b)
				b->set_local_offset(blend_offsets[i]);
		}
		model->enable_bits = saved | GM_ENABLE_VERTEX_BLEND;
	}

	model->current_bone = this;
	for (int i = 0; i < (int)part_indices.size(); i++) {
		if ((visible_parts >> i) & 1) {
			gmo_part *p = gmo_lookup_part(model, part_indices[i]);
			if (p)
				p->draw();
		}
	}
	model->enable_bits = saved;
}

/* world_mat without GMO_BONE_INHERITED_SCALE, otherwise scaled_world_mat (rebuilt on GMO_BONE_SCALED_DIRTY):
   world_mat with rows 0-2 scaled by the x/y/z of inherited_scale. */
mat4 &gmo_bone::get_world_matrix() const
{
	gmo_bone *self = (gmo_bone *)this;

	if (!(self->flags & GMO_BONE_INHERITED_SCALE))
		return self->world_mat;
	if (self->flags & GMO_BONE_SCALED_DIRTY) {
		self->flags &= ~GMO_BONE_SCALED_DIRTY;
		self->scaled_world_mat.m0 = self->world_mat.m0 * self->inherited_scale.x;
		self->scaled_world_mat.m1 = self->world_mat.m1 * self->inherited_scale.y;
		self->scaled_world_mat.m2 = self->world_mat.m2 * self->inherited_scale.z;
		self->scaled_world_mat.m3 = self->world_mat.m3;
	}
	return self->scaled_world_mat;
}

/* With t > 0 everything goes to animate_interp; otherwise the sample is
   assigned. The two ROTATE_* build the quaternion from half angles with two different
   axis orders.
   The field becomes set, changed and held (GMO_BONE_ANIMATED). */
void gmo_bone::animate(int tag, int index, float *data, int count, float t)
{
	if (t > 0.0f) {
		animate_interp(tag, index, data, count, t);
		return;
	}

	switch (tag) {
	case GMO_CMD_TRANSLATE:
		translate.Set(data[0], data[1], data[2], 1.0f);
		flags |= GMO_BONE_ANIMATED(GMO_BONE_TRANSLATE);
		break;
	case GMO_CMD_ROTATE:
		rotate.Set(data[0], data[1], data[2], data[3]);
		flags |= GMO_BONE_ANIMATED(GMO_BONE_ROTATE);
		break;
	case GMO_CMD_ROTATE_ZYX: {
		float hx = data[0] * 0.5f;
		float hy = data[1] * 0.5f;
		float hz = data[2] * 0.5f;
		float cx = cosf(hx);
		float sx = sinf(hx);
		float cy = cosf(hy);
		float sy = sinf(hy);
		float cz = cosf(hz);
		float sz = sinf(hz);
		vec4 q;
		q.x = cz * cy * sx - cx * sz * sy;
		q.y = cx * cz * sy + cy * sz * sx;
		q.z = cx * cy * sz - cz * sy * sx;
		q.w = cz * cy * cx + sz * sy * sx;
		rotate = q;
		flags |= GMO_BONE_ANIMATED(GMO_BONE_ROTATE);
		break;
	}
	case GMO_CMD_ROTATE_YXZ: {
		float hx = data[0] * 0.5f;
		float hy = data[1] * 0.5f;
		float hz = data[2] * 0.5f;
		float cx = cosf(hx);
		float sx = sinf(hx);
		float cy = cosf(hy);
		float sy = sinf(hy);
		float cz = cosf(hz);
		float sz = sinf(hz);
		vec4 q;
		q.x = cy * cz * sx + sy * cx * sz;
		q.y = sy * cx * cz - cy * sx * sz;
		q.z = cy * cx * sz - sy * cz * sx;
		q.w = cy * cx * cz + sy * sx * sz;
		rotate = q;
		flags |= GMO_BONE_ANIMATED(GMO_BONE_ROTATE);
		break;
	}
	case GMO_CMD_SCALE:
		scale.Set(data[0], data[1], data[2], 1.0f);
		flags = (flags | GMO_BONE_ANIMATED(GMO_BONE_SCALE)) & ~GMO_BONE_SCALE_INHERIT;
		break;
	case GMO_CMD_SCALE_INHERIT:
		scale.Set(data[0], data[1], data[2], 1.0f);
		flags |= GMO_BONE_ANIMATED(GMO_BONE_SCALE) | GMO_BONE_SCALE_INHERIT;
		break;
	case GMO_CMD_MULT_MATRIX:
		mult_matrix = *(const mat4 *)data;
		flags |= GMO_BONE_ANIMATED(GMO_BONE_MATRIX);
		break;
	case GMO_CMD_MORPH_WEIGHTS:
		set_morph_weights(data, count);
		flags |= GMO_BONE_CHANGED(GMO_BONE_MORPH) | GMO_BONE_HELD(GMO_BONE_MORPH);
		break;
	case GMO_CMD_VISIBILITY:
		visible_parts = (int)(data[0] + 0.5f);
		break;
	}
}

/* At most 16 weights; the vector only grows. */
void gmo_bone::set_morph_weights(const float *w, int n)
{
	if (n > 16)
		n = 16;

	if ((int)morph_weights.size() < n)
		morph_weights.resize(n);
	for (int i = 0; i < n; i++)
		morph_weights[i] = w[i];
}

/* Like animate(), interpolating: gmo_lerp on the fields, gmo_slerp on rotations. */
void gmo_bone::animate_interp(int tag, int index, float *data, int count, float t)
{
	switch (tag) {
	case GMO_CMD_TRANSLATE:
		gmo_lerp(&translate.x, data, 3, t);
		flags |= GMO_BONE_ANIMATED(GMO_BONE_TRANSLATE);
		break;
	case GMO_CMD_ROTATE:
		rotate = gmo_slerp(&rotate, (const vec4 *)data, t);
		flags |= GMO_BONE_ANIMATED(GMO_BONE_ROTATE);
		break;
	case GMO_CMD_ROTATE_ZYX: {
		float hx = data[0] * 0.5f;
		float hy = data[1] * 0.5f;
		float hz = data[2] * 0.5f;
		float cx = cosf(hx);
		float sx = sinf(hx);
		float cy = cosf(hy);
		float sy = sinf(hy);
		float cz = cosf(hz);
		float sz = sinf(hz);
		vec4 e;
		e.x = cz * cy * sx - cx * sz * sy;
		e.y = cx * cz * sy + cy * sz * sx;
		e.z = cx * cy * sz - cz * sy * sx;
		e.w = cz * cy * cx + sz * sy * sx;
		rotate = gmo_slerp(&rotate, &e, t);
		flags |= GMO_BONE_ANIMATED(GMO_BONE_ROTATE);
		break;
	}
	case GMO_CMD_ROTATE_YXZ: {
		float hx = data[0] * 0.5f;
		float hy = data[1] * 0.5f;
		float hz = data[2] * 0.5f;
		float cx = cosf(hx);
		float sx = sinf(hx);
		float cy = cosf(hy);
		float sy = sinf(hy);
		float cz = cosf(hz);
		float sz = sinf(hz);
		vec4 e;
		e.x = cy * cz * sx + sy * cx * sz;
		e.y = sy * cx * cz - cy * sx * sz;
		e.z = cy * cx * sz - sy * cz * sx;
		e.w = cy * cx * cz + sy * sx * sz;
		rotate = gmo_slerp(&rotate, &e, t);
		flags |= GMO_BONE_ANIMATED(GMO_BONE_ROTATE);
		break;
	}
	case GMO_CMD_SCALE:
		gmo_lerp(&scale.x, data, 3, t);
		flags = (flags | GMO_BONE_ANIMATED(GMO_BONE_SCALE)) & ~GMO_BONE_SCALE_INHERIT;
		break;
	case GMO_CMD_SCALE_INHERIT:
		gmo_lerp(&scale.x, data, 3, t);
		flags |= GMO_BONE_ANIMATED(GMO_BONE_SCALE) | GMO_BONE_SCALE_INHERIT;
		break;
	case GMO_CMD_MULT_MATRIX:
		gmo_lerp(&mult_matrix.m0.x, data, 16, t);
		flags |= GMO_BONE_ANIMATED(GMO_BONE_MATRIX);
		break;
	case GMO_CMD_MORPH_WEIGHTS:
		if ((int)morph_weights.size() < count)
			morph_weights.resize(count);
		gmo_lerp(&morph_weights[0], data, count, t);
		break;
	case GMO_CMD_VISIBILITY:
		visible_parts = (int)(data[0] + 0.5f);
		break;
	}
}

static int gmo_material_default_blend_func[4] = { GM_BLEND_ALPHA, GM_BLEND_ADD | GM_BLEND_DEST_ALPHA, GM_BLEND_OFF,
	GM_BLEND_OFF };
static int gmo_material_default_color_gen[4] = { 0, 1, 2, 2 };

gmo_part::~gmo_part()
{
	for (int i = 0; i < (int)meshes.size(); i++)
		if (meshes[i])
			delete meshes[i];
	for (int i = 0; i < (int)vertex_arrays.size(); i++)
		delete vertex_arrays[i];
}

void gmo_bone::reconfigure()
{}

void gmo_bone::set_bounding_box(const GmoBoundingBox *box)
{
	if (box) {
		if (!bounding_box)
			bounding_box = new GmoBoundingBox;
		*bounding_box = *box;
	} else {
		delete bounding_box;
		bounding_box = 0;
	}
}

/* Lazily rebuilt in world_offset, rebuilt while GMO_BONE_OFFSET_DIRTY is set. */
mat4 &gmo_bone::get_world_offset() const
{
	gmo_bone *self = (gmo_bone *)this;
	if (self->flags & GMO_BONE_OFFSET_DIRTY) {
		self->flags &= ~GMO_BONE_OFFSET_DIRTY;
		self->world_offset = multiply(get_world_matrix(), self->local_offset);
	}
	return self->world_offset;
}

void gmo_bone::set_local_offset(const mat4 &m)
{
	flags |= GMO_BONE_OFFSET_DIRTY;
	local_offset = m;
}

void gmo_bone::restore(float t)
{
	gmo_restore_bone(this, t);
}

void gmo_bone::set_mult_matrix(const mat4 &m)
{
	mult_matrix = m;
	flags |= GMO_BONE_MATRIX;
}

void gmo_bone::set_translate(const vec4 &v)
{
	translate = v;
	flags |= GMO_BONE_TRANSLATE;
}

void gmo_bone::set_rotate(const vec4 &v)
{
	rotate = v;
	flags |= GMO_BONE_ROTATE;
}

void gmo_bone::set_scale(const vec4 &v)
{
	scale = v;
	flags |= GMO_BONE_SCALE;
}

void gmo_bone::get_mult_matrix(mat4 &out) const
{
	out = mult_matrix;
}

void gmo_bone::get_translate(vec4 &out) const
{
	out = translate;
}

void gmo_bone::get_rotate(vec4 &out) const
{
	out = rotate;
}

void gmo_bone::get_scale(vec4 &out) const
{
	out = scale;
}

const std::vector<float> &gmo_bone::get_morph_weights() const
{
	return morph_weights;
}

gmo_part::gmo_part(gmo_model *m)
{
	model = m;
	unk_04 = 0;
}

static GmoArrays *gmo_load_arrays(GmoChunk *chunk);

void gmo_part::copy(const gmo_part &src)
{
	unk_04 = src.unk_04;
	for (int i = 0; i < (int)src.meshes.size(); i++) {
		gmo_mesh *m = new gmo_mesh(this);
		m->copy(*src.meshes[i]);
		meshes.push_back(m);
	}
}

/* GMO_BLOCK_MESH sub-chunks are meshes, created after the walk; GMO_BLOCK_ARRAYS sub-chunks are
   vertex blocks (gmo_load_arrays), freed once the meshes are loaded. */
int gmo_part::load(GmoChunk *chunk)
{
	std::vector<GmoChunk *> pending;
	GmoChunk *end = gmo_next_chunk(chunk);

	chunk = (chunk->tag & GMO_CHUNK_CONTAINER) ? gmo_first_child(chunk) : end;
	while (chunk < end) {
		int raw = chunk->tag;
		switch (raw & ~GMO_CHUNK_CONTAINER) {
		case GMO_BLOCK_MESH:
			pending.push_back(chunk);
			break;
		case GMO_BLOCK_ARRAYS: {
			GmoArrays *b = gmo_load_arrays(chunk);
			vertex_arrays.push_back(b);
			break;
		}
		}
		chunk = gmo_next_chunk(chunk);
	}

	for (int i = 0; i < (int)pending.size(); i++) {
		gmo_mesh *m = new gmo_mesh(this);
		m->load(pending[i]);
		meshes.push_back(m);
	}

	for (int j = 0; j < (int)vertex_arrays.size(); j++)
		delete vertex_arrays[j];
	vertex_arrays.erase(vertex_arrays.begin(), vertex_arrays.end());
	return 1;
}

/* Fills the mesh display list from the sub-chunks, the GMO_CMD_* of the mesh block. The data
   starts at chunk+16 for containers, chunk+8 otherwise. The four DRAW_* commands take a
   vertex array from the part's vertex_arrays (short index) and pass it to
   SetArrays; 0x60B and 0x60C set their shader_flags bit even when it does not
   resolve. */
int gmo_mesh::load(GmoChunk *chunk)
{
	gm_graph *g = model->graph;
	GmoChunk *end;

	if (!vb_handle)
		vb_handle = g->CreateList();
	g->ClearList(vb_handle);
	g->BeginList(vb_handle);

	end = gmo_next_chunk(chunk);
	{
		GmoChunk *first;

		if (chunk->tag & GMO_CHUNK_CONTAINER)
			first = gmo_first_child(chunk);
		else
			first = end;
		chunk = first;
	}

	for (; chunk < end; chunk = gmo_next_chunk(chunk)) {
		void *data;
		const int *w;

		if (!(chunk->tag & GMO_CHUNK_CONTAINER))
			data = (char *)chunk + 8;
		else
			data = (char *)chunk + 16;
		w = (const int *)data;

		switch (chunk->tag & ~GMO_CHUNK_CONTAINER) {
		case GMO_CMD_SET_MATERIAL:
			material_index = *(const short *)data;
			break;

		case GMO_CMD_BLEND_SUBSET: {
			int count = *w;
			int i;

			if (count > 5)
				count = 5;
			bone_slots.resize(count, 0);
			for (i = 0; i < count; i++)
				bone_slots[i] = w[1 + i];
			break;
		}

		case GMO_CMD_PARTICLE_SIZE:
			g->ParticleSize(*(const float *)data);
			break;
		case GMO_CMD_PARTICLE_CROP:
			g->ParticleCrop(*(const vec4 *)data);
			break;
		case GMO_CMD_PATCH_LEVEL:
			g->PatchLevel(*(const float *)data);
			break;
		case GMO_CMD_PATCH_SUBDIV:
			g->PatchSubdiv(((const float *)data)[0], ((const float *)data)[1]);
			break;
		case GMO_CMD_PATCH_CLIP:
			g->PatchClip(*(const vec4 *)data);
			break;
		case GMO_CMD_PATCH_CROP:
			g->PatchCrop(*(const vec4 *)data);
			break;

		case GMO_CMD_DRAW_ARRAYS: {
			GmoArrays *blob = gmo_lookup_arrays(part, *(const short *)data);
			if (!blob)
				break;
			g->SetArrays((const char *)blob + 16, blob->format, blob->vertex_count, blob->morph_count,
			    blob->morph_format, 0);
			g->DrawArrays(w[1], 0, w[2], w[3], (const short *)((const char *)data + 16));
			break;
		}

		case GMO_CMD_DRAW_MESH: {
			GmoArrays *blob = gmo_lookup_arrays(part, *(const short *)data);
			if (!blob)
				break;
			g->SetArrays((const char *)blob + 16, blob->format, blob->vertex_count, blob->morph_count,
			    blob->morph_format, 0);
			g->DrawMesh(w[1], 0, w[2], w[3], 0, (const short *)((const char *)data + 16));
			break;
		}

		case GMO_CMD_DRAW_PARTICLE: {
			GmoArrays *blob = gmo_lookup_arrays(part, *(const short *)data);
			if (blob) {
				g->SetArrays((const char *)blob + 16, blob->format, blob->vertex_count,
				    blob->morph_count, blob->morph_format, 0);
				g->DrawParticle(w[1], 0, w[2], w[3], (const short *)((const char *)data + 16));
			}
			shader_flags |= 1;
			break;
		}

		case GMO_CMD_DRAW_RECT_PATCH: {
			GmoArrays *blob = gmo_lookup_arrays(part, *(const short *)data);
			if (blob) {
				g->SetArrays((const char *)blob + 16, blob->format, blob->vertex_count,
				    blob->morph_count, blob->morph_format, 0);
				g->DrawRectPatch(w[1], w[2], 0, w[3], w[4], 0,
				    (const short *)((const char *)data + 20));
			}
			shader_flags |= 2;
			break;
		}
		}
	}

	g->EndList(0);
	return 1;
}

/* Three steps, each cached in the gmo_model: activate our material (falling
   back to the global one), activate the bone or pass the blend matrices,
   then shader and display list. Bone indices go through
   model->current_bone->blend_bones (unchecked), then gmo_lookup_bone. */
void gmo_mesh::draw()
{
	int n = material_index;

	if (model->active_material != n) {
		model->active_material = n;
		gmo_material *mat = gmo_lookup_material(model, n);
		if (!mat)
			mat = model->global_material;
		mat->activate();
	}

	n = bone_slots.end() - bone_slots.begin();
	gm_graph *g = model->graph;
	gmo_bone *root = model->current_bone;

	if (n == 0) {
		if (model->active_bone != (int)root) {
			model->active_bone = (int)root;
			root->activate();
		}
	} else {
		const mat4 *mats[8];
		int i;

		model->active_bone = 0;
		for (i = 0; i < n; i++) {
			gmo_bone *b = gmo_lookup_bone(model, root->blend_bones[bone_slots[i]]);
			const mat4 **slot = &mats[i];
			const mat4 *p;

			if (!b) {
				p = &gmo_null_matrix;
			} else {
				if (b->flags & 0x4000) {
					b->flags &= ~0x4000;
					(mat4 &)b->world_offset =
					    gmo_multiply(b->get_world_matrix(), (const mat4 &)b->local_offset);
				}
				p = &b->world_offset;
			}
			*slot = p;
		}
		g->SetBlendMatrices(mats, n);
	}

	if (shader_flags & 3)
		g->SetVertexShader((shader_flags & 1) ? -2 : -3);
	g->CallList(vb_handle);
}

gmo_material::gmo_material(gmo_model *m, bool global)
{
	model = m;
	src_material = 0;
	flags = 0;
	layer_flags = 0;
	transition_flags = 0;
	enable_mask = 0;
	enable_value = 0;
	transition_time = 0;
	for (int i = 3; i >= 0; i--)
		layers[i] = 0;
	if (!global)
		return;
	flags = 0x8000;
	set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	set_specular(vec4(0.0f, 0.0f, 0.0f, 1.0f), 10.0f);
	set_emission(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	set_tex_factor(vec4(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 0.0f);
	for (int i = 0; i < 4; i++) {
		set_layer_texture_handle(i, 0);
		set_layer_blend_func(i, gmo_material_default_blend_func[i]);
		set_layer_color_gen(i, gmo_material_default_color_gen[i], 2);
		set_layer_tex_gen(i, 0, GM_TEXGEN_TEXCOORD);
		set_layer_tex_func(i, GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGBA);
		set_layer_tex_wrap(i, GM_WRAP_REPEAT, GM_WRAP_REPEAT);
		set_layer_tex_filter(i, GM_FILTER_LINEAR, GM_FILTER_LINEAR_MIPMAP_LINEAR);
		set_layer_tex_crop(i, vec4(0.0f, 0.0f, 1.0f, 1.0f));
		set_layer_map_coord(i, 2, 0);
	}
}

void gmo_material::copy(const gmo_material &src)
{
	src_material = &src;
	flags = src.flags;
	layer_flags = src.layer_flags;
	transition_flags = src.transition_flags;
	enable_mask = src.enable_mask;
	enable_value = src.enable_value;
	transition_time = src.transition_time;
	for (int i = 0; i < 4; i++) {
		if (src.layers[i]) {
			gmo_layer *l = alloc_layer(i);
			l->copy(*src.layers[i]);
		}
	}
	diffuse = src.diffuse;
	specular = src.specular;
	emission = src.emission;
	tex_factor = src.tex_factor;
	specular_power = src.specular_power;
	tex_factor_a = src.tex_factor_a;
	tex_factor_b = src.tex_factor_b;
}

/* Sub-chunk dispatcher; the layer counter advances for every layer chunk. */
int gmo_material::load(GmoChunk *chunk)
{
	char *cursor = (char *)chunk;
	int n = 0;
	char *end = cursor + chunk->size;
	char *first = (chunk->tag & GMO_CHUNK_CONTAINER) ? cursor + chunk->first_child : end;

	cursor = first;
	while (cursor < end) {
		int raw = ((GmoChunk *)cursor)->tag;
		char *data = (raw & GMO_CHUNK_CONTAINER) ? cursor + 16 : cursor + 8;
		switch (raw & ~GMO_CHUNK_CONTAINER) {
		case GMO_BLOCK_LAYER: {
			gmo_layer *l = alloc_layer(n++);
			if (l)
				l->load((GmoChunk *)cursor);
			break;
		}
		case GMO_CMD_DIFFUSE:
			set_diffuse(*(const vec4 *)data);
			break;
		case GMO_CMD_SPECULAR:
			set_specular(*(const vec4 *)data, ((const float *)data)[4]);
			break;
		case GMO_CMD_EMISSION:
			set_emission(*(const vec4 *)data);
			break;
		case GMO_CMD_TEX_FACTOR:
			set_tex_factor(*(const vec4 *)data, ((const float *)data)[4], ((const float *)data)[5]);
			break;
		case GMO_CMD_ENABLE:
			enable(*(int *)data, true);
			break;
		case GMO_CMD_DISABLE:
			disable(*(int *)data, true);
			break;
		}
		cursor += ((GmoChunk *)cursor)->size;
	}
	return 1;
}

/* Sends the marked fields to gm_graph. pending_flags/pending_mask of the model cache the
   pending masks: the global material (flag 0x8000) clears and uses them, the
   others replace them after letting the global material set what is missing. */
void gmo_material::activate()
{
	if (model->option_flags & GMO_OPT_NO_MATERIALS)
		return;

	int a = model->pending_flags;
	int b = model->pending_mask;
	gm_graph *g = model->graph;

	if (flags & 0x8000) {
		model->pending_flags = 0;
		model->pending_mask = 0;
	} else {
		a &= ~flags;
		b &= ~layer_flags;
		if (a | b) {
			model->pending_flags = a;
			model->pending_mask = b;
			model->global_material->activate();
		}
		a = flags;
		model->pending_flags = a;
		b = layer_flags;
		model->pending_mask = b;
		g->SetEnableBits((model->enable_bits & ~enable_mask) | enable_value);
	}

	if (a) {
		if (a & 4)
			g->Diffuse(diffuse);
		if (a & 8)
			g->Specular(specular, specular_power);
		if (a & 0x10)
			g->Emission(emission);
		if (a & 0x20)
			g->TexFactor(tex_factor, tex_factor_a, tex_factor_b);
		a = (a >> 16) & 0x3ff;
		if (a) {
			gmo_layer *l = layers[0];
			if (l)
				l->activate(a);
		}
	}
	if (b) {
		/* three layers of 10 bits each */
		gmo_layer **p = &layers[1];
		for (int i = 2; i >= 0; i--) {
			int k = b & 0x3ff;
			if (k) {
				gmo_layer *l = *p;
				if (l)
					l->activate(k);
			}
			b >>= 10;
			p++;
		}
		g->SetLayer(0);
	}
}

/* Advances the interpolation towards the source material. transition_time is the
   remaining time; transition_flags holds the fields in transition in its low 16 bits and
   the finished ones in its high 16 bits. */
void gmo_material::update(float dt)
{
	vec4 tmp;

	if (transition_time == 0.0f)
		return;

	int done = ~(transition_flags >> 16) & 0xff;
	int mask = transition_flags & done;

	if (!mask) {
		transition_time = 0.0f;
		return;
	}

	float t = dt / transition_time;
	transition_time -= dt;
	if (transition_time < 0.0f) {
		transition_time = 0.0f;
		transition_flags &= (transition_flags >> 16) | 0xffffff00;
		t = 1.0f;
	}

	if (mask & 1) {
		src_material->get_diffuse(tmp);
		gmo_lerp(&diffuse.x, &tmp.x, 4, t);
	}
	if (mask & 2) {
		src_material->get_specular(tmp, specular_power);
		gmo_lerp(&specular.x, &tmp.x, 4, t);
	}
	if (mask & 4) {
		src_material->get_emission(tmp);
		gmo_lerp(&emission.x, &tmp.x, 4, t);
	}
	if (mask & 8) {
		src_material->get_tex_factor(tmp, tex_factor_a, tex_factor_b);
		gmo_lerp(&tex_factor.x, &tmp.x, 4, t);
	}
	for (int i = 0; i < 4; i++) {
		if ((16 << i) & mask) {
			gmo_layer *l = layers[i];
			if (l && (l->flags & 0x80)) {
				src_material->get_layer_tex_crop(i, tmp);
				gmo_lerp(&l->tex_crop.x, &tmp.x, 4, t);
			}
		}
	}
}

/* With t > 0 everything goes to animate_interp; otherwise the sample is
   assigned and the bits are set. */
void gmo_material::animate(int tag, int index, float *data, int count, float t)
{
	if (t > 0.0f) {
		animate_interp(tag, index, data, count, t);
		return;
	}
	switch (tag) {
	case GMO_CMD_DIFFUSE:
		diffuse.Set(data[0], data[1], data[2], data[3]);
		flags |= 4;
		transition_flags |= 0x10001;
		break;
	case GMO_CMD_SPECULAR:
		specular.Set(data[0], data[1], data[2], data[3]);
		specular_power = data[4];
		flags |= 8;
		transition_flags |= 0x20002;
		break;
	case GMO_CMD_EMISSION:
		emission.Set(data[0], data[1], data[2], data[3]);
		flags |= 0x10;
		transition_flags |= 0x40004;
		break;
	case GMO_CMD_TEX_FACTOR:
		tex_factor.Set(data[0], data[1], data[2], data[3]);
		tex_factor_a = data[4];
		tex_factor_b = data[5];
		flags |= 0x20;
		transition_flags |= 0x80008;
		break;
	case GMO_CMD_TEX_CROP: {
		gmo_layer *l = alloc_layer(index);
		if (l)
			l->set_tex_crop(*(const vec4 *)data);
		transition_flags |= 0x100010 << index;
		break;
	}
	}
}

void gmo_part::reconfigure()
{
	for (int i = 0; i < (int)meshes.size(); i++)
		meshes[i]->reconfigure();
}

void gmo_part::draw()
{
	for (int i = 0; i < (int)meshes.size(); i++)
		meshes[i]->draw();
}

gmo_mesh::gmo_mesh(gmo_part *p)
{
	gmo_model *m = p->model;
	part = p;
	shader_flags = 0;
	vb_handle = 0;
	material_index = -1;
	model = m;
}

gmo_mesh::~gmo_mesh()
{
	model->graph->DeleteList(vb_handle);
}

void gmo_mesh::copy(const gmo_mesh &src)
{
	shader_flags = src.shader_flags;
	vb_handle = model->graph->DuplicateList(src.vb_handle);
	material_index = src.material_index;
	bone_slots = src.bone_slots;
}

void gmo_mesh::reconfigure()
{}

gmo_material::~gmo_material()
{
	for (int i = 0; i < 4; i++) {
		if (layers[i])
			delete layers[i];
	}
}

void gmo_material::reconfigure()
{
	for (int i = 0; i < 4; i++) {
		if (layers[i])
			layers[i]->reconfigure();
	}
}

void gmo_material::restore(float t)
{
	gmo_restore_material(this, t);
}

void gmo_material::enable(int mask, bool on)
{
	if (!on) {
		disable(mask, true);
		return;
	}
	if (!(flags & 0x8000)) {
		enable_mask |= mask;
		enable_value |= mask;
	} else {
		for (int i = 0; i < (int)model->materials.size(); i++) {
			gmo_material *m = model->get_material(i);
			m->enable_mask |= mask;
			m->enable_value |= mask;
		}
	}
}

/* Flag 0x8000 (global material) decides whether to act on this material or
   on every material of the model. enable(mask, false) calls disable(mask, true)
   and the other way round. */
void gmo_material::disable(int mask, bool on)
{
	if (!on) {
		enable(mask, true);
	} else {
		if (!(flags & 0x8000)) {
			enable_mask |= mask;
			enable_value &= ~mask;
		} else {
			for (int i = 0; i < (int)model->materials.size(); i++) {
				gmo_material *m = gmo_lookup_material(model, i);
				m->enable_mask |= mask;
				m->enable_value &= ~mask;
			}
		}
	}
}

void gmo_material::force_global_layer(int index, int mask)
{
	if ((unsigned)index < 4) {
		if (index == 0) {
			mask = ~(mask << 16);
			for (int i = 0; i < (int)model->materials.size(); i++)
				gmo_lookup_material(model, i)->flags &= mask;
		} else {
			mask = ~(mask << ((index - 1) * 10));
			for (int i = 0; i < (int)model->materials.size(); i++)
				gmo_lookup_material(model, i)->layer_flags &= mask;
		}
	}
}

/* Sub-chunks: the layer commands (0x909 has no body). The texture index is a short. */
int gmo_layer::load(GmoChunk *chunk)
{
	char *cursor = (char *)chunk;
	char *end = cursor + chunk->size;
	char *first = (chunk->tag & GMO_CHUNK_CONTAINER) ? cursor + chunk->first_child : end;

	cursor = first;
	while (cursor < end) {
		int raw = ((GmoChunk *)cursor)->tag;
		char *data = (raw & GMO_CHUNK_CONTAINER) ? cursor + 16 : cursor + 8;
		switch (raw & ~GMO_CHUNK_CONTAINER) {
		case GMO_CMD_SET_TEXTURE:
			set_texture(*(const short *)data);
			break;
		case GMO_CMD_BLEND_FUNC:
			set_blend_func(((const int *)data)[0]);
			break;
		case GMO_CMD_COLOR_GEN:
			set_color_gen(((const int *)data)[0], ((const int *)data)[1]);
			break;
		case GMO_CMD_TEX_GEN:
			set_tex_gen(((const int *)data)[0], ((const int *)data)[1]);
			break;
		case GMO_CMD_TEX_FUNC:
			set_tex_func(((const int *)data)[0], ((const int *)data)[1]);
			break;
		case GMO_CMD_TEX_WRAP:
			set_tex_wrap(((const int *)data)[0], ((const int *)data)[1]);
			break;
		case GMO_CMD_TEX_FILTER:
			set_tex_filter(((const int *)data)[0], ((const int *)data)[1]);
			break;
		case GMO_CMD_TEX_CROP:
			set_tex_crop(*(const vec4 *)data);
			break;
		case GMO_CMD_MAP_COORD:
			set_map_coord(((const int *)data)[0], ((const int *)data)[1]);
			break;
		}
		cursor += ((GmoChunk *)cursor)->size;
	}
	return 1;
}

/* Sends to gm_graph the layer state selected by `mask` and, with bit 0x100
   and the layer's mapper enabled in the model, the texture matrix (slot index+4). */
void gmo_layer::activate(int mask)
{
	gm_graph *g = model->graph;

	g->SetLayer(index);
	if (mask & 0x01)
		g->SetTexture(texture_index);
	if (mask & 0x02)
		g->BlendFunc(blend_func);
	if (mask & 0x04)
		g->ColorGen(color_gen_mode, color_gen_arg);
	if (mask & 0x08)
		g->TexGen(tex_gen_mode, tex_gen_arg);
	if (mask & 0x10)
		g->TexFunc(tex_func_mode, tex_func_arg);
	if (mask & 0x20)
		g->TexWrap(tex_wrap_mode, tex_wrap_arg);
	if (mask & 0x40)
		g->TexFilter(tex_filter_mode, tex_filter_arg);
	if (mask & 0x80)
		g->TexCrop(tex_crop);
	if (!(mask & 0x100) || !(model->state & (16 << index)))
		return;

	int mode = map_coord_mode;
	if (mode == 2) {
		g->LoadIdentity(index + 4);
		g->MatrixMode(GM_MATRIX_MODEL);
		return;
	}

	mat4 m;
	gmo_bone *b = model->current_bone;
	if (b == 0)
		m = model->world_matrix;
	else if (b->blend_bones.size() == 0)
		m = b->get_world_matrix();
	else
		m = gmo_null_matrix;

	const mat4 &mapper = model->mapper[index];
	mat4 r = gmo_multiply(mapper, m);

	if (mode == 1) {
		static vec4 zaxis(0.0f, 0.0f, 1.0f, 1.0f);
		vec4 dir = normalize(mapper * m.m3);
		vec4 axis = cross(dir, zaxis);
		r = gmo_multiply(gmo_rotate_matrix(acosf(dot(dir, zaxis)), axis), r);
	}
	if (map_coord_arg == 16) {
		r = gmo_normalize(r);
		r.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	}
	if (flags & 8) {
		switch (tex_gen_mode) {
		case 2: {
			static mat4 sphere(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 0.0f,
			    0.0f, 0.0f, 0.0f);
			r = gmo_multiply(sphere, r);
			break;
		}
		case 3: {
			static mat4 reflect(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
			    0.0f, 0.0f, -1.0f, 1.0f);
			r = gmo_multiply(reflect, r);
			break;
		}
		}
	}
	g->LoadMatrix(index + 4, r);
	g->MatrixMode(GM_MATRIX_MODEL);
}

/* Walks the sub-chunks: 0x104 file name, 0x105 image (int length + data).
   Then, if GMO_OPT_LOAD_TEXTURES is set and there is no texture yet, creates it
   from image_data (then freed) or from file_name. */
int gmo_texture::load(GmoChunk *chunk)
{
	char *cursor = (char *)chunk;
	char *end = cursor + chunk->size;
	gm_graph *g = model->graph;
	char *first = (chunk->tag & GMO_CHUNK_CONTAINER) ? cursor + chunk->first_child : end;

	cursor = first;
	while (cursor < end) {
		GmoChunk *c = (GmoChunk *)cursor;
		int tag = c->tag & ~GMO_CHUNK_CONTAINER;
		char *data = (c->tag & GMO_CHUNK_CONTAINER) ? cursor + 0x10 : cursor + 8;
		switch (tag) {
		case GMO_CMD_FILE_NAME:
			set_file_name((const char *)data);
			break;
		case GMO_CMD_FILE_IMAGE:
			set_file_image((int *)(data + 4), *(int *)data);
			break;
		}
		cursor += c->size;
	}

	if (model->option_flags & GMO_OPT_LOAD_TEXTURES) {
		if (texture_handle == 0) {
			if (image_data) {
				texture_handle = g->CreateTexture(image_data, image_len);
				if (texture_handle)
					set_file_image(0, 0);
			} else if (file_name) {
				texture_handle = g->CreateTexture(file_name);
				if (!texture_handle)
					printf("cannot load \"%s\"\n", file_name);
			}
		}
	}
	return 1;
}

/* Sub-chunks: frame rate, frame loop, animation keys and GMO_BLOCK_FCURVE
   (gmo_load_fcurve). anim_data is created on first use. If
   loop_end < loop_start the two are swapped. */
int gmo_motion::load(GmoChunk *chunk)
{
	char *cursor = (char *)chunk;
	char *end = cursor + chunk->size;
	char *first = (chunk->tag & GMO_CHUNK_CONTAINER) ? cursor + chunk->first_child : end;

	cursor = first;
	while (cursor < end) {
		int raw = ((GmoChunk *)cursor)->tag;
		char *data = (raw & GMO_CHUNK_CONTAINER) ? cursor + 16 : cursor + 8;
		switch (raw & ~GMO_CHUNK_CONTAINER) {
		case GMO_CMD_FRAME_RATE:
			frame_rate = *(const float *)data;
			break;
		case GMO_CMD_FRAME_LOOP:
			loop_start = ((const float *)data)[0];
			loop_end = ((const float *)data)[1];
			break;
		case GMO_CMD_ANIMATE: {
			if (anim_data == 0)
				anim_data = new gmo_anim_data;
			GmoAnimate k;
			k.target = ((const int *)data)[0];
			k.property = ((const int *)data)[1];
			k.property_index = ((const int *)data)[2];
			k.fcurve = ((const int *)data)[3];
			k.fcurve = (short)k.fcurve;
			anim_data->keys.push_back(k);
			break;
		}
		case GMO_BLOCK_FCURVE: {
			if (anim_data == 0)
				anim_data = new gmo_anim_data;
			GmoFCurve *b = gmo_load_fcurve((GmoChunk *)cursor);
			anim_data->fcurves.push_back(b);
			break;
		}
		}
		cursor += ((GmoChunk *)cursor)->size;
	}

	if (loop_start > loop_end) {
		float t = loop_start;
		loop_start = loop_end;
		loop_end = t;
	}
	return 1;
}

/* Advances `interp` (remaining blend time) and `current_frame` (wrapping on
   [loop_start, loop_end) in both directions), then evaluates every key's curve
   and applies the sample to its target (4 bone, 8 material) with animate().
   ratio is dt/interp while interp > 0, otherwise interp itself. */
void gmo_motion::update(float dt)
{
	/* output of gmo_apply_curve, shared by every key (16 floats: a matrix) */
	static float gmo_anim_scratch[16];
	float ratio = interp;

	if (interp > 0.0f) {
		interp -= dt;
		ratio = dt / ratio;
		if (interp < 0.0f) {
			interp = 0.0f;
			ratio = 0.0f;
		}
	}

	float newFrame = current_frame + frame_rate * dt;
	time += dt;

	if (newFrame >= loop_end && current_frame < loop_end) {
		float loopLen = loop_end - loop_start;
		if (loopLen > 0.0f)
			newFrame = loop_start + fmodf(newFrame - loop_end, loopLen);
		else
			newFrame = loop_end;
	} else if (newFrame < loop_start && current_frame >= loop_start) {
		float loopLen = loop_end - loop_start;
		if (loopLen > 0.0f)
			newFrame = loop_end - fmodf(loop_start - newFrame, loopLen);
		else
			newFrame = loop_start;
	}
	current_frame = newFrame;

	if (anim_data) {
		for (int i = 0; i < (int)anim_data->keys.size(); i++) {
			GmoAnimate *k = &anim_data->keys[i];
			if (k->fcurve >= 0) {
				GmoFCurve *blob = anim_data->fcurves[k->fcurve];
				gmo_apply_curve(blob, newFrame, gmo_anim_scratch);
				int target = k->target;
				switch ((target >> 16) & 0xff) {
				case GMO_BLOCK_BONE: {
					gmo_bone *b = gmo_lookup_bone(model, (short)target);
					if (b)
						b->animate(k->property, k->property_index, gmo_anim_scratch,
						    blob->nvalues, ratio);
					break;
				}
				case GMO_BLOCK_MATERIAL: {
					gmo_material *m = gmo_lookup_material(model, (short)target);
					if (m)
						m->animate(k->property, k->property_index, gmo_anim_scratch,
						    blob->nvalues, ratio);
					break;
				}
				}
			}
		}
	}
}

/* Releases the target of every key: a bone or a material. */
void gmo_motion::stop(float t)
{
	for (int i = 0; i < (int)anim_data->keys.size(); i++) {
		GmoAnimate *k = &anim_data->keys[i];
		int target = k->target;
		switch ((target >> 16) & 0xff) {
		case GMO_BLOCK_BONE: {
			int idx = (short)target;
			gmo_bone *b = gmo_lookup_bone(model, idx);
			if (b)
				gmo_restore_bone(b, t);
			break;
		}
		case GMO_BLOCK_MATERIAL: {
			int idx = (short)target;
			gmo_material *m = gmo_lookup_material(model, idx);
			if (m)
				gmo_restore_material(m, t);
			break;
		}
		}
	}
}

/* Evaluates the fcurve at `frame` into out: finds the keys around the frame
   (before the first one, or with a single key, that key) and interpolates by
   mode: 0 step, 1 linear, 4 quaternion slerp. */
struct GmoAnimFrame {
	float time;
	float value[1];
};

gmo_motion::gmo_anim_data::~gmo_anim_data()
{
	for (int i = 0; i < (int)fcurves.size(); i++)
		delete fcurves[i];
}

static void gmo_apply_curve(GmoFCurve *blob, float frame, float *out)
{
	float *k0, *k1;
	int i, nkeys;

	k0 = blob->data;
	nkeys = blob->nkeys;
	k1 = k0;
	i = 0;

	/* Key search: there is at least one key and frame is not before the first.
	   k1 advances only while a next key exists, so past the last one k0 == k1
	   (step on the last key). */
	if (nkeys > 0) {
		float *v = k1 + 1;
		if (!(frame < *k1)) {
			do {
				k0 = k1;
				v += blob->nvalues;
				if (++i >= blob->nkeys)
					break;
				k1 = v;
				v = k1 + 1;
			} while (!(frame < *k1));
		}
	}

	int type = blob->type;
	if (k0 == k1)
		type = GMO_FCURVE_CONSTANT;
	switch (type) {
	case GMO_FCURVE_CONSTANT:
		for (int j = 0; j < blob->nvalues; j++)
			*out++ = *++k0;
		break;
	case GMO_FCURVE_LINEAR: {
		float t = (frame - *k0) / (*k1 - *k0);
		for (int j = 0; j < blob->nvalues; j++) {
			float a = *++k0;
			float b = *++k1;
			*out++ = (b - a) * t + a;
		}
		break;
	}
	case GMO_FCURVE_SPHERICAL: {
		const vec4 *r = (const vec4 *)(k0 + 1);
		const vec4 *sr = (const vec4 *)(k1 + 1);
		vec4 q;
		float dot = r->x * sr->x + r->y * sr->y + r->z * sr->z + r->w * sr->w;
		float t = (frame - k0[0]) / (k1[0] - k0[0]);
		int neg = 0;

		if (dot < 0.0f) {
			dot = -dot;
			neg = 1;
		}
		float ang = acosf(dot);
		float sn = sinf(ang);
		if (sn == 0.0f) {
			q = *sr;
		} else {
			float a = sinf((1.0f - t) * ang) / sn;
			float b = sinf(t * ang) / sn;
			if (neg)
				b = -b;
			float qx = r->x * a + sr->x * b;
			float qy = r->y * a + sr->y * b;
			float qz = r->z * a + sr->z * b;
			float qw = r->w * a + sr->w * b;

			q.x = qx;
			q.y = qy;
			q.z = qz;
			q.w = qw;
		}
		*(vec4 *)out = q;
		break;
	}
	}
}

/* Builds the vertex block of a GMO_BLOCK_ARRAYS chunk. Four header words: format of the
   first vertex set (0x8000 = compressed), vertex count, set count (morph),
   format of the following sets (0 = same). The data, in the GMO_CMD_DATA sub-chunk, is raw
   or quantized to shorts (position with scale and offset, normals, weights and
   texcoords in 1/4096, color 4 bits per channel). */
static GmoArrays *gmo_load_arrays(GmoChunk *chunk)
{
	const GmoArrays *hd =
	    (const GmoArrays *)((chunk->tag & GMO_CHUNK_CONTAINER) ? (char *)chunk + 16 : (char *)chunk + 8);
	int format = hd->format & ~0x8000;
	int morphFormat = hd->morph_format & ~0x8000;
	if (!morphFormat)
		morphFormat = format;
	int stride = gm_graph::GetStride(format);
	int morphStride = gm_graph::GetStride(morphFormat);
	int size = (morphStride * (hd->morph_count - 1) + stride) * hd->vertex_count;
	GmoArrays *blob = (GmoArrays *)new char[(unsigned int)(size + 16 + 3) / 4 * 4];

	blob->format = format;
	blob->vertex_count = hd->vertex_count;
	blob->morph_count = hd->morph_count;
	blob->morph_format = morphFormat;

	GmoChunk *end = gmo_next_chunk(chunk);
	chunk = (chunk->tag & GMO_CHUNK_CONTAINER) ? gmo_first_child(chunk) : end;
	while ((chunk->tag & ~GMO_CHUNK_CONTAINER) != GMO_CMD_DATA) {
		if (chunk == end) {
			printf("vertex data missing\n");
			delete blob;
			return 0;
		}
		chunk = gmo_next_chunk(chunk);
	}

	int len = (chunk->tag & GMO_CHUNK_CONTAINER) ? chunk->first_child - 16 : chunk->size - 8;
	if (!(hd->format & 0x8000)) {
		if (len < size) {
			printf("vertex data lacking %d / %d\n", len, size);
			delete blob;
			return 0;
		}
		memcpy(blob + 1, (chunk->tag & GMO_CHUNK_CONTAINER) ? (char *)chunk + 16 : (char *)chunk + 8, size);
		return blob;
	}

	int need = size / 2 + 24;
	if (len < need) {
		printf("vertex data lacking %d / %d\n", len, need);
		delete blob;
		return 0;
	}
	const float *scale =
	    (const float *)((chunk->tag & GMO_CHUNK_CONTAINER) ? (char *)chunk + 16 : (char *)chunk + 8);
	const float *bias = scale + 3;
	const short *q = (const short *)(scale + 6);
	float *dst = (float *)(blob + 1);
	for (int i = 0; i < hd->morph_count; i++) {
		for (int j = 0; j < hd->vertex_count; j++) {
			if (format & 1) {
				dst[0] = q[0] * scale[0] + bias[0];
				dst[1] = q[1] * scale[1] + bias[1];
				dst[2] = q[2] * scale[2] + bias[2];
				q += 3;
				dst += 3;
			}
			int nw = (format >> 8) & 0x3f;
			for (int k = 0; k < nw; k++)
				*dst++ = *q++ * (1.0f / 4096.0f);
			if (format & 2) {
				dst[0] = q[0] * (1.0f / 4096.0f);
				dst[1] = q[1] * (1.0f / 4096.0f);
				dst[2] = q[2] * (1.0f / 4096.0f);
				q += 3;
				dst += 3;
			}
			if (format & 8)
				*dst++ = *q++ * (1.0f / 4096.0f);
			if (format & 4) {
				/* 4:4:4:4 -> 8:8:8:8, each nibble replicated in the low 4 bits */
				unsigned short c = *(const unsigned short *)q++;
				int r = (c & 0xf) << 4;
				int g = ((c >> 4) & 0xf) << 4;
				int b = ((c >> 8) & 0xf) << 4;
				int a = ((c >> 12) & 0xf) << 4;
				*(unsigned int *)dst++ = (r | (c & 0xf)) | ((g | ((c >> 4) & 0xf)) << 8)
				    | ((b | ((c >> 8) & 0xf)) << 16) | ((a | ((c >> 12) & 0xf)) << 24);
			}
			if (format & 0x10) {
				dst[0] = q[0] * (1.0f / 4096.0f);
				dst[1] = q[1] * (1.0f / 4096.0f);
				q += 2;
				dst += 2;
			}
		}
		format = morphFormat;
	}
	return blob;
}

/* Builds the fcurve of a GMO_BLOCK_FCURVE chunk. The four header words of the chunk are
   the curve header (GMO_FCURVE_COMPRESSED: compressed); the data, in the GMO_CMD_DATA sub-chunk,
   is raw floats or shorts with a scale and an offset per component. */
static GmoFCurve *gmo_load_fcurve(GmoChunk *chunk)
{
	const GmoFCurve *hd =
	    (const GmoFCurve *)((chunk->tag & GMO_CHUNK_CONTAINER) ? (char *)chunk + 16 : (char *)chunk + 8);
	int size = (hd->nvalues * 4 + 4) * hd->nkeys;
	GmoFCurve *blob = (GmoFCurve *)new char[size + 16];

	blob->type = hd->type & ~GMO_FCURVE_COMPRESSED;
	blob->nvalues = hd->nvalues;
	blob->nkeys = hd->nkeys;
	blob->unk_0C = hd->unk_0C;

	GmoChunk *end = gmo_next_chunk(chunk);
	chunk = (chunk->tag & GMO_CHUNK_CONTAINER) ? gmo_first_child(chunk) : end;
	while ((chunk->tag & ~GMO_CHUNK_CONTAINER) != GMO_CMD_DATA) {
		if (chunk == end) {
			printf("fcurve data missing\n");
			delete blob;
			return 0;
		}
		chunk = gmo_next_chunk(chunk);
	}

	int len = (chunk->tag & GMO_CHUNK_CONTAINER) ? chunk->first_child - 16 : chunk->size - 8;
	if (!(hd->type & GMO_FCURVE_COMPRESSED)) {
		if (len < size) {
			printf("fcurve data lacking %d / %d\n", len, size);
			delete blob;
			return 0;
		}
		memcpy(blob->data, (chunk->tag & GMO_CHUNK_CONTAINER) ? (char *)chunk + 16 : (char *)chunk + 8, size);
		return blob;
	}

	int need = size / 2 + 24;
	if (len < need) {
		printf("fcurve data lacking %d / %d\n", len, need);
		delete blob;
		return 0;
	}
	const float *data =
	    (const float *)((chunk->tag & GMO_CHUNK_CONTAINER) ? (char *)chunk + 16 : (char *)chunk + 8);
	const float *scale = data + 1;
	const float *bias = scale + hd->nvalues + 1;
	const short *q = (const short *)(bias + hd->nvalues);
	float *dst = blob->data;
	for (int i = 0; i < hd->nkeys; i++)
		for (int j = -1; j < hd->nvalues; j++)
			*dst++ = *q++ * scale[j] + bias[j];
	return blob;
}

void gmo_material::set_diffuse(const vec4 &c)
{
	diffuse = c;
	flags |= 4;
	if (flags & 0x8000)
		force_global_material(4);
}

void gmo_material::set_specular(const vec4 &c, float power)
{
	specular = c;
	specular_power = power;
	flags |= 8;
	if (flags & 0x8000)
		force_global_material(8);
}

void gmo_material::set_emission(const vec4 &c)
{
	emission = c;
	flags |= 0x10;
	if (flags & 0x8000)
		force_global_material(0x10);
}

void gmo_material::set_tex_factor(const vec4 &c, float a, float b)
{
	tex_factor = c;
	tex_factor_a = a;
	tex_factor_b = b;
	flags |= 0x20;
	if (flags & 0x8000)
		force_global_material(0x20);
}

void gmo_material::set_layer_texture_handle(int index, int handle)
{
	gmo_layer *l = alloc_layer(index);
	if (l) {
		l->set_texture_handle(handle);
		l->texture = -1;
	}
	if (flags & 0x8000)
		force_global_layer(index, 1);
}

void gmo_material::set_layer_blend_func(int index, int func)
{
	gmo_layer *l = alloc_layer(index);
	if (l)
		l->set_blend_func(func);
	if (flags & 0x8000)
		force_global_layer(index, 2);
}

void gmo_material::set_layer_color_gen(int index, int mode, int arg)
{
	gmo_layer *l = alloc_layer(index);
	if (l)
		l->set_color_gen(mode, arg);
	if (flags & 0x8000)
		force_global_layer(index, 4);
}

void gmo_material::set_layer_tex_gen(int index, int mode, int arg)
{
	gmo_layer *l = alloc_layer(index);
	if (l)
		l->set_tex_gen(mode, arg);
	if (flags & 0x8000)
		force_global_layer(index, 8);
}

void gmo_material::set_layer_tex_func(int index, int mode, int arg)
{
	gmo_layer *l = alloc_layer(index);
	if (l)
		l->set_tex_func(mode, arg);
	if (flags & 0x8000)
		force_global_layer(index, 0x10);
}

void gmo_material::set_layer_tex_wrap(int index, int mode, int arg)
{
	gmo_layer *l = alloc_layer(index);
	if (l)
		l->set_tex_wrap(mode, arg);
	if (flags & 0x8000)
		force_global_layer(index, 0x20);
}

void gmo_material::set_layer_tex_filter(int index, int mode, int arg)
{
	gmo_layer *l = alloc_layer(index);
	if (l)
		l->set_tex_filter(mode, arg);
	if (flags & 0x8000)
		force_global_layer(index, 0x40);
}

void gmo_material::set_layer_tex_crop(int index, const vec4 &crop)
{
	gmo_layer *l = alloc_layer(index);
	if (l)
		l->set_tex_crop(crop);
	if (flags & 0x8000)
		force_global_layer(index, 0x80);
}

void gmo_material::set_layer_map_coord(int index, int mode, int arg)
{
	gmo_layer *l = alloc_layer(index);
	if (l)
		l->set_map_coord(mode, arg);
	if (flags & 0x8000)
		force_global_layer(index, 0x100);
}

void gmo_material::get_diffuse(vec4 &out) const
{
	if (flags & 4) {
		out = diffuse;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_diffuse(out);
}

void gmo_material::get_specular(vec4 &out, float &power) const
{
	if (flags & 8) {
		out = specular;
		power = specular_power;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_specular(out, power);
}

void gmo_material::get_emission(vec4 &out) const
{
	if (flags & 0x10) {
		out = emission;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_emission(out);
}

void gmo_material::get_tex_factor(vec4 &out, float &a, float &b) const
{
	if (flags & 0x20) {
		out = tex_factor;
		a = tex_factor_a;
		b = tex_factor_b;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_tex_factor(out, a, b);
}

void gmo_material::get_layer_texture_handle(int index, int &handle) const
{
	gmo_layer *l = layers[index];
	if (l && (l->flags & 1)) {
		handle = l->texture_index;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_layer_texture_handle(index, handle);
}

void gmo_material::get_layer_blend_func(int index, int &func) const
{
	gmo_layer *l = layers[index];
	if (l && (l->flags & 2)) {
		func = l->blend_func;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_layer_blend_func(index, func);
}

void gmo_material::get_layer_color_gen(int index, int &mode, int &arg) const
{
	gmo_layer *l = layers[index];
	if (l && (l->flags & 4)) {
		mode = l->color_gen_mode;
		arg = l->color_gen_arg;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_layer_color_gen(index, mode, arg);
}

void gmo_material::get_layer_tex_gen(int index, int &mode, int &arg) const
{
	gmo_layer *l = layers[index];
	if (l && (l->flags & 8)) {
		mode = l->tex_gen_mode;
		arg = l->tex_gen_arg;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_layer_tex_gen(index, mode, arg);
}

void gmo_material::get_layer_tex_func(int index, int &mode, int &arg) const
{
	gmo_layer *l = layers[index];
	if (l && (l->flags & 0x10)) {
		mode = l->tex_func_mode;
		arg = l->tex_func_arg;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_layer_tex_func(index, mode, arg);
}

void gmo_material::get_layer_tex_wrap(int index, int &mode, int &arg) const
{
	gmo_layer *l = layers[index];
	if (l && (l->flags & 0x20)) {
		mode = l->tex_wrap_mode;
		arg = l->tex_wrap_arg;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_layer_tex_wrap(index, mode, arg);
}

void gmo_material::get_layer_tex_filter(int index, int &mode, int &arg) const
{
	gmo_layer *l = layers[index];
	if (l && (l->flags & 0x40)) {
		mode = l->tex_filter_mode;
		arg = l->tex_filter_arg;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_layer_tex_filter(index, mode, arg);
}

void gmo_material::get_layer_tex_crop(int index, vec4 &crop) const
{
	gmo_layer *l = layers[index];
	if (l && (l->flags & 0x80)) {
		crop = l->tex_crop;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_layer_tex_crop(index, crop);
}

void gmo_material::get_layer_map_coord(int index, int &mode, int &arg) const
{
	gmo_layer *l = layers[index];
	if (l && (l->flags & 0x100)) {
		mode = l->map_coord_mode;
		arg = l->map_coord_arg;
		return;
	}
	if (flags & 0x8000)
		return;
	model->global_material->get_layer_map_coord(index, mode, arg);
}

/* Like animate(), interpolating with gmo_lerp. The 4th parameter is unused. */
void gmo_material::animate_interp(int tag, int index, float *data, int count, float t)
{
	switch (tag) {
	case GMO_CMD_DIFFUSE:
		gmo_lerp(&diffuse.x, data, 4, t);
		flags |= 4;
		break;
	case GMO_CMD_SPECULAR:
		gmo_lerp(&specular.x, data, 4, t);
		specular_power = data[4];
		flags |= 8;
		break;
	case GMO_CMD_EMISSION:
		gmo_lerp(&emission.x, data, 4, t);
		flags |= 0x10;
		break;
	case GMO_CMD_TEX_FACTOR:
		gmo_lerp(&tex_factor.x, data, 4, t);
		tex_factor_a = data[4];
		tex_factor_b = data[5];
		break;
	case GMO_CMD_TEX_CROP: {
		gmo_layer *l = alloc_layer(index);
		if (l) {
			gmo_lerp(&l->tex_crop.x, data, 4, t);
			l->set_flags(128);
		}
		break;
	}
	}
}

void gmo_material::force_global_material(int mask)
{
	int notmask = ~mask;
	for (int i = 0; i < (int)model->materials.size(); i++) {
		gmo_material *m = gmo_lookup_material(model, i);
		m->flags &= notmask;
	}
}

gmo_layer *gmo_material::alloc_layer(int index)
{
	if ((unsigned)index >= 4)
		return 0;
	gmo_layer *l = layers[index];
	if (!l) {
		l = new gmo_layer(this, index);
		layers[index] = l;
		flags |= (0x40 << index);
	}
	return l;
}

/* Sets the bits of layer `index` (in flags for layer 0, in layer_flags, 10 bits
   per layer, for the others) and calls gmo_layer::set_flags only if they changed. */
void gmo_material::set_layer_flags(int index, int mask)
{
	if ((unsigned)index < 4) {
		gmo_layer *l = layers[index];
		if (l) {
			if (index == 0) {
				int v = flags | (mask << 16);
				if (flags != v) {
					flags = v;
					l->set_flags(mask);
				}
			} else {
				index = (index - 1) * 10;
				int v = layer_flags | (mask << index);
				if (layer_flags != v) {
					layer_flags = v;
					l->set_flags(mask);
				}
			}
		}
	}
}

gmo_layer::gmo_layer(gmo_material *mat, int idx)
{
	gmo_model *m = mat->model;
	material = mat;
	index = idx;
	flags = 0;
	texture_index = 0;
	texture = -1;
	model = m;
}

gmo_layer::~gmo_layer()
{
	set_texture_handle(0);
}

void gmo_layer::copy(const gmo_layer &src)
{
	flags = src.flags;
	set_texture_handle(src.texture_index);
	texture = src.texture;
	blend_func = src.blend_func;
	color_gen_mode = src.color_gen_mode;
	color_gen_arg = src.color_gen_arg;
	tex_gen_mode = src.tex_gen_mode;
	tex_gen_arg = src.tex_gen_arg;
	tex_func_mode = src.tex_func_mode;
	tex_func_arg = src.tex_func_arg;
	tex_wrap_mode = src.tex_wrap_mode;
	tex_wrap_arg = src.tex_wrap_arg;
	tex_filter_mode = src.tex_filter_mode;
	tex_filter_arg = src.tex_filter_arg;
	tex_crop = src.tex_crop;
	map_coord_mode = src.map_coord_mode;
	map_coord_arg = src.map_coord_arg;
}

void gmo_layer::reconfigure()
{
	gmo_model *m = model;
	gmo_texture *t = (0 <= texture && texture < (int)m->textures.size()) ? m->textures[texture] : 0;
	if (t)
		set_texture_handle(t->texture_handle);
}

void gmo_layer::set_texture_handle(int idx)
{
	gm_graph *g = model->graph;
	if (idx == texture_index)
		return;
	if (texture_index != 0) {
		g->DeleteTexture(texture_index);
		texture_index = 0;
	}
	if (idx != 0) {
		g->DuplicateTexture(idx);
		texture_index = idx;
	}
	set_flags(1);
}

void gmo_layer::set_texture(int idx)
{
	texture = idx;
	set_flags(1);
}

void gmo_layer::set_blend_func(int func)
{
	blend_func = func;
	set_flags(2);
}

void gmo_layer::set_color_gen(int mode, int arg)
{
	color_gen_mode = mode;
	color_gen_arg = arg;
	set_flags(4);
}

void gmo_layer::set_tex_gen(int mode, int arg)
{
	tex_gen_mode = mode;
	tex_gen_arg = arg;
	set_flags(8);
}

void gmo_layer::set_tex_func(int mode, int arg)
{
	tex_func_mode = mode;
	tex_func_arg = arg;
	set_flags(0x10);
}

void gmo_layer::set_tex_wrap(int mode, int arg)
{
	tex_wrap_mode = mode;
	tex_wrap_arg = arg;
	set_flags(0x20);
}

void gmo_layer::set_tex_filter(int mode, int arg)
{
	tex_filter_mode = mode;
	tex_filter_arg = arg;
	set_flags(0x40);
}

void gmo_layer::set_tex_crop(const vec4 &crop)
{
	tex_crop = crop;
	set_flags(0x80);
}

void gmo_layer::set_map_coord(int mode, int arg)
{
	map_coord_mode = mode;
	map_coord_arg = arg;
	set_flags(0x100);
}

/* Propagates the dirty bits to the owning material: layer 0 in bits 16-31 of
   material->flags, layers 1-3 packed 10 bits each in material->layer_flags. */
void gmo_layer::set_flags(int bit)
{
	int newflags = flags | bit;

	if (flags == newflags)
		return;

	int i = index;

	flags = newflags;

	int j = i;
	gmo_material *m = material;

	if ((unsigned)i >= 4)
		return;

	gmo_layer *l = m->layers[j];
	if (!l)
		return;

	if (j == 0) {
		int v = m->flags | (bit << 16);
		if (m->flags != v) {
			m->flags = v;
			l->set_flags(bit);
		}
	} else {
		j = (j - 1) * 10;
		int v = m->layer_flags | (bit << j);
		if (m->layer_flags != v) {
			m->layer_flags = v;
			l->set_flags(bit);
		}
	}
}

gmo_texture::gmo_texture(gmo_model *m)
{
	model = m;
	unk_04 = 0;
	texture_handle = 0;
	file_name = 0;
	image_data = 0;
	image_len = 0;
}

gmo_texture::~gmo_texture()
{
	set_texture_handle(0);
	set_file_name(0);
	set_file_image(0, 0);
}

void gmo_texture::copy(const gmo_texture &src)
{
	unk_04 = src.unk_04;
	set_texture_handle(src.texture_handle);
	set_file_name(src.file_name);
	set_file_image(src.image_data, src.image_len);
}

void gmo_texture::reconfigure()
{}

void gmo_texture::set_texture_handle(int index)
{
	gm_graph *g = model->graph;
	if (index != texture_handle) {
		if (texture_handle) {
			g->DeleteTexture(texture_handle);
			texture_handle = 0;
		}
		if (index) {
			g->DuplicateTexture(index);
			texture_handle = index;
		}
		model->state |= 0x8000;
	}
}

void gmo_texture::set_file_name(const char *name)
{
	if (file_name)
		delete[] file_name;
	file_name = 0;
	if (name) {
		int len = strlen(name);
		char *buf = new char[len + 1];
		file_name = buf;
		memcpy(buf, name, len + 1);
	}
}

void gmo_texture::set_file_image(int *data, int len)
{
	free(image_data);
	image_data = 0;
	if (data) {
		int n = (len + 3) / 4 * 4;
		int *buf = (int *)memalign(16, n);
		image_data = buf;
		image_len = len;
		n += (int)buf;
		*(int *)(n - 4) = 0;
		memcpy(image_data, data, len);
	}
}

gmo_motion::gmo_motion(gmo_model *m)
{
	model = m;
	unk_04 = 0;
	anim_data = 0;
	current_frame = 0.0f;
	time = 0.0f;
	interp = 0.0f;
	frame_rate = 60.0f;
	loop_start = -1000000.0f;
	loop_end = 1000000.0f;
}

gmo_motion::~gmo_motion()
{
	anim_data->release();
}

void gmo_motion::copy(const gmo_motion &src)
{
	unk_04 = src.unk_04;
	anim_data = src.anim_data->duplicate();
	frame_rate = src.frame_rate;
	loop_start = src.loop_start;
	loop_end = src.loop_end;
	current_frame = src.current_frame;
	time = src.time;
	interp = src.interp;
}

void gmo_motion::reconfigure()
{}

void gmo_motion::start(float delay, float t)
{
	set_time(delay);
	set_interp(t);
}

void gmo_motion::set_frame_rate(float f)
{
	frame_rate = f;
}

void gmo_motion::set_loop_start(float t)
{
	loop_start = t;
	if (loop_end < t)
		loop_end = t;
}

void gmo_motion::set_loop_end(float t)
{
	loop_end = t;
	if (t < loop_start)
		loop_start = t;
}

void gmo_motion::set_time(float t)
{
	float frame = frame_rate * t;
	time = t;
	if (frame > 0.0f) {
		if (loop_end < frame) {
			float duration = loop_end - loop_start;
			if (duration > 0.0f)
				frame = loop_start + fmodf(frame - loop_end, duration);
			else
				frame = loop_end;
		}
	} else {
		if (frame < loop_start) {
			float duration = loop_end - loop_start;
			if (duration > 0.0f)
				frame = loop_end - fmodf(loop_start - frame, duration);
			else
				frame = loop_start;
		}
	}
	current_frame = frame;
}

static GmoFCurve *gmo_load_fcurve(GmoChunk *chunk);

void gmo_motion::set_frame(float f)
{
	current_frame = f;
}

void gmo_motion::set_interp(float f)
{
	interp = f;
}

gmo_motion::gmo_anim_data::gmo_anim_data()
{
	refcount = 1;
}

gmo_motion::gmo_anim_data *gmo_motion::gmo_anim_data::duplicate()
{
	if (this)
		refcount++;
	return this;
}

void gmo_motion::gmo_anim_data::release()
{
	if (this) {
		if (--refcount == 0)
			delete this;
	}
}

static void gmo_lerp(float *dst, const float *src, int n, float t)
{
	for (int i = 0; i < n; i++) {
		float a = *dst++;
		float b = *src++;
		dst[-1] = (b - a) * t + a;
	}
}

/* Reads a whole file into a new[] buffer rounded up to 4 bytes, last word zeroed. */
static int gmo_read_file(const char *name, void **buf, int *len)
{
	FILE *f = fopen(name, "rb");

	if (!f)
		return 0;
	fseek(f, 0, SEEK_END);
	*len = ftell(f);
	if (*len == 0) {
		fclose(f);
		return 0;
	}
	fseek(f, 0, SEEK_SET);
	int n = (*len + 3) / 4 * 4;
	char *p = new char[n];
	*buf = p;
	n += (int)p;
	*(int *)(n - 4) = 0;
	fread(p, *len, 1, f);
	fclose(f);
	return 1;
}
