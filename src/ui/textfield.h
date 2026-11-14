/*
 * TextField: an editable text field (or a label) of the 3D scene, derived from Object.
 */

#ifndef TEXTFIELD_H
#define TEXTFIELD_H

#include <vector>
#include <string>
#include "gfx/vec_types.h"
#include "scene/object.h"
#include "gfx/gm_vertex.h"
#include "gfx/pltm.h"
#include "scene/optid.h"
#include <stdlib.h>

class gm_graph;

/* CharRec::flags. */
#define CHARREC_SELECTED 0x01

/* Record of one character of buf (6 bytes). */
class CharRec {
public:
	/* 0x00 */ unsigned char flags;  /* CHARREC_* */
	/* 0x01 */ char unk_01;
	/* 0x02 */ short width;         /* glyph width in 1/64, -1 until measured */
	/* 0x04 */ unsigned short pitch; /* character pitch in 1/64 */
	CharRec() : flags(0), unk_01(0), width(0) {} /* pitch is left uninitialised */
	~CharRec() {}
};

/* Owning pointers: buf and xtab are freed with TextField. */
class OwnedCharBuf {
public:
	CharRec *p;
	~OwnedCharBuf() { delete[] p; }
	operator CharRec *() const { return p; }
	CharRec *operator=(CharRec *v) { return p = v; }
};

class OwnedShortArray {
public:
	unsigned short *p;
	~OwnedShortArray()
	{
		if (p)
			free(p);
	}   /* allocated with malloc */
	operator unsigned short *() const { return p; }
	unsigned short *operator=(unsigned short *v) { return p = v; }
};

class OwnedPtr {
public:
	char *p;
	~OwnedPtr() { delete p; }
	operator char *() const { return p; }
	char *operator=(char *v) { return p = v; }
};

class TextField : public Object {
public:
	/* Passed to m_modifyVerifyCallback before an insertion; the callback may clear allow. */
	struct CallData_ModifyVerify {
		/* 0x00 */ TextField *editor;
		/* 0x04 */ int option;  /* TF_VERIFY_* */
		/* 0x08 */ int allow;  /* 1 by default */
		/* 0x0C */ void *data;
	};

	/* CallData_ModifyVerify::option: the encoding of data. */
	enum {
		TF_VERIFY_UCS2 = 0,
		TF_VERIFY_UTF8 = 1
	};

	/* Paired with bInited, the guard of the class constants. */
	class ClassInitializer {
	public:
		ClassInitializer();
	};

	/* Action codes: 0 and 1 are handled only by Action(code, unsigned char *),
	   3, 8 and 9 do nothing, 13 is outside the range of Action(code). */
	enum tActionCode {
		AC_InsertString_Utf8_ = 0,
		AC_InsertString_Ucs2_ = 1,
		AC_SelectAll_ = 2,
		AC_Reserved3_ = 3,
		AC_Begin_ = 4,
		AC_End_ = 5,
		AC_PrevChar_ = 6,
		AC_NextChar_ = 7,
		AC_PrevWord_ = 8,
		AC_NextWord_ = 9,
		AC_DeletePrevChar_ = 10,
		AC_DeleteNextChar_ = 11,
		AC_DeleteAll_ = 12,
		AC_DeSelectAll_ = 13
	};

	/* 0x250 */ void (*m_modifyVerifyCallback)(CallData_ModifyVerify *);
	/* 0x254 */ int m_maxLen;  /* allocated capacity */
	/* 0x258 */ OwnedCharBuf m_buf;  /* array of CharRec */
	/* 0x25C */ OwnedShortArray m_xtab;  /* malloc block of 2 * (m_maxLen+1) shorts */
	/* 0x260 */ unsigned short *m_xtabMasked;  /* second half of the xtab block */
	/* 0x264 */ int m_length;  /* characters in use */
	/* 0x268 */ int m_visibleChars;  /* characters drawn by the last pass */
	/* 0x26C */ int m_numSelected;  /* selected characters */
	/* 0x270 */ gmVertexPC m_fieldQuad[4];  /* quad of the field (vertex format 5) */
	/* 0x2B0 */ gmVertexPC m_cursorQuad[4];
	/* 0x2F0 */ int m_textDirty;     /* rebuild the shown string before drawing */
	/* 0x2F4 */ int unk_2F4;       /* set with m_textDirty, never read */
	/* 0x2F8 */ PLTM_DrawSlot *m_glyphQuads;  /* one per character (m_maxLen), freed by the destructor */
	/* 0x2FC */ int m_rectDirty;     /* recompute the text rectangle */
	/* 0x300 */ int m_modified;  /* 1 after an edit */
	/* 0x304 */ int m_textDrawn;     /* the glyph quads are up to date */
	/* 0x308 */ int m_cursorPos;
	/* 0x30C */ int m_cursorX;       /* cursor position in 1/64, option "CX64" */
	/* 0x310 */ int m_offsetX;       /* scroll offset in 1/64, option "OX64" */
	/* 0x314 */ int m_leftMargin;    /* in 1/64, option "LM64" */
	/* 0x318 */ int m_rightMargin;   /* in 1/64, option "RM64" */
	/* 0x31C */ int unk_31C;
	/* 0x320 */ int unk_320;
	/* 0x324 */ int m_blinkInterval;  /* initially 15 */
	/* 0x328 */ int m_blinkTimer;  /* reset to m_blinkInterval + 1 by ResetCursorBlink */
	/* 0x32C */ int m_cursorBlinkOn; /* blink phase */
	/* 0x330 */ int m_cursorShown;   /* option "CRSW" */
	/* 0x334 */ int m_editable;  /* nonzero: drawn as an editable field, else as a label */
	/* 0x338 */ int m_passwordMode;  /* option "PWMD": the shown string is m_xtabMasked */
	/* 0x33C */ int m_width64;  /* width, 26.6 fixed point */
	/* 0x340 */ int m_height64;  /* height, 26.6 fixed point */
	/* 0x344 */ void *m_pltmId;
	/* 0x348 */ PLTM_String m_shownStr;  /* text as drawn */
	/* 0x378 */ PLTM_String m_plainStr;  /* text from xtab, also in password mode */
	/* 0x3A8 */ PLTM_String *m_shown;
	/* 0x3AC */ PLTM_String *m_plain;
	/* 0x3B0 */ int m_textReady;     /* the shown string was drawn completely */
	/* 0x3B4 */ int m_prevTextReady;
	/* 0x3B8 */ int m_asciiPitch;  /* initially 64 */
	/* 0x3BC */ int m_kanjiPitch;  /* initially 64 */
	/* 0x3C0 */ int m_fontSize;      /* option "FSIZ" */
	/* 0x3C4 */ int m_hAlign;        /* option "HALN", ALIGN_* */
	/* 0x3C8 */ int m_vAlign;        /* option "VALN", ALIGN_* */
	/* 0x3CC */ PltmColor m_textColor;  /* copy of _D_pltmTextFieldColor */
	/* 0x3DC */ int unk_3DC;

	static const int _D_pltmAsciiCharPitch64 = 64;
	static const int _D_pltmKanjiCharPitch64 = 64;
	static const PltmColor _D_pltmTextFieldColor;
	static const int _D_pltmFontSize = 48;
	static const int _D_HAlignment = ALIGN_LEFT;
	static const int _D_VAlignment = ALIGN_BASELINE;
	/* guard of the class constants */
	static bool bInited;

	TextField(Object *parent, int m_maxLen);
	virtual ~TextField();

	virtual void DrawThis(gm_graph &g);
	void DrawThis_Editable(gm_graph &g);
	void DrawThis_AsLabel(gm_graph &g);

	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const String &value);
	virtual int SetOption(int option, const void *data, unsigned int size);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, String &value) const;
	virtual int GetOption(int option, void *data, unsigned int size) const;

	void *PLTM_ID();
	void CalcRectangle();
	int CalcCursorPos64(int n, bool *sel);
	void CursorJustification();
	int BlockMove(int a, int b, int end);

	virtual int Action(tActionCode code);
	virtual int Action(tActionCode code, unsigned char *arg);

	virtual int SetFieldSize(int w, int h);
	virtual void SetModifyVerifyCallback(void (*cb)(CallData_ModifyVerify *));
	virtual void (*GetModifyVerifyCallback())(CallData_ModifyVerify *);

	void ResetCursorBlink();

	int AC_Begin();
	int AC_End();
	int AC_PrevChar();
	int AC_NextChar();
	int AC_PrevWord();
	int AC_NextWord();
	int AC_DeletePrevChar();
	int AC_DeleteNextChar();
	int AC_DeleteAll();
	int AC_SelectAll();
	int AC_DeSelectAll();
	int AC_InsertString_Utf8(unsigned char *s);
	int AC_InsertString_Ucs2(unsigned short *s);
};

#endif
