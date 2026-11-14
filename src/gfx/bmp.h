/*
 * BMP classes of the img_* texture hierarchy (img.h). bmp_clut derives from
 * bmp_image and inherits its five-argument load().
 */

#pragma once

#include "img.h"

#include "bmp_file.h"

class bmp_pict : public img_pict {
public:
	int load(int *cur, int *end);
};

class bmp_image : public img_image {
public:
	int load(int *cur, int *end, int mode, int width, int height);
};

class bmp_clut : public bmp_image {
public:
	int load(int *cur, int *end, int mode, int count);
};
