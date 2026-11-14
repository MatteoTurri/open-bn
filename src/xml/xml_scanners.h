/*
 * The token scanners of the XML parser, one per construct: start tag, end tag, CDATA section,
 * character data, comment, processing instruction, white space.
 */

#ifndef GUARD_XML_SCANNERS_H
#define GUARD_XML_SCANNERS_H
#include "xml_tokenscanner.h"
#include "xml_attributelistimpl.h"
#include "xml_string.h"

class Validator;
class EntityScanner;

class STagScanner : public TokenScanner {
public:
	STagScanner(XMLDocumentScanner *ds, Validator *v, EntityScanner *es);
	virtual ~STagScanner();
	virtual int Scan(XMLInputStream *stream, XMLStreamManager *mgr);
	void InitPE(XMLInputStream *stream);

private:
	/* 0x0C */ Validator *fValidator;
	/* 0x10 */ EntityScanner *fEntityScan;
	/* 0x14 */ XMLString fStopSet;  /* "<&": stop set of GetNextPortion */
	/* 0x30 */ XMLString fStopAlt;  /* "": used once the stream has changed */
	/* 0x4C */ XMLString fCDATAType;  /* "CDATA": type of the attributes */
	/* 0x68 */ XMLString fName;
	/* 0x84 */ XMLString fChar;
	/* 0xA0 */ XMLString fAttrName;
	/* 0xBC */ XMLString fValue;
	/* 0xD8 */ XMLString fType;
	/* 0xF4 */ XMLString fQuote;
	/* 0x110 */ XMLString fFrag;
	/* 0x12C */ XMLString fSpare;
	/* 0x148 */ bool fStandalone;
	/* 0x14C */ AttributeListImpl fAttrs;
};

class ETagScanner : public TokenScanner {
public:
	ETagScanner(XMLDocumentScanner *ds, Validator *v);
	virtual ~ETagScanner();
	virtual int Scan(XMLInputStream *stream, XMLStreamManager *mgr);

private:
	/* 0x0C */ Validator *fValidator;
	/* 0x10 */ XMLString fName;
	/* 0x2C */ XMLString fChar;
};

class CDATAScanner : public TokenScanner {
public:
	CDATAScanner(XMLDocumentScanner *ds, Validator *v);
	virtual ~CDATAScanner();
	virtual int Scan(XMLInputStream *stream, XMLStreamManager *mgr);

private:
	/* 0x0C */ XMLString fEndMark;  /* "]]>" */
	/* 0x28 */ XMLString fBuf;
	/* 0x44 */ Validator *fValidator;
};

class CharacterDataScanner : public TokenScanner {
public:
	CharacterDataScanner(XMLDocumentScanner *ds, Validator *v);
	virtual ~CharacterDataScanner();
	virtual int Scan(XMLInputStream *stream, XMLStreamManager *mgr);

private:
	/* 0x0C */ XMLString fStopB;  /* "]<" */
	/* 0x28 */ XMLString fStopA;  /* "&" */
	/* 0x44 */ Validator *fValidator;
	/* 0x48 */ XMLString fBuf;
	/* 0x64 */ XMLString fChar;
};

class CommentScanner : public TokenScanner {
public:
	CommentScanner(XMLDocumentScanner *ds);
	virtual ~CommentScanner();
	virtual int Scan(XMLInputStream *stream, XMLStreamManager *mgr);

private:
	/* 0x0C */ XMLString fEndMark;  /* "--" */
	/* 0x28 */ XMLString fBuf;
	/* 0x44 */ XMLString fChar;
};

class PIScanner : public TokenScanner {
public:
	PIScanner(XMLDocumentScanner *ds);
	virtual ~PIScanner();
	virtual int Scan(XMLInputStream *stream, XMLStreamManager *mgr);

private:
	/* 0x0C */ XMLString fEndMark;  /* "?>" */
	/* 0x28 */ XMLString fChar;
	/* 0x44 */ XMLString fTarget;
	/* 0x60 */ XMLString fBuf;
};

class SScanner : public TokenScanner {
public:
	SScanner(XMLDocumentScanner *ds);
	virtual ~SScanner();
	virtual int Scan(XMLInputStream *stream, XMLStreamManager *mgr);
};

#endif // GUARD_XML_SCANNERS_H
