/*
 * gm_texture is never instantiated: a gm_texture* is an opaque handle of the
 * texture manager (tm.h). The factories are static.
 */

#pragma once

#include "tm.h"

class gm_texture {
public:
	static int new_texture(void *pixels, int a, int width, int height, void *clut, int b, int bank);
	static int new_color_buffer(int width, int height, int bank);
	static int new_depth_buffer(int width, int height, int bank);

	void operator delete(void *p);
	gm_texture *duplicate();
	void send();
	int load_image(void *pixels);
	int load_clut(void *clut);
	int get_clut_format();

	static TmHandle g_texture_manager;
};
