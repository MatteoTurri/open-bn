/*
 * GCObject: the reference count of the DOM objects, with the hooks called on the first
 * and on the last reference.
 */

#include "xml_gc.h"

GCObject::GCObject()
{
	fRefCount = 0;
}

GCObject::~GCObject()
{}

void GCObject::AddRef(GCObject *obj)
{
	obj->fRefCount++;
	if (obj->fRefCount == 1)
		obj->firstReference();
}

void GCObject::RemoveRef(GCObject *obj)
{
	obj->fRefCount--;
	if (obj->fRefCount == 0)
		obj->lastReference();
}

void GCObject::firstReference()
{}

void GCObject::lastReference()
{}

int GCObject::Referenced()
{
	return fRefCount != 0;
}
