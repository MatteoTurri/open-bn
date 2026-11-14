/*
 * SAXException: SAX exception with a message (an owned XMLString copy).
 */

#ifndef GUARD_XML_SAXEXCEPTION_H
#define GUARD_XML_SAXEXCEPTION_H
#pragma interface
#include "xml_mmobject.h"
#include "xml_string.h"

class SAXException : public XMLMMObject {
public:
	SAXException();
	SAXException(XMLString *message);
	SAXException(SAXException &other);
	SAXException(XMLString *systemId, SAXException other);
	virtual ~SAXException();
	SAXException &operator=(SAXException &other);
	XMLString *getMessage();
	SAXException getException();

protected:
	/* 0x04 */ XMLString *fMessage;
};

#endif // GUARD_XML_SAXEXCEPTION_H
