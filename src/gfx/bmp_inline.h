/*
 * Inline loaders of bmp_file and bmp_clut and the unaligned little-endian
 * readers of the BMP header. Included at the end of bmp.cpp, after the callers.
 */

#ifndef BMP_INLINE_H
#define BMP_INLINE_H

#pragma interface

/* A BMP file holds a single picture. */
inline int bmp_file::load(int *cur, int *end)
{
	unload();
	bmp_pict *p = new bmp_pict();
	cur = (int *)p->load(cur, end);
	if (!cur) {
		delete p;
		return 0;
	}
	m_picts.push_back(p);
	return (int)cur;
}

/* Converts `d` palette entries (B, G, R, reserved) to RGBA words with
   alpha 0x80 (opaque on the GS). `b` is unused. */
inline int bmp_clut::load(int *cur, int *end, int mode, int count)
{
	unload();
	m_mode = mode;
	m_width = count;
	m_height = 1;
	m_buffer = new char[count * 4];
	unsigned char *s = (unsigned char *)cur;
	for (int i = 0; i < count; i++) {
		((unsigned *)m_buffer)[i] = 0x80000000 | (s[0] << 16) | (s[1] << 8) | s[2];
		s += 4;
	}
	return (int)s;
}

/* Unaligned little-endian reads; both return the advanced pointer. */
static inline unsigned char *readU16LE(unsigned char *p, unsigned short *out)
{
	if (((unsigned int)p & 1) != 0)
		*out = p[0] | (p[1] << 8);
	else
		*out = *(unsigned short *)p;
	return p + 2;
}

static inline unsigned char *readU32LE(unsigned char *p, unsigned int *out)
{
	if (((unsigned int)p & 3) != 0)
		*out = ((p[3] << 24 | p[2] << 16) | p[1] << 8) | p[0];
	else
		*out = *(unsigned int *)p;
	return p + 4;
}

#endif
