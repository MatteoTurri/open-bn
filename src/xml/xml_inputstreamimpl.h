/*
 * XMLInputStreamImpl: partial XMLInputStream (accessors and the dispatch to three
 * reading primitives, slots 25-27, left to the concrete streams).
 */

#ifndef GUARD_XML_INPUTSTREAMIMPL_H
#define GUARD_XML_INPUTSTREAMIMPL_H
#pragma interface
#include "xml_filedescriptor.h"
#include "xml_locator.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#include "xml_string.h"

class Locator;
class XMLInputSource;
class ScannerStateImpl;

class XMLInputStreamImpl : public XMLInputStream {
	friend class CharacterDataScanner;  /* reads the names of the predefined entities */
public:
	XMLInputStreamImpl(XMLCodeType codeType, XMLInputSource *source);
	~XMLInputStreamImpl();

	int GetName(XMLString *s);
	int GetNmtoken(XMLString *s);
	Locator *GetLocator();
	void *GetSource();
	int SkipWhiteSpace();
	int SenseNextChar(XMLString *s);
	int SkipNextChar();
	int GetNextChar(XMLString *s);
	int SkipUntil(XMLString *s);
	int GetCodeType();
	ScannerState *GetState();
	unsigned GetOpenCondSections();
	void SetOpenCondSections(unsigned n);
	void DisableEntityResolving();
	int IsEntityReference();
	void EntityReference(XMLString *s);

	virtual int skipWhiteSpace(long *count) = 0;           // slot 25
	virtual int readChar(XMLString *str, bool rewind) = 0; // slot 26
	virtual int getNameAndNmtoken(XMLString *str, bool isName) = 0; // slot 27

protected:
	/* 0x04 */ XMLString *fAmp;
	/* 0x08 */ XMLString *fLt;
	/* 0x0C */ XMLString *fGt;
	/* 0x10 */ XMLString *fQuot;
	/* 0x14 */ XMLString *fApos;
	/* 0x18 */ XMLString *fSemi;
	/* 0x1C */ int fUnused1C;  /* zeroed by the constructor, never read */
	/* 0x20 */ Locator *fLocator;
	/* 0x24 */ ScannerStateImpl *fState;
	/* 0x28 */ int fUnused28;  /* zeroed by the constructor, never read */
	/* 0x2C */ unsigned fOpenCondSections;
	/* 0x30 */ int fEntityResolving;
	/* 0x34 */ XMLInputSource *fSource;
	/* 0x38 */ XMLString *fEntityRef;
	/* 0x3C */ int fCodeType;
	/* 0x40 */ int fWidth;
	/* 0x44 */ XMLString fUntilBuf;
	/* 0x60 */ XMLString fMatchBuf;
	/* 0x7C */ XMLString fCharBuf;
};

#endif // GUARD_XML_INPUTSTREAMIMPL_H
