/*
 * GMO model loader (gmo_*): model, bones, parts, meshes, materials, layers,
 * textures and motions. The classes hold data only except gmo_model, whose
 * vptr sits at 0x1BC, after its data.
 */

#pragma once

#include <vector>
#include "vec_types.h"

class gm_graph;
class gmo_bone;
class gmo_part;
class gmo_material;
class gmo_texture;
class gmo_motion;
class gmo_mesh;
class gmo_layer;
/* Generic GMO chunk header (tag/size tree), passed to the load() methods.
   Tag bit 0x8000 marks a container with sub-chunks; the rest of the payload
   depends on the tag. */
/* Chunk tags: a block type, or a command (block type << 8 | n) inside that block. */
#define GMO_CHUNK_CONTAINER 0x8000  /* tag bit: the chunk has sub-chunks */
#define GMO_BLOCK_MODEL 3
#define GMO_BLOCK_BONE 4
#define GMO_BLOCK_PART 5
#define GMO_BLOCK_MESH 6
#define GMO_BLOCK_ARRAYS 7
#define GMO_BLOCK_MATERIAL 8
#define GMO_BLOCK_LAYER 9
#define GMO_BLOCK_TEXTURE 10
#define GMO_BLOCK_MOTION 11
#define GMO_BLOCK_FCURVE 12

#define GMO_CMD_DATA 0x102  /* the raw data of an arrays or fcurve block */
#define GMO_CMD_FILE_NAME 0x104
#define GMO_CMD_FILE_IMAGE 0x105
#define GMO_CMD_BOUNDING_BOX 0x106

#define GMO_CMD_PARENT_BONE 0x401
#define GMO_CMD_VISIBILITY 0x402
#define GMO_CMD_MORPH_WEIGHTS 0x403
#define GMO_CMD_BLEND_BONES 0x404
#define GMO_CMD_MULT_MATRIX 0x405
#define GMO_CMD_TRANSLATE 0x406
#define GMO_CMD_ROTATE_ZYX 0x407
#define GMO_CMD_ROTATE_YXZ 0x408
#define GMO_CMD_ROTATE 0x409  /* quaternion */
#define GMO_CMD_SCALE 0x40a
#define GMO_CMD_SCALE_INHERIT 0x40b  /* the scale also goes to inherited_scale, for the children */
#define GMO_CMD_DRAW_PART 0x40c

#define GMO_CMD_SET_MATERIAL 0x601
#define GMO_CMD_BLEND_SUBSET 0x602
#define GMO_CMD_PARTICLE_SIZE 0x603
#define GMO_CMD_PARTICLE_CROP 0x604
#define GMO_CMD_PATCH_LEVEL 0x605
#define GMO_CMD_PATCH_SUBDIV 0x606
#define GMO_CMD_PATCH_CLIP 0x607
#define GMO_CMD_PATCH_CROP 0x608
#define GMO_CMD_DRAW_ARRAYS 0x609
#define GMO_CMD_DRAW_MESH 0x60a
#define GMO_CMD_DRAW_PARTICLE 0x60b
#define GMO_CMD_DRAW_RECT_PATCH 0x60c

#define GMO_CMD_ENABLE 0x801
#define GMO_CMD_DISABLE 0x802
#define GMO_CMD_DIFFUSE 0x803
#define GMO_CMD_SPECULAR 0x804
#define GMO_CMD_EMISSION 0x805
#define GMO_CMD_TEX_FACTOR 0x806

#define GMO_CMD_SET_TEXTURE 0x901
#define GMO_CMD_BLEND_FUNC 0x902
#define GMO_CMD_COLOR_GEN 0x903
#define GMO_CMD_TEX_GEN 0x904
#define GMO_CMD_TEX_FUNC 0x905
#define GMO_CMD_TEX_FILTER 0x906
#define GMO_CMD_TEX_WRAP 0x907
#define GMO_CMD_TEX_CROP 0x908
#define GMO_CMD_MAP_COORD 0x90a

#define GMO_CMD_FRAME_LOOP 0xb01
#define GMO_CMD_FRAME_RATE 0xb02
#define GMO_CMD_ANIMATE 0xb03

struct GmoChunk {
	/* 0x00 */ int tag;  /* GMO_BLOCK_* or GMO_CMD_*, with GMO_CHUNK_CONTAINER */
	/* 0x04 */ int size;  /* offset from this chunk to the next sibling */
	/* 0x08 */ int first_child;  /* offset to the first sub-chunk (containers only) */
};

/* Opaque: 8 words copied as a block by set/get_bounding_box. */
struct GmoBoundingBox {
	int v[8];
};

/* gmo_model::state. */
#define GMO_STATE_MAPPER0     0x10  /* mapper i is set: GMO_STATE_MAPPER0 << i */
#define GMO_STATE_MAPPERS     0xf0
#define GMO_STATE_RECONFIGURE 0x8000  /* reconfigure before the next calc */

/* gmo_model::option_flags. */
#define GMO_OPT_LOAD_TEXTURES 0x1  /* gmo_texture::load creates the textures */
#define GMO_OPT_NO_MATERIALS  0x2  /* draw without activating the materials (the Z-only pass of Model) */

/* Root of a GMO model. Aligned to 16; when it is the base of ModelData it
   starts at 0x20, after Hook. */
class gmo_model {
public:
	/* 0x00 */ int state;  /* GMO_STATE_* */
	/* 0x04 */ gm_graph *graph;
	/* 0x08 */ gmo_material *global_material;
	/* 0x0C */ GmoBoundingBox *bounding_box;
	/* 0x10 */ std::vector<gmo_bone *> bones;
	/* 0x1C */ std::vector<gmo_part *> parts;
	/* 0x28 */ std::vector<gmo_material *> materials;
	/* 0x34 */ std::vector<gmo_texture *> textures;
	/* 0x40 */ std::vector<gmo_motion *> motions;
	/* 0x4C */ int option_flags;  /* GMO_OPT_* */
	/* 0x50 */ mat4 world_matrix;
	/* 0x90 */ mat4 mapper[4];
	/* 0x190 */ int current_motion[4];  /* no bounds check */
	/* 0x1A0 */ float transition_time;  /* longest pending transition */
	/* 0x1A4 */ gmo_bone *current_bone;  /* bone being drawn (set by gmo_bone::draw) */
	/* 0x1A8 */ int active_bone;  /* gmo_bone * whose matrix is loaded, 0 after blend matrices */
	/* 0x1AC */ int active_material;  /* index, -2 = none */
	/* 0x1B0 */ int pending_flags;  /* material fields still to send (gmo_material::activate) */
	/* 0x1B4 */ int pending_mask;
	/* 0x1B8 */ int enable_bits;  /* gm_graph enable bits saved by draw() */

	gmo_model(gm_graph &g);
	virtual ~gmo_model();
	void copy(const gmo_model &src);
	void unload();
	void set_graph(gm_graph &g);
	gm_graph &get_graph() const;
	gmo_material &get_global_material() const;
	GmoBoundingBox &get_bounding_box() const;
	void set_world_matrix(const mat4 &m);
	mat4 &get_world_matrix() const;
	int get_current_motion(int i);
	void set_option_flags(int flags);
	void clear_option_flags(int flags);
	int get_option_flags() const;
	gmo_bone *get_bone(int i) const;
	gmo_part *get_part(int i) const;
	gmo_material *get_material(int i) const;
	gmo_texture *get_texture(int i) const;
	gmo_motion *get_motion(int i) const;
	int get_bone_count() const;
	int get_part_count() const;
	int get_material_count() const;
	int get_texture_count() const;
	int get_motion_count() const;
	void notify_configure();
	void set_bounding_box(const GmoBoundingBox *box);
	void restore(float t);
	void reconfigure();
	void update(float dt);
	void calc();
	void draw();
	void set_mapper(int index, const mat4 *m);
	void set_current_motion(int slot, int idx, float t, float u);
	int load(GmoChunk *chunk);
	int load_from_file(const char *name, int index);
	int load_from_file(const void *data, int size, int index, const char *name);
} __attribute__((aligned(16)));

/* gmo.cpp defines GMO_IMPLEMENTATION and provides the out-of-line copies. */
#ifndef GMO_IMPLEMENTATION
inline int gmo_model::get_current_motion(int i)
{
	return current_motion[i];
}

inline void gmo_model::set_option_flags(int flags)
{
	option_flags |= flags;
}

inline void gmo_model::clear_option_flags(int flags)
{
	option_flags &= ~flags;
}

inline gmo_bone *gmo_model::get_bone(int i) const
{
	return (0 <= i && i < (int)bones.size()) ? bones[i] : 0;
}

inline gmo_material *gmo_model::get_material(int i) const
{
	return (0 <= i && i < (int)materials.size()) ? materials[i] : 0;
}

inline gmo_texture *gmo_model::get_texture(int i) const
{
	return (0 <= i && i < (int)textures.size()) ? textures[i] : 0;
}

inline gmo_motion *gmo_model::get_motion(int i) const
{
	return (0 <= i && i < (int)motions.size()) ? motions[i] : 0;
}

inline int gmo_model::get_bone_count() const
{
	return bones.size();
}

inline int gmo_model::get_texture_count() const
{
	return textures.size();
}
#endif

/* gmo_bone::flags. The low byte holds the fields set on the bone; GMO_BONE_CHANGED(f) marks the fields that differ
   from src_bone (update interpolates them back), GMO_BONE_HELD(f) the ones a playing motion holds (restore clears it). */
#define GMO_BONE_MATRIX          0x01  /* mult_matrix */
#define GMO_BONE_TRANSLATE       0x02
#define GMO_BONE_ROTATE          0x04
#define GMO_BONE_SCALE           0x08
#define GMO_BONE_MORPH           0x10  /* morph weights: only in GMO_BONE_CHANGED/GMO_BONE_HELD */
#define GMO_BONE_FIELDS          0x1f
#define GMO_BONE_SCALE_INHERIT   0x10  /* the scale passes to the children (GMO_CMD_SCALE_INHERIT) */
#define GMO_BONE_INHERITED_SCALE 0x20  /* the world matrix carries inherited_scale */
#define GMO_BONE_OFFSET_DIRTY    0x4000  /* rebuild world_offset */
#define GMO_BONE_SCALED_DIRTY    0x8000  /* rebuild scaled_world_mat */
#define GMO_BONE_CHANGED(f)      ((f) << 16)
#define GMO_BONE_HELD(f)         ((f) << 24)
#define GMO_BONE_ANIMATED(f)     ((f) | GMO_BONE_CHANGED(f) | GMO_BONE_HELD(f))  /* set by a motion */

/* Skeleton bone. */
class gmo_bone {
public:
	/* 0x00 */ gmo_model *model;
	/* 0x04 */ const gmo_bone *src_bone;  /* bone this one was copied from, or 0 */
	/* 0x08 */ int flags;  /* GMO_BONE_* */
	/* 0x0C */ int parent;  /* bone index, -1 = none */
	/* 0x10 */ int visible_parts;  /* bit i: draw part_indices[i] */
	/* 0x14 */ float transition_time;  /* remaining */
	/* 0x18 */ GmoBoundingBox *bounding_box;
	/* 0x1C */ int pad_1C;
	/* 0x20 */ mat4 mult_matrix;
	/* 0x60 */ vec4 translate;
	/* 0x70 */ vec4 rotate;
	/* 0x80 */ vec4 scale;
	/* 0x90 */ vec4 inherited_scale;  /* flag 0x20 */
	/* 0xA0 */ mat4 world_mat;
	/* 0xE0 */ mat4 scaled_world_mat;  /* world_mat scaled by inherited_scale */
	/* 0x120 */ mat4 world_offset;
	/* 0x160 */ mat4 local_offset;
	/* 0x1A0 */ std::vector<int> part_indices;
	/* 0x1AC */ std::vector<float> morph_weights;
	/* 0x1B8 */ std::vector<int> blend_bones;  /* bone indices of the blend matrices */
	/* 0x1C4 */ std::vector<mat4> blend_offsets;  /* local offset of each blend bone */

	gmo_bone(gmo_model *m);
	~gmo_bone();
	void copy(const gmo_bone &src);
	void reconfigure();
	void set_bounding_box(const GmoBoundingBox *box);
	void restore(float t);
	void activate();
	mat4 &get_world_matrix() const;
	mat4 &get_world_offset() const;
	void set_local_offset(const mat4 &m);
	void set_mult_matrix(const mat4 &m);
	void get_mult_matrix(mat4 &out) const;
	void set_translate(const vec4 &v);
	void get_translate(vec4 &out) const;
	void set_rotate(const vec4 &v);
	void get_rotate(vec4 &out) const;
	void set_scale(const vec4 &v);
	void get_scale(vec4 &out) const;
	void set_morph_weights(const float *w, int n);
	int load(GmoChunk *chunk);
	void update(float dt);
	void calc();
	void draw();
	void animate(int tag, int index, float *data, int count, float t);
	void animate_interp(int tag, int index, float *data, int count, float t);
	const std::vector<float> &get_morph_weights() const;
};

/* Texture layer of a gmo_material (up to 4 per material). */
class gmo_layer {
public:
	/* 0x00 */ gmo_model *model;
	/* 0x04 */ gmo_material *material;  /* owner */
	/* 0x08 */ int index;  /* index in material->layers[] */
	/* 0x0C */ int flags;
	/* 0x10 */ int texture_index;  /* active index, reference counted on the global material */
	/* 0x14 */ int texture;  /* index in model->textures, -1 if none */
	/* 0x18 */ int blend_func;
	/* 0x1C */ int color_gen_mode, color_gen_arg;
	/* 0x24 */ int tex_gen_mode, tex_gen_arg;
	/* 0x2C */ int tex_func_mode, tex_func_arg;
	/* 0x34 */ int tex_wrap_mode, tex_wrap_arg;
	/* 0x3C */ int tex_filter_mode, tex_filter_arg;
	/* 0x44 */ int map_coord_mode, map_coord_arg;
	/* 0x4C */ int pad_4C;
	/* 0x50 */ vec4 tex_crop;

	gmo_layer(gmo_material *mat, int index);
	~gmo_layer();
	void copy(const gmo_layer &src);
	void reconfigure();
	void set_flags(int flags);
	void set_texture_handle(int index);
	void set_texture(int index);
	void set_blend_func(int func);
	void set_color_gen(int mode, int arg);
	void set_tex_gen(int mode, int arg);
	void set_tex_func(int mode, int arg);
	void set_tex_wrap(int mode, int arg);
	void set_tex_filter(int mode, int arg);
	void set_tex_crop(const vec4 &crop);
	void set_map_coord(int mode, int arg);
	void activate(int mask);
	int load(GmoChunk *chunk);
};

/* Material. */
class gmo_material {
public:
	/* 0x00 */ gmo_model *model;
	/* 0x04 */ const gmo_material *src_material;  /* material this one was copied from, or 0 */
	/* 0x08 */ int
	    flags; /* bit 2 diffuse, 3 specular, 4 emission, 5 tex_factor; 0x40 << i layer i; 0x8000 global material */
	/* 0x0C */ int layer_flags, transition_flags, enable_mask, enable_value;
	/* 0x1C */ float transition_time;
	/* 0x20 */ gmo_layer *layers[4]; /* allocated on demand */
	/* 0x30 */ vec4 diffuse;
	/* 0x40 */ vec4 specular;
	/* 0x50 */ vec4 emission;
	/* 0x60 */ vec4 tex_factor;
	/* 0x70 */ float specular_power;
	/* 0x74 */ float tex_factor_a, tex_factor_b;
	/* 0x7C */ char pad_7C[0x80 - 0x7C];

	gmo_material(gmo_model *model, bool global);
	~gmo_material();
	gmo_layer *alloc_layer(int index);
	void force_global_layer(int index, int mask);
	void force_global_material(int mask);
	void enable(int mask, bool on);
	void disable(int mask, bool on);
	void copy(const gmo_material &src);
	void reconfigure();
	void restore(float t);
	void set_layer_texture_handle(int index, int handle);
	void set_layer_blend_func(int index, int func);
	void set_layer_color_gen(int index, int mode, int arg);
	void set_layer_tex_gen(int index, int mode, int arg);
	void set_layer_tex_func(int index, int mode, int arg);
	void set_layer_tex_wrap(int index, int mode, int arg);
	void set_layer_tex_filter(int index, int mode, int arg);
	void set_layer_tex_crop(int index, const vec4 &crop);
	void set_layer_map_coord(int index, int mode, int arg);
	void get_layer_texture_handle(int index, int &handle) const;
	void get_layer_blend_func(int index, int &func) const;
	void get_layer_color_gen(int index, int &mode, int &arg) const;
	void get_layer_tex_gen(int index, int &mode, int &arg) const;
	void get_layer_tex_func(int index, int &mode, int &arg) const;
	void get_layer_tex_wrap(int index, int &mode, int &arg) const;
	void get_layer_tex_filter(int index, int &mode, int &arg) const;
	void get_layer_tex_crop(int index, vec4 &crop) const;
	void get_layer_map_coord(int index, int &mode, int &arg) const;
	void get_diffuse(vec4 &out) const;
	void set_diffuse(const vec4 &c);
	void get_specular(vec4 &out, float &power) const;
	void set_specular(const vec4 &c, float power);
	void get_emission(vec4 &out) const;
	void set_emission(const vec4 &c);
	void get_tex_factor(vec4 &out, float &a, float &b) const;
	void set_tex_factor(const vec4 &c, float a, float b);
	int load(GmoChunk *chunk);
	void activate();
	void update(float dt);
	void animate(int a, int b, float *w, int n, float t);
	void animate_interp(int a, int b, float *w, int n, float t);
	void set_layer_flags(int index, int flags);
};

/* Texture descriptor, indexed by model->textures[]. */
class gmo_texture {
public:
	/* 0x00 */ gmo_model *model;
	/* 0x04 */ int unk_04;
	/* 0x08 */ int texture_handle; /* reference counted through model->graph */
	/* 0x0C */ char *file_name; /* new[] */
	/* 0x10 */ int *image_data; /* memalign(16), last word zeroed */
	/* 0x14 */ int image_len; /* unrounded length */

	gmo_texture(gmo_model *m);
	~gmo_texture();
	void copy(const gmo_texture &src);
	void reconfigure();
	void set_texture_handle(int index);
	void set_file_name(const char *name);
	void set_file_image(int *data, int len);
	int load(GmoChunk *chunk);
};

/* Polygon mesh of a gmo_part. */
class gmo_mesh {
public:
	/* 0x00 */ gmo_model *model;
	/* 0x04 */ gmo_part *part; /* owner */
	/* 0x08 */ int shader_flags; /* bit 0 / 1: vertex shader -2 / -3 */
	/* 0x0C */ int vb_handle;
	/* 0x10 */ int material_index; /* -1: the global material */
	/* 0x14 */ std::vector<int> bone_slots; /* indices into blend_bones of the bone being drawn */

	gmo_mesh(gmo_part *p);
	~gmo_mesh();
	void copy(const gmo_mesh &src);
	void reconfigure();
	void draw();
	int load(GmoChunk *chunk);
};

/* Element of gmo_part::vertex_arrays, freed with plain operator delete. */
struct GmoArrays {
	/* 0x00 */ int format; /* the four header words are passed by gmo_mesh::load to gm_graph::SetArrays */
	/* 0x04 */ int vertex_count;
	/* 0x08 */ int morph_count; /* vertex sets */
	/* 0x0C */ int morph_format; /* format of the sets after the first */
	/* vertex array data follows at +0x10 */
};

/* Group of meshes inside a gmo_model. */
class gmo_part {
public:
	/* 0x00 */ gmo_model *model;
	/* 0x04 */ int unk_04;
	/* 0x08 */ std::vector<gmo_mesh *> meshes;
	/* 0x14 */ std::vector<GmoArrays *> vertex_arrays;

	gmo_part(gmo_model *m);
	~gmo_part();
	void copy(const gmo_part &src);
	void reconfigure();
	void draw();
	int load(GmoChunk *chunk);
};

/* Element of gmo_anim_data::keys (16 bytes, trivial destructor). */
struct GmoAnimate {
	/* 0x00 */ int target; /* bits 16-23: block type (4 bone, 8 material); bits 0-15: signed index */
	/* 0x04 */ int property, property_index, fcurve;
};

/* GmoFCurve::type. */
#define GMO_FCURVE_CONSTANT   0  /* step */
#define GMO_FCURVE_LINEAR     1
#define GMO_FCURVE_SPHERICAL  4  /* quaternion slerp */
#define GMO_FCURVE_COMPRESSED 0x8000  /* file header only: shorts with a scale and an offset per component */

/* Function curve built by gmo_load_fcurve and evaluated by
   gmo_apply_curve; element of gmo_anim_data::fcurves. */
struct GmoFCurve {
	/* 0x00 */ int type; /* interpolation, GMO_FCURVE_* (GMO_FCURVE_COMPRESSED in the file header) */
	/* 0x04 */ int nvalues; /* values per key, time excluded */
	/* 0x08 */ int nkeys;
	/* 0x0C */ int unk_0C;
	/* 0x10 */ float data[1]; /* nkeys keys of (1 + nvalues) floats: time, values */
};

/* Animation applicable to a gmo_model. */
class gmo_motion {
public:
	/* Reference-counted animation data shared by gmo_motion copies. */
	class gmo_anim_data {
	public:
		/* 0x00 */ int refcount;
		/* 0x04 */ std::vector<GmoAnimate> keys;
		/* 0x10 */ std::vector<GmoFCurve *> fcurves;

		gmo_anim_data();
		~gmo_anim_data();
		gmo_anim_data *duplicate();
		void release();
	};

	/* 0x00 */ gmo_model *model;
	/* 0x04 */ int unk_04;
	/* 0x08 */ gmo_anim_data *anim_data; /* reference counted */
	/* 0x0C */ float frame_rate; /* 60 by default */
	/* 0x10 */ float loop_start;
	/* 0x14 */ float loop_end;
	/* 0x18 */ float time;
	/* 0x1C */ float current_frame;
	/* 0x20 */ float interp;

	gmo_motion(gmo_model *m);
	~gmo_motion();
	void copy(const gmo_motion &src);
	void reconfigure();
	void set_frame_rate(float f);
	void set_frame(float f);
	void set_interp(float f);
	void set_loop_start(float t);
	void set_loop_end(float t);
	void start(float delay, float t);
	void set_time(float t);
	void stop(float t);
	void update(float dt);
	int load(GmoChunk *chunk);
};
