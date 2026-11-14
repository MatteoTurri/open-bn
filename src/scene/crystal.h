/*
 * Crystal: glass cube effect of the scene.
 */

#ifndef CRYSTAL_H
#define CRYSTAL_H

#include "gfx/vec_types.h"
#include "gfx/gm_graph.h"
#include <vector>
#include "object.h"

class Object;

/*
 * Glass cube drawn with refraction, reflection, mirror cube, gloss map and
 * highlight passes through two color buffers; configured through options.
 */
/* Crystal::flags. */
#define CRYSTAL_ZBUF   0x001  /* option ZBUF: both color buffers with a Z buffer */
#define CRYSTAL_FADING 0x100  /* the fade factor is below 0.99 */

/* The cube itself and its six neighbours. */
#define CRYSTAL_SHAPE_COUNT 7

class Crystal : public Object {
public:
	/* 0x250 */ int flags;  /* CRYSTAL_* */
	/* 0x254 */ int shape_lists[CRYSTAL_SHAPE_COUNT];      /* display lists of the shapes */
	/* 0x270 */ int color_buf_a;
	/* 0x274 */ int color_buf_b;
	/* 0x278 */ int pad_278[2];
	/* 0x280 */ vec4 refract_color[2];   /* options "RFC0"/"RFC1" */
	/* 0x2A0 */ float refract_factor[2]; /* options "RFF0"/"RFF1" */
	/* 0x2A8 */ int pad_2A8[2];
	/* 0x2B0 */ vec4 reflect_color[2];   /* options "EMC0"/"EMC1" */
	/* 0x2D0 */ int reflect_tex[2];      /* option "EMT" */
	/* 0x2D8 */ int pad_2D8[2];
	/* 0x2E0 */ vec4 mirror_color;       /* option "MMC": mirror cube */
	/* 0x2F0 */ int mirror_tex;          /* option "MMT" */
	/* 0x2F4 */ int pad_2F4[3];
	/* 0x300 */ vec4 gloss_color;        /* option "GMC": gloss map */
	/* 0x310 */ int gloss_tex;           /* option "GMT" */
	/* 0x314 */ float fade;             /* option "FADE" */
	/* 0x318 */ float fade_near;         /* option "NEAR": distance where the fade starts */
	/* 0x31C */ int hilight_tex;
	/* 0x320 */ mat4 world_matrix;       /* GetMatrix(1) x world matrix */
	/* 0x360 */ mat4 proj_basis;         /* projection basis of DrawRefraction */

	Crystal(Object *parent);
	virtual ~Crystal();

	virtual void DrawThis(gm_graph &g);
	virtual void SetTexture(int texture, int option, int mode);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const void *data, unsigned int size);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, void *data, unsigned int size) const;

	void DrawShape(gm_graph &g, int shape);
	void DefineShape(gm_graph &g, int shape);
	void AddColorBuffer(gm_graph &g, int color_buffer, int texture, bool blend);
	void MixColorBuffer(gm_graph &g, int a, int b, int c);
	float GetFadeFactor();
	void DrawRefraction(gm_graph &g, int side);
	void DrawReflection(gm_graph &g, int side);
	void DrawMirrorCube(gm_graph &g, int side);
	void DrawGlossMap(gm_graph &g, int side);
	void DrawHilight(gm_graph &g, int side);
};

#endif
