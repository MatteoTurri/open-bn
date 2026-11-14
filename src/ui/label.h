/*
 * Label widget class.
 */

#ifndef LABEL_H
#define LABEL_H

#include <vector>
#include <string>
#include "gfx/pltm.h"
#include "scene/object.h"

class gm_graph;

/*
 * Single-line text widget drawn with PLTM, with an optional drop shadow and an
 * abbreviated ("...") form used when the text does not fit. Each form is a PLTM
 * "run": main, abbreviated, shadow and abbreviated shadow.
 */
class Label : public Object {
public:
	class ClassInitializer {
	public:
		ClassInitializer();
	};

	/* 0x250 */ String m_text;  /* text, option 'UTF8' */
	/* 0x254 */ String m_omitText;  /* abbreviated text, "..." by default */
	/* 0x258 */ PLTM_DrawSlot *m_lineRecs;  /* one per character of the main run */
	/* 0x25C */ PLTM_DrawSlot *m_omitLineRecs;  /* of the abbreviated run */
	/* 0x260 */ PLTM_DrawSlot *m_shadowLineRecs;  /* of the shadow run (0 without shadow) */
	/* 0x264 */ PLTM_DrawSlot *m_omitShadowLineRecs;  /* of the abbreviated shadow run (0 without shadow) */
	/* 0x268 */ int m_omitOffsetX;  /* x offset of the abbreviated run, from PLTM_CountVisibleChar */
	/* 0x26C */ int m_noRect;  /* 1 while the main run has no valid text rect */
	/* 0x270 */ int m_omitNoRect;  /* same, abbreviated run */
	/* 0x274 */ int m_shadowNoRect;  /* same, shadow run */
	/* 0x278 */ int m_omitShadowNoRect;  /* same, abbreviated shadow run */
	/* 0x27C */ char pad_27C[8];
	/* 0x284 */ int m_drawn;  /* main run drawn and up to date */
	/* 0x288 */ int m_omitDrawn;  /* same, abbreviated run */
	/* 0x28C */ int m_shadowDrawn;  /* same, shadow run */
	/* 0x290 */ int m_omitShadowDrawn;  /* same, abbreviated shadow run */
	/* 0x294 */ int m_widthLimit;  /* width limit, option 'W64 ' (0 = none) */
	/* 0x298 */ int m_visibleChars;  /* number of visible characters */
	/* 0x29C */ void *m_pltmId;  /* font handle, option 'FONT' (0 = scene default) */
	/* 0x2A0 */ PLTM_String m_run;  /* main text */
	/* 0x2D0 */ PLTM_String m_omitRun;  /* abbreviated text */
	/* 0x300 */ int m_shadowBlend;  /* shadow blend mode, option 'SALP' */
	/* 0x304 */ PLTM_String m_shadowRun;
	/* 0x334 */ PLTM_String m_omitShadowRun;
	/* 0x364 */ int m_asciiPitch;  /* ASCII character pitch in 1/64, option 'AP64' */
	/* 0x368 */ int m_kanjiPitch;  /* kanji character pitch in 1/64, option 'KP64' */
	/* 0x36C */ PltmColor m_textColor;  /* text colors, option 'COL4' */
	/* 0x37C */ PltmColor m_shadowColor;  /* shadow colors, option 'SCL4' */
	/* 0x38C */ int m_fontSize;  /* font size 1..128, option 'FSIZ' */
	/* 0x390 */ float m_shadowScale;  /* shadow size scale, option 'SRES' */
	/* 0x394 */ int m_shadowX;  /* shadow x offset, option 'SX64' */
	/* 0x398 */ int m_shadowY;  /* shadow y offset, option 'SY64' */
	/* 0x39C */ int m_hAlign;  /* horizontal alignment, option 'HALN', ALIGN_* */
	/* 0x3A0 */ int m_vAlign;  /* vertical alignment, option 'VALN', ALIGN_* */
	/* 0x3A4 */ int m_hasShadow;  /* has a drop shadow (constructor argument) */
	/* 0x3A8 */ int m_waitAll;  /* draw only once every run is ready, option 'ALTG' */

	Label(Object *parent, bool shadow, float scale);
	virtual ~Label();

	void *PLTM_ID();
	void _Init();

	/* Initialization flag and class defaults. */
	static bool _Inited;
	static const int _D_pltmAsciiCharPitch64;
	static const int _D_pltmKanjiCharPitch64;
	static const PltmColor _D_pltmLabelColor;
	static const PltmColor _D_pltmShadowLabelColor;
	static const int _D_pltmFontSize;
	static const int _D_HAlignment;
	static const int _D_VAlignment;
	void CalcRectangle();
	void CalcRectangle_OmitString();
	void CalcRectangle_Shadow();
	void CalcRectangle_OmitString_Shadow();
	void DecideOrigin(long long &x, long long &y);

	virtual void DrawThis(gm_graph &g);

	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const String &value);
	virtual int SetOption(int option, const void *data, unsigned int size);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, String &value) const;
	virtual int GetOption(int option, void *data, unsigned int size) const;
};  /* size = 0x3AC */

#endif
