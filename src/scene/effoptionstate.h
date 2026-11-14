/*
 * EffOptionState: the rendering options of a scene Object.
 */

#ifndef EFFOPTIONSTATE_H
#define EFFOPTIONSTATE_H

#include "gfx/vec_types.h"

class Object;
class gm_graph;

/* Options of an EffOptionState: bits of `flags`, `never_mask`, `always_mask` and of the Activate masks. */
#define EFF_DEPTH_OFFSET 0x1
#define EFF_LIGHTING 0x2  /* lighting mode and light group */
#define EFF_AMBIENT 0x4
#define EFF_FOG_MODE 0x8
#define EFF_FOG_COLOR 0x10
#define EFF_FOG_RANGE 0x20
#define EFF_TEXTURE_MODE 0x40
#define EFF_TEXTURE_CLAMP 0x80
#define EFF_TEXTURE_FILTER 0x100
#define EFF_TEXTURE_CROP 0x200
#define EFF_DIFFUSE 0x10000
#define EFF_SPECULAR 0x20000
#define EFF_EMISSION 0x40000
#define EFF_TEX_FACTOR 0x80000
#define EFF_PIXEL_TEST 0x100000  /* pixel test and the alpha/stencil references */
#define EFF_PIXEL_WRITE 0x200000
#define EFF_LAYER0 0x400000  /* layer n is EFF_LAYER0 << n */
#define EFF_LIGHT_RELOAD 0x40000000  /* reload every light of the group, not only the changed ones */
#define EFF_ROOT 0x80000000  /* the state of the root scene, which holds every default */

/* The options handled by each Activate*State. */
#define EFF_ENVIRONMENT_MASK 0x3e
#define EFF_TEXTURE_MASK 0x3c0
#define EFF_MATERIAL_MASK 0xf0000
#define EFF_PIXEL_MASK 0x300001
#define EFF_LAYER_MASK 0x3c00000

/* `option` of SetTexture/SetObject: EFF_SLOT_LAYER0 + n is layer n (0..3); 0 is layer 0 too. */
#define EFF_SLOT_LAYER0 256
/*
 * Per-object rendering state: the options set on this object (`flags`, EFF_*), some
 * inline modes and four lazily allocated blocks (environment, material,
 * pixel, layers); unset options are read from the state of `object`, the
 * root scene.
 */
class EffOptionState {
public:
	struct lay_params {
		/* 0x00 */ int mode[4];
		/* 0x10 */ int texture[4];
		/* 0x20 */ int object[4];
		lay_params();
	};
	struct env_params {
		/* 0x00 */ vec4 ambient;
		/* 0x10 */ vec4 fog_color;
		/* 0x20 */ float fog_near;
		/* 0x24 */ float fog_far;
		/* 0x28 */ int pad[2];
	};
	struct mtl_params {
		/* 0x00 */ vec4 diffuse;
		/* 0x10 */ vec4 specular;
		/* 0x20 */ vec4 emission;
		/* 0x30 */ vec4 tex_factor;
		/* 0x40 */ float specular_power;
		/* 0x44 */ float tex_factor_a;
		/* 0x48 */ float tex_factor_b;
		/* 0x4C */ int pad;
	};
	struct pix_params {
		/* 0x00 */ float depth_offset;
		/* 0x04 */ int pixel_test;
		/* 0x08 */ int pixel_write;
		/* 0x0C */ int reference_alpha;
		/* 0x10 */ int reference_stencil;
	};

	/* 0x00 */ Object *owner;
	/* 0x04 */ Object *object;
	/* 0x08 */ unsigned int flags;
	/* 0x0C */ int never_mask;  /* options not propagated to the parent */
	/* 0x10 */ int always_mask;  /* options always reported as set */
	/* 0x14 */ int lighting_mode;
	/* 0x18 */ int light_group;
	/* 0x1C */ int fog_mode;
	/* 0x20 */ int texture_mode;
	/* 0x24 */ int texture_clamp;
	/* 0x28 */ int texture_filter;
	/* 0x2C */ int pad_2C;
	/* 0x30 */ vec4 texture_crop;
	/* 0x40 */ env_params *env;
	/* 0x44 */ mtl_params *mtl;
	/* 0x48 */ pix_params *pix;
	/* 0x4C */ lay_params *lay;

	EffOptionState();
	~EffOptionState();
	void Init(Object *obj);
	void Term();
	int Activate(int mask);
	void ActivateEnvironmentState(gm_graph &g, int mask);
	int ActivateLayerState(gm_graph &g, int mask);
	void ActivatePixelState(gm_graph &g, int mask);
	void ActivateTextureState(gm_graph &g, int mask);
	void ActivateMaterialState(gm_graph &g, int mask);
	int SetOption(int option, const int &value, unsigned int size);
	int SetOption(int option, const void *data, unsigned int size);
	int SetOption(int option, const float &value, unsigned int size);
	int GetOption(int option, int &value, unsigned int size) const;
	int GetOption(int option, void *data, unsigned int size) const;
	int GetOption(int option, float &value, unsigned int size) const;
	int HasAutoCrop() const;
	void SetLightingMode(int mode);
	void SetLightGroup(int group);
	void SetFogMode(int mode);
	void SetTextureMode(int mode);
	void SetTextureClamp(int clamp);
	void SetTextureFilter(int filter);
	void SetTextureCrop(const vec4 &crop);
	void SetAmbient(const vec4 &color);
	void SetFogColor(const vec4 &color);
	void SetFogRange(float near, float far);
	void SetDiffuse(const vec4 &color);
	void SetSpecular(const vec4 &color, float power);
	void SetEmission(const vec4 &color);
	void SetTexFactor(const vec4 &color, float a, float b);
	void SetDepthOffset(float offset);
	void SetPixelTest(int test);
	void SetPixelWrite(int write);
	void SetReferenceAlpha(int alpha);
	void SetReferenceStencil(int stencil);
	void SetLayerMode(int no, int mode);
	void SetTexture(int texture, int option, int mode);
	void SetObject(Object *obj, int option, int mode);
	int GetLightingMode() const;
	int GetLightGroup() const;
	int GetFogMode() const;
	int GetTextureMode() const;
	int GetTextureClamp() const;
	int GetTextureFilter() const;
	void GetTextureCrop(vec4 &crop) const;
	void GetAmbient(vec4 &color) const;
	void GetFogColor(vec4 &color) const;
	void GetFogRange(float &near, float &far) const;
	void GetDiffuse(vec4 &color) const;
	void GetSpecular(vec4 &color, float &power) const;
	void GetEmission(vec4 &color) const;
	void GetTexFactor(vec4 &color, float &a, float &b) const;
	float GetDepthOffset() const;
	int GetPixelTest() const;
	int GetPixelWrite() const;
	int GetReferenceAlpha() const;
	int GetReferenceStencil() const;
	int GetLayerMode(int no) const;
	int GetLayerTexture(int no) const;
	const vec4 *GetAmbient() const;
	const vec4 *GetFogColor() const;
	const vec4 *GetDiffuse() const;
	const vec4 *GetSpecular() const;
	const vec4 *GetEmission() const;
	const vec4 *GetTexFactor() const;
	const vec4 *GetTextureCrop() const;
	unsigned int GetColorMask() const;
}; /* size = 0x50 */

#endif
