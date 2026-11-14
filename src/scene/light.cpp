/*
 * Light: scene-graph light (type 0 directional, 1 point, 2 spot) bound to a
 * gm_graph light slot; also emulates point and spot lights per object.
 */

#include <iostream.h>
#include "gfx/vec.h"
#include <vector>
#include <string>
#include <math.h>
#include "gfx/gm_graph.h"
#include "gfx/gm_vertex.h"
#include "object.h"
#include "scene.h"
#include "light.h"
#include "scene/optid.h"

/* Identity matrix and w = 1 row. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

class Object;
class Light;
class Scene;
class gm_graph;

static inline float DotVec3(const vec4 *a, const vec4 *b)
{
	return a->x * b->x + a->y * b->y + a->z * b->z;
}

/* Debug shapes, one per light type: 24 vertices of format 3 (position,
   normal) each. */
static gmVertexPN light_shape_0[24] = {
	{ -0.5f, 0.5f, 0.5f, -0.707f, 0.0f, -0.707f },
	{ -0.5f, -0.5f, 0.5f, -0.707f, 0.0f, -0.707f },
	{ -0.5f, -0.5f, 0.5f, 0.0f, -0.707f, -0.707f },
	{ 0.5f, -0.5f, 0.5f, 0.0f, -0.707f, -0.707f },
	{ 0.5f, -0.5f, 0.5f, 0.707f, 0.0f, -0.707f },
	{ 0.5f, 0.5f, 0.5f, 0.707f, 0.0f, -0.707f },
	{ 0.5f, 0.5f, 0.5f, 0.0f, 0.707f, -0.707f },
	{ -0.5f, 0.5f, 0.5f, 0.0f, 0.707f, -0.707f },
	{ 0.0f, 0.0f, 0.0f, -0.577f, 0.577f, -0.577f },
	{ -0.5f, 0.5f, 0.5f, -0.577f, 0.577f, -0.577f },
	{ 0.0f, 0.0f, 0.0f, -0.577f, -0.577f, -0.577f },
	{ -0.5f, -0.5f, 0.5f, -0.577f, -0.577f, -0.577f },
	{ 0.0f, 0.0f, 0.0f, 0.577f, 0.577f, -0.577f },
	{ 0.5f, 0.5f, 0.5f, 0.577f, 0.577f, -0.577f },
	{ 0.0f, 0.0f, 0.0f, 0.577f, -0.577f, -0.577f },
	{ 0.5f, -0.5f, 0.5f, 0.577f, -0.577f, -0.577f },
	{ -0.5f, 0.5f, 0.5f, -0.577f, 0.577f, -0.577f },
	{ -0.5f, 0.5f, 0.8f, -0.577f, 0.577f, -0.577f },
	{ -0.5f, -0.5f, 0.5f, -0.577f, -0.577f, -0.577f },
	{ -0.5f, -0.5f, 0.8f, -0.577f, -0.577f, -0.577f },
	{ 0.5f, 0.5f, 0.5f, 0.577f, 0.577f, -0.577f },
	{ 0.5f, 0.5f, 0.8f, 0.577f, 0.577f, -0.577f },
	{ 0.5f, -0.5f, 0.5f, 0.577f, -0.577f, -0.577f },
	{ 0.5f, -0.5f, 0.8f, 0.577f, -0.577f, -0.577f },
};
static gmVertexPN light_shape_1[24] = {
	{ 0.2f, 0.2f, 0.2f, 0.577f, 0.577f, 0.577f },
	{ 0.5f, 0.5f, 0.5f, 0.577f, 0.577f, 0.577f },
	{ -0.2f, 0.2f, 0.2f, -0.577f, 0.577f, 0.577f },
	{ -0.5f, 0.5f, 0.5f, -0.577f, 0.577f, 0.577f },
	{ 0.2f, -0.2f, 0.2f, 0.577f, -0.577f, 0.577f },
	{ 0.5f, -0.5f, 0.5f, 0.577f, -0.577f, 0.577f },
	{ -0.2f, -0.2f, 0.2f, -0.577f, -0.577f, 0.577f },
	{ -0.5f, -0.5f, 0.5f, -0.577f, -0.577f, 0.577f },
	{ 0.2f, 0.2f, -0.2f, 0.577f, 0.577f, -0.577f },
	{ 0.5f, 0.5f, -0.5f, 0.577f, 0.577f, -0.577f },
	{ -0.2f, 0.2f, -0.2f, -0.577f, 0.577f, -0.577f },
	{ -0.5f, 0.5f, -0.5f, -0.577f, 0.577f, -0.577f },
	{ 0.2f, -0.2f, -0.2f, 0.577f, -0.577f, -0.577f },
	{ 0.5f, -0.5f, -0.5f, 0.577f, -0.577f, -0.577f },
	{ -0.2f, -0.2f, -0.2f, -0.577f, -0.577f, -0.577f },
	{ -0.5f, -0.5f, -0.5f, -0.577f, -0.577f, -0.577f },
	{ 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
	{ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
	{ -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f },
	{ -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f },
	{ 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },
	{ 0.0f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f },
	{ 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f },
};
static gmVertexPN light_shape_2[24] = {
	{ -0.5f, 0.5f, 0.5f, -0.707f, 0.0f, -0.707f },
	{ -0.5f, -0.5f, 0.5f, -0.707f, 0.0f, -0.707f },
	{ -0.5f, -0.5f, 0.5f, 0.0f, -0.707f, -0.707f },
	{ 0.5f, -0.5f, 0.5f, 0.0f, -0.707f, -0.707f },
	{ 0.5f, -0.5f, 0.5f, 0.707f, 0.0f, -0.707f },
	{ 0.5f, 0.5f, 0.5f, 0.707f, 0.0f, -0.707f },
	{ 0.5f, 0.5f, 0.5f, 0.0f, 0.707f, -0.707f },
	{ -0.5f, 0.5f, 0.5f, 0.0f, 0.707f, -0.707f },
	{ 0.0f, 0.0f, 0.0f, -0.577f, 0.577f, -0.577f },
	{ -0.5f, 0.5f, 0.5f, -0.577f, 0.577f, -0.577f },
	{ 0.0f, 0.0f, 0.0f, -0.577f, -0.577f, -0.577f },
	{ -0.5f, -0.5f, 0.5f, -0.577f, -0.577f, -0.577f },
	{ 0.0f, 0.0f, 0.0f, 0.577f, 0.577f, -0.577f },
	{ 0.5f, 0.5f, 0.5f, 0.577f, 0.577f, -0.577f },
	{ 0.0f, 0.0f, 0.0f, 0.577f, -0.577f, -0.577f },
	{ 0.5f, -0.5f, 0.5f, 0.577f, -0.577f, -0.577f },
	{ -0.5f, 0.5f, 0.5f, -0.577f, 0.577f, -0.577f },
	{ -0.8f, 0.8f, 0.8f, -0.577f, 0.577f, -0.577f },
	{ -0.5f, -0.5f, 0.5f, -0.577f, -0.577f, -0.577f },
	{ -0.8f, -0.8f, 0.8f, -0.577f, -0.577f, -0.577f },
	{ 0.5f, 0.5f, 0.5f, 0.577f, 0.577f, -0.577f },
	{ 0.8f, 0.8f, 0.8f, 0.577f, 0.577f, -0.577f },
	{ 0.5f, -0.5f, 0.5f, 0.577f, -0.577f, -0.577f },
	{ 0.8f, -0.8f, 0.8f, 0.577f, -0.577f, -0.577f },
};
static const void *light_shape_table[3] = {
	light_shape_0,
	light_shape_1,
	light_shape_2,
};

void Light::ActivatePseudoPointLightState(gm_graph &g, int no, const vec4 &pos)
{
	if (visible) {
		vec4 dir;
		if (type == GM_LIGHT_DIRECTIONAL) {
			g.EnableLight(no, true);
			g.Light(no, GM_LIGHT_DIRECTIONAL, color, world_rot.m2);
			return;
		}
		const vec4 *wp = &world_mat.m3;
		dir.x = pos.x - wp->x;
		dir.y = pos.y - wp->y;
		dir.z = pos.z - wp->z;
		dir.w = pos.w - wp->w;
		float scale = 1.0f;
		if (type == GM_LIGHT_SPOT) {
			float dot = DotVec3(&dir, &world_rot.m2);
			float len = sqrtf(DotVec3(&dir, &dir));
			scale = cone_scale * (dot / len) + cone_bias;
			if (scale <= 0.0f)
				goto disable;
			scale = scale * scale * scale;
		}
		float dist = sqrtf(DotVec3(&dir, &dir));
		if (dist < range && 0.0f < dist) {
			float att = atten_const + atten_linear * dist + atten_quad * dist * dist;
			if (0.0f < att) {
				scale = scale / att;
				g.EnableLight(no, true);
				vec4 c;
				c.w = color.w;
				c.x = color.x * scale;
				c.y = color.y * scale;
				c.z = color.z * scale;
				g.Light(no, GM_LIGHT_DIRECTIONAL, c, dir);
				return;
			}
		}
	}
disable:
	g.DisableLight(no, true);
}

int Light::GetType() const
{
	return type;
}

Light::Light(Object *parent, int no) : Object(parent)
{
	this->no = no;
	type = GM_LIGHT_DIRECTIONAL;
	atten_linear = 0.0f;
	atten_quad = 0.0f;
	cone_falloff = 0.0f;
	range = 1000000.0f;
	atten_const = 1.0f;
	cone_angle = 3.1416f;
	((Scene *)scene)->AppendLight(this, no);
}

Light::~Light()
{
	((Scene *)scene)->RemoveLight(this, no);
}

void Light::DrawThis(gm_graph &g)
{
	if (!visible)
		return;
	if (!debug)
		return;
	g.BlendFunc(blend_mode);
	g.SetTexture(0);
	int t = type;
	if ((unsigned int)t >= 3)
		t = 0;
	g.SetArrays(light_shape_table[t], GM_VERTEX_PN, 24, 1, 0, (const void *)0);
	g.DrawArrays(GM_PRIM_LINE, 0, 24, 1, (const short *)0);
}

void Light::ActivateLightState(gm_graph &g, int no)
{
	bool vis = visible;
	g.EnableLight(no, vis);
	if (vis) {
		g.Light(no, type, color, world_mat.m3, world_rot.m2);
		if (type) {
			g.LightRange(no, range, atten_const, atten_linear, atten_quad);
			if (type != GM_LIGHT_POINT)
				g.LightCone(no, cone_angle, cone_falloff);
		}
	}
}

int Light::SetOption(int option, const int &value, unsigned int size)
{
	if (option == OPT_TYPE) {
		SetType(value);
		return 1;
	}
	return Object::SetOption(option, value, size);
}

int Light::SetOption(int option, const float &value, unsigned int size)
{
	if (option == OPT_CONE || option == OPT_RANG)
		return SetOption(option, &value, size * 4);
	return Object::SetOption(option, value, size);
}

int Light::SetOption(int option, const void *data, unsigned int size)
{
	const float *f = (const float *)data;

	switch (option) {
	case OPT_RANG:
		SetRange(f[0], f[1], f[2], f[3]);
		return 1;
	case OPT_CONE:
		SetCone(f[0], f[1]);
		return 1;
	}
	return Object::SetOption(option, data, size);
}

int Light::GetOption(int option, int &value, unsigned int size) const
{
	if (option == OPT_TYPE) {
		GetType(value);
		return 1;
	}
	return Object::GetOption(option, value, size);
}

int Light::GetOption(int option, float &value, unsigned int size) const
{
	if (option == OPT_CONE || option == OPT_RANG)
		return GetOption(option, &value, size * 4);
	return Object::GetOption(option, value, size);
}

int Light::GetOption(int option, void *data, unsigned int size) const
{
	float *f = (float *)data;

	switch (option) {
	case OPT_RANG:
		GetRange(f[0], f[1], f[2], f[3]);
		return 1;
	case OPT_CONE:
		GetCone(f[0], f[1]);
		return 1;
	}
	return Object::GetOption(option, data, size);
}

void Light::SetType(int type)
{
	this->type = type;
}

void Light::SetRange(float maxDist, float constant, float linear, float quadratic)
{
	range = maxDist;
	atten_const = constant;
	atten_linear = linear;
	atten_quad = quadratic;
}

void Light::SetCone(float angle, float falloff)
{
	cone_angle = angle;
	cone_falloff = falloff;
	if (angle < 0.01f)
		angle = 0.01f;
	if (angle > 3.14f)
		angle = 3.14f;
	angle = cosf(angle);
	falloff = falloff / 3.0f;
	if (falloff < 1.0f)
		falloff = 1.0f;
	float s = falloff / (1.0f - angle);
	cone_scale = s;
	cone_bias = -s * angle + (1.0f - falloff);
}

void Light::GetType(int &type) const
{
	type = this->type;
}

void Light::GetRange(float &maxDist, float &constant, float &linear, float &quadratic) const
{
	maxDist = range;
	constant = atten_const;
	linear = atten_linear;
	quadratic = atten_quad;
}

void Light::GetCone(float &angle, float &falloff) const
{
	angle = cone_angle;
	falloff = cone_falloff;
}
