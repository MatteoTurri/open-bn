/*
 * BNAPI: services the scripts call into: frame and cache control, pad input,
 * scene windows, per-script user data and system parameters.
 */

#include <string>
#include "xml/xml_filedescriptor.h"
#include "thread/thposixthread.h"
#include "systemoptions.h"
#include "script/scenewindow.h"
#include "script/script.h"
#include "fw/fw_gsdev.h"
#include "fw/fw_pk.h"
#include "gfx/font.h"
#include "gfx/texture.h"
#include "input/mypad.h"
#include "input/irrm.h"
#include "app/jobinit2.h"
#include "ime/conkbd.h"
#include "amx.h"
#include "bnapi.h"
#include "fs/thmediachecker.h"
#include "app/systemparam.h"

class __amx;

static const long TAG_RCSP = 0x52435350; /* 'RCSP' */

/* The Script that owns `amx` is registered on it as user data 'RCSP'. */
static inline Script *AmxScript(__amx *amx)
{
	Script *s;

	amx_GetUserData(amx, TAG_RCSP, (void **)&s);
	return s;
}

void BNAPI::BeginFullScreen()
{}

void BNAPI::EndFullScreen()
{}

void BNAPI::BeginFrame()
{
	fwPkSyncV();
}

void BNAPI::EndFrame()
{
	fwPkWaitDma();
}

void BNAPI::DestroyWindow(int windowId)
{
	switch (windowId) {
	case 0:
		SceneWindow::RemoveMain();
		break;
	case 1:
		SceneWindow::RemoveBG();
		break;
	case 2:
		SceneWindow::RemoveModal();
		break;
	}
}

void BNAPI::SetWindowCallback(__amx *amx, int type, int key, void (*fn)())
{
	SceneWindow *win = GetSceneWindow(amx);
	win->SetCallback((SceneWindow::CallbackType)type, key, fn);
}

void BNAPI::DelWindowCallback(__amx *amx, int type, int key)
{
	SceneWindow *win = GetSceneWindow(amx);
	win->DelCallback((SceneWindow::CallbackType)type, key);
}

string BNAPI::CellToString(__amx *amx, int cell)
{
	return ::CellToString(amx, cell);
}

/* Buttons held on either pad. */
int BNAPI::GetPadState()
{
	unsigned short state = 0;
	if (pPad0 != 0)
		state = pPad0->m_buttons[0].w;
	if (pPad1 != 0)
		state = pPad1->m_buttons[0].w | state;
	return state;
}

/* Polls the pads; pad 0 also receives the codes of the IR remote and of
   the USB keyboard. */
void BNAPI::PadRead()
{
	if (pPad0 != 0) {
		if (pRM0 != 0)
			pRM0->GetSignal();
		myUsbKeyboard_Run();
		int code = 0;
		if (pRM0 != 0)
			code = pRM0->GetPadCode();
		int arg = code;
		arg |= myUsbKeyboard_LastPseudoPadData;
		pPad0->GetData(arg);
	}
	if (pPad1 != 0)
		pPad1->GetData(0);
}

void BNAPI::FlushFontCache()
{
	Font::ExpireAll();
}

void BNAPI::FlushTextureGSCache()
{
	Texture::FlushGSCache();
	Font::FlushGSCache();
}

void *BNAPI::SCRIPT_GET_USERDATA(__amx *amx, unsigned int id)
{
	return AmxScript(amx)->GetUserData(id);
}

void *BNAPI::SCRIPT_SET_USERDATA(__amx *amx, unsigned int id, void *data)
{
	return AmxScript(amx)->SetUserData(id, data);
}

void *BNAPI::SCRIPT_DEL_USERDATA(__amx *amx, unsigned int id)
{
	return AmxScript(amx)->DelUserData(id);
}

void BNAPI::SCRIPT_SET_DESTROY_CALLBACK(__amx *amx, unsigned int id, void (*fn)(__amx *))
{
	AmxScript(amx)->SetCallback(Script::CALLBACK_ON_DESTROY, id, fn);
}

/* Integer system parameters: 300 = CD/DVD media type, 301 = network up,
   700..719 = the user settings held in SysOption. */
int BNAPI::GetSystemParam(int param)
{
	switch (param) {
	case 300:
		return thMediaChecker::getCDVDType();
	case 301:
		return SystemParam::network_up;
	case 700:
		return SysOption.m_language;
	case 701:
		return SysOption.m_keyboardType;
	case 702:
		return SysOption.m_keyboardRepeatWait;
	case 703:
		return SysOption.m_keyboardRepeatSpeed;
	case 704:
		return SysOption.m_mouseSpeed;
	case 705:
		return SysOption.m_mouseDClick;
	case 706:
		return SysOption.m_mouseChangeLR;
	case 707:
		return SysOption.m_atokMode;
	case 708:
		return SysOption.m_atokKeyBind;
	case 709:
		return SysOption.m_softkbOnOff;
	case 710:
		return SysOption.m_softkbQwerty;
	case 711:
		return SysOption.m_spdif;
	case 712:
		return SysOption.m_aspect;
	case 713:
		return SysOption.m_video;
	case 714:
		return SysOption.m_timeZone;
	case 715:
		return SysOption.m_timeZoneCity;
	case 716:
		return SysOption.m_summerTime;
	case 717:
		return SysOption.m_timeNotation;
	case 718:
		return SysOption.m_dateNotation;
	case 719:
		return SysOption.m_autoDiag;
	default:
		return 0;
	}
}

/* 900 = version string; any other selector gives an empty string. */
string BNAPI::GetSystemParamStr(int param)
{
	if (param == 900) {
		return "0.31";
	} else {
		return "";
	}
}
