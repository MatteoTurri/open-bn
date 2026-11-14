/*
 * AttributeListImpl: the SAX AttributeList as a singly linked list of Attribute records.
 */

#ifndef GUARD_XML_ATTRIBUTELISTIMPL_H
#define GUARD_XML_ATTRIBUTELISTIMPL_H
#include "xml_attributelist.h"
#include "xml_attribute.h"

class AttributeListImpl : public AttributeList {
public:
	AttributeListImpl();
	virtual ~AttributeListImpl();
	unsigned getLength();
	XMLString *getName(unsigned index);
	XMLString *getType(unsigned index);
	XMLString *getValue(unsigned index);
	XMLString *getType(XMLString *name);
	XMLString *getValue(XMLString *name);
	bool getSpecified(unsigned index);
	void AddAttribute(XMLString *name, XMLString *value, XMLString *type, bool specified);
	bool Declared(XMLString *name);
	void Clear();

private:
	/* 0x04 */ unsigned fLength;
	/* 0x08 */ Attribute *fHead;
	/* 0x0C */ Attribute *fTail;
};

#endif // GUARD_XML_ATTRIBUTELISTIMPL_H
