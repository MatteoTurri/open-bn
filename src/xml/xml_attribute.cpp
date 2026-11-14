/*
 * Constructor and destructor of Attribute, the record held by AttributeListImpl.
 */

#include "xml_filedescriptor.h"
#include "xml_attribute.h"
#include "xml_string.h"

Attribute::Attribute()
{
	fName = 0;
	fType = 0;
	fValue = 0;
	fNext = 0;
	fSpecified = 0;
}

Attribute::~Attribute()
{
	delete fNext;
	delete fName;
	delete fType;
	delete fValue;
}
