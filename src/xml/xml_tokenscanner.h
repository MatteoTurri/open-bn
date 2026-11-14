/*
 * TokenScanner: abstract base of the token scanners (one per construct of the document).
 */

#ifndef GUARD_XML_TOKENSCANNER_H
#define GUARD_XML_TOKENSCANNER_H
#include "xml_filedescriptor.h"
#include "xml_parser.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#include "xml_mmobject.h"
#include "xml_string.h"

class XMLDocumentScanner;
class XMLStreamManager;
class ParseErrorProcessor;

class TokenScanner : public XMLMMObject {
public:
	TokenScanner(XMLDocumentScanner *ds);
	virtual int Scan(XMLInputStream *stream, XMLStreamManager *mgr) = 0;

	/* never read */
	static int PEReference;

protected:
	/* 0x04 */ XMLDocumentScanner *fDocScanner;
	/* 0x08 */ ParseErrorProcessor *fErrorProcessor;
};

#endif // GUARD_XML_TOKENSCANNER_H
