/*
 * Defocus methods: the blur passes and the options.
 */

#include <iostream.h>
#include "gfx/vec.h"
#include "defocus.h"
#include <vector>
#include "gfx/gm_graph.h"
#include "gfx/gm_vertex.h"
#include "scenefacade.h"
#include "scene/optid.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

/* Blur weights (rgb, alpha) of the first 5 passes; the later passes repeat the last. */
static float s_defocusKernel[5][4] = {
	{ 0.535f, 0.535f, 0.535f, 0.4f },
	{ 0.525f, 0.525f, 0.525f, 0.55f },
	{ 0.515f, 0.515f, 0.515f, 0.7f },
	{ 0.505f, 0.505f, 0.505f, 0.85f },
	{ 0.5025f, 0.5025f, 0.5025f, 1.0f },
};

/* Full-screen quad of the blur passes. */
static gmVertexPT s_quad[2] = {
	{ -1.0f, 1.0f, 0.0f, 0.0f, 0.0f },
	{ 1.0f, -1.0f, 0.0f, 1.0f, 1.0f },
};

static void DrawKernelPass(gm_graph &, int, int, int);

Defocus::Defocus(Object *parent) : Object(parent), render_target_id(0), level(1)
{
	GetOptionState()->always_mask |= EFF_TEXTURE_MODE;
	render_target_id = SceneGetGraph()->CreateColorBuffer(320, 224, 0);
	for (int i = 0; i < 16; i++) {
		int j = i < 5 ? i : 4;
		kernel[i].Set(s_defocusKernel[j][0], s_defocusKernel[j][1], s_defocusKernel[j][2],
		    s_defocusKernel[j][3]);
	}
}

/*
 * Blurs the screen: `level` times, copies the screen into the color buffer
 * and draws it back tinted by the world color times kernel[i].
 */
void Defocus::DrawThis(gm_graph &g)
{
	if (!visible)
		return;

	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE_FIT);

	vec2 off;
	g.GetPixelOffset(off.x, off.y);
	g.SetPixelOffset(0.0f, 0.0f);

	g.SetEnableBits(g.fFlagsE | GM_ENABLE_COLOR_2X);
	g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGB);

	vec4 origColor = GetWorldColor();
	g.VertexGen(0);

	for (int i = 0; i < level; i++) {
		g.BlendFunc(GM_BLEND_COPY);
		g.TexFunc(GM_TEXFUNC_DECAL, GM_TEXFUNC_RGBA);
		DrawKernelPass(g, render_target_id, -1, level);

		g.BlendFunc(GM_BLEND_ALPHA);
		g.TexFunc(GM_TEXFUNC_MODULATE, GM_TEXFUNC_RGB);
		vec4 scaled = origColor * kernel[i];
		g.Color(scaled);

		DrawKernelPass(g, -1, render_target_id, level);
	}

	g.VertexGen(2);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_COLOR_2X);
	g.SetPixelOffset(off.x, off.y);
}

Defocus::~Defocus()
{
	SceneGetGraph()->DeleteColorBuffer(render_target_id);
}

/* One blur pass: draws texture `tex` into color buffer `colorBuf` (-1 = screen); `level` is unused. */
static void DrawKernelPass(gm_graph &g, int colorBuf, int tex, int level)
{
	(void)level;
	g.SetColorBuffer(colorBuf);
	g.SetTexture(tex);
	g.Viewport();
	g.SetArrays(s_quad, GM_VERTEX_PT, 2, 1, 0, 0);
	g.DrawArrays(GM_PRIM_SPRITE, 0, 2, 1, 0);
}

int Defocus::SetOption(int option, const int &value, unsigned int size)
{
	if (option == OPT_LEVL) {
		int v = value;
		level = v;
		if (v >= 17)
			level = 16;
		return 1;
	}
	return Object::SetOption(option, value, 1);
}

int Defocus::SetOption(int option, const float &value, unsigned int size)
{
	return Object::SetOption(option, value, 1);
}

int Defocus::SetOption(int option, const String &value)
{
	return Object::SetOption(option, value);
}

int Defocus::GetOption(int option, int &value, unsigned int size) const
{
	return Object::GetOption(option, value, 1);
}

int Defocus::GetOption(int option, float &value, unsigned int size) const
{
	return Object::GetOption(option, value, 1);
}

int Defocus::GetOption(int option, String &value) const
{
	return Object::GetOption(option, value);
}
