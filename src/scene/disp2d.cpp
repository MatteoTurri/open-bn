/*
 * Disp2d methods.
 */

#include <iostream.h>
#include "gfx/vec.h"
#include "disp2d.h"
#include <vector>

static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);  /* unused, but part of the layout */
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);  /* unused, but part of the layout */

/* Places the object on the virtual screen of `camera`, facing it. */
void Disp2d::AdjustCamera(Camera *camera)
{
	float w, h, z;
	camera->GetVirtualScreen(w, h, z);
	vec4 p = camera->GetPos(GetParent(), vec4(0.0f, 0.0f, 10.0f, 1.0f));
	SetPos(p, 0);
	LookAt(camera, vec4(0.0f, 0.0f, 0.0f, 0.0f), 0, vec4(0.0f, 0.0f, 0.0f, 0.0f));
	p = camera->GetPos(GetParent(), vec4(w * 0.5f, -h * 0.5f, z, 1.0f));
	SetPos(p, 0);
}

Disp2d::Disp2d(Object *parent) : Object(parent)
{
	zsort_mode = ZSORT_OVERLAY;
	flip |= 2;
}

Disp2d::~Disp2d()
{}

void Disp2d::DrawThis(gm_graph &g)
{}

int Disp2d::SetOption(int option, const int &value, unsigned int size)
{
	return Object::SetOption(option, value, 1);
}

int Disp2d::SetOption(int option, const float &value, unsigned int size)
{
	return Object::SetOption(option, value, 1);
}

int Disp2d::SetOption(int option, const String &value)
{
	return Object::SetOption(option, value);
}

int Disp2d::GetOption(int option, int &value, unsigned int size) const
{
	return Object::GetOption(option, value, 1);
}

int Disp2d::GetOption(int option, float &value, unsigned int size) const
{
	return Object::GetOption(option, value, 1);
}

int Disp2d::GetOption(int option, String &value) const
{
	return Object::GetOption(option, value);
}
