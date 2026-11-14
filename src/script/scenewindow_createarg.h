/*
 * SceneWindow::CreateArg: description of a window to create, filled by the XML
 * loader and copied by the MsgBox/NaviMenu callbacks.
 */

#ifndef SCENEWINDOW_CREATEARG_H
#define SCENEWINDOW_CREATEARG_H
#include "scenewindow.h"

/* The by-value constructor is expanded at the call site in the XML loader;
   the destructor is implicit. */
class SceneWindow::CreateArg {
public:
	CreateArg(int a_mode, int a_psml_version, int a_screen_w, int a_screen_h, int a_screen_d, int a_fade_time,
	    String a_uri, String a_dir, String a_name, String a_script, String a_key, Document a_doc,
	    std::map<String, FileImagePtr> a_file_image_cache, std::map<String, String> a_file_uri_cache)
	        : mode(a_mode), psml_version(a_psml_version), screen_w(a_screen_w), screen_h(a_screen_h),
	          screen_d(a_screen_d), fade_time(a_fade_time), uri(a_uri), dir(a_dir), name(a_name), script(a_script),
	          key(a_key), doc(a_doc), file_image_cache(a_file_image_cache), file_uri_cache(a_file_uri_cache)
	{}
	CreateArg(const CreateArg &o);
	/* 0x00 */ int mode;
	/* 0x04 */ int psml_version;
	/* 0x08 */ int screen_w;
	/* 0x0C */ int screen_h;
	/* 0x10 */ int screen_d;
	/* 0x14 */ int fade_time;  /* fade-out time of the window being replaced */
	/* 0x18 */ String uri;
	/* 0x1C */ String dir;
	/* 0x20 */ String name;
	/* 0x24 */ String script;  /* script source */
	/* 0x28 */ String key;
	/* 0x2C */ Document doc;
	/* 0x34 */ std::map<String, FileImagePtr> file_image_cache;
	/* 0x40 */ std::map<String, String> file_uri_cache;
};  /* size = 0x4C */

#endif
