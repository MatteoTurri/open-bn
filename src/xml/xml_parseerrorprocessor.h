/*
 * ParseErrorProcessor: turns the error codes of the scanners into a SAXParseException
 * for the SAX ErrorHandler. Also defines the error codes.
 */

#ifndef GUARD_XML_PARSEERRORPROCESSOR_H
#define GUARD_XML_PARSEERRORPROCESSOR_H
#include "xml_mmobject.h"
#include "xml_string.h"

class XMLInputStream;
class ErrorHandler;

/* Error codes, grouped by the scanner that reports them. */
enum XMLParseErrorState {
	/* XMLDocumentScanner (xml_scanner.cpp) */
	XPE_CannotOpenSource = 0x80fe,
	XPE_UnexpectedEndOfInput = 0x80e9,  /* last input stream but the token is still XMLToken_EOF */
	XPE_UnknownRootScanState = 0x8108,  /* ScannerState::GetState() != XSS_Unsupported after a failed Update */
	XPE_InvalidTokenState = 0x8118,     /* ScannerState::GetState() == XSS_Unsupported after a failed Update */
	XPE_ElementDepthMismatch = 0x8158,  /* fElementDepth > 0 outside an entity parse */
	/* XMLStreamManager (xml_streammanager.cpp) */
	XPE_SourceAlreadyOpen = 0x8155, /* ApplyInputStream(XMLString*, bool, bool): source already open on the stack */
	/* token scanners */
	XPE_ETagNameExpected = 0x80ea,
	XPE_PITargetExpected = 0x80ec,
	XPE_PITargetInvalid = 0x80ed,
	XPE_ETagGtExpected = 0x80f4,
	XPE_CommentUnterminated = 0x80fb,
	XPE_CDataUnterminated = 0x80fc,
	XPE_CharDataBadStart = 0x8110,
	XPE_CharDataCDataEnd = 0x8156,
	XPE_CommentGtExpected = 0x80ff,
	XPE_PIUnterminated = 0x8100,
	XPE_PIBadContent = 0x8108,
	XPE_PINoSpaceAfterTgt = 0x8109,
	XPE_ETagNotAllowed = 0x814e,
	XPE_CharRefUnresolved = 0x8152,
	/* STagScanner (xml_stagscanner.cpp) */
	XPE_AttrNameExpected = 0x80ee,
	XPE_AttrEqExpected = 0x80f7,
	XPE_AttrQuoteExpected = 0x810a,
	XPE_AttrDuplicate = 0x814f,
	XPE_AttrExternalEntity = 0x8150,
	XPE_AttrLtInValue = 0x8151,
	/* EntityScanner (xml_entityscanner.cpp) */
	XPE_EntityNotDeclared = 0x8118,
	XPE_CharRefBadDigit = 0x80e9,
	XPE_CharRefInvalid = 0x8105,
	XPE_CharRefNoSemi = 0x80f8,
	XPE_EntityNameNoSemi = 0x80f9,
	XPE_EntityUnresolved = 0x8153
};

class ParseErrorProcessor : public XMLMMObject {
public:
	ParseErrorProcessor();
	virtual ~ParseErrorProcessor();
	void SetErrorHandler(ErrorHandler *eh);
	XMLString GetErrorString(XMLParseErrorState state);
	void TreatErrorState(XMLParseErrorState state, XMLInputStream *stream);

protected:
	/* 0x04 */ ErrorHandler *fErrorHandler;
};

#endif // GUARD_XML_PARSEERRORPROCESSOR_H
