/*
 * TIM2 file loader.
 */

#ifndef TM2_FILE_H
#define TM2_FILE_H

#pragma interface
#include "img.h"

class tm2_file : public img_file {
public:
	int load(int *cur, int *end);
};

#endif
