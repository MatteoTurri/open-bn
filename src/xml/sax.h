/*
 * HandlerBase: the default SAX handler, implementing DocumentHandler, DTDHandler, EntityResolver
 * and ErrorHandler with empty bodies. Entirely inline.
 */

#ifndef GUARD_SAX_H
#define GUARD_SAX_H
#include "xml_documenthandler.h"
#include "xml_dtdhandler.h"
#include "xml_entityresolver.h"
#include "xml_errorhandler.h"
#include "xml_mmobject.h"

class XMLString;
class Locator;
class AttributeList;
class SAXParseException;
class InputSource;

class HandlerBase : public DocumentHandler,
                    public DTDHandler,
                    public EntityResolver,
                    public ErrorHandler,
                    public XMLMMObject {
public:
	HandlerBase() {}
	virtual ~HandlerBase() {}

	// DocumentHandler
	void characters(XMLString *chars, unsigned start, unsigned length) {}
	void endDocument() {}
	void endElement(XMLString *name) {}
	void ignorableWhitespace(XMLString *chars, unsigned start, unsigned length) {}
	void processingInstruction(XMLString *target, XMLString *data) {}
	void setDocumentLocator(Locator *locator) {}
	void startDocument() {}
	void startElement(XMLString *name, AttributeList &attrs) {}
	void characters(XMLString *chars, unsigned length) {}
	void ignorableWhitespace(XMLString *chars, unsigned length) {}
	// DTDHandler
	void notationDecl(XMLString *name, XMLString *publicId, XMLString *systemId) {}
	void unparsedEntityDecl(XMLString *name, XMLString *publicId, XMLString *systemId, XMLString *notationName) {}
	// EntityResolver
	InputSource *resolveEntity(XMLString *publicId, XMLString *systemId) { return 0; }
	// ErrorHandler
	void error(SAXParseException &exc) {}
	void fatalError(SAXParseException &exc) {}
	void warning(SAXParseException &exc) {}
};

#endif // GUARD_SAX_H
