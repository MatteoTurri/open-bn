/*
 * PLTM: text renderer. Two worker threads per font turn characters into FreeType
 * outlines and then into GS textures held in a glyph cache; the PLTM_Draw*
 * functions build GS packets from the cached glyphs.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <linux/ps2/gs.h>
#include "tm.h"
#include "gfx/pltm.h"
#include "gfx/freetype2.h"

#include <string.h>

/* PLTM_CacheItem::mode, the state of a cache entry ("pCacheRec->State" in the debug output). Mode 5 is
   handled like 6 and 7 but never set. */
#define PLTM_CACHE_FREE 0
#define PLTM_CACHE_QUEUED 1  /* waiting for the outline thread */
#define PLTM_CACHE_OUTLINE 2  /* the outline thread loads the glyph */
#define PLTM_CACHE_OUTLINE_DONE 3
#define PLTM_CACHE_BITMAP_QUEUED 4  /* waiting for the bitmap thread */
#define PLTM_CACHE_BITMAP 5  /* never set (UploadGlyph goes from 4 to 6): the bitmap thread at work, as PLTM_CACHE_OUTLINE */
#define PLTM_CACHE_RASTERIZED 6
#define PLTM_CACHE_ALLOCATED 7  /* GS resource allocated */
#define PLTM_CACHE_READY 8  /* GS texture ready */
#define PLTM_CACHE_WAIT 999  /* out of resources, waiting until something is freed */

/* Glyph cache entry (136 bytes): (charSize, code) identify the glyph in the slot. */
struct PLTM_CacheItem {
	/* 0x00 */ TmHandle tex;  /* glyph texture */
	/* 0x04 */ int used;
	/* 0x08 */ unsigned short mode;  /* PLTM_CACHE_* */
	/* 0x0A */ short pad_0A;
	/* 0x0C */ char pad_0C[28];
	/* 0x28 */ unsigned short code;
	/* 0x2A */ char pad_2A[2];
	/* 0x2C */ int charSize;
	/* 0x30 */ unsigned char glyphFlags;  /* copy of PLTM_Glyph.flags */
	/* 0x31 */ char pad_31[3];
	/* 0x34 */ FT_Glyph ftGlyph;
	/* 0x38 */ FT_Glyph_Metrics metrics;  /* of the glyph slot; horiAdvance is returned by GetCharWidthGM */
	/* 0x58 */ int top;  /* ascender: max over the two faces */
	/* 0x5C */ int bottom;  /* descender: min over the two faces */
	/* 0x60 */ void *resource;  /* GS resource, header at -16 */
	/* 0x64 */ int resSize;  /* 0 = no resource */
	/* 0x68 */ int texWidth;
	/* 0x6C */ int texHeight;
	/* 0x70 */ int texLogW;  /* log2 of the width (TEX0.TW) */
	/* 0x74 */ int texLogH;  /* log2 of the height (TEX0.TH) */
	/* 0x78 */ int bearingY;  /* subtracted from the baseline */
	/* 0x7C */ int bearingX;  /* added to the pen */
	/* 0x80 */ int gradTop;  /* top and bottom of the glyph in the line, 8.8 (color gradient) */
	/* 0x84 */ int gradBottom;
}; /* size = 0x88 */

/* Error characters of the UTF-8 and UTF-16 decoders. */
static unsigned int g_errCharUTF8 = 0;
static unsigned int g_errCharUTF16 = 0;

/* Debug tuning of the renderer: BlendValue256 is the filter weight of the
   glyph upload, ShiftX0/Y0 move the quad in pixels, HintSwitch enables
   FreeType hinting. Then the size below which glyph copies are sub-pixel and
   the ellipsis used when a string is truncated. */
int _pltmDBG_BlendValue256 = 64;
int _pltmDBG_ShiftX0 = 0;
int _pltmDBG_ShiftY0 = 0;
int _pltmDBG_HintSwitch = 0;
int _pltmSubPixelImageCopy_CharSize = 32;
char *_pltmOmmiter = "...";

/* Configuration copied by PLTM_Init; the callbacks are also kept separately. */
static struct PLTM_InitInfo s_initCfg = { 0 };
static void *(*s_allocFn)(int size) = 0;
static void (*s_freeFn)(void *p) = 0;
static void *(*s_poolAllocFn)(unsigned int size) = 0;
static void (*s_poolFreeFn)(void *hdr) = 0;
static void (*s_sendPacketFn)(void *pkt, int n) = 0;

/* Default queue and pool sizes, font directory, tm manager, default CLUT. */
static int s_cacheSize = 2000;
static int s_outlineQueueSize = 1000;
static int s_bitmapQueueSize = 2000;
static int s_polygonPoolSize = 1000;
static char *s_fontPath = 0;
static TmHandle s_tmID = 0;
static TmHandle s_defaultClut = 0;

/* Default font file per language id (PLTM_OpenI). */
/* clang-format off */
static char *s_langDefaultPaths[20] = {
	"SGR_SCE.PST", "SGR_SCE.PST", "SRGR_SCE.PST", "RM_SCE.PST",
	"SGR_SE1.PST", "SGR_SE2.PST", "SGB_SE1.PST", "SGB_SE2.PST",
	"SRGR_SE1.PST", "SRGR_SE2.PST", "SRGB_SE1.PST", "SRGB_SE2.PST",
	"MBR_SE1.PST", "MBR_SE2.PST", "MBDB_SE1.PST", "MBDB_SE2.PST",
	"RM_SE1.PST", "RM_SE2.PST", "REB_SE1.PST", "REB_SE2.PST",
};
/* clang-format on */

/* Options passed by address to RequestGlyph, which does not read them. */
static char s_drawOpts[32] = { 0 };

/* bytes allocated from the pool */
static int s_usedBytes = 0;

/* One quad vertex in A+D mode: RGBAQ, UV, XYZ2 (data + address). */
struct PLTM_GsVertex {
	unsigned long long rgbaq, rgbaq_addr;
	unsigned long long uv, uv_addr;
	unsigned long long xyz2, xyz2_addr;
};

/* GS packet of a glyph, 368 bytes (23 qwords). Only TEX0 and the four
   vertices change per glyph, the rest is prebuilt. */
struct PLTM_GlyphPacket {
	/* 0x00 */ unsigned long long head[4];
	/* 0x20 */ ps2_gsreg_tex0 tex0;
	/* 0x28 */ unsigned long long tex0_addr;
	/* 0x30 */ unsigned long long regs[16];
	/* 0xB0 */ struct PLTM_GsVertex v[4];
};

/* Debug traces, compiled with a false guard. */
#define PLTM_DEBUG 0
#define PLTM_DPRINT(args) if (PLTM_DEBUG) PLTM_Log args

struct PLTM_Item {
	int unk_00;
	char expired;
};

struct PLTM_Font;

/* Request queue entry (52 bytes). */
struct PLTM_Request {
	/* 0x00 */ unsigned short code;  /* = PLTM_CacheItem.code */
	short pad_02;
	/* 0x04 */ int charSize;  /* = PLTM_CacheItem.charSize */
	/* 0x08 */ int index;  /* index of the cache entry */
	char pad_0C[52 - 12];
};

/* Glyph cache: `count` entries of 136 bytes (PLTM_CacheItem). */
struct PLTM_CacheQueue {
	/* 0x00 */ struct PLTM_CacheItem *itemArrayBase;
	/* 0x04 */ int count;
	/* 0x08 */ pthread_mutex_t mutex;
	/* 0x20 */ pthread_cond_t condA;
	/* 0x2C */ pthread_cond_t condB;
};

/* Request queue: `count` entries of 52 bytes, ring indices, a mutex and three
   condvars. */
struct PLTM_ReqQueue {
	/* 0x00 */ struct PLTM_Request *items;
	/* 0x04 */ int queued;  /* entries queued */
	/* 0x08 */ int count;
	/* 0x0C */ int nonBlocking;  /* 0 for the bitmap queue */
	/* 0x10 */ int abort;
	/* 0x14 */ int closing;
	/* 0x18 */ int head;  /* next entry to take */
	/* 0x1C */ int tail;  /* next entry to fill */
	/* 0x20 */ pthread_mutex_t mutex;
	/* 0x38 */ pthread_cond_t notEmpty;
	/* 0x44 */ pthread_cond_t notFull;
	/* 0x50 */ pthread_cond_t drained;  /* signalled when the queue becomes empty */
};

/* An entry of the two 112-byte packet tables that OpenCommon allocates before
   the glyph packets; they are only copied around. */
struct PLTM_Pkt112 {
	int w[28];
};

/* Font handle returned by PLTM_OpenS/PLTM_OpenI: glyph cache, two worker
   threads (outline and bitmap) with their request queues, FreeType faces and
   GS packet buffers. +4 is a closing flag checked by every text function. */
struct PLTM_Font {
	/* 0x00 */ int numLoaded;  /* glyphs loaded */
	/* 0x04 */ unsigned char closing;
	char pad_05[3];
	/* 0x08 */ struct PLTM_CacheQueue queue;
	/* 0x40 */ struct PLTM_ReqQueue outlineQueue;
	char pad_9C[164 - 156];
	/* 0xA4 */ struct PLTM_ReqQueue bitmapQueue;
	char pad_100[264 - 256];
	/* 0x108 */ pthread_mutex_t mutex;
	/* 0x120 */ pthread_cond_t cond;
	/* 0x12C */ pthread_t outlineThread;
	/* 0x130 */ pthread_t bitmapThread;
	/* 0x134 */ int loadFlags;  /* FT_Load_Glyph load flags */
	/* 0x138 */ int renderMode;  /* FT_Glyph_To_Bitmap render mode */
	/* 0x13C */ int unk_13C;
	/* 0x140 */ FT_Library ftLibrary;
	/* 0x144 */ FT_Face ftFaceFull;  /* full-width face */
	/* 0x148 */ FT_Face ftFaceHalf;  /* half-width face */
	/* 0x14C */ int resX;  /* horizontal resolution (FT_Set_Char_Size) */
	/* 0x150 */ int resY;  /* vertical resolution */
	/* 0x154 */ struct PLTM_Pkt112 *pktTemplate;  /* start of the pool allocation (16-byte header before it) */
	/* 0x158 */ struct PLTM_Pkt112 *pktTemplate2;
	/* 0x15C */ struct PLTM_GlyphPacket *pools[2];  /* GS packet double buffer, indexed by curPool */
	/* 0x164 */ int pktPerPool;
	/* 0x168 */ int pktUsed;  /* reset by UpdateTimeStamp */
	/* 0x16C */ int curPool;  /* flipped by UpdateTimeStamp */
	/* 0x170 */ struct PLTM_CallbackBlock callbacks;  /* PLTM_SetCallbacks */
}; /* size = 0x190 */


/* Default CLUT pool set up by PLTM_Init. */
struct PLTM_Pool {
	/* 0x00 */ int entryBytes;  /* bytes per CLUT entry (4) */
	/* 0x04 */ int entryCount;  /* entry count (256) */
	/* 0x08 */ int clutFlags;  /* ORed with 0x40000000 for tmCreateClut */
	/* 0x0C */ int poolBytes;  /* buffer size (1024) */
	/* 0x10 */ void *buf;
};

static struct PLTM_Pool s_pool;

static pthread_mutex_t s_outlineSub;
static pthread_mutex_t s_bitmapSub;
static pthread_cond_t s_thirdSub;
static unsigned char s_flagPending;


/* GS register descriptor: two header bytes, then one record per register in
   alphabetical order of the name -- [register, n fields, n x (field id, shift,
   width)] -- and the terminator 0xff. The field ids are the tags of the
   variadic list of gsRegisterAddrAndParameters. XYZ3 and XYZF3 are 0x0c and
   0x0b here (0x0d and 0x0c in linux/ps2/gs.h). */
/* clang-format off */
static unsigned char s_gsRegDesc[] = {
	0xfe, 0,
	/* ALPHA_1 */
	0x42, 5,  99, 0, 2,  100, 2, 2,  101, 4, 2,  102, 6, 2,  103, 32, 8,
	/* ALPHA_2 */
	0x43, 5,  99, 0, 2,  100, 2, 2,  101, 4, 2,  102, 6, 2,  103, 32, 8,
	/* BITBLTBUF */
	0x50, 6,  104, 0, 14,  105, 16, 6,  106, 24, 6,  107, 32, 14,  108, 48, 6,  109, 56, 6,
	/* CLAMP_1 */
	0x08, 6,  110, 0, 2,  111, 2, 2,  112, 4, 10,  113, 14, 10,  114, 24, 10,  115, 34, 10,
	/* CLAMP_2 */
	0x09, 6,  110, 0, 2,  111, 2, 2,  112, 4, 10,  113, 14, 10,  114, 24, 10,  115, 34, 10,
	/* COLCLAMP */
	0x46, 1,  116, 0, 1,
	/* DIMX */
	0x44, 16,  117, 0, 3,  118, 4, 3,  119, 8, 3,  120, 12, 3,  121, 16, 3,  122, 20, 3,  123, 24, 3,  124, 28, 3,  125, 32, 3,  126, 36, 3,  127, 40, 3,  128, 44, 3,  129, 48, 3,  130, 52, 3,  131, 56, 3,  132, 60, 3,
	/* DTHE */
	0x45, 1,  133, 0, 1,
	/* FBA_1 */
	0x4a, 1,  134, 0, 1,
	/* FBA_2 */
	0x4b, 1,  134, 0, 1,
	/* FINISH */
	0x61, 0,
	/* FOG */
	0x0a, 1,  135, 56, 8,
	/* FOGCOL */
	0x3d, 3,  138, 0, 8,  137, 8, 8,  136, 16, 8,
	/* FRAME_1 */
	0x4c, 4,  46, 0, 9,  47, 16, 6,  48, 24, 6,  139, 32, 32,
	/* FRAME_2 */
	0x4d, 4,  46, 0, 9,  47, 16, 6,  48, 24, 6,  139, 32, 32,
	/* HWREG */
	0x54, 1,  140, 0, 64,
	/* LABEL */
	0x62, 2,  141, 32, 32,  87, 0, 32,
	/* MIPTBP1_1 */
	0x34, 6,  142, 0, 14,  143, 14, 6,  144, 20, 14,  145, 34, 6,  146, 40, 14,  147, 54, 6,
	/* MIPTBP1_2 */
	0x35, 6,  142, 0, 14,  143, 14, 6,  144, 20, 14,  145, 34, 6,  146, 40, 14,  147, 54, 6,
	/* MIPTBP2_1 */
	0x36, 6,  148, 0, 14,  149, 14, 6,  150, 20, 14,  151, 34, 6,  152, 40, 14,  153, 54, 6,
	/* MIPTBP2_2 */
	0x37, 6,  148, 0, 14,  149, 14, 6,  150, 20, 14,  151, 34, 6,  152, 40, 14,  153, 54, 6,
	/* PABE */
	0x49, 1,  154, 0, 1,
	/* PRIM */
	0x00, 9,  155, 0, 3,  156, 3, 1,  157, 4, 1,  158, 5, 1,  159, 6, 1,  160, 7, 1,  161, 8, 1,  162, 9, 1,  103, 10, 1,
	/* PRMODE */
	0x1b, 8,  156, 3, 1,  157, 4, 1,  158, 5, 1,  159, 6, 1,  160, 7, 1,  161, 8, 1,  162, 9, 1,  103, 10, 1,
	/* PRMODECONT */
	0x1a, 1,  163, 0, 1,
	/* RGBAQ */
	0x01, 5,  164, 0, 8,  165, 8, 8,  100, 16, 8,  99, 24, 8,  166, 32, 32,
	/* SCANMSK */
	0x22, 1,  167, 1, 2,
	/* SCISSOR_1 */
	0x40, 4,  168, 0, 11,  169, 16, 11,  170, 32, 11,  171, 48, 11,
	/* SCISSOR_2 */
	0x41, 4,  168, 0, 11,  169, 16, 11,  170, 32, 11,  171, 48, 11,
	/* SIGNAL */
	0x60, 2,  87, 0, 32,  141, 32, 32,
	/* ST */
	0x02, 2,  172, 0, 32,  173, 0, 32,
	/* TEST_1 */
	0x47, 8,  174, 0, 1,  175, 1, 3,  176, 4, 8,  177, 12, 2,  178, 14, 1,  179, 15, 1,  180, 16, 1,  181, 17, 2,
	/* TEST_2 */
	0x48, 8,  174, 0, 1,  175, 1, 3,  176, 4, 8,  177, 12, 2,  178, 14, 1,  179, 15, 1,  180, 16, 1,  181, 17, 2,
	/* TEX0_1 */
	0x06, 12,  182, 0, 14,  183, 14, 6,  48, 20, 6,  184, 26, 4,  185, 30, 4,  186, 34, 1,  187, 35, 2,  188, 37, 14,  189, 51, 4,  190, 55, 1,  191, 56, 5,  192, 61, 3,
	/* TEX0_2 */
	0x07, 12,  182, 0, 14,  183, 14, 6,  48, 20, 6,  184, 26, 4,  185, 30, 4,  186, 34, 1,  187, 35, 2,  188, 37, 14,  189, 51, 4,  190, 55, 1,  191, 56, 5,  192, 61, 3,
	/* TEX1_1 */
	0x14, 7,  193, 0, 1,  194, 2, 3,  195, 5, 1,  196, 6, 3,  197, 9, 1,  198, 15, 1,  199, 32, 8,
	/* TEX1_2 */
	0x15, 7,  193, 0, 1,  194, 2, 3,  195, 5, 1,  196, 6, 3,  197, 9, 1,  198, 15, 1,  199, 32, 8,
	/* TEX2_1 */
	0x16, 6,  48, 20, 6,  188, 37, 14,  189, 51, 4,  190, 55, 1,  191, 56, 5,  192, 61, 3,
	/* TEX2_2 */
	0x17, 6,  48, 20, 6,  188, 37, 14,  189, 51, 4,  190, 55, 1,  191, 56, 5,  192, 61, 3,
	/* TEXA */
	0x3b, 3,  200, 0, 8,  201, 32, 8,  202, 15, 1,
	/* TEXCLUT */
	0x1c, 3,  203, 0, 6,  204, 6, 6,  205, 12, 10,
	/* TEXFLUSH */
	0x3f, 0,
	/* TRXDIR */
	0x53, 1,  206, 0, 2,
	/* TRXPOS */
	0x51, 5,  207, 0, 11,  208, 16, 11,  209, 32, 11,  210, 48, 11,  95, 59, 2,
	/* TRXREG */
	0x52, 2,  211, 0, 12,  212, 32, 12,
	/* UV */
	0x03, 2,  213, 0, 16,  214, 16, 16,
	/* XYOFFSET_1 */
	0x18, 2,  215, 0, 16,  216, 32, 16,
	/* XYOFFSET_2 */
	0x19, 2,  215, 0, 16,  216, 32, 16,
	/* XYZ2 */
	0x05, 3,  217, 0, 16,  218, 16, 16,  219, 32, 32,
	/* XYZ3 */
	0x0c, 3,  217, 0, 16,  218, 16, 16,  219, 32, 32,
	/* XYZF2 */
	0x04, 4,  217, 0, 16,  218, 16, 16,  219, 32, 24,  135, 56, 8,
	/* XYZF3 */
	0x0b, 4,  217, 0, 16,  218, 16, 16,  219, 32, 24,  135, 56, 8,
	/* ZBUF_1 */
	0x4e, 3,  220, 0, 9,  48, 24, 4,  221, 32, 1,
	/* ZBUF_2 */
	0x4f, 3,  220, 0, 9,  48, 24, 4,  221, 32, 1,
	0xff,
};
/* clang-format on */
static unsigned short *s_gsRegTable = 0;

/* descriptor byte at offset i */
static __inline__ unsigned char *gsRegDescEntry(int i)
{
	return &s_gsRegDesc[i];
}

static int s_initFlag2620;


static __inline__ int IsHankakuCode(unsigned short code);
static __inline__ long long GsScreenX(long long a);
static __inline__ long long GsScreenY(long long a);
static __inline__ unsigned int ccc_decode_utf16(unsigned short **s);
static __inline__ unsigned int utf8_step(unsigned char **s);
static __inline__ unsigned long long gsPackParams(unsigned char *e, int tag, va_list ap);
static __inline__ void *PoolAlloc(int size);
static __inline__ void LerpColor4(unsigned char *dst, int di, const unsigned char *src, int si, int f);
static void *OpenCommon(const char *fullPath, const char *halfPath);
static int GetTextRectImpl(struct PLTM_Font *font, int halfW, int fullW, struct PLTM_String *str);
static int GetTextRectImplML(struct PLTM_Font *font, int halfW, int fullW, int maxWidth, int lineHeight,
    struct PLTM_String *str);
static int RequestGlyph(struct PLTM_Font *font, struct PLTM_CacheQueue *q, int code, int charSize, void *opts,
    int flag);
static int PushBitmapRequest(struct PLTM_Font *font, int index, struct PLTM_CacheItem *item);
static int PrepareGlyphs(struct PLTM_Font *font, int unusedA, int unusedB, struct PLTM_String *str, int *examined);
static int RenderText(struct PLTM_CacheQueue *q, int halfW, int fullW, int maxW, struct PLTM_String *str,
    unsigned char *buf, int xoff, int tw, int th, int *outX);
static void ClipQuadColors(unsigned char *dst, const unsigned char *src, int f0, int f1);
static void ClipQuadColorsGS(unsigned char *dst, const unsigned char *src, int f0, int f1);
static int InitCacheQueue(struct PLTM_CacheQueue *q);
static void *OutlineThread(struct PLTM_Font *font);
static void *BitmapThread(struct PLTM_Font *font);
static void RenderGlyph(struct PLTM_Font *font, struct PLTM_CacheItem *item);
static int UploadGlyph(struct PLTM_Font *font, struct PLTM_CacheItem *item);
static int AllocGlyphSlot(struct PLTM_CacheItem *item);
static int EmitGlyph(struct PLTM_Font *font, struct PLTM_CacheItem *item, long long x, long long y,
    struct PLTM_DrawSlot *slot);
static void BuildPacketTemplate(void *pkt_);
static void BuildGlyphPacket(struct PLTM_CacheItem *item, struct PLTM_GlyphPacket *p, long long x, long long y,
    const struct PltmColor *color, unsigned char narrow);
static int PLTM_Log(int level, const char *fmt, ...);
/* Declared inline before PLTM_Init calls it. */
__inline__ void PLTM_SetFontPath(const char *path);
/* Inline from their first use in this file on. */
static __inline__ int InitReqQueue(struct PLTM_ReqQueue *q, int count);
static __inline__ int ResolveGlyphIndex(struct PLTM_CacheQueue *q, struct PLTM_Glyph *key);
static __inline__ int ReclaimCacheSlot(struct PLTM_Font *font, struct PLTM_CacheQueue *q, int force);
static __inline__ int ReleaseCacheItem(struct PLTM_Font *font, struct PLTM_CacheItem *it);
static __inline__ int EmitGlyphPacket(struct PLTM_Font *font, struct PLTM_CacheItem *item, long long x, long long y,
    const struct PltmColor *color, unsigned char narrow);
static __inline__ int PLTM_BitsFor(int n);
static __inline__ unsigned int DecodeUtf8Step(const char **cursor);
__inline__ int _sceCccIsValidUTF16(unsigned int c);
__inline__ int _sceCccDecodeUTF16(unsigned short **s);
static __inline__ void _sceCccEncodeUTF16(unsigned short **s, unsigned int ucs);
static __inline__ int _sceCccStrlenUTF16(unsigned short *s);
static __inline__ int Utf8Len(unsigned char *s);
static __inline__ void BlitTile(unsigned char *dst, int width, int height, int xfix, int y, const unsigned char *src,
    int srcW, int srcH);
static __inline__ void BlitTileBlend(unsigned char *dst, int width, int height, int xfix, int y,
    const unsigned char *src, int srcW, int srcH);

static __inline__ long long GsScreenX(long long a)
{
	return a + ((2048 - 320) << 4);
}

static __inline__ long long GsScreenY(long long a)
{
	return a + ((2048 - 224) << 4);
}

/* 0x309B/0x309C: standalone (han)dakuten; 0xFF61-0xFF9F: half-width katakana. */
static __inline__ int IsHankakuCode(unsigned short code)
{
	int r = 1;
	if (code == 0x309B)
		return r;
	if (code == 0x309C)
		return r;
	r = 0;
	if (code >= 0xFF61 && code <= 0xFF9F)
		r = 1;
	return r;
}

static __inline__ void *PoolAlloc(int size)
{
	int *p = s_poolAllocFn(size + 16);

	if (p == 0)
		return 0;
	*p = size;
	s_usedBytes += size;
	return (char *)p + 16;
}

/* Lerps one RGBA color of a 2x2 grid towards the one below it (src + 8),
   8.8 factor. */
static __inline__ void LerpColor4(unsigned char *dst, int di, const unsigned char *src, int si, int f)
{
	dst[di] = src[si] + ((f * (src[si + 8] - src[si])) >> 8);
	dst[di + 1] = src[si + 1] + ((f * (src[si + 9] - src[si + 1])) >> 8);
	dst[di + 2] = src[si + 2] + ((f * (src[si + 10] - src[si + 2])) >> 8);
	dst[di + 3] = src[si + 3] + ((f * (src[si + 11] - src[si + 3])) >> 8);
}

/* Private copy of the ccc_decode_utf16 inline of src/lib/ccc.c. */
static __inline__ unsigned int ccc_decode_utf16(unsigned short **s)
{
	unsigned short *cur = *s;
	unsigned short c1 = *cur;
	unsigned short c2;
	unsigned int r;

	cur++;
	if ((unsigned short)(c1 + 0x2800) < 0x400) {
		c2 = *cur;
		cur++;
		if ((unsigned short)(c2 + 0x2400) < 0x400) {
			r = ((unsigned int)c1 << 16) | c2;
		} else {
			r = g_errCharUTF16;
		}
	} else {
		unsigned int d;

		d = (unsigned short)(c1 + 0x2400);
		r = (d > 1023) ? c1 : g_errCharUTF16;
	}
	*s = cur;
	return r;
}

static __inline__ unsigned int utf8_step(unsigned char **s)
{
	unsigned char *cur = *s;
	unsigned char lead = *cur;
	unsigned int acc;
	unsigned int limit;

	cur++;
	if ((signed char)lead >= 0) {
		acc = lead;
		goto done;
	}
	if ((unsigned char)(lead + 62) >= 51) {
		acc = g_errCharUTF8;
		goto done;
	}
	acc = lead & 0x3f;
	limit = 0x800;
	acc = (acc << 6) | (*cur & 0x3f);
	cur++;
	if (acc >= limit) {
		do {
			acc &= limit - 1;
			limit <<= 5;
			acc = (acc << 6) | (*cur & 0x3f);
			cur++;
		} while (acc >= limit);
	}
done:
	*s = cur;
	return acc;
}

static __inline__ unsigned long long gsPackParams(unsigned char *e, int tag, va_list ap)
{
	unsigned long long value = 0;
	int k = -1;
	int v;

	while (tag != 0) {
		/* NOTE: bug: the fast path tests e[k] instead of e[k + 2], one record off */
		if (k == -1 || e[k] != tag) {
			for (k = 0; k < 3 * e[1]; k += 3)
				if (e[k + 2] == tag)
					break;
			if (k == e[1] * 3)
				return 0;
		}
		v = va_arg(ap, int);
		tag = va_arg(ap, int);
		value |= (unsigned long long)(unsigned int)v << e[k + 3];
		k += 3;
	}
	return value;
}

/* Copies the user configuration, creates the tm manager with the user
   alloc/free, initializes the mutexes, the condvar and the default font path,
   then builds the default CLUT: 256 entries of 4 bytes in the GS CLUT swizzle
   order (blocks of 8 entries swapped between 16-entry halves), alpha 255 on the
   first entryBytes-1 bytes and a ramp (i*128)/entryCount on the last one. */
void PLTM_Init(struct PLTM_InitInfo *info)
{
	struct TmManagerInfoEx mgrInfo;
	int i, j;

	s_initCfg = *info;
	s_allocFn = s_initCfg.alloc;
	s_freeFn = s_initCfg.free;
	s_poolAllocFn = s_initCfg.poolAlloc;
	s_poolFreeFn = s_initCfg.poolFree;
	s_sendPacketFn = s_initCfg.sendPacket;

	s_tmID = tmCreateManager(s_initCfg.tmFrontSize, s_initCfg.tmBackSize);

	memset(&mgrInfo, 0, sizeof mgrInfo);
	mgrInfo.info.size = sizeof mgrInfo;
	tmGetManagerInfo(s_tmID, &mgrInfo.info);
	mgrInfo.initInfo.allocFn = s_poolAllocFn;
	mgrInfo.initInfo.freeFn = s_poolFreeFn;
	tmSetManagerInfo(s_tmID, &mgrInfo.info);

	pthread_mutex_init(&s_outlineSub, 0);
	pthread_mutex_init(&s_bitmapSub, 0);
	pthread_cond_init(&s_thirdSub, 0);
	s_flagPending = 0;
	s_initFlag2620 = 0;
	PLTM_SetFontPath("/opt0/bn/fonts");

	s_pool.entryBytes = 4;
	s_pool.entryCount = 256;
	s_pool.poolBytes = 1024;
	s_pool.clutFlags = 0;

	s_pool.buf = PoolAlloc(s_pool.poolBytes);

	for (i = 0; i < s_pool.entryCount; i++) {
		int m = i % 8;
		int lo = (i % 32 < 16) ? m : m + 8;
		int row = (i / 32) * 2 + (i % 16 >= 8);
		int pos;

		pos = (row << 4) + lo;
		for (j = 0; j < s_pool.entryBytes - 1; j++)
			((unsigned char *)s_pool.buf)[s_pool.entryBytes * pos + j] = 255;
		((unsigned char *)s_pool.buf)[s_pool.entryBytes * pos + j] = (i * 128) / s_pool.entryCount;
	}

	s_defaultClut = tmCreateClut(s_tmID, 19, s_pool.clutFlags | 0x40000000);
	tmUpdateClut(s_defaultClut, 0, s_pool.buf);
	nice(-20);
}

__inline__ void PLTM_SetCacheSize(int size)
{
	s_cacheSize = size;
}

__inline__ void PLTM_SetPolygonPoolSize(int size)
{
	s_polygonPoolSize = size;
}

__inline__ void PLTM_SetRequestQueueSize(int size)
{
	s_outlineQueueSize = size;
	s_bitmapQueueSize = size * 2;
}

__inline__ void PLTM_SetOutlineRequestQueueSize(int size)
{
	s_outlineQueueSize = size;
}

__inline__ void PLTM_SetBitmapRequestQueueSize(int size)
{
	s_bitmapQueueSize = size;
}

__inline__ void PLTM_SetFontPath(const char *path)
{
	if (s_fontPath)
		s_freeFn(s_fontPath);
	if (path) {
		int len = strlen(path);
		if (len != 0) {
			char *buf = s_allocFn(len + 1);
			s_fontPath = buf;
			strcpy(buf, path);
		}
	}
}

__inline__ void PLTM_InitClut(void)
{}

__inline__ void PLTM_PeekClut(int *format, void **addrOut, int *countOut)
{
	*format = 19; /* GS_PSMT8: 8-bit CLUT pixel format */
	/* the CLUT lives in s_pool: its address in clutFlags, its count in buf */
	*addrOut = (void *)s_pool.clutFlags;
	*countOut = (int)s_pool.buf;
}

__inline__ void PLTM_Terminate(void)
{
	void *hdr;
	int size;

	pthread_mutex_destroy(&s_outlineSub);
	pthread_mutex_destroy(&s_bitmapSub);
	pthread_cond_destroy(&s_thirdSub);

	s_freeFn(s_fontPath);

	s_fontPath = 0;
	if (s_defaultClut)
		tmRelease(s_defaultClut);
	s_defaultClut = 0;
	tmRelease(s_tmID);
	s_tmID = 0;

	hdr = (char *)s_pool.buf - 16;
	size = *(int *)hdr;
	s_usedBytes -= size;
	s_poolFreeFn(hdr);

	if (s_flagPending) {
		s_flagPending = 0;
		pthread_cond_broadcast(&s_thirdSub);
	}
}

__inline__ TmHandle PLTM_tmID(void)
{
	return s_tmID;
}

/* "%s/%s" joins the font directory and the file name; OpenI takes the file
   from s_langDefaultPaths. */
__inline__ void *PLTM_OpenS(const char *fullPath, const char *halfPath)
{
	char info[4096];
	char buf[4096];

	if (fullPath == 0 && halfPath == 0)
		return 0;
	if (fullPath == 0)
		fullPath = halfPath;
	if (halfPath == 0)
		halfPath = fullPath;

	sprintf(info, "%s/%s", s_fontPath, fullPath);
	sprintf(buf, "%s/%s", s_fontPath, halfPath);
	return OpenCommon(info, buf);
}

__inline__ void *PLTM_OpenI(int fullId, int halfId)
{
	char info[4096];
	char buf[4096];

	if (fullId == -1 && halfId == -1)
		return 0;
	if (fullId == -1)
		fullId = halfId;
	if (halfId == -1)
		halfId = fullId;

	sprintf(info, "%s/%s", s_fontPath, s_langDefaultPaths[fullId]);
	sprintf(buf, "%s/%s", s_fontPath, s_langDefaultPaths[halfId]);
	return OpenCommon(info, buf);
}

/* Walks the cache entries: when the queue has exactly 8 entries every used
   slot is freed and invalidated, otherwise the used slots are only counted. */
__inline__ int PLTM_ExpireAll(void *font_)
{
	struct PLTM_Font *font = font_;
	struct PLTM_CacheQueue *q;
	int expired = 0;
	int i;

	if (font == 0)
		return 0;
	q = &font->queue;
	for (i = 0; i < q->count; i++) {
		struct PLTM_CacheItem *item = &q->itemArrayBase[i];
		unsigned short mode = item->mode;
		if (mode != PLTM_CACHE_FREE) {
			if (mode == PLTM_CACHE_READY) {
				pthread_mutex_lock(&s_outlineSub);
				FT_Done_Glyph(item->ftGlyph);
				pthread_mutex_unlock(&s_outlineSub);
				tmRelease(item->tex);
				item->tex = 0;
				if (item->resource != 0) {
					char *hdr = (char *)item->resource - 16;
					int size = *(int *)hdr;
					s_usedBytes -= size;
					s_poolFreeFn(hdr);
					if (s_flagPending) {
						s_flagPending = 0;
						pthread_cond_broadcast(&s_thirdSub);
					}
					item->resource = 0;
				}
				item->mode = PLTM_CACHE_FREE;
				item->used = 0;
				item->code = 0xffff;
				item->charSize = -1;
			} else {
				expired++;
			}
		}
	}
	return expired;
}

__inline__ void PLTM_SetCallbacks(void *font_, struct PLTM_CallbackBlock *cb)
{
	struct PLTM_Font *font = font_;

	font->callbacks = *cb;
}

__inline__ void PLTM_CreateStringFromUtf8(int charSize, struct PLTM_String *out, const char *src)
{
	int len, allocSize, empty;
	struct PLTM_Glyph *g;

	if (out == 0)
		return;
	if (src == 0) {
		out->glyphs = 0;
		return;
	}
	len = Utf8Len(src);
	empty = len == 0;
	out->count = len;
	out->bottom = 0;
	out->top = 0;
	if (empty)
		allocSize = (len + 1) * 20;
	else
		allocSize = len * 20;
	out->glyphs = s_allocFn(allocSize);
	g = out->glyphs;
	if (empty) {
		g->code = 0;
		g->charSize = charSize;
		g->index = -1;
		g->flags = 0;
		out->count = out->count + 1;
	} else {
		int i;

		for (i = 0; i < out->count; i++) {
			unsigned short units[8];
			unsigned short *outIter;
			unsigned int decoded;

			decoded = DecodeUtf8Step(&src);
			outIter = units;
			_sceCccEncodeUTF16(&outIter, decoded);
			if (outIter - units < 2) {
				unsigned short code0 = units[0];

				g->charSize = charSize;
				g->index = -1;
				g->flags = 0;
				g->code = code0;
				g++;
			}
		}
	}
	out->rectFinal = 0;
	out->ready = 0;
	out->readyML = 0;
	out->hasRect = 0;
	out->spacing = 0;
	out->mode = 0;
}

__inline__ void PLTM_CreateStringFromUcs2(int charSize, struct PLTM_String *out, const unsigned short *src)
{
	int len, allocSize, empty;
	struct PLTM_Glyph *g;

	if (out == 0)
		return;
	if (src == 0) {
		out->glyphs = 0;
		return;
	}
	len = _sceCccStrlenUTF16((unsigned short *)src);
	empty = len == 0;
	out->count = len;
	out->bottom = 0;
	out->top = 0;
	if (empty)
		allocSize = (len + 1) * 20;
	else
		allocSize = len * 20;
	out->glyphs = s_allocFn(allocSize);
	g = out->glyphs;
	if (empty) {
		g->code = 0;
		g->charSize = charSize;
		g->index = -1;
		out->count = out->count + 1;
	} else {
		int i;
		for (i = 0; i < out->count; i++) {
			unsigned short c = *src;
			src++;
			g->charSize = charSize;
			g->index = -1;
			g->code = c;
			g++;
		}
	}
	out->rectFinal = 0;
	out->ready = 0;
	out->readyML = 0;
	out->hasRect = 0;
}

__inline__ void PLTM_CreateStringFromSjis(int charSize, struct PLTM_String *out, const char *src)
{
	int len, allocSize, empty;
	struct PLTM_Glyph *g;

	if (src == 0) {
		out->glyphs = 0;
		return;
	}
	len = strlen(src);
	empty = len == 0;
	out->count = len;
	out->bottom = 0;
	out->top = 0;
	if (empty)
		allocSize = (len + 1) * 20;
	else
		allocSize = len * 20;
	out->glyphs = s_allocFn(allocSize);
	g = out->glyphs;
	if (empty) {
		g->code = 0;
		g->charSize = charSize;
		g->index = -1;
		out->count = out->count + 1;
	} else {
		int i;
		for (i = 0; i < out->count; i++) {
			unsigned char c = *src;
			src++;
			g->charSize = charSize;
			g->index = -1;
			g->code = c;
			g++;
		}
	}
	out->rectFinal = 0;
	out->ready = 0;
	out->readyML = 0;
	out->hasRect = 0;
	out->spacing = 0;
	out->mode = 0;
}

__inline__ void PLTM_DestroyString(struct PLTM_String *s)
{
	if (s == 0)
		return;
	if (s->glyphs) {
		s_freeFn(s->glyphs);
		s->count = 0;
		s->bottom = 0;
		s->top = 0;
	}
}

__inline__ int PLTM_IsDirty(void *font_, int count, int start, struct PLTM_String *str)
{
	struct PLTM_Font *font = font_;
	int dirty = 0;
	int i;
	struct PLTM_CacheQueue *q;
	struct PLTM_Glyph *g;

	if (font == 0)
		return 1;
	q = &font->queue;
	if (font->closing != 0)
		return 1;
	if (count < 0)
		count = str->count;
	if (start + count > str->count)
		count = str->count - start;
	g = str->glyphs + start;
	for (i = 0; i < count; i++, g++) {
		int idx = g->index;
		struct PLTM_CacheItem *item;
		int mode;

		item = &q->itemArrayBase[idx];
		if (idx < 0)
			return 1;
		if (g->charSize != item->charSize || g->code != item->code) {
			dirty = 1;
			g->index = -1;
		}
		switch (item->mode) {
		case PLTM_CACHE_FREE:
		case PLTM_CACHE_QUEUED:
		case PLTM_CACHE_OUTLINE:
		case PLTM_CACHE_OUTLINE_DONE:
		case PLTM_CACHE_BITMAP_QUEUED:
		case PLTM_CACHE_BITMAP:
		case PLTM_CACHE_RASTERIZED:
		case PLTM_CACHE_ALLOCATED:
			dirty = 1;
			break;
		case PLTM_CACHE_READY:
			break;
		case PLTM_CACHE_WAIT:
			dirty = 1;
			break;
		}
	}
	if (dirty) {
		str->rectFinal = 0;
		str->ready = 0;
		str->readyML = 0;
		str->hasRect = 0;
		return 1;
	}
	return 0;
}

__inline__ void PLTM_Touch(void *font_, struct PLTM_String *str)
{
	struct PLTM_Font *font = font_;
	int i;
	struct PLTM_Glyph *g;
	struct PLTM_CacheQueue *q;

	if (font == 0)
		return;
	if (font->closing != 0)
		return;
	q = &font->queue;
	i = 0;
	g = str->glyphs;
	if (str->count <= 0)
		return;
	do {
		int idx = g->index;
		if (idx >= 0) {
			struct PLTM_CacheItem *item = &q->itemArrayBase[idx];
			if (g->charSize == item->charSize) {
				if (g->code == item->code)
					item->used = 0;
			}
		}
		i++;
		g++;
	} while (i < str->count);
}

__inline__ int PLTM_GetCharWidthGM(void *font_, struct PLTM_Glyph *ci)
{
	struct PLTM_Font *font = font_;
	struct PLTM_CacheQueue *q = &font->queue;
	int idx;

	if (font == 0)
		return -1;
	if (font->closing != 0)
		return -1;
	if (ci->index < 0) {
		struct PLTM_Glyph key;
		key.code = ci->code;
		key.charSize = ci->charSize;
		ci->index = ResolveGlyphIndex(q, &key);
		if (ci->index < 0)
			return -1;
	}
	idx = ci->index;
	{
		struct PLTM_CacheItem *item = &font->queue.itemArrayBase[idx];
		if (ci->charSize != item->charSize || ci->code != item->code)
			ci->index = -1;
	}
	if (ci->index >= 0) {
		struct PLTM_CacheItem *cached = &q->itemArrayBase[ci->index];

		switch (cached->mode) {
		case PLTM_CACHE_FREE:
		case PLTM_CACHE_QUEUED:
		case PLTM_CACHE_OUTLINE:
			return -1;
		case PLTM_CACHE_OUTLINE_DONE:
		case PLTM_CACHE_BITMAP_QUEUED:
		case PLTM_CACHE_BITMAP:
		case PLTM_CACHE_RASTERIZED:
		case PLTM_CACHE_ALLOCATED:
		case PLTM_CACHE_READY:
			cached->used = 0;
			return cached->metrics.horiAdvance;
		}
	}
	return -1;
}

__inline__ int PLTM_GetTextRect(void *font_, int halfW, int fullW, struct PLTM_String *str)
{
	struct PLTM_Font *font = font_;
	return GetTextRectImpl(font, halfW, fullW, str);
}

__inline__ int PLTM_GetTextRect_ML(void *font_, int halfW, int fullW, int maxWidth, int lineHeight,
    struct PLTM_String *str)
{
	struct PLTM_Font *font = font_;
	return GetTextRectImplML(font, halfW, fullW, maxWidth, lineHeight, str);
}

__inline__ void PLTM_UpdateTimeStamp(void *font_)
{
	struct PLTM_Font *font = font_;
	int i;
	struct PLTM_CacheQueue *q;
	struct PLTM_CacheItem *item;

	if (font == 0)
		return;
	q = &font->queue;
	for (i = 0, item = q->itemArrayBase; i < q->count; i++, item++) {
		if (item->mode != PLTM_CACHE_FREE) {
			if (item->mode == PLTM_CACHE_READY) {
				if (item->used <= 0x8c9f)
					item->used = item->used + 1;
			}
		}
	}
	pthread_mutex_lock(&s_bitmapSub);
	if (s_flagPending) {
		int j = 0;
		while (j < 10) {
			int r = ReclaimCacheSlot(font, &font->queue, 1);
			if (r == -2)
				break;
			j++;
		}
	}
	pthread_mutex_unlock(&s_bitmapSub);
	{
		int oldPool = font->curPool;
		font->pktUsed = 0;
		font->curPool = oldPool ^ 1;
	}
}

__inline__ void PLTM_DumpQueue(void *font)
{
	struct PLTM_ReqQueue *outline;
	struct PLTM_ReqQueue *bitmap;
	struct PLTM_Request *outReq;
	struct PLTM_Request *bitReq;
	struct PLTM_CacheItem *outItem;
	struct PLTM_CacheItem *bitItem;

	if (font == 0)
		return;

	outline = &((struct PLTM_Font *)font)->outlineQueue;
	bitmap = &((struct PLTM_Font *)font)->bitmapQueue;

	if (outline->queued <= 0 && bitmap->queued == 0)
		return;

	outReq = outline->items + outline->head;
	bitReq = bitmap->items + bitmap->head;

	outItem = &((struct PLTM_Font *)font)->queue.itemArrayBase[outReq->index];
	bitItem = &((struct PLTM_Font *)font)->queue.itemArrayBase[bitReq->index];

	fprintf(stderr, "%8.8x: OQ=%6d/%6d(%d) : BQ=%6d/%6d(%d)\n", font, outline->queued, outline->count,
	    outItem->mode, bitmap->queued, bitmap->count, bitItem->mode);
}

/* Destroys a font handle. 1: for every cache entry in mode PLTM_CACHE_BITMAP to PLTM_CACHE_READY releases
   the FreeType glyph and, when ready, the texture and the GS resource. 2: frees
   the packet templates. 3: stops the two worker threads (closing flags, signals, joins),
   destroys every mutex and condvar, the FreeType faces and library, and
   frees the font. */
void PLTM_Close(void *font_)
{
	struct PLTM_Font *font = font_;
	struct PLTM_CacheQueue *q;
	pthread_cond_t *outNotEmpty;
	pthread_cond_t *cacheCond2;
	pthread_mutex_t *fontMutex, *bmMutex;
	pthread_cond_t *outNotFull;
	struct PLTM_ReqQueue *outline;
	pthread_cond_t *outDrained, *bmNotEmpty;
	pthread_mutex_t *cacheMutex;
	pthread_cond_t *bmNotFull;
	struct PLTM_ReqQueue *bitmap;
	pthread_mutex_t *outMutex;
	pthread_cond_t *bmDrained, *cacheCond;
	int i;
	int n;
	void *ret;
	void **pret;

	if (font == 0)
		return;

	q = &font->queue;
	n = q->count;
	outline = &font->outlineQueue;
	outNotEmpty = &font->outlineQueue.notEmpty;
	bitmap = &font->bitmapQueue;
	bmNotEmpty = &font->bitmapQueue.notEmpty;
	pret = &ret;
	outNotFull = &font->outlineQueue.notFull;
	outDrained = &font->outlineQueue.drained;
	outMutex = &font->outlineQueue.mutex;
	bmNotFull = &font->bitmapQueue.notFull;
	bmDrained = &font->bitmapQueue.drained;
	bmMutex = &font->bitmapQueue.mutex;
	cacheCond = &font->queue.condA;
	cacheCond2 = &font->queue.condB;
	cacheMutex = &font->queue.mutex;
	fontMutex = &font->mutex;

	i = 0;
	if (i < n) {
		do {
			struct PLTM_CacheItem *item = &q->itemArrayBase[i];
			int mode = item->mode;

			if (mode != PLTM_CACHE_FREE) {
				if (mode <= PLTM_CACHE_READY) {
					if (mode > PLTM_CACHE_BITMAP_QUEUED) {
						pthread_mutex_lock(&s_outlineSub);
						FT_Done_Glyph(item->ftGlyph);
						pthread_mutex_unlock(&s_outlineSub);

						if (item->mode == PLTM_CACHE_READY) {
							tmRelease(item->tex);
							item->tex = 0;
							if (item->resource != 0) {
								char *hdr = (char *)item->resource - 16;
								int size = *(int *)hdr;
								s_usedBytes -= size;
								s_poolFreeFn(hdr);
								if (s_flagPending) {
									s_flagPending = 0;
									pthread_cond_broadcast(&s_thirdSub);
								}
								item->resource = 0;
							}
						}
					}
				}
			}
			i++;
		} while (i < q->count);
	}

	if (font->pktPerPool > 0) {
		char *hdr = (char *)font->pktTemplate - 16;
		int size = *(int *)hdr;
		s_usedBytes -= size;
		s_poolFreeFn(hdr);
		if (s_flagPending) {
			s_flagPending = 0;
			pthread_cond_broadcast(&s_thirdSub);
		}
	}

	outline->closing = 1;
	pthread_cond_signal(outNotEmpty);
	bitmap->closing = 1;
	pthread_cond_signal(bmNotEmpty);

	pthread_join(font->bitmapThread, pret);
	pthread_join(font->outlineThread, pret);

	pthread_cond_destroy(outNotEmpty);
	pthread_cond_destroy(outNotFull);
	pthread_cond_destroy(outDrained);
	pthread_mutex_destroy(outMutex);
	pthread_cond_destroy(bmNotEmpty);
	pthread_cond_destroy(bmNotFull);
	pthread_cond_destroy(bmDrained);
	pthread_mutex_destroy(bmMutex);
	pthread_cond_destroy(cacheCond);
	pthread_cond_destroy(cacheCond2);
	pthread_mutex_destroy(cacheMutex);
	pthread_mutex_destroy(fontMutex);

	FT_Done_Face(font->ftFaceHalf);
	FT_Done_Face(font->ftFaceFull);
	FT_Done_FreeType(font->ftLibrary);

	s_freeFn(font);
}

/* PLTM_CreateStringFromUtf8 with a substitution table: a code point below 256
   with a non-empty table[cp] is replaced by that UTF-8 string and the glyph
   gets flags = 1. useFontMetric sets out->spacing = font << 6; mode goes
   to out->mode. */
void PLTM_CreateStringFromUtf8_2(int charSize, struct PLTM_String *out, const char *src, const char **table,
    unsigned char useFontMetric, unsigned char mode)
{
	int len, allocSize, empty;
	struct PLTM_Glyph *g;

	if (out == 0)
		return;
	if (src == 0) {
		out->glyphs = 0;
		return;
	}
	len = Utf8Len(src);
	empty = len == 0;
	out->count = len;
	out->bottom = 0;
	out->top = 0;
	if (empty)
		allocSize = (len + 1) * 20;
	else
		allocSize = len * 20;
	out->glyphs = s_allocFn(allocSize);
	g = out->glyphs;
	if (empty) {
		g->code = 0;
		g->charSize = charSize;
		g->index = -1;
		g->flags = 0;
		out->count = out->count + 1;
	} else {
		int i;

		for (i = 0; i < out->count; i++) {
			unsigned short units[8];
			unsigned short *outIter;
			const char *sub;
			unsigned int decoded;

			decoded = DecodeUtf8Step(&src);
			g->flags = 0;
			if (decoded < 256 && table != 0) {
				const char *p = table[decoded];

				if (p != 0) {
					sub = p;
					if (*(const unsigned char *)sub != 0) {
						decoded = DecodeUtf8Step(&sub);
						g->flags = 1;
					}
				}
			}
			outIter = units;
			_sceCccEncodeUTF16(&outIter, decoded);
			if (outIter - units < 2) {
				unsigned short code0 = units[0];

				g->charSize = charSize;
				g->index = -1;
				g->code = code0;
				g++;
			}
		}
	}
	out->rectFinal = 0;
	out->ready = 0;
	out->readyML = 0;
	out->hasRect = 0;
	if (useFontMetric)
		out->spacing = charSize << 6;
	else
		out->spacing = 0;
	out->mode = mode;
}

/* Draws a laid-out string through the s_sendPacketFn packet callback. The "all ready"
   flags are str->ready/readyML. Codes 0x309B/0x309C and 0xFF61-0xFF9F use
   the glyph metric, the others the forced spacing str->spacing, scaled by 0.75
   when g->flags and str->mode are both set: adv + spacing * 0.75 for
   modes 3-7, (adv + spacing) * 0.75 for mode 8. */
int PLTM_DrawString(void *font_, int halfW, int fullW, long long pen, int baseline, int unused8, int count, int start,
    struct PLTM_String *str, const struct PltmColor *color)
{
	struct PLTM_Font *font = font_;
	int ret = 0;
	int i;
	struct PLTM_CacheQueue *q;
	struct PLTM_Glyph *g;
	struct PltmColor rec;

	(void)unused8;

	if (s_sendPacketFn == 0)
		return 0;
	if (font == 0)
		return 0;
	if (font->closing != 0)
		return 0;
	q = &font->queue;
	if (count < 0)
		count = str->count;
	if (str->count < start + count)
		count = str->count - start;
	if (count <= 0)
		return 0;
	PLTM_DPRINT((0, "====PLTM_DrawString====\n"));
	str->ready = 1;
	g = str->glyphs + start;
	for (i = 0; i < count; i++, g++) {
		unsigned short code;
		int pad, adv;
		struct PLTM_CacheItem *item;

		code = g->code;
		adv = (code >= 256) ? fullW : halfW;
		if (str->spacing)
			adv = fullW;
		pad = adv >> 1;
		if (i == 0)
			pad = 0;

		if (g->index < 0) {
			struct PLTM_Glyph key;

			PLTM_DPRINT((0, "AAA:\n"));
			key.code = code;
			key.charSize = g->charSize;
			g->index = ResolveGlyphIndex(q, &key);
			PLTM_DPRINT((0, "pCharRec->SlotNum=%d, i=%d\n", g->index, i));
			PLTM_DPRINT((0, "0x%4.4x(%d), ", code, adv));
			if (g->index < 0)
				goto missing;
		}
		{
			struct PLTM_CacheItem *chk = &q->itemArrayBase[g->index];

			PLTM_DPRINT((0, "2:pCharRec->SlotNum=%d\n", g->index));
			if (g->charSize != chk->charSize || g->code != chk->code) {
				PLTM_DPRINT(
				    (0, "Size=%d, %d,  Code=%d, %d\n", g->charSize, chk->charSize, g->code, chk->code));
				PLTM_DPRINT((0, "PLTM ===> Expired\n"));
				str->ready = 0;
				str->readyML = 0;
				g->index = -1;
			}
		}
		if (g->index < 0)
			goto missing;
		item = &q->itemArrayBase[g->index];
		PLTM_DPRINT((0, "===[pCacheRec->State=%d]===\n", item->mode));
		switch (item->mode) {
		case PLTM_CACHE_FREE:
		case PLTM_CACHE_QUEUED:
		case PLTM_CACHE_OUTLINE:
			str->ready = 0;
			str->readyML = 0;
			break;
		case PLTM_CACHE_OUTLINE_DONE:
		case PLTM_CACHE_BITMAP_QUEUED:
		case PLTM_CACHE_BITMAP:
		case PLTM_CACHE_RASTERIZED:
		case PLTM_CACHE_ALLOCATED:
			if (str->spacing != 0 && !IsHankakuCode(g->code)) {
				if (g->flags != 0 && str->mode != 0)
					pen += (long long)(adv + (int)((float)str->spacing * 0.75f));
				else
					pen += (long long)(adv + str->spacing);
			} else {
				pen += (long long)(adv + item->metrics.horiAdvance);
			}
			str->ready = 0;
			str->readyML = 0;
			break;
		case PLTM_CACHE_READY:
			PLTM_DPRINT((0, "3:pCharRec->SlotNum=%d\n", g->index));
			if (item->code == 0) {
				item->used = 0;
				break;
			}
			ClipQuadColors((unsigned char *)&rec, (const unsigned char *)color, item->gradTop,
			    item->gradBottom);
			{
				int x;

				if (str->spacing != 0 && !IsHankakuCode(g->code)) {
					if (g->flags != 0 && str->mode != 0)
						x = (int)((float)(pad + item->bearingX + str->spacing
						              - item->metrics.horiAdvance)
						        * 0.75f)
						    + (int)pen;
					else
						x = pad + (int)pen + item->bearingX
						    + (str->spacing - item->metrics.horiAdvance) / 2;
				} else {
					x = pad + (int)pen + item->bearingX;
				}
				EmitGlyphPacket(font, item, (long long)x, (long long)(baseline - item->bearingY), &rec,
				    g->flags && str->mode);
			}
			item->used = 0;
			if (str->spacing != 0 && !IsHankakuCode(g->code)) {
				if (g->flags != 0 && str->mode != 0)
					pen += (long long)(int)((float)(adv + str->spacing) * 0.75f);
				else
					pen += (long long)(adv + str->spacing);
			} else {
				pen += (long long)(adv + item->metrics.horiAdvance);
			}
			break;
		default:
			break;
		}
		continue;
missing:
		str->ready = 0;
		str->readyML = 0;
		{
			int idx = RequestGlyph(font, q, g->code, g->charSize, s_drawOpts, 0);

			g->index = idx;
			PLTM_DPRINT((0, "4:pCharRec->SlotNum=%d\n", idx));
			if (idx >= 0) {
				struct PLTM_CacheItem *it;

				it = &q->itemArrayBase[idx];
				it->used = 0;
				it->glyphFlags = g->flags;
			} else {
				ret = -1;
			}
		}
	}
	PLTM_DPRINT((0, "\n"));
	PLTM_DPRINT((0, "====PLTM_DrawString (end)====\n"));
	return ret;
}

/* Draws a laid-out string into the slot array: clamps the range, resolves
   every glyph in the cache (requesting the missing ones), and for ready
   glyphs emits the quad and advances the pen (64 bit). */
int PLTM_DrawStringGM(void *font_, int halfW, int fullW, long long penStart, long long baseline, int count, int start,
    struct PLTM_String *str, const struct PltmColor *color, struct PLTM_DrawSlot *slotsArg)
{
	struct PLTM_Font *font = font_;
	int i;
	int drawn = 0;
	int allOk = 1;
	long long pen = penStart;
	struct PLTM_DrawSlot *slot = slotsArg;
	int wasReady = str->ready;
	struct PLTM_CacheQueue *q;
	struct PLTM_Glyph *g;

	if (s_sendPacketFn == 0)
		return 0;
	if (font == 0)
		return 0;
	q = &font->queue;
	if (font->closing != 0)
		return 0;

	if (count < 0)
		count = str->count;
	if (str->count < start + count)
		count = str->count - start;
	g = str->glyphs + start;
	if (count == 1 && g->code == 0) {
		slot->tex = 0;
		return 1;
	}
	for (i = 0; i < count; i++) {
		unsigned short code;
		int adv, pad, w;
		struct PLTM_CacheItem *item;

		slot->tex = 0;
		code = g->code;
		adv = (code >= 256) ? fullW : halfW;
		if (str->spacing)
			adv = fullW;
		if (i == 0)
			pad = 0;
		else
			pad = adv >> 1;

		if (g->index < 0) {
			struct PLTM_Glyph key;

			key.code = code;
			key.charSize = g->charSize;
			g->index = ResolveGlyphIndex(q, &key);
			if (g->index < 0)
				goto missing;
		}
		{
			struct PLTM_CacheItem *chk = &q->itemArrayBase[g->index];

			if (g->charSize != chk->charSize || g->code != chk->code) {
				g->index = -1;
				allOk = 0;
			}
		}
		if (g->index < 0)
			goto missing;
		item = &q->itemArrayBase[g->index];
		switch (item->mode) {
		case PLTM_CACHE_FREE:
		case PLTM_CACHE_QUEUED:
		case PLTM_CACHE_OUTLINE:
			allOk = 0;
			break;
		case PLTM_CACHE_OUTLINE_DONE:
		case PLTM_CACHE_BITMAP_QUEUED:
		case PLTM_CACHE_BITMAP:
		case PLTM_CACHE_RASTERIZED:
		case PLTM_CACHE_ALLOCATED:
			allOk = 0;
			w = item->metrics.horiAdvance;
			pen += (long long)(adv + w);
			break;
		case PLTM_CACHE_WAIT:
			if (item->resource == 0) {
				g->index = -1;
				allOk = 0;
			}
			break;
		case PLTM_CACHE_READY:
			if (item->code == 0) {
				item->used = 0;
				break;
			}
			if (item->resource == 0) {
				g->index = -1;
				allOk = 0;
				break;
			}
			if (wasReady == 0) {
				ClipQuadColorsGS((unsigned char *)slot, (const unsigned char *)color, item->gradTop,
				    item->gradBottom);
				EmitGlyph(font, item, (long long)(pad + (int)pen + item->bearingX),
				    (long long)((int)baseline - item->bearingY), slot);
			}
			slot++;
			w = item->metrics.horiAdvance;
			item->used = 0;
			pen += (long long)(adv + w);
			drawn++;
			break;
		default:
			break;
		}
		g++;
		continue;
missing:
		allOk = 0;
		{
			int idx = RequestGlyph(font, q, g->code, g->charSize, s_drawOpts, 0);

			g->index = idx;
			if (idx >= 0) {
				struct PLTM_CacheItem *it;

				it = &q->itemArrayBase[idx];
				it->used = 0;
				it->glyphFlags = g->flags;
			}
		}
		g++;
	}
	str->ready = allOk;
	return drawn;
}

/* Multi-line PLTM_DrawStringGM. The "all ready" flag is str->readyML. Line
   breaks use the three PLTM_Font callbacks: +376 forced break (pen back to the
   start, y += lineHeight), +372 no break before this character, +368 break
   allowed after it; the break point is (slot, index, pen). When the pen
   passes maxWidth, the quads emitted since the break point are moved back by
   dx = brkPen and dy = lineHeight (26.6 converted to pixels). */
int PLTM_DrawStringGM_ML(void *font_, int halfW, int fullW, int maxWidth, int unused5, int lineHeight,
    long long penStart, long long y, int count, int start, struct PLTM_String *str, const struct PltmColor *color,
    struct PLTM_DrawSlot *slot)
{
	struct PLTM_Font *font = font_;
	struct PLTM_CacheQueue *q;
	int drawn = 0;
	int allOk = 1;
	int wasReady = str->readyML;
	struct PLTM_DrawSlot *brkSlot = 0;
	int brkIndex = 0;
	int drawable;
	long long brkPen;
	int canBreak;
	int nline;
	int quarter;
	int i;
	long long pen = penStart;
	struct PLTM_Glyph *g;

	(void)unused5;

	if (s_sendPacketFn == 0)
		return 0;
	if (font == 0)
		return 0;
	if (maxWidth == 0)
		return 0;
	if (font->closing != 0)
		return 0;
	q = &font->queue;
	if (count < 0)
		count = str->count;
	if (str->count < start + count)
		count = str->count - start;
	brkPen = pen;
	canBreak = 0;
	g = str->glyphs + start;
	if (count == 1 && g->code == 0) {
		slot->tex = 0;
		return 1;
	}
	nline = 0;
	for (i = 0; i < count; i++, g++) {
		unsigned short code;
		int adv, pad, w;
		struct PLTM_CacheItem *item;

		quarter = lineHeight >> 2;
		slot->tex = 0;
		drawable = 0;
		code = g->code;
		adv = (code >= 256) ? fullW : halfW;
		if (str->spacing)
			adv = fullW;
		pad = adv >> 1;
		if (nline == 0)
			pad = 0;

		if (g->index < 0) {
			struct PLTM_Glyph key;

			key.code = code;
			key.charSize = g->charSize;
			g->index = ResolveGlyphIndex(q, &key);
			if (g->index < 0)
				goto missing;
		}
		{
			struct PLTM_CacheItem *chk = &q->itemArrayBase[g->index];

			if (g->charSize != chk->charSize || g->code != chk->code) {
				g->index = -1;
				allOk = 0;
			}
		}
		if (g->index < 0)
			goto missing;
		item = &q->itemArrayBase[g->index];
		switch (item->mode) {
		case PLTM_CACHE_FREE:
		case PLTM_CACHE_QUEUED:
		case PLTM_CACHE_OUTLINE:
			allOk = 0;
			break;
		case PLTM_CACHE_OUTLINE_DONE:
		case PLTM_CACHE_BITMAP_QUEUED:
		case PLTM_CACHE_BITMAP:
		case PLTM_CACHE_RASTERIZED:
		case PLTM_CACHE_ALLOCATED:
			allOk = 0;
			break;
		case PLTM_CACHE_WAIT:
			if (item->resource == 0) {
				g->index = -1;
				allOk = 0;
			}
			break;
		case PLTM_CACHE_READY:
			if (item->code == 0) {
				item->tex = 0;
				item->used = 0;
				break;
			}
			if (item->resource == 0) {
				g->index = -1;
				allOk = 0;
				break;
			}
			drawable = 1;
			if (wasReady == 0) {
				ClipQuadColorsGS((unsigned char *)slot, (const unsigned char *)color, item->gradTop,
				    item->gradBottom);
				if (EmitGlyph(font, item, (long long)(pad + (int)pen + item->bearingX),
				        (long long)((int)y - item->bearingY), slot)
				    == -1)
					drawable = 0;
			}
			item->used = 0;
			break;
		default:
			break;
		}
		w = adv + item->metrics.horiAdvance;
		if (drawable == 0)
			continue;

		if (font->callbacks.isLineEnd != 0 && font->callbacks.isLineEnd(g->code) != 0) {
			canBreak = 0;
			pen = penStart;
			brkPen = 0;
			brkSlot = 0;
			y += lineHeight;
			w = 0;
		} else if (font->callbacks.cannotStartLine != 0 && font->callbacks.cannotStartLine(g->code) != 0) {
			canBreak = 1;
		} else if (canBreak
		    || (font->callbacks.canStartLine != 0 && font->callbacks.canStartLine(g->code) != 0)) {
			canBreak = 0;
			brkSlot = slot;
			brkIndex = i;
			brkPen = pen;
		}
		pen += (long long)w;

		nline++;
		if (!(pen < (long long)maxWidth)) {
			if (brkSlot != 0) {
				struct PLTM_DrawSlot *p = brkSlot;
				int k;

				y += lineHeight;
				for (k = 0; k <= i - brkIndex; k++) {
					float dx = (float)(brkPen >> 2) * 0.0625f;
					float dy = (float)quarter * 0.0625f;
					struct gmVertexPNCT *v = p->v;
					int j;

					for (j = 0; j < 4; j++) {
						v->x -= dx;
						v->y -= dy;
						v++;
					}
					p++;
				}
			}
			pen -= brkPen;
			brkPen = 0;
			brkSlot = 0;
			nline = 0;
		}
		slot++;
		drawn++;
		continue;
missing:
		allOk = 0;
		{
			int idx = RequestGlyph(font, q, g->code, g->charSize, s_drawOpts, 0);

			g->index = idx;
			if (idx >= 0) {
				struct PLTM_CacheItem *it;

				it = &q->itemArrayBase[idx];
				it->used = 0;
				it->glyphFlags = g->flags;
			}
		}
	}
	str->readyML = allOk;
	return drawn;
}

/* Renders a laid-out string into a new PSMT8 texture, truncated with the
   ellipsis (_pltmOmmiter) when it does not fit in maxW. The ellipsis glyphs
   are laid out with up to 100 retries, sleeping with the mutex released.
   Errors: -101 (bad string or mutex busy), -102 (allocation), -201, -203,
   -204, -205, -207; after the texture exists every error releases it and
   sets *texOut = 0. */
int PLTM_CreateImageString(void *font_, int halfW, int fullW, int maxW, struct PLTM_String *str, TmHandle tmMgr,
    TmHandle clut, int *texOut)
{
	struct PLTM_Font *font = font_;
	struct PLTM_String tmp;
	struct PLTM_String *t;
	unsigned short u16[8];
	int examined;
	const char *cursor;
	unsigned short *outIter;
	unsigned short *base;
	int retryExamined;
	int xoff;
	int ret;
	int th;
	int retry;
	int fnt;
	struct PLTM_CacheQueue *q;
	struct PLTM_Glyph *g;
	void *buf;
	char trunc;
	int tw, w, size, i, n;
	int allocSize;
	int empty;

	ret = PrepareGlyphs(font, halfW, fullW, str, &examined);
	if (ret == -1)
		return -201;
	if (ret < 0)
		return ret;
	if (str->ready == 0)
		return -101;
	q = &font->queue;
	if (pthread_mutex_trylock(&q->mutex) == 16)
		return -101;
	if (GetTextRectImpl(font, halfW, fullW, str) < 0) {
		pthread_mutex_unlock(&q->mutex);
		return -101;
	}
	if (str->rectFinal == 0) {
		pthread_mutex_unlock(&q->mutex);
		return -203;
	}
	if (0xa000 < maxW)
		maxW = 0xa000;
	if (maxW == 0)
		maxW = 0xa000;
	w = str->w;
	trunc = 0;
	if (maxW < w)
		trunc = 1;
	if (maxW < w)
		tw = (maxW + 64) >> 6;
	else
		tw = (w + 64) >> 6;
	tw += 2;
	th = ((str->h + 64) >> 6) + 2;
	size = tw * th;
	buf = s_allocFn(size);
	if (buf == 0) {
		pthread_mutex_unlock(&q->mutex);
		return -102;
	}
	memset(buf, 0, size);
	*texOut = tmCreateTexture(tmMgr, tw, th, PS2_GS_PSMT8);
	if (*texOut < 0) {
		s_freeFn(buf);
		pthread_mutex_unlock(&q->mutex);
		return *texOut;
	}
	if (clut == 0)
		clut = s_defaultClut;

	if (trunc) {
		retry = 100;
		base = u16;
		t = &tmp;
		xoff = 0;
		fnt = str->glyphs->charSize;
		cursor = _pltmOmmiter;
		if (cursor == 0) {
			t->glyphs = 0;
		} else {
			t->count = Utf8Len(cursor);
			empty = t->count == 0;
			t->bottom = 0;
			t->top = 0;
			n = t->count;
			if (empty)
				allocSize = (n + 1) * 20;
			else
				allocSize = n * 20;
			t->glyphs = s_allocFn(allocSize);
			g = t->glyphs;
			if (empty) {
				g->code = 0;
				g->charSize = fnt;
				g->index = -1;
				g->flags = 0;
				t->count = t->count + 1;
			} else {
				for (i = 0; i < t->count; i++) {
					unsigned int decoded;

					decoded = DecodeUtf8Step(&cursor);
					outIter = base;
					_sceCccEncodeUTF16(&outIter, decoded);
					if (outIter - base < 2) {
						g->code = u16[0];
						g->charSize = fnt;
						g->index = -1;
						g->flags = 0;
						g++;
					}
				}
			}
			t->rectFinal = 0;
			t->ready = 0;
			t->readyML = 0;
			t->hasRect = 0;
			t->spacing = 0;
			t->mode = 0;
		}
		pthread_mutex_unlock(&q->mutex);
		while (retry > 0) {
			int rc = PrepareGlyphs(font, halfW, fullW, &tmp, &retryExamined);

			if (rc >= 0)
				break;
			if (rc == -1) {
				s_freeFn(buf);
				tmRelease(*texOut);
				*texOut = 0;
				return -204;
			}
			usleep(30);
			retry--;
		}
		pthread_mutex_lock(&q->mutex);
		if (retry < 0) {
			pthread_mutex_unlock(&q->mutex);
			s_freeFn(buf);
			tmRelease(*texOut);
			*texOut = 0;
			return -205;
		}
		if (tmp.ready == 0) {
			pthread_mutex_unlock(&q->mutex);
			s_freeFn(buf);
			tmRelease(*texOut);
			*texOut = 0;
			return -101;
		}
		if (GetTextRectImpl(font, halfW, fullW, &tmp) < 0) {
			pthread_mutex_unlock(&q->mutex);
			s_freeFn(buf);
			tmRelease(*texOut);
			*texOut = 0;
			return -207;
		}
		RenderText(q, halfW, fullW, maxW - tmp.w, str, buf, 0, tw, th, &xoff);
		RenderText(q, halfW, fullW, 0, &tmp, buf, xoff, tw, th, 0);
		if (tmp.glyphs != 0) {
			s_freeFn(tmp.glyphs);
			tmp.count = 0;
			tmp.bottom = 0;
			tmp.top = 0;
		}
	} else {
		ret = RenderText(q, halfW, fullW, maxW, str, buf, 0, tw, th, 0);
	}
	tmUpdateTexture(*texOut, tw, th, 19, buf);
	s_freeFn(buf);
	tmSetClut(*texOut, clut);
	pthread_mutex_unlock(&q->mutex);
	return ret;
}

/* Given [pen, limit] in pen coordinates (64 bit), finds the first and last
   character inside (*outStart / *outEnd, -1 if none) and the offset of the
   first visible one in *outPos. Returns 0 if a glyph was not ready, else 1. */
int PLTM_GetVisibleArea(void *font_, int halfW, int fullW, long long penStart, long long limit, struct PLTM_String *str,
    long long *outPos, int *outStart, int *outEnd)
{
	struct PLTM_Font *font = font_;
	int i;
	int ret = 1;
	int done;
	long long pen = penStart;
	int count;
	struct PLTM_Glyph *g;
	struct PLTM_CacheQueue *q;

	done = 0;
	*outEnd = -1;
	*outStart = -1;
	*outPos = pen;
	if (font == 0)
		return 0;
	q = &font->queue;
	if (font->closing != 0)
		return 0;
	i = 0;
	count = str->count;
	g = str->glyphs;
	while (i < count && !done) {
		unsigned short code;
		int adv;
		int mode;
		struct PLTM_CacheItem *item;

		code = g->code;
		adv = (code >= 256) ? fullW : halfW;
		if (str->spacing)
			adv = fullW;
		if (g->index < 0) {
			struct PLTM_Glyph key;

			key.code = code;
			key.charSize = g->charSize;
			g->index = ResolveGlyphIndex(q, &key);
			if (g->index < 0)
				goto missing;
		}
		{
			struct PLTM_CacheItem *chk = &q->itemArrayBase[g->index];

			if (g->charSize != chk->charSize || g->code != chk->code)
				g->index = -1;
		}
		if (g->index < 0)
			goto missing;
		item = &q->itemArrayBase[g->index];
		mode = item->mode;
		if (mode >= PLTM_CACHE_FREE) {
			if (mode <= PLTM_CACHE_OUTLINE) {
				ret = 0;
			} else if (mode <= PLTM_CACHE_READY) {
				long long next;
				int w;

				next = pen + adv + item->metrics.horiAdvance;
				if (next >= 0 && limit >= pen) {
					if (*outStart == -1) {
						*outStart = i;
						*outPos = pen - *outPos;
					}
					*outEnd = i;
				} else {
					if (*outEnd != -1)
						done = 1;
				}
				w = adv + item->metrics.horiAdvance;
				item->used = 0;
				pen += (long long)w;
			}
		}
		goto next;
missing:
		{
			int idx = RequestGlyph(font, q, g->code, g->charSize, s_drawOpts, 0);

			g->index = idx;
			if (idx >= 0) {
				struct PLTM_CacheItem *it;

				it = &q->itemArrayBase[idx];
				it->used = 0;
				it->glyphFlags = g->flags;
			} else {
				ret = 0;
			}
		}
next:
		i++;
		g++;
	}
	return ret;
}

/* How many characters fit in maxWidth; *outWidth gets the width before the
   first one that does not fit. Returns -1 if a glyph was not ready. */
int PLTM_CountVisibleChar(void *font_, int halfW, int fullW, int maxWidth, struct PLTM_String *str, int *outWidth)
{
	struct PLTM_Font *font = font_;
	int i;
	int err;
	long long acc;
	int stop;
	int count;
	struct PLTM_Glyph *g;
	struct PLTM_CacheQueue *q;

	stop = 0;
	err = 0;
	acc = 0;
	*outWidth = 0;
	q = &font->queue;
	if (font == 0)
		return 0;
	i = 0;
	count = str->count;
	g = str->glyphs;
	while (!stop && i < count) {
		unsigned short code;
		int adv;
		int mode;
		struct PLTM_CacheItem *item;

		code = g->code;
		adv = (code >= 256) ? fullW : halfW;
		if (g->index < 0) {
			struct PLTM_Glyph key;

			key.code = code;
			key.charSize = g->charSize;
			g->index = ResolveGlyphIndex(q, &key);
			if (g->index < 0)
				goto missing;
		}
		{
			struct PLTM_CacheItem *chk = &q->itemArrayBase[g->index];

			if (g->charSize != chk->charSize || g->code != chk->code)
				g->index = -1;
		}
		if (g->index < 0)
			goto missing;
		item = &q->itemArrayBase[g->index];
		mode = item->mode;
		if (mode >= PLTM_CACHE_FREE) {
			if (mode <= PLTM_CACHE_OUTLINE) {
				err = -1;
			} else if (mode <= PLTM_CACHE_READY) {
				int w;

				*outWidth = adv + (int)acc;
				w = adv + item->metrics.horiAdvance;
				acc += (long long)w;
				if (acc >= maxWidth) {
					stop = 1;
					i--;
				} else {
					item->used = 0;
				}
			}
		}
		goto next;
missing:
		{
			int idx = RequestGlyph(font, q, g->code, g->charSize, s_drawOpts, 0);

			g->index = idx;
			if (idx >= 0) {
				struct PLTM_CacheItem *it;

				it = &q->itemArrayBase[idx];
				it->used = 0;
				it->glyphFlags = g->flags;
			}
		}
		err = -1;
next:
		i++;
		g++;
	}
	if (err == -1)
		return err;
	return i;
}

/* Common part of PLTM_OpenS/PLTM_OpenI: creates a 400-byte PLTM_Font for the
   full-width (fullPath) and half-width (halfPath) faces: the two request queues,
   the glyph cache, mutexes and condvars, the two worker threads (lowest
   SCHED_RR priority, at least 8 KB of stack), FreeType and the faces, the
   defaults (72 dpi, hinting per _pltmDBG_HintSwitch) and the two buffers of
   pktPerPool packets (112 and 368 bytes) from the pool. Returns the font, or 0;
   on failure the resources already taken are not released. */
static void *OpenCommon(const char *fullPath, const char *halfPath)
{
	struct PLTM_Font *font;
	pthread_attr_t attr;
	unsigned int stacksize;
	struct sched_param param;
	int prio;
	int n, templateBytes, packetBytes, size, i;
	int err;
	char *p;

	font = s_allocFn(400);
	if (font == 0)
		return 0;
	font->closing = 0;
	font->unk_13C = 0;
	if (InitReqQueue(&font->outlineQueue, s_outlineQueueSize) < 0)
		return 0;
	if (InitReqQueue(&font->bitmapQueue, s_bitmapQueueSize) < 0) {
		s_freeFn(font->outlineQueue.items);
		return 0;
	}
	font->bitmapQueue.nonBlocking = 0;
	if (InitCacheQueue(&font->queue) < 0) {
		s_freeFn(font->bitmapQueue.items);
		s_freeFn(font->outlineQueue.items);
		return 0;
	}
	pthread_mutex_init(&font->mutex, 0);
	pthread_cond_init(&font->cond, 0);
	pthread_attr_init(&attr);
	prio = sched_get_priority_min(2);
	pthread_attr_getstacksize(&attr, &stacksize);
	param.sched_priority = prio;
	pthread_attr_setschedparam(&attr, &param);
	if (stacksize < 8192)
		pthread_attr_setstacksize(&attr, 8192);
	pthread_create(&font->outlineThread, &attr, (void *(*)(void *))OutlineThread, font);
	pthread_create(&font->bitmapThread, &attr, (void *(*)(void *))BitmapThread, font);
	pthread_attr_destroy(&attr);
	err = FT_Init_FreeType(&font->ftLibrary);
	if (err != 0) {
		PLTM_DPRINT((0, "ft2Error=%d\n", err));
	}
	PLTM_DPRINT((0, "aFontfile=%s\n", fullPath));
	if (FT_New_Face(font->ftLibrary, fullPath, 0, &font->ftFaceFull) != 0
	    || FT_New_Face(font->ftLibrary, halfPath, 0, &font->ftFaceHalf) != 0) {
		s_freeFn(font);
		return 0;
	}
	font->loadFlags = _pltmDBG_HintSwitch ? 0 : 2;
	font->renderMode = 0;
	font->resX = 72;
	font->resY = 72;
	font->curPool = 0;
	font->pktUsed = 0;
	font->pktPerPool = s_polygonPoolSize;
	font->numLoaded = 0;
	memset(&font->callbacks, 0, sizeof font->callbacks);
	n = font->pktPerPool;
	if (n > 0) {
		templateBytes = n * 112;
		packetBytes = n * 368;
		size = (templateBytes + packetBytes) * 2;
		p = PoolAlloc(size);
		if (p == 0) {
			s_freeFn(font);
			return 0;
		}
		font->pktTemplate = (struct PLTM_Pkt112 *)p;
		font->pktTemplate2 = (struct PLTM_Pkt112 *)(p + templateBytes);
		font->pools[0] = (struct PLTM_GlyphPacket *)((char *)font->pktTemplate2 + templateBytes);
		font->pools[1] = (struct PLTM_GlyphPacket *)((char *)font->pools[0] + packetBytes);
		if (font->pktPerPool > 0) {
			BuildPacketTemplate(font->pools[0]);
			*font->pktTemplate2 = *font->pktTemplate;
			*font->pools[1] = *font->pools[0];
			for (i = 1; i < font->pktPerPool; i++) {
				font->pktTemplate[i] = font->pktTemplate2[i] = font->pktTemplate[0];
				font->pools[0][i] = font->pools[1][i] = font->pools[0][0];
			}
		}
	}
	return font;
}

/* Measures a laid-out string, requesting the missing glyphs. Ready glyphs
   with a non-zero code advance the pen; the first glyph's bearing is the
   origin and top/bottom keep the vertical extent. The rectangle goes to
   the string (+28 origin, +36 width, +32 ascent, +40 height, +44 valid) and
   rectFinal is set when every glyph was ready. Returns 0, or -1 if a glyph was
   not ready or the font is missing or closing. */
static int GetTextRectImpl(struct PLTM_Font *font, int halfW, int fullW, struct PLTM_String *str)
{
	int err = 0;
	long long pen = 0;
	long long end = 0;
	int first = 1;
	long long origin;
	int i;
	int count;
	struct PLTM_CacheQueue *q = &font->queue;
	struct PLTM_Glyph *g;

	if (font == 0)
		return -1;
	if (font->closing != 0)
		return -1;
	if (str->rectFinal != 0)
		return 0;
	origin = 0;
	count = str->count;
	g = str->glyphs;
	for (i = 0; i < count; i++, g++) {
		unsigned short code = g->code;
		int adv = (code >= 256) ? fullW : halfW;
		struct PLTM_CacheItem *item;

		if (g->index < 0) {
			struct PLTM_Glyph key;

			key.code = code;
			key.charSize = g->charSize;
			g->index = ResolveGlyphIndex(q, &key);
			if (g->index < 0)
				goto missing;
		}
		{
			struct PLTM_CacheItem *chk = &q->itemArrayBase[g->index];

			if (g->charSize != chk->charSize || g->code != chk->code)
				g->index = -1;
		}
		if (g->index < 0)
			goto missing;
		item = &q->itemArrayBase[g->index];
		switch (item->mode) {
		case PLTM_CACHE_FREE:
		case PLTM_CACHE_QUEUED:
		case PLTM_CACHE_OUTLINE:
			err = -1;
			break;
		case PLTM_CACHE_OUTLINE_DONE:
		case PLTM_CACHE_BITMAP_QUEUED:
		case PLTM_CACHE_BITMAP:
		case PLTM_CACHE_RASTERIZED:
		case PLTM_CACHE_ALLOCATED:
		case PLTM_CACHE_READY:
			if (item->code != 0) {
				if (first) {
					first = 0;
					origin = item->metrics.horiBearingX;
				}
				end = pen + adv + item->metrics.horiAdvance;
				pen += (long long)(adv + item->metrics.horiAdvance);
			}
			if (str->top < item->top)
				str->top = item->top;
			if (str->bottom > item->bottom)
				str->bottom = item->bottom;
			item->used = 0;
			break;
		default:
			err = -1;
			break;
		}
		continue;
missing:
		{
			int idx = RequestGlyph(font, q, g->code, g->charSize, s_drawOpts, 0);

			g->index = idx;
			if (idx >= 0) {
				struct PLTM_CacheItem *it;

				it = &q->itemArrayBase[idx];
				it->used = 0;
				it->glyphFlags = g->flags;
			}
		}
		err = -1;
	}
	str->x = (int)origin;
	str->w = (int)(end - origin);
	str->y = str->top;
	str->h = str->top - str->bottom;
	if (err != -1)
		str->rectFinal = 1;
	str->hasRect = 1;
	return err;
}

/* GetTextRectImpl with the line breaking of PLTM_DrawStringGM_ML: when the
   pen passes maxWidth it goes back to the saved break point and continues on
   the next line. +44 gets the number of line breaks. */
static int GetTextRectImplML(struct PLTM_Font *font, int halfW, int fullW, int maxWidth, int lineHeight,
    struct PLTM_String *str)
{
	int err = 0;
	int i, count, lines, brkI;
	struct PLTM_CacheQueue *q;
	struct PLTM_Glyph *brkG;
	long long pen, y;
	int nline;
	int canBreak = 0;
	int armed;
	long long minY, sMaxY, sMinY, sMinX, sMaxX;
	struct PLTM_Glyph *g;
	long long maxX, maxY, minX;

	if (font == 0)
		return -1;
	if (font->closing != 0)
		return -1;
	if (maxWidth == 0)
		return -1;
	if (str->rectFinal != 0)
		return 0;
	armed = 0;
	y = 0;
	pen = 0;
	minY = 0;
	minX = 0;
	sMaxY = 0;
	sMinY = 0;
	sMinX = 0;
	sMaxX = 0;
	brkI = 0;
	brkG = 0;
	nline = 0;
	i = 0;
	lines = 0;
	maxY = 0;
	count = str->count;
	g = str->glyphs;
	maxX = 0;
	for (; i < count; i++, g++) {
		unsigned short code;
		int adv;
		struct PLTM_CacheItem *item;
		long long t;

top:
		q = &font->queue;
		code = g->code;
		adv = (code >= 256) ? fullW : halfW;
		if (g->index < 0) {
			struct PLTM_Glyph key;

			key.code = code;
			key.charSize = g->charSize;
			g->index = ResolveGlyphIndex(q, &key);
			if (g->index < 0)
				goto missing;
		}
		{
			struct PLTM_CacheItem *chk = &font->queue.itemArrayBase[g->index];

			if (g->charSize != chk->charSize || g->code != chk->code)
				g->index = -1;
		}
		if (g->index < 0)
			goto missing;
		item = &q->itemArrayBase[g->index];
		switch (item->mode) {
		case PLTM_CACHE_FREE:
		case PLTM_CACHE_QUEUED:
		case PLTM_CACHE_OUTLINE:
			err = -1;
			break;
		case PLTM_CACHE_OUTLINE_DONE:
		case PLTM_CACHE_BITMAP_QUEUED:
		case PLTM_CACHE_BITMAP:
		case PLTM_CACHE_RASTERIZED:
		case PLTM_CACHE_ALLOCATED:
		case PLTM_CACHE_READY:
			if (font->callbacks.isLineEnd != 0 && font->callbacks.isLineEnd(g->code) != 0) {
				armed = 0;
				canBreak = 0;
				pen = 0;
				y += lineHeight;
				nline = 0;
				sMaxY = maxY;
				sMinX = minX;
				sMaxX = maxX;
				lines++;
				sMinY = minY;
				brkI = i + 1;
				brkG = g + 1;
				continue;
			}
			t = item->top - y;
			if (maxY < t)
				maxY = t;
			t = item->bottom - y;
			if (t < minY)
				minY = t;
			t = pen + item->metrics.horiBearingX;
			if (t < minX)
				minX = t;
			if (nline > 0) {
				if (font->callbacks.cannotStartLine != 0
				    && font->callbacks.cannotStartLine(g->code) != 0) {
					armed = 1;
				} else if (armed
				    || (font->callbacks.canStartLine != 0
				        && font->callbacks.canStartLine(g->code) != 0)) {
					armed = 0;
					canBreak = 1;
					sMaxY = maxY;
					sMinY = minY;
					sMinX = minX;
					sMaxX = maxX;
					brkG = g;
					brkI = i;
				}
			}
			{
				int right = item->metrics.horiBearingX + (int)pen + item->metrics.width;

				if (maxX < right)
					maxX = right;
			}
			pen += (long long)(adv + item->metrics.horiAdvance);
			if (pen >= maxWidth) {
				if (canBreak && brkG != 0) {
					g = brkG;
					maxY = sMaxY;
					minY = sMinY;
					minX = sMinX;
					maxX = sMaxX;
					armed = 0;
					canBreak = 0;
					brkG = 0;
					i = brkI;
					pen = 0;
					y += lineHeight;
					nline = 0;
					lines++;
					goto top;
				}
				nline++;
			} else
				nline++;
			item->used = 0;
			break;
		default:
			err = -1;
			break;
		}
		continue;
missing:
		{
			int idx = RequestGlyph(font, q, g->code, g->charSize, s_drawOpts, 0);

			g->index = idx;
			if (idx >= 0) {
				struct PLTM_CacheItem *it;

				it = &q->itemArrayBase[idx];
				it->used = 0;
				it->glyphFlags = g->flags;
			}
		}
		err = -1;
	}
	str->top = (int)maxY;
	str->bottom = (int)minY;
	str->x = (int)minX;
	str->y = (int)maxY;
	str->w = (int)maxX - (int)minX;
	str->h = (int)maxY - (int)minY;
	if (err != -1)
		str->rectFinal = 1;
	str->hasRect = lines;
	return err;
}

/* Producer of the outline queue: takes the lock with trylock, waits for a free
   entry, gets a cache slot from ReclaimCacheSlot (retrying 10 times when it
   returns -2 with a request pending), marks it PLTM_CACHE_QUEUED with (code, font) and
   queues the request. Returns the slot index, or -1. The last two parameters
   are unused. */
static int RequestGlyph(struct PLTM_Font *font, struct PLTM_CacheQueue *q, int code, int charSize, void *opts, int flag)
{
	struct PLTM_ReqQueue *rq = &font->outlineQueue;
	struct PLTM_Request *r;
	int idx;
	int i;
	int err;

	(void)opts;
	(void)flag;
	PLTM_DPRINT((0, "OutlineQSize=%d, BitmapQSize=%d, BitmapCount=%d\n", s_outlineQueueSize, s_bitmapQueueSize,
	    s_cacheSize));
	if (pthread_mutex_trylock(&rq->mutex) == 16)
		return -1;
	if (rq->queued == rq->count) {
		if (rq->nonBlocking != 0) {
			pthread_mutex_unlock(&rq->mutex);
			return -1;
		}
		while (rq->queued == rq->count && rq->closing == 0 && rq->abort == 0)
			pthread_cond_wait(&rq->notFull, &rq->mutex);
	}
	if (rq->closing || rq->abort) {
		pthread_mutex_unlock(&rq->mutex);
		return -1;
	}
	idx = ReclaimCacheSlot(font, q, 0);
	if (idx == -2) {
		if (s_flagPending) {
			i = 0;
			do {
				idx = ReclaimCacheSlot(font, q, 1);
				if (idx == -2)
					break;
				i++;
			} while (i < 10);
		}
		if (idx == -2) {
			pthread_mutex_unlock(&rq->mutex);
			return -1;
		}
	}
	if (idx == -1) {
		pthread_mutex_unlock(&rq->mutex);
		return -1;
	}
	q->itemArrayBase[idx].mode = PLTM_CACHE_QUEUED;
	q->itemArrayBase[idx].code = code;
	q->itemArrayBase[idx].charSize = charSize;
	r = &rq->items[rq->tail];
	r->code = code;
	r->charSize = charSize;
	r->index = idx;
	rq->queued++;
	rq->tail = (rq->tail + 1) % rq->count;
	if (rq->queued == 1) {
		PLTM_DPRINT((0,
		    "====_PLTM_AddOutlineQueue: Outline Request"
		    "(send signal )====\n"));
		err = pthread_cond_signal(&rq->notEmpty);
		PLTM_DPRINT((0, "pthread_cond_signal: %d\n", err));
	}
	pthread_mutex_unlock(&rq->mutex);
	PLTM_DPRINT((0, "====_PLTM_AddOutlineQueue: Outline Request(done)====\n"));
	return idx;
}

/* Producer of the bitmap queue: waits while the queue is full (leaving if
   the thread is closing), writes (code, charSize) of the cache entry, marks it
   PLTM_CACHE_BITMAP_QUEUED and wakes the consumer. Returns 1 if queued, -1 otherwise. */
static int PushBitmapRequest(struct PLTM_Font *font, int index, struct PLTM_CacheItem *item)
{
	struct PLTM_ReqQueue *q = &font->bitmapQueue;
	struct PLTM_Request *r;

	PLTM_DPRINT((0, "====Bitmap Create Request====\n"));
	pthread_mutex_lock(&q->mutex);
	if (q->queued == q->count) {
		if (q->nonBlocking != 0) {
			pthread_mutex_unlock(&q->mutex);
			return -1;
		}
		if (q->closing == 0 && q->abort == 0) {
			do {
				pthread_mutex_unlock(&font->queue.mutex);
				pthread_cond_wait(&q->notFull, &q->mutex);
				pthread_mutex_lock(&font->queue.mutex);
				if (q->queued != q->count)
					break;
			} while (q->closing == 0 && q->abort == 0);
		}
	}
	if (q->closing || q->abort) {
		pthread_mutex_unlock(&q->mutex);
		return -1;
	}
	r = &q->items[q->tail];
	r->code = item->code;
	r->charSize = item->charSize;
	r->index = index;
	q->queued++;
	q->tail = (q->tail + 1) % q->count;
	item->mode = PLTM_CACHE_BITMAP_QUEUED;
	if (q->queued > 0)
		pthread_cond_signal(&q->notEmpty);
	pthread_mutex_unlock(&q->mutex);
	PLTM_DPRINT((0, "====Bitmap Create Request end====\n"));
	return 1;
}

/* Resolves every glyph of a string in the cache, requesting the missing
   ones; clears str->ready/readyML when a glyph is not available. Returns 0
   if all are ready, -101 if some are pending, -1 if the font is missing or
   closing. The fifth parameter gets the number of glyphs examined; the two
   int parameters are unused. */
static int PrepareGlyphs(struct PLTM_Font *font, int unusedA, int unusedB, struct PLTM_String *str, int *examined)
{
	struct PLTM_CacheQueue *q;
	struct PLTM_Glyph *g;
	struct PLTM_CacheItem *item;
	struct PLTM_CacheItem *cached;
	struct PLTM_CacheItem *requested;
	struct PLTM_Glyph key;
	int ret = 0;
	int i;
	int n;
	int idx;

	if (font == 0)
		return -1;
	if (font->closing != 0)
		return -1;
	q = &font->queue;
	str->ready = 1;
	g = str->glyphs;
	n = str->count;
	*examined = 0;
	for (i = 0; i < n; i++, g++, *examined = *examined + 1) {
		int code = g->code;

		if (g->index < 0) {
			key.code = code;
			key.charSize = g->charSize;
			g->index = ResolveGlyphIndex(q, &key);
			if (g->index < 0)
				goto request;
		}
		item = q->itemArrayBase + g->index;
		if (g->charSize != item->charSize || g->code != item->code) {
			str->ready = 0;
			str->readyML = 0;
			g->index = -1;
		}
		if (g->index < 0)
			goto request;
		cached = q->itemArrayBase + g->index;
		switch (cached->mode) {
		case PLTM_CACHE_FREE:
		case PLTM_CACHE_QUEUED:
		case PLTM_CACHE_OUTLINE:
		case PLTM_CACHE_OUTLINE_DONE:
			ret = -101;
			str->ready = 0;
			str->readyML = 0;
			break;
		case PLTM_CACHE_BITMAP_QUEUED:
		case PLTM_CACHE_BITMAP:
		case PLTM_CACHE_RASTERIZED:
		case PLTM_CACHE_ALLOCATED:
			ret = -101;
			str->ready = 0;
			str->readyML = 0;
			break;
		case PLTM_CACHE_READY:
			if (cached->code == 0) {
				cached->used = 0;
				break;
			}
			cached->used = 0;
			break;
		}
		continue;
request:
		str->ready = 0;
		str->readyML = 0;
		idx = RequestGlyph(font, q, g->code, g->charSize, s_drawOpts, 0);
		g->index = idx;
		if (idx >= 0) {
			requested = q->itemArrayBase + idx;
			requested->used = 0;
			requested->glyphFlags = g->flags;
		} else {
			ret = -101;
		}
	}
	return ret;
}

/* Draws the ready glyphs of a string into the 8-bit buffer of a
   PLTM_CreateImageString texture, with the PLTM_DrawString metrics. Small
   fonts (below _pltmSubPixelImageCopy_CharSize) use BlitTileBlend. Stops when
   the pen passes maxW (if maxW > 0). Returns 0, -209 if a glyph is not
   resolved, -208 if it is not ready. */
static int RenderText(struct PLTM_CacheQueue *q, int halfW, int fullW, int maxW, struct PLTM_String *str,
    unsigned char *buf, int xoff, int tw, int th, int *outX)
{
	int i;
	int stop;
	int pen = xoff - str->x;
	int baseline = str->top;
	struct PLTM_Glyph *g;

	if (maxW >= 0) {
		stop = 0;
		i = 0;
		g = str->glyphs;
		while (i < str->count) {
			void (*blit)(unsigned char *, int, int, int, int, const unsigned char *, int, int);
			int adv, pad, x, y;
			struct PLTM_CacheItem *item;

			blit = (g->charSize < _pltmSubPixelImageCopy_CharSize) ? BlitTileBlend : BlitTile;
			adv = (g->code >= 256) ? fullW : halfW;
			if (str->spacing)
				adv = fullW;
			pad = adv >> 1;
			if (i == 0)
				pad = 0;
			if (g->index >= 0) {
				item = &q->itemArrayBase[g->index];
				if (item->mode != PLTM_CACHE_READY)
					return -208;
				if (str->spacing != 0 && !IsHankakuCode(g->code)) {
					if (g->flags != 0 && str->mode != 0)
						x = pen
						    + (int)((float)(pad + item->bearingX + str->spacing
						                - item->metrics.horiAdvance)
						        * 0.75f);
					else
						x = pen + pad + item->bearingX
						    + (str->spacing - item->metrics.horiAdvance) / 2;
				} else {
					x = pen + pad + item->bearingX;
				}
				if (outX != 0)
					*outX = pen;
				y = baseline - item->bearingY;
				if (str->spacing != 0 && !IsHankakuCode(g->code)) {
					if (g->flags != 0 && str->mode != 0)
						pen += (int)((float)(adv + str->spacing) * 0.75f);
					else
						pen += adv + str->spacing;
				} else {
					pen += adv + item->metrics.horiAdvance;
				}
				if (maxW > 0 && maxW < pen)
					stop = 1;
				else
					blit(buf, tw, th, x, (y - 128) >> 6, (const unsigned char *)item->resource,
					    item->texWidth, item->texHeight);
			} else
				return -209;
			i++;
			g++;
			if (stop)
				break;
		}
	}
	return 0;
}

/* Vertical interpolation of the colors of a clipped quad: src is a 2x2 grid
   of RGBA colors (top row src[0..7], bottom row src[8..15]) and the two 8.8
   factors give the new rows. This one writes a 16-byte buffer, the GS variant
   writes the color field of the 4 packet vertices. */
static void ClipQuadColors(unsigned char *dst, const unsigned char *src, int f0, int f1)
{
	LerpColor4(dst, 0, src, 0, f0);
	LerpColor4(dst, 4, src, 4, f0);
	LerpColor4(dst, 8, src, 0, f1);
	LerpColor4(dst, 12, src, 4, f1);
}

static void ClipQuadColorsGS(unsigned char *dst, const unsigned char *src, int f0, int f1)
{
	LerpColor4(dst, 24, src, 0, f0);
	LerpColor4(dst, 60, src, 4, f0);
	LerpColor4(dst, 96, src, 0, f1);
	LerpColor4(dst, 132, src, 4, f1);
}

/* Allocates s_cacheSize free cache entries (code 0xffff, charSize -1) and
   initializes the mutex and the two condvars of the cache. */
static int InitCacheQueue(struct PLTM_CacheQueue *q)
{
	int i;

	q->count = s_cacheSize;
	q->itemArrayBase = s_allocFn(s_cacheSize * 136);
	if (q->itemArrayBase == 0) {
		PLTM_DPRINT((0, "====!!!!====!!!!====\n"));
		return -1;
	}
	for (i = 0; i < q->count; i++) {
		q->itemArrayBase[i].resource = 0;
		q->itemArrayBase[i].mode = PLTM_CACHE_FREE;
		q->itemArrayBase[i].used = 0;
		q->itemArrayBase[i].code = 0xffff;
		q->itemArrayBase[i].charSize = -1;
		q->itemArrayBase[i].tex = 0;
	}
	pthread_mutex_init(&q->mutex, 0);
	pthread_cond_init(&q->condA, 0);
	pthread_cond_init(&q->condB, 0);
	return 0;
}

/* Outline worker thread: takes requests from the outline queue, exits when
   the closing flag is set, rasterizes the glyph with FreeType under the font
   mutex (PLTM_CACHE_QUEUED -> OUTLINE -> OUTLINE_DONE) and queues it for the bitmap thread. */
static void *OutlineThread(struct PLTM_Font *font)
{
	struct PLTM_Request req;
	struct PLTM_CacheItem *item;

	nice(10);
	for (;;) {
		struct PLTM_ReqQueue *q = &font->outlineQueue;
		int err;

		PLTM_DPRINT((0, "====PT_Outline (-A)====\n"));
		err = pthread_mutex_lock(&q->mutex);
		if (err != 0) {
			PLTM_DPRINT((0, "PLTM: pthread_mutex_lock error in PT_Outline (%d)\n", err));
		}
		PLTM_DPRINT((0, "====PT_Outline (A)====\n"));
		while (q->queued == 0 && q->closing == 0) {
			PLTM_DPRINT((0, "==%s(SLEEP)==\n", "PT_Outline"));
			pthread_cond_wait(&q->notEmpty, &q->mutex);
			PLTM_DPRINT((0, "==%s(WAKEUP)==\n", "PT_Outline"));
		}
		PLTM_DPRINT((0, "====PT_Outline (B)====\n"));
		if (q->closing) {
			pthread_mutex_unlock(&q->mutex);
			pthread_exit(0);
		}
		PLTM_DPRINT((0, "A\n"));
		q->queued--;
		req = q->items[q->head];
		q->head = (q->head + 1) % q->count;
		PLTM_DPRINT((0, "B\n"));
		if (q->nonBlocking == 0 && q->queued == q->count - 1)
			pthread_cond_signal(&q->notFull);
		PLTM_DPRINT((0, "C\n"));
		if (q->queued == 0)
			pthread_cond_signal(&q->drained);
		PLTM_DPRINT((0, "D\n"));
		pthread_mutex_unlock(&q->mutex);
		PLTM_DPRINT((0, "E\n"));
		pthread_mutex_lock(&font->queue.mutex);
		PLTM_DPRINT((0, "F\n"));
		item = font->queue.itemArrayBase + req.index;
		if (item->mode != PLTM_CACHE_QUEUED) {
			pthread_mutex_unlock(&font->queue.mutex);
			continue;
		}
		PLTM_DPRINT((0, "G\n"));
		item->mode = PLTM_CACHE_OUTLINE;
		RenderGlyph(font, item);
		PLTM_DPRINT((0, "H\n"));
		item->mode = PLTM_CACHE_OUTLINE_DONE;
		PLTM_DPRINT((0, "I\n"));
		PLTM_DPRINT((0, "==== ENTER _PLTM_AddBitmapQueue ====\n"));
		if (PushBitmapRequest(font, req.index, item) < 0) {
			PLTM_DPRINT((0, "==== error exit _PLTM_AddBitmapQueue ====\n"));
		} else {
			PLTM_DPRINT((0, "==== success exit _PLTM_AddBitmapQueue ====\n"));
		}
		PLTM_DPRINT((0, "J\n"));
		pthread_mutex_unlock(&font->queue.mutex);
		PLTM_DPRINT((0, "K\n"));
	}
}

/* Bitmap worker thread: like OutlineThread, but takes the mutex with trylock
   and usleep(1000), and retries UploadGlyph until it succeeds (PLTM_CACHE_BITMAP_QUEUED -> READY). */
static void *BitmapThread(struct PLTM_Font *font)
{
	struct PLTM_Request req;
	struct PLTM_CacheItem *item;

	nice(10);
	for (;;) {
		struct PLTM_ReqQueue *q = &font->bitmapQueue;

		while (pthread_mutex_trylock(&q->mutex) == 16) {
			PLTM_DPRINT((0, "====trylock failed in PT_Bitmap====\n"));
			usleep(1000);
		}
		PLTM_DPRINT((0, "==%s(%s)==\n", "PT_Bitmap", "A"));
		while (q->queued == 0 && q->closing == 0) {
			PLTM_DPRINT((0, "Bitmap:SLEEP\n"));
			pthread_cond_wait(&q->notEmpty, &q->mutex);
			PLTM_DPRINT((0, "Bitmap:WAKEUP\n"));
		}
		PLTM_DPRINT((0, "==%s(%s)==\n", "PT_Bitmap", "B"));
		if (q->closing) {
			pthread_mutex_unlock(&q->mutex);
			pthread_exit(0);
		}
		q->queued--;
		req = q->items[q->head];
		PLTM_DPRINT((0, "==%s(%s)==\n", "PT_Bitmap", "C"));
		if (q->nonBlocking == 0 && q->queued == q->count - 1)
			pthread_cond_signal(&q->notFull);
		PLTM_DPRINT((0, "==%s(%s)==\n", "PT_Bitmap", "D"));
		if (q->queued == 0)
			pthread_cond_signal(&q->drained);
		PLTM_DPRINT((0, "==%s(%s)==\n", "PT_Bitmap", "E"));
		q->head = (q->head + 1) % q->count;
		pthread_mutex_unlock(&q->mutex);
		PLTM_DPRINT((0, "==%s(%s)==\n", "PT_Bitmap", "F"));
		pthread_mutex_lock(&font->queue.mutex);
		PLTM_DPRINT((0, "==%s(%s)==\n", "PT_Bitmap", "G"));
		item = font->queue.itemArrayBase + req.index;
		if (item->mode != PLTM_CACHE_BITMAP_QUEUED) {
			pthread_mutex_unlock(&font->queue.mutex);
			continue;
		}
		PLTM_DPRINT((0, "==%s(%s)==\n", "PT_Bitmap", "H"));
		while (UploadGlyph(font, item) == 0)
			;
		item->mode = PLTM_CACHE_READY;
		pthread_mutex_unlock(&font->queue.mutex);
		PLTM_DPRINT((0, "==%s(%s)==\n", "PT_Bitmap", "I"));
	}
}

/* Loads the glyph outline: the full-width face for codes >= 256, the
   half-width one otherwise, both sized to charSize << 6 (26.6). Keeps the
   ascender/descender in top/bottom, the FT_Glyph in ftGlyph and its metrics in
   width..vertAdvance. */
static void RenderGlyph(struct PLTM_Font *font, struct PLTM_CacheItem *item)
{
	FT_Face face;
	FT_GlyphSlot slot;
	int gindex;
	int err;
	int a;
	int b;
	int c;
	int d;

	face = (item->code < 256) ? font->ftFaceHalf : font->ftFaceFull;
	pthread_mutex_lock(&s_outlineSub);
	FT_Set_Char_Size(font->ftFaceHalf, item->charSize << 6, item->charSize << 6, font->resX, font->resY);
	FT_Set_Char_Size(font->ftFaceFull, item->charSize << 6, item->charSize << 6, font->resX, font->resY);
	a = font->ftFaceHalf->size->metrics.ascender;
	item->top = a;
	b = font->ftFaceFull->size->metrics.ascender;
	if (a < b)
		item->top = b;
	c = font->ftFaceHalf->size->metrics.descender;
	item->bottom = c;
	d = font->ftFaceFull->size->metrics.descender;
	if (d < c)
		item->bottom = d;
	gindex = FT_Get_Char_Index(face, item->code);
	err = FT_Load_Glyph(face, gindex, font->loadFlags);
	if (err != 0) {
		PLTM_DPRINT((0, "ft2Error (3)=%d\n", err));
	}
	FT_Get_Glyph(face->glyph, &item->ftGlyph);
	PLTM_DPRINT((0, "             -> DONE====\n"));
	slot = face->glyph;
	PLTM_DPRINT((0, "    ---- ---- ---- ----\n"));
	PLTM_DPRINT((0, "    width=%d\n", slot->metrics.width));
	PLTM_DPRINT((0, "    height=%d\n", slot->metrics.height));
	PLTM_DPRINT((0, "    horiBearingX=%d\n", slot->metrics.horiBearingX));
	PLTM_DPRINT((0, "    horiBearingY=%d\n", slot->metrics.horiBearingY));
	PLTM_DPRINT((0, "    horiAdvance=%d\n", slot->metrics.horiAdvance));
	item->metrics.width = slot->metrics.width;
	item->metrics.height = slot->metrics.height;
	item->metrics.horiBearingX = slot->metrics.horiBearingX;
	item->metrics.horiBearingY = slot->metrics.horiBearingY;
	item->metrics.horiAdvance = slot->metrics.horiAdvance;
	item->metrics.vertBearingX = slot->metrics.vertBearingX;
	item->metrics.vertBearingY = slot->metrics.vertBearingY;
	item->metrics.vertAdvance = slot->metrics.vertAdvance;
	pthread_mutex_unlock(&s_outlineSub);
}

/* Takes a glyph from PLTM_CACHE_BITMAP_QUEUED to PLTM_CACHE_READY, one step per
   call: when a step lacks resources the entry goes to PLTM_CACHE_WAIT,
   s_flagPending is set and the thread sleeps until something is freed.
   Returns 1 when the glyph is ready, 0 if rasterization failed. */
static int UploadGlyph(struct PLTM_Font *font, struct PLTM_CacheItem *item)
{
	int n;

	if (item->mode == PLTM_CACHE_BITMAP_QUEUED) {
		pthread_mutex_lock(&s_outlineSub);
		n = FT_Glyph_To_Bitmap(&item->ftGlyph, font->renderMode, 0, 1);
		pthread_mutex_unlock(&s_outlineSub);
		if (n)
			return 0;
		item->mode = PLTM_CACHE_RASTERIZED;
	}
	if (item->mode == PLTM_CACHE_RASTERIZED) {
		pthread_mutex_lock(&s_bitmapSub);
		while (AllocGlyphSlot(item) < 0) {
			item->mode = PLTM_CACHE_WAIT;
			s_flagPending = 1;
			pthread_mutex_unlock(&font->queue.mutex);
			pthread_cond_wait(&s_thirdSub, &s_bitmapSub);
			pthread_mutex_lock(&font->queue.mutex);
		}
		pthread_mutex_unlock(&s_bitmapSub);
		item->mode = PLTM_CACHE_ALLOCATED;
		font->numLoaded++;
	}
	if (item->mode == PLTM_CACHE_ALLOCATED) {
		pthread_mutex_lock(&s_bitmapSub);
		while (
		    (item->tex = tmCreateTexture(s_tmID, item->texWidth, item->texHeight, TM_USER_IMAGE | PS2_GS_PSMT8))
		    == -3) {
			item->mode = PLTM_CACHE_WAIT;
			s_flagPending = 1;
			pthread_mutex_unlock(&font->queue.mutex);
			pthread_cond_wait(&s_thirdSub, &s_bitmapSub);
			pthread_mutex_lock(&font->queue.mutex);
		}
		pthread_mutex_unlock(&s_bitmapSub);
	}
	tmUpdateTexture(item->tex, item->texWidth, item->texHeight, 19, item->resource);
	tmSetClut(item->tex, s_defaultClut);
	if (item->top != item->bottom) {
		int r;

		r = ((item->top - item->bearingY) << 8) / (item->top - item->bottom);
		item->gradTop = r;
		if (r < 0)
			item->gradTop = 0;
		r = ((item->top - item->bearingY + item->metrics.height) << 8) / (item->top - item->bottom);
		item->gradBottom = r;
		if (r < 0)
			item->gradBottom = 0;
	} else {
		item->gradBottom = 0;
		item->gradTop = 0;
	}
	return 1;
}

/* Prepares the 8-bit texture buffer of a glyph from its FreeType bitmap:
   bearing (26.6), size (width aligned to 8 with a border, height + 4), buffer
   from the pool, and a vertical three-row filter: every source pixel s gives
   k = s * weight / 256 to the rows above and below and keeps s - 2k.
   Returns 0, or -1 if the pool is exhausted. */
static int AllocGlyphSlot(struct PLTM_CacheItem *item)
{
	FT_BitmapGlyph bg = (FT_BitmapGlyph)item->ftGlyph;
	FT_Bitmap *bm;
	unsigned char *buf, *prev, *next, *src, *p, *d, *q;
	int size, row, col;

	item->bearingX = bg->left << 6;
	item->bearingY = bg->top << 6;
	bm = &bg->bitmap;
	item->texWidth = bm->width + 2;
	item->texHeight = bm->rows + 4;
	item->texWidth = (item->texWidth + 7) & ~7;
	item->texLogW = PLTM_BitsFor(item->texWidth);
	item->texLogH = PLTM_BitsFor(item->texHeight);
	size = ((unsigned int)(item->texWidth * item->texHeight + 15) >> 4) << 4;
	item->resSize = size;
	buf = PoolAlloc(size);
	item->resource = buf;
	if (buf == 0)
		return -1;

	p = buf;
	for (row = 0; row < 3; row++)
		for (col = 0; col < item->texWidth; col++)
			*p++ = 0;
	p = (unsigned char *)item->resource + (item->texHeight - 2) * item->texWidth;
	for (row = item->texHeight - 2; row < item->texHeight; row++)
		for (col = 0; col < item->texWidth; col++)
			*p++ = 0;

	src = bm->buffer;
	p = (unsigned char *)item->resource + item->texWidth * 2;
	prev = p - item->texWidth;
	next = p + item->texWidth;
	for (row = 2; row < item->texHeight - 2; row++) {
		d = p;
		q = next;
		for (col = 0; col < 1; col++) {
			*d++ = 0;
			q++;
		}
		prev += col;
		for (col = 1; col < bm->width + 1; col++) {
			int s = *src++;
			int k = s * _pltmDBG_BlendValue256 / 256;
			int v;

			PLTM_DPRINT((0, "%2.2x, ", s));
			v = *prev + k;
			*prev = v;
			prev++;
			v = *d + (s - k * 2);
			*d = v;
			d++;
			*q++ = k;
		}
		for (; col < item->texWidth; col++)
			*d++ = 0;
		prev = p;
		p = next;
		next += item->texWidth;
	}
	return 0;
}

/* Fills the four GS vertices of the draw slot with the quad of one cached
   glyph. Coordinates arrive in 26.6 (64 bit), become 1/16 pixel and are moved
   by _pltmDBG_ShiftX0/Y0; y is negated. Texture coordinates are in texels with
   half a texel of border. Returns 0, or -1 if the glyph has no buffer. */
static int EmitGlyph(struct PLTM_Font *font, struct PLTM_CacheItem *item, long long x, long long y,
    struct PLTM_DrawSlot *slot)
{
	int w = (item->texWidth - 1) << 4;
	int h = (item->texHeight - 1) << 4;
	struct gmVertexPNCT *v, *p;
	float su, tv, s0, t0, s1, t1;
	long long x1, ny0, ny1;

	(void)font;
	if (item->resource == 0)
		return -1;
	if (item->resSize == 0)
		return -1;
	v = (struct gmVertexPNCT *)slot;
	p = v + 1;
	x = (x - 128) >> 2;
	y = (y - 128) >> 2;
	su = 1.0f / (float)(1 << (item->texLogW + 4));
	tv = 1.0f / (float)(1 << (item->texLogH + 4));
	x += _pltmDBG_ShiftX0;
	y += _pltmDBG_ShiftY0;
	s0 = su * 8.0f;
	t0 = tv * 8.0f;

	v->nx = 0;
	v->ny = 0;
	v->nz = 1.0f;
	v->u = s0;
	v->v = t0;
	v->x = (float)x * 0.0625f;
	ny0 = -y;
	v->y = (float)ny0 * 0.0625f;
	v->z = 0;

	x1 = x + w;
	s1 = (float)(w + 8) * su;
	p->v = t0;
	p->nx = 0;
	p->ny = 0;
	p->nz = 1.0f;
	p->u = s1;
	p->x = (float)x1 * 0.0625f;
	p->y = (float)ny0 * 0.0625f;
	p->z = 0;

	p = v + 2;
	t1 = (float)(h + 8) * tv;
	p->u = s0;
	p->nx = 0;
	p->ny = 0;
	p->nz = 1.0f;
	p->v = t1;
	p->x = (float)x * 0.0625f;
	ny1 = -(y + h);
	p->y = (float)ny1 * 0.0625f;
	p->z = 0;

	p = v + 3;
	p->u = s1;
	p->v = t1;
	p->nx = 0;
	p->ny = 0;
	p->nz = 1.0f;
	p->x = (float)x1 * 0.0625f;
	p->y = (float)ny1 * 0.0625f;
	p->z = 0;

	slot->tex = item->tex;
	return 0;
}

/* Builds one A+D entry (data, addr) for register `reg` from a variadic list
   of (field id, value) pairs ended by 0, using the s_gsRegDesc layout. */
void gsRegisterAddrAndParameters(unsigned long long *packet, unsigned int reg, ...)
{
	va_list ap;
	unsigned char *e;
	unsigned int idx;
	int tag;
	int i;

	va_start(ap, reg);
	tag = va_arg(ap, int);
	idx = reg;
	if (reg & 0x80)
		idx = reg & ~0x80;

	if (s_gsRegTable == 0) {
		unsigned short *t = calloc(99, 2);
		int first;

		i = 2;
		first = *gsRegDescEntry(i);
		s_gsRegTable = t;
		while (first != 0xff) {
			unsigned char *d = gsRegDescEntry(i);
			int len;
			t[d[0]] = i;
			len = 3 * d[1] + 2;
			i = ((short)i + len) & 0xffff;
			first = *gsRegDescEntry(i);
		}
	}

	if (s_gsRegTable[idx] == 0)
		e = 0;
	else
		e = gsRegDescEntry(s_gsRegTable[idx]);

	*packet++ = gsPackParams(e, tag, ap);
	*packet = reg;
}

/* Builds the 368-byte GS packet copied into every packet buffer: GIF tag
   (NLOOP 22, EOP, NREG 1, REGS A+D), the registers encoded by
   gsRegisterAddrAndParameters, then the four vertices (RGBAQ, UV, XYZ2 with
   Z = 0xFFFFFFFF) filled per glyph. */
static void BuildPacketTemplate(void *pkt_)
{
	unsigned long long *pkt = pkt_;
	unsigned int i;
	unsigned long long *p0, *p1, *p2;
	unsigned int *tag = pkt_;
	unsigned int regs = 0xe;  /* REGS: a single A+D descriptor */

	pkt[1] = regs;
	tag[1] = 1 << 28;   /* NREG 1 */
	tag[0] = 22 | (1 << 15);  /* NLOOP 22, EOP */
	gsRegisterAddrAndParameters(pkt + 2, 63, 0);
	gsRegisterAddrAndParameters(pkt + 4, 134, 182, 0, 183, 0, 48, 19, 184, 0, 185, 0, 186, 1, 187, 0, 188, 0, 189,
	    s_pool.clutFlags, 190, 0, 191, 0, 192, 1, 0);
	gsRegisterAddrAndParameters(pkt + 6, 148, 193, 1, 194, 0, 195, 1, 196, 1, 197, 0, 198, 0, 199, 16, 0);
	gsRegisterAddrAndParameters(pkt + 8, 59, 200, 128, 202, 0, 201, 128, 0);
	gsRegisterAddrAndParameters(pkt + 10, 199, 174, 1, 175, 6, 176, 0, 177, 0, 178, 0, 180, 0, 181, 0, 0);
	gsRegisterAddrAndParameters(pkt + 12, 136, 110, 1, 111, 1, 112, 0, 113, 255, 114, 0, 115, 255, 0);
	gsRegisterAddrAndParameters(pkt + 14, 194, 99, 0, 100, 1, 101, 0, 102, 1, 103, 128, 0);
	gsRegisterAddrAndParameters(pkt + 16, 73, 154, 0, 0);
	gsRegisterAddrAndParameters(pkt + 18, 26, 163, 1, 0);
	gsRegisterAddrAndParameters(pkt + 20, 0, 155, 4, 156, 1, 157, 1, 158, 0, 159, 1, 160, 0, 161, 1, 162, 0, 103, 0,
	    0);
	p2 = pkt + 26;
	p1 = pkt + 24;
	p0 = pkt + 22;
	for (i = 0; i < 4; i++) {
		gsRegisterAddrAndParameters(p0, 1, 0);
		gsRegisterAddrAndParameters(p1, 3, 0);
		gsRegisterAddrAndParameters(p2, 5, 0);
		((int *)p0)[9] = -1;
		p2 += 6;
		p1 += 6;
		p0 += 6;
	}
}

/* Fills the GS packet of the cached glyph: TEX0 from the cache texture and
   the four quad vertices (colors from `color`), with the pen in 1/4 pixel and
   the GS screen offset (2048-320, 2048-224) in 1/16 pixel. `narrow` makes
   the quad 3/4 wide. */
static void BuildGlyphPacket(struct PLTM_CacheItem *item, struct PLTM_GlyphPacket *p, long long x, long long y,
    const struct PltmColor *color, unsigned char narrow)
{
	const unsigned char *c;
	struct PLTM_GsVertex *vp;
	int u = (item->texWidth - 1) << 4;
	int v = (item->texHeight - 1) << 4;
	int w;

	TmHandle tex;

	if (!narrow)
		w = u;
	else
		w = (int)((float)(item->texWidth - 1) * 16.0f * 0.75f);
	x = (x - 128) >> 2;
	y = (y - 128) >> 2;
	tex = item->tex;
	p->tex0.TBP0 = tmGetBase(tex);
	p->tex0.TBW = tmGetBaseWidth(tex);
	p->tex0.CBP = tmGetClutBase(tex);
	p->tex0.TW = item->texLogW;
	p->tex0.TH = item->texLogH;
	x += _pltmDBG_ShiftX0;
	y += _pltmDBG_ShiftY0;

	c = color->rgba[0];
	vp = p->v;
	vp->uv = PS2_GS_SETREG_UV(8, 8);
	vp->xyz2 = PS2_GS_SETREG_XYZ2(GsScreenX(x), GsScreenY(y), 0xffffffff);
	vp->rgbaq = PS2_GS_SETREG_RGBAQ(c[0], c[1], c[2], c[3], 0);
	vp++;
	vp->uv = PS2_GS_SETREG_UV(u + 8, 8);
	vp->xyz2 = PS2_GS_SETREG_XYZ2(x + GsScreenX(w), GsScreenY(y), 0xffffffff);
	vp->rgbaq = PS2_GS_SETREG_RGBAQ(c[4], c[5], c[6], c[7], 0);
	vp++;
	vp->uv = PS2_GS_SETREG_UV(8, v + 8);
	vp->xyz2 = PS2_GS_SETREG_XYZ2(GsScreenX(x), y + GsScreenY(v), 0xffffffff);
	vp->rgbaq = PS2_GS_SETREG_RGBAQ(c[8], c[9], c[10], c[11], 0);
	vp++;
	vp->uv = PS2_GS_SETREG_UV(u + 8, v + 8);
	vp->xyz2 = PS2_GS_SETREG_XYZ2(x + GsScreenX(w), y + GsScreenY(v), 0xffffffff);
	vp->rgbaq = PS2_GS_SETREG_RGBAQ(c[12], c[13], c[14], c[15], 0);
}

static int PLTM_Log(int level, const char *fmt, ...)
{
	va_list ap;
	int r;

	va_start(ap, fmt);
	r = vfprintf(stderr, fmt, ap);
	va_end(ap);
	return r;
}

static __inline__ int InitReqQueue(struct PLTM_ReqQueue *q, int count)
{
	q->items = s_allocFn(count * 52);
	if (q == 0)
		return -1;
	q->count = count;
	q->queued = 0;
	q->nonBlocking = 1;
	q->abort = 0;
	q->closing = 0;
	q->tail = 0;
	q->head = 0;
	pthread_mutex_init(&q->mutex, 0);
	pthread_cond_init(&q->notEmpty, 0);
	pthread_cond_init(&q->notFull, 0);
	pthread_cond_init(&q->drained, 0);
	return 0;
}

/* (queue, key) -> index in the entry array, negative if not found. */
static __inline__ int ResolveGlyphIndex(struct PLTM_CacheQueue *q, struct PLTM_Glyph *key)
{
	int i;

	for (i = 0; i < q->count; i++) {
		struct PLTM_CacheItem *item = q->itemArrayBase + i;

		if (item->mode != PLTM_CACHE_FREE) {
			int hit = item->code == key->code && item->charSize == key->charSize;

			if (hit)
				return i;
		}
	}
	return -1;
}

/* Finds a cache slot to reuse: with force == 0 a never used slot (PLTM_CACHE_FREE) is
   returned at once; otherwise the PLTM_CACHE_READY slot with the highest `used`
   age is freed. Returns the index, -1 (nothing old enough) or -2 (mutex busy
   or no candidate). */
static __inline__ int ReclaimCacheSlot(struct PLTM_Font *font, struct PLTM_CacheQueue *q, int force)
{
	struct PLTM_CacheItem *base;
	struct PLTM_CacheItem *item;
	pthread_mutex_t *m = &q->mutex;
	int best = -1;
	int bestAge = 0;
	int i;
	int count;

	if (pthread_mutex_trylock(m) == 16)
		return -2;
	i = 0;
	item = q->itemArrayBase;
	count = q->count;
	base = item;
	while (i < count) {
		unsigned short mode = item->mode;

		if (force == 0 && mode == PLTM_CACHE_FREE)
			goto found;
		if (mode == PLTM_CACHE_READY) {
			if (bestAge < item->used) {
				bestAge = item->used;
				best = i;
			}
		}
		i++;
		item++;
	}
	if (best == -1) {
		pthread_mutex_unlock(m);
		return -2;
	}
	if (best < 0)
		goto fail;
	if (bestAge < 2)
		goto fail;
	if (ReleaseCacheItem(font, &base[best]))
		font->numLoaded--;
	pthread_mutex_unlock(m);
	return best;
found:
	pthread_mutex_unlock(m);
	return i;
fail:
	pthread_mutex_unlock(m);
	return -1;
}

/* Frees the resources of one PLTM_CACHE_READY cache slot and reinitializes it.
   The first parameter is unused. */
static __inline__ int ReleaseCacheItem(struct PLTM_Font *font, struct PLTM_CacheItem *it)
{
	int ret = 0;

	switch (it->mode) {
	case 5:
	case PLTM_CACHE_RASTERIZED:
	case PLTM_CACHE_ALLOCATED:
		break;
	case PLTM_CACHE_READY:
		pthread_mutex_lock(&s_outlineSub);
		FT_Done_Glyph(it->ftGlyph);
		pthread_mutex_unlock(&s_outlineSub);
		if (it->tex)
			tmRelease(it->tex);
		it->tex = 0;
		if (it->resource != 0) {
			char *hdr = (char *)it->resource - 16;
			int size = *(int *)hdr;
			s_usedBytes -= size;
			s_poolFreeFn(hdr);
			if (s_flagPending) {
				s_flagPending = 0;
				pthread_cond_broadcast(&s_thirdSub);
			}
			it->resource = 0;
		}
		it->mode = PLTM_CACHE_FREE;
		it->used = 0;
		it->code = 0xffff;
		it->charSize = -1;
		ret = 1;
		break;
	}
	return ret;
}

/* Takes the next free packet from the font's double buffer, fills it with
   BuildGlyphPacket and passes it (23 qwords) to the s_sendPacketFn callback. -1 if the
   slot has no resource or the buffer is full. */
static __inline__ int EmitGlyphPacket(struct PLTM_Font *font, struct PLTM_CacheItem *item, long long x, long long y,
    const struct PltmColor *color, unsigned char narrow)
{
	struct PLTM_GlyphPacket *pkt;
	int idx;

	if (item->resource == 0)
		return -1;
	if (item->resSize == 0)
		return -1;
	if (font->pktPerPool <= font->pktUsed)
		return -1;
	idx = font->pktUsed;
	pkt = &font->pools[font->curPool][idx];
	font->pktUsed = idx + 1;
	tmChainTexture(item->tex);
	BuildGlyphPacket(item, pkt, x, y, color, narrow);
	s_sendPacketFn(pkt, 23);
	return 0;
}

/* Smallest number of bits that holds n. */
static __inline__ int PLTM_BitsFor(int n)
{
	int i = 0;

	while (1) {
		if ((1 << i) >= n)
			return i;
		i++;
	}
}

/* Decodes one UTF-8 code point and advances *cursor: private copy of
   sceCccDecodeUTF8 (src/lib/ccc.c) with this file's error character. */
static __inline__ unsigned int DecodeUtf8Step(const char **cursor)
{
	unsigned char **s = (unsigned char **)cursor;
	unsigned char *cur = *s;
	unsigned char lead = *cur;
	unsigned int acc;
	unsigned int limit;

	cur++;
	if ((signed char)lead >= 0) {
		acc = lead;
		goto done;
	}
	if ((unsigned char)(lead + 62) >= 51) {
		acc = g_errCharUTF8;
		goto done;
	}
	acc = lead & 0x3f;
	limit = 0x800;
	acc = (acc << 6) | (*cur & 0x3f);
	cur++;
	if (acc >= limit) {
		do {
			acc &= limit - 1;
			limit <<= 5;
			acc = (acc << 6) | (*cur & 0x3f);
			cur++;
		} while (acc >= limit);
	}
done:
	*s = cur;
	return acc;
}

/* Private copies of the sceCcc* UTF-16 accessors of src/lib/ccc.c, unused here. */
__inline__ int _sceCccIsValidUTF16(unsigned int c)
{
	if (c > 0x10FFFF)
		return 0;
	return !((unsigned int)(c - 0xD800) < 0x800);
}

__inline__ int _sceCccDecodeUTF16(unsigned short **s)
{
	unsigned short *cur = *s;
	unsigned short c1 = *cur;
	unsigned short c2;
	unsigned int r;
	int is_high;
	unsigned int lim;

	is_high = (unsigned short)(c1 + 0x2800) < 0x400;
	cur++;
	if (is_high) {
		c2 = *cur;
		cur++;
		if ((unsigned short)(c2 + 0x2400) < 0x400) {
			r = ((unsigned int)c1 << 16) | c2;
		} else {
			r = g_errCharUTF16;
		}
	} else {
		unsigned int d;

		r = g_errCharUTF16;
		d = (unsigned short)(c1 + 0x2400);
		lim = 1023;
		if (lim < d)
			r = c1;
	}
	*s = cur;
	return r;
}

/* Private copy of sceCccEncodeUTF16: writes ucs at *s as UTF-16, advancing *s. */
static __inline__ void _sceCccEncodeUTF16(unsigned short **s, unsigned int ucs)
{
	unsigned short *cur = *s;
	int valid;
	unsigned int d;
	unsigned int lim;

	if (ucs > 0x10FFFF) {
		valid = 0;
		goto merge;
	}
	d = ucs - 0xD800;
	lim = 0x7FF;
	valid = lim < d;
merge:
	if (!valid)
		ucs = g_errCharUTF16;

	if (ucs <= 0xFFFF) {
		*cur = (unsigned short)ucs;
		cur++;
	} else {
		ucs -= 0x10000;
		*cur = (unsigned short)(0xD800 + (ucs >> 10));
		cur++;
		*cur = (unsigned short)(0xDC00 + (ucs & 0x3FF));
		cur++;
	}
	*s = cur;
}

/* Counts the UTF-16 characters of s. */
static __inline__ int _sceCccStrlenUTF16(unsigned short *s)
{
	int n = 0;
	while (ccc_decode_utf16(&s))
		n++;
	return n;
}

static __inline__ int Utf8Len(unsigned char *s)
{
	int n = 0;

	while (utf8_step(&s))
		n++;
	return n;
}

/* Copies an 8-bit tile into a WxH buffer, clipped per pixel. x is 26.6 fixed
   point, y is in pixels. */
static __inline__ void BlitTile(unsigned char *dst, int width, int height, int xfix, int y, const unsigned char *src,
    int srcW, int srcH)
{
	int row, col;
	unsigned char *dstRow;
	unsigned char *p;
	int x = (xfix >> 6) + 1;

	y = y + 1;
	dstRow = dst + (y * width + x);
	for (row = 0; row < srcH; row++, dstRow += width) {
		int yy = y + row;

		if (yy <= 0)
			continue;
		if (yy >= height - 1)
			break;
		p = dstRow;
		for (col = 0; col < srcW; col++, p++) {
			int xx = x + col;
			unsigned int v;

			if (xx <= 0)
				continue;
			if (xx >= width - 1)
				break;
			v = src[row * srcW + col];
			if (v >= 256)
				v = 255;
			*p = (unsigned char)v;
		}
	}
}

/* Like BlitTile with horizontal 1/64-pixel interpolation: each source byte is
   split between two pixels by the fraction of x, saturating at 255. */
static __inline__ void BlitTileBlend(unsigned char *dst, int width, int height, int xfix, int y,
    const unsigned char *src, int srcW, int srcH)
{
	int row, col;
	unsigned char *dstRow;
	unsigned char *p;
	int frac = xfix % 64;
	int x;

	if (frac < 0)
		frac += 64;
	x = (xfix + 64) >> 6;
	y = y + 1;
	dstRow = dst + (y * width + x);
	for (row = 0; row < srcH; row++, dstRow += width) {
		int yy = y + row;

		if (yy <= 0)
			continue;
		if (yy >= height - 1)
			break;
		p = dstRow;
		for (col = 0; col < srcW; col++, p++) {
			int xx = x + col;
			unsigned int s, hi, a, b;

			if (xx <= 0)
				continue;
			if (xx >= width - 1)
				break;
			s = src[row * srcW + col];
			hi = (frac * s) >> 6;
			a = p[0] + (s - hi);
			if (a >= 256)
				a = 255;
			p[0] = (unsigned char)a;
			b = hi + p[1];
			if (b >= 256)
				b = 255;
			p[1] = (unsigned char)b;
		}
	}
}

__inline__ void PLTM_Expire(struct PLTM_Item *item)
{
	if (item)
		item->expired = 1;
}

/* Exported but never called; always false in this version. */
__inline__ int _isHalfWidthFullWidthChar(int c)
{
	return 0;
}
