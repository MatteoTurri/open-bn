/*
 * Small/amx natives for scene objects (tables smallext_3d and smallext_3d_p):
 * creation of 3D and 2D objects, position, rotation, scale, color, options,
 * animations, lights, fog and camera. A script refers to an object by its
 * ObjectTable handle; handle 0 is the object that runs the script.
 */

#include "../../ref/small-fmt6/amx.h"
#include "natives/smallext.h"
#include <vector>
#include <string>
#include "xml/xml_filedescriptor.h"
/* The vector functions are called out of line here: vecmath.h, not the inline gfx/vec.h. */
#include "gfx/vecmath.h"
#include "gfx/tm.h"
#include "gfx/font.h"
#include "scene/object.h"
#include "scene/animation.h"
#include "scene/animcolor.h"
#include "scene/animscale.h"
#include "scene/animquat.h"
#include "scene/animrotate.h"
#include "scene/animmoveline.h"
#include "scene/animmovehermite.h"
#include "scene/scene.h"
#include "scene/objecttable.h"
#include "scene/camera.h"
#include "scene/disp2d.h"
#include "script/scenewindow.h"
#include "script/script.h"
#include "ime/textim_arg.h"
#include "scene/light.h"
#include "scene/plane.h"
#include "scene/particle.h"
#include "ui/label.h"
#include "scene/prim.h"
#include "scene/hermite.h"
#include "scene/crystal.h"
#include "scene/defocus.h"
#include "scene/icon.h"
#include "scene/sscope0.h"
#include "scene/model.h"
#include "scene/mesh.h"
#include "scene/cloud.h"
#include "app/resource.h"
#include "ui/multilinelabel.h"
#include "ui/textfield.h"
#include "ui/simpletextedit.h"

class Object;
class gm_graph;
class SceneWindow;

/* State block of the module; only hook is ever written. */
struct NativeModuleState {
	/* 0x00 */ void *hook;
	/* 0x04 */ int pad_04;  /* never used */
};

static NativeModuleState s_module = { 0, 0 };

/* Default material colors of the crystal, passed by address to SetOption by CreateCrystal. */
static float s_defRFC0[4] = { 1.2f, 1.2f, 1.2f, 1.0f };
static float s_defRFC1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static float s_defEMC0[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static float s_defEMC1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static float s_defMMC[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static float s_defGMC[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
/* Live multiline editors: all of them, and those opened by a scene from the network. */
static int s_editCount;
static int s_editNetCount;

static const long TAG_HOBJ = 0x4a424f48;   /* 'HOBJ' */
static const long TAG_HNDL = 0x4c444e48;   /* 'HNDL' */
static const long TAG_SWND = 0x444e5753;   /* 'SWND' */
static const long TAG_FONT = 0x544e4f46;   /* 'FONT' */
static const long TAG_CLUD = 0x44554c43;   /* 'CLUD' */
static const long TAG_DCLD = 0x444c4344;   /* 'DCLD' */
static const long TAG_RAYM = 0x4d594152;   /* 'RAYM' */
static const long TAG_ANMP = 0x504d4e41;   /* 'ANMP' */
static const long TAG_ANMR = 0x524d4e41;   /* 'ANMR' */
static const long TAG_ANMS = 0x534d4e41;   /* 'ANMS' */
static const long TAG_ANMC = 0x434d4e41;   /* 'ANMC' */
static const long TAG_ANMT = 0x544d4e41;   /* 'ANMT' */
static const long TAG_LOCK = 0x4b434f4c;   /* 'LOCK' */
static const long TAG_PSCR = 0x52435350;   /* 'PSCR' */
static const long TAG_FOGR = 0x52474f46;   /* 'FOGR' */
static const long TAG_FOGC = 0x43474f46;   /* 'FOGC' */
static const long TAG_AMB = 0x00424d41;   /* 'AMB' + NUL */

/* Target object of a native: params[1] is its ObjectTable handle, 0 = the object of the script. */
static inline Object *AmxObject(AMX *amx, cell *params)
{
	void *h;
	cell id;
	if (params[1] != 0) {
		id = params[1];
	} else {
		amx_GetUserData(amx, TAG_HOBJ, &h);
		id = (cell)h;
	}
	return ObjectTable::Get(id);
}

/* The object of the script: the handle stored as user data 'HOBJ' of the AMX. */
static inline Object *AmxSelf(AMX *amx)
{
	void *h;
	amx_GetUserData(amx, TAG_HOBJ, &h);
	return ObjectTable::Get((cell)h);
}

/* Same as AmxObject. */
static inline Object *AmxObjectAlt(AMX *amx, cell *params)
{
	void *h;
	cell id = params[1];
	if (id == 0) {
		amx_GetUserData(amx, TAG_HOBJ, &h);
		id = (cell)h;
	}
	return ObjectTable::Get(id);
}

/* Typed views of the user data of a hook: the animations of an object are kept under TAG_ANM*,
   the ObjectTable handle under TAG_HNDL, the window of a scene under TAG_SWND. */
static inline Animation *GetAnimation(Hook *h, long tag)
{
	return (Animation *)h->GetUserData(tag);
}

static inline Animation *SetAnimation(Hook *h, long tag, Animation *a)
{
	return (Animation *)h->SetUserData(tag, a);
}

static inline Animation *DelAnimation(Hook *h, long tag)
{
	return (Animation *)h->DelUserData(tag);
}

static inline cell GetHandle(Hook *h)
{
	return (cell)h->GetUserData(TAG_HNDL);
}

static inline SceneWindow *SceneWindowOf(Hook *scene)
{
	return (SceneWindow *)scene->GetUserData(TAG_SWND);
}

/* Stores the hook in the module state block and returns the block. Never called. */
static NativeModuleState *NativeObject_SetHook(void *p)
{
	s_module.hook = p;
	return &s_module;
}

/* FourCC of a string (first character in the low byte, stops at the first NUL). */
static long NativeObject_StrToFourCC(const unsigned char *s)
{
	int c;
	long v;

	if (s == 0)
		return 0;
	v = 0;
	c = s[0];
	if (c != 0) {
		v = c;
		c = s[1];
		if (c != 0) {
			v |= c << 8;
			c = s[2];
			if (c != 0) {
				v |= c << 16;
				c = s[3];
				if (c != 0)
					v |= c << 24;
			}
		}
	}
	return v;
}

static cell AMX_NATIVE_CALL n_Self(AMX *amx, cell *params)
{
	void *h;
	amx_GetUserData(amx, TAG_HOBJ, &h);
	return (cell)h;
}

static cell AMX_NATIVE_CALL n_Parent(AMX *amx, cell *params)
{
	cell r;
	Object *o = AmxObject(amx, params);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		r = 0;
	} else if (o->parent == 0) {
		r = 0;
	} else {
		r = GetHandle(o->parent);
	}
	return r;
}

static cell AMX_NATIVE_CALL n_Scene(AMX *amx, cell *params)
{
	cell r;
	Object *o = AmxObject(amx, params);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		r = 0;
	} else if (o->scene == 0) {
		r = 0;
	} else {
		r = GetHandle(o->scene);
	}
	return r;
}

static cell AMX_NATIVE_CALL n_Camera(AMX *amx, cell *params)
{
	cell r;
	Object *o = AmxObject(amx, params);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		r = 0;
	} else {
		SceneWindow *w = SceneWindowOf(o->scene);
		if (w->camera == 0)
			r = 0;
		else
			r = GetHandle(w->camera);
	}
	return r;
}

static cell AMX_NATIVE_CALL n_Disp2D(AMX *amx, cell *params)
{
	cell r;
	Object *o = AmxObject(amx, params);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		r = 0;
	} else {
		SceneWindow *w = SceneWindowOf(o->scene);
		if (w->disp2d == 0)
			r = 0;
		else
			r = GetHandle(w->disp2d);
	}
	return r;
}

static cell AMX_NATIVE_CALL n_IsValidObject(AMX *amx, cell *params)
{
	return ObjectTable::Get(params[1]) != 0;
}

static cell AMX_NATIVE_CALL n_CreateNull(AMX *amx, cell *params)
{
	cell h;
	Object *o;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	o = new Object(parent);
	o->visible = 0;
	h = (cell)Object__CommonSetup(o);
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateLight(AMX *amx, cell *params)
{
	cell h;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = (cell)Object__CommonSetup(new Light(parent, params[2]));
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreatePlane(AMX *amx, cell *params)
{
	cell h;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = (cell)Object__CommonSetup(new Plane(parent));
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateParticle(AMX *amx, cell *params)
{
	cell h;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = (cell)Object__CommonSetup(new Particle(parent));
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateLabel(AMX *amx, cell *params)
{
	cell h;
	Label *l;
	int font;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	l = new Label(parent, false, 1.0f);
	font = (int)Font::Get(0);
	l->SetOption(TAG_FONT, font, 1);
	h = (cell)Object__CommonSetup(l);
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateLabelShadow(AMX *amx, cell *params)
{
	cell h;
	Label *l;
	int font;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	l = new Label(parent, true, 1.0f);
	font = (int)Font::Get(0);
	l->SetOption(TAG_FONT, font, 1);
	h = (cell)Object__CommonSetup(l);
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateTextbox(AMX *amx, cell *params)
{
	cell h;
	MultilineLabel *l;
	int font;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	l = new MultilineLabel(parent);
	l->SetFrameSize64(6400, 6400);
	font = (int)Font::Get(0);
	l->SetOption(TAG_FONT, font, 1);
	h = (cell)Object__CommonSetup(l);
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateEdit(AMX *amx, cell *params)
{
	cell h;
	TextField *t;
	int font;
	Object *parent;
	cell max = 1024;
	if ((ucell)params[0] / sizeof(cell) >= 2)
		max = params[2];
	parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	t = new TextField(parent, max);
	font = (int)Font::Get(0);
	t->SetOption(TAG_FONT, font, 1);
	t->SetFieldSize(380, 20);
	h = (cell)Object__CommonSetup(t);
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

/* Destroy callback installed by CreateEditMLImpl: updates the live editor counters. */
static void EditML_OnDestroy(Hook *hook, void *a, void *b)
{
	SceneWindow *w = GetSceneWindow((Object *)hook);
	s_editCount--;
	if (w->IsFromNetwork())
		s_editNetCount--;
}

/* Modify verify callback installed by CreateEditMLImpl: does nothing. */
static void EditML_OnModify(SimpleTextEdit::CallData_ModifyVerify *data)
{}

static const long TAG_FSIZ = 0x5a495346;   /* 'FSIZ' */
static const long TAG_SDWE = 0x45574453;   /* 'SDWE' */
static const long TAG_CSPC = 0x43505343;   /* 'CSPC' */
static const long TAG_LSPC = 0x4350534c;   /* 'LSPC' */
static const long TAG_CRSW = 0x57535243;   /* 'CRSW' */
static const long TAG_EDTE = 0x45544445;   /* 'EDTE' */
static const long TAG_MDFY = 0x5946444d;   /* 'MDFY' */
static const long TAG_COLR = 0x524c4f43;   /* 'COLR' */
static const long TAG_BGC4 = 0x34434742;   /* 'BGC4' */
static const long TAG_CSC4 = 0x34435343;   /* 'CSC4' */

/* Default settings of the multiline editor. */
static inline void EditML_Setup(SimpleTextEdit *e)
{
	e->SetFieldSize(380, 100);
	e->SetModifyVerifyCallback(EditML_OnModify);
	e->SetFontPathE("/opt0/bn/fonts/SGR_SE1.PST");
	e->SetFontPathJ("/opt0/bn/fonts/SGR_SCE.PST");
	e->SetOption(TAG_FSIZ, 24, 1);
	e->SetOption(TAG_SDWE, 0, 1);
	e->SetOption(TAG_CSPC, 0, 1);
	e->SetOption(TAG_LSPC, 0, 1);
	e->SetOption(TAG_CRSW, 0, 1);
	e->SetOption(TAG_EDTE, 0, 1);
	e->SetOption(TAG_EDTE, 1, 1);
	e->SetOption(TAG_CRSW, 1, 1);
	e->SetOption(TAG_MDFY, 0, 1);
	e->SetWideModeCheckFunction(TextIM_SkbCheckShownSKB);
	unsigned char color[4] = { 0x80, 0xa0, 0xff, 0xff };
	e->SetOption(TAG_COLR, color, 0);
	unsigned char bgcolor[16] = { 0x40, 0x40, 0x50, 0x80, 0x40, 0x40, 0x50, 0x80, 0x60, 0x60, 0x80, 0x80, 0x60,
		0x60, 0x80, 0x80 };
	e->SetOption(TAG_BGC4, bgcolor, 0);
	unsigned char cscolor[16] = { 0xa0, 0xa0, 0xa0, 0xff, 0xa0, 0xa0, 0xa0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff };
	e->SetOption(TAG_CSC4, cscolor, 0);
}

/*
 * Body of CreateEditML and CreateEditML_P (password flag): at most 16 live
 * editors, at most 2 of them opened by a scene from the network.
 */
static cell CreateEditMLImpl(AMX *amx, cell *params, int password)
{
	SimpleTextEdit *e;
	SceneWindow *w;
	cell h;
	Object *parent = ObjectTable::Get(params[1]);

	if (parent == 0 || AmxSelf(amx)->scene != parent->scene)
		goto err;
	w = GetSceneWindow(amx);
	if (s_editCount + 1 > 16)
		goto err;
	if (w->IsFromNetwork() && s_editNetCount + 1 > 2)
		goto err;
	e = new SimpleTextEdit(parent, password);
	EditML_Setup(e);
	h = (cell)Object__CommonSetup(e);
	if (h == 0) {
err:
		amx_RaiseError(amx, 10);
		return 0;
	}
	s_editCount++;
	if (w->IsFromNetwork())
		s_editNetCount++;
	e->SetCallback(1, EditML_OnDestroy, 0);
	return h;
}

static cell AMX_NATIVE_CALL n_CreateEditML_P(AMX *amx, cell *params)
{
	return CreateEditMLImpl(amx, params, 1);
}

static cell AMX_NATIVE_CALL n_CreateEditML(AMX *amx, cell *params)
{
	return CreateEditMLImpl(amx, params, 0);
}

/* CalcNumLine(edit, width): number of lines of the text of a multiline editor. */
static cell AMX_NATIVE_CALL n_CalcNumLine(AMX *amx, cell *params)
{
	SimpleTextEdit *e;
	Object *o = ObjectTable::Get(params[1]);

	if (o == 0)
		goto err;
	if (AmxSelf(amx)->scene != o->scene)
		goto err;
	e = dynamic_cast<SimpleTextEdit *>(o);
	if (e == 0) {
err:
		amx_RaiseError(amx, 10);
		return 0;
	}
	return e->CalcNumLine(params[2]);
}

static cell AMX_NATIVE_CALL n_CreateCube(AMX *amx, cell *params)
{
	cell h;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = (cell)Object__CommonSetup(new Prim(parent));
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateCloud(AMX *amx, cell *params)
{
	cell h;
	Cloud *c;
	int raym;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	c = new Cloud(parent, 100, 50);
	c->SetOption(TAG_CLUD, &CloudInfo, 0);
	c->SetOption(TAG_DCLD, &CloudInfo, 0);
	raym = 3;
	c->SetOption(TAG_RAYM, raym, 1);
	c->SetColor(0.2f, 0.6f, 1.0f, 0.5f);
	h = (cell)Object__CommonSetup(c);
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateHermite(AMX *amx, cell *params)
{
	cell h;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = (cell)Object__CommonSetup(new Hermite(parent, 0));
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

/*
 * CreateCrystal(parent): creates a crystal and sets its material: the six
 * default colors, two reflection factors and the alpha mode.
 */
static cell AMX_NATIVE_CALL n_CreateCrystal(AMX *amx, cell *params)
{
	cell h;
	Crystal *c;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	c = new Crystal(parent);
	c->SetOption(NativeObject_StrToFourCC((const unsigned char *)"RFF0"), 0.2f, 1);
	c->SetOption(NativeObject_StrToFourCC((const unsigned char *)"RFF1"), 0.1f, 1);
	c->SetOption(NativeObject_StrToFourCC((const unsigned char *)"RFC0"), s_defRFC0, 0);
	c->SetOption(NativeObject_StrToFourCC((const unsigned char *)"RFC1"), s_defRFC1, 0);
	c->SetOption(NativeObject_StrToFourCC((const unsigned char *)"EMC0"), s_defEMC0, 0);
	c->SetOption(NativeObject_StrToFourCC((const unsigned char *)"EMC1"), s_defEMC1, 0);
	c->SetOption(NativeObject_StrToFourCC((const unsigned char *)"MMC"), s_defMMC, 0);
	c->SetOption(NativeObject_StrToFourCC((const unsigned char *)"GMC"), s_defGMC, 0);
	c->SetColor(0.0f, 0.0f, 0.0f, 0.0f);
	c->SetOption(NativeObject_StrToFourCC((const unsigned char *)"ALPH"), 2, 1);
	h = (cell)Object__CommonSetup(c);
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateDefocus(AMX *amx, cell *params)
{
	cell h;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = (cell)Object__CommonSetup(new Defocus(parent));
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateIcon(AMX *amx, cell *params)
{
	cell h;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = (cell)Object__CommonSetup(new Icon(parent));
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateSScope0(AMX *amx, cell *params)
{
	cell h;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = (cell)Object__CommonSetup(new SScope0(parent));
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateModel(AMX *amx, cell *params)
{
	cell h;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = (cell)Object__CommonSetup(new Model(parent));
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_CreateMesh(AMX *amx, cell *params)
{
	cell h;
	Object *parent = ObjectTable::Get(params[1]);
	if (parent == 0 || AmxSelf(amx)->scene != parent->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = (cell)Object__CommonSetup(new Mesh(parent));
	if (h == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return h;
}

static cell AMX_NATIVE_CALL n_InitNodeArray(AMX *amx, cell *params)
{
	Hermite *h;
	int nodeCount, loop;
	Object *o = ObjectTable::Get(params[1]);

	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = dynamic_cast<Hermite *>(o);
	loop = 0;
	if (h == 0)
		return -1;
	nodeCount = params[2];
	if (params[0] / sizeof(cell) == 3)
		loop = params[3];
	h->InitNodeArray(nodeCount, loop);
	return 1;
}

static cell AMX_NATIVE_CALL n_SetNodePos(AMX *amx, cell *params)
{
	Hermite *h;
	vec4 v;
	Object *o = ObjectTable::Get(params[1]);

	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = dynamic_cast<Hermite *>(o);
	if (h == 0)
		return -1;
	v.x = amx_ctof(params[2]);
	v.y = amx_ctof(params[3]);
	v.z = amx_ctof(params[4]);
	h->SetNodePos(v, params[5]);
	return 1;
}

static cell AMX_NATIVE_CALL n_GetPosHermite(AMX *amx, cell *params)
{
	Hermite *h;
	cell *p;
	cell c;
	int last;
	int seg;
	float t;
	float f;
	Object *o = ObjectTable::Get(params[1]);

	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	h = dynamic_cast<Hermite *>(o);
	if (h == 0)
		return -1;
	last = params[6] - 1;
	t = amx_ctof(params[5]) * (float)last;
	seg = (int)t;
	f = t - (float)seg;
	if (seg >= last) {
		seg = params[6] - 2;
		f = 1.0f;
	}
	h->SetSegment(seg);
	vec4 v = h->GetPosHermite(f, 1, 0);
	if (amx_GetAddr(amx, params[2], &p) == 0) {
		c = amx_ftoc(v.x);
		*p = c;
	}
	if (amx_GetAddr(amx, params[3], &p) == 0) {
		c = amx_ftoc(v.y);
		*p = c;
	}
	if (amx_GetAddr(amx, params[4], &p) == 0) {
		c = amx_ftoc(v.z);
		*p = c;
	}
	return 1;
}

static cell AMX_NATIVE_CALL n_Destroy(AMX *amx, cell *params)
{
	void *script;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (o->GetUserData(TAG_LOCK) != 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	delete o;
	amx_GetUserData(amx, TAG_PSCR, &script);
	if (((Script *)script)->state == 3) {
		amx_RaiseError(amx, 1);
		return 0;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_TimerDie(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (o->GetUserData(TAG_LOCK) != 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	o->SetSuicideTimer(params[2]);
	return 0;
}

static cell AMX_NATIVE_CALL n_Show(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o != 0 && AmxSelf(amx)->scene == o->scene) {
		o->visible = 1;
		return 0;
	}
	amx_RaiseError(amx, 10);
	return 0;
}

static cell AMX_NATIVE_CALL n_ShowChildren(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o != 0 && AmxSelf(amx)->scene == o->scene) {
		o->children_visible = 1;
		return 0;
	}
	amx_RaiseError(amx, 10);
	return 0;
}

static cell AMX_NATIVE_CALL n_Hide(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	o->visible = 0;
	return 0;
}

static cell AMX_NATIVE_CALL n_HideChildren(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o != 0 && AmxSelf(amx)->scene == o->scene) {
		o->children_visible = 0;
		return 0;
	}
	amx_RaiseError(amx, 10);
	return 0;
}

static cell AMX_NATIVE_CALL n_SetScreen(AMX *amx, cell *params)
{
	SceneWindow *w;
	Camera *cam;
	Disp2d *d2;
	float x, y, z;
	Object *o = AmxObject(amx, params);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	w = SceneWindowOf(o->scene);
	cam = w->camera;
	d2 = w->disp2d;
	z = amx_ctof(params[4]);
	x = amx_ctof(params[2]);
	y = amx_ctof(params[3]);
	if (z < 0.0f)
		cam->SetVirtualScreen(x, y, -1.0f);
	else
		cam->SetVirtualScreen(x, y, z);
	d2->AdjustCamera(cam);
	return 0;
}

static cell AMX_NATIVE_CALL n_GetScreen(AMX *amx, cell *params)
{
	SceneWindow *w;
	cell *p;
	cell c;
	float x, y, z;
	Object *o = AmxObject(amx, params);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	w = SceneWindowOf(o->scene);
	x = 640.0f;
	y = 480.0f;
	z = 500.0f;
	w->camera->GetVirtualScreen(x, y, z);
	if (amx_GetAddr(amx, params[2], &p) == 0) {
		c = amx_ftoc(x);
		*p = c;
	}
	if (amx_GetAddr(amx, params[3], &p) == 0) {
		c = amx_ftoc(y);
		*p = c;
	}
	if (amx_GetAddr(amx, params[4], &p) == 0) {
		c = amx_ftoc(z);
		*p = c;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_SetClipRange(AMX *amx, cell *params)
{
	SceneWindow *w;
	Object *o = AmxObject(amx, params);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	w = SceneWindowOf(o->scene);
	w->camera->SetClipRange(amx_ctof(params[2]), amx_ctof(params[3]));
	return 0;
}

static cell AMX_NATIVE_CALL n_GetClipRange(AMX *amx, cell *params)
{
	Object *o = AmxObject(amx, params);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	SceneWindow *w = SceneWindowOf(o->scene);
	float n = 1.0f;
	float f = 32768.0f;
	w->camera->GetClipRange(n, f);
	cell *p;
	cell c;
	if (amx_GetAddr(amx, params[2], &p) == 0) {
		c = amx_ftoc(n);
		*p = c;
	}
	if (amx_GetAddr(amx, params[3], &p) == 0) {
		c = amx_ftoc(f);
		*p = c;
	}
	return 0;
}

/* SetAmbient(obj, r, g, b [, a]): ambient light of the scene; a defaults to 1.0. */
static cell AMX_NATIVE_CALL n_SetAmbient(AMX *amx, cell *params)
{
	float c[4];
	float a;
	Object *o = AmxObjectAlt(amx, params);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	c[0] = amx_ctof(params[2]);
	c[1] = amx_ctof(params[3]);
	c[2] = amx_ctof(params[4]);
	if (params[0] >= 5 * (int)sizeof(cell)) {
		cell v = params[5];
		a = amx_ctof(v);
	} else
		a = 1.0f;
	c[3] = a;
	o->SetOption(TAG_AMB, c, sizeof(c));
	return 0;
}

/* SetLightColor(obj, light, r, g, b [, a]): a defaults to 1.0. */
static cell AMX_NATIVE_CALL n_SetLightColor(AMX *amx, cell *params)
{
	Scene *sc;
	float w;
	Object *o = AmxObjectAlt(amx, params);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	sc = dynamic_cast<Scene *>(o);
	if (sc == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	int idx = params[2];
	float r = amx_ctof(params[3]);
	float g = amx_ctof(params[4]);
	float b = amx_ctof(params[5]);
	if (params[0] >= 6 * (int)sizeof(cell)) {
		cell v = params[6];
		w = amx_ctof(v);
	} else
		w = 1.0f;
	sc->SetLightColor(idx, vec4(r, g, b, w));
	return 0;
}

static cell AMX_NATIVE_CALL n_SetLightVector(AMX *amx, cell *params)
{
	Scene *sc;
	Object *o = AmxObjectAlt(amx, params);

	if (o == 0)
		goto err;
	sc = dynamic_cast<Scene *>(o);
	if (sc == 0)
		goto err;
	sc->SetLightVector(params[2], vec4(amx_ctof(params[3]), amx_ctof(params[4]), amx_ctof(params[5]), 0.0f));
	return 0;
err:
	amx_RaiseError(amx, 10);
	return 0;
}

static cell AMX_NATIVE_CALL n_SetFogColor(AMX *amx, cell *params)
{
	float c[4];
	Object *o = AmxObjectAlt(amx, params);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	c[0] = amx_ctof(params[2]);
	c[1] = amx_ctof(params[3]);
	c[2] = amx_ctof(params[4]);
	c[3] = 1.0f;
	o->SetOption(TAG_FOGC, c, sizeof(c));
	return 0;
}

static cell AMX_NATIVE_CALL n_SetFogRange(AMX *amx, cell *params)
{
	float r[2];
	Object *o = AmxObjectAlt(amx, params);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	r[0] = amx_ctof(params[2]);
	r[1] = amx_ctof(params[3]);
	o->SetOption(TAG_FOGR, r, sizeof(r));
	return 0;
}

static cell AMX_NATIVE_CALL n_SetPos(AMX *amx, cell *params)
{
	Object *rel;
	float x, y, z;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	rel = ObjectTable::Get(params[5]);
	x = amx_ctof(params[2]);
	y = amx_ctof(params[3]);
	z = amx_ctof(params[4]);
	o->SetPos(x, y, z, rel);
	delete DelAnimation(o, TAG_ANMP);
	return 0;
}

static cell AMX_NATIVE_CALL n_GetPos(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	Object *rel = ObjectTable::Get(params[5]);
	vec4 v = o->GetPos(rel, vec4(0.0f, 0.0f, 0.0f, 1.0f));
	cell *p;
	cell c;
	if (amx_GetAddr(amx, params[2], &p) == 0) {
		c = amx_ftoc(v.x);
		*p = c;
	}
	if (amx_GetAddr(amx, params[3], &p) == 0) {
		c = amx_ftoc(v.y);
		*p = c;
	}
	if (amx_GetAddr(amx, params[4], &p) == 0) {
		c = amx_ftoc(v.z);
		*p = c;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_GetOffPos(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	Object *rel = ObjectTable::Get(params[8]);
	vec4 off(amx_ctof(params[5]), amx_ctof(params[6]), amx_ctof(params[7]), 1.0f);
	vec4 v = o->GetPos(rel, off);
	cell *p;
	cell c;
	if (amx_GetAddr(amx, params[2], &p) == 0) {
		c = amx_ftoc(v.x);
		*p = c;
	}
	if (amx_GetAddr(amx, params[3], &p) == 0) {
		c = amx_ftoc(v.y);
		*p = c;
	}
	if (amx_GetAddr(amx, params[4], &p) == 0) {
		c = amx_ftoc(v.z);
		*p = c;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_SetRot(AMX *amx, cell *params)
{
	Object *rel;
	float x, y, z;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	rel = ObjectTable::Get(params[5]);
	x = amx_ctof(params[2]);
	y = amx_ctof(params[3]);
	z = amx_ctof(params[4]);
	o->SetRot(x, y, z, rel, params[6]);
	delete DelAnimation(o, TAG_ANMR);
	return 0;
}

static cell AMX_NATIVE_CALL n_GetRot(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	Object *rel = ObjectTable::Get(params[5]);
	vec4 zero(0.0f, 0.0f, 0.0f, 0.0f);
	vec4 v = o->GetRot(rel, zero, params[6]);
	cell *p;
	cell c;
	if (amx_GetAddr(amx, params[2], &p) == 0) {
		c = amx_ftoc(v.x);
		*p = c;
	}
	if (amx_GetAddr(amx, params[3], &p) == 0) {
		c = amx_ftoc(v.y);
		*p = c;
	}
	if (amx_GetAddr(amx, params[4], &p) == 0) {
		c = amx_ftoc(v.z);
		*p = c;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_SetQuat(AMX *amx, cell *params)
{
	Object *rel;
	float x, y, z, w;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	rel = ObjectTable::Get(params[6]);
	x = amx_ctof(params[2]);
	y = amx_ctof(params[3]);
	z = amx_ctof(params[4]);
	w = amx_ctof(params[5]);
	o->SetQuat(x, y, z, w, rel);
	delete DelAnimation(o, TAG_ANMR);
	return 0;
}

static cell AMX_NATIVE_CALL n_GetQuat(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	Object *rel = ObjectTable::Get(params[6]);
	vec4 q = o->GetQuat(rel, vec4(0.0f, 0.0f, 0.0f, 0.0f));
	q = quat_normalize(q);
	cell *p;
	cell c;
	if (amx_GetAddr(amx, params[2], &p) == 0) {
		c = amx_ftoc(q.x);
		*p = c;
	}
	if (amx_GetAddr(amx, params[3], &p) == 0) {
		c = amx_ftoc(q.y);
		*p = c;
	}
	if (amx_GetAddr(amx, params[4], &p) == 0) {
		c = amx_ftoc(q.z);
		*p = c;
	}
	if (amx_GetAddr(amx, params[5], &p) == 0) {
		c = amx_ftoc(q.w);
		*p = c;
	}
	return 0;
}

/* QuatInterp(&x, &y, &z, &w, a0..a3, b0..b3, t): normalized slerp of quaternions a and b. */
static cell AMX_NATIVE_CALL n_QuatInterp(AMX *amx, cell *params)
{
	cell *p;
	cell c;

	vec4 a(amx_ctof(params[5]), amx_ctof(params[6]), amx_ctof(params[7]), amx_ctof(params[8]));
	vec4 b(amx_ctof(params[9]), amx_ctof(params[10]), amx_ctof(params[11]), amx_ctof(params[12]));
	vec4 v = quat_slerp(a, b, amx_ctof(params[13]));
	v = quat_normalize(v);
	if (amx_GetAddr(amx, params[1], &p) == 0) {
		c = amx_ftoc(v.x);
		*p = c;
	}
	if (amx_GetAddr(amx, params[2], &p) == 0) {
		c = amx_ftoc(v.y);
		*p = c;
	}
	if (amx_GetAddr(amx, params[3], &p) == 0) {
		c = amx_ftoc(v.z);
		*p = c;
	}
	if (amx_GetAddr(amx, params[4], &p) == 0) {
		c = amx_ftoc(v.w);
		*p = c;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_QuatMultiply(AMX *amx, cell *params)
{
	cell *p;
	cell c;

	vec4 a(amx_ctof(params[5]), amx_ctof(params[6]), amx_ctof(params[7]), amx_ctof(params[8]));
	vec4 b(amx_ctof(params[9]), amx_ctof(params[10]), amx_ctof(params[11]), amx_ctof(params[12]));
	vec4 v = quat_multiply(a, b);
	v = quat_normalize(v);
	if (amx_GetAddr(amx, params[1], &p) == 0) {
		c = amx_ftoc(v.x);
		*p = c;
	}
	if (amx_GetAddr(amx, params[2], &p) == 0) {
		c = amx_ftoc(v.y);
		*p = c;
	}
	if (amx_GetAddr(amx, params[3], &p) == 0) {
		c = amx_ftoc(v.z);
		*p = c;
	}
	if (amx_GetAddr(amx, params[4], &p) == 0) {
		c = amx_ftoc(v.w);
		*p = c;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_SetDir(AMX *amx, cell *params)
{
	Object *rel;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	rel = ObjectTable::Get(params[8]);
	o->SetPosture(vec4(amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]), 1.0f),
	    vec4(amx_ctof(params[5]), amx_ctof(params[6]), amx_ctof(params[7]), 1.0f), rel);
	delete DelAnimation(o, TAG_ANMR);
	return 0;
}

/* LookAt(obj, target, ...): turns the object towards target and deletes its rotation animation. */
static cell AMX_NATIVE_CALL n_LookAt(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	Object *t = ObjectTable::Get(params[2]);
	if (t == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	Object *rel = ObjectTable::Get(params[6]);
	o->LookAt(t, vec4(amx_ctof(params[3]), amx_ctof(params[4]), amx_ctof(params[5]), 1.0f), rel,
	    vec4(amx_ctof(params[7]), amx_ctof(params[8]), amx_ctof(params[9]), 1.0f));
	delete DelAnimation(o, TAG_ANMR);
	return 0;
}

static cell AMX_NATIVE_CALL n_SetScale(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	o->SetScale(amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]));
	delete DelAnimation(o, TAG_ANMS);
	return 0;
}

static cell AMX_NATIVE_CALL n_GetScale(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	vec4 v = o->GetScale();
	cell *p;
	cell c;
	if (amx_GetAddr(amx, params[2], &p) == 0) {
		c = amx_ftoc(v.x);
		*p = c;
	}
	if (amx_GetAddr(amx, params[3], &p) == 0) {
		c = amx_ftoc(v.y);
		*p = c;
	}
	if (amx_GetAddr(amx, params[4], &p) == 0) {
		c = amx_ftoc(v.z);
		*p = c;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_SetLocalScale(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	o->SetLocalScale(amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]));
	return 0;
}

static cell AMX_NATIVE_CALL n_GetLocalScale(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	vec4 v = o->GetLocalScale();
	cell *p;
	cell c;
	if (amx_GetAddr(amx, params[2], &p) == 0) {
		c = amx_ftoc(v.x);
		*p = c;
	}
	if (amx_GetAddr(amx, params[3], &p) == 0) {
		c = amx_ftoc(v.y);
		*p = c;
	}
	if (amx_GetAddr(amx, params[4], &p) == 0) {
		c = amx_ftoc(v.z);
		*p = c;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_SetColor(AMX *amx, cell *params)
{
	float r, g, b, a;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	r = amx_ctof(params[2]);
	g = amx_ctof(params[3]);
	b = amx_ctof(params[4]);
	a = amx_ctof(params[5]);
	o->SetColor(r, g, b, a);
	delete DelAnimation(o, TAG_ANMC);
	return 0;
}

static cell AMX_NATIVE_CALL n_GetColor(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	vec4 v = o->GetColor();
	cell *p;
	cell c;
	if (amx_GetAddr(amx, params[2], &p) == 0) {
		c = amx_ftoc(v.x);
		*p = c;
	}
	if (amx_GetAddr(amx, params[3], &p) == 0) {
		c = amx_ftoc(v.y);
		*p = c;
	}
	if (amx_GetAddr(amx, params[4], &p) == 0) {
		c = amx_ftoc(v.z);
		*p = c;
	}
	if (amx_GetAddr(amx, params[5], &p) == 0) {
		c = amx_ftoc(v.w);
		*p = c;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_SetTexture(AMX *amx, cell *params)
{
	int tex;
	int mode;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (params[2] != 0)
		tex = params[2] ^ 0x89f974c3;
	else
		tex = 0;
	params[2] = tex;
	if (tex != 0 && tmCheck(tex) != 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	mode = -1;
	if (params[0] >= 16)
		mode = params[4];
	o->SetTexture(params[2], params[3], mode);
	return 0;
}

static cell AMX_NATIVE_CALL n_SetObject(AMX *amx, cell *params)
{
	Object *arg;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	arg = 0;
	if (params[2] != 0)
		arg = ObjectTable::Get(params[2]);
	o->SetObject(arg, params[3], params[4]);
	return 0;
}

static cell AMX_NATIVE_CALL n_SetModel(AMX *amx, cell *params)
{
	ModelData *d;
	Model *mo;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	mo = dynamic_cast<Model *>(o);
	if (mo == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	d = ModelTable::Get(params[2]);
	if (params[2] != 0 && d == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	mo->SetModel(d);
	return 0;
}

static cell AMX_NATIVE_CALL n_SetOptionI(AMX *amx, cell *params)
{
	cell *p;
	int n;
	int i;
	int *buf;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	n = (ucell)params[0] / sizeof(cell) - 2;
	buf = (int *)__builtin_alloca(n * sizeof(int));
	buf[0] = params[3];
	for (i = 1; i < n; i++) {
		if (amx_GetAddr(amx, params[i + 3], &p) != 0)
			return -1;
		buf[i] = *p;
	}
	return o->SetOption(params[2], *buf, n);
}

static cell AMX_NATIVE_CALL n_SetItemOptionI(AMX *amx, cell *params)
{
	cell *p;
	int n;
	int i;
	int *buf;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	n = (ucell)params[0] / sizeof(cell) - 3;
	buf = (int *)__builtin_alloca(n * sizeof(int));
	buf[0] = params[4];
	for (i = 1; i < n; i++) {
		if (amx_GetAddr(amx, params[i + 4], &p) != 0)
			return -1;
		buf[i] = *p;
	}
	return o->SetItemOption(params[3], params[2], *buf, n);
}

static cell AMX_NATIVE_CALL n_SetOptionF(AMX *amx, cell *params)
{
	cell *p;
	int n;
	int i;
	float *buf;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	n = (ucell)params[0] / sizeof(cell) - 2;
	buf = (float *)__builtin_alloca(n * sizeof(float));
	buf[0] = amx_ctof(params[3]);
	for (i = 1; i < n; i++) {
		if (amx_GetAddr(amx, params[i + 3], &p) != 0)
			return -1;
		buf[i] = amx_ctof(*p);
	}
	return o->SetOption(params[2], *buf, n);
}

static cell AMX_NATIVE_CALL n_SetItemOptionF(AMX *amx, cell *params)
{
	cell *p;
	int n;
	int i;
	float *buf;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	n = (ucell)params[0] / sizeof(cell) - 3;
	buf = (float *)__builtin_alloca(n * sizeof(float));
	buf[0] = amx_ctof(params[4]);
	for (i = 1; i < n; i++) {
		if (amx_GetAddr(amx, params[i + 4], &p) != 0)
			return -1;
		buf[i] = amx_ctof(*p);
	}
	return o->SetItemOption(params[3], params[2], *buf, n);
}

/* SetOptionS(obj, option, str): string variant of SetOption. */
static cell AMX_NATIVE_CALL n_SetOptionS(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	String s = CellToString(amx, params[3]);
	return o->SetOption(params[2], s);
}

/* SetItemOptionS(obj, item, option, str): string variant of SetItemOption. */
static cell AMX_NATIVE_CALL n_SetItemOptionS(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	String s = CellToString(amx, params[4]);
	return o->SetItemOption(params[3], params[2], s);
}

static cell AMX_NATIVE_CALL n_SetOptionP(AMX *amx, cell *params)
{
	cell *p;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	amx_GetAddr(amx, params[3], &p);
	if (params[0] == 3 * sizeof(cell))
		return o->SetOption(params[2], p, 0);
	if (params[4] <= 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return o->SetOption(params[2], p, params[4] * sizeof(cell));
}

static cell AMX_NATIVE_CALL n_SetItemOptionP(AMX *amx, cell *params)
{
	cell *p;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (amx_GetAddr(amx, params[4], &p) != 0)
		return -1;
	if (params[0] == 4 * sizeof(cell))
		return o->SetItemOption(params[3], params[2], p, 0);
	if (params[5] <= 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	return o->SetItemOption(params[3], params[2], p, params[5] * sizeof(cell));
}

static cell AMX_NATIVE_CALL n_GetOptionI(AMX *amx, cell *params)
{
	cell *p;
	cell r;
	int n;
	int i;
	int *buf;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	n = (ucell)params[0] / sizeof(cell) - 2;
	buf = (int *)__builtin_alloca(n * sizeof(int));
	r = o->GetOption(params[2], *buf, n);
	for (i = 0; i < n; i++) {
		if (amx_GetAddr(amx, params[3 + i], &p) == 0)
			*p = buf[i];
	}
	return r;
}

static cell AMX_NATIVE_CALL n_GetItemOptionI(AMX *amx, cell *params)
{
	cell *p;
	cell r;
	int n;
	int i;
	int *buf;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	n = (ucell)params[0] / sizeof(cell) - 3;
	buf = (int *)__builtin_alloca(n * sizeof(int));
	r = o->GetItemOption(params[3], params[2], *buf, n);
	for (i = 0; i < n; i++) {
		if (amx_GetAddr(amx, params[4 + i], &p) == 0)
			*p = buf[i];
	}
	return r;
}

static cell AMX_NATIVE_CALL n_GetOptionF(AMX *amx, cell *params)
{
	cell *p;
	cell c;
	cell r;
	int n;
	int i;
	float *buf;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	n = (ucell)params[0] / sizeof(cell) - 2;
	buf = (float *)__builtin_alloca(n * sizeof(float));
	r = o->GetOption(params[2], *buf, n);
	for (i = 0; i < n; i++) {
		if (amx_GetAddr(amx, params[3 + i], &p) == 0) {
			c = amx_ftoc(buf[i]);
			*p = c;
		}
	}
	return r;
}

static cell AMX_NATIVE_CALL n_GetItemOptionF(AMX *amx, cell *params)
{
	cell *p;
	cell c;
	cell r;
	int n;
	int i;
	float *buf;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	n = (ucell)params[0] / sizeof(cell) - 3;
	buf = (float *)__builtin_alloca(n * sizeof(float));
	r = o->GetItemOption(params[3], params[2], *buf, n);
	for (i = 0; i < n; i++) {
		if (amx_GetAddr(amx, params[4 + i], &p) == 0) {
			c = amx_ftoc(buf[i]);
			*p = c;
		}
	}
	return r;
}

/* GetOptionS(obj, option, buf, size): string variant of GetOption, truncated to size cells. */
static cell AMX_NATIVE_CALL n_GetOptionS(AMX *amx, cell *params)
{
	cell *dest;
	cell len, addr, end;
	int ok;
	int r;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	String s;
	r = o->GetOption(params[2], s);
	if (params[4] * 4 < s.length() + 1)
		s = String(s, 0, params[4] * 4 - 1);
	len = s.length();
	addr = params[3];
	end = len + addr;
	if (addr >= 0 && addr < amx->hea && end >= 0 && end < amx->hea)
		ok = 1;
	else
		ok = addr >= amx->stk && addr < amx->stp && end >= amx->stk && end < amx->stp;
	if (!ok) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[3], &dest);
	amx_SetString(dest, (char *)s.c_str(), 1);
	return r;
}

/* GetItemOptionS(obj, item, option, buf, size): string variant of GetItemOption. */
static cell AMX_NATIVE_CALL n_GetItemOptionS(AMX *amx, cell *params)
{
	cell *dest;
	cell len, addr, end;
	int ok;
	int r;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	String s;
	r = o->GetItemOption(params[3], params[2], s);
	if (params[5] * 4 < s.length() + 1)
		s = String(s, 0, params[5] * 4 - 1);
	len = s.length();
	addr = params[4];
	end = len + addr;
	if (addr >= 0 && addr < amx->hea && end >= 0 && end < amx->hea)
		ok = 1;
	else
		ok = addr >= amx->stk && addr < amx->stp && end >= amx->stk && end < amx->stp;
	if (!ok) {
		amx_RaiseError(amx, AMX_ERR_MEMACCESS);
		return 0;
	}
	amx_GetAddr(amx, params[4], &dest);
	amx_SetString(dest, (char *)s.c_str(), 1);
	return r;
}

static cell AMX_NATIVE_CALL n_GetOptionP(AMX *amx, cell *params)
{
	cell *p;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	amx_GetAddr(amx, params[3], &p);
	return o->GetOption(params[2], p, params[4] * sizeof(cell));
}

static cell AMX_NATIVE_CALL n_GetItemOptionP(AMX *amx, cell *params)
{
	cell *p;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	amx_GetAddr(amx, params[4], &p);
	return o->GetItemOption(params[3], params[2], p, params[5] * sizeof(cell));
}

/* AnimMoveLine: linear move from the current position. */
static cell AMX_NATIVE_CALL n_AnimMoveLine(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	float x = amx_ctof(params[2]);
	float y = amx_ctof(params[3]);
	float z = amx_ctof(params[4]);
	float duration = amx_ctof(params[5]);
	float delay = amx_ctof(params[6]);
	int mode = params[7];
	AnimMoveLine *a = new AnimMoveLine(o->GetPos(0, vec4(0.0f, 0.0f, 0.0f, 1.0f)), vec4(x, y, z, 1.0f), duration,
	    delay, mode, 0.0f, 0.0f);
	delete SetAnimation(o, TAG_ANMP, a);
	return 0;
}

/*
 * AnimMoveHermite: Hermite move from the current position. When an animation
 * is already attached, the start velocity is (GetPos - GetPrePos) * params[8].
 */
static cell AMX_NATIVE_CALL n_AnimMoveHermite(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	float x = amx_ctof(params[2]);
	float y = amx_ctof(params[3]);
	float z = amx_ctof(params[4]);
	float duration = amx_ctof(params[5]);
	float delay = amx_ctof(params[6]);
	int mode = params[7];
	float scale = amx_ctof(params[8]);
	Animation *old = GetAnimation(o, TAG_ANMP);
	vec4 vel(0.0f, 0.0f, 0.0f, 1.0f);
	if (old != 0) {
		vel = (o->GetPos(0, vec4(0.0f, 0.0f, 0.0f, 1.0f)) - o->GetPrePos(0, vec4(0.0f, 0.0f, 0.0f, 1.0f)))
		    * scale;
		delete old;
	}
	AnimMoveHermite *a = new AnimMoveHermite(o->GetPos(0, vec4(0.0f, 0.0f, 0.0f, 1.0f)), vec4(x, y, z, 1.0f), vel,
	    vec4(0.0f, 0.0f, 0.0f, 1.0f), duration, delay, mode);
	o->SetUserData(TAG_ANMP, a);
	return 0;
}

static cell AMX_NATIVE_CALL n_AnimRotate(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	float x = amx_ctof(params[2]);
	float y = amx_ctof(params[3]);
	float z = amx_ctof(params[4]);
	float duration = amx_ctof(params[5]);
	float delay = amx_ctof(params[6]);
	int mode = params[7];
	int mode2 = params[8];
	AnimRotate *a = new AnimRotate(o->GetRot(0, vec4(0.0f, 0.0f, 0.0f, 1.0f), 0), vec4(x, y, z, 1.0f), duration,
	    delay, mode, mode2);
	delete SetAnimation(o, TAG_ANMR, a);
	return 0;
}

/* AnimQuat: rotation from the current quaternion. */
static cell AMX_NATIVE_CALL n_AnimQuat(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	float x = amx_ctof(params[2]);
	float y = amx_ctof(params[3]);
	float z = amx_ctof(params[4]);
	float w = amx_ctof(params[5]);
	float duration = amx_ctof(params[6]);
	float delay = amx_ctof(params[7]);
	int mode = params[8];
	AnimQuat *a =
	    new AnimQuat(o->GetQuat(0, vec4(0.0f, 0.0f, 0.0f, 0.0f)), vec4(x, y, z, w), duration, delay, mode);
	delete SetAnimation(o, TAG_ANMR, a);
	return 0;
}

/* AnimScale: as AnimColor on the scale (the fourth component is 1.0). */
static cell AMX_NATIVE_CALL n_AnimScale(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	float x = amx_ctof(params[2]);
	float y = amx_ctof(params[3]);
	float z = amx_ctof(params[4]);
	float duration = amx_ctof(params[5]);
	float delay = amx_ctof(params[6]);
	int mode = params[7];
	AnimScale *a = new AnimScale(o->GetScale(), vec4(x, y, z, 1.0f), duration, delay, mode);
	delete SetAnimation(o, TAG_ANMS, a);
	return 0;
}

/* AnimColor: attaches a color animation to the object, deleting the previous one. */
static cell AMX_NATIVE_CALL n_AnimColor(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	float r = amx_ctof(params[2]);
	float g = amx_ctof(params[3]);
	float b = amx_ctof(params[4]);
	float al = amx_ctof(params[5]);
	float duration = amx_ctof(params[6]);
	float delay = amx_ctof(params[7]);
	int mode = params[8];
	AnimColor *a = new AnimColor(o->GetColor(), vec4(r, g, b, al), duration, delay, mode);
	delete SetAnimation(o, TAG_ANMC, a);
	return 0;
}

static cell AMX_NATIVE_CALL n_AnimModMove(AMX *amx, cell *params)
{
	Animation *a;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	vec4 v(amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]), 1.0f);
	a = GetAnimation(o, TAG_ANMP);
	if (a != 0)
		a->ResetTarget(v);
	else
		o->SetPos(v, 0);
	return 0;
}

static cell AMX_NATIVE_CALL n_AnimModScale(AMX *amx, cell *params)
{
	Animation *a;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	vec4 v(amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]), 1.0f);
	a = GetAnimation(o, TAG_ANMS);
	if (a != 0)
		a->ResetTarget(v);
	else
		o->SetScale(v);
	return 0;
}

static cell AMX_NATIVE_CALL n_AnimModColor(AMX *amx, cell *params)
{
	Animation *a;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	vec4 v(amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]), amx_ctof(params[5]));
	a = GetAnimation(o, TAG_ANMC);
	if (a != 0)
		a->ResetTarget(v);
	else
		o->SetColor(v);
	return 0;
}

static cell AMX_NATIVE_CALL n_AnimModQuat(AMX *amx, cell *params)
{
	Animation *a;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	vec4 v(amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]), amx_ctof(params[5]));
	a = GetAnimation(o, TAG_ANMR);
	if (a != 0)
		a->ResetTarget(v);
	else
		o->SetQuat(v, 0);
	return 0;
}

static cell AMX_NATIVE_CALL n_AnimModRotate(AMX *amx, cell *params)
{
	Animation *a;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	vec4 v(amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]), 1.0f);
	a = GetAnimation(o, TAG_ANMR);
	if (a != 0)
		a->ResetTarget(v);
	else
		o->SetRot(v, 0, 0);
	return 0;
}

/* AnimTimerManager(obj, ...): attaches a bare Animation (a timer) as 'ANMT', deleting the previous one. */
static cell AMX_NATIVE_CALL n_AnimTimerManager(AMX *amx, cell *params)
{
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (AmxSelf(amx)->scene != o->scene) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	float duration = amx_ctof(params[2]);
	float delay = amx_ctof(params[3]);
	int mode = params[4];
	Animation *a = new Animation(duration, delay, mode);
	delete SetAnimation(o, TAG_ANMT, a);
	return 0;
}

/* AnimDoTimer(obj, mode [, factor]): the factor defaults to 1.0. */
static cell AMX_NATIVE_CALL n_AnimDoTimer(AMX *amx, cell *params)
{
	Animation *a;
	float t = 1.0f;
	float r;

	if (params[0] >= 12)
		t = amx_ctof(params[3]);
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0)
		goto err;
	if (AmxSelf(amx)->scene != o->scene)
		goto err;
	switch (params[2]) {
	case 0:
		a = GetAnimation(o, TAG_ANMP);
		break;
	case 1:
		a = GetAnimation(o, TAG_ANMR);
		break;
	case 2:
		a = GetAnimation(o, TAG_ANMS);
		break;
	case 3:
		a = GetAnimation(o, TAG_ANMC);
		break;
	case 4:
		a = GetAnimation(o, TAG_ANMT);
		break;
	default:
err:
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (a != 0) {
		r = a->DoTimer(ANIM_FUNC_DEFAULT, 0.0f, 0.0f, t);
		return amx_ftoc(r);
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_AnimGetCurrTime(AMX *amx, cell *params)
{
	cell r;
	float t;
	Animation *a;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0)
		goto err;
	switch (params[2]) {
	case 0:
		a = GetAnimation(o, TAG_ANMP);
		break;
	case 1:
		a = GetAnimation(o, TAG_ANMR);
		break;
	case 2:
		a = GetAnimation(o, TAG_ANMS);
		break;
	case 3:
		a = GetAnimation(o, TAG_ANMC);
		break;
	case 4:
		a = GetAnimation(o, TAG_ANMT);
		break;
	default:
err:
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (a == 0) {
		r = 0;
	} else {
		t = a->GetCurrValue();
		r = amx_ftoc(t);
	}
	return r;
}

static cell AMX_NATIVE_CALL n_AnimGetState(AMX *amx, cell *params)
{
	Animation *a;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	switch (params[2]) {
	case 0:
		a = GetAnimation(o, TAG_ANMP);
		break;
	case 1:
		a = GetAnimation(o, TAG_ANMR);
		break;
	case 2:
		a = GetAnimation(o, TAG_ANMS);
		break;
	case 3:
		a = GetAnimation(o, TAG_ANMC);
		break;
	case 4:
		a = GetAnimation(o, TAG_ANMT);
		break;
	default:
		amx_RaiseError(amx, 10);
		return 0;
	}
	if (a == 0)
		return 0;
	switch (a->m_state) {
	case ANIM_STATE_DELAY:
		return 1;
	case ANIM_STATE_RUN:
		return 2;
	case ANIM_STATE_DONE:
		return 0;
	default:
		return 0;
	}
}

static cell AMX_NATIVE_CALL n_Distance(AMX *amx, cell *params)
{
	float d;
	Object *a = ObjectTable::Get(params[1]);
	if (a == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	Object *b = ObjectTable::Get(params[2]);
	if (b == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	vec4 pa = a->GetPos(0, vec4(0.0f, 0.0f, 0.0f, 1.0f));
	vec4 pb = b->GetPos(0, vec4(0.0f, 0.0f, 0.0f, 1.0f));
	d = length(pa - pb);
	return amx_ftoc(d);
}

static cell AMX_NATIVE_CALL n_DistanceXYZ(AMX *amx, cell *params)
{
	vec4 a(amx_ctof(params[1]), amx_ctof(params[2]), amx_ctof(params[3]), 1.0f);
	vec4 b(amx_ctof(params[4]), amx_ctof(params[5]), amx_ctof(params[6]), 1.0f);
	float d = length(a - b);
	return amx_ftoc(d);
}

static cell AMX_NATIVE_CALL n_RealVelocity(AMX *amx, cell *params)
{
	float d;
	Object *o = ObjectTable::Get(params[1]);
	if (o == 0) {
		amx_RaiseError(amx, 10);
		return 0;
	}
	vec4 pos = o->GetPos(0, vec4(0.0f, 0.0f, 0.0f, 1.0f));
	vec4 pre = o->GetPrePos(0, vec4(0.0f, 0.0f, 0.0f, 1.0f));
	d = length(pos - pre);
	return amx_ftoc(d);
}

/* Normalize(&x, &y, &z): normalizes the vector in place (0.0 for an invalid address). */
static cell AMX_NATIVE_CALL n_Normalize(AMX *amx, cell *params)
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
	cell *p;
	cell *q;
	cell c;

	if (amx_GetAddr(amx, params[1], &p) == 0)
		x = amx_ctof(*p);
	if (amx_GetAddr(amx, params[2], &p) == 0)
		y = amx_ctof(*p);
	if (amx_GetAddr(amx, params[3], &p) == 0)
		z = amx_ctof(*p);
	vec4 v(x, y, z, 1.0f);
	v = normalize(v);
	if (amx_GetAddr(amx, params[1], &q) == 0) {
		c = amx_ftoc(v.x);
		*q = c;
	}
	if (amx_GetAddr(amx, params[2], &q) == 0) {
		c = amx_ftoc(v.y);
		*q = c;
	}
	if (amx_GetAddr(amx, params[3], &q) == 0) {
		c = amx_ftoc(v.z);
		*q = c;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_FontID(AMX *amx, cell *params)
{
	return (cell)Font::Get(params[1]);
}

AMX_NATIVE_INFO smallext_3d[] = {
	{ "Self", n_Self },
	{ "Parent", n_Parent },
	{ "Scene", n_Scene },
	{ "Camera", n_Camera },
	{ "Disp2D", n_Disp2D },
	{ "IsValidObject", n_IsValidObject },
	{ "CreateNull", n_CreateNull },
	{ "CreateLight", n_CreateLight },
	{ "CreateCube", n_CreateCube },
	{ "CreatePlane", n_CreatePlane },
	{ "CreatePlane3x3", n_CreatePlane },
	{ "CreateParticle", n_CreateParticle },
	{ "CreateLabel", n_CreateLabel },
	{ "CreateTextbox", n_CreateTextbox },
	{ "CreateHermite", n_CreateHermite },
	{ "CreateCrystal", n_CreateCrystal },
	{ "CreateDefocus", n_CreateDefocus },
	{ "CreateCloud", n_CreateCloud },
	{ "CreateEdit", n_CreateEdit },
	{ "CreateEditML", n_CreateEditML },
	{ "CreatePrim", n_CreateCube },
	{ "CreateIcon", n_CreateIcon },
	{ "CreateSScope0", n_CreateSScope0 },
	{ "CreateModel", n_CreateModel },
	{ "CreateMesh", n_CreateMesh },
	{ "CreateStar", n_CreateNull },
	{ "Destroy", n_Destroy },
	{ "TimerDie", n_TimerDie },
	{ "Show", n_Show },
	{ "Hide", n_Hide },
	{ "ShowChildren", n_ShowChildren },
	{ "HideChildren", n_HideChildren },
	{ "SetScreen", n_SetScreen },
	{ "GetScreen", n_GetScreen },
	{ "SetClipRange", n_SetClipRange },
	{ "GetClipRange", n_GetClipRange },
	{ "SetAmbient", n_SetAmbient },
	{ "SetLightColor", n_SetLightColor },
	{ "SetLightVector", n_SetLightVector },
	{ "SetFogColor", n_SetFogColor },
	{ "SetFogRange", n_SetFogRange },
	{ "SetPos", n_SetPos },
	{ "GetPos", n_GetPos },
	{ "GetOffPos", n_GetOffPos },
	{ "SetRot", n_SetRot },
	{ "GetRot", n_GetRot },
	{ "SetQuat", n_SetQuat },
	{ "GetQuat", n_GetQuat },
	{ "LookAt", n_LookAt },
	{ "SetDir", n_SetDir },
	{ "QuatInterp", n_QuatInterp },
	{ "QuatMultiply", n_QuatMultiply },
	{ "SetScale", n_SetScale },
	{ "GetScale", n_GetScale },
	{ "SetLocalScale", n_SetLocalScale },
	{ "GetLocalScale", n_GetLocalScale },
	{ "SetColor", n_SetColor },
	{ "GetColor", n_GetColor },
	{ "SetOptionI", n_SetOptionI },
	{ "SetOptionF", n_SetOptionF },
	{ "SetOptionS", n_SetOptionS },
	{ "SetOptionP", n_SetOptionP },
	{ "GetOptionI", n_GetOptionI },
	{ "GetOptionF", n_GetOptionF },
	{ "GetOptionS", n_GetOptionS },
	{ "GetOptionP", n_GetOptionP },
	{ "SetItemOptionI", n_SetItemOptionI },
	{ "SetItemOptionF", n_SetItemOptionF },
	{ "SetItemOptionS", n_SetItemOptionS },
	{ "SetItemOptionP", n_SetItemOptionP },
	{ "GetItemOptionI", n_GetItemOptionI },
	{ "GetItemOptionF", n_GetItemOptionF },
	{ "GetItemOptionS", n_GetItemOptionS },
	{ "GetItemOptionP", n_GetItemOptionP },
	{ "SetTexture", n_SetTexture },
	{ "SetObject", n_SetObject },
	{ "SetModel", n_SetModel },
	{ "InitNodeArray", n_InitNodeArray },
	{ "SetNodePos", n_SetNodePos },
	{ "GetPosHermite", n_GetPosHermite },
	{ "AnimMoveHermite", n_AnimMoveHermite },
	{ "AnimMoveLine", n_AnimMoveLine },
	{ "AnimRotate", n_AnimRotate },
	{ "AnimQuat", n_AnimQuat },
	{ "AnimScale", n_AnimScale },
	{ "AnimColor", n_AnimColor },
	{ "AnimModMove", n_AnimModMove },
	{ "AnimModScale", n_AnimModScale },
	{ "AnimModColor", n_AnimModColor },
	{ "AnimModQuat", n_AnimModQuat },
	{ "AnimModRotate", n_AnimModRotate },
	{ "AnimTimerManager", n_AnimTimerManager },
	{ "AnimDoTimer", n_AnimDoTimer },
	{ "AnimGetCurrTime", n_AnimGetCurrTime },
	{ "AnimGetState", n_AnimGetState },
	{ "Distance", n_Distance },
	{ "DistanceXYZ", n_DistanceXYZ },
	{ "RealVelocity", n_RealVelocity },
	{ "Normalize", n_Normalize },
	{ "FontID", n_FontID },
	{ 0, 0 },
};

AMX_NATIVE_INFO smallext_3d_p[] = {
	{ "CreateLabelShadow", n_CreateLabelShadow },
	{ "CreateEditML_P", n_CreateEditML_P },
	{ "CalcNumLine", n_CalcNumLine },
	{ 0, 0 },
};
