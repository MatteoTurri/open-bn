/*
 * Object: node of the scene graph, base of every scene and widget object.
 * Interface header: object.cpp emits the inline members.
 */

#ifndef OBJECT_H
#define OBJECT_H

#pragma interface

#include <vector>
#include <string>
#include "app/bnstring.h"
#include "gfx/vec_types.h"
#include "hook.h"
#include "effoptionstate.h"

class Object;
class Scene;
class gm_graph;

typedef vector<Object *> ObjVec;

/* Values of zsort_mode (option 'ZSRT', 0..5) that Scene::sort_objs acts on; with the others the object
   goes into the layer of its parent. */
#define ZSORT_NONE 0
#define ZSORT_OVERLAY 3  /* a new layer drawn after the others */
#define ZSORT_BACKGROUND 4  /* a new layer drawn before the first one */

/*
 * A node with its children, its local and world transforms, color, z-sort,
 * references to other objects and the generic option interface (options are
 * four-character ids).
 */
class Object : public Hook {
public:
	/* vptr at 0x10, after the Hook data */
	/* 0x14 */ Object *parent;
	/* 0x18 */ Object *scene;  /* the root of the graph, a Scene */
	/* 0x1C */ ObjVec children;  /* begin, end, capacity */
	/* 0x28 */ int zsort_mode;  /* layer of Scene::sort_objs: ZSORT_* */
	/* 0x2C */ float zsort_offset;  /* added to the depth key */
	/* 0x30 */ float zsort_value;  /* camera-space depth, written by Scene::sort_objs */
	/* 0x34 */ float sphere_radius;  /* of the culling sphere, <= 0 disables culling */
	/* 0x38 */ int sphere_visibility;  /* 1 if the culling sphere passed the frustum test */
	/* 0x3C */ bool visible;
	/* 0x40 */ int children_visible;
	/* 0x44 */ int debug;  /* option 'DEBG', 0 or 1 */
	/* 0x48 */ int color_enable;  /* of the root scene: when 0, GetWorldColor returns the local color */
	/* 0x4C */ int suicide_timer;
	/* 0x50 */ int flip;  /* option 'FLIP' */
	/* 0x54 */ int blend_mode;  /* option 'ALPH', for gm_graph::BlendFunc */
	/* 0x58 */ int pad_58[2];
	/* 0x60 */ vec4 pos;  /* world position */
	/* 0x70 */ vec4 prepos;  /* previous position */
	/* 0x80 */ vec4 scale;
	/* 0x90 */ vec4 local_scale;
	/* 0xA0 */ mat4 local_rot;
	/* 0xE0 */ mat4 world_rot;  /* rows 0..2 are the X/Y/Z axes */
	/* 0x120 */ mat4 local_mat;
	/* 0x160 */ mat4 world_mat;  /* row 3 is the world position */
	/* 0x1A0 */ mat4 world_inv;
	/* 0x1E0 */ vec4 color;
	/* 0x1F0 */ int local_dirty;
	/* 0x1F4 */ int world_dirty;
	/* 0x1F8 */ ObjVec *reference;  /* objects referenced by this one */
	/* 0x1FC */ ObjVec *referenced_by;  /* objects referencing this one */
	/* 0x200 */ EffOptionState option_state;

	Object(Object *parent);

	virtual ~Object();
	virtual void Draw(gm_graph &g);
	virtual void DrawThis(gm_graph &g);
	virtual void Anim();
	virtual void SetTexture(int texture, int option, int mode);
	virtual void SetObject(Object *obj, int option, int mode);
	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const String &value);
	virtual int SetOption(int option, const void *data, unsigned int size);
	virtual int GetOption(int option, int &value, unsigned int size) const;
	virtual int GetOption(int option, float &value, unsigned int size) const;
	virtual int GetOption(int option, String &value) const;
	virtual int GetOption(int option, void *data, unsigned int size) const;
	virtual int SetItemOption(int item, int option, const int &value, unsigned int size);
	virtual int SetItemOption(int item, int option, const float &value, unsigned int size);
	virtual int SetItemOption(int item, int option, const String &value);
	virtual int SetItemOption(int item, int option, const void *data, unsigned int size);
	virtual int GetItemOption(int item, int option, int &value, unsigned int size) const;
	virtual int GetItemOption(int item, int option, float &value, unsigned int size) const;
	virtual int GetItemOption(int item, int option, String &value) const;
	virtual int GetItemOption(int item, int option, void *data, unsigned int size) const;

	mat4 &UpdateMatrix();
	vec4 GetPos(Object *o, const vec4 &v);
	vec4 GetPrePos(Object *o, const vec4 &v);
	vec4 GetRot(Object *o, const vec4 &v, int mode);
	vec4 GetQuat(Object *o, const vec4 &q);
	void SetQuat(const vec4 &q, Object *o);
	void SetQuat(float x, float y, float z, float w, Object *o);
	void SetPos(float x, float y, float z, Object *o);
	void SetPos(const vec4 &v, Object *o);
	void SetPrePos(float x, float y, float z, Object *o);
	void SetPrePos(const vec4 &v, Object *o);
	void SetRot(float x, float y, float z, Object *o, int mode);
	void SetRot(const vec4 &v, Object *o, int mode);
	void LookAt(const vec4 &target, const vec4 &up);
	void LookAt(Object *targetObj, const vec4 &target, Object *upObj, const vec4 &up);
	void SetPosture(const vec4 &target, const vec4 &up, Object *o);
	void DrawChildren(gm_graph &g);
	void AnimChildren();
	void SetColor(float r, float g, float b, float a);
	void SetColor(const vec4 &c);
	const vec4 &GetColor() const;
	const vec4 &GetWorldColor() const;
	void SetScale(float x, float y, float z);
	void SetScale(const vec4 &s);
	const vec4 &GetScale();
	void SetLocalScale(float x, float y, float z);
	void SetLocalScale(const vec4 &s);
	const vec4 &GetLocalScale();
	void SetLocalDirty();
	void SetWorldDirty();
	int CreateReference(Object *obj);
	void DeleteReference(int no);
	Object *GetReference(int no) const;
	void CleanupReference();
	Object *GetParent() const { return parent; }
	Scene *GetScene() const { return (Scene *)scene; }
	int GetNumChildren() const { return children.end() - children.begin(); }
	Object *GetChild(int no) const
	{
		Object *const *child = children.begin() + no;
		return *child;
	}
	void Show() { visible = 1; }
	void Hide() { visible = 0; }
	void ShowChildren() { children_visible = 1; }
	void HideChildren() { children_visible = 0; }
	int IsVisible() { return visible; }
	int IsChildrenVisible() { return children_visible; }
	void SetZSortMode(int mode) { zsort_mode = mode; }
	void SetZSortOffset(float offset) { zsort_offset = offset; }
	void SetZSortValue(float value) { zsort_value = value; }
	int GetZSortMode() const { return zsort_mode; }
	float GetZSortOffset() const { return zsort_offset; }
	float GetZSortValue() const { return zsort_value; }
	void SetSphereRadius(float radius) { sphere_radius = radius; }
	float GetSphereRadius() const { return sphere_radius; }
	void SetSphereVisibility(bool visibility) { sphere_visibility = visibility; }
	int GetSphereVisibility() const { return sphere_visibility; }
	static int InstanceCount() { return instance_count; }
	const mat4 &GetWorldMatrix() const { return world_mat; }
	const mat4 &GetWorldRotMatrix() const { return world_rot; }
	const mat4 &GetWorldInvMatrix() const { return world_inv; }
	const vec4 &GetWorldPos() const { return world_mat.m3; }
	const vec4 &GetWorldAxisX() const { return world_rot.m0; }
	const vec4 &GetWorldAxisY() const { return world_rot.m1; }
	const vec4 &GetWorldAxisZ() const { return world_rot.m2; }
	int clamp(int val, int min, int max)
	{
		int v = (val < min) ? min : val;
		return (max < v) ? max : v;
	}
	void ActivateOptionState(int no) { option_state.Activate(no); }
	EffOptionState *GetOptionState() { return &option_state; }
	const EffOptionState *GetOptionState() const { return &option_state; }
	void SetSuicideTimer(int time);
	int CountDownSuicide();

	static int instance_count;
	static int upmat_count_L;
	static int upmat_count_W;
}; /* size = 0x250 */

#endif
