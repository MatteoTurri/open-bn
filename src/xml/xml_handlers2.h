/*
 * DeclHandler and LexicalHandler: the SAX2 extension interfaces, with empty defaults.
 */

#ifndef GUARD_XML_HANDLERS2_H
#define GUARD_XML_HANDLERS2_H

class XMLString;

class DeclHandler {
public:
	virtual void attributeDecl(XMLString *elementName, XMLString *attrName, XMLString *type, XMLString *mode,
	    XMLString *value);
	virtual void elementDecl(XMLString *name, XMLString *model);
	virtual void externalEntityDecl(XMLString *name, XMLString *publicId, XMLString *systemId);
	virtual void internalEntityDecl(XMLString *name, XMLString *value);
};

class LexicalHandler {
public:
	virtual void comment(XMLString *chars, unsigned long start, unsigned long length);
	virtual void endCDATA();
	virtual void endDTD();
	virtual void endEntity(XMLString *name);
	virtual void startCDATA();
	virtual void startDTD(XMLString *name, XMLString *publicId, XMLString *systemId);
	virtual void startEntity(XMLString *name);
};

#endif // GUARD_XML_HANDLERS2_H
