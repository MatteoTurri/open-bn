/*
 * bmp_file: a BMP file of the img_* hierarchy, holding a single picture.
 */

#ifndef BMP_FILE_H
#define BMP_FILE_H

#pragma interface
#include "img.h"

class bmp_file : public img_file {
public:
	int load(int *cur, int *end);
};

#endif
