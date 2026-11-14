/* Base: giflib-4.1.0.tar.gz (Toshio Kuratomi, 1999-02-09, archive.org item
 * tucows_9274_Giflib), the GENUINE release with the LZW encoder (not
 * libungif, which drops the LZW encoder because of the Unisys patent).
 * Identification: the binary has the version strings "GIFLIB IBMPC Version
 * 4.0" and "GIF_LIBRARY IBMPC Version 4.0" (two different PROGRAM_NAMEs: one
 * from this header, one defined again locally in gif_err.c/gif_hash.c/
 * quantize.c) with Sony's real __DATE__/__TIME__ (Aug/Sep 2002).
 * GIF_LIB_VERSION is a fixed "4.0" string that upstream kept for years, not
 * the version of the tarball. The symbol set agrees (the DGif... and EGif...
 * functions, BitSize, MakeMapObject, ..., plus UnionColorMap,
 * ApplyTranslation, MakeExtension, AddExtensionBlock, FreeExtension,
 * MakeSavedImage, PrintGifError: all in 4.1.0 with the same signatures).
 * Flags: -O0 (frame-pointer style, every value spilled; NOT -O2), asflags
 * -g2 (gas mips_optimize=1: no swapping of delay slots and branches).
 *
 * Differences from the tarball, each read off the binary's code (the
 * library Sony used is a fork with a simplified encoder and some fixes):
 *  1. Prefix[LZ_MAX_CODE], not +1 (below): sizeof(GifFilePrivateType) is
 *     24900, not 24904, as the malloc(24900) in EGifOpen/DGifOpen/
 *     DGifOpenFileHandle/EGifOpenFileHandle shows.
 *  2. The encoder has no hash table: EGifOpenFileHandle does not call
 *     _InitHashTable, EGifCloseFile does not free it, EGifSetupCompress
 *     starts RunningCode at 0, and EGifCompressLine emits every pixel as
 *     a literal code with a ClearCode when RunningCode reaches
 *     (1 << BitsPerPixel) - 2 (see egif_lib.c).
 *  3. DGifCloseFile has an extra "&& !GifFile->SavedImages" guard on the
 *     free of the per-image ColorMap (otherwise a double free after
 *     DGifSlurp) - see dgif_lib.c.
 *  4. DGifGetImageDesc: the two memcpy of the ColorMap duplicated into
 *     SavedImages copy the CONTENTS (no &) and size the Colors array by
 *     the real ColorCount (not a fixed sizeof(GifColorType)) - see
 *     dgif_lib.c: the public 4.1.0 copies only the bytes of the pointer.
 * The unused declarations in EGifCompressLine (NewCode, NewKey, HashTable)
 * stay: at -O0 they occupy stack slots, and the frame of the original
 * (80 B) and the slots of Private and HashTable depend on them.
 * gif_hash.c is not built (the binary has none of its functions); this
 * header still includes gif_hash.h for the GifHashTableType of the
 * private encoder state. The static encoder functions of egif_lib.c sit
 * in the binary at EGifPutWord 0x6A1770, EGifSetupCompress 0x6A1888,
 * EGifCompressLine 0x6A1AA0, EGifCompressOutput 0x6A1D30 and
 * EGifBufferedOutput 0x6A1F48.
 *
 */

#ifndef GIF_LIB_PRIVATE_H
#define GIF_LIB_PRIVATE_H

#include "gif_lib.h"
#include "gif_hash.h"

#define PROGRAM_NAME	"GIFLIB"

#define LZ_MAX_CODE	4095		/* Biggest code possible in 12 bits. */
#define LZ_BITS		12

#define FLUSH_OUTPUT		4096    /* Impossible code, to signal flush. */
#define FIRST_CODE		4097    /* Impossible code, to signal first. */
#define NO_SUCH_CODE		4098    /* Impossible code, to signal empty. */

#define FILE_STATE_WRITE    0x01
#define FILE_STATE_SCREEN   0x02
#define FILE_STATE_IMAGE    0x04
#define FILE_STATE_READ     0x08

#define IS_READABLE(Private)    (Private->FileState & FILE_STATE_READ)
#define IS_WRITEABLE(Private)   (Private->FileState & FILE_STATE_WRITE)


typedef struct GifFilePrivateType {
    int FileState,
	FileHandle,			     /* Where all this data goes to! */
	BitsPerPixel,	    /* Bits per pixel (Codes uses at least this + 1). */
	ClearCode,				       /* The CLEAR LZ code. */
	EOFCode,				         /* The EOF LZ code. */
	RunningCode,		    /* The next code algorithm can generate. */
	RunningBits,/* The number of bits required to represent RunningCode. */
	MaxCode1,  /* 1 bigger than max. possible code, in RunningBits bits. */
	LastCode,		        /* The code before the current code. */
	CrntCode,				  /* Current algorithm code. */
	StackPtr,		         /* For character stack (see below). */
	CrntShiftState;		        /* Number of bits in CrntShiftDWord. */
    unsigned long CrntShiftDWord;     /* For bytes decomposition into codes. */
    unsigned long PixelCount;		       /* Number of pixels in image. */
    FILE *File;						  /* File as stream. */
    InputFunc Read;                 /* function to read gif input (TVT) */
    OutputFunc Write;               /* function to write gif output (MRB) */
    GifByteType Buf[256];	       /* Compressed input is buffered here. */
    GifByteType Stack[LZ_MAX_CODE];	 /* Decoded pixels are stacked here. */
    GifByteType Suffix[LZ_MAX_CODE+1];	       /* So we can trace the codes. */
    /* LZ_MAX_CODE, not +1 - see the comment at the top of the file: this
       release does not have the upstream off-by-one fix on the Prefix
       array (sizeof(GifFilePrivateType) is 4 bytes smaller). */
    unsigned int Prefix[LZ_MAX_CODE];
    GifHashTableType *HashTable;
} GifFilePrivateType;

extern int _GifError;

#ifdef SYSV
static char *VersionStr =
        "Gif library module,\t\tEric S. Raymond\n\
	(C) Copyright 1997 Eric S. Raymond\n";
#else
static char *VersionStr =
	PROGRAM_NAME
	"	IBMPC "
	GIF_LIB_VERSION
	"	Eric S. Raymond,	"
	__DATE__ ",   " __TIME__ "\n"
	"(C) Copyright 1997 Eric S. Raymond\n";
#endif /* SYSV */

#endif /* GIF_LIB_PRIVATE_H */
