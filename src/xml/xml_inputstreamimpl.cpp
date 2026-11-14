/*
 * XMLInputStreamImpl: the character stream the scanners read, on top of an
 * XMLInputSource; single characters, names, white space and entity references.
 */

#pragma implementation "xml_inputstreamimpl.h"
#include "xml_locator.h"
#include "xml_inputstreamimpl.h"
#include "xml_unicodeutils.h"
#include "xml_locatorimpl.h"
#include "xml_inputsource.h"

XMLInputStreamImpl::XMLInputStreamImpl(XMLCodeType codeType, XMLInputSource *source)
{
	fUnused1C = 0;
	fState = new ScannerStateImpl();
	fOpenCondSections = 0;
	fUnused28 = 0;
	fEntityResolving = 1;
	fCodeType = codeType;
	fEntityRef = 0;
	fAmp = new XMLString("amp", false);
	fLt = new XMLString("lt", false);
	fGt = new XMLString("gt", false);
	fQuot = new XMLString("quot", false);
	fApos = new XMLString("apos", false);
	fSemi = new XMLString(";", false);
	/* bytes per character unit, as in XMLString::codetypeToAligntype */
	switch (codeType) {
	case 3:
	case 4:
	case 5:
	case 6:
		fWidth = 2;
		break;
	case 7:
	case 8:
	case 9:
	case 10:
		fWidth = 4;
		break;
	default:
		fWidth = 1;
	}
	if (source) {
		fSource = source;
		fLocator = source->fLocator;
	} else {
		fSource = 0;
		fLocator = 0;
	}
	fUntilBuf.SetCodeType((XMLCodeType)fCodeType);
	fMatchBuf.SetCodeType((XMLCodeType)fCodeType);
	fCharBuf.SetCodeType((XMLCodeType)fCodeType);
}

XMLInputStreamImpl::~XMLInputStreamImpl()
{
	delete fState;
	if (fEntityRef) {
		delete fEntityRef;
		fEntityRef = 0;
	}
	delete fSource;
	delete fAmp;
	delete fLt;
	delete fGt;
	delete fQuot;
	delete fApos;
	delete fSemi;
}

int XMLInputStreamImpl::SenseNextChar(XMLString *s)
{
	return readChar(s, true);
}
int XMLInputStreamImpl::GetNextChar(XMLString *s)
{
	return readChar(s, false);
}
int XMLInputStreamImpl::SkipNextChar()
{
	return readChar(&fCharBuf, false);
}
int XMLInputStreamImpl::SkipUntil(XMLString *s)
{
	return GetUntil(&fUntilBuf, s);
}

int XMLInputStreamImpl::GetCodeType()
{
	return fCodeType;
}

Locator *XMLInputStreamImpl::GetLocator()
{
	return fLocator;
}
void *XMLInputStreamImpl::GetSource()
{
	return fSource;
}
ScannerState *XMLInputStreamImpl::GetState()
{
	return fState;
}
int XMLInputStreamImpl::IsEntityReference()
{
	return (int)fEntityRef;
}

void XMLInputStreamImpl::EntityReference(XMLString *s)
{
	if (s) {
		delete fEntityRef;
		fEntityRef = s->Clone();
	} else {
		fEntityRef = 0;
	}
}
unsigned XMLInputStreamImpl::GetOpenCondSections()
{
	return fOpenCondSections;
}
void XMLInputStreamImpl::SetOpenCondSections(unsigned n)
{
	fOpenCondSections = n;
}
void XMLInputStreamImpl::DisableEntityResolving()
{
	fEntityResolving = 0;
}
int XMLInputStreamImpl::GetName(XMLString *s)
{
	return getNameAndNmtoken(s, true);
}
int XMLInputStreamImpl::GetNmtoken(XMLString *s)
{
	return getNameAndNmtoken(s, false);
}

int XMLInputStreamImpl::SkipWhiteSpace()
{
	long count = 0;
	return skipWhiteSpace(&count);
}
