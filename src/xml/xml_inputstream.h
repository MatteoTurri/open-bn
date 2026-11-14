/*
 * XMLInputStream: abstract input stream of the XML scanners (24 pure virtuals).
 */

#ifndef GUARD_XML_INPUTSTREAM_H
#define GUARD_XML_INPUTSTREAM_H
#include "xml_mmobject.h"
#include "xml_string.h"

class Locator;
class ScannerState;

class XMLInputStream : public XMLMMObject {
public:
	virtual ~XMLInputStream() {}
	virtual int GetName(XMLString *s) = 0;            // slot 1
	virtual int GetNmtoken(XMLString *s) = 0;         // slot 2
	virtual Locator *GetLocator() = 0;                // slot 3
	virtual void *GetSource() = 0;                    // slot 4
	virtual int SkipWhiteSpace() = 0;                 // slot 5
	virtual int SenseNextChar(XMLString *s) = 0;      // slot 6
	virtual int SkipNextChar() = 0;                   // slot 7
	virtual int GetNextChar(XMLString *s) = 0;        // slot 8
	virtual int SkipUntil(XMLString *s) = 0;          // slot 9
	virtual int GetUntil(XMLString *dst, XMLString *marker) = 0;   // slot 10
	virtual int GetNextInteger(int *result) = 0;                   // slot 11
	virtual int GetNextHex(int *result) = 0;                       // slot 12
	virtual int GetNextPortion(XMLString *dst, XMLString *stopSet, XMLString *notSet) = 0; // slot 13
	virtual int GetCodeType() = 0;                                 // slot 14
	virtual int GetNextToken() = 0;                                // slot 15
	virtual void GetXMLReservedChar(XMLString *str, XMLChar ch) = 0; // slot 16
	virtual void GetXMLReferenceChar(XMLString *str, unsigned long code) = 0; // slot 17
	virtual unsigned GetXMLReferenceChar(XMLString *str, XMLChar ch) = 0;    // slot 18
	virtual ScannerState *GetState() = 0;             // slot 19
	virtual unsigned GetOpenCondSections() = 0;       // slot 20
	virtual void SetOpenCondSections(unsigned n) = 0; // slot 21
	virtual void DisableEntityResolving() = 0;        // slot 22
	virtual int IsEntityReference() = 0;              // slot 23
	virtual void EntityReference(XMLString *s) = 0;   // slot 24
};

#endif // GUARD_XML_INPUTSTREAM_H
