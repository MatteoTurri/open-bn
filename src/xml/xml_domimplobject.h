/*
 * DOMImplObject: base of the DOM implementation objects (status and owner count).
 */

#ifndef GUARD_XML_DOMIMPLOBJECT_H
#define GUARD_XML_DOMIMPLOBJECT_H
#pragma interface
#include "xml_gc.h"
#include "xml_domexception.h"

class DOMImplObject : public GCObject {
public:
	DOMImplObject();
	virtual ~DOMImplObject();
	int Status();
	void resetException() { fStatus = 0; }
	void treatException(DOMException::ExceptionCode code);
	void AddOwner();
	bool RemoveOwner();

protected:
	/* 0x08 */ int fStatus;
	/* 0x0C */ int fOwnerCount;
};

#endif // GUARD_XML_DOMIMPLOBJECT_H
