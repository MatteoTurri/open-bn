/*
 * ModelData constructors (from a file or a memory buffer) and reference
 * counting.
 */

#pragma implementation "modeldata.h"
#include "modeldata.h"
#include "gfx/vec.h"

/* Identity matrix and w = 1 row. */
static mat4 s_unitMatrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f);
static vec4 s_unitRow3(0.0f, 0.0f, 0.0f, 1.0f);

void ModelData::AddRef()
{
	refcount++;
}

void ModelData::Release()
{
	if (--refcount == 0)
		delete this;
}

ModelData::ModelData(const char *name) : Hook(1), gmo_model(*SceneGetGraph())
{
	refcount = 1;
	if (name)
		load_from_file(name, 0);
}

ModelData::ModelData(void *data, int size, const char *name) : Hook(1), gmo_model(*SceneGetGraph())
{
	refcount = 1;
	if (data)
		load_from_file(data, size, 0, name);
}

inline ModelData::~ModelData()
{
	DoCallback(0, 0);
}
