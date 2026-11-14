/*
 * Constructor of LocatorImpl (the rest of the class is inline).
 */

#include "xml_locatorimpl.h"

LocatorImpl::LocatorImpl(XMLString *publicId, XMLString *systemId)
{
	fPublicId = publicId;
	fSystemId = systemId;
	fLineNumber = 0;
	fColumnNumber = 0;
}
