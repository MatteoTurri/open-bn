/*
 * TIM2 (PS2 texture format) loaders derived from the img_* classes.
 * tm2_clut is a tm2_image: it inherits its load() override and hides it with a
 * 4-argument load().
 */

#pragma once

#include "img.h"

#include "tm2_file.h"

class tm2_pict : public img_pict {
public:
	int load(int *cur, int *end);
};

class tm2_image : public img_image {
public:
	int load(int *cur, int *end, int mode, int width, int height);
};

class tm2_clut : public tm2_image {
public:
	int load(int *cur, int *end, int type, int count);
	void sort();
};
