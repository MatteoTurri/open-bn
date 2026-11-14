/*
 * Base image loaders of the gmo layer: img_file, img_pict, img_image.
 */

#include <iostream.h>
#include "vec.h"
#include "img.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

/* Indexed by TIM2 pixel type (1 = 16 bit, 2 = 24, 3 = 32, 4 = 4-bit indexed,
   5 = 8-bit indexed): bits per pixel and GS PSM (PSMCT16 2, PSMCT24 1,
   PSMCT32 0, PSMT4 0x14, PSMT8 0x13). */
static signed char s_bitsPerPixel[8] = { 0, 16, 24, 32, 4, 8, 0, 0 };
static signed char s_gsFormat[8] = { 0x00, 0x02, 0x01, 0x00, 0x14, 0x13, 0x00, 0x00 };

/* --- accessors --- */

int img_file::get_pict_count()
{
	return m_picts.size();
}

img_pict *img_file::get_pict(int i)
{
	if (i >= 0 && i < (int)m_picts.size())
		goto ok;
	return 0;
ok:
	return m_picts[i];
}

int img_pict::get_image_count()
{
	return m_images.size();
}

img_image *img_pict::get_image(int i)
{
	if (i >= 0 && i < (int)m_images.size())
		goto ok;
	return 0;
ok:
	return m_images[i];
}

int img_pict::get_clut_count()
{
	return m_cluts.size();
}

img_image *img_pict::get_clut(int i)
{
	if (i >= 0 && i < (int)m_cluts.size())
		goto ok;
	return 0;
ok:
	return m_cluts[i];
}

int img_image::get_mode()
{
	return m_mode;
}

int img_image::get_width()
{
	return m_width;
}

int img_image::get_height()
{
	return m_height;
}

void *img_image::get_buffer()
{
	return m_buffer;
}

/* --- img_file --- */

img_file::img_file()
{}

img_file::~img_file()
{
	unload();
}

bool img_file::is_loaded()
{
	return m_picts.size() != 0;
}

void img_file::unload()
{
	for (int i = 0; i < (int)m_picts.size(); i++) {
		if (m_picts[i])
			delete m_picts[i];
	}
	m_picts.erase(m_picts.begin(), m_picts.end());
}

int img_file::load(const std::string &path)
{
	return 0;
}

int img_file::load(int *cur, int *end)
{
	unload();
	return 0;
}

/* --- img_pict --- */

img_pict::img_pict()
{}

img_pict::~img_pict()
{
	unload();
}

bool img_pict::is_loaded()
{
	return m_images.size() != 0;
}

void img_pict::unload()
{
	for (int i = 0; i < (int)m_images.size(); i++) {
		if (m_images[i])
			delete m_images[i];
	}
	for (int i = 0; i < (int)m_cluts.size(); i++) {
		if (m_cluts[i])
			delete m_cluts[i];
	}
	m_images.erase(m_images.begin(), m_images.end());
	m_cluts.erase(m_cluts.begin(), m_cluts.end());
}

int img_pict::load(int *cur, int *end)
{
	unload();
	return 0;
}

/* --- img_image --- */

img_image::img_image() : m_mode(0), m_width(0), m_height(0), m_buffer(0)
{}

img_image::~img_image()
{
	unload();
}

bool img_image::is_loaded()
{
	return m_buffer != 0;
}

void img_image::unload()
{
	if (m_buffer) {
		delete[] (char *)m_buffer;
	}
	m_buffer = 0;
}

int img_image::get_depth()
{
	return s_bitsPerPixel[m_mode & 7];
}

int img_image::get_pitch()
{
	int bits = s_bitsPerPixel[m_mode & 7] * m_width;
	return (bits + 7) / 8;
}

int img_image::get_gs_format()
{
	return s_gsFormat[m_mode & 7];
}

int img_image::load(int *cur, int *end, int mode, int width, int height)
{
	unload();
	return 0;
}
