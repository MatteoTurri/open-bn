/*
 * SceneWindow: windows showing a PSML document. Creation and destruction, the
 * per-frame Anim/Draw of all windows (background, fading out, main, modal,
 * foreground), user data, callbacks and the image caches.
 */

#pragma implementation "scenewindow.h"
#include <vector>
#include "app/buffer.h"
#include "xml/xml_filedescriptor.h"
#include "app/systemoptions.h"
#include "thread/thposixthread.h"
#include "scenewindow.h"
#include "app/strutil.h"
#include "gfx/vec_types.h"
#include "scene/object.h"
#include "gfx/gm_graph.h"
#include "app/jobinit2.h"
#include "scene/scene.h"
#include "scene/camera.h"
#include "scene/disp2d.h"
#include "script.h"
#include "app/systemparam.h"
#include "ime/textim.h"
#include "thxmlloader.h"

SceneWindow *SceneWindow::main_window = 0;
SceneWindow *SceneWindow::fg_window = 0;
SceneWindow *SceneWindow::bg_window = 0;
int SceneWindow::instance_count = 0;
std::vector<SceneWindow *> SceneWindow::modal_window_list;
std::vector<SceneWindow *> SceneWindow::fadeout_window_list;

int FileImage::instance_count = 0;

/* Inline copy of ModalWindow(). */
static inline SceneWindow *TopModalWindow()
{
	int n = SceneWindow::modal_window_list.size();
	if (n != 0)
		return SceneWindow::modal_window_list[n - 1];
	return 0;
}

/* Inline copy of SetTargetAlpha(). */
static inline void SetTargetAlphaOf(SceneWindow *w, float a)
{
	w->target_alpha = a;
}

static inline StringUtil Prefix(const String &s, unsigned int n)
{
	return s.substr(0, n);
}

static inline int SchemeCmp(const StringUtil &pre, StringUtil pat)
{
	StringUtil head(pre);
	head.MakeUpper();
	pat.MakeUpper();
	return head.compare(pat);
}

void SceneWindow::Init()
{}

void SceneWindow::Exit()
{
	RemoveFadeoutWindowList();
	RemoveBG();
	RemoveFG();
	RemoveMain();
	RemoveModal();
}

SceneWindow::SceneWindow(const CreateArg &arg)
        : mode(arg.mode), psml_version(arg.psml_version), name(arg.name), uri(arg.uri), dir(arg.dir), key(arg.key),
          state(STATE_NONE), screen_w(arg.screen_w), screen_h(arg.screen_h), screen_d(arg.screen_d), rect_x(0),
          rect_y(0), rect_w(640), rect_h(SysOption.m_resHeight), shown(1), alpha(1.0f), target_alpha(1.0f),
          fadeout_time(500), hide_flag(0), scene(0), camera(0), disp2d(0), start_delay(4), saved_padhelp_onoff(0),
          saved_fg_color_a(0), saved_fgbar_color_a(0), document(arg.doc), file_image_cache(arg.file_image_cache),
          file_uri_cache(arg.file_uri_cache)
{
	switch (mode) {
	case MODE_MAIN:
		if (main_window)
			start_delay = SysOption.m_frameRate * arg.fade_time / 1000 + 4;
		StartFadeoutMainWindow(arg.fade_time);
		break;
	case MODE_FG:
		RemoveFG();
		break;
	case MODE_BG:
		if (bg_window)
			start_delay = SysOption.m_frameRate * arg.fade_time / 1000 + 4;
		StartFadeoutBGWindow(arg.fade_time);
		break;
	case MODE_MODAL:
		saved_padhelp_onoff = SystemParam::padhelp_onoff;
		saved_padhelp_str_rr = SystemParam::padhelp_str_rr;
		saved_padhelp_str_rd = SystemParam::padhelp_str_rd;
		saved_padhelp_str_ru = SystemParam::padhelp_str_ru;
		saved_padhelp_str_rl = SystemParam::padhelp_str_rl;
		saved_fg_color_a = SystemParam::fg_color_a;
		saved_fgbar_color_a = SystemParam::fgbar_color_a;
		break;
	}

	switch (mode) {
	case MODE_MAIN:
		BroadcastMessage(MSG_MAIN_CREATED, 0, 0, 0, 0);
		main_window = this;
		ForgetMainWindowFGStatus();
		break;
	case MODE_FG:
		fg_window = this;
		break;
	case MODE_BG:
		BroadcastMessage(MSG_BG_CREATED, 0, 0, 0, 0);
		bg_window = this;
		break;
	case MODE_MODAL:
		modal_window_list.push_back(this);
		break;
	}
	instance_count++;

	scene = new Scene;
	scene->SetAmbient(vec4(0.5f, 0.5f, 0.5f, 1.0f));
	scene->SetLightColor(0, vec4(1.0f, 1.0f, 1.0f, 0.0f));
	scene->SetLightColor(1, vec4(1.0f, 1.0f, 1.0f, 0.0f));
	scene->SetLightColor(2, vec4(0.0f, 0.0f, 0.0f, 0.0f));
	scene->SetLightColor(3, vec4(0.0f, 0.0f, 0.0f, 0.0f));
	scene->SetLightVector(0, vec4(-1.0f, -1.0f, 1.0f, 1.0f));
	scene->SetLightVector(1, vec4(1.0f, 1.0f, -1.0f, 1.0f));
	scene->SetLightVector(2, vec4(-1.0f, 1.0f, -1.0f, 1.0f));
	scene->SetLightVector(3, vec4(1.0f, -1.0f, 1.0f, 1.0f));
	scene->SetUserData('DNWS', this);
	if (!Object__CommonSetup(scene)) {
		scene = 0;
		state = STATE_FAILED;
		return;
	}
	scene->SetCallback(1, (void (*)(Hook *, void *, void *))cbObjReleaseScene, this);
	if (mode == MODE_MAIN || mode == MODE_BG)
		scene->SetPacketFence(0x8000, 0x40000);
	if (psml_version == 100)
		scene->SetPixelOffset(0.5625f, 0.5625f);

	camera = new Camera(scene);
	camera->SetVirtualScreen(screen_w, screen_h, screen_d);
	camera->SetViewport(rect_x, rect_y, rect_w, rect_h);
	if (!Object__CommonSetup(camera)) {
		delete scene;
		scene = 0;
		camera = 0;
		state = STATE_FAILED;
		return;
	}
	Object__DisableDestroyFromScript(camera);

	disp2d = new Disp2d(camera);
	if (!Object__CommonSetup(disp2d)) {
		delete scene;
		scene = 0;
		camera = 0;
		disp2d = 0;
		state = STATE_FAILED;
		return;
	}
	disp2d->AdjustCamera(camera);
	Object__DisableDestroyFromScript(camera);

	Script *s = CreateScript(scene, "main", "", arg.script);
	if (!s) {
		state = STATE_FAILED;
		return;
	}
	scene->SetUserData('LAMS', s);
	if (start_delay)
		s->flags |= 1;

	{
		NodeList list = document.getElementsByTagName(XMLString("*", false));
		for (unsigned long i = 0; i < list.get_length(); i++)
			node_list.Regist(list.item(i));
	}
	state = STATE_READY;
}

/* Runs the destroy callbacks; a modal window restores the pad-help and FG
   color parameters saved by the constructor. Then deletes the scene and
   releases the script modules of the cached images. */
SceneWindow::~SceneWindow()
{
	DoCallback(CALLBACK_ON_DESTROY);
	if (mode == MODE_MODAL) {
		SystemParam::padhelp_onoff = saved_padhelp_onoff;
		SystemParam::padhelp_str_rr = saved_padhelp_str_rr;
		SystemParam::padhelp_str_rd = saved_padhelp_str_rd;
		SystemParam::padhelp_str_ru = saved_padhelp_str_ru;
		SystemParam::padhelp_str_rl = saved_padhelp_str_rl;
		SystemParam::fg_color_a = saved_fg_color_a;
		SystemParam::fgbar_color_a = saved_fgbar_color_a;
		if (fg_window)
			fg_window->target_alpha = saved_fg_color_a / 255.0f;
	}
	delete scene;
	for (std::map<String, FileImagePtr>::iterator it = file_image_cache.begin(); it != file_image_cache.end(); it++)
		Script::ReleaseModule((*it).second);
	instance_count--;
}

/* Tells whether uri starts with "http:" or "https:", ignoring case. */
bool SceneWindow::IsFromNetwork() const
{
	String u = uri;
	return SchemeCmp(Prefix(u, 5), "http:") == 0 || SchemeCmp(Prefix(u, 6), "https:") == 0;
}

void SceneWindow::Show()
{
	shown = 1;
}

void SceneWindow::Hide()
{
	shown = 0;
}

void SceneWindow::GetRect(int &x, int &y, int &w, int &h) const
{
	x = rect_x;
	y = rect_y;
	w = rect_w;
	h = rect_h;
}

void SceneWindow::SetRect(int x, int y, int w, int h)
{
	rect_x = x;
	rect_y = y;
	rect_w = w;
	rect_h = h;
}

void SceneWindow::GetScreen(int &w, int &h, int &d) const
{
	w = screen_w;
	h = screen_h;
	d = screen_d;
}

void *SceneWindow::GetFileImage(const String &key) const
{
	std::map<String, FileImagePtr>::const_iterator it = file_image_cache.find(key);
	if (it != file_image_cache.end())
		return it->second;
	return 0;
}

String SceneWindow::GetFileURI(const String &key) const
{
	std::map<String, String>::const_iterator it = file_uri_cache.find(key);
	if (it != file_uri_cache.end())
		return it->second;
	return "";
}

bool SceneWindow::Fade()
{
	if (target_alpha == alpha)
		return true;
	if (alpha < target_alpha) {
		alpha += 1000.0f / (SysOption.m_frameRate * fadeout_time);
		if (alpha >= target_alpha) {
			alpha = target_alpha;
			return true;
		}
	} else {
		alpha -= 1000.0f / (SysOption.m_frameRate * fadeout_time);
		if (alpha <= target_alpha) {
			alpha = target_alpha;
			return true;
		}
	}
	return false;
}

void SceneWindow::DrawAll()
{
	std::vector<SceneWindow *>::iterator it;

	if (bg_window && bg_window->state == STATE_READY)
		bg_window->Draw();

	for (it = fadeout_window_list.begin(); it != fadeout_window_list.end(); ++it)
		(*it)->Draw();

	if (main_window && main_window->state == STATE_READY)
		main_window->Draw();

	for (it = modal_window_list.begin(); it != modal_window_list.end(); ++it)
		(*it)->Draw();

	if (fg_window && fg_window->state == STATE_READY)
		fg_window->Draw();
}

/* Per frame: deletes the released or failed windows, fades out and deletes the
   fading windows, sets the alpha targets and runs Anim. Background and main
   wait while a modal window is up or loading; background, main and modal also
   wait while the text input method is open on a window that is not PSML 100. */
void SceneWindow::AnimAll()
{
	if (bg_window) {
		if (bg_window->state == STATE_RELEASED)
			goto bg_delete;
		if (bg_window->state != STATE_FAILED)
			goto bg_skip;
bg_delete:
		delete bg_window;
		bg_window = 0;
bg_skip:;
	}

	std::vector<SceneWindow *>::iterator it = fadeout_window_list.begin();
	while (it != fadeout_window_list.end()) {
		SetTargetAlphaOf(*it, 0.0f);
		if ((*it)->Fade()) {
			delete *it;
			it = fadeout_window_list.erase(it);
		} else {
			++it;
		}
	}

	if (main_window) {
		if (main_window->state == STATE_RELEASED)
			goto main_delete;
		if (main_window->state != STATE_FAILED)
			goto main_skip;
main_delete:
		delete main_window;
		main_window = 0;
		ForgetMainWindowFGStatus();
main_skip:;
	}

	it = modal_window_list.begin();
	while (it != modal_window_list.end()) {
		if ((*it)->state == STATE_RELEASED)
			goto modal_delete;
		if ((*it)->state != STATE_FAILED)
			goto modal_next;
modal_delete:
		delete *it;
		it = modal_window_list.erase(it);
		continue;
modal_next:
		++it;
	}

	if (fg_window) {
		if (fg_window->state == STATE_RELEASED)
			goto fg_delete;
		if (fg_window->state != STATE_FAILED)
			goto fg_skip;
fg_delete:
		delete fg_window;
		fg_window = 0;
fg_skip:;
	}

	int modalActive = (TopModalWindow() != 0) || (thXmlLoader::for_modal_window_count != 0);

	int textImActive = TextIM::IsOpened();
	if (textImActive) {
		Object *clientObj = TextIM::GetClientObject();
		if (clientObj) {
			SceneWindow *w = GetSceneWindow(clientObj);
			if (w->psml_version == 100)
				textImActive = 0;
		}
	}

	float bgAlpha = 1.0f;
	if (main_window && (main_window->hide_flag & 1))
		bgAlpha = 0.0f;
	for (unsigned int i = 0; i < modal_window_list.size(); i++) {
		if (modal_window_list[i]->hide_flag & 1) {
			bgAlpha = 0.0f;
			break;
		}
	}
	if (bg_window) {
		SetTargetAlphaOf(bg_window, bgAlpha);
		bg_window->Fade();
	}
	if (main_window) {
		SetTargetAlphaOf(main_window, modalActive ? 0.0f : 1.0f);
		main_window->Fade();
	}

	for (unsigned int i = 0; i < modal_window_list.size(); i++) {
		SetTargetAlphaOf(modal_window_list[i], (i < modal_window_list.size() - 1) ? 0.0f : 1.0f);
		modal_window_list[i]->Fade();
	}

	if (fg_window)
		fg_window->Fade();

	if (!modalActive) {
		if (!textImActive) {
			if (bg_window && bg_window->state == STATE_READY)
				bg_window->Anim();
		}
	}
	if (!modalActive) {
		if (!textImActive) {
			if (main_window && main_window->state == STATE_READY)
				main_window->Anim();
		}
	}
	if (!textImActive) {
		if (thXmlLoader::for_modal_window_count == 0) {
			int n = modal_window_list.size();
			if (n != 0)
				modal_window_list[n - 1]->Anim();
		}
	}
	if (fg_window && fg_window->state == STATE_READY)
		fg_window->Anim();
}

void SceneWindow::Draw()
{
	if (!shown)
		return;
	DoCallback(CALLBACK_EVERY_FRAME);
	if (alpha == 0.0f)
		return;
	if (g_graph.fState == 0)
		return;

	Object::upmat_count_L = 0;
	Object::upmat_count_W = 0;
	scene->UpdateScene(g_graph);

	camera->SetViewport(rect_x, rect_y, rect_w, rect_h);

	scene->RefCamera(scene->camera);

	scene->SetColor(1.0f, 1.0f, 1.0f, alpha);

	scene->DrawScene(g_graph);
}

/* Counts start_delay down; when it reaches 0, runs PageInit for a main window and
   clears bit 0 of the flags of the script attached to the scene. Then calls
   the per-frame hook of the scene. */
void SceneWindow::Anim()
{
	if (start_delay > 0) {
		start_delay--;
		if (start_delay == 0) {
			if (mode == MODE_MAIN)
				SystemParam::PageInit();
			Script *r = (Script *)scene->GetUserData(0x4c414d53u);  /* 'LAMS' */
			if (r)
				r->flags &= ~1;
		}
	}

	scene->Anim();
}

void SceneWindow::cbObjReleaseScene(Object *obj, void *userData, void *arg2)
{
	((SceneWindow *)userData)->cbReleaseScene();
}

void SceneWindow::cbReleaseScene()
{
	scene = 0;
	state = STATE_RELEASED;
}

void SceneWindow::RemoveMain()
{
	delete main_window;
	main_window = 0;
	ForgetMainWindowFGStatus();
}

void SceneWindow::RemoveBG()
{
	delete bg_window;
	bg_window = 0;
}

void SceneWindow::RemoveFG()
{
	delete fg_window;
	fg_window = 0;
}

void SceneWindow::RemoveModal()
{
	std::vector<SceneWindow *>::iterator it = modal_window_list.begin();
	while (it != modal_window_list.end()) {
		delete *it;
		it = modal_window_list.erase(it);
	}
}

void SceneWindow::RemoveFadeoutWindowList()
{
	std::vector<SceneWindow *>::iterator it = fadeout_window_list.begin();
	while (it != fadeout_window_list.end()) {
		delete *it;
		it = fadeout_window_list.erase(it);
	}
}

/* Move the active window to the fade-out list, after announcing it with
   BroadcastMessage (MSG_MAIN_FADEOUT or MSG_BG_FADEOUT); only the main variant forgets the FG status. */
void SceneWindow::StartFadeoutMainWindow(int time)
{
	if (main_window) {
		BroadcastMessage(MSG_MAIN_FADEOUT, 0, 0, 0, 0);
		main_window->AbortAttachedScript();
		main_window->fadeout_time = time;
		fadeout_window_list.push_back(main_window);
		main_window = 0;
		ForgetMainWindowFGStatus();
	}
}

void SceneWindow::StartFadeoutBGWindow(int time)
{
	if (bg_window) {
		BroadcastMessage(MSG_BG_FADEOUT, 0, 0, 0, 0);
		bg_window->AbortAttachedScript();
		bg_window->fadeout_time = time;
		fadeout_window_list.insert(fadeout_window_list.begin(), bg_window);
		bg_window = 0;
	}
}

/* Detaches and releases the script attached to every object of the tree. */
static void SceneAbortScript(Object *o)
{
	if (!o)
		return;

	Script *s = (Script *)o->DelUserData('LAMS');

	if (s)
		s->Release();

	int n = o->children.size();

	if (n == 0)
		return;
	for (int i = 0; i < n; i++)
		SceneAbortScript(o->children[i]);
}

/* Resets the pad-help state saved in the first modal window to the defaults. */
void SceneWindow::ForgetMainWindowFGStatus()
{
	if (modal_window_list.size() != 0) {
		modal_window_list[0]->saved_padhelp_onoff = 0;
		modal_window_list[0]->saved_padhelp_str_rr = SystemParam::padhelp_str_def_rr;
		modal_window_list[0]->saved_padhelp_str_rd = SystemParam::padhelp_str_def_rd;
		modal_window_list[0]->saved_padhelp_str_ru = SystemParam::padhelp_str_def_ru;
		modal_window_list[0]->saved_padhelp_str_rl = SystemParam::padhelp_str_def_rl;
		modal_window_list[0]->saved_fg_color_a = 255;
		modal_window_list[0]->saved_fgbar_color_a = 100;
	}
}

void SceneWindow::AbortAttachedScript()
{
	SceneAbortScript(scene);
}

void SceneWindow::MakeFullPath(String &path)
{
	path = GetFullPath(dir, path, psml_version);
}

/* Records key among the images in use: a set for the lookup and a vector for
   the order, capped at 256 entries (the oldest one is dropped). */
void SceneWindow::UsingImageList_Add(const String &key)
{
	std::set<String>::iterator it = image_list_check.find(key);
	if (it != image_list_check.end())
		return;
	image_list_check.insert(key);
	image_list_order.push_back(key);
	if (image_list_order.size() > 256) {
		String front = image_list_order[0];
		it = image_list_check.find(front);
		if (it != image_list_check.end()) {
			image_list_check.erase(it);
			image_list_order.erase(image_list_order.begin());
		}
	}
}

bool SceneWindow::UsingImageList_Check(const String &key) const
{
	return image_list_check.find(key) != image_list_check.end();
}

FileImage::FileImage(void *buf) : data(0)
{
	instance_count++;
	data = buf;
	m_ref = 1;
}

FileImage::~FileImage()
{
	instance_count--;
	if (data)
		free(data);
}

FileImagePtr::FileImagePtr() : m_p(0)
{}

FileImagePtr::FileImagePtr(void *buf) : m_p(0)
{
	m_p = new FileImage(buf);
}

FileImagePtr::FileImagePtr(const FileImagePtr &o) : m_p(0)
{
	if (&o != this) {
		if (m_p) {
			m_p->m_ref--;
			if (m_p->m_ref == 0)
				delete m_p;
		}
		if (o.m_p)
			o.m_p->m_ref++;
		m_p = o.m_p;
	}
}

FileImagePtr::~FileImagePtr()
{
	if (m_p) {
		m_p->m_ref--;
		if (m_p->m_ref == 0)
			delete m_p;
	}
}

FileImagePtr::operator void *() const
{
	if (m_p != 0)
		return m_p->data;
	return 0;
}

void FileImagePtr::operator=(void *buf)
{
	m_p = new FileImage(buf);
}

void FileImagePtr::operator=(const FileImagePtr &o)
{
	if (&o != this) {
		if (m_p) {
			m_p->m_ref--;
			if (m_p->m_ref == 0)
				delete m_p;
		}
		if (o.m_p)
			o.m_p->m_ref++;
		m_p = o.m_p;
	}
}

SceneWindow *CreateSceneWindow(const SceneWindow::CreateArg &arg)
{
	return new SceneWindow(arg);
}

void SceneWindow::BroadcastMessage(int msg, int a, int b, int c, int d)
{
	Script *s;

	s = GetSceneScript(main_window);
	if (s)
		s->SendMessage(msg, a, b, c, d);

	s = GetSceneScript(bg_window);
	if (s)
		s->SendMessage(msg, a, b, c, d);

	int n = modal_window_list.size();
	SceneWindow *top = (n != 0) ? modal_window_list[n - 1] : 0;
	s = GetSceneScript(top);
	if (s)
		s->SendMessage(msg, a, b, c, d);

	s = GetSceneScript(fg_window);
	if (s)
		s->SendMessage(msg, a, b, c, d);
}

void SceneWindow::BroadcastMessage(int msg, const String &str)
{
	Script *s;

	s = GetSceneScript(main_window);
	if (s)
		s->SendMessage(msg, str);

	s = GetSceneScript(bg_window);
	if (s)
		s->SendMessage(msg, str);

	int n = modal_window_list.size();
	SceneWindow *top = (n != 0) ? modal_window_list[n - 1] : 0;
	s = GetSceneScript(top);
	if (s)
		s->SendMessage(msg, str);

	s = GetSceneScript(fg_window);
	if (s)
		s->SendMessage(msg, str);
}

void *SceneWindow::SetUserData(unsigned int key, void *data)
{
	std::map<unsigned int, void *>::const_iterator it = user_data.find(key);
	void *old = (it != user_data.end()) ? it->second : 0;

	user_data[key] = data;
	return old;
}

void *SceneWindow::GetUserData(unsigned int key)
{
	std::map<unsigned int, void *>::const_iterator it = user_data.find(key);
	if (it != user_data.end())
		return it->second;
	return 0;
}

/* Removes key and returns its value, or 0 if it was not there. */
void *SceneWindow::DelUserData(unsigned int key)
{
	std::map<unsigned int, void *>::iterator it = user_data.find(key);
	if (it != user_data.end()) {
		void *old = (*it).second;
		user_data.erase(it);
		return old;
	}
	return 0;
}

void SceneWindow::SetCallback(CallbackType type, unsigned int key, void (*fn)(void))
{
	for (std::vector<CallbackData>::iterator it = callbacks[type].begin(); it != callbacks[type].end(); ++it) {
		if ((*it).key == key) {
			(*it).fn = fn;
			return;
		}
	}
	CallbackData e;
	CallbackData *p = &e;
	p->key = key;
	p->fn = fn;
	callbacks[type].push_back(*p);
}

void SceneWindow::DelCallback(CallbackType type, unsigned int key)
{
	std::vector<CallbackData>::iterator it;
	for (it = callbacks[type].begin(); it != callbacks[type].end(); ++it) {
		if (it->key == key) {
			callbacks[type].erase(it);
			return;
		}
	}
}

void SceneWindow::DoCallback(CallbackType type)
{
	std::vector<CallbackData>::iterator it;
	for (it = callbacks[type].begin(); it != callbacks[type].end(); ++it) {
		(*it->fn)();
	}
}

inline int SceneWindow::Mode() const
{
	return mode;
}

inline int SceneWindow::PSMLVersion() const
{
	return psml_version;
}

inline const String &SceneWindow::GetName() const
{
	return name;
}

inline const String &SceneWindow::GetURI() const
{
	return uri;
}

inline const String &SceneWindow::GetDir() const
{
	return dir;
}

inline const String &SceneWindow::GetKey() const
{
	return key;
}

inline int SceneWindow::State() const
{
	return state;
}

inline Scene *SceneWindow::GetScene() const
{
	return scene;
}

inline Camera *SceneWindow::GetCamera() const
{
	return camera;
}

inline Disp2d *SceneWindow::GetDisp2d() const
{
	return disp2d;
}

inline void *SceneWindow::GetNodeList()
{
	return &node_list;
}

inline void *SceneWindow::GetDocument()
{
	return &document;
}

inline void SceneWindow::SetTargetAlpha(float a)
{
	target_alpha = a;
}

inline void SceneWindow::SetFadeoutTime(int t)
{
	fadeout_time = t;
}

inline void SceneWindow::SetHide(int hide)
{
	hide_flag = hide;
}

inline int SceneWindow::InstanceCount()
{
	return instance_count;
}

inline SceneWindow *SceneWindow::MainWindow()
{
	return main_window;
}

inline SceneWindow *SceneWindow::ModalWindow()
{
	int n = modal_window_list.size();
	if (n != 0)
		return modal_window_list[n - 1];
	return 0;
}

inline SceneWindow *SceneWindow::FGWindow()
{
	return fg_window;
}

inline SceneWindow *SceneWindow::BGWindow()
{
	return bg_window;
}
