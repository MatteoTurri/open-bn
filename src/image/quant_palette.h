/*
 * Optimized palette of a 24-bit RGB image by median cut (quant_palette.cpp).
 * A palette is three 256-byte planes (R, G, B).
 */

#ifndef QUANT_PALETTE_H
#define QUANT_PALETTE_H

int _make_optimized_palette(unsigned char (*pal)[256], const unsigned char *buf, unsigned ncol, unsigned width,
    unsigned height, const bool padded);

#endif
