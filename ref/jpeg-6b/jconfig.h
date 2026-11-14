/* jconfig.h --- written for this build (PS2 Linux, Sony gcc 2.95.2) */
/* The standard "Unix ANSI C" configuration; see jconfig.doc for the meaning of each define. */

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
/* #define void char */
/* #define const */
#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED
/* #define INLINE __inline__ -- jmorecfg.h already defines it when __GNUC__ */

#endif /* JPEG_INTERNALS */

#ifdef JPEG_CJPEG_DJPEG

#define BMP_SUPPORTED		/* BMP image file format */
#define GIF_SUPPORTED		/* GIF image file format */
#define PPM_SUPPORTED		/* PBMPLUS PPM/PGM image file format */
#undef RLE_SUPPORTED		/* Utah RLE image file format */
#define TARGA_SUPPORTED		/* Targa image file format */

#define TWO_FILE_COMMANDLINE
#define USE_SETMODE
#undef NEED_SIGNAL_CATCHER
#undef DONT_USE_B_MODE
#undef PROGRESS_REPORT

#endif /* JPEG_CJPEG_DJPEG */
