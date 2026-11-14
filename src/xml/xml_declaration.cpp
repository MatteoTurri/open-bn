/*
 * Declaration: a named DTD declaration (name, system and public id) that owns its
 * strings and the rest of its chain (fNext).
 */

#include "xml_filedescriptor.h"
#pragma implementation "xml_declaration.h"
#include "xml_declaration.h"
#include "xml_string.h"

Declaration::Declaration()
{
	fSystemId = 0;
	fPublicId = 0;
	fName = 0;
	fNext = 0;
}

Declaration::Declaration(XMLString *name, XMLString *systemId, XMLString *publicId)
{
	if (systemId)
		fSystemId = systemId->Clone();
	else
		fSystemId = 0;
	if (publicId)
		fPublicId = publicId->Clone();
	else
		fPublicId = 0;
	if (name)
		fName = name->Clone();
	else
		fName = 0;
	fNext = 0;
}

Declaration::~Declaration()
{
	if (fSystemId) {
		delete fSystemId;
		fSystemId = 0;
	}
	if (fPublicId) {
		delete fPublicId;
		fPublicId = 0;
	}
	if (fName) {
		delete fName;
		fName = 0;
	}
	if (fNext) {
		delete fNext;
		fNext = 0;
	}
}

XMLString *Declaration::GetSystemId()
{
	return fSystemId;
}
XMLString *Declaration::GetPublicId()
{
	return fPublicId;
}
XMLString *Declaration::GetName()
{
	return fName;
}
void Declaration::SetName(XMLString *name)
{
	fName = name;
}
Declaration *Declaration::Next()
{
	return fNext;
}
