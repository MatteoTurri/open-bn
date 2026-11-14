/*
 * SimpleTextEdit: multi-line text editor widget.
 */

#ifndef SIMPLETEXTEDIT_H
#define SIMPLETEXTEDIT_H

#include "gfx/vec_types.h"
#include "gfx/freetype2.h"
#include "gfx/gm_vertex.h"
#include <vector>
#include "scene/object.h"

class Object;
class gm_graph;

/* String is only declared here; simpletextedit.cpp includes <string>. */

/* One cell of a vvram row: what is already drawn there, compared with the
   character to draw so that unchanged glyphs are skipped. */
struct SteVvramCell {
	/* 0x00 */ int x;  /* x in pixels */
	/* 0x04 */ int w;  /* advance */
	/* 0x08 */ int selected;
	/* 0x0C */ unsigned short ch;
	unsigned short pad_0E;
};

/* One character of a simpleline. */
struct simpleline_chunk {
	/* 0x00 */ int len;  /* character width in pixels (<= 0: not known yet) */
	/* 0x04 */ int selected;  /* character is selected */
	/* 0x08 */ unsigned short ch;  /* UCS-2 character */
	/* 0x0A */ unsigned short pad_0A;
};

/* Font path buffer; the int view keeps it word-aligned. */
union FontPathBuf {
	char c[4096];
	int w[1024];
};

union FontPathBufConst {
	char c[28];
	int w[7];
};

/* Editors in the registry of the draw thread. */
#define STE_MAX_EDITORS 16
/* Display rows of the vvram (m_vvramSlots). */
#define STE_VVRAM_ROWS 64
/* Entries of the deferred action queue (m_ring). */
#define STE_RING_SIZE 128

/* SimpleTextEdit_RingEntry::type: the Action overload that queued the entry. */
#define STE_RING_ACTION     1  /* Action(code) */
#define STE_RING_ACTION_STR 2  /* Action(code, unsigned char *), with a heap copy of the string in strCopy */
#define STE_RING_ACTION_INT 3  /* Action(code, int), with the int in value */

/* CallData_ModifyVerify::option: the edit about to be made. */
#define STE_VERIFY_NEW_LINE 0  /* AC_NewLine; data is 0 */
#define STE_VERIFY_PASTE    5  /* one character of AC_PasteRegion; data is the UCS-2 character */
#define STE_VERIFY_UCS2     6  /* AC_InsertString_Ucs2; data is the UCS-2 string */
#define STE_VERIFY_UTF8     7  /* AC_InsertString_Utf8; data is the UTF-8 string */

/* Entry of the deferred action queue (Action -> Action_exec); strCopy is freed once the entry has run. */
struct SimpleTextEdit_RingEntry {
	int type;  /* STE_RING_* */
	int code;
	int value;
	int strCopy;
};

/*
 * Multi-line text editor widget: a list of logical lines (simpleline) wrapped
 * into display rows, rendered with FreeType into a GS texture by a background
 * thread, with a cursor, a selection, a clipboard and a deferred action queue.
 */
class SimpleTextEdit : public Object {
public:
	struct simpleline; /* defined after the class */

	/* Its constructor runs the one-time module setup. */
	class ClassInitializer {
	public:
		ClassInitializer();
	};

	/* Action codes. AC_SCROLL_LINE is handled only by Action(code, int),
	   AC_INSERT_STRING_UTF8 and AC_INSERT_STRING_UCS2_DIV only by
	   Action(code, unsigned char *). */
	enum tActionCode {
		AC_PREV_CHAR = 0,
		AC_NEXT_CHAR,
		AC_PREV_WORD,
		AC_NEXT_WORD,
		AC_PREV_LINE,
		AC_NEXT_LINE,
		AC_NEW_LINE,
		AC_SCROLL_LINE,
		AC_PREV_PAGE,
		AC_NEXT_PAGE,
		AC_BEGIN_LINE,
		AC_END_LINE,
		AC_BEGIN_TEXT,
		AC_END_TEXT,
		AC_INSERT,
		AC_DELETE_PREV_CHAR,
		AC_DELETE_NEXT_CHAR,
		AC_DELETE_ALL,
		AC_SELECT_MARK,
		AC_SELECT_REGION,
		AC_SELECT_WORD,
		AC_SELECT_LINE,
		AC_SELECT_ALL,
		AC_DESELECT_ALL,
		AC_COPY_REGION,
		AC_CUT_REGION,
		AC_PASTE_REGION,
		AC_INSERT_STRING_UTF8,
		AC_INSERT_STRING_UCS2_DIV
	};
	/* Passed to the modify-verify callback, which may clear `allow` to reject the
	   change. */
	struct CallData_ModifyVerify {
		/* 0x00 */ SimpleTextEdit *editor;
		/* 0x04 */ int option;  /* STE_VERIFY_* */
		/* 0x08 */ int allow;  /* 1; the callback clears it to reject */
		/* 0x0C */ void *data;
		/* 0x10 */ int textLength;  /* characters of the text plus one per line */
		/* 0x14 */ int textBytes;  /* size checked against a byte limit; counted as textLength */
	};

	typedef void (*ModifyVerifyCallback)(CallData_ModifyVerify *);
	typedef int (*WideModeCheckFunction)();

	/* 0x250 */ ModifyVerifyCallback m_modifyVerifyCallback;
	/* 0x254 */ WideModeCheckFunction m_wideModeCheckFunction;
	/* 0x258 */ int m_wideMode;       /* wide-mode state last applied by DrawThis */
	/* 0x25C */ int m_wideModeDelay;  /* countdown before DrawThis applies a new wide-mode state */
	/* 0x260 */ int m_fieldSizeFlag;  /* 1 after SetFieldSize2 */
	/* 0x264 */ int m_mode;  /* 0 or 1 (constructor argument) */
	/* 0x268 */ int pad_268[2];
	/* 0x270 */ int m_defWidth;  /* default buffer width */
	/* 0x274 */ int m_defHeight;  /* default buffer height */
	/* 0x278 */ int pad_278[2];
	/* 0x280 */ int m_fieldWidth;  /* width used while the wide-mode check returns 0, at most 640 */
	/* 0x284 */ int m_fieldHeight;  /* same, height, at most 480 */
	/* Three parallel 4-entry vertex tables, copied by the constructor from
	   kTextQuad..kCursorQuad: text quad (with uv), field quad, cursor quad. */
	/* 0x288 */ gmVertexPCT m_textQuad[4];
	/* 0x2E8 */ gmVertexPC m_fieldQuad[4];
	/* 0x328 */ gmVertexPC m_cursorQuad[4];
	/* 0x368 */ int m_fontELoaded;  /* 1 once the E font face is loaded */
	/* 0x36C */ FT_Face m_fontEFace;  /* FreeType face for E */
	/* 0x370 */ FontPathBuf m_fontEPath;  /* E font path, up to 4096 bytes with the terminator */
	/* 0x1370 */ int m_fontJLoaded;  /* 1 once the J font face is loaded */
	/* 0x1374 */ FT_Face m_fontJFace;  /* FreeType face for J */
	/* 0x1378 */ FontPathBuf m_fontJPath;  /* J font path */
	/* 0x2378 */ simpleline *m_textHead;  /* first logical line */
	/* 0x237C */ simpleline *m_textTail;  /* last logical line */
	/* 0x2380 */ simpleline *m_firstViewLine;  /* first visible line */
	/* 0x2384 */ simpleline *m_curLine;  /* line holding the cursor */
	/* 0x2388 */ int m_selRangeStart;  /* index of the first line (0) */
	/* 0x238C */ int m_selRangeEnd;  /* index of the last line */
	/* 0x2390 */ int m_firstViewLineNo;  /* index of the first visible line */
	/* 0x2394 */ int m_curLineNo;  /* index of the cursor line */
	/* 0x2398 */ int m_endLineNo;  /* index of the last visible line (calc_drawend_pos) */
	/* Draw thread state (SteFontThread), which renders the text line by line
	   off the main thread. */
	/* 0x239C */ simpleline *m_drawLine;  /* line being drawn (0: done) */
	/* 0x23A0 */ int m_drawX;  /* pixel x of the next character */
	/* 0x23A4 */ int m_drawY;  /* pixel y of the display row */
	/* 0x23A8 */ int m_drawCol;  /* column in the vvram row */
	/* 0x23AC */ int m_drawRow;  /* vvram row */
	/* 0x23B0 */ int m_drawChunk;  /* character in the logical line */
	/* 0x23B4 */ int m_drawLineNo;  /* logical line */
	/* 0x23B8 */ int m_drawRowCount;  /* display rows of the logical line */
	/* 0x23BC */ int m_curPixelPos;  /* cursor pixel x within its display row (calc_curpos) */
	/* 0x23C0 */ int m_curCharNo;  /* cursor character index in its line */
	/* 0x23C4 */ int m_firstViewCharNo;  /* first visible character in the first visible line */
	/* 0x23C8 */ int m_endCharNo;  /* last visible character (calc_drawend_pos) */
	/* 0x23CC */ int m_texture;  /* GS texture handle (<= 0: none) */
	/* 0x23D0 */ int m_texWidth;  /* m_dispWidth when the texture was created */
	/* 0x23D4 */ int m_texHeight;  /* same, m_dispHeight */
	/* 0x23D8 */ int m_clut;  /* CLUT handle (-1: creation failed) */
	/* 0x23DC */ int m_dispWidth;  /* display width in pixels, the wrap limit (124 by default) */
	/* 0x23E0 */ int m_dispHeight;  /* display height (124 by default) */
	/* 0x23E4 */ int m_caretPixelX;  /* caret x, option 'CX64' (returned in 1/64) */
	/* 0x23E8 */ int m_caretPixelY;  /* caret y, option 'CY64' */
	/* 0x23EC */ int m_vvramWidthCache;  /* m_dispWidth as seen by vvram_update */
	/* 0x23F0 */ int m_vvramCount;  /* number of vvram rows allocated */
	/* 0x23F4 */ unsigned char *m_buffer;  /* image buffer, m_dispWidth * m_dispHeight bytes */
	/* 0x23F8 */ int m_needRedraw;  /* redraw needed */
	/* 0x23FC */ int m_clearOnRedraw;  /* clear m_buffer at the next redraw */
	/* 0x2400 */ SteVvramCell *m_vvramSlots[STE_VVRAM_ROWS];  /* vvram rows, one per display row */
	/* 0x2500 */ int m_fontPointSize;  /* font size in points (16 by default) */
	/* 0x2504 */ int m_shadowOffset;  /* option 'SDWE', 0..64; shadow offset in pixels */
	/* 0x2508 */ int m_wrapPad;  /* character spacing, option 'CSPC' */
	/* 0x250C */ int m_padTop;  /* line spacing above, option 'LSPU' */
	/* 0x2510 */ int m_padBot;  /* line spacing below, option 'LSPL'; 'LSPC' is the sum */
	/* 0x2514 */ char m_tag[4];  /* four bytes copied from kTag */
	/* 0x2518 */ int m_registryIndex;  /* slot in the registry of the STE_MAX_EDITORS editors (-1: none) */
	/* 0x251C */ int m_ctorFailed;  /* construction failed (no memory, or registry full) */
	/* 0x2520 */ int m_ringHead;  /* read index (drained by Action_exec) */
	/* 0x2524 */ int m_ringTail;  /* write index */
	/* 0x2528 */ SimpleTextEdit_RingEntry
	    m_ring[STE_RING_SIZE];  /* deferred action queue; full when (tail + 1) % STE_RING_SIZE == head */
	/* 0x2D28 */ int m_gssy;  /* rows scrolled by AC_ScrollLine, option 'GSSY' (read only) */
	/* 0x2D2C */ simpleline *m_selFromLine;  /* first line of the selection */
	/* 0x2D30 */ simpleline *m_selToLine;  /* last line of the selection */
	/* 0x2D34 */ simpleline *m_markLine;  /* line of the mark (AC_SelectMark) */
	/* 0x2D38 */ int m_selFromRow;  /* line number of the first selected character */
	/* 0x2D3C */ int m_selToRow;  /* line number of the last selected character */
	/* 0x2D40 */ int m_markRow;  /* line number of the mark */
	/* 0x2D44 */ int m_selFromCol;  /* character index of the first selected character */
	/* 0x2D48 */ int m_selToCol;  /* character index of the last selected character */
	/* 0x2D4C */ int m_markCol;  /* character index of the mark */
	/* Clipboard: AC_CopyRegion stores the selected characters as UCS-2, with a
	   '\n' after each fully selected line. */
	/* 0x2D50 */ int m_clipLen;  /* number of UCS-2 code units */
	/* 0x2D54 */ unsigned short *m_clipBuf;  /* UCS-2 buffer */
	/* 0x2D58 */ int m_bufWidth;  /* buffer width in pixels, m_dispWidth + 4 (128 by default) */
	/* 0x2D5C */ int m_bufHeight;  /* same, height */
	/* 0x2D60 */ int m_cursorBlinkCounter;  /* blink half-period */
	/* 0x2D64 */ int m_cursorBlinkTarget;  /* blink counter */
	/* 0x2D68 */ int m_cursorBlinkFlag;  /* blink phase */
	/* 0x2D6C */ int m_cursorSwitch;  /* cursor shown, option 'CRSW' (stored as a boolean) */
	/* 0x2D70 */ int m_insertMode;  /* editing allowed; the AC_* handlers return 0 otherwise */
	/* 0x2D74 */ int m_insertToggle;  /* insert (1) or overwrite (0), toggled by AC_Insert */
	/* 0x2D78 */ int m_verifyInProgress;  /* set once the modify-verify callback has run */

	SimpleTextEdit(Object *parent, int mode);
	virtual ~SimpleTextEdit();

	virtual void DrawThis(gm_graph &g);

	virtual int SetFieldSize(int w, int h);
	virtual int SetFieldSize2(int w, int h);
	virtual void SetModifyVerifyCallback(ModifyVerifyCallback cb);
	virtual ModifyVerifyCallback GetModifyVerifyCallback();
	virtual void SetWideModeCheckFunction(WideModeCheckFunction fn);
	virtual WideModeCheckFunction GetWideModeCheckFunction();
	virtual int Action(tActionCode code);
	virtual int Action(tActionCode code, int value);
	virtual int Action(tActionCode code, unsigned char *s);
	virtual int SetFontPathE(char *path);
	virtual int SetFontPathJ(char *path);
	virtual int CalcNumLine(int n) const;

	int vvram_attrupdate();
	void reset_cursorblink();
	int calc_text_lines() const;
	int calc_text_bytes() const;
	int ucs2_characterization(unsigned short c);
	void free_text(simpleline *line);
	int SetOption(int option, const int &value, unsigned int size);
	int GetOption(int option, int &value, unsigned int size) const;
	int SetOption(int option, const float &value, unsigned int size);
	int GetOption(int option, float &value, unsigned int size) const;
	int SetOption(int option, const String &value);
	int GetOption(int option, String &value) const;
	int SetOption(int option, const void *data, unsigned int size);
	int GetOption(int option, void *data, unsigned int size) const;
	void rewrite_screen();
	int Action_i(tActionCode code);
	int Action_i(tActionCode code, int value);
	int Action_i(tActionCode code, unsigned char *s);
	int Action_exec();

	/* AC_* handlers, called by Action_i. */
	int AC_PrevChar();
	int AC_NextChar();
	int AC_PrevWord();
	int AC_NextWord();
	int AC_PrevLine();
	int AC_NextLine();
	bool AC_NewLine(bool a = true, bool b = false);
	int AC_ScrollLine(int n);
	int AC_PrevPage();
	int AC_NextPage();
	int AC_BeginLine();
	int AC_EndLine();
	int AC_BeginText();
	int AC_EndText();
	int AC_Insert();
	int AC_DeletePrevChar();
	int AC_DeleteNextChar(bool b);
	int AC_DeleteAll(bool b);
	int AC_SelectMark();
	int AC_SelectRegion();
	int AC_SelectWord();
	int AC_SelectLine();
	int AC_SelectAll();
	int AC_DeSelectAll(int fullReset);
	int AC_CopyRegion();
	int AC_CutRegion();
	int AC_PasteRegion();
	int AC_InsertString_Utf8(unsigned char *s, bool b);
	int AC_InsertString_Ucs2_div(unsigned short *s, bool b);
	int AC_InsertString_Ucs2(unsigned short *s, bool a, bool b);

	int vvram_update();
	int image_update();
	int fontface_update();
	simpleline *alloc_text(int count);
	void replace_text(simpleline *oldLine, simpleline *newLine);
	int check_modify_verify_callback(int option, void *data);

	int up_line(int step, simpleline **pLine, int *pRow, int *pSub, int pix);
	int down_line(int step, simpleline **pLine, int *pRow, int *pSub, int pix);
	int calc_drawend_pos();
	int vvram_upscroll(int n);
	int vvram_downscroll(int n);
	int update_bg_displines();
	int DrawThis_update(gm_graph &g);
	void check_cursorpos(bool doScroll);

	int calc_curpos(simpleline *line, int col);
	int update_complete_oneline(simpleline *line) const;
	int calc_text_displines() const;
};  /* size = 0x2D7C */

/* A logical line of text; chunks[] holds one entry per character. */
struct SimpleTextEdit::simpleline {
	/* 0x00 */ int total_chars;  /* characters in use */
	/* 0x04 */ int alloc_chars;  /* allocated characters */
	/* 0x08 */ int disp_lines;  /* display rows (cached; <= 0: recompute) */
	/* 0x0C */ int pad_0C;
	/* 0x10 */ int line_selected;  /* line is selected */
	/* 0x14 */ simpleline *next;
	/* 0x18 */ simpleline *prev;
	/* 0x1C */ simpleline_chunk chunks[64];  /* alloc_chars + 1 entries */
};

#endif
