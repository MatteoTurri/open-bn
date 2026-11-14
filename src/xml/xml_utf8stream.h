/*
 * XMLUTF8Stream: the UTF-8 input stream created by XMLStreamManager::createInputStream.
 */

#ifndef GUARD_XML_UTF8STREAM_H
#define GUARD_XML_UTF8STREAM_H
#pragma interface
#include "xml_inputstreamimpl.h"

class XMLUTF8Stream : public XMLInputStreamImpl {
public:
	XMLUTF8Stream(XMLCodeType codeType, XMLInputSource *source);
	~XMLUTF8Stream();

	unsigned GetXMLReferenceChar(XMLString *str, XMLChar ch);
	void GetXMLReferenceChar(XMLString *str, unsigned long code);
	void GetXMLReservedChar(XMLString *str, XMLChar ch);
	int skipWhiteSpace(long *count);
	int GetNextHex(int *result);
	int readChar(XMLString *str, bool rewind);
	int GetNextInteger(int *result);
	int getNameAndNmtoken(XMLString *str, bool isName);
	int GetUntil(XMLString *dst, XMLString *marker);
	int GetNextToken();
	int GetNextPortion(XMLString *dst, XMLString *stopSet, XMLString *notSet);
};

#endif // GUARD_XML_UTF8STREAM_H
