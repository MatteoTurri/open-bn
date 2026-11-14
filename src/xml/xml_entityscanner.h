/*
 * EntityScanner: scanner of the entity references; resolution through the EntityDeclManager.
 */

#ifndef GUARD_XML_ENTITYSCANNER_H
#define GUARD_XML_ENTITYSCANNER_H
#pragma interface
#include "xml_tokenscanner.h"
#include "xml_mmobject.h"
#include "xml_string.h"
#include "xml_entitydeclmanager.h"

class XMLDocumentScanner;
class EntityDeclManager;
class EntityDeclaration;
class XMLInputStream;
class XMLStreamManager;
class ParseErrorProcessor;

class EntityScanner : public TokenScanner {
public:
	EntityScanner(XMLDocumentScanner *ds, EntityDeclManager *edm);
	virtual ~EntityScanner();
	int Resolve(XMLInputStream *stream, XMLStreamManager *mgr, bool parameterEnt);
	int scan(EntityDeclaration **decl, XMLInputStream *stream, XMLStreamManager *mgr, bool parameterEnt,
	    bool nameOnly);
	virtual int Scan(XMLInputStream *stream, XMLStreamManager *mgr);
	int ScanAndResolve(EntityDeclaration **decl, XMLInputStream *stream, XMLStreamManager *mgr, bool parameterEnt,
	    bool nameOnly);
	int IsBuiltInEntity(XMLString *name);

protected:
	/* 0x0C */ EntityDeclManager *fEntityDeclManager;
	/* "x": after "&#" tells a hex reference (&#x...) from a decimal one */
	/* 0x10 */ XMLString fHexMarker;
	/* 0x2C */ XMLString fName;
	/* 0x48 */ XMLString fChar;
};

#endif // GUARD_XML_ENTITYSCANNER_H
