/*
 * MultilineLabel widget: construction, options, layout and drawing.
 */

#include "multilinelabel.h"

#include "gfx/vec.h"
#include "scene/optid.h"
#include "gfx/gm_graph.h"

/* Unit matrix and unit w row: the file-scope objects built at startup. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

bool MultilineLabel::_Inited = false;

void MultilineLabel::_Init()
{
	_Inited = 1;
}

/* Class defaults; the color is opaque white. */
const int MultilineLabel::_D_pltmAsciiCharPitch64 = 0;
const int MultilineLabel::_D_pltmKanjiCharPitch64 = 0;
const int MultilineLabel::_D_pltmLineSpace64 = 0;
const short MultilineLabel::_D_pltmCRcode = '\n';
const PltmColor MultilineLabel::_D_pltmMultilineLabelColor = { {
    { 0xff, 0xff, 0xff, 0xff },
    { 0xff, 0xff, 0xff, 0xff },
    { 0xff, 0xff, 0xff, 0xff },
    { 0xff, 0xff, 0xff, 0xff },
} };
const int MultilineLabel::_D_pltmFontSize = 48;
const int MultilineLabel::_D_HAlignment = ALIGN_LEFT;
const int MultilineLabel::_D_VAlignment = ALIGN_BASELINE;

MultilineLabel::MultilineLabel(Object *parent) : Object(parent)
{
	if (!_Inited)
		_Inited = 1;

	m_noRect = 1;
	m_pltmId = 0;
	m_frameWidth = 0;
	m_frameHeight = 0;
	m_drawn = 0;
	m_fontSize = _D_pltmFontSize;
	m_textColor = _D_pltmMultilineLabelColor;

	m_text = "";

	PLTM_CreateStringFromUtf8(m_fontSize, &m_run, m_text.c_str());

	m_asciiPitch = 0;
	m_kanjiPitch = 0;
	m_hAlign = ALIGN_LEFT;
	m_vAlign = ALIGN_BASELINE;
	m_lineSpacing = 0;
	m_breakChar = 10;
	m_lineRecs = (PLTM_DrawSlot *)malloc(m_run.count * sizeof(PLTM_DrawSlot));
}

MultilineLabel::~MultilineLabel()
{
	PLTM_DestroyString(&m_run);
	if (m_lineRecs)
		free(m_lineRecs);
}

void MultilineLabel::DrawThis(gm_graph &g)
{
	if (!visible)
		return;

	CalcRectangle();

	PLTM_DrawSlot *lines = m_lineRecs;

	/* nlines first holds the x offset of the alignment, which is never used,
	   then the number of lines drawn. */
	int nlines;
	switch (m_hAlign) {
	case ALIGN_CENTER:
		nlines = -m_run.w >> 1;
		break;
	case ALIGN_RIGHT:
		nlines = -m_run.w;
		break;
	case ALIGN_LEFT:
	default:
		nlines = m_run.x;
		break;
	}

	if (m_drawn == 0 || PLTM_IsDirty(PLTM_ID(), -1, 0, &m_run)) {
		m_run.ready = 0;
		m_run.readyML = 0;
		nlines = PLTM_DrawStringGM_ML(PLTM_ID(), m_asciiPitch, m_kanjiPitch, m_frameWidth, m_frameHeight,
		    m_lineSpacing + (m_fontSize << 6), 0, 0, -1, 0, &m_run, &m_textColor, lines);
		if (nlines != m_run.count)
			m_drawn = 0;
		else
			m_drawn = 1;
	} else {
		PLTM_Touch(PLTM_ID(), &m_run);
		nlines = m_run.count;
	}

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
	g.BlendFunc(blend_mode);

	if (nlines <= 0)
		return;
	do {
		if (lines->tex) {
			g.SetArrays(lines, GM_VERTEX_PNCT, 4, 1, 0, 0);
			g.SetTexture(lines->tex);
			g.DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);
		}
		lines++;
	} while (--nlines);
}

int MultilineLabel::SetOption(int option, const int &value, unsigned int size)
{
	switch (option) {
	case OPT_FSIZ: {
		int v = value;
		m_fontSize = v;
		if (v <= 0)
			m_fontSize = 1;
		else if (v > 128)
			m_fontSize = 128;
		PLTM_DestroyString(&m_run);
		PLTM_CreateStringFromUtf8(m_fontSize, &m_run, m_text.c_str());
		free(m_lineRecs);
		m_lineRecs = (PLTM_DrawSlot *)malloc(sizeof(PLTM_DrawSlot) * m_run.count);
		m_noRect = 1;
		m_drawn = 0;
		return 1;
	}
	case OPT_LS64:
		m_lineSpacing = value;
		m_noRect = 1;
		m_drawn = 0;
		return 1;
	case OPT_TW64:
		m_frameWidth = value;
		m_noRect = 1;
		m_drawn = 0;
		m_run.ready = 0;
		m_run.readyML = 0;
		m_run.rectFinal = 0;
		return 1;
	case OPT_TH64:
		m_frameHeight = value;
		m_noRect = 1;
		m_drawn = 0;
		m_run.ready = 0;
		m_run.readyML = 0;
		m_run.rectFinal = 0;
		return 1;
	case OPT_FONT:
		m_pltmId = (void *)value;
		m_noRect = 1;
		m_drawn = 0;
		m_run.ready = 0;
		m_run.readyML = 0;
		return 1;
	case OPT_HALN:
		m_hAlign = value;
		m_noRect = 1;
		m_drawn = 0;
		m_run.ready = 0;
		m_run.readyML = 0;
		return 1;
	case OPT_VALN:
		m_vAlign = value;
		m_noRect = 1;
		m_drawn = 0;
		m_run.ready = 0;
		m_run.readyML = 0;
		return 1;
	case OPT_AP64:
		m_asciiPitch = value;
		return 1;
	case OPT_KP64:
		m_kanjiPitch = value;
		return 1;
	}
	return Object::SetOption(option, value, size);
}

int MultilineLabel::SetOption(int option, const String &value)
{
	if (option == OPT_UTF8) {
		PLTM_DestroyString(&m_run);
		m_text = value;
		PLTM_CreateStringFromUtf8(m_fontSize, &m_run, m_text.c_str());
		free(m_lineRecs);
		m_lineRecs = (PLTM_DrawSlot *)malloc(sizeof(PLTM_DrawSlot) * m_run.count);
		m_noRect = 1;
		m_run.ready = 0;
		m_run.readyML = 0;
		m_drawn = 0;
		return 1;
	}
	return Object::SetOption(option, value);
}

/* 'RECT' fills five words: enabled, x origin, then three layout values.
   'FONT' writes nothing and succeeds only when a font id is set. */
int MultilineLabel::GetOption(int option, void *data, unsigned int size) const
{
	if (option == OPT_RECT)
		goto h_rect;
	if (option > OPT_RECT)
		goto right;
	if (option == OPT_COL4)
		goto h_col4v;
	goto fallback;
right:
	if (option == OPT_FONT)
		goto h_font;
	goto fallback;

h_col4v:
	if (size < 16)
		return 0;
	*(PltmColor *)data = m_textColor;
	return 1;
h_font:
	if (!m_pltmId)
		goto fallback;
	return 1;
h_rect:
	{
		if (size < 16)  /* NOTE: bug: checks for 16 bytes, then writes 20 */
			return 0;

		int v;
		switch (m_hAlign) {
		case ALIGN_CENTER:
			v = -m_run.w >> 1;
			break;
		case ALIGN_RIGHT:
			v = -m_run.w;
			break;
		case ALIGN_LEFT:  /* redundant as written */
		default:
			v = m_run.x;
			break;
		}

		int *p = (int *)data;
		if (!m_run.rectFinal)
			p[0] = 0;
		else if (m_noRect)
			p[0] = 0;
		else
			p[0] = 1;
		p[1] = v;
		p[2] = m_run.top;
		p[3] = m_run.w;
		p[4] = m_run.h;
		return 1;
	}
fallback:
	return Object::GetOption(option, data, size);
}

int MultilineLabel::SetFrameSize64(int w, int h)
{
	m_frameWidth = w;
	m_frameHeight = h;
	m_noRect = 1;
	m_drawn = 0;
	return 1;
}

int MultilineLabel::SetOption(int option, const void *data, unsigned int size)
{
	switch (option) {
	case OPT_COL4:
		m_textColor = *(const PltmColor *)data;
		m_drawn = 0;
		m_run.ready = 0;
		m_run.readyML = 0;
		return 1;
	case OPT_FONT:
		m_pltmId = (void *)data;
		m_noRect = 1;
		m_drawn = 0;
		m_run.ready = 0;
		m_run.readyML = 0;
		return 1;
	}
	return Object::SetOption(option, data, size);
}

int MultilineLabel::SetOption(int option, const float &value, unsigned int size)
{
	return Object::SetOption(option, value, size);
}

int MultilineLabel::GetOption(int option, float &value, unsigned int size) const
{
	return Object::GetOption(option, value, size);
}

int MultilineLabel::GetOption(int option, int &value, unsigned int size) const
{
	switch (option) {
	case OPT_FSIZ:
		value = m_fontSize;
		return 1;
	case OPT_AP64:
		value = m_asciiPitch;
		return 1;
	case OPT_VALN:
		value = m_vAlign;
		return 1;
	case OPT_FONT:
		if (m_pltmId) {
			value = (int)m_pltmId;
			return 1;
		}
		/* FALLTHROUGH: without a font id, 'FONT' answers with the line spacing */
	case OPT_LS64:
		value = m_lineSpacing;
		return 1;
	case OPT_HALN:
		value = m_hAlign;
		return 1;
	case OPT_KP64:
		value = m_kanjiPitch;
		return 1;
	}
	return Object::GetOption(option, value, size);
}

int MultilineLabel::GetOption(int option, String &value) const
{
	if (option == OPT_UTF8) {
		value = m_text;
		return 1;
	}
	return Object::GetOption(option, value);
}

void MultilineLabel::CalcRectangle()
{
	void *id;
	if (m_pltmId)
		id = m_pltmId;
	else
		id = SceneGetDefaultPltmFontID();
	int combined = m_lineSpacing + (m_fontSize << 6);
	if (PLTM_GetTextRect_ML(id, m_asciiPitch, m_kanjiPitch, m_frameWidth, combined, &m_run) >= 0)
		m_noRect = 0;
	else
		m_noRect = 1;
}
