/*
 * Locator: SAX interface for the current position in the document.
 */

#ifndef GUARD_XML_LOCATOR_H
#define GUARD_XML_LOCATOR_H
#include "xml_mmobject.h"

class XMLString;

class Locator : public XMLMMObject {
public:
	virtual ~Locator() {}
	virtual XMLString *getPublicId() = 0;
	virtual XMLString *getSystemId() = 0;
	virtual unsigned getLineNumber() = 0;
	virtual unsigned getColumnNumber() = 0;
};

#endif // GUARD_XML_LOCATOR_H
