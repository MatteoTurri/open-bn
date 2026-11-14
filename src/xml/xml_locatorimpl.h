/*
 * LocatorImpl: the Locator of an input stream (public and system id, line and column).
 */

#ifndef GUARD_XML_LOCATORIMPL_H
#define GUARD_XML_LOCATORIMPL_H
#include "xml_locator.h"

class LocatorImpl : public Locator {
	/* XMLStreamManager::createInputStream resets line and column to 1 */
	friend class XMLStreamManager;

public:
	LocatorImpl(XMLString *publicId, XMLString *systemId);
	virtual ~LocatorImpl() {}
	XMLString *getPublicId() { return fPublicId; }
	XMLString *getSystemId() { return fSystemId; }
	unsigned getLineNumber() { return fLineNumber; }
	unsigned getColumnNumber() { return fColumnNumber; }

private:
	/* 0x04 */ XMLString *fPublicId;
	/* 0x08 */ XMLString *fSystemId;
	/* 0x0C */ unsigned fLineNumber;
	/* 0x10 */ unsigned fColumnNumber;
};

#endif // GUARD_XML_LOCATORIMPL_H
