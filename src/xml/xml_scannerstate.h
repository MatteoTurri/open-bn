/*
 * ScannerState: the state of the scanner attached to each XMLInputStream, with ScannerStateImpl.
 */

#ifndef GUARD_XML_SCANNERSTATE_H
#define GUARD_XML_SCANNERSTATE_H
#include "xml_mmobject.h"

/* Token of the scanner, as recognised by XMLUTF8Stream::GetNextToken (other values unused). */
enum XMLToken {
	XMLToken_None = 0,
	XMLToken_AttlistDecl = 2,   /* "<!ATTLIST" */
	XMLToken_CDataSect = 3,     /* "<![CDATA[" */
	XMLToken_CharData = 4,      /* text in the content of an element (set by ScannerStateImpl) */
	XMLToken_Doctype = 5,       /* "<!DOCTYPE" */
	XMLToken_ElementDecl = 6,   /* "<!ELEMENT" */
	XMLToken_DeclEnd = 8,       /* '>' while resolving entities */
	XMLToken_CDataEnd = 9,      /* "]]>" */
	XMLToken_SubsetEnd = 10,    /* ']' spaces '>' */
	XMLToken_EntityDecl = 11,   /* "<!ENTITY" */
	XMLToken_EntityRef = 12,    /* '&' and a non-predefined entity name */
	XMLToken_ETag = 14,         /* "</" */
	XMLToken_CondSect = 19,     /* "<![" */
	XMLToken_NotationDecl = 24, /* "<!NOTATION" */
	XMLToken_PERef = 25,        /* '%' */
	XMLToken_PI = 26,           /* "<?" */
	XMLToken_STag = 29,         /* '<' and a name */
	XMLToken_Space = 30,        /* white space */
	XMLToken_Text = 32,         /* any other character */
	XMLToken_Comment = 41,      /* "<!--" */
	XMLToken_EOF = 44           /* end of the input stream */
};

/* State of ScannerStateImpl (2 and 3 unused). */
enum XMLScannerState {
	XSS_Rejected,      /* a token not allowed in the state */
	XSS_Prolog,        /* before the root element */
	XSS_2,
	XSS_3,
	XSS_Content,       /* inside the root element */
	XSS_Epilog,        /* after the root element */
	XSS_Unsupported    /* a DOCTYPE, or '>' in the content */
};

/* Result of a token scanner: bit 0 = success. */
enum XMLScanResult {
	XSR_Failed = 0,
	XSR_RootClosed = 17  /* success, and the close tag ended the root element */
};

/* Pure interface. */
class ScannerState : public XMLMMObject {
public:
	virtual int Update(XMLScanResult r) = 0;
	virtual int Update(XMLToken *tok) = 0;
	virtual XMLScannerState GetState() = 0;
	virtual void SetState(XMLScannerState s) = 0;
};

/* The concrete state, created by the constructor of XMLInputStreamImpl. */
class ScannerStateImpl : public ScannerState {
public:
	ScannerStateImpl();
	virtual ~ScannerStateImpl();
	int Update(XMLScanResult r);
	int Update(XMLToken *tok);
	XMLScannerState GetState();
	void SetState(XMLScannerState s);

protected:
	/* 0x04 */ int fState;
};

#endif // GUARD_XML_SCANNERSTATE_H
