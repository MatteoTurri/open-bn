/*
 * Inline TIM2 image and CLUT loaders.
 * Include after the callers that keep an out-of-line call.
 */

#ifndef TM2_INLINE_H
#define TM2_INLINE_H

#pragma interface

/* Copies one image from the cursor `cur` (end is unused); returns the cursor past
   the copied data. */
inline int tm2_image::load(int *cur, int *end, int mode, int width, int height)
{
	unload();
	m_mode = mode;
	m_width = width;
	m_height = height;
	int size = get_pitch() * height;
	int aligned = (size + 3) / 4 * 4;
	char *buf = new char[aligned];
	m_buffer = buf;
	memcpy(buf, cur, size);
	return (int)cur + size;
}

/* Copies a one-line palette from the cursor `cur`. Bit 0x80 of type means the
   entries are already in linear order, otherwise sort() reorders them.
   Returns the cursor past the data, or 0. */
inline int tm2_clut::load(int *cur, int *end, int type, int count)
{
	int mode = type & 0x7f;
	int height = 1;

	unload();
	m_mode = mode;
	m_width = count;
	m_height = height;
	int size = get_pitch() * height;
	int aligned = (size + 3) / 4 * 4;
	char *buf = new char[aligned];
	m_buffer = buf;
	memcpy(buf, cur, size);
	int cursor = (int)cur + size;
	if (!cursor)
		return 0;
	if (!(type & 0x80))
		sort();
	return cursor;
}

#endif
