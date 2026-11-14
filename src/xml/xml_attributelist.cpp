/*
 * AttributeListImpl: the attribute list passed to SAX startElement, a singly linked
 * list of Attribute (name, value, type, specified) looked up by index or by name.
 */

#include "xml_filedescriptor.h"
#include "xml_attributelist.h"
#include "xml_inputstreamimpl.h"
#include "xml_attributelistimpl.h"
#include "xml_string.h"

AttributeListImpl::AttributeListImpl()
{
	fHead = 0;
	fTail = 0;
	fLength = 0;
}

AttributeListImpl::~AttributeListImpl()
{
	delete fHead;
}

void AttributeListImpl::Clear()
{
	delete fHead;
	fHead = 0;
	fTail = 0;
	fLength = 0;
}

unsigned AttributeListImpl::getLength()
{
	return fLength;
}

XMLString *AttributeListImpl::getName(unsigned index)
{
	if (index >= fLength)
		return 0;
	Attribute *a = fHead;
	for (unsigned i = 0; i < index; i++)
		a = a->fNext;
	return a->fName;
}

XMLString *AttributeListImpl::getType(unsigned index)
{
	if (index >= fLength)
		return 0;
	Attribute *a = fHead;
	for (unsigned i = 0; i < index; i++)
		a = a->fNext;
	return a->fType;
}

XMLString *AttributeListImpl::getType(XMLString *name)
{
	Attribute *a = fHead;
	while (a) {
		if (*a->fName == *name->Clone())
			return a->fType;
		a = a->fNext;
	}
	return 0;
}

XMLString *AttributeListImpl::getValue(unsigned index)
{
	if (index >= fLength)
		return 0;
	Attribute *a = fHead;
	for (unsigned i = 0; i < index; i++)
		a = a->fNext;
	return a->fValue;
}

XMLString *AttributeListImpl::getValue(XMLString *name)
{
	Attribute *a = fHead;
	while (a) {
		if (*a->fName == *name)
			return a->fValue;
		a = a->fNext;
	}
	return 0;
}

void AttributeListImpl::AddAttribute(XMLString *name, XMLString *value, XMLString *type, bool specified)
{
	Attribute *a = new Attribute();
	a->fName = new XMLString(name->GetBytes(), name->ByteLength(), (XMLCodeType)name->GetCodeType());
	a->fValue = new XMLString(value->GetBytes(), value->ByteLength(), (XMLCodeType)value->GetCodeType());
	a->fType = new XMLString(type->GetBytes(), type->ByteLength(), (XMLCodeType)type->GetCodeType());
	a->fSpecified = specified;
	if (fHead) {
		fTail->fNext = a;
		fTail = a;
	} else {
		fHead = a;
		fTail = a;
	}
	++fLength;
}

bool AttributeListImpl::Declared(XMLString *name)
{
	Attribute *a = fHead;
	for (unsigned i = 0; i < fLength; i++) {
		if (*name == *a->fName)
			return true;
		a = a->fNext;
	}
	return false;
}

bool AttributeListImpl::getSpecified(unsigned index)
{
	if (index >= fLength)
		return false;
	Attribute *a = fHead;
	for (unsigned i = 0; i < index; i++)
		a = a->fNext;
	return a->fSpecified;
}
