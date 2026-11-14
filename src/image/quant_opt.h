/*
 * Color quantization of a 24-bit RGB image onto an optimized palette
 * (quant_opt.cpp).
 */

#ifndef QUANT_OPT_H
#define QUANT_OPT_H

void _get_nearest_color_in_palette(unsigned char *out, const unsigned char (*pal)[256], const int *rgb, int *diff);
void _quant_opt(unsigned char *dst, const unsigned char *src, unsigned ncolor, unsigned width, unsigned height,
    bool padded);

#endif
