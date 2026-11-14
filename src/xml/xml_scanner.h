/*
 * XMLDocumentScanner: the XML parser (the SAX Parser implementation) driving the token scanners.
 */

#ifndef GUARD_XML_SCANNER_H
#define GUARD_XML_SCANNER_H
#pragma interface
#include "xml_parser.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#include "xml_handlers2.h"
#include "xml_mmobject.h"
#include "xml_string.h"
#include "xml_outputsource.h"
#include "xml_parseerrorprocessor.h"

class InputSource;
class XMLStreamManager;
class XMLInputSource;

class XMLDocumentScanner : public Parser {
public:
	XMLDocumentScanner();
	virtual ~XMLDocumentScanner();

	/* Overrides of the pure virtuals of Parser, in the order of the vtable. */
	virtual void parse(XMLInputSource *src);
	virtual void parse(InputSource &src);
	virtual void parse(XMLString *src, XMLIOType type, XMLCodeType codeType);
	virtual void parse(XMLFileDescriptor *fd);
	virtual void setDocumentHandler(DocumentHandler *h);
	virtual void setEntityResolver(EntityResolver *h);
	virtual void setErrorHandler(ErrorHandler *h);
	virtual void setDTDHandler(DTDHandler *h);
	virtual void parse(char *text, XMLIOType type, XMLCodeType codeType);

	/* not virtual */
	/* inline: ParseString (xmlparse_ns.cpp) writes the two fields directly */
	void setDeclHandler(DeclHandler *h);
	void setLexicalHandler(LexicalHandler *h);
	ParseErrorProcessor *GetErrorProcessor();
	bool success(XMLScanResult r);
	void notationDecl(XMLString *name, XMLString *publicId, XMLString *systemId, XMLStreamManager *m);
	void unparsedEntityDecl(XMLString *name, XMLString *publicId, XMLString *systemId, XMLString *notationName,
	    XMLStreamManager *m);
	void startDocument();
	void endDocument();
	void startCDATA();
	void endCDATA();
	void endDTD();
	void startEntity(XMLString *name, XMLStreamManager *m);
	void endEntity(XMLString *name, XMLStreamManager *m);
	void startDTD(XMLString *name, XMLString *pub, XMLString *sys, XMLStreamManager *m);
	void externalEntityDecl(XMLString *name, XMLString *publicId, XMLString *systemId, XMLStreamManager *m);
	void internalEntityDecl(XMLString *name, XMLString *value, XMLStreamManager *m);
	void processingInstruction(XMLString *target, XMLString *data, XMLStreamManager *m);
	void startElement(XMLString *name, AttributeList &attrs, XMLStreamManager *m);
	void endElement(XMLString *name, XMLStreamManager *m);
	void attributeDecl(XMLString *eName, XMLString *aName, XMLString *type, XMLString *mode, XMLString *value,
	    XMLStreamManager *m);
	void comment(XMLString *chars, unsigned length, XMLStreamManager *m);
	void characters(XMLString *chars, unsigned length, XMLStreamManager *m);
	void ignorableWhitespace(XMLString *chars, unsigned length, XMLStreamManager *m);
	void TreatErrorState(XMLParseErrorState state, XMLInputStream *stream);

protected:
	/* 0x00: vtable pointer */
	/* 0x04 */ XMLString *fPercentText;  /* new XMLString("%", false) */
	/* 0x08 */ int fElementDepth;  /* element depth, counted in start/endElement */
	/* 0x0C */ int fTrackDepth;  /* enables the depth count */
	/* 0x10 */ XMLString *fPercentSpare;  /* new XMLString("%", false) */
	/* 0x14 */ char fPad20[4];
	/* 0x18 */ ParseErrorProcessor *fErrorProcessor;
	/* 0x1C */ EntityResolver *fEntityResolver;
	/* 0x20 */ ErrorHandler *fErrorHandler;
	/* 0x24 */ DTDHandler *fDTDHandler;
	/* 0x28 */ DeclHandler *fDeclHandler;
	/* 0x2C */ LexicalHandler *fLexicalHandler;
	/* 0x30 */ DocumentHandler *fDocumentHandler;
};

inline void XMLDocumentScanner::parse(InputSource &)
{}

inline void XMLDocumentScanner::setEntityResolver(EntityResolver *h)
{
	fEntityResolver = h;
}

inline void XMLDocumentScanner::setDTDHandler(DTDHandler *h)
{
	fDTDHandler = h;
}

inline void XMLDocumentScanner::setDeclHandler(DeclHandler *h)
{
	fDeclHandler = h;
}

inline void XMLDocumentScanner::setLexicalHandler(LexicalHandler *h)
{
	fLexicalHandler = h;
}

inline ParseErrorProcessor *XMLDocumentScanner::GetErrorProcessor()
{
	return fErrorProcessor;
}

#endif // GUARD_XML_SCANNER_H
