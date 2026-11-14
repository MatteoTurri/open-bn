/*
 * gm_texture: GS textures, CLUTs and render buffers of the gm_* graphics
 * layer, as handles of the texture manager (tm.h).
 */

#include "gm_texture.h"

#include <iostream.h>
#include "vec.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

TmHandle gm_texture::g_texture_manager = 0;

/* Creates the texture (and its CLUT, if any), uploads pixels and palette and
   returns the handle; `bank` goes in the top byte of the format. 0 on failure. */
int gm_texture::new_texture(void *pixels, int psm, int width, int height, void *clut, int clutPsm, int bank)
{
	if (!g_texture_manager)
		return 0;
	unsigned combined = (unsigned)psm | ((unsigned)bank << 24);
	int handle = tmCreateTexture(g_texture_manager, width, height, combined);
	if (handle <= 0)
		return 0;
	if (clut) {
		int r = tmCreateClut(g_texture_manager, psm, clutPsm);
		if (r == -3) {
			tmRelease(handle);
			return 0;
		}
		tmSetClut(handle, r);
		tmRelease(r);
	}
	((gm_texture *)handle)->load_image(pixels);
	((gm_texture *)handle)->load_clut(clut);
	return handle;
}

int gm_texture::new_color_buffer(int width, int height, int bank)
{
	if (!g_texture_manager)
		return 0;
	int even = (bank ^ 1) & 1;
	unsigned base = even ? 0xa0000000u : 0x80000000u;
	int handle = tmCreateTexture(g_texture_manager, width, height, base);
	int result = 0;
	if (handle > 0)
		result = handle;
	return result;
}

int gm_texture::new_depth_buffer(int width, int height, int bank)
{
	if (!g_texture_manager)
		return 0;
	int even = (bank ^ 1) & 1;
	unsigned base = even ? 0xa0000000u : 0x80000000u;
	int handle = tmCreateTexture(g_texture_manager, width, height, base);
	int result = 0;
	if (handle > 0)
		result = handle;
	return result;
}

void gm_texture::operator delete(void *p)
{
	if ((unsigned)((char *)p + 1) >= 2)
		tmRelease((TmHandle)p);
}

gm_texture *gm_texture::duplicate()
{
	if (this)
		tmDuplicate((TmHandle)this);
	return this;
}

int gm_texture::load_image(void *pixels)
{
	if (!this)
		goto fail;
	if (!pixels)
		goto fail;
	{
		int fmt = tmGetPixelFormat((TmHandle)this);
		int flag = tmGetMipmapLevel((TmHandle)this) << 24;
		int width = tmGetWidth((TmHandle)this);
		int height = tmGetHeight((TmHandle)this);
		tmUpdateTexture((TmHandle)this, width, height, fmt | flag, pixels);
	}
	return 1;
fail:
	return 0;
}

int gm_texture::load_clut(void *clut)
{
	if (!this)
		return 0;
	if (!clut)
		return 0;
	TmHandle clutHandle = tmGetClut((TmHandle)this);
	if (!clutHandle)
		return 0;
	tmUpdateClut(clutHandle, 1, clut);
	tmRelease(clutHandle);
	return 1;
}

void gm_texture::send()
{
	if (this)
		tmChainTexture((TmHandle)this);
}

int gm_texture::get_clut_format()
{
	if (!this)
		return 0;
	TmHandle clutHandle = tmGetClut((TmHandle)this);
	if (!clutHandle)
		return 0;
	int fmt = tmGetPixelFormat(clutHandle);
	tmRelease(clutHandle);
	return fmt;
}
