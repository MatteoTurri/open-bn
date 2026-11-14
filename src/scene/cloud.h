/*
 * Cloud: particle cloud effect of the scene.
 */

#ifndef CLOUD_H
#define CLOUD_H

#include <vector>
#include "gfx/vec_types.h"
#include "object.h"

class gm_graph;
struct CloudParticle;

/* Cloud::alpha_shape, option RAYM: the alpha of a particle from its position (CalcAlphaValue). */
#define CLOUD_ALPHA_SPHERE  0
#define CLOUD_ALPHA_SHELL   1
#define CLOUD_ALPHA_VOLUME  2  /* trilinear sampling of the noise volume of option RAY (also any other value) */
#define CLOUD_ALPHA_NOISE1D 3  /* 1D noise along x, scrolled by the phase */
#define CLOUD_ALPHA_COSINES 4

/*
 * Particle cloud: two particle arrays drawn as sprites, with an optional
 * full-screen post effect; configured through options.
 */
class Cloud : public Object {
public:
	/* 0x250 */ int num_cloud_tex;   /* option CLUD: texture handles of the particles */
	/* 0x254 */ int cloud_tex;      /* const int *, indexed by cloud_tex_id */
	/* 0x258 */ int num_dcloud_tex;  /* option DCLD: the same for the second array */
	/* 0x25C */ int dcloud_tex;
	/* 0x260 */ int ray_width;      /* option RAY: noise volume of CLOUD_ALPHA_VOLUME */
	/* 0x264 */ int ray_height;
	/* 0x268 */ int ray_depth;
	/* 0x26C */ int ray_slices;     /* const unsigned char **, ray_depth slices */
	/* 0x270 */ float jitter_r;     /* option CDW: color jitter per channel */
	/* 0x274 */ float jitter_g;
	/* 0x278 */ float jitter_b;
	/* 0x27C */ float jitter_a;
	/* 0x280 */ int cloud_tex_id;    /* option CLID */
	/* 0x284 */ int dcloud_tex_id;   /* option DCID */
	/* 0x288 */ int num_sprites;    /* option NSPR: particles of the first array drawn */
	/* 0x28C */ int num_dsprites;   /* option NDSP */
	/* 0x290 */ int alpha_shape;    /* option RAYM, CLOUD_ALPHA_* */
	/* 0x294 */ int ray_x;          /* option RAYX, modulo ray_width */
	/* 0x298 */ int ray_y;          /* option RAYY, modulo ray_height */
	/* 0x29C */ int ray_scale_x;     /* option RAYW */
	/* 0x2A0 */ int ray_scale_y;     /* option RAYH */
	/* 0x2A4 */ int color_dirty;    /* recompute the particle colors before drawing */
	/* 0x2A8 */ float speed;       /* option SPD0 */
	/* 0x2AC */ float ray_offset_x;  /* option RXO */
	/* 0x2B0 */ float ray_offset_y;  /* option RYO */
	/* 0x2B4 */ int pad_2B4[6];
	/* 0x2CC */ int num_particles;
	/* 0x2D0 */ int num_dparticles;
	/* 0x2D4 */ CloudParticle *particles;
	/* 0x2D8 */ CloudParticle *dparticles;
	/* 0x2DC */ float offset_x;     /* option XOFS */
	/* 0x2E0 */ float offset_y;     /* option YOFS */
	/* 0x2E4 */ float offset_z;     /* option ZOFS */
	/* 0x2E8 */ int animate_noise;  /* option RSCL: advance the noise phase every frame */
	/* 0x2EC */ int color_buffer;   /* option BSIZ: post effect buffer, 0 = none */

	Cloud(Object *parent, int count, int unused);
	virtual ~Cloud();

	virtual void DrawThis(gm_graph &g);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const String &value);
	virtual int SetOption(int option, const void *data, unsigned int size);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, String &value) const;
	virtual int GetOption(int option, void *data, unsigned int size) const;

	virtual void SetColor(float r, float g, float b, float a);

	void InitModel();
	void CalcModel();
	void InitColor();
	int CalcAlphaValue(int i, vec4 *out);
	void UpdatePosition();
	void CheckPosRange();

	/* Class registrar; its global instance is __Cloud_ClassInitializer__. */
	class ClassInitializer {
	public:
		ClassInitializer();

	private:
		int dummy;  /* unused, but part of the layout */
	};
};

#endif
