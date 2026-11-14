/*
 * Camera: scene object holding the view, the projection and the viewport.
 */

#ifndef CAMERA_H
#define CAMERA_H

#include "gfx/vec_types.h"
#include "object.h"

class gm_graph;

/* Sets up the view, projection and viewport of the scene and clears the background. */
class Camera : public Object {
public:
	/* 0x250 */ mat4 view;  /* view basis: world_mat with m0 and m2 negated */
	/* 0x290 */ mat4 view_inv;  /* its inverse, the modelview matrix */
	/* 0x2D0 */ vec4 bg_color;  /* background color */
	/* 0x2E0 */ int flag_back;  /* clear to bg_color (option BACK) */
	/* 0x2E4 */ int screen_x;  /* viewport x */
	/* 0x2E8 */ int screen_y;  /* viewport y */
	/* 0x2EC */ int screen_w;  /* viewport width */
	/* 0x2F0 */ int screen_h;  /* viewport height */
	/* 0x2F4 */ float vscreen_w;  /* virtual screen width */
	/* 0x2F8 */ float vscreen_h;  /* virtual screen height */
	/* 0x2FC */ float vscreen_z;  /* distance of the virtual screen */
	/* 0x300 */ float tan_half_fov_y;  /* tangent of half the vertical field of view */
	/* 0x304 */ float clip_near;
	/* 0x308 */ float clip_far;
	/* 0x30C */ int tex_lod_offset;  /* texture LOD offset, from the field of view */
	/* 0x310 */ int tex_lod_bias;  /* texture LOD bias */

	Camera(Object *parent);
	virtual ~Camera();

	virtual int SetOption(int option, const int &value, unsigned int size);
	virtual int SetOption(int option, const float &value, unsigned int size);
	virtual int SetOption(int option, const String &value);

	void ViewSetup(gm_graph &g);
	void SetViewport(int x, int y, int w, int h);
	void GetViewport(int &x, int &y, int &w, int &h);
	void SetBGColor(const vec4 &c);
	void SetVirtualScreen(float w, float h, float z);
	void GetVirtualScreen(float &w, float &h, float &z);
	void SetClipRange(float near_, float far_);
	void GetClipRange(float &near_, float &far_);
	float GetPixelAdjustDepth(float value);
	float GetPixelAdjustScale(float value);
};

#endif
