/*
 * Camera methods: options, viewport and virtual screen, and ViewSetup, which
 * loads the projection and view matrices and clears the screen.
 */

#include <vector>
#include "gfx/vec.h"
#include <math.h>
#include "gfx/gm_graph.h"
#include "scene.h"
#include "scene/optid.h"

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

extern const mat4 &mat4_identity();

/* Local copies of inverse() (Gauss-Jordan; identity if singular) and mat4_perspective_tan() from vec.h. */
static inline mat4 cam_inverse(const mat4 &m)
{
	float a[4][4], b[4][4];
	float piv = 0.0f;
	int i, j, k;
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++) {
			a[i][j] = ((const float *)((const char *)&m + (i << 4)))[j];
			b[i][j] = (i == j) ? 1.0f : 0.0f;
		}
	for (i = 0; i < 4; i++) {
		for (j = i; j < 4; j++) {
			piv = a[j][i];
			if (piv != 0.0)
				break;
		}
		if (j == 4)
			return mat4_identity();
		if (j != i)
			for (k = 0; k < 4; k++) {
				float t;
				t = a[i][k];
				a[i][k] = a[j][k];
				a[j][k] = t;
				t = b[i][k];
				b[i][k] = b[j][k];
				b[j][k] = t;
			}
		for (j = 0; j < 4; j++) {
			if (j == i)
				a[i][j] = 1.0f;
			if (i < j)
				a[i][j] /= piv;
			b[i][j] /= piv;
		}
		for (j = 0; j < 4; j++)
			if (j != i) {
				piv = a[j][i];
				for (k = 0; k < 4; k++) {
					if (k == i)
						a[j][k] = 0.0f;
					if (i < k)
						a[j][k] -= a[i][k] * piv;
					b[j][k] -= b[i][k] * piv;
				}
			}
	}
	mat4 r;
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++) {
			float *rp = (float *)&r;
			((float *)((char *)rp + (i << 4)))[j] = b[i][j];
		}
	return r;
}

static inline mat4 cam_perspective_tan(float t, float aspect, float n, float f)
{
	float cot = 1.0f / t;
	return mat4(cot / aspect, 0.0f, 0.0f, 0.0f, 0.0f, cot, 0.0f, 0.0f, 0.0f, 0.0f, (f + n) / (n - f), -1.0f, 0.0f,
	    0.0f, 2.0f * f * n / (n - f), 0.0f);
}

/* Local copy of mat4_identity() from vec.h. */
inline const mat4 &mat4_identity()
{
	return s_unitMatrix;
}

#include "camera.h"

Camera::Camera(Object *parent) : Object(parent)
{
	view.m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	view.m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	view.m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	view.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	view_inv.m0.Set(1.0f, 0.0f, 0.0f, 0.0f);
	view_inv.m1.Set(0.0f, 1.0f, 0.0f, 0.0f);
	view_inv.m2.Set(0.0f, 0.0f, 1.0f, 0.0f);
	view_inv.m3.Set(0.0f, 0.0f, 0.0f, 1.0f);
	bg_color.Set(0.0f, 0.0f, 0.0f, 0.0f);

	flag_back = 1;
	screen_x = 0;
	screen_y = 0;
	screen_w = 640;
	screen_h = 448;
	vscreen_w = 640.0f;
	vscreen_h = 480.0f;
	vscreen_z = 500.0f;
	clip_near = 1.0f;
	clip_far = 32768.0f;

	GetScene()->RefCamera(this);

	tan_half_fov_y = (vscreen_h * 0.5f) / vscreen_z;
	tex_lod_offset = 10;
	tex_lod_bias = 0;
}

Camera::~Camera()
{}

int Camera::SetOption(int option, const int &value, unsigned int size)
{
	if (option == OPT_BACK) {
		flag_back = (value != 0);
		return 1;
	}
	return Object::SetOption(option, value, 1);
}

int Camera::SetOption(int option, const float &value, unsigned int size)
{
	return Object::SetOption(option, value, 1);
}

int Camera::SetOption(int option, const String &value)
{
	return Object::SetOption(option, value);
}

void Camera::SetBGColor(const vec4 &c)
{
	bg_color = c;
}

void Camera::SetViewport(int x, int y, int w, int h)
{
	screen_x = x;
	screen_y = y;
	screen_w = w;
	screen_h = h;
}

void Camera::GetViewport(int &x, int &y, int &w, int &h)
{
	x = screen_x;
	y = screen_y;
	w = screen_w;
	h = screen_h;
}

void Camera::SetVirtualScreen(float w, float h, float z)
{
	vscreen_w = w;
	vscreen_h = h;
	if (z > 0.0f)
		vscreen_z = z;
}

void Camera::GetVirtualScreen(float &w, float &h, float &z)
{
	w = vscreen_w;
	h = vscreen_h;
	z = vscreen_z;
}

void Camera::SetClipRange(float near_, float far_)
{
	clip_near = near_;
	clip_far = far_;
}

void Camera::GetClipRange(float &near_, float &far_)
{
	near_ = clip_near;
	far_ = clip_far;
}

float Camera::GetPixelAdjustDepth(float value)
{
	if (value == 0.0f)
		return -1.0f;
	return vscreen_z / value;
}

float Camera::GetPixelAdjustScale(float value)
{
	return value / vscreen_z;
}

void Camera::ViewSetup(gm_graph &g)
{
	view = mat4(-world_mat.m0, world_mat.m1, -world_mat.m2, world_mat.m3);
	view_inv = cam_inverse(view);

	g.MatrixMode(GM_MATRIX_PROJECTION);
	g.LoadIdentity();
	tan_half_fov_y = vscreen_h * 0.5f / vscreen_z;
	g.LoadMatrix(cam_perspective_tan(tan_half_fov_y, vscreen_w / vscreen_h, clip_near, clip_far));
	g.SetEnableBits(g.fFlagsE | 0x10);

	tex_lod_offset = (int)(-logf(vscreen_w * 128.0f / (tan_half_fov_y * 128.0f * 2.0f)) / logf(2.0f) * 16.0f);
	g.TexLodFunc(0, 0, tex_lod_offset + tex_lod_bias);

	g.MatrixMode(GM_MATRIX_VIEW);
	g.LoadIdentity();
	g.MultMatrix(view_inv);

	g.Viewport((float)screen_x, (float)screen_y, (float)screen_w, (float)screen_h);

	if (flag_back)
		g.ClearColor(bg_color);
	g.ClearDepth(1.0f);
	g.ClearStencil(0);
	if (flag_back)
		g.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}
