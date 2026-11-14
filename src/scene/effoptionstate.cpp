/*
 * EffOptionState: the per-object rendering options (lighting, fog, texture,
 * material, pixel tests, layers), their inheritance and their activation on
 * gm_graph.
 */

#include <vector>
#include "gfx/vec.h"
#include "scenefacade.h"
#include "gfx/tm.h"
#include <math.h>
#include "script/parseresult.h"
#include "scene/optid.h"
#include "effoptionstate.h"
#include "object.h"
#include "scene.h"
#include "light.h"
#include "gfx/gm_graph.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

class Object;
class Scene;
class Light;
class gm_graph;

/* Rotation matrix of a quaternion. */
static inline mat4 mat4_quat(float x, float y, float z, float w)
{
	float x2 = x + x;
	float w2 = w + w;
	float z2 = z + z;
	float y2 = y + y;
	return mat4(1.0f - y2 * y - z2 * z, x2 * y + w2 * z, z2 * x - w2 * y, 0.0f, x2 * y - w2 * z,
	    1.0f - x2 * x - z2 * z, y2 * z + w2 * x, 0.0f, z2 * x + w2 * y, y2 * z - w2 * x, 1.0f - x2 * x - y2 * y,
	    0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

/* Rotation of `angle` around `axis`. */
static inline mat4 mat4_rotate(const vec4 &axis, float angle)
{
	vec4 q = normalize(axis) * gmGetTableSin((int)(angle * 32768.0f / 3.14159265f));
	return mat4_quat(q.x, q.y, q.z, gmGetTableCos((int)(angle * 32768.0f / 3.14159265f)));
}

/* Light group last activated (ActivateEnvironmentState). */
static int s_light_group = 0;
static int s_light_active = 0;

/* Default layer modes (lay_params constructor). */
static int lay_mode_default[4] = { 0x10001, 0x20102, 0xffff, 0xffff };

/* RGBA color mask for each value of the 4 pixel_write bits. */
static unsigned int color_mask_table[16] = {
	0x00000000,
	0x000000ff,
	0x0000ff00,
	0x0000ffff,
	0x00ff0000,
	0x00ff00ff,
	0x00ffff00,
	0x00ffffff,
	0xff000000,
	0xff0000ff,
	0xff00ff00,
	0xff00ffff,
	0xffff0000,
	0xffff00ff,
	0xffffff00,
	0xffffffff,
};

/* Conversions of the texture states for gm_graph. */
static int tex_func_table[2] = { GM_TEXFUNC_MODULATE, GM_TEXFUNC_DECAL };
static int tex_func_mode[2] = { GM_TEXFUNC_RGBA, GM_TEXFUNC_RGB };
/* Used for both s and t. */
static int tex_wrap_table[2] = { GM_WRAP_REPEAT, GM_WRAP_CLAMP };
static inline int tex_wrap(int clamp)
{
	return tex_wrap_table[clamp & 1];
}
static int tex_filter_mag[2] = { GM_FILTER_NEAREST, GM_FILTER_LINEAR };
static int tex_filter_min[8] = {
	GM_FILTER_NEAREST_MIPMAP_NEAREST,
	GM_FILTER_LINEAR_MIPMAP_NEAREST,
	GM_FILTER_NEAREST_MIPMAP_LINEAR,
	GM_FILTER_LINEAR_MIPMAP_LINEAR,
	GM_FILTER_NEAREST,
	GM_FILTER_LINEAR,
	GM_FILTER_NEAREST,
	GM_FILTER_LINEAR,
};

/* Conversions for AlphaFunc/DepthFunc/StencilFunc. */
static int pixel_func_table[8] = {
	GM_ALWAYS,
	GM_NEVER,
	GM_EQUAL,
	GM_NOTEQUAL,
	GM_LESS,
	GM_GREATER,
	GM_LEQUAL,
	GM_GEQUAL,
};

/* Conversions for the ColorGen/TexGen of the layers. */
static int color_gen_table[4] = { 2, 0, 1, 2 };
static int tex_gen_table[4] = { 0, 1, 2, 3 };
static int tex_gen_param[8] = {
	GM_TEXGEN_TEXCOORD,
	GM_TEXGEN_TEXCOORD,
	GM_TEXGEN_TEXCOORD,
	GM_TEXGEN_TEXCOORD,
	GM_TEXGEN_POSITION,
	GM_TEXGEN_NORMAL,
	GM_TEXGEN_REFLECTION,
	GM_TEXGEN_TEXCOORD,
};

EffOptionState::EffOptionState()
{
	owner = 0;
	object = 0;
	flags = 0;
	never_mask = 0;
	always_mask = 0;
	env = 0;
	mtl = 0;
	pix = 0;
	lay = 0;
}

EffOptionState::~EffOptionState()
{
	Term();
}

void EffOptionState::Init(Object *obj)
{
	Term();
	owner = obj;
	object = obj->scene;
	if (object != obj)
		return;

	/* The root scene: set every default. */
	flags |= EFF_ROOT;
	SetLightingMode(0);
	SetLightGroup(0);

	vec4 v;
	float *p = &v.x;
	p[0] = 0.0f;
	p[1] = 0.0f;
	p[2] = 0.0f;
	p[3] = 1.0f;
	SetAmbient(v);
	SetFogMode(0);
	p[0] = 0.0f;
	p[1] = 0.0f;
	p[2] = 0.0f;
	p[3] = 1.0f;
	SetFogColor(v);
	SetFogRange(100.0f, 10000.0f);
	p[0] = 1.0f;
	p[1] = 1.0f;
	p[2] = 1.0f;
	p[3] = 1.0f;
	SetDiffuse(v);
	p[0] = 0.0f;
	p[1] = 0.0f;
	p[2] = 0.0f;
	p[3] = 1.0f;
	SetSpecular(v, 1.0f);
	p[0] = 0.0f;
	p[1] = 0.0f;
	p[2] = 0.0f;
	p[3] = 1.0f;
	SetEmission(v);
	p[0] = 1.0f;
	p[1] = 1.0f;
	p[2] = 1.0f;
	p[3] = 1.0f;
	SetTexFactor(v, 1.0f, 0.0f);
	SetTextureMode(0);
	SetTextureClamp(0x11);
	SetTextureFilter(0x111);
	p[0] = 0.0f;
	p[1] = 0.0f;
	p[2] = 1.0f;
	p[3] = 1.0f;
	SetTextureCrop(v);
	SetDepthOffset(0.0f);
	SetPixelTest(0);
	SetPixelWrite(15);
	for (int i = 0; i < 4; i++) {
		SetLayerMode(i, lay_mode_default[i]);
		SetTexture(0, i + EFF_SLOT_LAYER0, -1);
	}
}

void EffOptionState::Term()
{
	if (!owner)
		return;
	if (lay) {
		int *tex = lay->texture;
		int *obj = lay->object;
		for (int i = 0; i < 4; i++) {
			if (tex[i])
				tmRelease(tex[i]);
			if (obj[i] >= 0)
				owner->DeleteReference(obj[i]);
		}
	}
	owner = 0;
	delete env;
	delete mtl;
	delete pix;
	delete lay;
	env = 0;
	mtl = 0;
	pix = 0;
	lay = 0;
}

int EffOptionState::HasAutoCrop() const
{
	if (!(flags & EFF_LAYER0))
		return 0;
	return (lay->mode[0] & 0x0f000000) != 0;
}

int EffOptionState::Activate(int mask)
{
	int f = (int)flags;

	if (f < 0) {
		f = mask;
		mask = 0;
	} else {
		int rest = mask & ~f & ~never_mask;
		if (rest)
			object->option_state.Activate(rest);
		mask = f | always_mask;
	}
	if (f == 0)
		return mask;

	gm_graph *g = SceneGetGraph();
	if (f & EFF_ENVIRONMENT_MASK)
		ActivateEnvironmentState(*g, f);
	if (f & EFF_TEXTURE_MASK)
		ActivateTextureState(*g, f);
	if (f & EFF_MATERIAL_MASK)
		ActivateMaterialState(*g, f);
	if (f & EFF_PIXEL_MASK)
		ActivatePixelState(*g, f);
	if (f & EFF_LAYER_MASK)
		mask |= ActivateLayerState(*g, f);
	return mask;
}

void EffOptionState::ActivateEnvironmentState(gm_graph &g, int mask)
{
	if (mask & EFF_LIGHTING) {
		int lit = lighting_mode;
		int bits;
		if (lit & 1)
			bits = g.fFlagsE | GM_ENABLE_LIGHTING;
		else
			bits = g.fFlagsE & ~GM_ENABLE_LIGHTING;
		g.SetEnableBits(bits);
		int force = mask & EFF_LIGHT_RELOAD;
		if ((lit & 1) || force) {
			int group = light_group;
			if (group != s_light_group) {
				force = 1;
				s_light_group = group;
				s_light_active = 0;
			}
			int active = 0;
			Scene *scene = (Scene *)object;
			int no = 0;
			while (no < 4) {
				Light *light = scene->GetLight(no, group);
				if (!light) {
					if (!force)
						break;
					scene->ActivateLightState(g, no);
				} else if (!light->visible) {
					if (force)
						g.DisableLight(no, true);
				} else if (!light->type) {
					if (force)
						light->ActivateLightState(g, no);
				} else {
					int bit = 1 << no;
					switch (lit & 0xf0) {
					case 0x00:
						light->ActivatePseudoPointLightState(g, no, owner->world_mat.m3);
						active |= bit;
						break;
					case 0x10:
						if (force || (bit & s_light_active))
							light->ActivateLightState(g, no);
						break;
					case 0x20:
						g.DisableLight(no, true);
						active |= bit;
						break;
					}
				}
				no++;
			}
			s_light_active = active;
		}
	}
	if (mask & EFF_FOG_MODE) {
		int bits;
		if (fog_mode & 1)
			bits = g.fFlagsE | GM_ENABLE_FOG;
		else
			bits = g.fFlagsE & ~GM_ENABLE_FOG;
		g.SetEnableBits(bits);
	}
	env_params *e = env;
	if (e) {
		if (mask & EFF_AMBIENT)
			g.Ambient(e->ambient);
		if (mask & EFF_FOG_COLOR)
			g.FogColor(e->fog_color);
		if (mask & EFF_FOG_RANGE)
			g.FogRange(e->fog_near, e->fog_far);
	}
}

void EffOptionState::ActivatePixelState(gm_graph &g, int mask)
{
	pix_params *p = pix;
	if (!p)
		return;
	if (mask & EFF_DEPTH_OFFSET) {
		float ofs = p->depth_offset;
		if (ofs != 0.0f) {
			if (owner->zsort_value < 0.0f) {
				float s = 1.0f / owner->zsort_value;
				const mat4 *pm = g.GetMatrix(GM_MATRIX_PROJECTION);
				ofs = ofs * (pm->m3.z * s * s * 0.5f);
			}
		}
		if (ofs < -0.01f)
			ofs = -0.01f;
		if (ofs > 0.01f)
			ofs = 0.01f;
		g.DepthRange(ofs + 0.01f, ofs + 0.99f);
	}
	if (mask & EFF_PIXEL_TEST) {
		int test = p->pixel_test;
		int bits = 0;
		int afunc = test & 7;
		if (afunc) {
			bits = GM_ENABLE_ALPHA_TEST;
			g.AlphaFunc(pixel_func_table[afunc], p->reference_alpha);
		}
		int zfunc = (test >> 4) & 7;
		if (zfunc) {
			bits |= GM_ENABLE_DEPTH_TEST;
			g.DepthFunc(pixel_func_table[zfunc]);
		}
		if ((test >> 8) & 7) {
			bits |= GM_ENABLE_STENCIL_TEST;
			g.StencilFunc(pixel_func_table[zfunc], p->reference_stencil);
		}
		g.SetEnableBits(g.fFlagsE | bits);
		g.SetEnableBits(
		    g.fFlagsE & (bits ^ ~(GM_ENABLE_ALPHA_TEST | GM_ENABLE_STENCIL_TEST | GM_ENABLE_DEPTH_TEST)));
	}
	if (mask & EFF_PIXEL_WRITE) {
		unsigned int write = p->pixel_write;
		g.ColorMask(color_mask_table[write & 0xf]);
		g.DepthMask((write >> 4) & 1);
		g.StencilOp(1, 1, (write & 0x100) ? 3 : 1);
	}
}

void EffOptionState::ActivateTextureState(gm_graph &g, int mask)
{
	if (mask & EFF_TEXTURE_MODE) {
		int mode = texture_mode;
		g.TexFunc(tex_func_table[mode & 1], tex_func_mode[(mode >> 4) & 1]);
	}
	if (mask & EFF_TEXTURE_CLAMP) {
		int clamp = texture_clamp;
		g.TexWrap(tex_wrap(clamp), tex_wrap(clamp >> 4));
	}
	if (mask & EFF_TEXTURE_FILTER) {
		int filter = texture_filter;
		g.TexFilter(tex_filter_mag[filter & 1], tex_filter_min[((filter >> 4) & 1) | ((filter >> 7) & 6)]);
	}
	if (mask & EFF_TEXTURE_CROP)
		g.TexCrop(texture_crop);
}

void EffOptionState::ActivateMaterialState(gm_graph &g, int mask)
{
	mtl_params *m = mtl;
	if (!m)
		return;
	if (mask & EFF_DIFFUSE)
		g.Diffuse(m->diffuse);
	if (mask & EFF_SPECULAR)
		g.Specular(m->specular, m->specular_power);
	if (mask & EFF_EMISSION)
		g.Emission(m->emission);
	if (mask & EFF_TEX_FACTOR)
		g.TexFactor(m->tex_factor, m->tex_factor_a, m->tex_factor_b);
}

/* Rows 0-2 normalized, row 3 = (0, 0, 0, 1). */
static inline mat4 lay_orthonormalize(const mat4 &m)
{
	return mat4(normalize(m.m0), normalize(m.m1), normalize(m.m2), s_unitRow3);
}

int EffOptionState::ActivateLayerState(gm_graph &g, int mask)
{
	lay_params *lp = lay;
	if (!lp)
		return 0;

	int result = 0;

	for (int no = 0; no < 4; no++) {
		if (!((EFF_LAYER0 << no) & mask))
			continue;
		g.SetLayer(no);
		int mode = lp->mode[no];
		if (no != 0) {
			int blend = mode & 0xffff;
			if (blend == 0xffff) {
				g.BlendFunc(GM_BLEND_OFF);
				continue;
			}
			g.BlendFunc(blend);
		}
		int lit = (mode & 0x80000) != 0;
		g.ColorGen(color_gen_table[(mode >> 16) & 3], lit);
		if (no != 0) {
			int texture = lp->texture[no];
			g.SetTexture(texture);
			if (!texture)
				continue;
			int wrap = (mode & 0x8000000) != 0;
			g.TexWrap(wrap, wrap);
		} else if (!(flags & EFF_TEXTURE_CLAMP)) {
			int wrap = (mode & 0x8000000) != 0;
			g.TexWrap(wrap, wrap);
			result = EFF_TEXTURE_CLAMP;
		}
		int gen = tex_gen_table[(mode >> 24) & 3];
		g.TexGen(gen, tex_gen_param[(mode >> 20) & 7]);
		if (gen == 0) {
			if (no != 0) {
				static vec4 crop_unit(0.0f, 0.0f, 1.0f, 1.0f);
				g.TexCrop(crop_unit);
			}
		} else {
			static vec4 crop_half(0.5f, 0.5f, -1.0f, -1.0f);
			static vec4 crop_center(0.5f, 0.5f, -0.5f, -0.5f);
			g.TexCrop((mode & 0x800000) ? crop_center : crop_half);
			if (no == 0)
				result |= EFF_TEXTURE_CROP;
		}
		if (lp->object[no] < 0)
			continue;
		int genmat = mode & 0x30000000;
		if (genmat == 0x20000000) {
			g.LoadIdentity(no + 4);
			continue;
		}
		Object *obj = owner->GetReference(lp->object[no]);
		if (!obj)
			continue;
		const mat4 &inv = obj->world_inv;
		Object *own = owner;
		mat4 m = inv * own->world_mat;
		if (genmat == 0x10000000) {
			static vec4 s_axis_z(0.0f, 0.0f, 1.0f, 1.0f);
			vec4 dir = normalize(apply(inv, own->world_mat.m3, 1.0f));
			vec4 c = cross(dir, s_axis_z);
			float half = acosf(dot(dir, s_axis_z)) * 0.5f;
			m = mat4_rotate(c, half) * m;
		}
		if (mode & 0x800000) {
			m = lay_orthonormalize(m);
			m.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
		}
		switch (gen) {
		case 2: {
			static mat4 proj_sphere(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 2.0f,
			    0.0f, 0.0f, 0.0f, 0.0f);
			m = proj_sphere * m;
			break;
		}
		case 3: {
			static mat4 proj_reflex(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
			    0.0f, 0.0f, -1.0f, 1.0f);
			m = proj_reflex * m;
			break;
		}
		}
		g.LoadMatrix(no + 4, m);
	}
	g.SetLayer(0);
	g.MatrixMode(GM_MATRIX_MODEL);
	return result;
}

int EffOptionState::SetOption(int option, const int &value, unsigned int size)
{
	switch (option) {
	case OPT_LIT:
		SetLightingMode(value);
		break;
	case OPT_LITG:
		SetLightGroup(value);
		break;
	case OPT_FOG:
		SetFogMode(value);
		break;
	case OPT_TEXM:
		SetTextureMode(value);
		break;
	case OPT_TEXC:
		SetTextureClamp(value);
		break;
	case OPT_TEXF:
		SetTextureFilter(value);
		break;
	case OPT_PIXT:
		SetPixelTest(value);
		break;
	case OPT_PIXW:
		SetPixelWrite(value);
		break;
	case OPT_REFA:
		SetReferenceAlpha(value);
		break;
	case OPT_REFS:
		SetReferenceStencil(value);
		break;
	case OPT_LAY:
	case OPT_LAY0:
		SetLayerMode(0, value);
		break;
	case OPT_LAY1:
		SetLayerMode(1, value);
		break;
	case OPT_LAY2:
		SetLayerMode(2, value);
		break;
	case OPT_LAY3:
		SetLayerMode(3, value);
		break;
	default:
		return 0;
	}
	return 1;
}

int EffOptionState::SetOption(int option, const void *data, unsigned int size)
{
	unsigned int sz = size ? size : 256;
	int n = sz >> 2;
	const float *f = (const float *)data;

	if (option == OPT_FOGR) {
		float near = 100.0f;
		if (n > 0)
			near = f[0];
		float far = 10000.0f;
		if (n >= 2)
			far = f[1];
		SetFogRange(near, far);
		return 1;
	}

	vec4 v;
	if (n < 4) {
		float *p = &v.x;
		p[0] = 0.0f;
		p[1] = 0.0f;
		p[2] = 0.0f;
		p[3] = 1.0f;
	}
	ParseVec4(v, f, n);
	float p = 1.0f;
	if (n >= 5)
		p = f[4];
	float q = 0.0f;
	if (n >= 6)
		q = f[5];

	switch (option) {
	case OPT_AMB:
		SetAmbient(v);
		return 1;
	case OPT_FOGC:
		SetFogColor(v);
		return 1;
	case OPT_DIFF:
		SetDiffuse(v);
		return 1;
	case OPT_SPEC:
		SetSpecular(v, p);
		return 1;
	case OPT_EMIS:
		SetEmission(v);
		return 1;
	case OPT_FACT:
		SetTexFactor(v, p, q);
		return 1;
	case OPT_CROP:
		if (n < 3)
			v.z = 1.0f;
		SetTextureCrop(v);
		return 1;
	}
	return 0;
}

int EffOptionState::SetOption(int option, const float &value, unsigned int size)
{
	switch (option) {
	case OPT_DOFS:
		SetDepthOffset(value);
		return 1;
	case OPT_AMB:
	case OPT_SPEC:
	case OPT_FOGC:
	case OPT_DIFF:
	case OPT_FOGR:
	case OPT_EMIS:
	case OPT_FACT:
	case OPT_CROP:
		return SetOption(option, &value, size * 4);
	}
	return 0;
}

int EffOptionState::GetOption(int option, int &value, unsigned int size) const
{
	int no;

	switch (option) {
	case OPT_LIT:
		value = GetLightingMode();
		break;
	case OPT_LITG:
		value = GetLightGroup();
		break;
	case OPT_FOG:
		value = GetFogMode();
		break;
	case OPT_TEXM:
		value = GetTextureMode();
		break;
	case OPT_TEXC:
		value = GetTextureClamp();
		break;
	case OPT_TEXF:
		value = GetTextureFilter();
		break;
	case OPT_PIXT:
		value = GetPixelTest();
		break;
	case OPT_PIXW:
		value = GetPixelWrite();
		break;
	case OPT_REFA:
		value = GetReferenceAlpha();
		break;
	case OPT_REFS:
		value = GetReferenceStencil();
		break;
	case OPT_LAY:
	case OPT_LAY0:
		no = 0;
		goto lay;
	case OPT_LAY1:
		no = 1;
		goto lay;
	case OPT_LAY2:
		no = 2;
		goto lay;
	case OPT_LAY3:
		no = 3;
lay:
		value = GetLayerMode(no);
		break;
	default:
		return 0;
	}
	return 1;
}

int EffOptionState::GetOption(int option, void *data, unsigned int size) const
{
	int n = size >> 2;
	float *f = (float *)data;

	if (option == OPT_FOGR) {
		float near, far;
		GetFogRange(near, far);
		if (n > 0)
			f[0] = near;
		if (n >= 2)
			f[1] = far;
		return 1;
	}

	vec4 v;
	float p = 0.0f;
	float q = 0.0f;

	switch (option) {
	case OPT_AMB:
		GetAmbient(v);
		break;
	case OPT_FOGC:
		GetFogColor(v);
		break;
	case OPT_DIFF:
		GetDiffuse(v);
		break;
	case OPT_SPEC:
		GetSpecular(v, p);
		break;
	case OPT_EMIS:
		GetEmission(v);
		break;
	case OPT_FACT:
		GetTexFactor(v, p, q);
		break;
	case OPT_CROP:
		GetTextureCrop(v);
		break;
	default:
		return 0;
	}
	ResultVec4(v, f, n);
	if (n >= 5)
		f[4] = p;
	if (n >= 6)
		f[5] = q;
	return 1;
}

int EffOptionState::GetOption(int option, float &value, unsigned int size) const
{
	switch (option) {
	case OPT_DOFS:
		value = GetDepthOffset();
		break;
	case OPT_AMB:
	case OPT_SPEC:
	case OPT_FOGC:
	case OPT_DIFF:
	case OPT_FOGR:
	case OPT_EMIS:
	case OPT_FACT:
	case OPT_CROP:
		return GetOption(option, &value, size * 4);
	default:
		return 0;
	}
	return 1;
}

void EffOptionState::SetLightingMode(int mode)
{
	if (!(flags & EFF_LIGHTING))
		light_group = 0;
	lighting_mode = mode;
	flags |= EFF_LIGHTING;
}

void EffOptionState::SetLightGroup(int group)
{
	if (!(flags & EFF_LIGHTING))
		lighting_mode = 0;
	light_group = group;
	flags |= EFF_LIGHTING;
}

void EffOptionState::SetFogMode(int mode)
{
	fog_mode = mode;
	flags |= EFF_FOG_MODE;
}

void EffOptionState::SetTextureMode(int mode)
{
	texture_mode = mode;
	flags |= EFF_TEXTURE_MODE;
}

void EffOptionState::SetTextureClamp(int clamp)
{
	texture_clamp = clamp;
	flags |= EFF_TEXTURE_CLAMP;
}

void EffOptionState::SetTextureFilter(int filter)
{
	texture_filter = filter;
	flags |= EFF_TEXTURE_FILTER;
}

void EffOptionState::SetTextureCrop(const vec4 &crop)
{
	texture_crop = crop;
	flags |= EFF_TEXTURE_CROP;
}

void EffOptionState::SetAmbient(const vec4 &color)
{
	if (!env)
		env = new env_params;
	env->ambient = color;
	flags |= EFF_AMBIENT;
}

void EffOptionState::SetFogColor(const vec4 &color)
{
	if (!env)
		env = new env_params;
	env->fog_color = color;
	flags |= EFF_FOG_COLOR;
}

void EffOptionState::SetFogRange(float near, float far)
{
	if (!env)
		env = new env_params;
	env->fog_near = near;
	env->fog_far = far;
	flags |= EFF_FOG_RANGE;
}

void EffOptionState::SetDiffuse(const vec4 &color)
{
	if (!mtl)
		mtl = new mtl_params;
	mtl->diffuse = color;
	flags |= EFF_DIFFUSE;
}

void EffOptionState::SetSpecular(const vec4 &color, float power)
{
	if (!mtl)
		mtl = new mtl_params;
	mtl->specular = color;
	mtl->specular_power = power;
	flags |= EFF_SPECULAR;
}

void EffOptionState::SetEmission(const vec4 &color)
{
	if (!mtl)
		mtl = new mtl_params;
	mtl->emission = color;
	flags |= EFF_EMISSION;
}

void EffOptionState::SetTexFactor(const vec4 &color, float a, float b)
{
	if (!mtl)
		mtl = new mtl_params;
	mtl->tex_factor = color;
	mtl->tex_factor_a = a;
	mtl->tex_factor_b = b;
	flags |= EFF_TEX_FACTOR;
}

void EffOptionState::SetDepthOffset(float offset)
{
	if (!pix)
		pix = new pix_params;
	pix->depth_offset = offset;
	flags |= EFF_DEPTH_OFFSET;
}

void EffOptionState::SetPixelTest(int test)
{
	if (!pix)
		pix = new pix_params;
	if (!(flags & EFF_PIXEL_TEST)) {
		pix->reference_alpha = 0;
		pix->reference_stencil = 0;
	}
	pix->pixel_test = test;
	flags |= EFF_PIXEL_TEST;
}

void EffOptionState::SetPixelWrite(int write)
{
	if (!pix)
		pix = new pix_params;
	pix->pixel_write = write;
	flags |= EFF_PIXEL_WRITE;
}

void EffOptionState::SetReferenceAlpha(int alpha)
{
	if (!pix)
		pix = new pix_params;
	if (!(flags & EFF_PIXEL_TEST)) {
		pix->pixel_test = 0;
		pix->reference_stencil = 0;
	}
	pix->reference_alpha = alpha;
	flags |= EFF_PIXEL_TEST;
}

void EffOptionState::SetReferenceStencil(int stencil)
{
	if (!pix)
		pix = new pix_params;
	if (!(flags & EFF_PIXEL_TEST)) {
		pix->pixel_test = 0;
		pix->reference_alpha = 0;
	}
	pix->reference_stencil = stencil;
	flags |= EFF_PIXEL_TEST;
}

void EffOptionState::SetLayerMode(int no, int mode)
{
	if ((unsigned int)no < 4) {
		if (!lay)
			lay = new lay_params;
		lay->mode[no] = mode;
		flags |= EFF_LAYER0 << no;
	}
}

void EffOptionState::SetTexture(int texture, int option, int mode)
{
	int no = option ? option - EFF_SLOT_LAYER0 : 0;

	if ((unsigned int)no < 4) {
		if (!lay)
			lay = new lay_params;
		int old = lay->texture[no];
		if (old != texture) {
			if (old)
				tmRelease(old);
			if (texture)
				tmDuplicate(texture);
			lay->texture[no] = texture;
			flags |= EFF_LAYER0 << no;
		}
	}
}

void EffOptionState::SetObject(Object *obj, int option, int mode)
{
	int no = option ? option - EFF_SLOT_LAYER0 : 0;

	if ((unsigned int)no < 4) {
		if (!lay)
			lay = new lay_params;
		int *ref = &lay->object[no];
		owner->DeleteReference(*ref);
		*ref = owner->CreateReference(obj);
	}
}

int EffOptionState::GetLightingMode() const
{
	if (flags & EFF_LIGHTING)
		return lighting_mode;
	return object->option_state.lighting_mode;
}

int EffOptionState::GetLightGroup() const
{
	if (flags & EFF_LIGHTING)
		return light_group;
	return object->option_state.light_group;
}

int EffOptionState::GetFogMode() const
{
	if (flags & EFF_FOG_MODE)
		return fog_mode;
	return object->option_state.fog_mode;
}

int EffOptionState::GetTextureMode() const
{
	if (flags & EFF_TEXTURE_MODE)
		return texture_mode;
	return object->option_state.texture_mode;
}

int EffOptionState::GetTextureClamp() const
{
	if (flags & EFF_TEXTURE_CLAMP)
		return texture_clamp;
	return object->option_state.texture_clamp;
}

int EffOptionState::GetTextureFilter() const
{
	if (flags & EFF_TEXTURE_FILTER)
		return texture_filter;
	return object->option_state.texture_filter;
}

void EffOptionState::GetTextureCrop(vec4 &crop) const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_TEXTURE_CROP))
		s = &object->option_state;
	crop = s->texture_crop;
}

void EffOptionState::GetAmbient(vec4 &color) const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_AMBIENT))
		s = &object->option_state;
	color = s->env->ambient;
}

void EffOptionState::GetFogColor(vec4 &color) const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_FOG_COLOR))
		s = &object->option_state;
	color = s->env->fog_color;
}

void EffOptionState::GetFogRange(float &near, float &far) const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_FOG_RANGE))
		s = &object->option_state;
	near = s->env->fog_near;
	far = s->env->fog_far;
}

void EffOptionState::GetDiffuse(vec4 &color) const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_DIFFUSE))
		s = &object->option_state;
	color = s->mtl->diffuse;
}

void EffOptionState::GetSpecular(vec4 &color, float &power) const
{
	const EffOptionState *s;
	if (flags & EFF_SPECULAR)
		s = this;
	else
		s = &object->option_state;
	color = s->mtl->specular;
	power = s->mtl->specular_power;
}

void EffOptionState::GetEmission(vec4 &color) const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_EMISSION))
		s = &object->option_state;
	color = s->mtl->emission;
}

void EffOptionState::GetTexFactor(vec4 &color, float &a, float &b) const
{
	const EffOptionState *s;
	if (flags & EFF_TEX_FACTOR)
		s = this;
	else
		s = &object->option_state;
	color = s->mtl->tex_factor;
	a = s->mtl->tex_factor_a;
	b = s->mtl->tex_factor_b;
}

float EffOptionState::GetDepthOffset() const
{
	const EffOptionState *s;
	if (flags & EFF_DEPTH_OFFSET)
		s = this;
	else
		s = &object->option_state;
	return s->pix->depth_offset;
}

int EffOptionState::GetPixelTest() const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_PIXEL_TEST))
		s = &object->option_state;
	return s->pix->pixel_test;
}

int EffOptionState::GetPixelWrite() const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_PIXEL_WRITE))
		s = &object->option_state;
	return s->pix->pixel_write;
}

int EffOptionState::GetReferenceAlpha() const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_PIXEL_TEST))
		s = &object->option_state;
	return s->pix->reference_alpha;
}

int EffOptionState::GetReferenceStencil() const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_PIXEL_TEST))
		s = &object->option_state;
	return s->pix->reference_stencil;
}

int EffOptionState::GetLayerMode(int no) const
{
	if ((unsigned int)no >= 4)
		return 0xffff;
	const EffOptionState *s;
	if ((EFF_LAYER0 << no) & flags)
		s = this;
	else
		s = &object->option_state;
	return s->lay->mode[no];
}

int EffOptionState::GetLayerTexture(int no) const
{
	if ((unsigned int)no >= 4)
		return 0;
	const EffOptionState *s;
	if ((EFF_LAYER0 << no) & flags)
		s = this;
	else
		s = &object->option_state;
	return s->lay->texture[no];
}

const vec4 *EffOptionState::GetAmbient() const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_AMBIENT))
		s = &object->option_state;
	return &s->env->ambient;
}

const vec4 *EffOptionState::GetFogColor() const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_FOG_COLOR))
		s = &object->option_state;
	return &s->env->fog_color;
}

const vec4 *EffOptionState::GetDiffuse() const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_DIFFUSE))
		s = &object->option_state;
	return &s->mtl->diffuse;
}

const vec4 *EffOptionState::GetSpecular() const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_SPECULAR))
		s = &object->option_state;
	return &s->mtl->specular;
}

const vec4 *EffOptionState::GetEmission() const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_EMISSION))
		s = &object->option_state;
	return &s->mtl->emission;
}

const vec4 *EffOptionState::GetTexFactor() const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_TEX_FACTOR))
		s = &object->option_state;
	return &s->mtl->tex_factor;
}

const vec4 *EffOptionState::GetTextureCrop() const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_TEXTURE_CROP))
		s = &object->option_state;
	return &s->texture_crop;
}

unsigned int EffOptionState::GetColorMask() const
{
	const EffOptionState *s = this;
	if (!(flags & EFF_PIXEL_WRITE))
		s = &object->option_state;
	return color_mask_table[s->pix->pixel_write & 0xf];
}

EffOptionState::lay_params::lay_params()
{
	for (int i = 0; i < 4; i++) {
		mode[i] = lay_mode_default[i];
		texture[i] = 0;
		object[i] = -1;
	}
}
