/*
 * TIM2 loaders: file, picture, image and CLUT.
 */

#pragma implementation "tm2_file.h"
#pragma implementation "tm2_inline.h"
#include <iostream.h>
#include "vec.h"
#include "tm2.h"
#include "tim2.h"
#include <string.h>

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

/* Returns the cursor past the last picture, or 0. The picture count is read
   as a signed value. */
int tm2_file::load(int *cur, int *end)
{
	unload();
	TIM2_FILE_HEAD *hdr = (TIM2_FILE_HEAD *)cur;
	cur = (int *)(hdr + 1);
	for (int i = 0; i < (short)hdr->pictures; i++) {
		tm2_pict *p = new tm2_pict();
		cur = (int *)p->load(cur, end);
		if (!cur) {
			delete p;
			unload();
			return 0;
		}
		m_picts.push_back(p);
	}
	return (int)cur;
}

/* Images and CLUT are copied; returns the cursor past the picture. Like
   tm2_file::load, it reads the header counts and sizes as signed values (the
   img_* loaders work with int). */
int tm2_pict::load(int *cur, int *end)
{
	unload();
	TIM2_PIC_HEAD *hdr = (TIM2_PIC_HEAD *)cur;
	int w = (short)hdr->width;
	int h = (short)hdr->height;
	cur = (int *)(hdr + 1);
	TIM2_MIPMAP_HEAD *mip = ((signed char)hdr->mipMapTextures < 2) ? 0 : (TIM2_MIPMAP_HEAD *)cur;
	cur = (int *)((char *)hdr + (short)hdr->headerSize);
	for (int i = 0; i < (signed char)hdr->mipMapTextures; i++) {
		tm2_image *p = new tm2_image();
		if (!p->load(cur, end, (signed char)hdr->imageType, w, h)) {
			delete p;
			unload();
			return 0;
		}
		m_images.push_back(p);
		if (mip)
			cur = (int *)((char *)cur + mip->imageSize[i]);
		w = w / 2;
		h = h / 2;
	}
	cur = (int *)((char *)hdr + (short)hdr->headerSize + hdr->imageSize);
	if ((hdr->clutType & 7) != 0) {
		tm2_clut *c = new tm2_clut();
		if (!c->load(cur, end, (signed char)hdr->clutType, (short)hdr->clutColors)) {
			delete c;
			unload();
			return 0;
		}
		m_cluts.push_back(c);
	}
	return (int)((char *)hdr + hdr->totalSize);
}

/* Restores the linear order of a swizzled 256-entry palette: in every group
   of 4n words, words [n, 2n) and [2n, 3n) are swapped (n = 4, 6, 8 for
   16, 24, 32-bit entries). */
void tm2_clut::sort()
{
	if (m_buffer == 0)
		return;
	if (m_width != 256)
		return;
	/* NOTE: bug: no breaks: types 1 and 2 also run the later passes, past the end of their buffer */
	switch (m_mode & 7) {
	case 1: {
		int *p = (int *)m_buffer;
		for (int j = 0; j < 8; j++) {
			for (int i = 0; i < 4; i++) {
				int t = p[i + 8];
				int u = p[i + 4];
				p[i + 4] = t;
				p[i + 8] = u;
			}
			p += 16;
		}
	}
	case 2: {
		int *p = (int *)m_buffer;
		for (int j = 0; j < 8; j++) {
			for (int i = 0; i < 6; i++) {
				int t = p[i + 12];
				int u = p[i + 6];
				p[i + 6] = t;
				p[i + 12] = u;
			}
			p += 24;
		}
	}
	case 3: {
		int *p = (int *)m_buffer;
		for (int j = 0; j < 8; j++) {
			for (int i = 0; i < 8; i++) {
				int t = p[i + 16];
				int u = p[i + 8];
				p[i + 8] = t;
				p[i + 16] = u;
			}
			p += 32;
		}
	}
	}
}

/* The inline definitions follow their callers. */
#include "tm2_inline.h"
