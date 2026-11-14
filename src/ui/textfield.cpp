/*
 * Methods of TextField: construction, drawing (as an editable field or as a label),
 * options, cursor layout and the editing actions.
 */

#include "textfield.h"
#include "gfx/vec.h"
#include <string.h>
#include "scene/scenefacade.h"
#include "gfx/gm_graph.h"
#include "scene/optid.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);
/* instance of the class initializer (its constructor is empty) */
TextField::ClassInitializer __TextField_ClassInitializer__;

const int TextField::_D_pltmAsciiCharPitch64;
const int TextField::_D_pltmKanjiCharPitch64;
const PltmColor TextField::_D_pltmTextFieldColor = { {
    { 0xff, 0xff, 0xff, 0xff },
    { 0xff, 0xff, 0xff, 0xff },
    { 0xff, 0xff, 0xff, 0xff },
    { 0xff, 0xff, 0xff, 0xff },
} };
const int TextField::_D_pltmFontSize;
const int TextField::_D_HAlignment;
const int TextField::_D_VAlignment;

/*
 * Replacement characters: the first for UTF-8 (putUtf8 and the decoder of
 * AC_InsertString_Utf8), the second for UTF-16.
 */
static unsigned int s_utf8ReplacementChar = 0;
static unsigned int s_utf16ReplacementChar = 0;
/* guard of ClassInitializer; nothing reads it */
bool TextField::bInited = false;

/* Two static vertex quads; the drawing rewrites only x and y. */
static gmVertexPC s_editQuadA[4] = {
	{ 0.0f, 0.5f, 0.0f, 0 },
	{ 0.0f, -0.5f, 0.0f, 0 },
	{ 1.0f, 0.5f, 0.0f, 0 },
	{ 1.0f, -0.5f, 0.0f, 0 },
};
static gmVertexPC s_editQuadB[4] = {
	{ 0.0f, 0.5f, 0.0f, 0xff000000 },
	{ 0.0f, -0.5f, 0.0f, 0xff000000 },
	{ 1.0f, 0.5f, 0.0f, 0xff000000 },
	{ 1.0f, -0.5f, 0.0f, 0xff000000 },
};

/* Default quads copied by the constructor into m_fieldQuad and m_cursorQuad. */
static gmVertexPC s_fieldQuad[4] = {
	{ 0.0f, 0.5f, 0.0f, 0x80404040 },
	{ 0.0f, -0.5f, 0.0f, 0x80808080 },
	{ 1.0f, 0.5f, 0.0f, 0x80404040 },
	{ 1.0f, -0.5f, 0.0f, 0x80808080 },
};
static gmVertexPC s_cursorQuad[4] = {
	{ 0.0f, 0.5f, 0.0f, 0xffffffff },
	{ 0.0f, -0.5f, 0.0f, 0xffffffff },
	{ 1.0f, 0.5f, 0.0f, 0xffffffff },
	{ 1.0f, -0.5f, 0.0f, 0xffffffff },
};

static void utf16ToUtf8(unsigned char *dst, unsigned short *src);
static void putUtf8(unsigned char **pp, unsigned int c);
static unsigned short nextUtf16(unsigned short **pp);

void *TextField::PLTM_ID()
{
	void *id = m_pltmId;
	if (id)
		return id;
	return SceneGetDefaultPltmFontID();
}

TextField::ClassInitializer::ClassInitializer()
{}

/* Falls back to length 0 when an allocation failed; returns 1 either way. */
static inline int tf_check_buffers(TextField *t)
{
	if (t->m_buf && t->m_xtab)
		return 1;
	t->m_maxLen = 0;
	return 1;
}

TextField::TextField(Object *parent, int max_len_param) : Object(parent)
{
	m_pltmId = 0;
	GetOptionState()->always_mask |= EFF_TEXTURE_CLAMP | EFF_DIFFUSE;
	GetOptionState()->always_mask |= EFF_PIXEL_TEST | EFF_PIXEL_WRITE;
	m_maxLen = max_len_param;

	m_buf = new CharRec[max_len_param];

	m_xtab = (unsigned short *)malloc(2 * (m_maxLen + 1) * sizeof(unsigned short));
	if (m_xtab)
		m_xtabMasked = m_xtab + m_maxLen + 1;
	else
		m_xtabMasked = 0;

	int on = tf_check_buffers(this);

	m_leftMargin = 1280;
	m_rightMargin = 1280;
	m_modifyVerifyCallback = 0;
	m_numSelected = 0;
	m_cursorPos = 0;
	m_cursorX = 0;
	m_offsetX = 0;
	unk_31C = 0;
	unk_320 = 0;
	m_textDirty = on;
	unk_2F4 = on;
	m_textDrawn = 0;
	m_modified = on;
	m_cursorShown = on;
	m_editable = on;
	m_passwordMode = 0;
	m_rectDirty = on;
	unk_3DC = on;
	m_fontSize = _D_pltmFontSize;
	m_textColor = _D_pltmTextFieldColor;

	m_asciiPitch = _D_pltmAsciiCharPitch64;
	m_kanjiPitch = _D_pltmKanjiCharPitch64;
	m_hAlign = _D_HAlignment;
	m_vAlign = _D_VAlignment;
	m_blinkInterval = 15;
	m_blinkTimer = 0;
	m_cursorBlinkOn = on;
	m_length = 0;
	m_visibleChars = 0;
	m_xtab[0] = 0;
	m_xtabMasked[0] = 0;
	m_buf[0].width = 0;
	m_width64 = 0;

	m_height64 = 0;
	if (m_passwordMode) {
		PLTM_CreateStringFromUcs2(m_fontSize, &m_shownStr, m_xtabMasked);
		PLTM_CreateStringFromUcs2(m_fontSize, &m_plainStr, m_xtabMasked);
	} else {
		PLTM_CreateStringFromUcs2(m_fontSize, &m_shownStr, m_xtab);
		PLTM_CreateStringFromUcs2(m_fontSize, &m_plainStr, m_xtab);
	}

	m_plain = &m_plainStr;
	m_shown = &m_shownStr;
	m_textReady = 0;
	m_prevTextReady = 0;
	m_glyphQuads = (PLTM_DrawSlot *)malloc(m_maxLen * sizeof(PLTM_DrawSlot));
	if (!m_glyphQuads)
		m_maxLen = 0;

	for (int i = 0; i < 4; i++) {
		m_fieldQuad[i] = s_fieldQuad[i];
		m_cursorQuad[i] = s_cursorQuad[i];
	}
}

TextField::~TextField()
{
	PLTM_DestroyString(&m_plainStr);
	PLTM_DestroyString(&m_shownStr);
	if (m_glyphQuads)
		free(m_glyphQuads);
}

/* Same as TextField::PLTM_ID. */
static inline void *pltm_id_of(const TextField *t)
{
	void *id = t->m_pltmId;
	if (id)
		return id;
	return SceneGetDefaultPltmFontID();
}

/*
 * Draws in five passes, each a 4-vertex quad: the field (m_fieldQuad), the static quads A and B
 * (B with the stencil), the blinking cursor (m_cursorQuad) and the glyph quads of the text.
 * Coordinates are 26.6 fixed point.
 */
void TextField::DrawThis_Editable(gm_graph &g)
{
	if (!visible)
		return;

	if (!m_prevTextReady && m_textReady) {
		PLTM_DestroyString(m_plain);
		PLTM_CreateStringFromUcs2(m_fontSize, m_plain, m_xtab);
	}
	m_prevTextReady = m_textReady;

	if (m_textDirty) {
		if (m_passwordMode)
			PLTM_CreateStringFromUcs2(m_fontSize, m_shown, m_xtabMasked);
		else
			PLTM_CreateStringFromUcs2(m_fontSize, m_shown, m_xtab);
		m_textDirty = 0;
		m_modified = 1;
	}

	PLTM_Touch(pltm_id_of(this), m_plain);
	PLTM_Touch(pltm_id_of(this), m_shown);

	if (!m_shown->rectFinal || m_rectDirty)
		CalcRectangle();

	do {
		if (!m_modified)
			break;
		if (m_length > 0)
			ResetCursorBlink();
		m_textDrawn = 0;
		if (!m_shown->rectFinal)
			break;
		m_cursorX = CalcCursorPos64(m_cursorPos, (bool *)&m_modified) + m_offsetX;
		CursorJustification();
	} while (0);

	m_blinkTimer = m_blinkTimer + 1;
	if (m_blinkTimer % (m_blinkInterval * 2) > m_blinkInterval)
		m_cursorBlinkOn = 1;
	else
		m_cursorBlinkOn = 0;

	/* ---- pass 1: quad of the field (m_fieldQuad) ---- */
	float qy = 0.0f - m_height64 * (1.0f / 64.0f);
	float qx = m_width64 * (1.0f / 64.0f) + 0.0f;
	m_fieldQuad[0].x = 0.0f;
	m_fieldQuad[0].y = 0.0f;
	m_fieldQuad[1].x = 0.0f;
	m_fieldQuad[1].y = qy;
	m_fieldQuad[2].x = qx;
	m_fieldQuad[2].y = 0.0f;
	m_fieldQuad[3].x = qx;
	m_fieldQuad[3].y = qy;

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE);
	g.BlendFunc(GM_BLEND_ALPHA);
	g.Diffuse(*option_state.GetDiffuse());
	g.SetArrays(&m_fieldQuad[0], GM_VERTEX_PC, 4, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);

	/* ---- pass 2: static quad A, the rectangle widened by half a size ---- */
	gmVertexPC *q = s_editQuadA;
	q[0].x = 0.0f - (m_width64 >> 1) * (1.0f / 64.0f);
	q[0].y = (m_height64 >> 1) * (1.0f / 64.0f) + 0.0f;
	q[1].x = 0.0f - (m_width64 >> 1) * (1.0f / 64.0f);
	q[1].y = 0.0f - m_height64 * (1.0f / 64.0f) - (m_height64 >> 1) * (1.0f / 64.0f);
	q[2].x = m_width64 * (1.0f / 64.0f) + 0.0f + (m_width64 >> 1) * (1.0f / 64.0f);
	q[2].y = (m_height64 >> 1) * (1.0f / 64.0f) + 0.0f;
	q[3].x = m_width64 * (1.0f / 64.0f) + 0.0f + (m_width64 >> 1) * (1.0f / 64.0f);
	q[3].y = 0.0f - m_height64 * (1.0f / 64.0f) - (m_height64 >> 1) * (1.0f / 64.0f);

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_STENCIL_TEST);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE);
	g.BlendFunc(GM_BLEND_KEEP);
	static vec4 s_shadowColor(0.0f, 0.0f, 0.0f, 0.0f);
	g.Diffuse(s_shadowColor);
	g.SetArrays(q, GM_VERTEX_PC, 4, 1, 0, 0);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);

	/* ---- pass 3: static quad B (the rectangle of the field) and stencil ---- */
	q = s_editQuadB;
	q[0].x = 0.0f;
	q[0].y = 0.0f;
	q[1].x = 0.0f;
	q[1].y = 0.0f - m_height64 * (1.0f / 64.0f);
	q[2].x = m_width64 * (1.0f / 64.0f) + 0.0f;
	q[2].y = 0.0f;
	q[3].x = m_width64 * (1.0f / 64.0f) + 0.0f;
	q[3].y = 0.0f - m_height64 * (1.0f / 64.0f);

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_STENCIL_TEST);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE);
	g.BlendFunc(GM_BLEND_KEEP);
	static vec4 s_stencilColor(0.0f, 0.0f, 0.0f, 1.0f);
	g.Diffuse(s_stencilColor);
	g.SetArrays(q, GM_VERTEX_PC, 4, 1, 0, 0);
	g.StencilOp(1, 1, 3);
	g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);
	g.StencilOp(1, 1, 1);

	/* ---- pass 4: blinking cursor ---- */
	if (m_cursorShown && m_cursorBlinkOn) {
		m_cursorQuad[0].y = 0.0f;
		m_cursorQuad[2].y = 0.0f;
		float cx0 = (m_cursorX - 8) * (1.0f / 64.0f);
		float cx1 = (m_cursorX + 8) * (1.0f / 64.0f);
		float cy = (-m_height64) * (1.0f / 64.0f);
		m_cursorQuad[0].x = cx0;
		m_cursorQuad[1].x = cx0;
		m_cursorQuad[2].x = cx1;
		m_cursorQuad[3].x = cx1;
		m_cursorQuad[1].y = cy;
		m_cursorQuad[3].y = cy;

		g.SetEnableBits(g.fFlagsE | GM_ENABLE_ANTIALIAS);
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_STENCIL_TEST);
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE);
		g.BlendFunc(GM_BLEND_ALPHA);
		g.Diffuse(*option_state.GetDiffuse());
		g.SetArrays(&m_cursorQuad[0], GM_VERTEX_PC, 4, 1, 0, 0);
		g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_ANTIALIAS);
	}

	/* ---- text layout ---- */
	int ok = 0;
	int drawn = 0;
	long long xbase = m_offsetX;
	PLTM_DrawSlot *quads = m_glyphQuads;
	long long ybase = m_shown->y + ((m_height64 - m_shown->h) >> 1);

	if (!m_textDrawn || m_rectDirty || PLTM_IsDirty(pltm_id_of(this), -1, 0, m_shown)) {
		long long area;
		int first, last;
		PLTM_GetVisibleArea(pltm_id_of(this), m_asciiPitch, m_kanjiPitch, xbase, (long long)m_width64, m_shown,
		    &area, &first, &last);
		m_visibleChars = last - first + 1;
		if (first >= 0) {
			if (m_visibleChars > 0)
				ok = 1;
			else
				m_visibleChars = 0;
		} else {
			m_visibleChars = 0;
		}
		if (ok) {
			m_shown->ready = 0;
			drawn = PLTM_DrawStringGM(pltm_id_of(this), m_asciiPitch, m_kanjiPitch, xbase + area, ybase,
			    m_visibleChars, first, m_shown, &m_textColor, quads);
		}
		if (drawn != m_visibleChars || !m_shown->ready) {
			m_modified = 1;
			m_shown->rectFinal = 0;
		}
		if (!drawn)
			m_textReady = 0;
		if (drawn == m_visibleChars)
			m_textDrawn = 1;
	} else {
		PLTM_Touch(pltm_id_of(this), m_shown);
		drawn = m_visibleChars;
	}

	/* ---- pass 5: glyph quads ---- */
	if (drawn > 0) {
		if (m_shown->ready)
			m_textReady = 1;
		if (m_length > 0) {
			g.BlendFunc(blend_mode);
			g.SetEnableBits(g.fFlagsE | GM_ENABLE_STENCIL_TEST);
			g.StencilFunc(GM_EQUAL, 1);
			g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT);
			g.TexWrap(GM_WRAP_CLAMP, GM_WRAP_CLAMP);
			g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
			g.Diffuse(*option_state.GetDiffuse());
			g.ColorMask(255, 255, 255, 0);
			for (int i = 0; i < drawn; i++, quads++) {
				if (quads->tex) {
					g.SetArrays(quads, GM_VERTEX_PNCT, 4, 1, 0, 0);
					g.SetTexture(quads->tex);
					g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);
				}
			}
		}
	}

	if (!m_shown->ready)
		m_textReady = 0;
}

void TextField::DrawThis(gm_graph &g)
{
	if (!m_maxLen)
		return;
	if (m_editable)
		DrawThis_Editable(g);
	else
		DrawThis_AsLabel(g);
}

void TextField::DrawThis_AsLabel(gm_graph &g)
{}

int TextField::SetFieldSize(int w, int h)
{
	m_width64 = w << 6;
	m_height64 = h << 6;
	return 1;
}

void TextField::SetModifyVerifyCallback(void (*cb)(CallData_ModifyVerify *))
{
	m_modifyVerifyCallback = cb;
}

void (*TextField::GetModifyVerifyCallback())(CallData_ModifyVerify *)
{
	return m_modifyVerifyCallback;
}

int TextField::SetOption(int option, const float &value, unsigned int size)
{
	return Object::SetOption(option, value, size);
}

int TextField::GetOption(int option, float &value, unsigned int size) const
{
	return Object::GetOption(option, value, size);
}

/*
 * The colors of the eight quad vertices: OPT_BGC4 writes those of m_fieldQuad,
 * OPT_CSC4 those of m_cursorQuad, byte by byte from a 16-byte buffer whose size is not checked.
 */

int TextField::SetOption(int option, const void *data, unsigned int size)
{
	if (!m_maxLen)
		return 0;
	const unsigned char *src = (const unsigned char *)data;

	if (option == OPT_CSC4)
		goto h_4csc;
	if (option > OPT_CSC4)
		goto right;
	if (option == OPT_BGC4)
		goto h_4cgb;
	goto fallback;
right:
	if (option == OPT_COL4)
		goto h_col4;
	goto fallback;

h_col4:
	m_textColor = *(const PltmColor *)data;
	m_textDrawn = 0;
	return 1;

h_4csc:
	{
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
	}
h_4cgb:
	{
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
fallback:
	return Object::SetOption(option, data, size);
}

int TextField::SetOption(int option, const int &value, unsigned int size)
{
	if (!m_maxLen)
		return 0;
	switch (option) {
	case OPT_W64:
		m_width64 = value;
		return 1;
	case OPT_H64:
		m_height64 = value;
		return 1;
	case OPT_PWMD:
		m_passwordMode = value;
		AC_DeleteAll();
		return 1;
	case OPT_FSIZ: {
		int v = value;
		m_fontSize = v;
		if (v <= 0)
			m_fontSize = 1;
		else if (v >= 129)
			m_fontSize = 128;
		m_textDirty = 1;
		m_rectDirty = 1;
		m_textDrawn = 0;
		return 1;
	}
	case OPT_FONT: {
		int v = value;
		m_textDirty = 1;
		m_rectDirty = 1;
		m_textDrawn = 0;
		m_pltmId = (void *)v;
		return 1;
	}
	case OPT_HALN: {
		int v = value;
		m_rectDirty = 1;
		m_textDrawn = 0;
		m_hAlign = v;
		return 1;
	}
	case OPT_VALN: {
		int v = value;
		m_rectDirty = 1;
		m_textDrawn = 0;
		m_vAlign = v;
		return 1;
	}
	case OPT_CRSW:
		m_cursorShown = (value != 0);
		ResetCursorBlink();
		return 1;
	case OPT_CPOS: {
		int v = value;
		m_modified = 1;
		m_cursorPos = v;
		return 1;
	}
	case OPT_CX64: {
		int v = value;
		m_modified = 1;
		m_cursorX = v;
		return 1;
	}
	case OPT_OX64: {
		int v = value;
		m_modified = 1;
		m_offsetX = v;
		return 1;
	}
	case OPT_LM64: {
		int v = value;
		m_modified = 1;
		m_leftMargin = v;
		return 1;
	}
	case OPT_RM64: {
		int v = value;
		m_modified = 1;
		m_rightMargin = v;
		return 1;
	}
	}
	return Object::SetOption(option, value, size);
}

int TextField::SetOption(int option, const String &value)
{
	if (!m_maxLen)
		return 0;
	if (option == OPT_UTF8) {
		AC_DeleteAll();
		AC_InsertString_Utf8((unsigned char *)value.c_str());
		return 1;
	}
	return Object::SetOption(option, value);
}

int TextField::GetOption(int option, void *data, unsigned int size) const
{
	if (!m_maxLen)
		return 0;
	unsigned char *dst = (unsigned char *)data;

	if (option == OPT_CSC4)
		goto h_4csc;
	if (option > OPT_CSC4)
		goto right;
	if (option == OPT_BGC4)
		goto h_4cgb;
	goto fallback;
right:
	if (option != OPT_COL4)
		goto fallback;
	if (size < 16)
		return 0;
	*(PltmColor *)data = m_textColor;
	return 1;

h_4csc:
	{
		if (size < 4)
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
	}
h_4cgb:
	{
		if (size < 4)
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
	}
fallback:
	return Object::GetOption(option, data, size);
}

int TextField::GetOption(int option, int &value, unsigned int size) const
{
	if (!m_maxLen)
		return 0;
	switch (option) {
	case OPT_H64:
		value = m_height64;
		return 1;
	case OPT_CRSW:
		value = m_cursorShown;
		return 1;
	case OPT_CPOS:
		value = m_cursorPos;
		return 1;
	case OPT_CX64:
		value = m_cursorX;
		return 1;
	case OPT_OX64:
		value = m_offsetX;
		return 1;
	case OPT_LM64:
		value = m_leftMargin;
		return 1;
	case OPT_LEN:
		value = m_length;
		return 1;
	case OPT_W64:
		value = m_width64;
		return 1;
	case OPT_RM64:
		value = m_rightMargin;
		return 1;
	case OPT_PWMD:
		value = m_passwordMode;
		return 1;
	case OPT_FSIZ:
		value = m_fontSize;
		return 1;
	}
	return Object::GetOption(option, value, size);
}

int TextField::GetOption(int option, String &value) const
{
	if (!m_maxLen)
		return 0;
	if (option == OPT_UTF8) {
		char buf[1024];
		utf16ToUtf8((unsigned char *)buf, m_xtab);
		char *p = buf;
		unsigned int len = strlen(p);
		value.replace(0, (unsigned)-1, p, len);
		return 1;
	}
	return Object::GetOption(option, value);
}

void TextField::CalcRectangle()
{
	if (!m_maxLen)
		return;
	void *id = m_pltmId;
	if (!id)
		id = SceneGetDefaultPltmFontID();
	if (PLTM_GetTextRect(id, m_asciiPitch, m_kanjiPitch, m_shown) >= 0)
		m_rectDirty = 0;
}

int TextField::CalcCursorPos64(int n, bool *sel)
{
	int total = 0;
	if (m_maxLen == 0)
		return 0;
	*sel = 0;
	if (n == 0)
		return 0;
	if (m_shown->count < n)
		n = m_shown->count;
	for (int i = 0; i <= n - 1; i++) {
		if (m_buf.p[i].width < 0) {
			int w = PLTM_GetCharWidthGM(PLTM_ID(), &m_shown->glyphs[i]);
			m_buf.p[i].width = w;
			if (m_buf.p[i].width < 0)
				goto bad;
		}
		total += m_buf.p[i].width;
		total += m_buf.p[i].pitch;
		continue;
bad:
		*sel = 1;
	}
	return total;
}

static void utf16ToUtf8(unsigned char *dst, unsigned short *src)
{
	unsigned short c;

	while ((c = nextUtf16(&src)) != 0)
		putUtf8(&dst, c);
	*dst++ = 0;
}

static void putUtf8(unsigned char **pp, unsigned int c)
{
	unsigned char *p;

	c = ((int)c < 0) ? s_utf8ReplacementChar : c;
	p = *pp;
	if (c < 128) {
		*p = c;
		p++;
	} else {
		unsigned int lim = 2048;
		int n = 1;
		unsigned char *q;
		unsigned char mask;

		while (c >= lim) {
			n++;
			lim <<= 5;
		}
		p += n + 1;
		q = p;
		mask = (unsigned char)((1 << (7 - n)) - 1);
		n--;
		while (n != -1) {
			q--;
			*q = 0x80 | (c & 0x3f);
			c >>= 6;
			n--;
		}
		q[-1] = ~mask | (c & (mask >> 1));
	}
	*pp = p;
}

static unsigned short nextUtf16(unsigned short **pp)
{
	unsigned short *p = *pp;
	unsigned short c = *p++;

	*pp = p;
	return c;
}

void TextField::CursorJustification()
{
	PLTM_String *r = m_shown;
	int textRight = r->w + r->x;
	if (!m_maxLen)
		return;

	if (m_cursorX < m_leftMargin) {
		int delta = m_offsetX + (m_leftMargin - m_cursorX);
		m_offsetX = delta;
		if (delta > 0) {
			m_cursorX = m_leftMargin - delta;
			m_offsetX = 0;
			return;
		}
		m_cursorX = m_leftMargin;
		return;
	}

	int fieldWidth = m_width64;
	int innerRight = fieldWidth - 16;
	int cursorLimit = innerRight - m_rightMargin;
	if (!(cursorLimit < m_cursorX))
		return;

	int right = m_cursorX + 16;
	int overflow = m_rightMargin + right - fieldWidth;
	int newOffset = m_offsetX - overflow;
	m_cursorX = cursorLimit;
	m_offsetX = newOffset;
	int textEnd = newOffset + textRight;
	if (textEnd < innerRight) {
		int textEndPad = textEnd + 16;
		int gap = fieldWidth - textEndPad;
		m_cursorX = cursorLimit + gap;
		m_offsetX = newOffset + gap;
	} else {
		m_cursorX = cursorLimit;
	}

	int curOffsetX = m_offsetX;
	if (curOffsetX <= 0)
		return;
	if (!(m_rightMargin < m_width64))
		return;
	m_cursorX = m_cursorX - curOffsetX;
	m_offsetX = 0;
}

/* Moves n records of buf (6 bytes each) from src to dst, for the two AC_InsertString. */
static inline void ShiftRecs(char *dst, char *src, int n)
{
	if (dst == src)
		return;
	if (n == 0)
		return;
	n = n * 6;
	if (dst < src) {
		while (n != 0) {
			unsigned char c = *src;
			src++;
			n--;
			*dst = c;
			dst++;
		}
	} else {
		src += n;
		dst += n;
		while (n != 0) {
			dst--;
			src--;
			unsigned char c = *src;
			n--;
			*dst = c;
		}
	}
}

/* Moves count elements of size bytes from src to dst (either direction). */
static inline void ShiftElems(void *to, void *from, int count, int size)
{
	char *dst = (char *)to;
	char *src = (char *)from;

	if (dst == src)
		return;
	if (count == 0)
		return;
	int n = count * size;
	if (dst < src) {
		while (n != 0) {
			unsigned char c = *src;
			src++;
			n--;
			*dst = c;
			dst++;
		}
	} else {
		src += n;
		dst += n;
		while (n != 0) {
			dst--;
			src--;
			unsigned char c = *src;
			n--;
			*dst = c;
		}
	}
}

/* Deletes record n of the len records at base, moving the following ones back. */
static inline void ShiftRecsDown(CharRec *base, int n, int len)
{
	CharRec *p = base + n;
	CharRec *q = p + 1;
	char *dst = (char *)p;
	char *src = (char *)q;
	n = len - n;
	if (dst == src)
		return;
	if (n == 0)
		return;
	n = n * 6;
	if (dst < src) {
		while (n != 0) {
			unsigned char c = *src;
			src++;
			n--;
			*dst = c;
			dst++;
		}
	} else {
		src += n;
		dst += n;
		while (n != 0) {
			dst--;
			src--;
			unsigned char c = *src;
			n--;
			*dst = c;
		}
	}
}

int TextField::AC_DeletePrevChar()
{
	if (!m_cursorPos)
		return 0;

	ShiftElems(m_buf.p + (m_cursorPos - 1), m_buf.p + m_cursorPos, m_length - m_cursorPos, sizeof(CharRec));
	ShiftElems(m_xtab.p + (m_cursorPos - 1), m_xtab.p + m_cursorPos, m_length - m_cursorPos + 1,
	    sizeof(unsigned short));
	ShiftElems(m_xtabMasked + (m_cursorPos - 1), m_xtabMasked + m_cursorPos, m_length - m_cursorPos + 1,
	    sizeof(unsigned short));

	m_modified = 1;
	m_textDirty = 1;
	unk_2F4 = 1;
	m_rectDirty = 1;
	m_length--;
	m_cursorPos--;
	return 1;
}

int TextField::AC_DeleteNextChar()
{
	if (m_length <= m_cursorPos)
		return 0;

	ShiftRecsDown(m_buf, m_cursorPos, m_length);
	ShiftElems(m_xtab.p + m_cursorPos, m_xtab.p + m_cursorPos + 1, m_length - m_cursorPos + 1,
	    sizeof(unsigned short));
	ShiftElems(m_xtabMasked + m_cursorPos, m_xtabMasked + m_cursorPos + 1, m_length - m_cursorPos + 1,
	    sizeof(unsigned short));

	m_textDirty = 1;
	unk_2F4 = 1;
	m_rectDirty = 1;
	m_length--;
	return 1;
}

/* Runs the action of a tActionCode (2..12); any other code returns 0. */
int TextField::Action(tActionCode code)
{
	switch (code) {
	case AC_PrevChar_:
		return AC_PrevChar();
	case AC_NextChar_:
		return AC_NextChar();
	case AC_SelectAll_:
		return AC_SelectAll();
	case AC_Begin_:
		return AC_Begin();
	case AC_End_:
		return AC_End();
	case AC_DeletePrevChar_:
		return AC_DeletePrevChar();
	case AC_DeleteNextChar_:
		return AC_DeleteNextChar();
	case AC_DeleteAll_:
		return AC_DeleteAll();
	}
	return 0;
}

int TextField::Action(tActionCode code, unsigned char *arg)
{
	switch (code) {
	case AC_InsertString_Utf8_:
		return AC_InsertString_Utf8(arg);
	case AC_InsertString_Ucs2_:
		return AC_InsertString_Ucs2((unsigned short *)arg);
	}
	return 0;
}

int TextField::BlockMove(int a, int b, int end)
{
	if (a == b)
		return 1;
	if (a < b) {
		CharRec *s = m_buf.p + end;
		CharRec *d = m_buf.p + (end + (b - a));
		for (int i = 0; i < end - a; i++)
			*d-- = *s--;
	} else if (b < a) {
		CharRec *s = m_buf.p + a;
		CharRec *d = m_buf.p + b;
		for (int i = 0; i < end - a; i++)
			*d++ = *s++;
	}
	return 1;
}

void TextField::ResetCursorBlink()
{
	m_blinkTimer = m_blinkInterval + 1;
}

int TextField::AC_SelectAll()
{
	int i = 0;
	CharRec *p = m_buf;
	if (m_length > 0) {
		do {
			p->flags |= CHARREC_SELECTED;
			i++;
			p++;
		} while (i < m_length);
	}
	m_numSelected = m_length;
	return 1;
}

int TextField::AC_DeSelectAll()
{
	int i = 0;
	CharRec *p = m_buf;
	if (m_length > 0) {
		do {
			p->flags &= ~CHARREC_SELECTED;
			i++;
			p++;
		} while (i < m_length);
	}
	m_numSelected = 0;
	return 1;
}

int TextField::AC_Begin()
{
	m_cursorPos = 0;
	m_modified = 1;
	return 1;
}

int TextField::AC_End()
{
	m_modified = 1;
	m_cursorPos = m_length;
	return 1;
}

int TextField::AC_PrevChar()
{
	if (m_cursorPos) {
		m_cursorPos = m_cursorPos - 1;
		m_modified = 1;
	}
	return 1;
}

int TextField::AC_NextChar()
{
	if (m_cursorPos < m_length) {
		m_cursorPos = m_cursorPos + 1;
		m_modified = 1;
	}
	return 1;
}

int TextField::AC_PrevWord()
{
	return 0;
}

int TextField::AC_NextWord()
{
	return 0;
}

/* Decodes one UTF-8 character at *pp and advances *pp. */
static inline unsigned int utf8_decode(unsigned char **pp)
{
	unsigned char *p = *pp;
	unsigned int c = *p;
	unsigned int ch;

	p++;
	if ((signed char)c >= 0) {
		ch = c;
	} else if ((unsigned char)(c + 62) >= 51) {
		ch = s_utf8ReplacementChar;
	} else {
		unsigned int bound;
		ch = c & 0x3f;
		ch = (ch << 6) | (*p & 0x3f);
		bound = 2048;
		p++;
		while (ch >= bound) {
			ch &= bound - 1;
			bound <<= 5;
			ch = (ch << 6) | (*p & 0x3f);
			p++;
		}
	}
	*pp = p;
	return ch;
}

static inline unsigned int utf8_decode2(unsigned char **pp)
{
	unsigned char *p = *pp;
	unsigned int c = *p;
	unsigned int ch;

	p++;
	if ((signed char)c >= 0) {
		ch = c;
	} else {
		if ((unsigned char)(c + 62) >= 51) {
			ch = s_utf8ReplacementChar;
		} else {
			unsigned int bound;
			ch = c & 0x3f;

			ch = (ch << 6) | (*p & 0x3f);
			bound = 2048;
			p++;
			while (ch >= bound) {
				ch &= bound - 1;
				bound <<= 5;
				ch = (ch << 6) | (*p & 0x3f);
				p++;
			}
		}
	}
	*pp = p;
	return ch;
}

/* True for a Unicode scalar value: not above 0x10FFFF, not a surrogate. */
static inline int ucs4_is_valid(unsigned int ch)
{
	if (ch > 0x10ffff)
		return 0;
	if (ch >= 0xd800 && ch <= 0xdfff)
		return 0;
	return 1;
}

/* Writes ch as UTF-16 at *dst: a surrogate pair above 0xFFFF, invalid values replaced. */
static inline void put_utf16(unsigned int ch, unsigned short **dst)
{
	unsigned int val = ch;
	unsigned short *d = *dst;

	if (!ucs4_is_valid(val))
		val = s_utf16ReplacementChar;

	if (val <= 0xffff) {
		*d++ = (unsigned short)val;
	} else {
		val -= 0x10000;
		*d++ = (unsigned short)(0xd800 + (val >> 10));
		*d++ = (unsigned short)(0xdc00 + (val & 0x3ff));
	}
	*dst = d;
}

/*
 * Asks m_modifyVerifyCallback (option 1), counts the UTF-8 characters, then inserts as
 * many as fit at the cursor.
 */
int TextField::AC_InsertString_Utf8(unsigned char *s)
{
	if (m_modifyVerifyCallback) {
		CallData_ModifyVerify cd;
		cd.editor = this;
		cd.option = TF_VERIFY_UTF8;
		cd.allow = 1;
		cd.data = s;
		m_modifyVerifyCallback(&cd);
		if (!cd.allow)
			return 0;
	}

	int n = 0;
	unsigned char *p = s;
	for (;;) {
		unsigned int ch = utf8_decode(&p);
		if (ch == 0)
			break;
		n++;
	}

	int cnt = n;
	if (m_maxLen < m_length + n)
		cnt = m_maxLen - m_length;
	if (cnt == 0)
		return 0;
	n = cnt;

	ShiftRecs((char *)(m_buf.p + (m_cursorPos + n)), (char *)(m_buf.p + m_cursorPos), m_length - m_cursorPos);
	ShiftElems(m_xtab.p + (m_cursorPos + n), m_xtab.p + m_cursorPos, m_length - m_cursorPos + 1,
	    sizeof(unsigned short));
	ShiftElems(m_xtabMasked + (m_cursorPos + n), m_xtabMasked + m_cursorPos, m_length - m_cursorPos + 1,
	    sizeof(unsigned short));

	unsigned char *q = s;
	int pos = m_cursorPos;
	CharRec *pb = m_buf.p + pos;
	for (int i = 0; i < n; i++) {
		unsigned int ch = utf8_decode2(&q);
		unsigned short *px = m_xtab.p + (m_cursorPos + i);
		unsigned short *pm = m_xtabMasked + (m_cursorPos + i);
		put_utf16(ch, &px);
		put_utf16(m_passwordMode, &pm);
		pb[i].flags = 0;
		pb[i].unk_01 = 0;
		pb[i].width = -1;
		if (m_passwordMode)
			pb[i].pitch = m_asciiPitch;
		else
			pb[i].pitch = (m_xtab[m_cursorPos + i] < 256) ? m_asciiPitch : m_kanjiPitch;
	}

	m_modified = 1;
	m_textDrawn = 0;
	m_textDirty = 1;
	unk_2F4 = 1;
	m_rectDirty = 1;
	m_length += n;
	m_cursorPos += n;
	return 1;
}

/* Inserts the UCS-2 string at m_cursorPos, moving m_buf, m_xtab and m_xtabMasked to make room. */
int TextField::AC_InsertString_Ucs2(unsigned short *s)
{
	if (m_modifyVerifyCallback) {
		CallData_ModifyVerify cd;
		cd.editor = this;
		cd.option = TF_VERIFY_UCS2;
		cd.allow = 1;
		cd.data = s;
		m_modifyVerifyCallback(&cd);
		if (!cd.allow)
			return 0;
	}

	int len = 0;
	unsigned short *q = s + 1;
	unsigned short c = s[0];
	if (c) {
		do {
			len++;
			c = *q;
			q++;
		} while (c);
	}

	int n = len;
	if (m_maxLen < m_length + len)
		n = m_maxLen - m_length;
	if (n == 0)
		return 0;

	ShiftRecs((char *)(m_buf.p + (m_cursorPos + n)), (char *)(m_buf.p + m_cursorPos), m_length - m_cursorPos);
	ShiftElems(m_xtab.p + (m_cursorPos + n), m_xtab.p + m_cursorPos, m_length - m_cursorPos + 1,
	    sizeof(unsigned short));
	ShiftElems(m_xtabMasked + (m_cursorPos + n), m_xtabMasked + m_cursorPos, m_length - m_cursorPos + 1,
	    sizeof(unsigned short));

	CharRec *pb = m_buf.p + m_cursorPos;
	unsigned short *px = m_xtab.p + m_cursorPos;
	unsigned short *pm = m_xtabMasked + m_cursorPos;
	for (int i = 0; i < n; i++) {
		px[i] = *s;
		pm[i] = m_passwordMode;
		pb[i].flags = 0;
		pb[i].unk_01 = 0;
		pb[i].width = -1;
		s++;
		if (m_passwordMode)
			pb[i].pitch = m_asciiPitch;
		else
			pb[i].pitch = (m_xtab[m_cursorPos + i] < 256) ? m_asciiPitch : m_kanjiPitch;
	}

	m_modified = 1;
	m_length += n;
	m_cursorPos += n;
	m_shown->rectFinal = 0;
	m_textDrawn = 0;
	m_textDirty = 1;
	unk_2F4 = 1;
	m_rectDirty = 1;
	return 1;
}

int TextField::AC_DeleteAll()
{
	m_length = 0;
	m_visibleChars = 0;
	m_cursorPos = 0;
	m_modified = 1;
	m_textDirty = 1;
	unk_2F4 = 1;
	m_rectDirty = 1;
	m_textDrawn = 0;
	m_shown->rectFinal = 0;
	m_xtab[0] = 0;
	m_xtabMasked[0] = 0;
	m_buf[0].width = 0;
	return 1;
}
