/*
 * Median-cut palette generation: a 6-bit-per-channel histogram is split into
 * boxes along the channel with the widest range, and each box is averaged.
 */

/* Histogram bucket, indexed by the RGB value reduced to 6 bits per channel:
 * (r >> 2) * 4096 + (g >> 2) * 64 + (b >> 2). */

#include <stdlib.h>
#include <string.h>
#include "quant_palette.h"

struct shist {
	/* 0x00 */ unsigned char r;
	/* 0x01 */ unsigned char g;
	/* 0x02 */ unsigned char b;
	/* 0x03 */ unsigned char pad;
	/* 0x04 */ int count;
};

/* Median-cut box: first color in the compacted array, number of colors and
 * total pixel count. */
struct sbox {
	/* 0x00 */ unsigned start;
	/* 0x04 */ unsigned count;
	/* 0x08 */ unsigned total;
};

/* qsort comparators: by one color channel, or by box total, descending. */
static int cmp_r(const void *a, const void *b)
{
	return ((const unsigned char *)a)[0] - ((const unsigned char *)b)[0];
}

static int cmp_g(const void *a, const void *b)
{
	return ((const unsigned char *)a)[1] - ((const unsigned char *)b)[1];
}

static int cmp_b(const void *a, const void *b)
{
	return ((const unsigned char *)a)[2] - ((const unsigned char *)b)[2];
}

static int cmp_box(const void *a, const void *b)
{
	return ((const struct sbox *)b)->total - ((const struct sbox *)a)->total;
}

/* Counts the pixels of each histogram bucket and returns the number of
 * buckets touched for the first time. */
int _calchist(unsigned char *buf, unsigned width, unsigned height, struct shist *hist, bool padded)
{
	unsigned x, y, idx;
	int n = 0;
	int bits = 6;
	int shift = 8 - bits;
	int mul_r = 1 << (bits * 2);
	int mul_g = 1 << bits;

	for (y = 0; y < height; y++) {
		unsigned stride = padded ? ((width * 3 + 3) & ~3) : (width * 3);
		unsigned char *p = buf + y * stride;

		for (x = 0; x < width; x++) {
			idx = (p[0] >> shift) * mul_r + (p[1] >> shift) * mul_g + (p[2] >> shift);
			if (hist[idx].count == 0) {
				hist[idx].r = p[0];
				hist[idx].g = p[1];
				hist[idx].b = p[2];
				n++;
			}
			hist[idx].count++;
			p += 3;
		}
	}
	return n;
}

/* Builds a palette of ncol colors with median cut: histogram, compaction into
 * an array of colors, repeated splitting of the box along the channel with
 * the widest range (weights 1/6/3), then the count-weighted average of each
 * box. Returns 1, or 0 if out of memory. */
int _make_optimized_palette(unsigned char (*pal)[256], const unsigned char *buf, unsigned ncol, unsigned width,
    unsigned height, const bool padded)
{
	struct shist *hist;
	struct sbox *boxes;
	struct shist *ent;
	unsigned lo[3], hi[3];
	unsigned nmax, nbox, i = 0, j, k, h;
	unsigned start, count, total, sum, half;

	hist = (struct shist *)malloc(262144 * 8);
	if (hist == 0)
		return 0;
	memset(hist, 0, 262144 * 8);

	boxes = (struct sbox *)malloc(256 * 12);
	/* NOTE: bug: the buffers already allocated are leaked when an allocation fails. */
	if (boxes == 0)
		return 0;
	memset(boxes, 0, 256 * 12);

	boxes[0].start = 0;
	boxes[0].count = _calchist((unsigned char *)buf, width, height, hist, padded);
	h = 0;
	boxes[0].total = width * height;

	ent = (struct shist *)malloc(boxes[0].count * 8);
	if (ent == 0)
		return 0;

	nmax = ncol - 1;
	for (k = 0; k < boxes[0].count; k++) {
		while (hist[h].count == 0)
			h++;
		ent[k] = hist[h];
		h++;
	}
	free(hist);

	for (nbox = 1; nbox < nmax;) {
		for (i = 0; i < nbox; i++)
			if (boxes[i].count > 1)
				break;
		if (i == nbox)
			break;

		start = boxes[i].start;
		count = boxes[i].count;
		total = boxes[i].total;

		lo[0] = hi[0] = ent[start].r;
		lo[1] = hi[1] = ent[start].g;
		lo[2] = hi[2] = ent[start].b;
		for (k = 1; k < count; k++) {
			unsigned c;

			c = ent[start + k].r;
			if (c < lo[0])
				lo[0] = c;
			if (hi[0] < c)
				hi[0] = c;
			c = ent[start + k].g;
			if (c < lo[1])
				lo[1] = c;
			if (hi[1] < c)
				hi[1] = c;
			c = ent[start + k].b;
			if (c < lo[2])
				lo[2] = c;
			if (hi[2] < c)
				hi[2] = c;
		}

		hi[0] = hi[0] - lo[0];
		hi[1] = (hi[1] - lo[1]) * 6;
		hi[2] = (hi[2] - lo[2]) * 3;

		if (hi[0] >= hi[1] && hi[0] >= hi[2])
			qsort(&ent[start], count, 8, cmp_r);
		else if (hi[1] >= hi[2])
			qsort(&ent[start], count, 8, cmp_g);
		else
			qsort(&ent[start], count, 8, cmp_b);

		sum = ent[start].count;
		half = total >> 1;
		for (k = 1; k < count - 1; k++) {
			if (sum >= half)
				break;
			sum += ent[start + k].count;
		}

		boxes[i].count = k;
		boxes[i].total = sum;
		boxes[nbox].start = start + k;
		boxes[nbox].count = count - k;
		boxes[nbox].total = total - sum;
		nbox++;
		qsort(boxes, nbox, 12, cmp_box);
	}

	for (i = 0; i < nbox; i++) {
		lo[0] = lo[1] = lo[2] = 0;
		start = boxes[i].start;
		count = boxes[i].count;
		for (k = 0; k < count; k++) {
			lo[0] += ent[start + k].r * ent[start + k].count;
			lo[1] += ent[start + k].g * ent[start + k].count;
			lo[2] += ent[start + k].b * ent[start + k].count;
		}
		pal[0][i] = lo[0] / boxes[i].total;
		pal[1][i] = lo[1] / boxes[i].total;
		pal[2][i] = lo[2] / boxes[i].total;
	}

	free(ent);
	free(boxes);
	return 1;
}
