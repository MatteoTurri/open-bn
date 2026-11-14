/*
 * IME status bar: top-level init/shutdown/move driving both the ATOK
 * conversion bar (fepbar.c) and the software keyboard bar (skbbar.c).
 */

#include "textim_debug.h"
#include "textim_glob.h"
#include "bar.h"
#include "fepbar.h"
#include "skbbar.h"

/* Mode bar state, also read by fepbar.c. */
TextIM_ReturnMessage_t msg_modebar;

int bar_init(void)
{
	msg_modebar.type = TEXTIM_RET_MODEBAR;
	msg_modebar.u.value = 0;

	if (!fepbar_init())
		return 0;
	if (skbbar_init())
		return 1;
	fepbar_shutdown();
	return 0;
}

int bar_shutdown(void)
{
	skbbar_shutdown();
	fepbar_shutdown();
	return 1;
}

void bar_move(int x, int y, int flag)
{
	fepbar_move(x, y, flag);
	skbbar_move(x, y, flag);
}
