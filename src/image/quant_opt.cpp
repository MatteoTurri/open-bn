/*
 * Color quantization of a 24-bit RGB image onto an optimized palette.
 * A palette is three 256-byte planes (R, G, B).
 */

#include "quant_opt.h"
#include <string.h>
#include "quant_palette.h"

/* Finds the palette entry nearest to rgb (squared Euclidean distance) and
 * stores it in out; if diff is not NULL it receives the per-channel error. */
void _get_nearest_color_in_palette(unsigned char *out, const unsigned char (*pal)[256], const int *rgb, int *diff)
{
	unsigned char best = 0;
	int mind = 0x7fffffff;
	int c0 = rgb[0];
	int c1 = rgb[1];
	int c2 = rgb[2];
	unsigned i = 0;

	do {
		int d = (c0 - pal[0][i]) * (c0 - pal[0][i]);

		d += (c1 - pal[1][i]) * (c1 - pal[1][i]);
		d += (c2 - pal[2][i]) * (c2 - pal[2][i]);

		if (d < mind) {
			mind = d;
			best = (unsigned char)i;
			if (mind == 0)
				break;
		}
		i++;
	} while (i < 256);
	out[0] = pal[0][best];
	out[1] = pal[1][best];
	out[2] = pal[2][best];
	if (diff != 0) {
		diff[0] = out[0] - rgb[0];
		diff[1] = out[1] - rgb[1];
		diff[2] = out[2] - rgb[2];
	}
}

/* Quantizes src onto a palette of ncolor entries computed by
 * _make_optimized_palette (quant_palette.cpp). padded selects rows aligned to
 * 4 bytes instead of exactly width * 3 bytes. */
void _quant_opt(unsigned char *dst, const unsigned char *src, unsigned ncolor, unsigned width, unsigned height,
    bool padded)
{
	unsigned char pal[3][256];
	unsigned total;
	unsigned x, y;
	unsigned h3;

	if (padded)
		total = height * ((width * 3 + 3) & ~3);
	else {
		h3 = height * 3;
		total = h3 * width;
	}
	memset(dst, 0, total);
	memset(pal, 0, 768);
	_make_optimized_palette(pal, src, ncolor, width, height, padded);

	for (y = 0; y < height; y++) {
		unsigned char *o;
		const unsigned char *s;
		unsigned stride = (width * 3 + 3) & ~3;

		if (padded)
			o = dst + y * stride;
		else
			o = dst + y * 3 * width;
		if (padded)
			s = src + y * stride;
		else
			s = src + y * 3 * width;

		for (x = 0; x < width; x++) {
			int rgb[3];
			unsigned k;

			for (k = 0; k < 3; k++)
				rgb[k] = s[k];
			_get_nearest_color_in_palette(o, pal, rgb, 0);
			s += 3;
			o += 3;
		}
	}
}
