/*
 * Particle: Object drawing point sprites or quads from shared shapes
 * (shape_info, up to 16 morph targets) through a list of instances (item_info).
 */

#ifndef PARTICLE_H
#define PARTICLE_H

#pragma interface

#include <vector>
#include "gfx/vec_types.h"
#include "gfx/gm_graph.h"
#include "particle_base.h"

/* Particle::flags. */
#define PARTICLE_SHAPE_CHANGED 0x001  /* the shared shape changed: fixup_shape_sharing drops the items' own shapes */
#define PARTICLE_ITEM_SHAPES   0x100  /* some item has its own shape */
#define PARTICLE_TEX_NONPOW2   0x200  /* the texture is not a power of two */

/* Particle::shape_info::flags. */
#define PARTICLE_LIST_DIRTY   0x1  /* rebuild the display list */
#define PARTICLE_ARRAYS_DIRTY 0x2  /* rebuild the interleaved vertices */

/* Particle::shape_info::type. */
#define PARTICLE_QUADS  0  /* a textured quad per point */
#define PARTICLE_POINTS 1
#define PARTICLE_LINES  2

class Particle : public Object {
public:
	class shape_info {
	public:
		/* 0x00 */ int flags;  /* PARTICLE_LIST_DIRTY, PARTICLE_ARRAYS_DIRTY */
		/* 0x04 */ int type;  /* PARTICLE_QUADS, otherwise one vertex per point */
		/* 0x08 */ float size;  /* point size */
		/* 0x0C */ int list;  /* display list */
		/* 0x10 */ int point_count;  /* 0..4096 */
		/* 0x14 */ int morph_count;  /* morph targets, 1..16 */
		/* 0x18 */ int format;  /* vertex layout, GM_VERTEX_* */
		/* 0x1C */ int morph_format;  /* vertex layout of the other morph targets */
		/* 0x20 */ vector<float> vertices;  /* interleaved vertices built by fixup_arrays */
		/* 0x2C */ vector<float> positions;
		/* 0x38 */ vector<float> normals;
		/* 0x44 */ vector<float> pointsizes;
		/* 0x50 */ vector<int> colors;  /* packed RGBA */
		/* 0x5C */ vector<float> texcoords;
		/* 0x68 */ vector<float> axes;

		shape_info();
		shape_info(const shape_info &o);
		shape_info &operator=(const shape_info &o);
		~shape_info();

		void set_type(int t);
		void set_size(float s);
		void set_point_count(int n);
		void set_morph_count(int n);

		void set_points(vector<float> &dst, const float *src, int count, int start, int clamp, int comps = 3);
		void get_points(const vector<float> &src, float *dst, int count, int start, int comps);

		void set_positions(const float *src, int count, int start, int clamp);
		void set_normals(const float *src, int count, int start, int clamp);
		void set_pointsizes(const float *src, int count, int start, int clamp);
		void set_texcoords(const float *src, int count, int start, int clamp);
		void set_axes(const float *src, int count, int start, int clamp);
		void set_colors(const float *src, int count, int start, int clamp);

		void get_positions(float *dst, int count, int start);
		void get_normals(float *dst, int count, int start);
		void get_pointsizes(float *dst, int count, int start);
		void get_texcoords(float *dst, int count, int start);
		void get_axes(float *dst, int count, int start);
		void get_colors(float *dst, int count, int start);

		void fixup_arrays();
		void draw_shape(gm_graph &g);
		void draw(gm_graph &g);
	};

	/* One instance of the particle system, with its own shape or a shared
	   master shape. */
	class item_info {
	public:
		/* 0x00 */ int flags;  /* ITEM_* */
		/* 0x04 */ shape_info *master_shape;
		/* 0x08 */ shape_info *own_shape;  /* owned shape, never shared by copies */
		/* 0x0C */ float size;  /* option 'SIZE' */
		/* 0x10 */ int disp;  /* option 'DISP', DISP_* */
		/* 0x14 */ int blend;  /* option 'ALPH' */
		/* 0x18 */ int morph_mode;  /* option 'MMOD', MORPH_* */
		/* 0x1C */ float morph_time;  /* option 'MTIM' */
		/* 0x20 */ vector<float> weights;  /* morph weights, option 'MWGT' */
		/* 0x2C */ int pad_2C;
		/* Six vec4 copied as whole 16-byte blocks. */
		union Block24 {
			float v[24];
			struct {
				fvec4 morph_range, diffuse, crop, pos, rot, scale;
			} q;
		};
		/* 0x30 */ Block24 vec_options;  /* 'MTRG', 'DIFF', 'CROP', 'POS', 'ROT', 'SCAL' */

		item_info();
		item_info(const item_info &o);
		item_info &operator=(const item_info &o);
		~item_info();

		shape_info *get_shape() const;
		void set_master_shape(shape_info *s);
		shape_info *alloc_shape();
		void free_shape();
		/* Returns 0 if the item should not be drawn. */
		int set_morph_weights(gm_graph &g);
		void draw(gm_graph &g, float f);
	};

	/* 0x250 */ int flags;  /* PARTICLE_* */
	/* 0x254 */ int tex_handle;
	/* 0x258 */ vector<item_info> items;
	/* 0x264 */ shape_info shape;
	/* 0x2D8 */ int pmod;  /* option 'PMOD' */
	/* 0x2DC */ int pidx;  /* option 'PIDX' */

	Particle(Object *parent);
	virtual ~Particle();
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
};

#endif
