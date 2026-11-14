/*
 * SScope0: oscilloscope-like scene object. A strip of 64 vertex pairs follows
 * a buffer of 64 signed samples and is drawn over a one-frame-old copy (trail).
 */

#include <iostream.h>
#include "gfx/vec.h"
#include "sscope0.h"
#include "scene/optid.h"
#include "gfx/gm_graph.h"
/* Texture manager (C interface). */
#include "gfx/tm.h"

/* Unit matrix and unit w row: the file-scope objects built at startup. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);
SScope0::ClassInitializer __SScope0_ClassInitializer__;

SScope0::ClassInitializer::ClassInitializer()
{}
SScope0::SScope0(Object *parent) : Object(parent)
{
	samples_enabled = 1;
	samples = 0;
	tex_handle = 0;
	InitModel();
}

SScope0::~SScope0()
{
	if (tex_handle)
		tmRelease(tex_handle);
}

/* Like Prim::SetTexture, but returns at once when the texture does not
   change and always sets SSCOPE0_TEX_CHANGED. */
void SScope0::SetTexture(int texture, int option, int mode)
{
	if (option == 0 || option == EFF_SLOT_LAYER0) {
		int old = tex_handle;
		if (texture == old)
			return;
		flags &= ~SSCOPE0_TEX_NONPOW2;
		int changed;
		if (old == texture)  /* redundant as written */
			goto merge;
		if (old) {
			tmRelease(old);
			tex_handle = 0;
		}
		if (texture == 0)
			goto merge;
		tmDuplicate(texture);
		tex_handle = texture;
		changed = 1;
		goto tail;
merge:
		changed = 0;
tail:
		if (changed) {
			if (tmIsNonPow2(texture))
				flags |= SSCOPE0_TEX_NONPOW2;
		}
		flags |= SSCOPE0_TEX_CHANGED;
		return;
	}
	option_state.SetTexture(texture, option, mode);
}

int SScope0::SetOption(int option, const int &value, unsigned int size)
{
	if (option == OPT_SBF)
		goto set_2658;
	if (option == OPT_ACTV) {
		samples_enabled = (value != 0);
		return 1;
	}
	goto fallback;
set_2658:
	samples = value;
	return 1;
fallback:
	return Object::SetOption(option, value, 1);
}

int SScope0::SetOption(int option, const void *data, unsigned int size)
{
	if (option == OPT_SBF) {
		samples = (int)data;
		return 1;
	}
	return Object::SetOption(option, data, 0);
}

int SScope0::GetOption(int option, int &value, unsigned int size) const
{
	return Object::GetOption(option, value, 1);
}

void SScope0::SetColor(float r, float g, float b, float a)
{
	Object::SetColor(r, g, b, a);
}

/* Draws one buffer as a strip of 128 vertices. */
static inline void DrawStrip(gm_graph &g, const gmVertexPNCT *buf)
{
	g.SetArrays(buf, GM_VERTEX_PNCT, 128, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 128, 1, 0);
}

/* Draws the trail (buffer_a), then the current strip (buffer_b). */
void SScope0::DrawThis(gm_graph &g)
{
	if (!visible)
		return;

	if (flags & SSCOPE0_TEX_NONPOW2)
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE_FIT);
	else
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT);

	g.BlendFunc(blend_mode);

	int texbits;
	if (tex_handle)
		texbits = g.fFlagsE | GM_ENABLE_TEXTURE;
	else
		texbits = g.fFlagsE & ~GM_ENABLE_TEXTURE;
	g.SetEnableBits(texbits);

	g.SetTexture(tex_handle);

	MakeGraphics();

	DrawStrip(g, buffer_a);
	DrawStrip(g, buffer_b);
}

/* Copies the positions of buffer_b into buffer_a, so that buffer_a lags one
   frame behind; then, if enabled (samples_enabled) and a sample buffer is set
   (samples), moves every vertex pair of buffer_b by -sample/256. */
void SScope0::MakeGraphics()
{
	for (int i = 0; i < 128; i++) {
		float *dst = &buffer_a[i].x;
		const float *src = &buffer_b[i].x;
		for (int j = 2; j != -1; j--)
			*dst++ = *src++;
	}

	if (samples_enabled == 0)
		return;
	if (samples == 0)
		return;

	const signed char *p = (const signed char *)samples;
	for (int i = 0; i < 64; i++) {
		float d = (float)*p++ / 256.0f;
		int k = i * 2 + 1;
		buffer_b[k - 1].y = -0.5f - d;
		buffer_b[k].y = 0.5f - d;
	}
}

/* Builds buffer_b as a strip of 64 opaque white vertex pairs (x from -0.5 to
   0.5, y = -0.5/+0.5) and copies it into buffer_a with half the alpha. */
void SScope0::InitModel()
{
	float half = 0.5f;

	for (int i = 0; i < 64; i++) {
		float u = (float)i / 63.0f;
		int k = i * 2 + 1;

		buffer_b[k - 1].x = u - 0.5f;
		buffer_b[k - 1].y = -half;
		buffer_b[k - 1].z = 0.0f;
		buffer_b[k - 1].nx = 0.0f;
		buffer_b[k - 1].ny = 0.0f;
		buffer_b[k - 1].nz = 1.0f;
		buffer_b[k - 1].color.c[0] = 255;
		buffer_b[k - 1].color.c[1] = 255;
		buffer_b[k - 1].color.c[2] = 255;
		buffer_b[k - 1].color.c[3] = 255;
		buffer_b[k - 1].u = u;
		buffer_b[k - 1].v = 0.0f;

		buffer_b[k].x = u - 0.5f;
		buffer_b[k].y = half;
		buffer_b[k].z = 0.0f;
		buffer_b[k].nx = 0.0f;
		buffer_b[k].ny = 0.0f;
		buffer_b[k].nz = 1.0f;
		buffer_b[k].color.c[0] = 255;
		buffer_b[k].color.c[1] = 255;
		buffer_b[k].color.c[2] = 255;
		buffer_b[k].color.c[3] = 255;
		buffer_b[k].u = u;
		buffer_b[k].v = 1.0f;
	}

	for (int j = 0; j < 128; j++) {
		buffer_a[j] = buffer_b[j];
		buffer_a[j].color.c[3] = buffer_b[j].color.c[3] >> 1;
	}
}
