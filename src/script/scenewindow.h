/*
 * SceneWindow: window showing a PSML document (3D scene, camera, 2D display,
 * node list). The main, foreground and background windows and the modal and
 * fade-out lists are class statics.
 */

#ifndef SCENEWINDOW_H
#define SCENEWINDOW_H
#pragma interface

#include <string>
#include "app/bnstring.h"
#include <vector>
#include <map>
#include <set>
#include "xml/xml_document.h"
#include "xml/xmltagtable.h"

class Scene;
class Camera;
class Disp2d;
class NodeList;
class Object;

/* Image buffers shared by SceneWindow and its image cache. */
class FileImage {
public:
	FileImage(void *buf);
	~FileImage();
	static int instance_count;
	void *data;
	int m_ref;
};

class FileImagePtr {
public:
	FileImagePtr();
	FileImagePtr(void *buf);
	FileImagePtr(const FileImagePtr &o);
	~FileImagePtr();
	void operator=(void *buf);
	void operator=(const FileImagePtr &o);
	operator void *() const;

private:
	FileImage *m_p;
};

class SceneWindow {
public:
	struct CallbackData {
		unsigned int key;
		void (*fn)(void);
	};

	enum CallbackType {
		CALLBACK_EVERY_FRAME = 0,
		CALLBACK_ON_DESTROY
	};

	/* mode: the slot the window takes. */
	enum WindowMode {
		MODE_MAIN,
		MODE_FG,
		MODE_BG,
		MODE_MODAL /* pushed on modal_window_list */
	};

	/* state */
	enum WindowState {
		STATE_NONE,
		STATE_READY,    /* the scene, camera and script are set up */
		STATE_RELEASED, /* the scene was released */
		STATE_FAILED    /* the setup failed */
	};

	/* Messages broadcast to the scripts of the windows. */
	enum Message {
		MSG_NAVIMENU_OPENED = -100, /* NaviMenu::Open */
		MSG_MAIN_CREATED = -200,
		MSG_BG_CREATED = -201,
		MSG_MAIN_FADEOUT = -300,
		MSG_BG_FADEOUT = -301
	};


	/* 0x00 */ int mode; /* MODE_* */
	/* 0x04 */ int psml_version;
	/* 0x08 */ String name;
	/* 0x0C */ String uri;
	/* 0x10 */ String dir;
	/* 0x14 */ String key;
	/* 0x18 */ int state; /* STATE_* */
	/* 0x1C */ int screen_w;
	/* 0x20 */ int screen_h;
	/* 0x24 */ int screen_d;
	/* 0x28 */ int rect_x;
	/* 0x2C */ int rect_y;
	/* 0x30 */ int rect_w;
	/* 0x34 */ int rect_h;
	/* 0x38 */ int shown;  /* Show() = 1, Hide() = 0 */
	/* 0x3C */ float alpha;  /* current alpha, moved toward target_alpha by Fade() */
	/* 0x40 */ float target_alpha;
	/* 0x44 */ int fadeout_time;
	/* 0x48 */ int hide_flag;  /* set by SetHide, distinct from shown */
	/* 0x4C */ Scene *scene;
	/* 0x50 */ Camera *camera;
	/* 0x54 */ Disp2d *disp2d;
	/* 0x58 */ int start_delay;  /* frames before the page starts; the script is held until then */
	/* Pad help and FG settings saved by a modal window, restored when it closes. */
	/* 0x5C */ int saved_padhelp_onoff;
	/* 0x60 */ String saved_padhelp_str_rr;
	/* 0x64 */ String saved_padhelp_str_rd;
	/* 0x68 */ String saved_padhelp_str_ru;
	/* 0x6C */ String saved_padhelp_str_rl;
	/* 0x70 */ int saved_fg_color_a;
	/* 0x74 */ int saved_fgbar_color_a;
	/* Embedded objects: the accessors return their address. */
	/* 0x78 */ Document document;
	/* 0x80 */ XmlTagTable node_list;

	/* 0x84 */ std::map<String, FileImagePtr> file_image_cache;
	/* 0x90 */ std::map<String, String> file_uri_cache;
	/* 0x9C */ std::vector<String> image_list_order;  /* in order of use, capped at 256 by UsingImageList_Add */
	/* 0xA8 */ std::set<String> image_list_check;
	/* 0xB4 */ std::map<unsigned int, void *> user_data;
	/* 0xC0 */ std::vector<CallbackData> callbacks[2];  /* one list per CallbackType */

	static void RemoveMain();
	static void RemoveBG();
	static void RemoveFG();
	static void RemoveModal();
	static void RemoveFadeoutWindowList();
	static void ForgetMainWindowFGStatus();

	static SceneWindow *MainWindow();
	static SceneWindow *FGWindow();
	static SceneWindow *BGWindow();
	static SceneWindow *ModalWindow();
	static int InstanceCount();

	static void Init();
	static void Exit();

	void Show();
	void Hide();
	void SetHide(int hide);

	int Mode() const;
	int PSMLVersion() const;
	const String &GetName() const;
	const String &GetURI() const;
	const String &GetDir() const;
	const String &GetKey() const;
	bool IsFromNetwork() const;
	int State() const;

	void GetRect(int &x, int &y, int &w, int &h) const;
	void SetRect(int x, int y, int w, int h);
	void GetScreen(int &w, int &h, int &d) const;

	Scene *GetScene() const;
	Camera *GetCamera() const;
	Disp2d *GetDisp2d() const;
	void *GetDocument();
	void *GetNodeList();

	void SetTargetAlpha(float a);
	void SetFadeoutTime(int t);
	bool Fade();

	void cbReleaseScene();
	static void cbObjReleaseScene(Object *obj, void *userData, void *arg2);

	void AbortAttachedScript();

	void *SetUserData(unsigned int key, void *data);
	void *GetUserData(unsigned int key);
	void *DelUserData(unsigned int key);

	void SetCallback(CallbackType type, unsigned int key, void (*fn)(void));
	void DelCallback(CallbackType type, unsigned int key);
	void DoCallback(CallbackType type);

	void *GetFileImage(const String &key) const;
	String GetFileURI(const String &key) const;
	bool UsingImageList_Check(const String &key) const;
	void UsingImageList_Add(const String &key);
	void MakeFullPath(String &path);

	void Anim();
	static void AnimAll();
	void Draw();
	static void DrawAll();

	static void StartFadeoutMainWindow(int time);
	static void StartFadeoutBGWindow(int time);

	static void BroadcastMessage(int msg, int a, int b, int c, int d);
	static void BroadcastMessage(int msg, const String &s);

	/* Window description passed to the constructor (scenewindow_createarg.h). */
	class CreateArg;

	SceneWindow(const CreateArg &arg);

	virtual ~SceneWindow();  /* vptr at 0xD8 */

	static SceneWindow *main_window;
	static SceneWindow *fg_window;
	static SceneWindow *bg_window;
	static int instance_count;
	static std::vector<SceneWindow *> modal_window_list;
	static std::vector<SceneWindow *> fadeout_window_list;
};  /* size = 0xDC */

SceneWindow *CreateSceneWindow(const SceneWindow::CreateArg &arg);

#include "scenewindow_createarg.h"

#endif
