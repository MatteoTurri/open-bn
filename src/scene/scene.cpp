/*
 * Scene: root object of the 3D scene. Owns the lights and, every frame, sorts
 * the object tree by depth into layers and draws it.
 */

#include <iostream.h>
#include "gfx/vec.h"
#include <vector>
#include <list>
#include <algorithm>
#include "object.h"
#include "camera.h"
#include "gfx/gm_graph.h"
#include "scene.h"
#include "scenefacade.h"
#include <math.h>

/* Unit matrix and unit w row: the file-scope objects built at startup. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

static int sort_objs_accept(Object *o, const vec4 &p);

static int sphere_tangents(float z, float x, float r, float *t0, float *t1);

Scene::Scene() : Object(0)
{
	packet_fence_size = 0;
	packet_fence_min_free = 0;
	pixel_offset_x = 0.0f;
	pixel_offset_y = 0.0f;
	camera = 0;
	SetAmbient(vec4(0.5f, 0.5f, 0.5f, 1.0f));
	SetLightColor(0, vec4(1.0f, 1.0f, 1.0f, 0.0f));
	SetLightColor(1, vec4(1.0f, 1.0f, 1.0f, 0.0f));
	SetLightColor(2, vec4(0.0f, 0.0f, 0.0f, 0.0f));
	SetLightColor(3, vec4(0.0f, 0.0f, 0.0f, 0.0f));
	SetLightVector(0, vec4(-1.0f, -1.0f, 1.0f, 1.0f));
	SetLightVector(1, vec4(1.0f, 1.0f, -1.0f, 1.0f));
	SetLightVector(2, vec4(-1.0f, 1.0f, -1.0f, 1.0f));
	SetLightVector(3, vec4(1.0f, -1.0f, 1.0f, 1.0f));
}

const Camera *Scene::GetCamera() const
{
	return camera;
}

void Scene::SetPacketFence(int size, int minFree)
{
	packet_fence_size = size;
	packet_fence_min_free = minFree;
}

void Scene::SetPixelOffset(float x, float y)
{
	pixel_offset_x = x;
	pixel_offset_y = y;
}

void Scene::UpdateScene(gm_graph &g)
{
	g.MatrixMode(GM_MATRIX_MODEL);
	g.LoadIdentity();
	DrawChildren(g);
}

void Scene::RefCamera(Camera *camera)
{
	this->camera = camera;
}

void Scene::SetFogColor(const vec4 &color)
{
	option_state.SetFogColor(color);
}

const vec4 &Scene::GetFogColor() const
{
	return *option_state.GetFogColor();
}

void Scene::SetFogRange(float near, float far)
{
	option_state.SetFogRange(near, far);
}

void Scene::GetFogRange(float &near, float &far) const
{
	option_state.GetFogRange(near, far);
}

void Scene::SetAmbient(const vec4 &color)
{
	option_state.SetAmbient(color);
}

const vec4 &Scene::GetAmbient() const
{
	return *option_state.GetAmbient();
}

void Scene::ActivateLightState(gm_graph &g, int no)
{
	if ((unsigned int)no < 4) {
		light_state &s = light_states[no];
		bool on = s.enabled;
		g.EnableLight(no, on);
		if (on)
			g.Light(no, GM_LIGHT_DIRECTIONAL, s.color, s.vector);
	}
}

void Scene::SetLightColor(int no, const vec4 &color)
{
	if ((unsigned int)no < 4) {
		light_states[no].color = color;
		light_states[no].enabled = color.x + color.y + color.z != 0.0f;
	}
}

void Scene::SetLightVector(int no, const vec4 &vector)
{
	if ((unsigned int)no < 4)
		light_states[no].vector = vector;
}

const vec4 &Scene::GetLightColor(int no) const
{
	static vec4 def(0.0f, 0.0f, 0.0f, 0.0f);
	if ((unsigned int)no >= 4)
		return def;
	return light_states[no].color;
}

const vec4 &Scene::GetLightVector(int no) const
{
	static vec4 def(0.0f, -1.0f, 0.0f, 0.0f);
	if ((unsigned int)no >= 4)
		return def;
	return light_states[no].vector;
}

int Scene::AppendLight(Light *light, int no)
{
	if ((unsigned int)no >= 8)
		return -1;
	vector<Light *> &v = lights[no];
	int n = v.size();
	v.push_back(light);
	return n;
}

void Scene::RemoveLight(Light *light, int no)
{
	int idx = FindLight(light, no);
	RemoveLight(no, idx);  /* NOTE: bug: arguments swapped, the signature is (index, no) */
}

void Scene::RemoveLight(int index, int no)
{
	if ((unsigned int)no >= 8)
		return;
	vector<Light *> &v = lights[no];
	if (index < 0)
		return;
	if (index >= (int)v.size())
		return;
	v.erase(v.begin() + index);
}

int Scene::GetLightCount(int no) const
{
	if ((unsigned int)no >= 8)
		return 0;
	const vector<Light *> &v = lights[no];
	return v.size();
}

Light *Scene::GetLight(int index, int no) const
{
	if ((unsigned int)no >= 8)
		return 0;
	const vector<Light *> &v = lights[no];
	if (index < 0 || index >= (int)v.size())
		return 0;
	return v[index];
}

int Scene::FindLight(Light *light, int no) const
{
	if ((unsigned int)no >= 8)
		return 0;
	const vector<Light *> &v = lights[no];
	int count = v.size();
	for (int i = 0; i < count; i++) {
		if (v[i] == light)
			return i;
	}
	return -1;
}

Scene::~Scene()
{
	ClearLights(-1);
}

void Scene::DrawScene(gm_graph &g)
{
	g.SetPacketFence(packet_fence_size, packet_fence_min_free);
	g.SetPixelOffset(pixel_offset_x, pixel_offset_y);
	g.Start();
	g.ShadeModel(1);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_DITHER);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_ANTIALIAS);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_CULL_FACE);
	g.FrontFace(1);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_BLEND);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_VERTEX_BLEND);
	g.SetEnableBits(g.fFlagsE | GM_ENABLE_TEXTURE);
	g.BlendFunc(GM_BLEND_ALPHA);
	g.SetEnableBits(g.fFlagsE & ~GM_ENABLE_TEXTURE_FIT);
	camera->ViewSetup(g);
	g.SetColorBuffer(-1);
	g.LoadIdentity(GM_MATRIX_MODEL);
	sort_and_draw(g);
	g.Finish();
	g.SetPacketFence(0, 0);
	g.SetPixelOffset(0.0f, 0.0f);
}

void Scene::ClearLights(int no)
{
	if (no < 0) {
		for (int i = 0; i < 8; i++)
			lights[i].clear();
		return;
	}
	if (no < 8)
		lights[no].clear();
}

void Scene::sort_and_draw(gm_graph &g)
{
	saved_matrix = *g.GetMatrix(GM_MATRIX_VIEW);

	layers.clear();
	layers.push_front(staging);
	front_node_cache = layers.begin()._M_node;

	sort_objs(this, *layers.begin());
	draw_objs(g);
}

/* Recursive walk of the object tree that fills the layers with {object, depth
   key} nodes. ZSORT_OVERLAY opens a new layer at the back of the list,
   ZSORT_BACKGROUND inserts one before front_node_cache; children go into the
   layer of their parent. Visible objects with sphere_radius > 0 are culled
   with sort_objs_accept. */
void Scene::sort_objs(Object *root, vector<node> &out)
{
	vector<node> *target = &out;

	if (root->zsort_mode != ZSORT_NONE) {  /* redundant as written */
		switch (root->zsort_mode) {
		case ZSORT_OVERLAY:
			layers.push_back(staging);
			target = &layers.back();
			break;
		case ZSORT_BACKGROUND:
			target = &*layers.insert(list<vector<node> >::iterator(front_node_cache), staging);
			break;
		}
	}

	if (root->visible) {
		vec4 p = apply(saved_matrix, root->world_mat.m3, 1.0f);
		root->zsort_value = p.z;
		if (root->sphere_radius <= 0.0f)
			goto noCull;
		if (!sort_objs_accept(root, p))
			goto children;
noCull:
		{
			float bias = root->zsort_offset;
			node n;
			n.obj = root;
			n.key = p.z + bias;
			target->push_back(n);
		}
	}
children:
	if (root->children_visible) {
		int cnt = root->children.end() - root->children.begin();

		if (cnt) {
			int i;

			for (i = 0; i < cnt; i++) {
				Object **pp = root->children.begin() + i;

				sort_objs(*pp, *target);
			}
		}
	}
}

void Scene::draw_objs(gm_graph &g)
{
	option_state.Activate(-1);
	int accum = 0;
	int ce = 0;
	if (color.w != 1.0f || color.x != 1.0f || color.y != 1.0f || color.z != 1.0f)
		ce = 1;
	color_enable = ce;

	list<vector<node> >::iterator lit;
	for (lit = layers.begin(); lit != layers.end(); lit++) {
		vector<node> &v = *lit;
		sort(v.begin(), v.end());
		vector<node>::iterator it;
		for (it = v.begin(); it != v.end(); ++it) {
			Object *obj = it->obj;
			accum = obj->option_state.Activate(accum);
			g.LoadMatrix(GM_MATRIX_MODEL, obj->world_mat);
			g.Scale(obj->GetLocalScale());
			if (!color_enable) {
				g.Color(obj->GetColor());
			} else {
				const vec4 &oc = obj->GetColor();
				const vec4 &sc = GetColor();
				vec4 tmp(oc.x * sc.x, oc.y * sc.y, oc.z * sc.z, oc.w * sc.w);
				g.Color(tmp);
			}
			obj->DrawThis(g);
		}
	}
}

/* Culls the sphere of radius o->sphere_radius centred on p (camera space)
   against the projection matrix; sets o->sphere_visibility when it is kept. */
static int sort_objs_accept(Object *o, const vec4 &p)
{
	const mat4 *m;
	float r;
	float z;
	float t0, t1, t2, t3;
	float sx, sy;

	o->sphere_visibility = 0;
	m = SceneGetGraph()->GetMatrix(GM_MATRIX_PROJECTION);
	r = o->sphere_radius;
	z = p.z - r;
	if (z >= 0.0f)
		return 0;
	if (m->m2.z + m->m3.z / z >= 1.0f)
		return 0;
	z = p.z + r;
	if (z != 0.0f) {
		if (m->m2.z + m->m3.z / z <= -1.0f)
			return 0;
	}
	if (!sphere_tangents(p.z, p.x, r, &t0, &t1))
		return 1;
	if (!sphere_tangents(p.z, -p.y, r, &t2, &t3))
		return 1;
	sx = m->m0.x;
	sy = m->m1.y;
	if (t0 * sx > 1.0f || t1 * sx < -1.0f || t2 * sy > 1.0f || t3 * sy < -1.0f)
		return 0;
	o->sphere_visibility = 1;
	return 1;
}

/* Slopes of the two tangents from the point (z, x) to the circle of radius r
   centred on the origin; returns 0 if the point is inside or there is none. */
static int sphere_tangents(float z, float x, float r, float *t0, float *t1)
{
	float zx = z * x;
	float zz = z * z - r * r;
	float xx = x * x - r * r;
	float d;

	if (zz <= 0.0f)
		return 0;
	d = zx * zx - zz * xx;
	if (d <= 0.0f)
		return 0;
	d = sqrtf(d);
	zz = 1.0f / zz;
	*t0 = (-zx - d) * zz;
	*t1 = (-zx + d) * zz;
	return 1;
}
