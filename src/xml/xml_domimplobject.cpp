/*
 * DOMImplObject: the status code and owner count shared by the DOM implementation
 * objects.
 */

#pragma implementation "xml_domimplobject.h"
#include "xml_domimplobject.h"

inline DOMImplObject::DOMImplObject()
{
	fStatus = 0;
}

inline DOMImplObject::~DOMImplObject()
{}

inline void DOMImplObject::treatException(DOMException::ExceptionCode code)
{
	fStatus = code;
}

inline int DOMImplObject::Status()
{
	return fStatus;
}

inline bool DOMImplObject::RemoveOwner()
{
	return --fOwnerCount != 0;
}

inline void DOMImplObject::AddOwner()
{
	fOwnerCount++;
}
