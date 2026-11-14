/*
 * Parser: the SAX parser interface, implemented by XMLDocumentScanner.
 */

#ifndef GUARD_XML_PARSER_H
#define GUARD_XML_PARSER_H
#include "xml_filedescriptor.h"
#include "xml_documenthandler.h"
#include "xml_entityresolver.h"
#include "xml_errorhandler.h"
#include "xml_dtdhandler.h"
#include "xml_mmobject.h"
#include "xml_string.h"
#include "xml_outputsource.h"

class InputSource;
class XMLInputSource;

class Parser : public XMLMMObject {
public:
	Parser() {}
	virtual ~Parser() {}
	virtual void parse(XMLInputSource *src) = 0;
	virtual void parse(InputSource &src) = 0;
	virtual void parse(XMLString *src, XMLIOType type, XMLCodeType codeType) = 0;
	virtual void parse(XMLFileDescriptor *fd) = 0;
	virtual void setDocumentHandler(DocumentHandler *h) = 0;
	virtual void setEntityResolver(EntityResolver *h) = 0;
	virtual void setErrorHandler(ErrorHandler *h) = 0;
	virtual void setDTDHandler(DTDHandler *h) = 0;
	virtual void parse(char *text, XMLIOType type, XMLCodeType codeType) = 0;
};

#endif // GUARD_XML_PARSER_H
