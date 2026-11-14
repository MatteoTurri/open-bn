/*
 * DOMUnit: the SAX handler that builds a DOM Document from the events of the parser.
 * Bases: DeclHandler (primary), LexicalHandler at 0x04, HandlerBase at 0x08.
 */

#ifndef GUARD_XML_DOMUNIT_H
#define GUARD_XML_DOMUNIT_H
#pragma interface
#include "xml_filedescriptor.h"
#include "sax.h"
#include "xml_scanner.h"
#include "xml_handlers2.h"
#include "xml_document.h"
#include "xml_saxparseexception.h"

class DOMUnit : public DeclHandler, public LexicalHandler, public HandlerBase {
public:
	DOMUnit();
	virtual ~DOMUnit();

	// DocumentHandler (via HandlerBase)
	void startElement(XMLString *name, AttributeList &attrs);
	void characters(XMLString *chars, unsigned length);
	void ignorableWhitespace(XMLString *chars, unsigned length);
	void endElement(XMLString *name);
	void processingInstruction(XMLString *target, XMLString *data);
	void startDocument();
	void endDocument();
	// ErrorHandler (via HandlerBase)
	void error(SAXParseException &exc);
	void fatalError(SAXParseException &exc);
	// LexicalHandler
	void comment(XMLString *chars, unsigned long start, unsigned long length);
	void startCDATA();
	void endCDATA();

	/* own methods (not virtual) */
	SAXParseException Status();
	void Reset();
	Document getDocument();
	void PreserveWhiteSpace(bool b);
	void InternalCheck(bool b);

protected:
	/* 0x1C */ Document fDocument;
	/* 0x24 */ Node fCurrentNode;
	/* 0x2C */ SAXParseException fStatus;
	/* 0x44 */ int fInCDATA;  /* inside a CDATA section */
	/* 0x48 */ int fEntityValue;  /* append as entity value node */
	/* 0x4C */ int fPreserveWhiteSpace;
};

#endif // GUARD_XML_DOMUNIT_H
