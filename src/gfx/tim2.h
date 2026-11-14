/*
 * TIM2, the PS2 texture file format (and BTM2, the same container with an
 * MPEG-2 intra bitstream as the image): file header, picture header, mipmap
 * header. Read by the gmo loader (tm2.cpp), the texture loader (bt2ipu.cpp)
 * and the IME (textimsub.c).
 *
 * A file is a TIM2_FILE_HEAD (padded to 128 bytes when formatId is 1) and
 * `pictures` pictures. Each picture is a TIM2_PIC_HEAD, then a
 * TIM2_MIPMAP_HEAD when there are 2 or more mipmap levels, the images at
 * `headerSize` and the CLUT after the images.
 */

#ifndef TIM2_H
#define TIM2_H

#ifdef __cplusplus
extern "C" {
#endif

#define TIM2_MAGIC 0x324d4954    /* 'TIM2' little-endian */
#define BTM2_MAGIC 0x324d5442    /* 'BTM2' little-endian */

/* Pixel types of imageType and of the low bits of clutType. */
#define TIM2_RGB16  1
#define TIM2_RGB24  2
#define TIM2_RGB32  3
#define TIM2_IDTEX4 4
#define TIM2_IDTEX8 5

typedef struct TIM2_FILE_HEAD {
	/* 0x00 */ unsigned int fileId;         /* TIM2_MAGIC or BTM2_MAGIC */
	/* 0x04 */ unsigned char formatVersion;
	/* 0x05 */ unsigned char formatId;       /* 0 = 16-byte header, 1 = 128-byte header */
	/* 0x06 */ unsigned short pictures;
	/* 0x08 */ unsigned char pad_08[8];
} TIM2_FILE_HEAD; /* size = 0x10 */

typedef struct TIM2_PIC_HEAD {
	/* 0x00 */ unsigned int totalSize;      /* whole picture, header included */
	/* 0x04 */ unsigned int clutSize;
	/* 0x08 */ unsigned int imageSize;      /* all the mipmap levels; for BTM2 the bitstream */
	/* 0x0C */ unsigned short headerSize;     /* offset of the images from this header */
	/* 0x0E */ unsigned short clutColors;
	/* 0x10 */ unsigned char pictFormat;     /* 1 for a BTM2 bitstream */
	/* 0x11 */ unsigned char mipMapTextures; /* number of levels */
	/* 0x12 */ unsigned char clutType;       /* pixel type in the low bits, storage mode in the top two */
	/* 0x13 */ unsigned char imageType;      /* TIM2_RGB16 ... TIM2_IDTEX8 */
	/* 0x14 */ unsigned short width;
	/* 0x16 */ unsigned short height;
	/* 0x18 */ unsigned int gsTex0[2];
	/* 0x20 */ unsigned int gsTex1[2];
	/* 0x28 */ unsigned int gsTexaFbaPabe;
	/* 0x2C */ unsigned int gsTexClut;
} TIM2_PIC_HEAD; /* size = 0x30 */

/* After the picture header when mipMapTextures >= 2: GS MIPTBP1/MIPTBP2 and
   the size of every level. */
typedef struct TIM2_MIPMAP_HEAD {
	/* 0x00 */ unsigned int gsMiptbp1[2];
	/* 0x08 */ unsigned int gsMiptbp2[2];
	/* 0x10 */ unsigned int imageSize[1];     /* mipMapTextures entries */
} TIM2_MIPMAP_HEAD;

/* Entry of the loaders' tables from pixel type to GS format, indexed by
   type - 1: GS psm and `bytes` bytes for every `pixels` pixels (16 bit:
   PSMCT16 2/1, 24: PSMCT24 3/1, 32: PSMCT32 4/1, 4-bit indexed: PSMT4 1/2,
   8-bit indexed: PSMT8 1/1). The tables end with psm = -1. */
typedef struct TIM2_FORMAT {
	/* 0x00 */ int psm;
	/* 0x04 */ int bytes;
	/* 0x08 */ int pixels;
} TIM2_FORMAT; /* size = 0x0C */

#ifdef __cplusplus
}
#endif

#endif
