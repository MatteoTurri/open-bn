/*
 * ModelData: reference-counted GMO model (a Hook and a gmo_model, the
 * gmo_model base at offset 0x20).
 */

#ifndef MODELDATA_H
#define MODELDATA_H

#include "gfx/gmo.h"

#include "scene/hook.h"

#include "scenefacade.h"

#pragma interface

class ModelData : public Hook, public gmo_model {
public:
	/* 0x1E0 */ int refcount;

	ModelData(const char *name);
	ModelData(void *data, int size, const char *name);
	~ModelData();

	void AddRef();
	void Release();
};

#endif
