/*
 * DOMImplementation: the reference-counted handle of the DOM implementation object.
 */

#include "xml_domimpl.h"

DOMImplementation::DOMImplementation()
{}

DOMImplementation::DOMImplementation(const DOMImplementation &other)
{
	fImpl = other.fImpl;
	if (fImpl)
		GCObject::AddRef(fImpl);
}

DOMImplementation::DOMImplementation(DOMImplementationImpl *impl)
{
	fImpl = impl;
}

DOMImplementation::~DOMImplementation()
{}

DOMImplementation &DOMImplementation::operator=(const DOMImplementation &other)
{
	if (fImpl != other.fImpl) {
		if (fImpl)
			GCObject::RemoveRef(fImpl);
		fImpl = other.fImpl;
		if (fImpl)
			GCObject::AddRef(fImpl);
	}
	return *this;
}

bool DOMImplementation::operator==(const DOMImplementation &other) const
{
	return fImpl == other.fImpl;
}

bool DOMImplementation::operator!=(const DOMImplementation &other) const
{
	return fImpl != other.fImpl;
}

DOMImplementation &DOMImplementation::operator=(const int *other)
{
	if (other == 0) {
		if (fImpl) {
			GCObject::RemoveRef(fImpl);
			fImpl = 0;
		}
	}
	return *this;
}

bool DOMImplementation::operator==(const int *other) const
{
	if (other == 0)
		return fImpl == 0;
	return false;
}

bool DOMImplementation::operator!=(const int *other) const
{
	if (other == 0)
		return fImpl != 0;
	return true;
}
