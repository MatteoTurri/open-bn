/*
 * Labels of the soft-key buttons.
 */

#include "gettext.h"

const char *GetText(int n)
{
#ifdef DEFINITIVE_PATCH
	/* PSBBN Definitive Patch: English labels padded to the 6 bytes of the
	   originals, so that the layout of the binary does not change. */
	static const char *const s_text[4] = { "Delete", "Back  ", "Enter ", "変換" };
#else
	/* "Delete", "Back", "Enter", "Convert" */
	static const char *const s_text[4] = { "削除", "戻る", "決定", "変換" };
#endif
	if ((unsigned)n >= 4)
		return "?????";
	return s_text[n];
}
