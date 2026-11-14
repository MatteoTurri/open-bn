/*
 * SimpleTextEdit widget: construction, options, editing actions, line wrap and
 * cursor tracking, the glyph-rendering thread and drawing.
 */

#include <iostream.h>
#include "gfx/vec.h"
#include "simpletextedit.h"
#include <pthread.h>
#include <string>
#include <assert.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gfx/freetype2.h"
#include "gfx/tm.h"
#include "scene/optid.h"
#include "gfx/gm_graph.h"

/* Unit matrix, unit w row and the ClassInitializer instance: the file-scope
   objects built at startup. */


static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);
SimpleTextEdit::ClassInitializer __SimpleTextEdit_ClassInitializer__;


static SimpleTextEdit *s_registry[STE_MAX_EDITORS];

/* Set before the draw thread starts and cleared by it once running; set
   again later to ask it to stop. Shared between threads. */


static volatile int s_fontThreadStarting;
static pthread_t s_fontThreadId;

/* Guards the registry of the STE_MAX_EDITORS editors and the field-size fields. */


static pthread_mutex_t s_registryMutex;


/* Default vertex tables (position and RGBA color, plus uv in kTextQuad) copied by
   the constructor, the quads of drawing passes 2 and 3, and the default tag. */


static gmVertexPCT kTextQuad[4] = {
	{ 0.0f, 0.5f, 0.0f, 0xffffffff, 0.0f, 0.0f },
	{ 0.0f, -0.5f, 0.0f, 0xffffffff, 0.0f, 1.0f },
	{ 1.0f, 0.5f, 0.0f, 0xffffffff, 1.0f, 0.0f },
	{ 1.0f, -0.5f, 0.0f, 0xffffffff, 1.0f, 1.0f },
};
/* Static quads of drawing passes 2 and 3 (vertex format 5); DrawThis
   rewrites only x and y. */

static gmVertexPC s_steQuadA[4] = {
	{ 0.0f, 0.5f, 0.0f, 0x00000000 },
	{ 0.0f, -0.5f, 0.0f, 0x00000000 },
	{ 1.0f, 0.5f, 0.0f, 0x00000000 },
	{ 1.0f, -0.5f, 0.0f, 0x00000000 },
};
static gmVertexPC s_steQuadB[4] = {
	{ 0.0f, 0.5f, 0.0f, 0xff000000 },
	{ 0.0f, -0.5f, 0.0f, 0xff000000 },
	{ 1.0f, 0.5f, 0.0f, 0xff000000 },
	{ 1.0f, -0.5f, 0.0f, 0xff000000 },
};
static gmVertexPC kFieldQuad[4] = {
	{ 0.0f, 0.5f, 0.0f, 0x80404040 },
	{ 0.0f, -0.5f, 0.0f, 0x80808080 },
	{ 1.0f, 0.5f, 0.0f, 0x80404040 },
	{ 1.0f, -0.5f, 0.0f, 0x80808080 },
};
static gmVertexPC kCursorQuad[4] = {
	{ 0.0f, 0.5f, 0.0f, 0xffffffff },
	{ 0.0f, -0.5f, 0.0f, 0xffffffff },
	{ 1.0f, 0.5f, 0.0f, 0xffffffff },
	{ 1.0f, -0.5f, 0.0f, 0xffffffff },
};
/* Character used for invalid UTF-8 and UCS-2 input. */

static unsigned int s_utf8ReplacementChar = 0;
static char kTag[4] = { -1, -1, -1, -1 };

/* Encodes c as UTF-8 at *pp and advances *pp; a negative c is replaced by
   s_utf8ReplacementChar. */


static inline void utf8_encode(unsigned char **pp, int c)
{
	unsigned char *p = *pp;
	unsigned int ch = c < 0 ? s_utf8ReplacementChar : (unsigned int)c;

	if (ch < 128) {
		*p = (unsigned char)ch;
		p++;
	} else {
		unsigned int lim = 2048;
		int n = 1;

		while (ch >= lim) {
			n++;
			lim <<= 5;
		}
		p += n + 1;
		{
			unsigned char *q = p;
			unsigned int mask = (unsigned char)((1 << (7 - n)) - 1);

			while (n--) {
				q--;
				*q = (unsigned char)(0x80 | (ch & 0x3f));
				ch >>= 6;
			}
			q[-1] = (unsigned char)(~mask | (ch & (mask >> 1)));
		}
	}
	*pp = p;
}

static int SimpleTextEditUcs2ToUtf8Size(const unsigned short *ucs2);
static void SimpleTextEditUcs2ToUtf8(unsigned char *dst, const unsigned short *ucs2);

/* Returns the next code unit and advances *pp. */


static inline int ucs2_next(const unsigned short **pp)
{
	return *(*pp)++;
}

/*
 * GetOption(int &) options:
 *   'CPDL'          display rows per page
 *   'TW64', 'W64 '  buffer width, in 1/64
 *   'TH64', 'H64 '  buffer height, in 1/64
 *   'FSIZ'          font size
 *   'SDWE'          m_shadowOffset
 *   'CSPC'          character spacing
 *   'LSPC'          total line spacing ('LSPU' above, 'LSPL' below)
 *   'CRSW'          cursor shown
 *   'EDTE'          editing enabled
 *   'INST'          insert mode
 *   'MDFY'          m_verifyInProgress
 *   'CPSX', 'CPSY'  cursor column and line
 *   'CPSM'          number of logical lines
 *   'CPDY'          display rows before the cursor
 *   'CPDM'          total display rows
 *   'CX64', 'CY64'  caret position, in 1/64
 *   'LEN '          text size (characters plus one per line)
 *   'GSSY'          m_gssy
 *   'GNLN'          CalcNumLine(value)
 *   'DS64'          smallest descender of the loaded faces
 * The counting options take the registry lock.
 */

#define STE_BLEND_PIXEL(p, q)					\
	if ((p) >= lo && (p) < hi) {				\
		int v = *(p) + (q);				\
								\
		if (v < 0)					\
			v = 0;					\
		else if (v > 255)				\
			v = 255;				\
		*(p) = (unsigned char) v;			\
	}

/* Blends a glyph into dst, spreading each pixel over three rows with weights
   1/4, 1/2, 1/4. The shadow copy is offset by d pixels diagonally at a quarter
   of the intensity; `inv` subtracts instead of adding. */


static inline void SteBlendGlyph(SimpleTextEdit *ste, unsigned char *dst, int d, unsigned char *lo, unsigned char *hi,
    unsigned char *src, int bw, int bh, int inv, int shadow)
{
	int w = ste->m_dispWidth;
	unsigned char *ctr = dst + d + d * w;
	unsigned char *up = ctr - w, *dn = ctr + w;
	int r, c;

	for (r = 0; r < bh; r++) {
		unsigned char *pc = ctr, *pu = up, *pd = dn;

		for (c = 0; c < bw; c++) {
			int g = src[r * bw + c];
			int q;

			if (shadow)
				g = g >> 2;
			if (inv)
				g = -g;
			q = g * 64 >> 8;
			STE_BLEND_PIXEL(pu, q);
			pu++;
			STE_BLEND_PIXEL(pc, g - q * 2);
			pc++;
			STE_BLEND_PIXEL(pd, q);
			pd++;
		}
		up = ctr;
		ctr = dn;
		dn = dn + w;
	}
}


/* Records the caret position when the draw thread reaches the cursor. */


static inline void SteUpdateCaret(SimpleTextEdit *e)
{
	if (e->m_drawChunk == e->m_curCharNo && e->m_drawLineNo == e->m_curLineNo) {
		e->m_caretPixelX = e->m_drawX;
		e->m_caretPixelY = e->m_drawY + e->m_padTop;
	}
}

/*
 * SetOption(int &): every option is handled under the registry lock. The
 * geometry options then run vvram_update(), rewrite_screen() and
 * check_cursorpos(false). Upper-case TW64/TH64 set the default size, lower-case
 * tw64/th64 (mode 1 only) the field size.
 */

/* FreeType calls: sizes in 26.6 fixed point, 72 dpi. */


/* Palette of the text texture: white, with alpha 0..127 (filled by the
   ClassInitializer). */


struct SteClutEntry {
	unsigned char r, g, b, a;
};
static SteClutEntry s_stePalette[256];

/* The process-wide FreeType library. */

static FT_Library s_fontLibrary;

/* Current glyph: the face chosen by SteLoadGlyph and the request size. */

static FT_Face s_glyphFace;
static int s_glyphSelected;
static int s_glyphWidth;


/* Character class for word motion: 0 space, 1 other half-width, 2 ideographic
   space, 3 other full-width. ucs2_ctype and ucs2_class compute the same value. */


static inline char ucs2_ctype(unsigned short c)
{
	if (c >= 0x100) {
		if (c == 0x3000)
			return 2;
		return 3;
	}
	if (c == 0x20)
		return 0;
	return 1;
}

static inline int ucs2_class(const unsigned short &c)
{
	if (c >= 0x100) {
		if (c == 0x3000)
			return 2;
		return 3;
	}
	if (c == 0x20)
		return 0;
	return 1;
}


/* Decodes one UTF-8 sequence at *pp and advances *pp; a lead byte outside
   0xC2..0xF4 gives s_utf8ReplacementChar. */


static inline unsigned int utf8_decode(unsigned char **pp)
{
	unsigned char *p = *pp;
	unsigned char *q = p + 1;
	unsigned int c = *p;
	unsigned int ch;

	if (*(signed char *)p >= 0) {
		ch = c;
	} else if ((unsigned char)(c + 62) >= 51) {
		ch = s_utf8ReplacementChar;
	} else {
		unsigned int bound;
		ch = c & 0x3f;
		ch = (ch << 6) | (p[1] & 0x3f);
		bound = 2048;
		q = p + 2;
		while (ch >= bound) {
			ch &= bound - 1;
			bound <<= 5;
			ch = (ch << 6) | (*q & 0x3f);
			q++;
		}
	}
	*pp = q;
	return ch;
}


/* Returns the UTF-8 size of a UCS-2 string, terminator included. */
static int SimpleTextEditUcs2ToUtf8Size(const unsigned short *ucs2)
{
	unsigned char buf[16];
	int total = 0;
	int c;

	while ((c = ucs2_next(&ucs2)) != 0) {
		unsigned char *p = buf;

		utf8_encode(&p, c);
		total += p - buf;
	}
	return total + 1;
}
/* Converts a UCS-2 string to UTF-8, terminator included. */
static void SimpleTextEditUcs2ToUtf8(unsigned char *dst, const unsigned short *ucs2)
{
	int c = ucs2_next(&ucs2);

	while (c != 0) {
		utf8_encode(&dst, c);
		c = ucs2_next(&ucs2);
	}
	*dst++ = 0;
}

/* Picks a face holding the character (E, then J, else '?' from J; '*' for
   code 0), loads its glyph and widens req->len to the advance in pixels.
   Returns 0 when no glyph is loaded. */


static int SteLoadGlyph(const SimpleTextEdit *ste, simpleline_chunk *req, int *outWidth)
{
	int code;
	int idx;

	if (req->len < 2)
		req->len = 1;
	*outWidth = req->len;

	code = req->ch;
	if (code == 0)
		code = 42;
	s_glyphWidth = req->len;
	s_glyphSelected = req->selected;

	if (ste->m_fontELoaded == 0)
		return 0;

	idx = FT_Get_Char_Index(ste->m_fontEFace, code);
	if (idx == 0) {
		if (ste->m_fontJLoaded != 0) {
			idx = FT_Get_Char_Index(ste->m_fontJFace, code);
			s_glyphFace = ste->m_fontJFace;
			if (idx == 0)
				idx = FT_Get_Char_Index(ste->m_fontJFace, 63);
		} else {
			s_glyphFace = 0;
		}
	} else {
		s_glyphFace = ste->m_fontEFace;
	}

	if (s_glyphFace == 0)
		return 0;
	if (FT_Load_Glyph(s_glyphFace, idx, 2) != 0)
		return 0;

	{
		int adv = s_glyphFace->glyph->advance.x >> 6;

		if (req->len < adv)
			req->len = adv;
	}
	*outWidth = req->len;
	s_glyphWidth = req->len;
	return 1;
}
/* Draws the loaded glyph into m_buffer, with a diagonal shadow when m_shadowOffset > 0. */
static int SteDrawGlyph(SimpleTextEdit *ste, int y, int x)
{
	FT_Glyph glyph;
	FT_BitmapGlyph bmg;
	FT_Bitmap *bm;
	unsigned char *lo, *hi, *dst;
	int i, j, fill, err, bw, bh, w, shadow;

	err = FT_Get_Glyph(s_glyphFace->glyph, &glyph);
	if (err != 0)
		return 0;

	fill = s_glyphSelected ? 255 : 0;
	for (i = 0; i < ste->m_padTop + ste->m_fontPointSize + ste->m_padBot + 1; i++) {
		for (j = 0; j < s_glyphWidth + ste->m_wrapPad; j++) {
			int off = (x + i) * ste->m_dispWidth + (y + j);

			if (off >= 0 && off < ste->m_dispWidth * ste->m_dispHeight)
				ste->m_buffer[off] = (unsigned char)fill;
		}
	}


	err = FT_Glyph_To_Bitmap(&glyph, 0, 0, 1);  /* NOTE: bug: the error is not checked */
	bmg = (FT_BitmapGlyph)glyph;
	bm = &bmg->bitmap;
	shadow = ste->m_shadowOffset > 0;
	bw = bm->width;
	bh = bmg->bitmap.rows;


	if (bw <= 0) {
		FT_Done_Glyph(glyph);
		return 1;
	}
	if (bh <= 0) {
		FT_Done_Glyph(glyph);
		return 1;
	}
	w = ste->m_dispWidth;
	lo = ste->m_buffer;
	dst = lo + ((y + bmg->left) + (x + ste->m_padTop + ste->m_fontPointSize - bmg->top) * w);
	hi = lo + w * ste->m_dispHeight;
	SteBlendGlyph(ste, dst, 0, lo, hi, bm->buffer, bw, bh, s_glyphSelected, 0);
	if (shadow)
		SteBlendGlyph(ste, dst, ste->m_shadowOffset, lo, hi, bm->buffer, bw, bh, s_glyphSelected, shadow);
	FT_Done_Glyph(glyph);
	return 1;
}
/* Draw thread: renders up to 16 glyphs per round for the visible editors. */
static void *SteFontThread(void *arg)
{
	SimpleTextEdit *e;
	int i, j, n, fresh, w;

	s_fontThreadStarting = 0;
	while (s_fontThreadStarting == 0) {
		j = 0;
		if (pthread_mutex_trylock(&s_registryMutex) != 0) {
			usleep(1000);
			continue;
		}
		for (i = 0; i < STE_MAX_EDITORS; i++) {
			e = s_registry[i];
			if (e && (e->m_needRedraw || e->m_drawLine) && e->visible)
				j = 1;
		}
		if (!j) {
			pthread_mutex_unlock(&s_registryMutex);
			usleep(1000);
			continue;
		}
		n = 0;
		for (i = 0; i < STE_MAX_EDITORS && n < 16; i++) {
			e = s_registry[i];
			if (e == 0 || !e->visible)
				continue;

			if (e->m_needRedraw) {
				e->m_needRedraw = 0;
				if (!e->image_update() && e->m_clearOnRedraw)
					memset(e->m_buffer, 0, e->m_dispWidth * e->m_dispHeight);
				e->m_clearOnRedraw = 0;
				e->m_drawY = 0;
				e->m_drawX = 0;
				e->m_drawRow = 0;
				e->m_drawCol = 0;
				e->m_drawRowCount = 1;
				e->m_drawLine = e->m_firstViewLine;
				e->m_drawChunk = e->m_firstViewCharNo;
				e->m_drawLineNo = e->m_firstViewLineNo;
				e->update_complete_oneline(e->m_drawLine);


				for (j = 0; j <= e->m_firstViewCharNo; j++) {
					if (j >= e->m_drawLine->total_chars)
						break;
					if (e->m_drawX + e->m_drawLine->chunks[j].len >= e->m_dispWidth) {
						e->m_drawX = 0;
						e->m_drawRowCount++;
					}
					e->m_drawX += e->m_drawLine->chunks[j].len + e->m_wrapPad;
				}
				e->m_drawX = 0;
			}

			while (e && e->m_drawLine && n < 16) {
				SteUpdateCaret(e);
				if (e->m_drawChunk >= e->m_drawLine->total_chars) {
					/* end of the logical line: clear the rest of the
					   display row and go to the next line */
					if (e->m_drawLine->disp_lines <= 0)
						e->m_drawLine->disp_lines = e->m_drawRowCount;
					for (j = 0; j < e->m_padTop + e->m_fontPointSize + e->m_padBot + 1; j++) {
						int y = e->m_drawY + j;

						if (y < e->m_dispHeight) {
							int rest = e->m_dispWidth - e->m_drawX;

							if (rest > 0)
								memset(e->m_buffer + (y * e->m_dispWidth + e->m_drawX),
								    0, rest);
						}
					}
					for (j = e->m_drawCol; j < e->m_vvramWidthCache; j++) {
						e->m_vvramSlots[e->m_drawRow][j].ch = 0;
						e->m_vvramSlots[e->m_drawRow][j].selected = 0;
						e->m_vvramSlots[e->m_drawRow][j].w = 0;
						e->m_vvramSlots[e->m_drawRow][j].x = 0;
					}


					e->m_drawLine = e->m_drawLine->next;
					e->m_drawX = 0;
					e->m_drawY += e->m_padTop + e->m_fontPointSize + e->m_padBot;
					e->m_drawCol = 0;
					e->m_drawRow++;
					e->m_drawChunk = 0;
					e->m_drawLineNo++;
					e->m_drawRowCount = 1;
					if (e->m_drawLine == 0) {
						int h = e->m_dispHeight - e->m_drawY;

						if (h > 0) {
							memset(e->m_buffer + e->m_drawY * e->m_dispWidth, 0,
							    e->m_dispWidth * h);
							for (j = e->m_drawRow; j < e->m_vvramCount; j++) {
								unsigned int bits = e->m_vvramWidthCache * 128;
								memset(e->m_vvramSlots[j], 0, bits / 8);
							}
						}
					}
					if (e->m_drawY + e->m_padTop + e->m_fontPointSize >= e->m_dispHeight)
						e->m_drawLine = 0;
					continue;
				}

				/* a character: load its glyph unless the vvram cell
				   already holds it at the same place */
				fresh = 1;
				if (e->m_vvramSlots[e->m_drawRow][e->m_drawCol].ch
				        == e->m_drawLine->chunks[e->m_drawChunk].ch
				    && e->m_vvramSlots[e->m_drawRow][e->m_drawCol].selected
				        == e->m_drawLine->chunks[e->m_drawChunk].selected
				    && e->m_vvramSlots[e->m_drawRow][e->m_drawCol].w
				        == e->m_drawLine->chunks[e->m_drawChunk].len
				    && e->m_vvramSlots[e->m_drawRow][e->m_drawCol].w > 0
				    && e->m_vvramSlots[e->m_drawRow][e->m_drawCol].x == e->m_drawX)
					fresh = 0;
				if (fresh)
					SteLoadGlyph(s_registry[i], &e->m_drawLine->chunks[e->m_drawChunk], &w);
				else
					w = e->m_vvramSlots[e->m_drawRow][e->m_drawCol].w;

				if (e->m_drawX + w >= e->m_dispWidth) {
					/* line wrap: clear the rest of the display row */
					for (j = 0; j < e->m_padTop + e->m_fontPointSize + e->m_padBot + 1; j++) {
						int y = e->m_drawY + j;

						if (y < e->m_dispHeight) {
							int rest = e->m_dispWidth - e->m_drawX;

							if (rest > 0)
								memset(e->m_buffer + (y * e->m_dispWidth + e->m_drawX),
								    0, rest);
						}
					}
					for (j = e->m_drawCol; j < e->m_vvramWidthCache; j++) {
						e->m_vvramSlots[e->m_drawRow][j].ch = 0;
						e->m_vvramSlots[e->m_drawRow][j].selected = 0;
						e->m_vvramSlots[e->m_drawRow][j].w = 0;
						e->m_vvramSlots[e->m_drawRow][j].x = 0;
					}
					e->m_drawX = 0;
					e->m_drawCol = 0;
					e->m_drawY += e->m_padTop + e->m_fontPointSize + e->m_padBot;
					e->m_drawRow++;
					e->m_drawRowCount++;
				}
				if (e->m_drawY + e->m_padTop + e->m_fontPointSize >= e->m_dispHeight)
					e->m_drawLine = 0;
				if (e->m_drawLine == 0)
					break;

				if (!fresh) {
					fresh = 1;
					if (e->m_vvramSlots[e->m_drawRow][e->m_drawCol].ch
					        == e->m_drawLine->chunks[e->m_drawChunk].ch
					    && e->m_vvramSlots[e->m_drawRow][e->m_drawCol].selected
					        == e->m_drawLine->chunks[e->m_drawChunk].selected
					    && e->m_vvramSlots[e->m_drawRow][e->m_drawCol].w
					        == e->m_drawLine->chunks[e->m_drawChunk].len
					    && e->m_vvramSlots[e->m_drawRow][e->m_drawCol].w > 0
					    && e->m_vvramSlots[e->m_drawRow][e->m_drawCol].x == e->m_drawX)
						fresh = 0;
					if (fresh)
						SteLoadGlyph(s_registry[i], &e->m_drawLine->chunks[e->m_drawChunk], &w);
					else
						w = e->m_vvramSlots[e->m_drawRow][e->m_drawCol].w;
				}
				if (fresh) {
					SteDrawGlyph(s_registry[i], e->m_drawX, e->m_drawY);
					e->m_vvramSlots[e->m_drawRow][e->m_drawCol].ch =
					    e->m_drawLine->chunks[e->m_drawChunk].ch;
					e->m_vvramSlots[e->m_drawRow][e->m_drawCol].selected =
					    e->m_drawLine->chunks[e->m_drawChunk].selected;
					e->m_vvramSlots[e->m_drawRow][e->m_drawCol].w = w;
					n++;
					e->m_vvramSlots[e->m_drawRow][e->m_drawCol].x = e->m_drawX;
				}
				SteUpdateCaret(e);
				e->m_drawX += w + e->m_wrapPad;
				e->m_drawChunk++;
				e->m_drawCol++;
			}
			if (e)
				SteUpdateCaret(e);
		}
		pthread_mutex_unlock(&s_registryMutex);
	}
	return 0;
}

/* Module setup: palette, registry, FreeType and the draw thread. */
SimpleTextEdit::ClassInitializer::ClassInitializer()
{
	for (int i = 0; i < 256; i++) {
		s_stePalette[i].r = 255;
		s_stePalette[i].g = 255;
		s_stePalette[i].b = 255;
		s_stePalette[i].a = i / 2;
	}

	pthread_mutex_init(&s_registryMutex, 0);
	memset(s_registry, 0, sizeof(s_registry));

	if (FT_Init_FreeType(&s_fontLibrary))
		return;

	pthread_attr_t attr;
	int policy;
	struct sched_param param;

	pthread_attr_init(&attr);
	pthread_attr_getschedpolicy(&attr, &policy);
	pthread_attr_getschedparam(&attr, &param);
	if (pthread_attr_setschedpolicy(&attr, 0) == 0) {
		int pri = sched_get_priority_max(0);
		if (pri != -1) {
			param.sched_priority = pri;
			pthread_attr_setschedparam(&attr, &param);
		}
		pthread_attr_setinheritsched(&attr, 1);
	}

	s_fontThreadStarting = 1;
	pthread_create(&s_fontThreadId, &attr, SteFontThread, 0);
	while (s_fontThreadStarting)
		usleep(30);
}


/* Returns with m_ctorFailed set when memory runs out or all STE_MAX_EDITORS registry slots
   are taken. */


SimpleTextEdit::SimpleTextEdit(Object *parent, int mode) : Object(parent)
{


	EffOptionState *os = &option_state;


	int i;
	os->always_mask |= EFF_TEXTURE_CLAMP | EFF_DIFFUSE;
	os->always_mask |= EFF_PIXEL_TEST | EFF_PIXEL_WRITE;

	m_mode = 0;
	if (mode == 1)
		m_mode = mode;

	m_modifyVerifyCallback = 0;
	m_wideModeCheckFunction = 0;
	m_wideMode = 0;
	m_wideModeDelay = 0;
	m_fieldSizeFlag = 0;
	m_fontPointSize = 16;


	{
		char *td = m_tag;
		int i = 3;
		const char *ts = kTag;
		for (; i != -1; i--)
			*td++ = *ts++;
	}

	m_defWidth = 128;
	m_bufWidth = 128;
	m_defHeight = 128;
	m_bufHeight = 128;
	m_fieldWidth = 0;
	m_fieldHeight = 0;
	m_insertToggle = 1;
	m_cursorSwitch = 1;
	m_insertMode = 1;
	m_shadowOffset = 0;
	m_verifyInProgress = 0;
	m_wrapPad = 0;
	m_padBot = 2;
	m_padTop = 1;
	m_ctorFailed = 0;
	m_registryIndex = -1;
	m_needRedraw = 1;
	m_clearOnRedraw = 1;
	m_drawLine = 0;


	m_fontELoaded = 0;
	strcpy(m_fontEPath.c, "/opt0/bn/fonts/SGR_SE1.PST");
	m_fontJLoaded = 0;
	strcpy(m_fontJPath.c, "/opt0/bn/fonts/SGR_SCE.PST");

	m_curPixelPos = 0;
	m_curCharNo = 0;
	m_endCharNo = 0;
	m_firstViewCharNo = 0;
	m_texHeight = -1;
	m_texWidth = -1;
	m_texture = -1;
	m_clut = -1;
	m_dispWidth = 124;
	m_dispHeight = 124;

	m_buffer = (unsigned char *)malloc(124 * 124);
	if (!m_buffer) {
		m_ctorFailed = 1;
		return;
	}
	memset(m_buffer, 0, m_dispWidth * m_dispHeight);

	m_vvramCount = 0;
	m_vvramWidthCache = 0;
	for (i = 0; i < STE_VVRAM_ROWS; i++)
		m_vvramSlots[i] = 0;

	vvram_update();

	m_cursorBlinkCounter = 15;
	m_cursorBlinkTarget = 0;
	m_cursorBlinkFlag = 1;
	m_caretPixelX = -1;
	m_caretPixelY = -1;
	m_selRangeStart = 0;

	simpleline *line = alloc_text(0);
	m_textHead = line;
	m_selRangeEnd = 0;
	m_textTail = line;
	m_firstViewLineNo = 0;
	m_firstViewLine = line;
	m_endLineNo = 0;
	m_curLineNo = 0;
	m_curLine = line;

	m_selToLine = 0;
	m_selFromLine = 0;
	m_markLine = 0;
	m_selToRow = -1;
	m_selFromRow = -1;
	m_markRow = -1;
	m_markCol = 0;
	m_selToCol = 0;
	m_selFromCol = 0;
	m_clipLen = 0;
	m_clipBuf = 0;
	m_ringHead = 0;
	m_ringTail = 0;
	memset(m_ring, 0, sizeof(SimpleTextEdit_RingEntry));

	int ok = 0;
	m_gssy = -1;

	fontface_update();

	pthread_mutex_lock(&s_registryMutex);
	for (i = 0; i < STE_MAX_EDITORS; i++) {
		if (!s_registry[i]) {
			ok = 1;
			m_registryIndex = i;
			s_registry[i] = this;
			break;
		}
	}
	pthread_mutex_unlock(&s_registryMutex);

	if (!ok) {
		m_ctorFailed = 1;
		return;
	}

	for (int j = 0; j < 4; j++) {
		m_textQuad[j] = kTextQuad[j];
		m_fieldQuad[j] = kFieldQuad[j];
		m_cursorQuad[j] = kCursorQuad[j];
	}
}
/* Unregisters the editor and frees its lines, queued strings, buffers and fonts. */
SimpleTextEdit::~SimpleTextEdit()
{
	if (m_registryIndex >= 0) {
		pthread_mutex_lock(&s_registryMutex);
		s_registry[m_registryIndex] = 0;
		pthread_mutex_unlock(&s_registryMutex);
	}

	simpleline *prev = 0;
	for (simpleline *p = m_textHead; p; p = p->next) {
		if (prev)
			free_text(prev);
		prev = p;
	}
	if (prev)
		free_text(prev);

	if (m_clipBuf) {
		free(m_clipBuf);
		m_clipBuf = 0;
		m_clipLen = 0;
	}

	for (int n = 0; n < 30 && m_ringHead != m_ringTail; n++) {
		if (m_ring[m_ringHead].strCopy) {
			free((void *)m_ring[m_ringHead].strCopy);
			m_ring[m_ringHead].strCopy = 0;
		}
		m_ringHead++;
		if (m_ringHead >= STE_RING_SIZE)
			m_ringHead = 0;
	}

	if (m_buffer)
		free(m_buffer);

	if (m_fontELoaded) {
		FT_Done_Face(m_fontEFace);
		m_fontELoaded = 0;
	}
	if (m_fontJLoaded) {
		FT_Done_Face(m_fontJFace);
		m_fontJLoaded = 0;
	}
	if (m_texture > 0)
		tmRelease(m_texture);
	if (m_clut > 0)
		tmRelease(m_clut);

	for (int i = 0; i < STE_VVRAM_ROWS; i++) {
		if (m_vvramSlots[i])
			free(m_vvramSlots[i]);
		m_vvramSlots[i] = 0;
	}
}


/* Allocates a zeroed line with room for count + 1 characters; 0 on failure. */

inline SimpleTextEdit::simpleline *SimpleTextEdit::alloc_text(int count)
{
	int size = (count + 1) * 12 + 28;
	simpleline *line = (simpleline *)malloc(size);
	if (!line)
		return 0;
	memset(line, 0, size);
	line->total_chars = count;
	line->alloc_chars = count;
	line->disp_lines = 0;
	return line;
}


inline void SimpleTextEdit::free_text(simpleline *line)
{
	if (line)
		free(line);
}


/* Puts newLine in place of oldLine in the line pointers and frees oldLine. */
inline void SimpleTextEdit::replace_text(simpleline *oldLine, simpleline *newLine)
{
	if (m_textHead == oldLine)
		m_textHead = newLine;
	if (m_textTail == oldLine)
		m_textTail = newLine;
	if (m_firstViewLine == oldLine)
		m_firstViewLine = newLine;
	if (m_curLine == oldLine)
		m_curLine = newLine;
	if (oldLine)
		free(oldLine);
}

/* Character class for word wrap: bit 1 = full-width (>= 0x100), bit 0 = not a
   space (0x20 and the ideographic space 0x3000 are spaces). */

inline int SimpleTextEdit::ucs2_characterization(unsigned short c)
{
	if (c < 0x100)
		return c != 0x20;
	return (c ^ 0x3000) ? 3 : 2;
}

/* Returns the number of display rows of line, computing and caching it when it
   is not known. */


inline int SimpleTextEdit::update_complete_oneline(simpleline *line) const
{
	if (line->disp_lines > 0)
		return line->disp_lines;
	int used = 0;
	int subline = 1;


	for (int i = 0; i < line->total_chars; i++) {
		int w;
		int charLen = line->chunks[i].len;
		if (charLen <= 0)
			SteLoadGlyph(this, &line->chunks[i], &w);
		else
			w = charLen;
		if (used + w >= m_dispWidth) {
			used = 0;
			subline++;
		}
		used += w + m_wrapPad;
	}
	line->disp_lines = subline;
	return subline;
}

/* Returns the pixel x of character col within its display row; computes the
   line wrap first when it is not cached. */


int SimpleTextEdit::calc_curpos(simpleline *line, int col)
{


	int running = 0;
	if (line->disp_lines <= 0) {
		int used = 0;
		int subline = 1;


		for (int i = 0; i < line->total_chars; i++) {
			int w;
			int charLen = line->chunks[i].len;
			if (charLen <= 0)
				SteLoadGlyph(this, &line->chunks[i], &w);
			else
				w = charLen;
			if (used + w >= m_dispWidth) {
				used = 0;
				subline++;
			}
			used += w + m_wrapPad;
		}
		line->disp_lines = subline;
	}

	int i;


	for (i = 0; i < col; i++) {
		int len = line->chunks[i].len;
		if (running + len >= m_dispWidth)
			running = 0;
		running += len + m_wrapPad;
	}
	if (running >= m_dispWidth)
		running = 0;
	if (i < line->total_chars && running + line->chunks[i].len >= m_dispWidth)
		running = 0;
	return running;
}

/* Walks the visible text from m_firstViewLine, recording the caret pixel position on
   the way, and stores the last row and column reached in m_endLineNo and
   m_endCharNo. Returns 1 when the walk ends at the cursor and the cursor is not
   fully visible. */


int SimpleTextEdit::calc_drawend_pos()
{
	int pixY = 0;
	int ret = 0;
	int pixX = 0;
	int subline = 1;
	simpleline *line = m_firstViewLine;
	int col = m_firstViewCharNo;
	int row = m_firstViewLineNo;

	update_complete_oneline(line);

	int j = 0;
	if (m_firstViewCharNo >= 0 && line->total_chars > 0) {  /* redundant as written */
		do {
			int len = line->chunks[j].len;
			if (pixX + len >= m_dispWidth) {
				pixX = 0;
				subline++;
			}
			pixX += len + m_wrapPad;
			j++;
			if (m_firstViewCharNo < j)
				break;
		} while (j < line->total_chars);
	}

	pixX = 0;
	while (line) {
		if (col == m_curCharNo && row == m_curLineNo) {
			m_caretPixelX = pixX;
			m_caretPixelY = pixY + m_padTop;
		}
		if (col >= line->total_chars) {
			if (line->disp_lines <= 0)
				line->disp_lines = subline;
			pixX = 0;
			col = 0;
			row++;
			subline = 1;
			line = line->next;
			pixY += m_padTop + m_fontPointSize + m_padBot;
			if (pixY + m_padTop + m_fontPointSize < m_dispHeight) {
				/* visible */
			} else {
				line = 0;
			}
		} else {
			int w;
			int charLen = line->chunks[col].len;
			if (charLen <= 0)
				SteLoadGlyph(this, &line->chunks[col], &w);
			else
				w = charLen;
			if (pixX + w >= m_dispWidth) {
				pixX = 0;
				subline++;
				pixY += m_padTop + m_fontPointSize + m_padBot;
			}
			int y = pixY + m_padTop;
			if (y + m_fontPointSize < m_dispHeight) {
				/* visible */
			} else {
				line = 0;
			}
			if (!line)
				break;
			if (col == m_curCharNo && row == m_curLineNo) {
				m_caretPixelX = pixX;
				m_caretPixelY = y;
			}
			pixX += w + m_wrapPad;
			col++;
		}
	}

	if (col == m_curCharNo && row == m_curLineNo) {
		int y = pixY + m_padTop;
		m_caretPixelX = pixX;
		if (y + m_fontPointSize < m_dispHeight)
			ret = 0;
		else
			ret = 1;
		m_caretPixelY = y;
	}
	m_endLineNo = row;
	m_endCharNo = col;
	return ret;
}

/* Moves (*pLine, *pRow, *pSub) up by step display rows, keeping the pixel
   column pix; returns 0 when the position did not change. */


int SimpleTextEdit::up_line(int step, simpleline **pLine, int *pRow, int *pSub, int pix)
{
	int running;
	int startSub;
	simpleline *first;
	int subline;
	int col;

	pix = pix > -1 ? pix : 0;
	first = *pLine;
	startSub = *pSub;
	update_complete_oneline(first);

	running = 0;
	subline = 0;
	for (col = 0; col < *pSub; col++) {
		int len = (*pLine)->chunks[col].len;
		if (running + len >= m_dispWidth) {
			running = 0;
			subline++;
		}
		running += len + m_wrapPad;
	}
	if (running >= m_dispWidth) {
		running = 0;
		subline++;
	}
	if (col < (*pLine)->total_chars) {
		if (running + (*pLine)->chunks[col].len >= m_dispWidth) {
			running = 0;
			subline++;
		}
	}

	while (step > 0) {
		running = 0;
		if (subline <= 0) {
			if ((*pLine)->prev == 0)
				break;
			*pLine = (*pLine)->prev;
			(*pRow)--;
			subline = update_complete_oneline(*pLine);
		}
		subline--;
		step--;
	}

	running = 0;
	for (col = 0; col < (*pLine)->total_chars; col++) {
		int w = (*pLine)->chunks[col].len;
		if (running + w >= m_dispWidth) {
			running = 0;
			subline--;
		}
		if (pix < running + w && subline == 0)
			break;
		running += (*pLine)->chunks[col].len + m_wrapPad;
		if (col + 1 < (*pLine)->total_chars && running + (*pLine)->chunks[col + 1].len >= m_dispWidth) {
			if (subline == 0 && pix > 0)
				break;
			running = 0;
			subline--;
		}
	}
	*pSub = col;

	if (startSub == col && first == *pLine)
		return 0;
	return 1;
}

/* Moves (*pLine, *pRow, *pSub) down by step display rows, keeping the pixel
   column pix; returns 0 when the position did not change. */


int SimpleTextEdit::down_line(int step, simpleline **pLine, int *pRow, int *pSub, int pix)
{
	int running = 0;
	int startSub;
	simpleline *first;
	simpleline *line;
	int subline;
	int col;
	int maxSub;

	int px = pix > -1 ? pix : 0;
	startSub = *pSub;
	first = *pLine;

	maxSub = update_complete_oneline(first) - 1;

	subline = 0;
	col = 0;
	{

		for (col = 0; col < *pSub; col++) {
			int len = (*pLine)->chunks[col].len;
			if (running + len >= m_dispWidth) {
				running = 0;
				subline++;
			}
			running += len + m_wrapPad;
		}
		if (running >= m_dispWidth) {
			running = 0;
			subline++;
		}
		if (col < (*pLine)->total_chars) {
			if (running + (*pLine)->chunks[col].len >= m_dispWidth) {
				running = 0;
				subline++;
			}
		}
	}

	while (step > 0) {
		if (subline < maxSub) {
			subline++;
		} else {
			if ((*pLine)->next == 0)
				break;
			*pLine = (*pLine)->next;
			(*pRow)++;
			maxSub = update_complete_oneline(*pLine) - 1;
			subline = 0;
		}
		step--;
	}

	running = 0;
	line = *pLine;
	for (col = 0; col < (*pLine)->total_chars; col++) {
		if (running + line->chunks[col].len >= m_dispWidth) {
			if (subline == 0 && px > 0) {
				if (col > 0)
					col--;
				break;
			}
			running = 0;
			subline--;
		}
		int w = line->chunks[col].len;
		if (px < running + w && subline == 0)
			break;
		running += w + m_wrapPad;
	}
	*pSub = col;

	if (startSub == col && first == *pLine)
		return 0;
	return 1;
}

/* Recomputes the cached wrap of up to two lines before m_curLine (from
   m_curLine back), then of up to two lines after it; computed lines are skipped. */
/* NOTE: bug: declared int, but returns no value. */


int SimpleTextEdit::update_bg_displines()
{
	int n = 0;

	for (simpleline *line = m_curLine; line; line = line->prev) {
		if (line->disp_lines <= 0) {
			int used = 0;
			int subline = 1;
			for (int i = 0; i < line->total_chars; i++) {
				int w;
				int charLen = line->chunks[i].len;
				if (charLen <= 0)
					SteLoadGlyph(this, &line->chunks[i], &w);
				else
					w = charLen;
				if (used + w >= m_dispWidth) {
					used = 0;
					subline++;
				}
				used += w + m_wrapPad;
			}
			line->disp_lines = subline;
			n++;
			if (n >= 2)
				break;
		}
	}

	n = 0;
	for (simpleline *line = m_curLine; line; line = line->next) {
		if (line->disp_lines <= 0) {
			int used = 0;
			int subline = 1;
			for (int i = 0; i < line->total_chars; i++) {
				int w;
				int charLen = line->chunks[i].len;
				if (charLen <= 0)
					SteLoadGlyph(this, &line->chunks[i], &w);
				else
					w = charLen;
				if (used + w >= m_dispWidth) {
					used = 0;
					subline++;
				}
				used += w + m_wrapPad;
			}
			line->disp_lines = subline;
			n++;
			if (n >= 2)
				break;
		}
	}
}

int SimpleTextEdit::fontface_update()
{
	if (m_fontELoaded) {
		FT_Done_Face(m_fontEFace);
		m_fontELoaded = 0;
	}
	if (FT_New_Face(s_fontLibrary, m_fontEPath.c, 0, &m_fontEFace))
		return 0;
	m_fontELoaded = 1;
	if (FT_Set_Char_Size(m_fontEFace, m_fontPointSize << 6, m_fontPointSize << 6, 72, 72))
		return 0;

	if (m_fontJLoaded) {
		FT_Done_Face(m_fontJFace);
		m_fontJLoaded = 0;
	}
	if (FT_New_Face(s_fontLibrary, m_fontJPath.c, 0, &m_fontJFace))
		return 0;
	m_fontJLoaded = 1;
	if (FT_Set_Char_Size(m_fontJFace, m_fontPointSize << 6, m_fontPointSize << 6, 72, 72))
		return 0;

	vvram_update();
	return 1;
}

/* Reallocates the vvram rows for the current field and font size; sets
   m_ctorFailed when memory runs out. */


int SimpleTextEdit::vvram_update()
{
	int divisor = m_fontPointSize + m_padTop + m_padBot;
	m_vvramWidthCache = m_dispWidth;
	m_vvramCount = m_dispHeight / divisor + 1;

	for (int i = 0; i < STE_VVRAM_ROWS; i++) {
		if (m_vvramSlots[i])
			free(m_vvramSlots[i]);
		m_vvramSlots[i] = 0;
	}

	for (int i = 0; i < m_vvramCount; i++) {
		SteVvramCell *p = (SteVvramCell *)malloc((unsigned)(m_vvramWidthCache << 7) >> 3);
		m_vvramSlots[i] = p;
		if (!p) {
			m_ctorFailed = 1;
			return 0;
		}
		memset(p, 0, (unsigned)(m_vvramWidthCache << 7) >> 3);
	}
	return 1;
}


/* Scrolls the image buffer and the vvram rows up by n text rows, clearing the
   bottom row. */


int SimpleTextEdit::vvram_upscroll(int n)
{
	for (int i = 0; i < n; i++) {
		int acc = 0;
		int k = 0;
		while (k < m_vvramCount - 1) {
			if (m_dispHeight - (m_fontPointSize + m_padTop + m_padBot) * 2 >= acc) {
				memcpy(m_buffer + acc * m_dispWidth,
				    m_buffer + (acc + m_fontPointSize + m_padTop + m_padBot) * m_dispWidth,
				    m_dispWidth * (m_fontPointSize + m_padTop + m_padBot));
				unsigned int bits = m_vvramWidthCache * 128;
				memcpy(m_vvramSlots[k], m_vvramSlots[k + 1], bits / 8);
			} else {
				if (acc < m_dispHeight)
					memset(m_buffer + acc * m_dispWidth, 0, m_dispWidth * (m_dispHeight - acc));
				unsigned int bits = m_vvramWidthCache * 128;
				memset(m_vvramSlots[k], 0, bits / 8);
			}
			k++;
			acc += m_fontPointSize + m_padTop + m_padBot;
		}
		if (acc < m_dispHeight)
			memset(m_buffer + acc * m_dispWidth, 0, m_dispWidth * (m_dispHeight - acc));
		unsigned int bits = m_vvramWidthCache * 128;
		memset(m_vvramSlots[k], 0, bits / 8);
	}
	return 1;
}

/* Scrolls the image buffer and the vvram rows down by n text rows, clearing
   the top row. */


int SimpleTextEdit::vvram_downscroll(int n)
{
	if (m_dispHeight - (m_fontPointSize + m_padTop + m_padBot) * 3 <= 0)
		return 0;

	for (int i = 0; i < n; i++) {
		int cnt = 0;
		int acc = 0;
		int lh = m_fontPointSize + m_padTop + m_padBot;


		if (m_dispHeight - lh * 3 >= 0) {
			do {
				cnt++;
				acc += lh;
			} while (m_dispHeight - lh * 3 >= acc);
		}


		for (; cnt >= 0; cnt--) {
			int off = (acc + m_fontPointSize + m_padTop + m_padBot) * m_dispWidth;
			if (off >= 0)
				memcpy(m_buffer + off, m_buffer + acc * m_dispWidth,
				    m_dispWidth * (m_fontPointSize + m_padTop + m_padBot));
			unsigned int bits = m_vvramWidthCache * 128;
			memcpy(m_vvramSlots[cnt + 1], m_vvramSlots[cnt], bits / 8);
			acc -= m_fontPointSize + m_padTop + m_padBot;
		}
		memset(m_buffer, 0, m_dispWidth * (m_fontPointSize + m_padTop + m_padBot));
		unsigned int bits0 = m_vvramWidthCache * 128;
		memset(m_vvramSlots[0], 0, bits0 / 8);
	}
	return 1;
}

/* Scrolls the view until the cursor is visible; doScroll also scrolls the
   vvram image. */
inline void SimpleTextEdit::check_cursorpos(bool doScroll)
{
	while (
	    m_curLineNo < m_firstViewLineNo || (m_curLineNo == m_firstViewLineNo && m_curCharNo < m_firstViewCharNo)) {
		if (up_line(1, &m_firstViewLine, &m_firstViewLineNo, &m_firstViewCharNo, 0) && doScroll)
			vvram_downscroll(1);
		m_needRedraw = 1;
		if (m_curLineNo == m_firstViewLineNo)
			break;
	}

	int r = calc_drawend_pos();
	while (m_endLineNo < m_curLineNo || r || (m_endLineNo == m_curLineNo && m_endCharNo < m_curCharNo)) {
		if (down_line(1, &m_firstViewLine, &m_firstViewLineNo, &m_firstViewCharNo, 0) && doScroll)
			vvram_upscroll(1);
		r = calc_drawend_pos();
		m_needRedraw = 1;
	}
}

inline void SimpleTextEdit::rewrite_screen()
{
	for (simpleline *l = m_textHead; l; l = l->next) {
		l->disp_lines = 0;
		for (int i = 0; i < l->total_chars; i++)
			l->chunks[i].len = 0;
	}
	m_needRedraw = 1;
}

inline int SimpleTextEdit::calc_text_bytes() const
{
	int n = 0;
	for (simpleline *l = m_textHead; l; l = l->next)
		n += l->total_chars + 1;
	return n;
}

inline int SimpleTextEdit::calc_text_lines() const
{
	int n = 0;
	for (simpleline *l = m_textHead; l; l = l->next)
		n++;
	return n;
}


inline int SimpleTextEdit::calc_text_displines() const
{
	int total = 0;
	for (simpleline *line = m_textHead; line; line = line->next) {
		if (line->disp_lines <= 0) {
			int used = 0;
			int subline = 1;


			for (int i = 0; i < line->total_chars; i++) {
				int w;
				int charLen = line->chunks[i].len;
				if (charLen <= 0)
					SteLoadGlyph(this, &line->chunks[i], &w);
				else
					w = charLen;
				if (used + w >= m_dispWidth) {
					used = 0;
					subline++;
				}
				used += w + m_wrapPad;
			}
			line->disp_lines = subline;
		}
		total += line->disp_lines;
	}
	return total;
}

inline int SimpleTextEdit::image_update()
{
	int newWidth = m_bufWidth - 4;
	if (m_dispWidth != newWidth || m_dispHeight != m_bufHeight - 4) {
		m_dispWidth = newWidth;
		m_dispHeight = m_bufHeight - 4;
		if (m_buffer)
			free(m_buffer);
		m_buffer = (unsigned char *)malloc(m_dispWidth * m_dispHeight);
		if (!m_buffer) {
			m_ctorFailed = 1;
			return 1;
		}
		memset(m_buffer, 0, m_dispWidth * m_dispHeight);
		vvram_update();
		return 1;
	}
	return 0;
}

inline int SimpleTextEdit::vvram_attrupdate()
{
	return 1;
}

/* Restarts the cursor blink. */


inline void SimpleTextEdit::reset_cursorblink()
{
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
}

/* Creates the CLUT and the GS texture of the field (format 19) when needed and
   uploads m_buffer; returns 0 when there is no buffer or a create fails. */

inline int SimpleTextEdit::DrawThis_update(gm_graph &g)
{
	int tm = g.GetTextureManager();
	if (!m_buffer)
		return 0;

	if (m_clut <= 0) {
		m_clut = tmCreateClut(tm, 19, 0);
		if (m_clut <= 0) {
			m_clut = -1;
			return 0;
		}
		tmUpdateClut(m_clut, 0, s_stePalette);
	}

	if (m_texWidth != m_dispWidth || m_texHeight != m_dispHeight || m_texture <= 0) {
		if (m_texture > 0)
			tmRelease(m_texture);
		m_texWidth = m_dispWidth;
		m_texHeight = m_dispHeight;
		m_texture = tmCreateTexture(tm, m_dispWidth, m_dispHeight, PS2_GS_PSMT8);
		if (m_texture <= 0)
			return 0;
		if (m_clut > 0)
			tmSetClut(m_texture, m_clut);
	}

	tmUpdateTexture(m_texture, m_texWidth, m_texHeight, 19, m_buffer);
	return 1;
}


/* The first two phases are skipped while the registry lock is busy: the field
   size switch asked by the wide-mode check, then the queued actions and the
   texture update. The third phase draws five passes. */


void SimpleTextEdit::DrawThis(gm_graph &g)
{


	float ox = 0.0f;

	if (m_ctorFailed)
		return;

	if (pthread_mutex_trylock(&s_registryMutex) == 0) {
		if (m_mode && m_wideModeCheckFunction && m_fieldSizeFlag) {
			int on = m_wideModeCheckFunction() != 0;
			/* The size switches once the wide-mode state has differed for 10 calls. */


			if (m_wideMode != on) {
				if (m_wideModeDelay <= 0) {
					if (!on) {


						if (m_fieldHeight > 0)
							m_bufHeight = m_fieldHeight;
						else
							m_bufHeight = m_defHeight;
						if (m_fieldWidth > 0)
							m_bufWidth = m_fieldWidth;
						else
							m_bufWidth = m_defWidth;
					} else {
						m_bufHeight = m_defHeight;
						m_bufWidth = m_defWidth;
					}
					m_wideMode = on;
					m_wideModeDelay = 10;
					image_update();
					m_firstViewCharNo = 0;
					rewrite_screen();
					check_cursorpos(false);
				} else {
					m_wideModeDelay--;
				}
			} else {
				m_wideModeDelay = 10;
			}
		}
unlock:
		pthread_mutex_unlock(&s_registryMutex);
	}

	if (pthread_mutex_trylock(&s_registryMutex) == 0) {
		if (!Action_exec())
			update_bg_displines();
		check_cursorpos(true);
		int ok = DrawThis_update(g);
		pthread_mutex_unlock(&s_registryMutex);
		if (!ok)
			return;
	}

	if (!visible)
		return;

	m_cursorBlinkTarget = m_cursorBlinkTarget + 1;
	m_cursorBlinkFlag = m_cursorBlinkTarget % (m_cursorBlinkCounter * 2) > m_cursorBlinkCounter;

	/* P1: field quad (m_fieldQuad) */
	float h = (float)m_bufHeight;
	float w = (float)m_bufWidth;
	m_fieldQuad[0].x = 0.0f;
	m_fieldQuad[0].y = 0.0f;
	m_fieldQuad[1].x = 0.0f;
	m_fieldQuad[1].y = ox - h;
	m_fieldQuad[2].x = ox + w;
	m_fieldQuad[2].y = 0.0f;
	m_fieldQuad[3].x = ox + w;
	m_fieldQuad[3].y = ox - h;

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE);
	g.BlendFunc(GM_BLEND_ALPHA);
	g.Diffuse(*option_state.GetDiffuse());
	g.SetArrays(&m_fieldQuad[0], GM_VERTEX_PC, 4, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);

	/* P2: static quad A, the field grown by half its size on each side */
	s_steQuadA[0].x = ox - (m_bufWidth >> 1);
	s_steQuadA[0].y = ox + (m_bufHeight >> 1);
	s_steQuadA[1].x = ox - (m_bufWidth >> 1);
	s_steQuadA[1].y = ox - m_bufHeight - (m_bufHeight >> 1);
	s_steQuadA[2].x = ox + m_bufWidth + (m_bufWidth >> 1);
	s_steQuadA[2].y = ox + (m_bufHeight >> 1);
	s_steQuadA[3].x = ox + m_bufWidth + (m_bufWidth >> 1);
	s_steQuadA[3].y = ox - m_bufHeight - (m_bufHeight >> 1);

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_STENCIL_TEST);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE);
	g.BlendFunc(GM_BLEND_KEEP);
	static vec4 s_steShadow(0.0f, 0.0f, 0.0f, 0.0f);
	g.Diffuse(s_steShadow);
	g.SetArrays(s_steQuadA, GM_VERTEX_PC, 4, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);

	/* P3: static quad B over the field, with stencil */
	s_steQuadB[0].x = 0.0f;
	s_steQuadB[0].y = 0.0f;
	s_steQuadB[1].x = 0.0f;
	s_steQuadB[1].y = ox - m_bufHeight;
	s_steQuadB[2].x = ox + m_bufWidth;
	s_steQuadB[2].y = 0.0f;
	s_steQuadB[3].x = ox + m_bufWidth;
	s_steQuadB[3].y = ox - m_bufHeight;

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_STENCIL_TEST);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE);
	g.BlendFunc(GM_BLEND_KEEP);
	static vec4 s_steStencil(0.0f, 0.0f, 0.0f, 1.0f);
	g.Diffuse(s_steStencil);
	g.SetArrays(s_steQuadB, GM_VERTEX_PC, 4, 1, 0, 0);
	g.StencilOp(1, 1, 3);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);
	g.StencilOp(1, 1, 1);

	/* P4: the text texture (m_textQuad), once the CLUT exists */

	if (m_texture > 0 && m_clut > 0) {
		float th = (float)m_bufHeight;
		float tw = (float)m_bufWidth;
		float x0 = ox + 2.0f;
		float y0 = ox - 2.0f;
		float y1 = (ox - th) + 2.0f;
		float x1 = (ox + tw) - 2.0f;
		m_textQuad[0].x = x0;
		m_textQuad[0].y = y0;
		m_textQuad[1].x = x0;
		m_textQuad[1].y = y1;
		m_textQuad[2].x = x1;
		m_textQuad[2].y = y0;
		m_textQuad[3].x = x1;
		m_textQuad[3].y = y1;

		g.BlendFunc(GM_BLEND_ALPHA);
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_STENCIL_TEST);
		g.StencilFunc(GM_EQUAL, 1);
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE_FIT);
		g.TexWrap(GM_WRAP_CLAMP, GM_WRAP_CLAMP);
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
		g.Diffuse(*option_state.GetDiffuse());
		g.SetTexture(m_texture);
		g.SetArrays(&m_textQuad[0], GM_VERTEX_PCT, 4, 1, 0, 0);
		g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);
	}

	/* P5: blinking cursor (m_cursorQuad) */
	if (!m_cursorSwitch)
		return;
	if (m_caretPixelX < 0)
		return;
	if (!m_cursorBlinkFlag)
		return;

	float cx = (float)m_caretPixelX;
	float cy = (float)m_caretPixelY;
	float bx0 = (cx - 0.5f) + 2.0f;
	float bx1 = (cx + 0.5f) + 2.0f;
	float by0 = -((cy + 2.0f) + 2.0f);
	float by1 = -(((float)(m_caretPixelY + m_fontPointSize) + 2.0f) + 1.0f);
	m_cursorQuad[0].x = bx0;
	m_cursorQuad[0].y = by0;
	m_cursorQuad[1].x = bx0;
	m_cursorQuad[1].y = by1;
	m_cursorQuad[2].x = bx1;
	m_cursorQuad[2].y = by0;
	m_cursorQuad[3].x = bx1;
	m_cursorQuad[3].y = by1;

	g.SetEnableBits(g.fFlagsE | GM_ENABLE_ANTIALIAS);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_STENCIL_TEST);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE);
	g.BlendFunc(GM_BLEND_ALPHA);
	g.Diffuse(*option_state.GetDiffuse());
	g.SetArrays(&m_cursorQuad[0], GM_VERTEX_PC, 4, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_ANTIALIAS);
}

/* Sets the buffer and default size (at most 640 x 480). When the display size
   changes the image buffer is reallocated; then the wrap is recomputed and the
   cursor kept visible. */


int SimpleTextEdit::SetFieldSize(int w, int h)
{
	if (w > 640)
		w = 640;
	else if (w < 0)
		w = 1;
	if (h > 480)
		h = 480;
	else if (h < 0)
		h = 1;

	pthread_mutex_lock(&s_registryMutex);
	m_bufWidth = w;
	m_defWidth = w;
	m_bufHeight = h;
	m_defHeight = h;

	if (m_dispWidth != w - 4 || m_dispHeight != h - 4) {
		m_dispWidth = w - 4;
		m_dispHeight = h - 4;
		if (m_buffer)
			free(m_buffer);
		m_buffer = (unsigned char *)malloc(m_dispWidth * m_dispHeight);
		if (!m_buffer) {
			m_ctorFailed = 1;
		} else {
			memset(m_buffer, 0, m_dispWidth * m_dispHeight);
			vvram_update();
		}
	}

	m_firstViewCharNo = 0;
	rewrite_screen();
	check_cursorpos(false);
	pthread_mutex_unlock(&s_registryMutex);
	return 1;
}

inline int SimpleTextEdit::SetFieldSize2(int w, int h)
{
	if (w > 640)
		w = 640;
	else if (w < 0)
		w = 1;
	if (h > 480)
		h = 480;
	else if (h < 0)
		h = 1;
	pthread_mutex_lock(&s_registryMutex);
	m_fieldWidth = w;
	m_fieldHeight = h;
	m_fieldSizeFlag = 1;
	pthread_mutex_unlock(&s_registryMutex);
	return 1;
}

inline int SimpleTextEdit::check_modify_verify_callback(int option, void *data)
{
	if (!m_modifyVerifyCallback)
		return 1;
	m_verifyInProgress = 1;
	CallData_ModifyVerify cd;
	cd.editor = this;
	cd.option = option;
	cd.allow = 1;
	cd.data = data;
	int textBytes = 0;
	for (simpleline *l = m_textHead; l; l = l->next)
		textBytes += l->total_chars + 1;
	cd.textBytes = textBytes;
	int textLength = 0;
	for (simpleline *l = m_textHead; l; l = l->next)
		textLength += l->total_chars + 1;
	cd.textLength = textLength;
	m_modifyVerifyCallback(&cd);
	return cd.allow;
}

inline void SimpleTextEdit::SetModifyVerifyCallback(ModifyVerifyCallback cb)
{
	m_modifyVerifyCallback = cb;
}

inline SimpleTextEdit::ModifyVerifyCallback SimpleTextEdit::GetModifyVerifyCallback()
{
	return m_modifyVerifyCallback;
}

inline void SimpleTextEdit::SetWideModeCheckFunction(WideModeCheckFunction fn)
{
	m_wideModeCheckFunction = fn;
}

inline SimpleTextEdit::WideModeCheckFunction SimpleTextEdit::GetWideModeCheckFunction()
{
	return m_wideModeCheckFunction;
}

inline int SimpleTextEdit::SetFontPathE(char *path)
{
	if (strlen(path) + 1 >= 4096)
		return 0;
	pthread_mutex_lock(&s_registryMutex);
	strcpy(m_fontEPath.c, path);
	fontface_update();
	for (simpleline *l = m_textHead; l; l = l->next) {
		l->disp_lines = 0;
		for (int i = 0; i < l->total_chars; i++)
			l->chunks[i].len = 0;
	}
	m_needRedraw = 1;
	pthread_mutex_unlock(&s_registryMutex);
	return 1;
}

inline int SimpleTextEdit::SetFontPathJ(char *path)
{
	if (strlen(path) + 1 >= 4096)
		return 0;
	pthread_mutex_lock(&s_registryMutex);
	strcpy(m_fontJPath.c, path);
	fontface_update();
	for (simpleline *l = m_textHead; l; l = l->next) {
		l->disp_lines = 0;
		for (int i = 0; i < l->total_chars; i++)
			l->chunks[i].len = 0;
	}
	m_needRedraw = 1;
	pthread_mutex_unlock(&s_registryMutex);
	return 1;
}

/* Returns how many display rows the next n characters take from the scroll
   position (m_firstViewLine, m_firstViewCharNo); with n <= 0, the number of logical
   lines. Runs under the registry lock. */


int SimpleTextEdit::CalcNumLine(int n) const
{
	while (pthread_mutex_trylock(&s_registryMutex))
		usleep(30000);

	int ret;

	if (n <= 0) {


		int m = 0;
		for (simpleline *l = m_textHead; l; l = l->next)
			m++;
		ret = m;
	} else {
		ret = 1;
		simpleline *line = m_firstViewLine;
		int col = m_firstViewCharNo;
		int pixel = 0;


		int count = 0;


		if (line->disp_lines <= 0) {
			int used = 0;
			int subline = 1;
			for (int i = 0; i < line->total_chars; i++) {
				int w;
				int charLen = line->chunks[i].len;
				if (charLen <= 0)
					SteLoadGlyph(this, &line->chunks[i], &w);
				else
					w = charLen;
				if (used + w >= m_dispWidth) {
					used = 0;
					subline++;
				}
				used += w + m_wrapPad;
			}
			line->disp_lines = subline;
		}


		while (count < n) {
			if (col >= line->total_chars) {
				line = line->next;
				if (!line)
					break;


				col = 0;
				if (line->disp_lines <= 0) {
					int used = 0;
					int subline = 1;
					for (int i = 0; i < line->total_chars; i++) {
						int w;
						int charLen = line->chunks[i].len;
						if (charLen <= 0)
							SteLoadGlyph(this, &line->chunks[i], &w);
						else
							w = charLen;
						if (used + w >= m_dispWidth) {
							used = 0;
							subline++;
						}
						used += w + m_wrapPad;
					}
					line->disp_lines = subline;
				}
				pixel = 0;
				ret++;
			}
			int w = line->chunks[col].len;
			if (pixel + w >= m_dispWidth) {
				pixel = 0;
				ret++;
			}
			count++;
			pixel += w + m_wrapPad;


			col++;
			if (pixel >= m_dispWidth) {
				pixel = 0;
				ret++;
			}
			if (col < line->total_chars) {
				if (pixel + line->chunks[col].len >= m_dispWidth) {
					pixel = 0;
					ret++;
				}
			}
		}
	}
	pthread_mutex_unlock(&s_registryMutex);
	return ret;
}

int SimpleTextEdit::SetOption(int option, const int &value, unsigned int size)
{
	int ret = 0;

	if (m_ctorFailed)
		return 0;

	while (pthread_mutex_trylock(&s_registryMutex))
		usleep(30000);

	switch (option) {
	case OPT_TW64:
	case OPT_W64: {
		int cw = value;
		if (cw > 0xa000)
			cw = 0xa000;
		else if (cw <= 63)
			cw = 64;


		int w = (cw + 63) >> 6;
		int dw = w - 4;
		m_bufWidth = w;
		m_defWidth = w;
		m_wideMode = 0;
		if (m_dispWidth != dw || m_dispHeight != m_bufHeight - 4) {
			m_dispWidth = dw;
			m_dispHeight = m_bufHeight - 4;
			if (m_buffer)
				free(m_buffer);
			m_buffer = (unsigned char *)malloc(m_dispWidth * m_dispHeight);
			if (!m_buffer) {
				m_ctorFailed = 1;
			} else {
				memset(m_buffer, 0, m_dispWidth * m_dispHeight);
				vvram_update();
			}
		}
		m_firstViewCharNo = 0;
		rewrite_screen();
		check_cursorpos(false);
		ret = 1;
		break;
	}

	case OPT_tw64:
	case OPT_w64:
		if (m_mode) {
			int cw = value;
			if (cw > 0xa000)
				cw = 0xa000;
			else if (cw <= 63)
				cw = 64;
			int w = (cw + 63) >> 6;
			int dw = w - 4;
			m_bufWidth = w;
			m_fieldWidth = w;
			m_wideMode = 0;
			m_fieldSizeFlag = 1;
			if (m_dispWidth != dw || m_dispHeight != m_bufHeight - 4) {
				m_dispWidth = dw;
				m_dispHeight = m_bufHeight - 4;
				if (m_buffer)
					free(m_buffer);
				m_buffer = (unsigned char *)malloc(m_dispWidth * m_dispHeight);
				if (!m_buffer) {
					m_ctorFailed = 1;
				} else {
					memset(m_buffer, 0, m_dispWidth * m_dispHeight);
					vvram_update();
				}
			}
			m_firstViewCharNo = 0;
			rewrite_screen();
			check_cursorpos(false);
			ret = 1;
		}
		break;

	case OPT_TH64:
	case OPT_H64: {
		int h = value;
		if (h > 30720)
			h = 30720;
		else if (h < 0)
			h = 64;
		h = (h + 63) >> 6;
		m_bufHeight = h;
		m_defHeight = h;
		m_wideMode = 0;
		if (m_dispWidth != m_bufWidth - 4 || m_dispHeight != h - 4) {
			m_dispWidth = m_bufWidth - 4;
			m_dispHeight = h - 4;
			if (m_buffer)
				free(m_buffer);
			m_buffer = (unsigned char *)malloc(m_dispWidth * m_dispHeight);
			if (!m_buffer) {
				m_ctorFailed = 1;
			} else {
				memset(m_buffer, 0, m_dispWidth * m_dispHeight);
				vvram_update();
			}
		}
		m_firstViewCharNo = 0;
		rewrite_screen();
		check_cursorpos(false);
		ret = 1;
		break;
	}

	case OPT_th64:
	case OPT_h64:
		if (m_mode) {
			int h = value;
			if (h > 30720)
				h = 30720;
			else if (h < 0)
				h = 64;
			h = (h + 63) >> 6;
			m_bufHeight = h;
			m_fieldHeight = h;
			m_wideMode = 0;
			m_fieldSizeFlag = 1;
			if (m_dispWidth != m_bufWidth - 4 || m_dispHeight != h - 4) {
				m_dispWidth = m_bufWidth - 4;
				m_dispHeight = h - 4;
				if (m_buffer)
					free(m_buffer);
				m_buffer = (unsigned char *)malloc(m_dispWidth * m_dispHeight);
				if (!m_buffer) {
					m_ctorFailed = 1;
				} else {
					memset(m_buffer, 0, m_dispWidth * m_dispHeight);
					vvram_update();
				}
			}
			m_firstViewCharNo = 0;
			rewrite_screen();
			check_cursorpos(false);
			ret = 1;
		}
		break;

	case OPT_FSIZ: {
		int v = value;
		m_fontPointSize = v;
		if (v < 4)
			m_fontPointSize = 4;
		else if (v > 128)
			m_fontPointSize = 128;
		fontface_update();
		memset(m_buffer, 0, m_dispWidth * m_dispHeight);
		vvram_update();
		m_firstViewCharNo = 0;
		rewrite_screen();
		check_cursorpos(false);
		ret = 1;
		break;
	}

	case OPT_SDWE: {
		int v = value;
		m_shadowOffset = v;
		if (v < 0)
			m_shadowOffset = 0;
		else if (v > 32)
			m_shadowOffset = 32;
		if (m_wrapPad < m_shadowOffset)
			m_wrapPad = m_shadowOffset;
		if (m_shadowOffset > 0 && m_padBot < m_shadowOffset + 3)
			m_padBot = m_shadowOffset + 3;
		vvram_update();
		m_firstViewCharNo = 0;
		rewrite_screen();
		check_cursorpos(false);
		ret = 1;
		break;
	}

	case OPT_CSPC: {
		int v = value;
		m_wrapPad = v;
		if (v < 0)
			m_wrapPad = 0;
		else if (v > 32)
			m_wrapPad = 32;
		if (m_wrapPad < m_shadowOffset)
			m_wrapPad = m_shadowOffset;
		vvram_update();
		m_firstViewCharNo = 0;
		rewrite_screen();
		check_cursorpos(false);
		ret = 1;
		break;
	}

	case OPT_LSPC: {
		int v = value;
		m_padBot = v;
		m_padTop = v / 2 - 1;
		if (m_padTop <= 0)
			m_padTop = 1;
		m_padBot = m_padBot - m_padTop;
		if (m_padBot < 2)
			m_padBot = 2;
		if (m_shadowOffset > 0 && m_padBot < m_shadowOffset + 3)
			m_padBot = m_shadowOffset + 3;
		vvram_update();
		m_firstViewCharNo = 0;
		rewrite_screen();
		check_cursorpos(false);
		ret = 1;
		break;
	}

	case OPT_LSPL: {
		int v = value;
		m_padBot = v;
		if (v < 2)
			m_padBot = 2;
		if (m_shadowOffset > 0 && m_padBot < m_shadowOffset + 3)
			m_padBot = m_shadowOffset + 3;
		vvram_update();
		m_firstViewCharNo = 0;
		rewrite_screen();
		check_cursorpos(false);
		ret = 1;
		break;
	}

	case OPT_LSPU: {
		int v = value;
		m_padTop = v;
		if (v <= 0)
			m_padTop = 1;
		vvram_update();
		m_firstViewCharNo = 0;
		rewrite_screen();
		check_cursorpos(false);
		ret = 1;
		break;
	}

	case OPT_CRSW:
		m_cursorSwitch = (value != 0);
		reset_cursorblink();
		ret = 1;
		break;

	case OPT_EDTE:
		m_insertMode = (value != 0);
		reset_cursorblink();
		ret = 1;
		break;

	case OPT_INST:
		m_insertToggle = (value != 0);
		reset_cursorblink();
		ret = 1;
		break;

	case OPT_MDFY:
		m_verifyInProgress = (value != 0);
		ret = 1;
		break;

	case OPT_CPSX:
		m_curCharNo = value;
		reset_cursorblink();
		ret = 1;
		break;

	case OPT_CPSY: {
		/* Walks the list to line `value`; when the lines run out first, line and
		   index stop at the last node. */


		int i = 0;
		int idx = 0;
		simpleline *line = 0;
		simpleline *l = m_textHead;
		if (l) {
			int n = value;
			if (n) {


				do {
					line = l;
					l = line->next;
					idx = i;
					i = idx + 1;
					if (!l)
						break;
				} while (i != n);
			}
		}
		if (l) {
			m_curLine = l;
			m_curLineNo = i;
		} else {
			m_curLine = line;
			m_curLineNo = idx;
		}
		check_cursorpos(true);


		m_needRedraw = 1;
		reset_cursorblink();
		ret = 1;
		break;
	}

	case OPT_CPDY: {
		m_curPixelPos = 0;
		m_curCharNo = 0;
		m_firstViewCharNo = 0;
		m_curLine = m_textHead;
		m_curLineNo = m_selRangeStart;
		m_firstViewLine = m_textHead;
		m_firstViewLineNo = m_selRangeStart;
		if (value > 0) {
			down_line(value, &m_firstViewLine, &m_firstViewLineNo, &m_firstViewCharNo, 0);
			down_line(value, &m_curLine, &m_curLineNo, &m_curCharNo, 0);
		}
		check_cursorpos(true);
		m_needRedraw = 1;
		reset_cursorblink();
		ret = 1;
		break;
	}

	case OPT_SCLY:
		m_gssy = -1;
		Action(AC_SCROLL_LINE, value);
		ret = 1;
		break;
	}

	pthread_mutex_unlock(&s_registryMutex);
	if (ret)
		return 1;
	return Object::SetOption(option, value, size);
}

inline int SimpleTextEdit::SetOption(int option, const float &value, unsigned int size)
{
	if (m_ctorFailed)
		return 0;
	return Object::SetOption(option, value, size);
}


int SimpleTextEdit::SetOption(int option, const String &value)
{
	if (m_ctorFailed)
		return 0;
	if (option == OPT_UTF8) {
		pthread_mutex_lock(&s_registryMutex);
		AC_DeleteAll(true);
		AC_InsertString_Utf8((unsigned char *)value.c_str(), true);
		AC_BeginText();
		pthread_mutex_unlock(&s_registryMutex);
		return 1;
	}
	return Object::SetOption(option, value);
}

/* Colors are four RGBA bytes copied byte by byte (data may be unaligned):
   'COLR' sets m_tag and the color of the four m_textQuad entries, 'CSC4' the cursor
   colors (m_cursorQuad), 'BGC4' the background colors (m_fieldQuad). For CSC4 and BGC4 the
   second and third source colors go to entries 2 and 1. */


int SimpleTextEdit::SetOption(int option, const void *data, unsigned int size)
{
	if (m_ctorFailed)
		return 0;

	const char *src = (const char *)data;
	switch (option) {
	case OPT_COLR:
		m_tag[0] = src[0];
		m_tag[1] = src[1];
		m_tag[2] = src[2];
		m_tag[3] = src[3];
		for (int i = 0; i < 4; i++) {
			m_textQuad[i].color.c[0] = src[0];
			m_textQuad[i].color.c[1] = src[1];
			m_textQuad[i].color.c[2] = src[2];
			m_textQuad[i].color.c[3] = src[3];
		}
		return 1;
	case OPT_CSC4:
		m_cursorQuad[0].color.c[0] = src[0];
		m_cursorQuad[0].color.c[1] = src[1];
		m_cursorQuad[0].color.c[2] = src[2];
		m_cursorQuad[0].color.c[3] = src[3];
		m_cursorQuad[1].color.c[0] = src[8];
		m_cursorQuad[1].color.c[1] = src[9];
		m_cursorQuad[1].color.c[2] = src[10];
		m_cursorQuad[1].color.c[3] = src[11];
		m_cursorQuad[2].color.c[0] = src[4];
		m_cursorQuad[2].color.c[1] = src[5];
		m_cursorQuad[2].color.c[2] = src[6];
		m_cursorQuad[2].color.c[3] = src[7];
		m_cursorQuad[3].color.c[0] = src[12];
		m_cursorQuad[3].color.c[1] = src[13];
		m_cursorQuad[3].color.c[2] = src[14];
		m_cursorQuad[3].color.c[3] = src[15];
		return 1;
	case OPT_BGC4:
		m_fieldQuad[0].color.c[0] = src[0];
		m_fieldQuad[0].color.c[1] = src[1];
		m_fieldQuad[0].color.c[2] = src[2];
		m_fieldQuad[0].color.c[3] = src[3];
		m_fieldQuad[1].color.c[0] = src[8];
		m_fieldQuad[1].color.c[1] = src[9];
		m_fieldQuad[1].color.c[2] = src[10];
		m_fieldQuad[1].color.c[3] = src[11];
		m_fieldQuad[2].color.c[0] = src[4];
		m_fieldQuad[2].color.c[1] = src[5];
		m_fieldQuad[2].color.c[2] = src[6];
		m_fieldQuad[2].color.c[3] = src[7];
		m_fieldQuad[3].color.c[0] = src[12];
		m_fieldQuad[3].color.c[1] = src[13];
		m_fieldQuad[3].color.c[2] = src[14];
		m_fieldQuad[3].color.c[3] = src[15];
		return 1;
	}
	return Object::SetOption(option, data, size);
}

int SimpleTextEdit::GetOption(int option, int &value, unsigned int size) const
{
	if (m_ctorFailed)
		return 0;

	switch (option) {
	case OPT_CPDL:
		value = (m_bufHeight - 4) / (m_fontPointSize + m_padTop + m_padBot);
		return 1;

	case OPT_TW64:
	case OPT_W64:
		value = m_bufWidth << 6;
		return 1;

	case OPT_TH64:
	case OPT_H64:
		value = m_bufHeight << 6;
		return 1;

	case OPT_FSIZ:
		value = m_fontPointSize;
		return 1;

	case OPT_SDWE:
		value = m_shadowOffset;
		return 1;

	case OPT_CSPC:
		value = m_wrapPad;
		return 1;

	case OPT_LSPC:
		value = m_padTop + m_padBot;
		return 1;

	case OPT_LSPL:
		value = m_padBot;
		return 1;

	case OPT_LSPU:
		value = m_padTop;
		return 1;

	case OPT_CRSW:
		value = m_cursorSwitch;
		return 1;

	case OPT_EDTE:
		value = m_insertMode;
		return 1;

	case OPT_INST:
		value = m_insertToggle;
		return 1;

	case OPT_MDFY:
		value = m_verifyInProgress;
		return 1;

	case OPT_CPSX:
		value = m_curCharNo;
		return 1;

	case OPT_CPSY:
		value = m_curLineNo;
		return 1;

	case OPT_CPSM: {
		while (pthread_mutex_trylock(&s_registryMutex))
			usleep(30000);
		int n = 0;
		for (simpleline *l = m_textHead; l; l = l->next)
			n++;
		value = n;
		pthread_mutex_unlock(&s_registryMutex);
		return 1;
	}

	case OPT_CPDY: {
		while (pthread_mutex_trylock(&s_registryMutex))
			usleep(30000);
		int total = 0;
		simpleline *line = m_textHead;
		if (line) {
			do {
				if (line->disp_lines <= 0) {
					int used = 0;
					int subline = 1;
					for (int i = 0; i < line->total_chars; i++) {
						int w;
						int charLen = line->chunks[i].len;
						if (charLen <= 0)
							SteLoadGlyph(this, &line->chunks[i], &w);
						else
							w = charLen;
						if (used + w >= m_dispWidth) {
							used = 0;
							subline++;
						}
						used += w + m_wrapPad;
					}
					line->disp_lines = subline;
				}
				if (line == m_curLine)
					break;
				total += line->disp_lines;
				line = line->next;
			} while (line);
		}
		int col = m_curCharNo;
		int acc = 0;
		for (int i = 0; i < col; i++) {
			int len = line->chunks[i].len;
			if (acc + len >= m_dispWidth)
				total++;
			acc += len + m_wrapPad;
		}
		value = total;
		pthread_mutex_unlock(&s_registryMutex);
		return 1;
	}

	case OPT_CPDM: {
		while (pthread_mutex_trylock(&s_registryMutex))
			usleep(30000);
		int total = 0;
		simpleline *line = m_textHead;
		if (line) {
			do {
				if (line->disp_lines <= 0) {
					int used = 0;
					int subline = 1;
					for (int i = 0; i < line->total_chars; i++) {
						int w;
						int charLen = line->chunks[i].len;
						if (charLen <= 0)
							SteLoadGlyph(this, &line->chunks[i], &w);
						else
							w = charLen;
						if (used + w >= m_dispWidth) {
							used = 0;
							subline++;
						}
						used += w + m_wrapPad;
					}
					line->disp_lines = subline;
				}
				total += line->disp_lines;
				line = line->next;
			} while (line);
		}
		value = total;
		pthread_mutex_unlock(&s_registryMutex);
		return 1;
	}

	case OPT_CX64:
		value = m_caretPixelX << 6;
		return 1;

	case OPT_CY64:
		value = m_caretPixelY << 6;
		return 1;

	case OPT_LEN: {
		while (pthread_mutex_trylock(&s_registryMutex))
			usleep(30000);
		int n = 0;
		for (simpleline *l = m_textHead; l; l = l->next)
			n += l->total_chars + 1;
		value = n;
		pthread_mutex_unlock(&s_registryMutex);
		return 1;
	}

	case OPT_GSSY:
		value = m_gssy;
		return 1;

	case OPT_GNLN:
		value = CalcNumLine(value);
		return 1;

	case OPT_DS64: {
		value = 0;
		if (m_fontJLoaded)
			value = m_fontJFace->size->metrics.descender;
		if (m_fontELoaded) {
			int d = m_fontEFace->size->metrics.descender;
			if (d < value)
				value = d;
		}
		return 1;
	}
	}

	return Object::GetOption(option, value, size);
}

inline int SimpleTextEdit::GetOption(int option, float &value, unsigned int size) const
{
	if (m_ctorFailed)
		return 0;
	return Object::GetOption(option, value, size);
}

int SimpleTextEdit::GetOption(int option, String &value) const
{
	if (m_ctorFailed)
		return 0;
	if (option == OPT_UTF8) {
		while (pthread_mutex_trylock(&s_registryMutex))
			usleep(30000);

		simpleline *cur = m_textHead;
		int n = 0;
		simpleline *c = cur;
		if (cur) {
			do {
				n += c->total_chars + 1;
				c = c->next;
			} while (c);
		}

		unsigned short *ucs2buf = (unsigned short *)__builtin_alloca((n + 1) * 2);
		unsigned short *head = ucs2buf;
		while (cur != 0 && (cur->next != 0 || cur->total_chars != 0)) {
			for (int i = 0; i < cur->total_chars; i++)
				*ucs2buf++ = cur->chunks[i].ch;
			*ucs2buf++ = 10;
			cur = cur->next;
		}
		*ucs2buf = 0;
		ucs2buf = head;

		pthread_mutex_unlock(&s_registryMutex);

		int utf8Len = SimpleTextEditUcs2ToUtf8Size(ucs2buf);
		char *utf8buf = (char *)__builtin_alloca(utf8Len);
		SimpleTextEditUcs2ToUtf8((unsigned char *)utf8buf, ucs2buf);
		unsigned int len = strlen(utf8buf);
		value.replace(0, (unsigned)-1, utf8buf, len);
		return 1;
	}
	return Object::GetOption(option, value);
}

/* Same colors as SetOption(void *), plus 'UTF8', which copies the text as
   UTF-8 into data when size is large enough. */


int SimpleTextEdit::GetOption(int option, void *data, unsigned int size) const
{
	if (m_ctorFailed)
		return 0;

	char *dst = (char *)data;
	switch (option) {
	case OPT_COLR:
		if (size < 4)
			return 0;
		dst[0] = m_tag[0];
		dst[1] = m_tag[1];
		dst[2] = m_tag[2];
		dst[3] = m_tag[3];
		return 1;
	case OPT_CSC4:
		if (size < 16)
			return 0;
		dst[0] = m_cursorQuad[0].color.c[0];
		dst[1] = m_cursorQuad[0].color.c[1];
		dst[2] = m_cursorQuad[0].color.c[2];
		dst[3] = m_cursorQuad[0].color.c[3];
		dst[8] = m_cursorQuad[1].color.c[0];
		dst[9] = m_cursorQuad[1].color.c[1];
		dst[10] = m_cursorQuad[1].color.c[2];
		dst[11] = m_cursorQuad[1].color.c[3];
		dst[4] = m_cursorQuad[2].color.c[0];
		dst[5] = m_cursorQuad[2].color.c[1];
		dst[6] = m_cursorQuad[2].color.c[2];
		dst[7] = m_cursorQuad[2].color.c[3];
		dst[12] = m_cursorQuad[3].color.c[0];
		dst[13] = m_cursorQuad[3].color.c[1];
		dst[14] = m_cursorQuad[3].color.c[2];
		dst[15] = m_cursorQuad[3].color.c[3];
		return 1;
	case OPT_BGC4:
		if (size < 16)
			return 0;
		dst[0] = m_fieldQuad[0].color.c[0];
		dst[1] = m_fieldQuad[0].color.c[1];
		dst[2] = m_fieldQuad[0].color.c[2];
		dst[3] = m_fieldQuad[0].color.c[3];
		dst[8] = m_fieldQuad[1].color.c[0];
		dst[9] = m_fieldQuad[1].color.c[1];
		dst[10] = m_fieldQuad[1].color.c[2];
		dst[11] = m_fieldQuad[1].color.c[3];
		dst[4] = m_fieldQuad[2].color.c[0];
		dst[5] = m_fieldQuad[2].color.c[1];
		dst[6] = m_fieldQuad[2].color.c[2];
		dst[7] = m_fieldQuad[2].color.c[3];
		dst[12] = m_fieldQuad[3].color.c[0];
		dst[13] = m_fieldQuad[3].color.c[1];
		dst[14] = m_fieldQuad[3].color.c[2];
		dst[15] = m_fieldQuad[3].color.c[3];
		return 1;
	case OPT_UTF8: {
		while (pthread_mutex_trylock(&s_registryMutex))
			usleep(30000);

		simpleline *cur = m_textHead;
		int n = 0;
		simpleline *c = cur;


		if (cur) {
			do {
				n += c->total_chars + 1;
				c = c->next;
			} while (c);
		}


		unsigned short *ucs2buf = (unsigned short *)__builtin_alloca((n + 1) * 2);
		unsigned short *p = ucs2buf;


		while (cur != 0 && (cur->next != 0 || cur->total_chars != 0)) {
			for (int i = 0; i < cur->total_chars; i++)
				*p++ = cur->chunks[i].ch;
			*p++ = 10;
			cur = cur->next;
		}
		*p = 0;

		pthread_mutex_unlock(&s_registryMutex);

		int utf8Len = SimpleTextEditUcs2ToUtf8Size(ucs2buf);
		if ((int)size < utf8Len)
			return 0;
		SimpleTextEditUcs2ToUtf8((unsigned char *)dst, ucs2buf);
		return 1;
	}
	}
	return Object::GetOption(option, data, size);
}

inline int SimpleTextEdit::Action(tActionCode code)
{
	if (m_ctorFailed)
		return 0;
	int head = m_ringHead;
	int tail = m_ringTail;
	if (tail == head - 1)
		return 0;
	if (head) {
		/* not full */
	} else if (tail == STE_RING_SIZE - 1) {
		return 0;
	}
	m_ring[m_ringTail].type = STE_RING_ACTION;
	m_ring[m_ringTail].code = code;
	m_ring[m_ringTail].value = 0;
	m_ring[m_ringTail].strCopy = 0;
	m_ringTail++;
	if (m_ringTail >= STE_RING_SIZE)
		m_ringTail = 0;
	return 1;
}

inline int SimpleTextEdit::Action(tActionCode code, int value)
{
	if (m_ctorFailed)
		return 0;
	int head = m_ringHead;
	int tail = m_ringTail;
	if (tail == head - 1)
		return 0;
	if (head) {
		/* not full */
	} else if (tail == STE_RING_SIZE - 1) {
		return 0;
	}
	m_ring[m_ringTail].type = STE_RING_ACTION_INT;
	m_ring[m_ringTail].code = code;
	m_ring[m_ringTail].value = value;
	m_ring[m_ringTail].strCopy = 0;
	m_ringTail++;
	if (m_ringTail >= STE_RING_SIZE)
		m_ringTail = 0;
	return 1;
}

/* Like the other two overloads, but queues a heap copy of s (UTF-8 bytes plus
   NUL, or UCS-2 units plus terminator; nothing for other codes), freed by
   Action_exec after running it. STE_RING_ACTION_STR. */


int SimpleTextEdit::Action(tActionCode code, unsigned char *s)
{
	int len = 0;
	int head, tail;
	unsigned char *copy;
	if (m_ctorFailed)
		goto fail;
	head = m_ringHead;
	tail = m_ringTail;
	if (tail == head - 1)
		return 0;
	if (head) {
		/* not full */
	} else if (tail == STE_RING_SIZE - 1) {
		goto fail;
	}
	switch (code) {
	case AC_INSERT_STRING_UTF8: {
		unsigned char *p = s;
		while (*p) {
			p++;
			len++;
		}
		len++;
		break;
	}
	case AC_INSERT_STRING_UCS2_DIV: {
		unsigned short *p = (unsigned short *)s;
		while (*p) {
			p++;
			len += 2;
		}
		len += 2;
		break;
	}
	default:
		len = 0;
		break;
	}
	copy = (unsigned char *)malloc(len);
	if (copy)
		goto ok;
fail:
	return 0;
ok:
	memcpy(copy, s, len);
	m_ring[m_ringTail].type = STE_RING_ACTION_STR;
	m_ring[m_ringTail].code = code;
	m_ring[m_ringTail].value = 0;
	m_ring[m_ringTail].strCopy = (int)copy;
	m_ringTail++;
	if (m_ringTail >= STE_RING_SIZE)
		m_ringTail = 0;
	return 1;
}

/* Runs the queued actions, at most 30 per call. */


int SimpleTextEdit::Action_exec()
{
	int did = 0;
	int iter = 0;
	while (m_ringHead != m_ringTail) {
		switch (m_ring[m_ringHead].type) {
		case STE_RING_ACTION:
			Action_i((tActionCode)m_ring[m_ringHead].code);
			did = 1;
			break;
		case STE_RING_ACTION_INT:
			Action_i((tActionCode)m_ring[m_ringHead].code, m_ring[m_ringHead].value);
			did = 1;
			break;
		case STE_RING_ACTION_STR:
			Action_i((tActionCode)m_ring[m_ringHead].code, (unsigned char *)m_ring[m_ringHead].strCopy);
			did = 1;
			break;
		}
		if (m_ring[m_ringHead].strCopy) {
			free((void *)m_ring[m_ringHead].strCopy);
			m_ring[m_ringHead].strCopy = 0;
		}
		m_ringHead++;
		if (m_ringHead >= STE_RING_SIZE)
			m_ringHead = 0;
		iter++;
		if (iter >= 30)
			break;
	}
	return did;
}

int SimpleTextEdit::Action_i(tActionCode code)
{
	switch (code) {
	case AC_PREV_CHAR:
		return AC_PrevChar();
	case AC_NEXT_CHAR:
		return AC_NextChar();
	case AC_PREV_WORD:
		return AC_PrevWord();
	case AC_NEXT_WORD:
		return AC_NextWord();
	case AC_PREV_LINE:
		return AC_PrevLine();
	case AC_NEXT_LINE:
		return AC_NextLine();
	case AC_NEW_LINE:
		return AC_NewLine(true, false);
	case AC_PREV_PAGE:
		return AC_PrevPage();
	case AC_NEXT_PAGE:
		return AC_NextPage();
	case AC_BEGIN_LINE:
		return AC_BeginLine();
	case AC_END_LINE:
		return AC_EndLine();
	case AC_BEGIN_TEXT:
		return AC_BeginText();
	case AC_END_TEXT:
		return AC_EndText();
	case AC_INSERT:
		return AC_Insert();
	case AC_DELETE_PREV_CHAR:
		return AC_DeletePrevChar();
	case AC_DELETE_NEXT_CHAR:
		return AC_DeleteNextChar(false);
	case AC_DELETE_ALL:
		return AC_DeleteAll(false);
	case AC_SELECT_MARK:
		return AC_SelectMark();
	case AC_SELECT_REGION:
		return AC_SelectRegion();
	case AC_SELECT_WORD:
		return AC_SelectWord();
	case AC_SELECT_LINE:
		return AC_SelectLine();
	case AC_SELECT_ALL:
		return AC_SelectAll();
	case AC_DESELECT_ALL:
		return AC_DeSelectAll(1);
	case AC_COPY_REGION:
		return AC_CopyRegion();
	case AC_CUT_REGION:
		return AC_CutRegion();
	case AC_PASTE_REGION:
		return AC_PasteRegion();
	default:
		return 0;
	}
}

inline int SimpleTextEdit::Action_i(tActionCode code, int value)
{
	if (code == AC_SCROLL_LINE)
		return AC_ScrollLine(value);
	return 0;
}

inline int SimpleTextEdit::Action_i(tActionCode code, unsigned char *s)
{
	switch (code) {
	case AC_INSERT_STRING_UTF8:
		return AC_InsertString_Utf8(s, false);
	case AC_INSERT_STRING_UCS2_DIV:
		return AC_InsertString_Ucs2_div((unsigned short *)s, false);
	default:
		return 0;
	}
}

inline int SimpleTextEdit::AC_Insert()
{
	if (!m_insertMode)
		return 0;
	m_insertToggle ^= 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}

inline int SimpleTextEdit::AC_SelectMark()
{
	AC_DeSelectAll(1);
	m_markLine = m_curLine;
	m_markRow = m_curLineNo;
	m_markCol = m_curCharNo;
	return 1;
}

inline int SimpleTextEdit::AC_SelectLine()
{
	AC_DeSelectAll(0);
	simpleline *line = m_curLine;
	m_selFromCol = 0;
	int startIdx = m_curLineNo;
	m_selFromLine = line;
	m_selToLine = line;
	m_selFromRow = startIdx;
	m_selToRow = startIdx;
	m_selToCol = line->total_chars - 1;


	int j = 0;
	if (line->total_chars > 0) {
		do {
			m_selFromLine->chunks[j].selected = 1;
			j++;
		} while (j < m_selFromLine->total_chars);
	}
	m_selFromLine->line_selected = 1;
	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}


/* Inserts s one character at a time: CR and LF become new lines, other
   control codes are skipped; stops at the first rejected character. */


inline int SimpleTextEdit::AC_InsertString_Ucs2_div(unsigned short *s, bool div)
{
	unsigned short buf[2];
	if (!m_insertMode && !div)
		return 0;
	buf[1] = 0;
	unsigned short *p = s;
	while (*p) {
		if (*p < 32) {
			if (*p == 13 || *p == 10)
				AC_NewLine(true, true);
		} else {
			buf[0] = *p;
			if (!AC_InsertString_Ucs2(buf, true, div))
				break;
		}
		p++;
	}
	return 1;
}


int SimpleTextEdit::AC_PrevChar()
{
	if (m_curCharNo > 0) {
		m_curCharNo--;
	} else {
		simpleline *prev = m_curLine->prev;
		if (prev) {


			m_curLine = prev;
			m_curCharNo = prev->total_chars;
			m_curLineNo--;
		}
	}
	m_curPixelPos = calc_curpos(m_curLine, m_curCharNo);

	check_cursorpos(true);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}

int SimpleTextEdit::AC_NextChar()
{
	if (m_curCharNo < m_curLine->total_chars) {
		m_curCharNo++;
	} else {
		simpleline *next = m_curLine->next;
		if (next) {
			m_curLineNo++;
			m_curLine = next;
			m_curCharNo = 0;
		}
	}
	m_curPixelPos = calc_curpos(m_curLine, m_curCharNo);

	check_cursorpos(true);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}

/* Move the cursor back (AC_PrevWord) or forward (AC_NextWord) over
   chunks[m_curCharNo].ch until the character class changes. */


int SimpleTextEdit::AC_PrevWord()
{
	int cls = ucs2_ctype(m_curLine->chunks[m_curCharNo].ch);

	while (m_curCharNo > 0) {
		m_curCharNo--;
		char c = ucs2_class(m_curLine->chunks[m_curCharNo].ch);
		if ((char)cls != c)
			break;
	}
	m_curPixelPos = calc_curpos(m_curLine, m_curCharNo);

	check_cursorpos(true);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 0;
}

int SimpleTextEdit::AC_NextWord()
{
	int cls = ucs2_ctype(m_curLine->chunks[m_curCharNo].ch);

	while (m_curCharNo < m_curLine->total_chars) {
		m_curCharNo++;
		if (cls != ucs2_class(m_curLine->chunks[m_curCharNo].ch))
			break;
	}
	m_curPixelPos = calc_curpos(m_curLine, m_curCharNo);

	check_cursorpos(true);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 0;
}


int SimpleTextEdit::AC_PrevLine()
{
	up_line(1, &m_curLine, &m_curLineNo, &m_curCharNo, m_curPixelPos);

	check_cursorpos(true);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}


int SimpleTextEdit::AC_NextLine()
{
	down_line(1, &m_curLine, &m_curLineNo, &m_curCharNo, m_curPixelPos);

	check_cursorpos(true);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}

/* Splits the current line at the cursor. `a` runs the modify-verify callback
   first; `b` skips the m_insertMode check, splits in overwrite mode too and
   leaves out the redraw and the cursor update (only the blink restarts). */


bool SimpleTextEdit::AC_NewLine(bool a, bool b)
{
	if (!m_insertMode && !b)
		return 0;

	if (a) {
		if (!check_modify_verify_callback(STE_VERIFY_NEW_LINE, 0))
			return 0;
	}

	AC_DeSelectAll(1);

	if (m_insertToggle || b) {
		simpleline *line = m_curLine;
		int movelen = line->total_chars - m_curCharNo;
		/* clang-format off */
#line 3139  /* the line number is part of the binary */
		assert(movelen >= 0);
		/* clang-format on */

		simpleline *nl = alloc_text(movelen);  /* NOTE: bug: the result of alloc_text is not checked */

		nl->prev = line;
		nl->next = line->next;
		if (movelen > 0) {


			unsigned int bits = movelen * 96;
			char *bp = (char *)line +  /* redundant as written */
			    m_curCharNo * sizeof(simpleline_chunk);
			memcpy(nl->chunks, bp + 28, bits / 8);
		}


		line->total_chars = m_curCharNo;
		line->next = nl;
		line->disp_lines = 0;
		m_curLine = nl;
		if (nl->next)
			nl->next->prev = nl;

		m_curLineNo++;
		m_selRangeEnd++;
		if (m_selRangeEnd == m_curLineNo)
			m_textTail = m_curLine;
		m_curPixelPos = 0;
		m_curCharNo = 0;
	} else {
		simpleline *n = m_curLine->next;
		if (n) {
			m_curLine = n;
			m_curPixelPos = 0;
			m_curCharNo = 0;
			m_curLineNo++;
		}
	}

	if (!b) {
		check_cursorpos(true);
		m_needRedraw = 1;
	}
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}

/* Scrolls the view by |n| rows without moving the logical cursor (n < 0 up,
   n > 0 down), dragging the cursor along only when it leaves the view. m_gssy
   counts the rows actually scrolled. */


int SimpleTextEdit::AC_ScrollLine(int n)
{
	m_gssy = 0;
	if (n == 0)
		return 1;

	int i;
	if (n < 0) {
		for (i = 0; i < -n; i++) {
			if (!up_line(1, &m_firstViewLine, &m_firstViewLineNo, &m_firstViewCharNo, 0))
				goto done;
			vvram_downscroll(1);
			int r = calc_drawend_pos();
			if (m_endLineNo < m_curLineNo || r || (m_endLineNo == m_curLineNo && m_endCharNo < m_curCharNo))
				up_line(1, &m_curLine, &m_curLineNo, &m_curCharNo, m_curPixelPos);
			m_gssy++;
		}
	} else {
		for (i = 0; i < n; i++) {
			if (!down_line(1, &m_firstViewLine, &m_firstViewLineNo, &m_firstViewCharNo, 0))
				goto done;
			vvram_upscroll(1);
			if (m_curLineNo < m_firstViewLineNo
			    || (m_curLineNo == m_firstViewLineNo && m_curCharNo < m_firstViewCharNo))
				down_line(1, &m_curLine, &m_curLineNo, &m_curCharNo, m_curPixelPos);
			m_gssy++;
		}
	}

done:
	check_cursorpos(true);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}

/* Move the cursor by a page (the rows that fit in the buffer height less 4
   pixels). Both return 0, unlike the other AC_* handlers. */


int SimpleTextEdit::AC_PrevPage()
{
	up_line((m_bufHeight - 4) / (m_fontPointSize + m_padTop + m_padBot), &m_curLine, &m_curLineNo, &m_curCharNo,
	    m_curPixelPos);
	up_line((m_bufHeight - 4) / (m_fontPointSize + m_padTop + m_padBot), &m_firstViewLine, &m_firstViewLineNo,
	    &m_firstViewCharNo, -1);

	check_cursorpos(false);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 0;
}

int SimpleTextEdit::AC_NextPage()
{
	down_line((m_bufHeight - 4) / (m_fontPointSize + m_padTop + m_padBot), &m_curLine, &m_curLineNo, &m_curCharNo,
	    m_curPixelPos);
	down_line((m_bufHeight - 4) / (m_fontPointSize + m_padTop + m_padBot), &m_firstViewLine, &m_firstViewLineNo,
	    &m_firstViewCharNo, -1);

	check_cursorpos(false);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 0;
}

int SimpleTextEdit::AC_BeginLine()
{
	m_curPixelPos = 0;
	m_curCharNo = 0;

	check_cursorpos(true);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}

int SimpleTextEdit::AC_EndLine()
{
	m_curCharNo = m_curLine->total_chars;
	m_curPixelPos = calc_curpos(m_curLine, m_curCharNo);

	check_cursorpos(true);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}


int SimpleTextEdit::AC_BeginText()
{
	m_curPixelPos = 0;
	m_curCharNo = 0;
	m_firstViewCharNo = 0;
	m_curLine = m_textHead;
	m_curLineNo = m_selRangeStart;
	m_firstViewLine = m_textHead;
	m_firstViewLineNo = m_selRangeStart;

	check_cursorpos(false);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}


int SimpleTextEdit::AC_EndText()
{
	m_curLine = m_textTail;
	m_curLineNo = m_selRangeEnd;
	m_curCharNo = m_curLine->total_chars;
	m_curPixelPos = calc_curpos(m_curLine, m_curCharNo);
	m_firstViewCharNo = 0;
	m_firstViewLine = m_curLine;
	m_firstViewLineNo = m_curLineNo;

	up_line((m_bufHeight - 4) / (m_fontPointSize + m_padTop + m_padBot), &m_firstViewLine, &m_firstViewLineNo,
	    &m_firstViewCharNo, -1);

	check_cursorpos(false);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}

/* Deletes the character before the cursor; at the start of a line, joins the
   line to the previous one. */


int SimpleTextEdit::AC_DeletePrevChar()
{
	if (!m_insertMode)
		return 0;

	AC_DeSelectAll(1);

	if (m_curCharNo > 0) {
		int rest = m_curLine->total_chars - m_curCharNo;
		if (rest > 0) {


			unsigned int bits = rest * 96;
			char *b = (char *)m_curLine +  /* redundant as written */
			    m_curCharNo * sizeof(simpleline_chunk);
			memcpy(b + 16, b + 28, bits / 8);  /* NOTE: bug: overlapping memcpy */
		}
		m_curLine->total_chars--;
		m_curCharNo--;
		m_curLine->disp_lines = 0;
	} else {
		simpleline *old = m_curLine;
		if (old->prev) {
			old->prev->next = old->next;

			if (m_firstViewLineNo == m_curLineNo) {
				simpleline *p = m_curLine->prev;
				m_firstViewLineNo--;
				m_firstViewLine = p;
			} else if (m_curLineNo < m_firstViewLineNo) {
				m_firstViewLineNo--;
			}
			if (m_selRangeEnd == m_curLineNo) {
				simpleline *p = m_curLine->prev;
				m_selRangeEnd--;
				m_textTail = p;
			} else if (m_curLineNo < m_selRangeEnd) {
				m_selRangeEnd--;
			}

			simpleline *p = m_curLine->prev;
			m_curLineNo--;
			m_curLine = p;
			m_curCharNo = p->total_chars;

			int i = 0;
			unsigned short *buf =
			    (unsigned short *)__builtin_alloca(sizeof(unsigned short) * (old->total_chars + 1));
			for (; i < old->total_chars; i++)
				buf[i] = old->chunks[i].ch;
			buf[i] = 0;


			i = m_curCharNo;
			AC_InsertString_Ucs2(buf, true, false);
			m_curCharNo = i;

			free_text(old);
		}
	}

	m_curPixelPos = calc_curpos(m_curLine, m_curCharNo);

	check_cursorpos(true);

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}

/* Deletes the character after the cursor; at the end of a line, joins the next
   line to this one. */


int SimpleTextEdit::AC_DeleteNextChar(bool b)
{
	if (!m_insertMode && !b)
		return 0;
	if (!b)
		AC_DeSelectAll(1);

	simpleline *line = m_curLine;
	int count = line->total_chars - m_curCharNo;

	if (count > 0) {
		if (count - 1 > 0) {
			unsigned int bits = (count - 1) * 96;
			memcpy(&line->chunks[m_curCharNo],  /* NOTE: bug: overlapping memcpy */
			    &line->chunks[m_curCharNo + 1], bits / 8);
		}
		m_curLine->total_chars--;
		m_curLine->disp_lines = 0;
	} else {
		simpleline *next = line->next;
		if (next) {
			line->next = next->next;

			if (m_firstViewLineNo == m_curLineNo + 1) {
				m_firstViewLineNo = m_curLineNo;
				m_firstViewLine = m_curLine;
			} else if (m_curLineNo + 1 < m_firstViewLineNo) {
				m_firstViewLineNo--;
			}
			if (m_selRangeEnd == m_curLineNo + 1) {
				m_selRangeEnd = m_curLineNo;
				m_textTail = m_curLine;
			} else if (m_curLineNo + 1 < m_selRangeEnd) {
				m_selRangeEnd--;
			}

			int n = next->total_chars;
			unsigned short *buf = (unsigned short *)__builtin_alloca((n + 1) * 2);
			int i;
			for (i = 0; i < next->total_chars; i++)
				buf[i] = next->chunks[i].ch;
			buf[i] = 0;


			i = m_curCharNo;
			AC_InsertString_Ucs2(buf, true, false);
			m_curCharNo = i;
			free_text(next);
		}
	}

	if (!b) {
		m_curPixelPos = calc_curpos(m_curLine, m_curCharNo);
		check_cursorpos(true);
		m_needRedraw = 1;
		m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	}
	return 1;
}

int SimpleTextEdit::AC_DeleteAll(bool clearOnly)
{
	if (!m_insertMode && !clearOnly)
		return 0;
	AC_DeSelectAll(1);

	simpleline *line = m_textHead;
	while (line) {
		simpleline *next = line->next;
		free_text(line);
		line = next;
	}
	m_selRangeStart = 0;

	simpleline *newLine = alloc_text(0);
	m_textHead = newLine;
	m_selRangeEnd = 0;
	m_textTail = newLine;
	m_firstViewLineNo = 0;
	m_firstViewLine = newLine;
	m_curLineNo = 0;
	m_curLine = newLine;
	m_firstViewCharNo = 0;
	m_curPixelPos = 0;
	m_curCharNo = 0;

	check_cursorpos(false);

	if (m_insertMode)
		m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	m_needRedraw = 1;
	return 1;
}


/* Selects from the mark (AC_SelectMark) to the cursor, in either order. */


int SimpleTextEdit::AC_SelectRegion()
{
	if (!m_markLine)
		return 0;

	if (m_markRow < m_curLineNo || (m_markRow == m_curLineNo && m_markCol <= m_curCharNo)) {
		m_selFromLine = m_markLine;
		m_selFromRow = m_markRow;
		m_selToLine = m_curLine;
		m_selToRow = m_curLineNo;
		m_selFromCol = m_markCol;
		m_selToCol = m_curCharNo - 1;
	} else {
		m_selFromLine = m_curLine;
		m_selToLine = m_markLine;
		m_selFromRow = m_curLineNo;
		m_selToRow = m_markRow;
		m_selFromCol = m_curCharNo;
		m_selToCol = m_markCol - 1;
	}

	if (m_selToCol < 0) {
		m_selToRow--;
		m_selToLine = m_selToLine->prev;
		m_selToCol = m_selToLine->total_chars;
	}

	simpleline *line = m_selFromLine;
	int row = m_selFromRow;
	int idx = m_selFromCol;
	if (m_selToRow >= m_selFromRow) {
		line->chunks[idx].selected = 1;
		while (line != m_selToLine || idx != m_selToCol) {
			idx++;
			if (idx >= line->total_chars) {
				row++;
				idx = 0;
				line->line_selected = 1;
				line = line->next;
			}
			if (line && row >= m_selFromRow && m_selToRow >= row && m_selToRow >= m_selFromRow)
				line->chunks[idx].selected = 1;
			else
				break;
		}
	}

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}
/* Extends the selection to the word around the cursor. */
int SimpleTextEdit::AC_SelectWord()
{
	AC_DeSelectAll(0);

	simpleline *line = m_curLine;
	int col = m_curCharNo;
	if (col >= line->total_chars)
		return 1;

	m_selFromLine = line;
	m_selFromRow = m_curLineNo;
	m_selToRow = m_curLineNo;
	m_selToLine = line;
	m_selFromCol = col;
	m_selToCol = col;

	int cls = ucs2_ctype(line->chunks[col].ch);

	m_selFromLine->chunks[m_selFromCol].selected = 1;


	while (m_selFromCol > 0) {
		m_selFromCol--;
		char c = ucs2_class(m_selFromLine->chunks[m_selFromCol].ch);
		if ((char)cls != c)
			break;
		m_selFromLine->chunks[m_selFromCol].selected = 1;
	}

	while (m_selToCol < m_selToLine->total_chars) {
		m_selToCol++;
		if (cls != ucs2_class(m_selToLine->chunks[m_selToCol].ch))
			break;
		m_selToLine->chunks[m_selToCol].selected = 1;
	}

	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}


int SimpleTextEdit::AC_SelectAll()
{
	AC_DeSelectAll(0);
	m_selFromCol = 0;
	m_selFromLine = m_textHead;
	m_selToLine = m_textTail;
	m_selToRow = m_selRangeEnd;
	m_selFromRow = m_selRangeStart;
	if (m_textTail->total_chars > 0) {
		m_selToCol = m_textTail->total_chars - 1;
	} else {
		simpleline *prev = m_textTail->prev;
		m_selToRow = m_selRangeEnd - 1;
		m_selToLine = prev;
		m_selToCol = prev->total_chars - 1;
	}

	simpleline *line = m_selFromLine;
	for (int count = 0; line && count < m_selRangeEnd - m_selRangeStart + 1; count++) {
		for (int j = 0; j < line->total_chars; j++)
			line->chunks[j].selected = 1;
		line->line_selected = 1;
		line = line->next;
	}
	m_needRedraw = 1;
	m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	return 1;
}

int SimpleTextEdit::AC_DeSelectAll(int fullReset)
{
	simpleline *sel = m_selFromLine;
	if (sel) {
		simpleline *endLine = m_selToLine;
		if (endLine) {
			simpleline *line = sel;
			int lineIdx = m_selFromRow;
			int chunkIdx = m_selFromCol;
			while (!(lineIdx < m_selFromRow) && !(m_selToRow < lineIdx) && !(m_selToRow < m_selFromRow)) {
				line->chunks[chunkIdx].selected = 0;
				if (line == m_selToLine && chunkIdx == m_selToCol)
					break;
				chunkIdx++;
				if (chunkIdx >= line->total_chars) {
					lineIdx++;
					chunkIdx = 0;
					line->line_selected = 0;
					line = line->next;
				}
				if (!line)
					break;
			}
			if (line)
				line->line_selected = 0;
		}
	}
	m_selToLine = 0;
	m_selFromLine = 0;
	m_selToRow = -1;
	m_selFromRow = -1;
	m_selToCol = 0;
	m_selFromCol = 0;
	if (fullReset) {
		m_markLine = 0;
		m_markRow = -1;
		m_markCol = 0;
		m_needRedraw = 1;
		m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	}
	return 1;
}

/* Copies the selection to the internal clipboard (UCS-2) in two passes, count
   then copy; a '\n' ends each fully selected line. */


int SimpleTextEdit::AC_CopyRegion()
{
	if (m_clipBuf) {
		free(m_clipBuf);
		m_clipBuf = 0;
		m_clipLen = 0;
	}
	if (!m_selFromLine)
		return 0;
	if (!m_selToLine)
		return 0;

	simpleline *line = m_selFromLine;
	int lineIdx = m_selFromRow;
	int chunkIdx = m_selFromCol;
	int n = 0;
	while (!(lineIdx < m_selFromRow) && !(m_selToRow < lineIdx) && !(m_selToRow < m_selFromRow)) {
		n++;
		if (line == m_selToLine && chunkIdx == m_selToCol)
			break;
		chunkIdx++;
		if (chunkIdx >= line->total_chars) {
			if (!line->line_selected)
				break;
			n++;
			line = line->next;
			lineIdx++;
			chunkIdx = 0;
		}
		if (!line)
			break;
	}
	if (line && line->line_selected && !(m_selToRow < lineIdx))
		n++;

	m_clipLen = n;
	m_clipBuf = (unsigned short *)malloc(n * 2);


	if (m_clipBuf) {
		n = 0;
		chunkIdx = m_selFromCol;
		line = m_selFromLine;
		lineIdx = m_selFromRow;
		while (!(lineIdx < m_selFromRow) && !(m_selToRow < lineIdx) && !(m_selToRow < m_selFromRow)) {
			m_clipBuf[n] = line->chunks[chunkIdx].ch;
			n++;
			if (line == m_selToLine && chunkIdx == m_selToCol)
				break;
			chunkIdx++;
			if (chunkIdx >= line->total_chars) {
				if (!line->line_selected)
					break;
				m_clipBuf[n] = 10;
				n++;
				line = line->next;
				lineIdx++;
				chunkIdx = 0;
			}
			if (!line)
				break;
		}
		if (line && line->line_selected && !(m_selToRow < lineIdx))
			m_clipBuf[n] = 10;
		return 1;
	}
	return 0;
}

/* Deletes the selection without copying it: moves the cursor to its start and
   calls AC_DeleteNextChar once per character. */


int SimpleTextEdit::AC_CutRegion()
{
	if (!m_selFromLine)
		return 0;
	if (!m_selToLine)
		return 0;

	simpleline *line = m_selFromLine;
	int lineIdx = m_selFromRow;
	int chunkIdx = m_selFromCol;
	int n = 0;
	while (!(lineIdx < m_selFromRow) && !(m_selToRow < lineIdx) && !(m_selToRow < m_selFromRow)) {
		n++;
		if (line == m_selToLine && chunkIdx == m_selToCol)
			break;
		chunkIdx++;
		if (chunkIdx >= line->total_chars) {
			if (!line->line_selected)
				break;
			n++;
			line = line->next;
			lineIdx++;
			chunkIdx = 0;
		}
		if (!line)
			break;
	}
	if (line && line->line_selected && !(m_selToRow < lineIdx))
		n++;
	if (n > 0) {
		m_curLine = m_selFromLine;
		m_curLineNo = m_selFromRow;
		m_curCharNo = m_selFromCol;
		AC_DeSelectAll(1);


		for (chunkIdx = 0; chunkIdx < n; chunkIdx++)
			AC_DeleteNextChar(true);
		m_curPixelPos = calc_curpos(m_curLine, m_curCharNo);

		check_cursorpos(true);

		m_needRedraw = 1;
		m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
		return 1;
	}
	return 0;
}


/* Inserts the clipboard one code unit at a time through the modify-verify
   callback (option 5); CR and LF become AC_NewLine, other control codes are
   skipped. Stops and returns 0 at the first rejection. */


int SimpleTextEdit::AC_PasteRegion()
{


	int changed = 0;
	int ret = 1;
	unsigned short buf[2];
	int i;
	unsigned short *p;

	if (!m_insertMode)
		return 0;

	buf[1] = 0;
	p = m_clipBuf;
	for (i = 0; i < m_clipLen; i++, p++) {
		if (*p < 32) {
			if (*p == 13 || *p == 10) {
				if (!AC_NewLine(true, true)) {
					ret = 0;
					break;
				}
				changed = 1;
			}
			continue;
		}
		buf[0] = *p;
		if (!check_modify_verify_callback(STE_VERIFY_PASTE, buf)) {
			ret = 0;
			break;
		}
		AC_InsertString_Ucs2(buf, true, true);
		changed = 1;
	}

	if (changed) {
		m_curPixelPos = calc_curpos(m_curLine, m_curCharNo);
		check_cursorpos(true);
		m_needRedraw = 1;
		m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	}
	return ret;
}
/* Decodes s from UTF-8 and inserts it one character at a time. */
int SimpleTextEdit::AC_InsertString_Utf8(unsigned char *s, bool b)
{
	unsigned short buf[2];

	if (!m_insertMode && !b)
		return 0;
	if (!check_modify_verify_callback(STE_VERIFY_UTF8, s))
		return 0;

	buf[1] = 0;
	unsigned char *p = s;
	while (*p) {
		if (*p < 32) {
			if (*p == 10 || *p == 13)
				AC_NewLine(false, true);
			p++;
			continue;
		}
		buf[0] = (unsigned short)utf8_decode(&p);
		if (!AC_InsertString_Ucs2(buf, false, b))
			break;
	}
	return 1;
}

/* Inserts the UCS-2 string s at the cursor by rebuilding the current line. */


int SimpleTextEdit::AC_InsertString_Ucs2(unsigned short *s, bool a, bool b)
{
	if (!m_insertMode && !b)
		return 0;

	if (a && !check_modify_verify_callback(STE_VERIFY_UCS2, s))
		return 0;

	AC_DeSelectAll(1);

	int len = 0;
	if (s[0]) {
		unsigned short *p = s;
		do {
			p++;
			len++;
		} while (*p);
	}

	simpleline *nl =
	    alloc_text(m_curLine->total_chars + len);  /* NOTE: bug: the result of alloc_text is not checked */

	nl->next = m_curLine->next;
	simpleline *pv = m_curLine->prev;
	nl->prev = pv;
	if (pv)
		pv->next = nl;
	if (nl->next)
		nl->next->prev = nl;

	if (m_curCharNo > 0) {
		unsigned int bits = m_curCharNo * 96;
		memcpy(&nl->chunks[0], &m_curLine->chunks[0], bits / 8);
	}

	int k = 0;
	if (s[0]) {
		do {
			nl->chunks[m_curCharNo + k].ch = s[k];
			nl->chunks[m_curCharNo + k].len = 0;
			k++;
		} while (s[k]);
	}

	if (m_insertToggle || b) {
		int cf = m_curCharNo;
		int n = m_curLine->total_chars - cf;
		if (n > 0) {


			unsigned int bits = n * 96;
			char *bp = (char *)m_curLine + cf * sizeof(simpleline_chunk);  /* redundant as written */
			memcpy(&nl->chunks[cf + k], bp + 28, bits / 8);
		}
	} else {
		int off = m_curCharNo + k;
		int n = m_curLine->total_chars - off;
		if (n > 0) {
			unsigned int bits = n * 96;
			memcpy(&nl->chunks[off], &m_curLine->chunks[off], bits / 8);
			nl->total_chars -= k;
		} else {
			nl->total_chars = off;
		}
	}

	replace_text(m_curLine, nl);

	m_curCharNo += len;
	if (nl->total_chars < m_curCharNo)
		m_curCharNo = nl->total_chars;

	if (!b) {
		m_curPixelPos = calc_curpos(m_curLine, m_curCharNo);
		check_cursorpos(true);
		m_needRedraw = 1;
		m_cursorBlinkTarget = m_cursorBlinkCounter + 1;
	} else {
		m_curPixelPos = 0;
	}
	return 1;
}
