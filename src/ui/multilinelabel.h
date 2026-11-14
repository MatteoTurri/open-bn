/*
 * MultilineLabel widget class.
 */

#ifndef MULTILINELABEL_H
#define MULTILINELABEL_H

#include "scene/scenefacade.h"
#include <vector>
#include <string>
#include "gfx/pltm.h"
#include "scene/object.h"

class gm_graph;

/*
 * Multi-line text widget drawn with PLTM inside a frame (SetFrameSize64).
 */
class MultilineLabel : public Object {
public:
	class ClassInitializer {
	public:
		ClassInitializer();
	};

	/* 0x250 */ int m_frameWidth;  /* frame width, option 'TW64' */
	/* 0x254 */ int m_frameHeight;  /* frame height, option 'TH64' */
	/* 0x258 */ String m_text;  /* text, option 'UTF8' */
	/* 0x25C */ PLTM_DrawSlot *m_lineRecs;  /* one per character of the run */
	/* 0x260 */ int m_noRect;  /* 1 while the run has no valid text rect */
	/* 0x264 */ char pad_264[8];
	/* 0x26C */ int m_drawn;  /* run drawn and up to date */
	/* 0x270 */ void *m_pltmId;  /* font handle, option 'FONT' (0 = scene default) */
	/* 0x274 */ PLTM_String m_run;
	/* 0x2A4 */ int m_asciiPitch;  /* ASCII character pitch in 1/64, option 'AP64' */
	/* 0x2A8 */ int m_kanjiPitch;  /* kanji character pitch in 1/64, option 'KP64' */
	/* 0x2AC */ PltmColor m_textColor;  /* text colors, option 'COL4' */
	/* 0x2BC */ int m_fontSize;  /* font size 1..128, option 'FSIZ' */
	/* 0x2C0 */ int m_hAlign;  /* horizontal alignment, option 'HALN', ALIGN_* */
	/* 0x2C4 */ int m_vAlign;  /* vertical alignment, option 'VALN', ALIGN_* */
	/* 0x2C8 */ int m_lineSpacing;  /* line spacing in 1/64, option 'LS64' */
	/* 0x2CC */ short m_breakChar;  /* line break character ('\n') */

	MultilineLabel(Object *parent);
	virtual ~MultilineLabel();

	/* Returns the font handle, or the scene default when none is set. */
	void *PLTM_ID()
	{
		void *id = m_pltmId;
		if (id)
			return id;
		return SceneGetDefaultPltmFontID();
	}
	void _Init();
	void CalcRectangle();
	virtual int SetFrameSize64(int w, int h);

	/* Initialization flag and class defaults. */
	static bool _Inited;
	static const int _D_pltmAsciiCharPitch64;
	static const int _D_pltmKanjiCharPitch64;
	static const int _D_pltmLineSpace64;
	static const short _D_pltmCRcode;
	static const PltmColor _D_pltmMultilineLabelColor;
	static const int _D_pltmFontSize;
	static const int _D_HAlignment;
	static const int _D_VAlignment;

	virtual void DrawThis(gm_graph &g);

	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const String &value);
	virtual int SetOption(int option, const void *data, unsigned int size);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, String &value) const;
	virtual int GetOption(int option, void *data, unsigned int size) const;
};  /* size = 0x2D0 */

#endif
