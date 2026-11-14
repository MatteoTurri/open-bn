/*
 * Mesh: Object drawing a grid surface (count_u x count_v points, up to 16
 * morph targets) as a list of instances (item_info) that may share shapes.
 */

#ifndef MESH_H
#define MESH_H

#include <vector>
#include "gfx/vec_types.h"
#include "gfx/gm_graph.h"
#include "object.h"
#include "scenefacade.h"

class Object;
class gm_graph;

#pragma interface

/* Mesh::flags. */
#define MESH_SHAPE_CHANGED 0x001  /* the shared shape changed: fixup_shape_sharing drops the items' own shapes */
#define MESH_ITEM_SHAPES   0x100  /* some item has its own shape */
#define MESH_TEX_NONPOW2   0x200  /* the texture is not a power of two */

/* Mesh::shape_info::flags. */
#define MESH_LISTS_DIRTY  0x1  /* rebuild the display lists */
#define MESH_ARRAYS_DIRTY 0x2  /* rebuild the control grid */

/* Mesh::shape_info::type: the patch kind in the low byte, plus the wrap bits. */
#define MESH_TYPE_BILINEAR 17      /* bilinear patch; any other kind is a uniform cubic B-spline */
#define MESH_TYPE_WRAP_U   0x1000  /* closed along u */
#define MESH_TYPE_WRAP_V   0x2000  /* closed along v */

/* Mesh::item_info::flags, besides ITEM_*. */
#define MESH_ITEM_MORPH_DIRTY 0x8000  /* recompute the morph weights ('MTIM' set) */

class Mesh : public Object {
public:
	class item_info;

	class shape_info {
	public:
		/* 0x00 */ int flags;  /* MESH_LISTS_DIRTY, MESH_ARRAYS_DIRTY */
		/* 0x04 */ int type;  /* shape type, MESH_TYPE_* */
		/* 0x08 */ vector<int> lists;  /* display lists */
		/* 0x14 */ int count_u;  /* points along u, 1..256 */
		/* 0x18 */ int count_v;  /* points along v, 1..256 */
		/* 0x1C */ int morph_count;  /* morph targets, 1..16 */
		/* 0x20 */ vector<float> control_grid;  /* control grid with guard rows (fixup_arrays) */
		/* 0x2C */ vector<float> positions;  /* xyz per point and morph target */

		shape_info();
		shape_info(const shape_info &o);
		shape_info &operator=(const shape_info &o);
		~shape_info();

		void set_type(int t);
		void set_point_count(int u, int v);
		void set_morph_count(int n);

		void set_positions(const float *src, int count, int start, int clamp);
		void get_positions(float *dst, int count, int start);

		int fixup_arrays();
		int fixup_disp_lists();
		vec4 calc_surface(float a, float b, int idx, int morph) const;
		void draw(gm_graph &g, const item_info &it);
	};

	/* One instance of the mesh, with its own shape or a shared master shape. */
	class item_info {
	public:
		/* 0x00 */ mutable int flags;  /* ITEM_*, MESH_ITEM_MORPH_DIRTY */
		/* 0x04 */ shape_info *master_shape;
		/* 0x08 */ shape_info *own_shape;  /* owned shape, never shared by copies */
		/* 0x0C */ int disp;  /* option 'DISP', DISP_* */
		/* 0x10 */ int blend;  /* option 'ALPH' */
		/* 0x14 */ vector<float> div_u;  /* option 'DIVU' */
		/* 0x20 */ vector<float> div_v;  /* option 'DIVV' */
		/* 0x2C */ float div_level;  /* option 'DIVL' */
		/* 0x30 */ int morph_mode;  /* option 'MMOD', MORPH_* */
		/* 0x34 */ float morph_time;  /* option 'MTIM' */
		/* 0x38 */ vector<float> weights;  /* morph weights */
		/* 0x44 */ vec3 pad_44;
		/* 0x50 */ fvec4 morph_range;  /* option 'MTRG' */
		/* 0x60 */ fvec4 diffuse;  /* option 'DIFF' */
		/* 0x70 */ fvec4 clip;  /* option 'CLIP' */
		/* 0x80 */ fvec4 crop;  /* option 'CROP' */
		/* 0x90 */ fvec4 pos;  /* option 'POS' */
		/* 0xA0 */ fvec4 rot;  /* option 'ROT' */
		/* 0xB0 */ fvec4 scale;  /* option 'SCAL' */
		/* 0xC0 */ float surf_u;  /* option 'SUV': surface point of calc_surface */
		/* 0xC4 */ float surf_v;
		/* 0xC8 */ vec2 pad_C8;

		item_info();
		item_info(const item_info &o);
		item_info &operator=(const item_info &o);
		~item_info();

		shape_info *get_shape() const;
		void set_master_shape(shape_info *s);
		shape_info *alloc_shape();
		void free_shape();
		void draw(gm_graph &g);
		vec4 calc_surface(int idx) const;
		/* Returns 0 if the item should not be drawn. */
		int set_morph_weights();
	};

	/* 0x250 */ int flags;  /* MESH_* */
	/* 0x254 */ int tex_handle;
	/* 0x258 */ vector<item_info> items;
	/* 0x264 */ shape_info shape;
	/* Mesh-wide options (not per item). */
	/* 0x29C */ int pmod;  /* option 'PMOD' */
	/* 0x2A0 */ int pidx;  /* option 'PIDX' */
	/* 0x2A4 */ int face;  /* option 'FACE' */
	/* 0x2A8 */ int fade;  /* option 'FADE' */
	/* 0x2AC */ int anti;  /* option 'ANTI' */

	Mesh(Object *parent);
	virtual ~Mesh();
	void fixup_shape_sharing();

	virtual void DrawThis(gm_graph &g);
	virtual void SetTexture(int texture, int option, int mode);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const void *data, unsigned int size);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, void *data, unsigned int size) const;
	virtual int SetItemOption(int option, int item, const int &value, unsigned int size);
	virtual int SetItemOption(int option, int item, const float &value, unsigned int size);
	virtual int SetItemOption(int option, int item, const void *data, unsigned int size);
	virtual int GetItemOption(int option, int item, int &value, unsigned int size) const;
	virtual int GetItemOption(int option, int item, float &value, unsigned int size) const;
	virtual int GetItemOption(int option, int item, void *data, unsigned int size) const;

	void DrawItems(gm_graph &g);
};

#endif
