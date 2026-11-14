/*
 * SAXParseException: SAX exception with a position (column, line, public and system id).
 */

#ifndef GUARD_XML_SAXPARSEEXCEPTION_H
#define GUARD_XML_SAXPARSEEXCEPTION_H
#pragma interface
#include "xml_saxexception.h"
#include "xml_locator.h"

class SAXParseException : public SAXException {
public:
	SAXParseException();
	SAXParseException(XMLString *message, Locator *locator);
	SAXParseException(SAXParseException &other);
	SAXParseException(XMLString *message, Locator *locator, SAXException *rootException);
	SAXParseException(XMLString *message, XMLString *publicId, XMLString *systemId, unsigned line, unsigned column);
	SAXParseException(XMLString *message, XMLString *publicId, XMLString *systemId, unsigned line, unsigned column,
	    SAXException other);
	virtual ~SAXParseException();
	SAXParseException &operator=(SAXParseException &other);

	unsigned getColumnNumber();
	unsigned getLineNumber();
	XMLString *getPublicId();
	XMLString *getSystemId();
	bool Success();

private:
	/* 0x08 */ unsigned fColumn;
	/* 0x0C */ unsigned fLine;
	/* 0x10 */ XMLString *fPublicId;
	/* 0x14 */ XMLString *fSystemId;
};

#endif // GUARD_XML_SAXPARSEEXCEPTION_H
