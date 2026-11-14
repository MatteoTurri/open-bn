/*
 * Attribute: one entry of the linked list of AttributeListImpl.
 */

#ifndef GUARD_XML_ATTRIBUTE_H
#define GUARD_XML_ATTRIBUTE_H
#include "xml_mmobject.h"

class XMLString;

class Attribute : public XMLMMObject {
public:
	Attribute();
	virtual ~Attribute();
	/* 0x04 */ XMLString *fName;
	/* 0x08 */ XMLString *fType;
	/* 0x0C */ XMLString *fValue;
	/* 0x10 */ bool fSpecified;
	/* 0x14 */ Attribute *fNext;
};

#endif // GUARD_XML_ATTRIBUTE_H
