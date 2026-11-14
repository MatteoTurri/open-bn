/*
 * DocumentHandler: the SAX document event interface (pure virtual).
 */

#ifndef GUARD_XML_DOCUMENTHANDLER_H
#define GUARD_XML_DOCUMENTHANDLER_H
#include "xml_attributelist.h"
#include "xml_locator.h"

class XMLString;

class DocumentHandler {
public:
	virtual ~DocumentHandler() {}
	virtual void characters(XMLString *chars, unsigned start, unsigned length) = 0;
	virtual void endDocument() = 0;
	virtual void endElement(XMLString *name) = 0;
	virtual void ignorableWhitespace(XMLString *chars, unsigned start, unsigned length) = 0;
	virtual void processingInstruction(XMLString *target, XMLString *data) = 0;
	virtual void setDocumentLocator(Locator *locator) = 0;
	virtual void startDocument() = 0;
	virtual void startElement(XMLString *name, AttributeList &attrs) = 0;
	virtual void characters(XMLString *chars, unsigned length) = 0;
	virtual void ignorableWhitespace(XMLString *chars, unsigned length) = 0;
};

#endif // GUARD_XML_DOCUMENTHANDLER_H
