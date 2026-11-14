/*
 * SystemParam: global state shared by the Broadband Navigator pages (pad help
 * labels, foreground alpha, sound and menu switches, authentication).
 */

#ifndef SYSTEMPARAM_H
#define SYSTEMPARAM_H

#include <string>

class SystemParam {
public:
	static void Init();
	static void PageInit();

	static int alert_illegal_disc;
	static int padhelp_onoff;
	static int fg_color_a;
	static int fgbar_color_a;
	static int auth_save;
	static int auth_ok;
	static int page_max;
	static int page_no;
	static int enable_se;
	static int enable_bgm;
	static int enable_navimenu;  /* 0: NaviMenu::Open() fails */
	static int network_up;

	/* labels of the pad help buttons, and their defaults */
	static string padhelp_str_rr;
	static string padhelp_str_rd;
	static string padhelp_str_ru;
	static string padhelp_str_rl;
	static string padhelp_str_def_rr;
	static string padhelp_str_def_rd;
	static string padhelp_str_def_ru;
	static string padhelp_str_def_rl;
	static string auth_id;
	static string auth_pass;
};

#endif
