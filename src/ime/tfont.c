/*
 * IME text/font layer: converts strings to UTF-8 and hands them to the host
 * text callbacks in TextIM_arg.
 */

#include "textim_debug.h"
#include "textim_arg.h"
#include "tfont.h"
#include "unicode.h"

/* Table passed to TextIM_arg.newString2 by tfont_newstr_ucs2_2; only the
 * address is used. */
static const char *g_tfontExtra[32] = { 0 };

/* ASCII 0x20..0x7E -> full-width character (UTF-8), with the JIS forms of
 * ", ', ` (U+2033, U+2019, U+2018); 0x7F (DEL) has no form and stays 0.
 * Unused: left over from a removed hankaku -> zenkaku conversion. */
/* clang-format off */
static const char *g_tfontZenkaku[0x80 - 0x20] = {  /* unused, but part of the layout */
    "　", "！", "″", "＃", "＄", "％", "＆", "’",    /* 0x20 */
    "（", "）", "＊", "＋", "，", "－", "．", "／",    /* 0x28 */
    "０", "１", "２", "３", "４", "５", "６", "７",    /* 0x30 */
    "８", "９", "：", "；", "＜", "＝", "＞", "？",    /* 0x38 */
    "＠", "Ａ", "Ｂ", "Ｃ", "Ｄ", "Ｅ", "Ｆ", "Ｇ",    /* 0x40 */
    "Ｈ", "Ｉ", "Ｊ", "Ｋ", "Ｌ", "Ｍ", "Ｎ", "Ｏ",    /* 0x48 */
    "Ｐ", "Ｑ", "Ｒ", "Ｓ", "Ｔ", "Ｕ", "Ｖ", "Ｗ",    /* 0x50 */
    "Ｘ", "Ｙ", "Ｚ", "［", "＼", "］", "＾", "＿",    /* 0x58 */
    "‘", "ａ", "ｂ", "ｃ", "ｄ", "ｅ", "ｆ", "ｇ",    /* 0x60 */
    "ｈ", "ｉ", "ｊ", "ｋ", "ｌ", "ｍ", "ｎ", "ｏ",    /* 0x68 */
    "ｐ", "ｑ", "ｒ", "ｓ", "ｔ", "ｕ", "ｖ", "ｗ",    /* 0x70 */
    "ｘ", "ｙ", "ｚ", "｛", "｜", "｝", "～",          /* 0x78 */
};
/* clang-format on */

/* Unused 256-character UCS2 buffer, like the local one of tfont_newstr_ucs2*. */
static unsigned short g_tfontUcs2Buf[256] = { 0 };  /* unused, but part of the layout */

/* UCS2 substitutions used by tfont_newstr_ucs2*: characters missing from the
 * glyph set are replaced with their full-width forms (MINUS SIGN, WAVE DASH,
 * CENT SIGN, POUND SIGN). */
typedef struct {
	unsigned short from;
	unsigned short to;
} TFontSubEntry;
static TFontSubEntry g_tfontSubTable[4] = {
	{ 0x2212, 0xff0d },
	{ 0x301c, 0xff5e },
	{ 0x00a2, 0xffe0 },
	{ 0x00a3, 0xffe1 },
};

int tfont_init(void)
{
	return 1;
}

void tfont_shutdown(void)
{}

void tfont_newstr_euc(const char *s, int len, struct PLTM_String *str, int size)
{
	void *handle;

	handle = unicode_euc2utf8(s, len);
	if (handle != 0) {
		TextIM_arg.newString(size, str, handle);
		TextIM_arg.free(handle);
	}
}

void tfont_newstr_ucs2(const unsigned short *s, int nbytes, struct PLTM_String *str, int size)
{
	unsigned short buf[256];
	int i;
	unsigned int j;
	void *handle;

	for (i = 0; i < nbytes; i++) {
		buf[i] = s[i];
		for (j = 0; j < 4; j++) {
			if (s[i] == g_tfontSubTable[j].from) {
				buf[i] = g_tfontSubTable[j].to;
				break;
			}
		}
	}
	handle = unicode_uni2utf8(buf, nbytes);
	if (handle != 0) {
		TextIM_arg.newString(size, str, handle);
		TextIM_arg.free(handle);
	}
}

void tfont_newstr_ucs2_2(const unsigned short *s, int nbytes, struct PLTM_String *str, int size)
{
	unsigned short buf[256];
	int i;
	unsigned int j;
	void *handle;

	for (i = 0; i < nbytes; i++) {
		buf[i] = s[i];
		for (j = 0; j < 4; j++) {
			if (s[i] == g_tfontSubTable[j].from) {
				buf[i] = g_tfontSubTable[j].to;
				break;
			}
		}
	}
	handle = unicode_uni2utf8(buf, nbytes);
	if (handle != 0) {
		TextIM_arg.newString2(size, str, handle, g_tfontExtra, 1, 1);
		TextIM_arg.free(handle);
	}
}

void tfont_deletestr(struct PLTM_String *str)
{
	TextIM_arg.destroyString(str);
}

void tfont_drawstr(int x, int y, struct PLTM_String *str, void *attr)
{
	TextIM_arg.drawString(0, (long long)(x << 6), (long long)(y << 6), -1, 0, str, attr);
}

int tfont_gettextrect(struct PLTM_String *str)
{
	return TextIM_arg.getTextRect(0, 0, str);
}
