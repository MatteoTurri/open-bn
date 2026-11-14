/*
 * Base classes of the image loaders: a file holds pictures, a picture holds
 * images and palettes (both stored as img_image). TIM2 and BMP derive from these.
 */

#pragma once

#include <vector>
#include <string>

class img_pict;
class img_image;

class img_file {
public:
	/* 0x00 */ std::vector<img_pict *> m_picts;
	/* vptr at 0x0C */

	img_file();
	virtual ~img_file();
	virtual bool is_loaded();
	virtual void unload();
	virtual int load(const std::string &path);
	virtual int load(int *cur, int *end);

	int get_pict_count();
	img_pict *get_pict(int i);
};

class img_pict {
public:
	/* 0x00 */ std::vector<img_image *> m_images;
	/* 0x0C */ std::vector<img_image *> m_cluts;
	/* vptr at 0x18 */

	img_pict();
	virtual ~img_pict();
	virtual bool is_loaded();
	virtual void unload();
	virtual int load(int *cur, int *end);

	int get_image_count();
	img_image *get_image(int i);
	int get_clut_count();
	img_image *get_clut(int i);
};

class img_image {
public:
	/* 0x00 */ int m_mode;      /* TIM2 pixel type in the low 3 bits */
	/* 0x04 */ int m_width;
	/* 0x08 */ int m_height;
	/* 0x0C */ void *m_buffer;  /* pixel data, new[]'d, freed by unload() */
	/* vptr at 0x10 */

	img_image();
	virtual ~img_image();
	virtual bool is_loaded();
	virtual void unload();
	virtual int load(int *cur, int *end, int mode, int width, int height);

	int get_mode();
	int get_width();
	int get_height();
	void *get_buffer();
	int get_depth();
	int get_pitch();
	int get_gs_format();
};
