/*
 * Labels of the soft-key buttons.
 */

#include "gettext.h"

const char *GetText(int n)
{
	/* "Delete", "Back", "Enter", "Convert" */
	static const char *const s_text[4] = { "削除", "戻る", "決定", "変換" };
	if ((unsigned)n >= 4)
		return "?????";
	return s_text[n];
}
