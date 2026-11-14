/*
 * SystemParam: global state shared by the Broadband Navigator pages (pad help
 * labels, foreground alpha, sound and menu switches, authentication).
 */

#include <string>
#include "xml/xml_filedescriptor.h"
#include "script/scenewindow.h"
#include "app/systemparam.h"

int SystemParam::alert_illegal_disc;
int SystemParam::padhelp_onoff;
int SystemParam::fg_color_a;
int SystemParam::fgbar_color_a;
int SystemParam::auth_save;
int SystemParam::auth_ok;
int SystemParam::page_max;
int SystemParam::page_no;
int SystemParam::enable_se;
int SystemParam::enable_bgm;
int SystemParam::enable_navimenu;
int SystemParam::network_up;

string SystemParam::padhelp_str_rr;
string SystemParam::padhelp_str_rd;
string SystemParam::padhelp_str_ru;
string SystemParam::padhelp_str_rl;
string SystemParam::padhelp_str_def_rr;
string SystemParam::padhelp_str_def_rd;
string SystemParam::padhelp_str_def_ru;
string SystemParam::padhelp_str_def_rl;
string SystemParam::auth_id;
string SystemParam::auth_pass;

void SystemParam::Init()
{
	alert_illegal_disc = 0;
	padhelp_onoff = 0;
	fg_color_a = 255;
	fgbar_color_a = 100;

	padhelp_str_rr = "";
	padhelp_str_rd = "";
	padhelp_str_ru = "";
	padhelp_str_rl = "";
	padhelp_str_def_rr = "";
	padhelp_str_def_rd = "";
	padhelp_str_def_ru = "";
	padhelp_str_def_rl = "";
	auth_id = "";
	auth_pass = "";

	auth_save = 0;
	auth_ok = 1;
	page_max = 0;
	page_no = 0;
	enable_se = 1;
	enable_bgm = 1;
	enable_navimenu = 1;
	network_up = 0;
}

/* Resets the per-page state: colors and pad help like Init(), foreground
   window fully opaque, and the pad help labels back to their defaults. */
void SystemParam::PageInit()
{
	alert_illegal_disc = 0;
	padhelp_onoff = 0;
	fg_color_a = 255;
	fgbar_color_a = 100;

	if (SceneWindow::fg_window)
		SceneWindow::fg_window->target_alpha = 1.0f;

	enable_se = 1;
	enable_bgm = 1;
	enable_navimenu = 1;
	page_max = 0;
	page_no = 0;

	padhelp_str_rr = padhelp_str_def_rr;
	padhelp_str_rd = padhelp_str_def_rd;
	padhelp_str_ru = padhelp_str_def_ru;
	padhelp_str_rl = padhelp_str_def_rl;
}
