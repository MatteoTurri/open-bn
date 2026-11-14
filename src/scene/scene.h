/*
 * Scene: root node of the 3D scene graph (lights, depth-sorted draw layers).
 */

#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <list>
#include <algorithm>
#include "gfx/vec_types.h"
#include "object.h"

class Light;
class Camera;
class gm_graph;

/* Per-light state kept by Scene. */
struct light_state {
	/* 0x00 */ bool enabled;
	/* 0x04 */ int pad_04[3];
	/* 0x10 */ vec4 color;
	/* 0x20 */ vec4 vector;
};

class Scene : public Object {
public:
	/* Drawable node: object and depth key. */
	struct node {
		/* 0x00 */ Object *obj;
		/* 0x04 */ float key;
	};
	friend bool operator<(const node &a, const node &b) { return a.key < b.key; }

	Scene();
	~Scene();

	void DrawScene(gm_graph &g);
	void ClearLights(int no);
	void sort_and_draw(gm_graph &g);
	void sort_objs(Object *root, vector<node> &out);
	void draw_objs(gm_graph &g);

	const Camera *GetCamera() const;
	void SetPacketFence(int size, int minFree);
	void SetPixelOffset(float x, float y);
	void UpdateScene(gm_graph &g);
	void RefCamera(Camera *camera);
	void SetFogColor(const vec4 &color);
	const vec4 &GetFogColor() const;
	void SetFogRange(float near, float far);
	void GetFogRange(float &near, float &far) const;
	void SetAmbient(const vec4 &color);
	const vec4 &GetAmbient() const;
	void ActivateLightState(gm_graph &g, int no);
	void SetLightColor(int no, const vec4 &color);
	void SetLightVector(int no, const vec4 &vector);
	const vec4 &GetLightColor(int no) const;
	const vec4 &GetLightVector(int no) const;
	int AppendLight(Light *light, int no);
	void RemoveLight(Light *light, int no);
	void RemoveLight(int index, int no);
	int GetLightCount(int no) const;
	Light *GetLight(int index, int no) const;
	int FindLight(Light *light, int no) const;

	/* 0x250 */ int packet_fence_size;
	/* 0x254 */ int packet_fence_min_free;
	/* 0x258 */ float pixel_offset_x;
	/* 0x25C */ float pixel_offset_y;
	/* 0x260 */ Camera *camera;
	/* 0x264 */ int pad_264[3];
	/* 0x270 */ light_state light_states[4];
	/* 0x330 */ vector<Light *> lights[8];
	/* 0x390 */ mat4 saved_matrix;  /* graph matrix 1, copied at the start of sort_and_draw */
	/* 0x3D0 */ list<vector<node> > layers;  /* one vector of nodes per layer */
	/* 0x3D4 */ _List_node<vector<node> > *front_node_cache; /* list node before which kind 4 layers are inserted */
	/* 0x3D8 */ vector<node> staging; /* empty vector copied into every new layer */
};

#endif
