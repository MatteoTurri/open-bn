/*
 * tagRECT: a rectangle in pixels, the band of an image that a decoder
 * reports to its progress callback.
 */

#ifndef RECT_H
#define RECT_H

struct tagRECT {
	/* 0x00 */ int left;
	/* 0x04 */ int top;
	/* 0x08 */ int right;
	/* 0x0C */ int bottom;
}; /* size = 0x10 */

#endif
