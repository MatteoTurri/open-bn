/*
 * The FreeType 2 declarations that bn uses. bn links the system's
 * libfreetype.so.6, but the kit has no FreeType headers: the records below
 * follow FreeType 2.0 and stop after the last field that bn reads.
 */

#ifndef BN_FREETYPE2_H
#define BN_FREETYPE2_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int FT_Error;
typedef long FT_Pos;     /* 26.6 fixed point */
typedef long FT_Fixed;   /* 16.16 fixed point */
typedef long FT_F26Dot6;
typedef unsigned char FT_Bool;

typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_FaceRec_ *FT_Face;
typedef struct FT_SizeRec_ *FT_Size;
typedef struct FT_GlyphSlotRec_ *FT_GlyphSlot;
typedef struct FT_GlyphRec_ *FT_Glyph;

typedef struct FT_Vector_ {
	FT_Pos x;
	FT_Pos y;
} FT_Vector;

typedef struct FT_Generic_ {
	void *data;
	void (*finalizer)(void *object);
} FT_Generic;

typedef struct FT_BBox_ {
	FT_Pos xMin, yMin;
	FT_Pos xMax, yMax;
} FT_BBox;

typedef struct FT_Glyph_Metrics_ {
	/* 0x00 */ FT_Pos width;
	/* 0x04 */ FT_Pos height;
	/* 0x08 */ FT_Pos horiBearingX;
	/* 0x0C */ FT_Pos horiBearingY;
	/* 0x10 */ FT_Pos horiAdvance;
	/* 0x14 */ FT_Pos vertBearingX;
	/* 0x18 */ FT_Pos vertBearingY;
	/* 0x1C */ FT_Pos vertAdvance;
} FT_Glyph_Metrics; /* size = 0x20 */

typedef struct FT_Size_Metrics_ {
	/* 0x00 */ unsigned short x_ppem;
	/* 0x02 */ unsigned short y_ppem;
	/* 0x04 */ FT_Fixed x_scale;
	/* 0x08 */ FT_Fixed y_scale;
	/* 0x0C */ FT_Pos ascender;
	/* 0x10 */ FT_Pos descender;
	/* 0x14 */ FT_Pos height;
	/* 0x18 */ FT_Pos max_advance;
} FT_Size_Metrics; /* size = 0x1C */

typedef struct FT_SizeRec_ {
	/* 0x00 */ FT_Face face;
	/* 0x04 */ FT_Generic generic;
	/* 0x0C */ FT_Size_Metrics metrics;
} FT_SizeRec;

/* Cut after advance. */
typedef struct FT_GlyphSlotRec_ {
	/* 0x00 */ FT_Library library;
	/* 0x04 */ FT_Face face;
	/* 0x08 */ FT_GlyphSlot next;
	/* 0x0C */ unsigned int flags;
	/* 0x10 */ FT_Generic generic;
	/* 0x18 */ FT_Glyph_Metrics metrics;
	/* 0x38 */ FT_Fixed linearHoriAdvance;
	/* 0x3C */ FT_Fixed linearVertAdvance;
	/* 0x40 */ FT_Vector advance;
} FT_GlyphSlotRec;

/* Cut after size. */
typedef struct FT_FaceRec_ {
	/* 0x00 */ long num_faces;
	/* 0x04 */ long face_index;
	/* 0x08 */ long face_flags;
	/* 0x0C */ long style_flags;
	/* 0x10 */ long num_glyphs;
	/* 0x14 */ char *family_name;
	/* 0x18 */ char *style_name;
	/* 0x1C */ int num_fixed_sizes;
	/* 0x20 */ void *available_sizes;
	/* 0x24 */ int num_charmaps;
	/* 0x28 */ void *charmaps;
	/* 0x2C */ FT_Generic generic;
	/* 0x34 */ FT_BBox bbox;
	/* 0x44 */ unsigned short units_per_EM;
	/* 0x46 */ short ascender;
	/* 0x48 */ short descender;
	/* 0x4A */ short height;
	/* 0x4C */ short max_advance_width;
	/* 0x4E */ short max_advance_height;
	/* 0x50 */ short underline_position;
	/* 0x52 */ short underline_thickness;
	/* 0x54 */ FT_GlyphSlot glyph;
	/* 0x58 */ FT_Size size;
} FT_FaceRec;

typedef struct FT_GlyphRec_ {
	/* 0x00 */ FT_Library library;
	/* 0x04 */ const void *clazz;
	/* 0x08 */ int format;
	/* 0x0C */ FT_Vector advance;
} FT_GlyphRec; /* size = 0x14 */

typedef struct FT_Bitmap_ {
	/* 0x00 */ int rows;
	/* 0x04 */ int width;
	/* 0x08 */ int pitch;
	/* 0x0C */ unsigned char *buffer;
	/* 0x10 */ short num_grays;
	/* 0x12 */ char pixel_mode;
	/* 0x13 */ char palette_mode;
	/* 0x14 */ void *palette;
} FT_Bitmap; /* size = 0x18 */

typedef struct FT_BitmapGlyphRec_ {
	/* 0x00 */ FT_GlyphRec root;
	/* 0x14 */ int left;
	/* 0x18 */ int top;
	/* 0x1C */ FT_Bitmap bitmap;
} FT_BitmapGlyphRec, *FT_BitmapGlyph; /* size = 0x34 */

FT_Error FT_Init_FreeType(FT_Library *alibrary);
FT_Error FT_Done_FreeType(FT_Library library);
FT_Error FT_New_Face(FT_Library library, const char *filepathname, long face_index, FT_Face *aface);
FT_Error FT_Done_Face(FT_Face face);
FT_Error FT_Set_Char_Size(FT_Face face, FT_F26Dot6 char_width, FT_F26Dot6 char_height, unsigned int horz_resolution,
    unsigned int vert_resolution);
unsigned int FT_Get_Char_Index(FT_Face face, unsigned long charcode);
FT_Error FT_Load_Glyph(FT_Face face, unsigned int glyph_index, int load_flags);
FT_Error FT_Get_Glyph(FT_GlyphSlot slot, FT_Glyph *aglyph);
FT_Error FT_Glyph_To_Bitmap(FT_Glyph *the_glyph, unsigned long render_mode, FT_Vector *origin, FT_Bool destroy);
void FT_Done_Glyph(FT_Glyph glyph);

#ifdef __cplusplus
}
#endif

#endif /* BN_FREETYPE2_H */
