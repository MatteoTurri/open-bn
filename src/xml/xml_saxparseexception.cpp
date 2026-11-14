/*
 * SAXParseException: a SAXException with the position (line, column, public and
 * system id) taken from a Locator.
 */

#include "xml_filedescriptor.h"
#pragma implementation "xml_saxparseexception.h"
#include "xml_saxparseexception.h"

unsigned SAXParseException::getColumnNumber()
{
	return fColumn;
}

unsigned SAXParseException::getLineNumber()
{
	return fLine;
}

XMLString *SAXParseException::getPublicId()
{
	return fPublicId;
}

XMLString *SAXParseException::getSystemId()
{
	return fSystemId;
}

SAXParseException::SAXParseException() : SAXException(0)
{
	fColumn = 0;
	fLine = 0;
	fPublicId = 0;
	fSystemId = 0;
}

SAXParseException::SAXParseException(SAXParseException &other) : SAXException(other)
{
	fColumn = other.fColumn;
	fLine = other.fLine;
	fPublicId = other.fPublicId;
	fSystemId = other.fSystemId;
}

SAXParseException &SAXParseException::operator=(SAXParseException &other)
{
	fColumn = other.fColumn;
	fLine = other.fLine;
	fPublicId = other.fPublicId;
	fSystemId = other.fSystemId;
	return (SAXParseException &)SAXException::operator=(other);
}

SAXParseException::SAXParseException(XMLString *message, Locator *locator) : SAXException(message)
{
	if (locator) {
		fColumn = locator->getColumnNumber();
		fLine = locator->getLineNumber();
		fPublicId = locator->getPublicId();
		fSystemId = locator->getSystemId();
	} else {
		fColumn = 0;
		fLine = 0;
		fPublicId = 0;
		fSystemId = 0;
	}
}

SAXParseException::SAXParseException(XMLString *, Locator *, SAXException *) : SAXException()
{}

SAXParseException::SAXParseException(XMLString *, XMLString *, XMLString *, unsigned, unsigned) : SAXException()
{}

/* Never implemented: the body is empty. */
SAXParseException::SAXParseException(XMLString *, XMLString *, XMLString *, unsigned, unsigned, SAXException)
        : SAXException()
{}

SAXParseException::~SAXParseException()
{}

bool SAXParseException::Success()
{
	return fMessage == 0;
}
