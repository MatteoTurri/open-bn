/*
 * Declaration: base class of the DTD declarations (name, system and public identifier).
 */

#ifndef GUARD_XML_DECLARATION_H
#define GUARD_XML_DECLARATION_H
#pragma interface
#include "xml_mmobject.h"

class XMLString;

class EntityDeclManager;

class Declaration : public XMLMMObject {
	friend class EntityDeclManager;

public:
	Declaration();
	Declaration(XMLString *name, XMLString *systemId, XMLString *publicId);
	virtual ~Declaration();
	XMLString *GetName();
	XMLString *GetPublicId();
	XMLString *GetSystemId();
	void SetName(XMLString *name);
	Declaration *Next();

protected:
	/* 0x04 */ XMLString *fSystemId;
	/* 0x08 */ XMLString *fPublicId;
	/* 0x0C */ XMLString *fName;
	/* 0x10 */ Declaration *fNext;
};

#endif // GUARD_XML_DECLARATION_H
