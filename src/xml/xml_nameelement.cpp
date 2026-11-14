/*
 * Constructors and destructor of NameElement.
 */

#include "xml_locator.h"
#include "xml_filedescriptor.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#include "xml_validator.h"

NameElement::NameElement(XMLString *name)
{
	fName = name->Clone();
	fFlag = 0;
}

NameElement::NameElement()
{
	fName = 0;
	fFlag = 0;
}

NameElement::~NameElement()
{
	delete fName;
}
