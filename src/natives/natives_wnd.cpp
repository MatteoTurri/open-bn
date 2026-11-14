/*
 * Small/amx natives "wnd*": name, scene and screen size of a scene window.
 * The window index of a script is 0 = the window of the running script,
 * 1 = SceneWindow::main_window, 2 = SceneWindow::bg_window.
 */

#include "amx.h"
#include "natives/smallext.h"
#include "script/scenewindow.h"
#include "script/script.h"

#include <string>
#include "xml/xml_filedescriptor.h"

#include "scene/object.h"
#include "scene/scene.h"

struct __amx;

static const unsigned int TAG_HNDL = 0x4c444e48;   /* 'HNDL': user data tag holding the script handle of an object */

/* Resolves a script window index (0..2); 0 for an unknown index. */
static SceneWindow *GetWindow(AMX *amx, cell id)
{
	switch (id) {
	case 0:
		return GetSceneWindow(amx);
	case 1:
		return SceneWindow::main_window;
	case 2:
		return SceneWindow::bg_window;
	}
	return 0;
}

/* True when [p,q] lies entirely in the script heap or entirely in its stack. */
static inline int amx_addr_ok(AMX *amx, cell p, cell q)
{
	if (p >= 0 && p < amx->hea && q >= 0 && q < amx->hea)
		return 1;
	if (p >= amx->stk && p < amx->stp && q >= amx->stk && q < amx->stp)
		return 1;
	return 0;
}

/*
 * wndGetName(win, buf, size): copies the window name into the script buffer,
 * truncated to size cells (size*4-1 characters).
 */
static cell AMX_NATIVE_CALL n_wndGetName(AMX *amx, cell *params)
{
	SceneWindow *win;
	cell *cptr;

	if ((unsigned int)params[1] >= 3) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	win = GetWindow(amx, params[1]);
	if (win == 0)
		return 0;
	string v = win->name;

	if ((unsigned int)(params[3] * 4) < v.length() + 1)
		v = v.substr(0, params[3] * 4 - 1);
	if (!amx_addr_ok(amx, params[2], v.length() + params[2])) {
		amx_RaiseError(amx, 5);  /* AMX_ERR_MEMACCESS */
		return 0;
	}
	amx_GetAddr(amx, params[2], &cptr);
	amx_SetString(cptr, (char *)v.c_str(), 1);
	return 1;
}

static cell AMX_NATIVE_CALL n_wndGetScene(AMX *amx, cell *params)
{
	SceneWindow *win;
	Hook *scene;

	if ((unsigned int)params[1] >= 3) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	win = GetWindow(amx, params[1]);
	if (win == 0 || (scene = win->scene) == 0)
		return 0;
	return (cell)scene->GetUserData(TAG_HNDL);
}

static cell AMX_NATIVE_CALL n_wndGetScreen(AMX *amx, cell *params)
{
	SceneWindow *win;
	cell *cptr;
	int w, h, d;

	if ((unsigned int)params[1] >= 3) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	win = GetWindow(amx, params[1]);
	if (win == 0)
		return 0;
	win->GetScreen(w, h, d);
	if (amx_GetAddr(amx, params[2], &cptr) == AMX_ERR_NONE)
		*cptr = w;
	if (amx_GetAddr(amx, params[3], &cptr) == AMX_ERR_NONE)
		*cptr = h;
	if (amx_GetAddr(amx, params[4], &cptr) == AMX_ERR_NONE)
		*cptr = d;
	return 1;
}

AMX_NATIVE_INFO smallext_wnd[] = {
	{ "wndGetName", n_wndGetName },
	{ "wndGetScene", n_wndGetScene },
	{ "wndGetScreen", n_wndGetScreen },
	{ 0, 0 },
};
