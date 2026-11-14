/*
 * DTDHandler: SAX DTD event interface (pure virtual).
 */

#ifndef GUARD_XML_DTDHANDLER_H
#define GUARD_XML_DTDHANDLER_H

class XMLString;

class DTDHandler {
public:
	virtual ~DTDHandler() {}
	virtual void notationDecl(XMLString *name, XMLString *publicId, XMLString *systemId) = 0;
	virtual void unparsedEntityDecl(XMLString *name, XMLString *publicId, XMLString *systemId,
	    XMLString *notationName) = 0;
};

#endif // GUARD_XML_DTDHANDLER_H
