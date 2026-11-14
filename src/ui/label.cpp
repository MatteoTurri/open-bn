/*
 * Label widget: construction, options, layout and drawing.
 */

#include "label.h"

#include "gfx/vec.h"
#include "scene/scenefacade.h"
#include <stdlib.h>
#include "scene/optid.h"
#include "gfx/gm_graph.h"

/* Unit matrix and unit w row: the file-scope objects built at startup. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

bool Label::_Inited = false;

/* Class defaults. The colors are opaque white for the text, opaque black for
   the shadow. */
const int Label::_D_pltmAsciiCharPitch64 = 64;
const int Label::_D_pltmKanjiCharPitch64 = 64;
const PltmColor Label::_D_pltmLabelColor = { {
    { 0xff, 0xff, 0xff, 0xff },
    { 0xff, 0xff, 0xff, 0xff },
    { 0xff, 0xff, 0xff, 0xff },
    { 0xff, 0xff, 0xff, 0xff },
} };
const PltmColor Label::_D_pltmShadowLabelColor = { {
    { 0x00, 0x00, 0x00, 0xff },
    { 0x00, 0x00, 0x00, 0xff },
    { 0x00, 0x00, 0x00, 0xff },
    { 0x00, 0x00, 0x00, 0xff },
} };
const int Label::_D_pltmFontSize = 48;
const int Label::_D_HAlignment = ALIGN_LEFT;
const int Label::_D_VAlignment = ALIGN_BASELINE;

Label::Label(Object *parent, bool shadow, float scale) : Object(parent)
{
	m_shadowBlend = GM_BLEND_ALPHA;
	if (!_Inited)
		_Inited = 1;

	m_hasShadow = shadow;
	m_shadowScale = scale;
	m_pltmId = 0;
	m_waitAll = 1;
	m_shadowNoRect = 1;
	m_noRect = 1;
	m_omitDrawn = 0;
	m_drawn = 0;
	m_omitShadowDrawn = 0;
	m_shadowDrawn = 0;
	m_omitNoRect = 1;
	m_omitShadowNoRect = 1;
	m_fontSize = _D_pltmFontSize;

	m_textColor = _D_pltmLabelColor;
	m_shadowColor = _D_pltmShadowLabelColor;

	m_text = "";
	m_omitText = "...";

	m_widthLimit = 0;
	m_visibleChars = 0;
	m_shadowX = 128;
	m_shadowY = 128;

	PLTM_CreateStringFromUtf8(m_fontSize, &m_run, m_text.c_str());
	PLTM_CreateStringFromUtf8(m_fontSize, &m_omitRun, m_omitText.c_str());

	if (m_hasShadow != 0) {
		PLTM_CreateStringFromUtf8((int)(m_fontSize * m_shadowScale), &m_shadowRun, m_text.c_str());
		PLTM_CreateStringFromUtf8((int)(m_fontSize * m_shadowScale), &m_omitShadowRun, m_omitText.c_str());
	}

	m_asciiPitch = 64;
	m_kanjiPitch = 64;
	m_hAlign = ALIGN_LEFT;
	m_vAlign = ALIGN_BASELINE;
	m_omitOffsetX = 0;

	m_lineRecs = (PLTM_DrawSlot *)malloc(sizeof(PLTM_DrawSlot) * m_run.count);
	m_omitLineRecs = (PLTM_DrawSlot *)malloc(sizeof(PLTM_DrawSlot) * m_omitRun.count);

	if (m_hasShadow != 0) {
		m_shadowLineRecs = (PLTM_DrawSlot *)malloc(sizeof(PLTM_DrawSlot) * m_shadowRun.count);
		m_omitShadowLineRecs = (PLTM_DrawSlot *)malloc(sizeof(PLTM_DrawSlot) * m_omitShadowRun.count);
	} else {
		m_omitShadowLineRecs = 0;
		m_shadowLineRecs = 0;
	}
}

/* Destroys the PLTM runs (the shadow ones only with a shadow) and frees their
   line records. */
Label::~Label()
{
	PLTM_DestroyString(&m_run);
	PLTM_DestroyString(&m_omitRun);
	if (m_hasShadow) {
		PLTM_DestroyString(&m_shadowRun);
		PLTM_DestroyString(&m_omitShadowRun);
	}
	if (m_lineRecs)
		free(m_lineRecs);
	if (m_omitLineRecs)
		free(m_omitLineRecs);
	if (m_shadowLineRecs)
		free(m_shadowLineRecs);
	if (m_omitShadowLineRecs)
		free(m_omitShadowLineRecs);
}

/* Returns the font id, or the scene default when none is set. */
void *Label::PLTM_ID()
{
	void *id = m_pltmId;
	if (id)
		return id;
	return SceneGetDefaultPltmFontID();
}

/*
 * Drawing uses four independent runs, each redrawn only when dirty
 * (PLTM_IsDirty -> PLTM_DrawStringGM, otherwise PLTM_Touch); the shadow phase
 * is drawn before the text:
 *   m_run            records m_lineRecs            colors m_textColor    drawn m_drawn
 *   m_omitRun        records m_omitLineRecs        colors m_textColor    drawn m_omitDrawn
 *   m_shadowRun      records m_shadowLineRecs      colors m_shadowColor  drawn m_shadowDrawn
 *   m_omitShadowRun  records m_omitShadowLineRecs  colors m_shadowColor  drawn m_omitShadowDrawn
 */


/* Draws cnt line records starting at r, advancing r; records without a
   texture are skipped. */
#define LABEL_DRAW_RUNS(gg, r, cnt)					\
	for (int k_ = 0; k_ < (cnt); k_++) {				\
		if ((r)->tex) {					\
			(gg).SetArrays((r), GM_VERTEX_PNCT, 4, 1, 0, 0);		\
			(gg).SetTexture((r)->tex);			\
			(gg).DrawArrays(GM_PRIM_TRIANGLE_STRIP, 0, 4, 1, 0);			\
		}							\
		(r)++;							\
	}

void Label::DrawThis(gm_graph &g)
{
	long long x, y, y2;
	int enabled;
	int textCount, omitCount, shadowCount, omitShadowCount;
	PLTM_DrawSlot *textSlots, *omitSlots, *shadowSlots, *omitShadowSlots;

	if (!visible)
		return;

	enabled = (m_run.rectFinal != 0);

	CalcRectangle();
	CalcRectangle_OmitString();
	if (m_hasShadow) {
		CalcRectangle_Shadow();
		CalcRectangle_OmitString_Shadow();
	}

	if (m_run.rectFinal && m_omitRun.rectFinal) {
		int oldOffsetX = m_omitOffsetX;
		m_visibleChars = PLTM_CountVisibleChar(PLTM_ID(), m_asciiPitch, m_kanjiPitch,
		    m_widthLimit - m_omitRun.w, &m_run, &m_omitOffsetX);
		if (m_omitOffsetX != oldOffsetX) {
			m_drawn = 0;
			m_shadowDrawn = 0;
			m_omitDrawn = 0;
			m_omitShadowDrawn = 0;
			m_run.ready = 0;
			m_omitRun.ready = 0;
			if (m_hasShadow) {
				m_shadowRun.ready = 0;
				m_omitShadowRun.ready = 0;
			}
		}
	}

	textSlots = m_lineRecs;
	omitSlots = m_omitLineRecs;
	shadowSlots = m_shadowLineRecs;
	omitShadowSlots = m_omitShadowLineRecs;

	DecideOrigin(x, y);
	shadowCount = 0;
	omitShadowCount = 0;
	y2 = y;

	/* main run, +672 */
	{
		if (m_drawn == 0 || PLTM_IsDirty(PLTM_ID(), -1, 0, &m_run)) {
			DecideOrigin(x, y);
			m_run.ready = 0;
			textCount = PLTM_DrawStringGM(PLTM_ID(), m_asciiPitch, m_kanjiPitch, x, y, -1, 0, &m_run,
			    &m_textColor, textSlots);
			if (textCount != m_run.count) {
				m_drawn = 0;
				m_run.rectFinal = 0;
			} else {
				m_drawn = 1;
			}
		} else {
			PLTM_Touch(PLTM_ID(), &m_run);
			textCount = m_run.count;
		}
	}

	/* abbreviated run, +720 */
	{
		if (m_omitDrawn == 0 || PLTM_IsDirty(PLTM_ID(), -1, 0, &m_omitRun)) {
			long long x2 = x + m_omitOffsetX;

			m_omitRun.ready = 0;
			omitCount = PLTM_DrawStringGM(PLTM_ID(), m_asciiPitch, m_kanjiPitch, x2, y2, -1, 0, &m_omitRun,
			    &m_textColor, omitSlots);
			if (omitCount != m_omitRun.count) {
				m_omitDrawn = 0;
			} else if (enabled) {
				m_omitDrawn = 1;
			} else {
				m_omitRun.rectFinal = 0;
			}
		} else {
			PLTM_Touch(PLTM_ID(), &m_omitRun);
			omitCount = m_omitRun.count;
		}
	}

	if (m_hasShadow) {
		/* shadow run, +772 */
		{
			if (m_shadowDrawn == 0 || PLTM_IsDirty(PLTM_ID(), -1, 0, &m_shadowRun)) {
				m_shadowRun.ready = 0;
				shadowCount = PLTM_DrawStringGM(PLTM_ID(), (int)((float)m_asciiPitch * m_shadowScale),
				    (int)((float)m_kanjiPitch * m_shadowScale), x + m_shadowX, y + m_shadowY, -1, 0,
				    &m_shadowRun, &m_shadowColor, shadowSlots);
				if (shadowCount != m_shadowRun.count)
					m_shadowDrawn = 0;
				else
					m_shadowDrawn = 1;
			} else {
				PLTM_Touch(PLTM_ID(), &m_shadowRun);
				shadowCount = m_shadowRun.count;
			}
		}

		/* abbreviated shadow run, +820 */
		{
			if (m_omitShadowDrawn == 0 || PLTM_IsDirty(PLTM_ID(), -1, 0, &m_omitShadowRun)) {
				long long x4;

				m_omitShadowRun.ready = 0;
				x4 = x + m_omitOffsetX + m_shadowX;
				y2 += m_shadowY;
				omitShadowCount =
				    PLTM_DrawStringGM(PLTM_ID(), (int)((float)m_asciiPitch * m_shadowScale),
				        (int)((float)m_kanjiPitch * m_shadowScale), x4, y2, -1, 0, &m_omitShadowRun,
				        &m_shadowColor, omitShadowSlots);
				if (omitShadowCount != m_omitShadowRun.count) {
					m_omitShadowDrawn = 0;
				} else if (enabled) {
					m_omitShadowDrawn = 1;
				} else {
					m_omitShadowRun.rectFinal = 0;
				}
			} else {
				PLTM_Touch(PLTM_ID(), &m_omitShadowRun);
				omitShadowCount = m_omitShadowRun.count;
			}
		}
	}

	if (m_waitAll) {
		if (!m_run.ready)
			return;
		if (!m_omitRun.ready)
			return;
	}

	if (m_hasShadow) {
		if (m_waitAll) {
			if (!m_shadowRun.ready)
				return;
			if (!m_omitShadowRun.ready)
				return;
		}
		g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT);
		g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
		g.BlendFunc(m_shadowBlend);
		/* No width limit, or the text fits: draw the whole run. Otherwise draw the
		   visible lines plus the abbreviated run. */
		if (m_widthLimit == 0 || m_widthLimit >= m_shadowRun.w) {
			LABEL_DRAW_RUNS(g, shadowSlots, shadowCount);
		} else {
			int n = (shadowCount < m_visibleChars) ? shadowCount : m_visibleChars;
			if (m_shadowDrawn) {
				LABEL_DRAW_RUNS(g, shadowSlots, n);
			}
			if (m_omitShadowDrawn) {
				LABEL_DRAW_RUNS(g, omitShadowSlots, omitShadowCount);
			}
		}
	}

	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
	g.BlendFunc(blend_mode);
	if (m_widthLimit == 0 || m_widthLimit >= m_run.w) {
		LABEL_DRAW_RUNS(g, textSlots, textCount);
	} else {
		int n = (textCount < m_visibleChars) ? textCount : m_visibleChars;
		if (m_drawn) {
			LABEL_DRAW_RUNS(g, textSlots, n);
		}
		if (m_omitDrawn) {
			LABEL_DRAW_RUNS(g, omitSlots, omitCount);
		}
	}
}

/* Recreates a PLTM run from text at the given font size. */
static inline void RefreshRun(int size, PLTM_String *run, const String &text)
{
	PLTM_DestroyString(run);
	PLTM_CreateStringFromUtf8(size, run, text.c_str());
}

int Label::SetOption(int option, const int &value, unsigned int size)
{
	switch (option) {
	case OPT_W64: {
		if (m_widthLimit == value)
			return 1;
		m_widthLimit = value;
		m_shadowNoRect = 1;
		m_noRect = 1;
		m_drawn = 0;
		m_shadowDrawn = 0;
		m_omitDrawn = 0;
		m_omitShadowDrawn = 0;
		m_run.ready = 0;
		m_omitRun.ready = 0;
		if (m_hasShadow != 0) {
			m_shadowRun.ready = 0;
			m_omitShadowRun.ready = 0;
		}
		return 1;
	}
	case OPT_FSIZ: {
		int v = value;
		m_fontSize = v;
		if (v <= 0)
			m_fontSize = 1;
		else if (v >= 129)
			m_fontSize = 128;

		PLTM_DestroyString(&m_run);
		PLTM_CreateStringFromUtf8(m_fontSize, &m_run, m_text.c_str());
		if (m_hasShadow != 0) {
			PLTM_DestroyString(&m_shadowRun);
			PLTM_CreateStringFromUtf8(m_fontSize, &m_shadowRun, m_text.c_str());
		}

		m_shadowNoRect = 1;
		m_noRect = 1;
		m_omitDrawn = 0;
		m_drawn = 0;
		m_omitShadowDrawn = 0;
		m_shadowDrawn = 0;
		PLTM_DestroyString(&m_omitRun);
		PLTM_CreateStringFromUtf8(m_fontSize, &m_omitRun, m_omitText.c_str());

		if (m_hasShadow != 0) {
			PLTM_DestroyString(&m_omitShadowRun);
			PLTM_CreateStringFromUtf8((int)(m_fontSize * m_shadowScale), &m_omitShadowRun,
			    m_omitText.c_str());
		}
		m_omitShadowNoRect = 1;
		m_omitNoRect = 1;
		return 1;
	}
	case OPT_FONT:
		m_pltmId = (void *)value;
		m_shadowNoRect = 1;
		m_noRect = 1;
		m_omitDrawn = 0;
		m_drawn = 0;
		m_omitShadowDrawn = 0;
		m_shadowDrawn = 0;
		m_run.ready = 0;
		m_omitRun.ready = 0;
		if (m_hasShadow != 0) {
			m_shadowRun.ready = 0;
			m_omitShadowRun.ready = 0;
		}
		return 1;
	case OPT_HALN:
		m_hAlign = value;
		m_shadowNoRect = 1;
		m_noRect = 1;
		m_omitDrawn = 0;
		m_drawn = 0;
		m_omitShadowDrawn = 0;
		m_shadowDrawn = 0;
		m_run.ready = 0;
		m_omitRun.ready = 0;
		if (m_hasShadow != 0) {
			m_shadowRun.ready = 0;
			m_omitShadowRun.ready = 0;
		}
		return 1;
	case OPT_VALN:
		m_vAlign = value;
		m_shadowNoRect = 1;
		m_noRect = 1;
		m_omitDrawn = 0;
		m_drawn = 0;
		m_omitShadowDrawn = 0;
		m_shadowDrawn = 0;
		m_run.ready = 0;
		m_omitRun.ready = 0;
		if (m_hasShadow != 0) {
			m_shadowRun.ready = 0;
			m_omitShadowRun.ready = 0;
		}
		return 1;
	case OPT_AP64:
		m_asciiPitch = value;
		m_run.ready = 0;
		m_omitRun.ready = 0;
		if (m_hasShadow != 0) {
			m_shadowRun.ready = 0;
			m_omitShadowRun.ready = 0;
		}
		return 1;
	case OPT_KP64:
		m_kanjiPitch = value;
		m_run.ready = 0;
		m_omitRun.ready = 0;
		if (m_hasShadow != 0) {
			m_shadowRun.ready = 0;
			m_omitShadowRun.ready = 0;
		}
		return 1;
	case OPT_ALTG:
		m_waitAll = value != 0;
		return 1;
	case OPT_SX64:
		m_shadowX = value;
		if (m_hasShadow != 0) {
			m_shadowRun.ready = 0;
			m_omitShadowRun.ready = 0;
		}
		return 1;
	case OPT_SY64:
		m_shadowY = value;
		m_run.ready = 0;
		m_omitRun.ready = 0;
		if (m_hasShadow != 0) {
			m_shadowRun.ready = 0;
			m_omitShadowRun.ready = 0;
		}
		return 1;
	case OPT_SALP:
		m_shadowBlend = value;
		return 1;
	}
	return Object::SetOption(option, value, size);
}

int Label::SetOption(int option, const String &value)
{
	if (option == OPT_UTF8) {
		/* Nothing to do when both the new and the current text are empty. */
		if (value[0] == 0 && m_text[0] == 0)
			return 1;

		{
			PLTM_DestroyString(&m_run);
			m_text = value;
			PLTM_CreateStringFromUtf8(m_fontSize, &m_run, m_text.c_str());
			free(m_lineRecs);
			m_lineRecs = (PLTM_DrawSlot *)malloc(sizeof(PLTM_DrawSlot) * m_run.count);

			if (m_hasShadow != 0) {
				PLTM_DestroyString(&m_shadowRun);
				PLTM_CreateStringFromUtf8(m_fontSize, &m_shadowRun, m_text.c_str());
				free(m_shadowLineRecs);
				m_shadowLineRecs = (PLTM_DrawSlot *)malloc(sizeof(PLTM_DrawSlot) * m_shadowRun.count);
			}

			m_shadowNoRect = 1;
			m_noRect = 1;
			m_run.ready = 0;
			if (m_hasShadow != 0) {
				m_shadowRun.ready = 0;
			}
			m_drawn = 0;
			m_shadowDrawn = 0;
			return 1;
		}
	}
	return Object::SetOption(option, value);
}

int Label::GetOption(int option, void *data, unsigned int size) const
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

		int left;
		switch (m_hAlign) {
		case ALIGN_CENTER:
			left = (-m_run.w) >> 1;
			break;
		case ALIGN_RIGHT:
			left = -m_run.w;
			break;
		case ALIGN_LEFT:
		default:
			left = m_run.x;
			break;
		}

		int top;
		switch (m_vAlign) {
		case ALIGN_TOP:
			top = 0;
			break;
		case ALIGN_CENTER:
			top = m_run.h >> 1;
			break;
		case ALIGN_BOTTOM:
			top = m_run.h;
			break;
		case ALIGN_BASELINE:
		default:
			top = m_run.y;
			break;
		}

		int *out = (int *)data;
		if (!m_run.rectFinal)
			out[0] = 0;
		else if (m_noRect)
			out[0] = 0;
		else
			out[0] = 1;

		out[1] = (flip & 1) ? -left : left;
		out[2] = (flip & 2) ? -top : top;
		out[3] = m_run.w;
		out[4] = m_run.h;
		return 1;
	}
fallback:
	return Object::GetOption(option, data, size);
}

void Label::DecideOrigin(long long &x, long long &y)
{
	switch (m_hAlign) {
	case ALIGN_CENTER: {
		int len = m_widthLimit;
		if (len == 0 || m_run.w < len) {
			x = (-m_run.w) >> 1;
			x -= m_run.x >> 1;
			break;
		}
		x = (-len) >> 1;
		x -= m_run.x >> 1;
		break;
	}
	case ALIGN_RIGHT: {
		int len = m_widthLimit;
		if (len == 0 || m_run.w < len) {
			x = -m_run.w;
			x -= m_run.x;
		} else {
			x = -len;
			x -= m_run.x;
		}
		break;
	}
	case ALIGN_LEFT:
	default:
		x = 0;
		break;
	}

	switch (m_vAlign) {
	case ALIGN_TOP:
		y = m_run.y;
		break;
	case ALIGN_BOTTOM:
		y = m_run.y - m_run.h;
		break;
	case ALIGN_CENTER:
		y = m_run.y - (m_run.h >> 1);
		break;
	case ALIGN_BASELINE:
	default:
		y = 0;
		break;
	}
}

void Label::_Init()
{
	_Inited = 1;
}

int Label::SetOption(int option, const void *data, unsigned int size)
{
	if (option == OPT_COL4)
		goto h_col4;
	if (option > OPT_COL4)
		goto right;
	if (option == OPT_SCL4)
		goto h_scl4;
	goto fallback;
right:
	if (option == OPT_FONT)
		goto h_font;
	goto fallback;

h_col4:
	m_textColor = *(const PltmColor *)data;
	m_omitDrawn = 0;
	m_drawn = 0;
	m_run.ready = 0;
	m_omitRun.ready = 0;
	return 1;
h_scl4:
	m_shadowColor = *(const PltmColor *)data;
	m_omitShadowDrawn = 0;
	m_shadowDrawn = 0;
	if (m_hasShadow != 0) {
		m_shadowRun.ready = 0;
		m_omitShadowRun.ready = 0;
	}
	return 1;
h_font:
	{
		void *id = (void *)data;
		if (m_pltmId == id)
			return 1;
		m_pltmId = id;
		m_shadowNoRect = 1;
		m_noRect = 1;
		m_omitNoRect = 1;
		m_omitShadowNoRect = 1;
		m_omitDrawn = 0;
		m_drawn = 0;
		m_omitShadowDrawn = 0;
		m_shadowDrawn = 0;
		m_run.ready = 0;
		m_omitRun.ready = 0;
		if (m_hasShadow != 0) {
			m_shadowRun.ready = 0;
			m_omitShadowRun.ready = 0;
		}
		return 1;
	}
fallback:
	return Object::SetOption(option, data, size);
}

int Label::SetOption(int option, const float &value, unsigned int size)
{
	if (option == OPT_SRES) {
		m_shadowScale = value;
		return 1;
	}
	return Object::SetOption(option, value, size);
}

int Label::GetOption(int option, float &value, unsigned int size) const
{
	return Object::GetOption(option, value, size);
}

int Label::GetOption(int option, int &value, unsigned int size) const
{
	switch (option) {
	case OPT_W64:
		value = m_widthLimit;
		return 1;
	case OPT_FSIZ:
		value = m_fontSize;
		return 1;
	case OPT_FONT:
		if (!m_pltmId)
			break;
		value = (int)m_pltmId;
		return 1;
	case OPT_HALN:
		value = m_hAlign;
		return 1;
	case OPT_VALN:
		value = m_vAlign;
		return 1;
	case OPT_AP64:
		value = m_asciiPitch;
		return 1;
	case OPT_KP64:
		value = m_kanjiPitch;
		return 1;
	case OPT_ALTG:
		value = m_waitAll;
		return 1;
	}
	return Object::GetOption(option, value, size);
}

int Label::GetOption(int option, String &value) const
{
	if (option == OPT_UTF8) {
		value = m_text;
		return 1;
	}
	return Object::GetOption(option, value);
}

void Label::CalcRectangle()
{
	void *id = PLTM_ID();
	if (PLTM_GetTextRect(id, m_asciiPitch, m_kanjiPitch, &m_run) >= 0)
		m_noRect = 0;
	else
		m_noRect = 1;
}

void Label::CalcRectangle_OmitString()
{
	void *id = PLTM_ID();
	if (PLTM_GetTextRect(id, m_asciiPitch, m_kanjiPitch, &m_omitRun) >= 0)
		m_omitNoRect = 0;
	else
		m_omitNoRect = 1;
}

void Label::CalcRectangle_Shadow()
{
	void *id = PLTM_ID();
	int w = (int)((float)m_asciiPitch * m_shadowScale);
	int h = (int)((float)m_kanjiPitch * m_shadowScale);
	if (PLTM_GetTextRect(id, w, h, &m_shadowRun) >= 0)
		m_shadowNoRect = 0;
	else
		m_shadowNoRect = 1;
}

void Label::CalcRectangle_OmitString_Shadow()
{
	void *id = PLTM_ID();
	int w = (int)((float)m_asciiPitch * m_shadowScale);
	int h = (int)((float)m_kanjiPitch * m_shadowScale);
	if (PLTM_GetTextRect(id, w, h, &m_omitShadowRun) >= 0)
		m_omitShadowNoRect = 0;
	else
		m_omitShadowNoRect = 1;
}
