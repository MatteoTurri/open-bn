/*
 * Methods of XMLDocumentScanner, the XML parser.
 */

#pragma implementation "xml_scanner.h"
#include "xml_scanner.h"
#include "xml_tokenscanner.h"
#include "xml_entitydeclmanager.h"
#include "xml_inputsource.h"
#include "xml_attributelist.h"
#include "xml_streammanager.h"
#include "xml_misc.h"

/*
 * The token loop: opens the source, then for each token updates the scanner state and
 * runs the scanner of the token (from the ScannerFactory), until XMLToken_EOF or a failed
 * scan. XMLToken_EOF releases the exhausted streams (end of an entity): reaching the last one
 * before the end state is XPE_UnexpectedEndOfInput. A '%' in the content is reported as
 * the characters "%".
 */
void XMLDocumentScanner::parse(XMLInputSource *src)
{
	fElementDepth = 0;
	fTrackDepth = 0;
	XMLStreamManager mgr(this);

	int result = mgr.ApplyInputStream(src);
	if ((result & 1) == 0)
		return;

	startDocument();
	int tok = XMLToken_Text;

	for (;;) {
		tok = mgr.GetCurrentStream()->GetNextToken();
		while (tok == XMLToken_EOF) {
			XMLInputStream *cur = mgr.GetCurrentStream();
			int done = 1;
			if (cur)
				done = cur->GetState()->GetState() == XSS_Epilog;
			if (!done && mgr.LastInputStream()) {
				fErrorProcessor->TreatErrorState(XPE_UnexpectedEndOfInput, mgr.GetCurrentStream());
				mgr.ReleaseAllInputStream();
				return;
			}
			if (!mgr.ReleaseInputStream())
				break;
			tok = mgr.GetCurrentStream()->GetNextToken();
		}

		if (mgr.GetCurrentStream()) {
			if (mgr.GetCurrentStream()->GetState()->Update((XMLToken *)&tok)) {
				ScannerFactory *factory = mgr.fFactory;
				void *scanner = factory->Create((XMLToken)tok);
				int entityParse = mgr.isEntityParse();
				fTrackDepth = entityParse;
				if (fElementDepth > 0 && !entityParse) {
					fErrorProcessor->TreatErrorState(XPE_ElementDepthMismatch,
					    mgr.GetCurrentStream());
					mgr.ReleaseAllInputStream();
					return;
				}
				if (scanner)
					result = ((TokenScanner *)scanner)->Scan(mgr.GetCurrentStream(), &mgr);
			} else {
				XMLParseErrorState err = XPE_UnknownRootScanState;
				result = 0;
				if (mgr.GetCurrentStream()->GetState()->GetState() == XSS_Unsupported)
					err = XPE_InvalidTokenState;
				fErrorProcessor->TreatErrorState(err, mgr.GetCurrentStream());
			}
		} else
			break;

		if (tok == XMLToken_PERef && mgr.GetCurrentStream()->GetState()->GetState() == XSS_Content)
			characters(fPercentText, 1, &mgr);

		if (mgr.GetCurrentStream())
			mgr.GetCurrentStream()->GetState()->Update((XMLScanResult)result);

		if (tok == XMLToken_EOF || (result & 1) == 0)
			break;
	}

	if ((result & 1) == 0)  /* redundant as written */
		mgr.ReleaseAllInputStream();
	if (result & 1)
		endDocument();
	else
		mgr.ReleaseAllInputStream();
}

inline void XMLDocumentScanner::setDocumentHandler(DocumentHandler *h)
{
	fDocumentHandler = h;
}

inline bool XMLDocumentScanner::success(XMLScanResult r)
{
	return r & 1;
}

inline XMLDocumentScanner::XMLDocumentScanner()
{
	fElementDepth = 0;
	fTrackDepth = 0;
	fPercentSpare = new XMLString("%", false);
	fDocumentHandler = 0;
	fDeclHandler = 0;
	fLexicalHandler = 0;
	fEntityResolver = 0;
	fErrorHandler = 0;
	fDTDHandler = 0;
	fErrorProcessor = new ParseErrorProcessor();
	fPercentText = new XMLString("%", false);
}

inline XMLDocumentScanner::~XMLDocumentScanner()
{
	delete fPercentSpare;
	delete fErrorProcessor;
	delete fPercentText;
}
/* Parses an XMLString buffer through a temporary descriptor. */
inline void XMLDocumentScanner::parse(XMLString *src, XMLIOType type, XMLCodeType codeType)
{
	XMLFileDescriptor fd;
	fd.fIOType = type;
	fd.fBuffer = src;
	fd.fIsEntity = 0;
	fd.fHasBufferCode = 0;
	fd.fCodeType = codeType;
	if (type == XMLIOType_Memory)
		fd.fSize = src->ByteLength();
	parse(&fd);
}
/* Parses a C string through a temporary XMLString and descriptor. */
inline void XMLDocumentScanner::parse(char *text, XMLIOType type, XMLCodeType codeType)
{
	XMLFileDescriptor fd;
	fd.fIOType = type;
	XMLString buf(text, false);
	fd.fBuffer = &buf;
	fd.fIsEntity = 0;
	fd.fHasBufferCode = 0;
	fd.fCodeType = codeType;
	if (fd.fIOType == XMLIOType_Memory)
		fd.fSize = fd.fBuffer->ByteLength();
	parse(&fd);
}
/* Opens an XMLInputSource on fd and parses it; XPE_CannotOpenSource if it does not open. */
inline void XMLDocumentScanner::parse(XMLFileDescriptor *fd)
{
	XMLInputSource *src = new XMLInputSource(fd);
	if (!src->fIsOpen) {
		delete src;
		TreatErrorState(XPE_CannotOpenSource, 0);
	} else {
		parse(src);
	}
}

inline void XMLDocumentScanner::setErrorHandler(ErrorHandler *h)
{
	fErrorHandler = h;
	fErrorProcessor->SetErrorHandler(h);
}

inline void XMLDocumentScanner::endElement(XMLString *name, XMLStreamManager *)
{
	if (fTrackDepth)
		fElementDepth--;
	if (fDocumentHandler)
		fDocumentHandler->endElement(name);
}

inline void XMLDocumentScanner::startElement(XMLString *name, AttributeList &attrs, XMLStreamManager *)
{
	if (fTrackDepth)
		fElementDepth++;
	if (fDocumentHandler) {
		unsigned n = attrs.getLength();
		for (unsigned i = 0; i < n; i++)
			;
		fDocumentHandler->startElement(name, attrs);
	}
}

inline void XMLDocumentScanner::startDocument()
{
	if (fDocumentHandler)
		fDocumentHandler->startDocument();
}
inline void XMLDocumentScanner::endDocument()
{
	if (fDocumentHandler)
		fDocumentHandler->endDocument();
}
inline void XMLDocumentScanner::characters(XMLString *chars, unsigned, XMLStreamManager *)
{
	if (fDocumentHandler)
		fDocumentHandler->characters(chars, chars->ByteLength());
}
inline void XMLDocumentScanner::comment(XMLString *chars, unsigned, XMLStreamManager *)
{
	if (fLexicalHandler)
		fLexicalHandler->comment(chars, 0, chars->ByteLength());
}
inline void XMLDocumentScanner::endCDATA()
{
	if (fLexicalHandler)
		fLexicalHandler->endCDATA();
}
inline void XMLDocumentScanner::endDTD()
{
	if (fLexicalHandler)
		fLexicalHandler->endDTD();
}
inline void XMLDocumentScanner::endEntity(XMLString *name, XMLStreamManager *)
{
	if (fLexicalHandler)
		fLexicalHandler->endEntity(name);
}
inline void XMLDocumentScanner::startCDATA()
{
	if (fLexicalHandler)
		fLexicalHandler->startCDATA();
}
inline void XMLDocumentScanner::startDTD(XMLString *name, XMLString *pub, XMLString *sys, XMLStreamManager *)
{
	if (fLexicalHandler)
		fLexicalHandler->startDTD(name, pub, sys);
}
inline void XMLDocumentScanner::startEntity(XMLString *name, XMLStreamManager *)
{
	if (fLexicalHandler)
		fLexicalHandler->startEntity(name);
}
inline void XMLDocumentScanner::attributeDecl(XMLString *eName, XMLString *aName, XMLString *type, XMLString *mode,
    XMLString *value, XMLStreamManager *)
{
	if (fDeclHandler)
		fDeclHandler->attributeDecl(eName, aName, type, mode, value);
}

inline void XMLDocumentScanner::externalEntityDecl(XMLString *name, XMLString *publicId, XMLString *systemId,
    XMLStreamManager *)
{
	if (fDeclHandler)
		fDeclHandler->externalEntityDecl(name, publicId, systemId);
}

inline void XMLDocumentScanner::internalEntityDecl(XMLString *name, XMLString *value, XMLStreamManager *)
{
	if (fDeclHandler)
		fDeclHandler->internalEntityDecl(name, value);
}

inline void XMLDocumentScanner::ignorableWhitespace(XMLString *chars, unsigned, XMLStreamManager *)
{
	if (fDocumentHandler)
		fDocumentHandler->ignorableWhitespace(chars, chars->ByteLength());
}

inline void XMLDocumentScanner::processingInstruction(XMLString *target, XMLString *data, XMLStreamManager *)
{
	if (fDocumentHandler)
		fDocumentHandler->processingInstruction(target, data);
}

inline void XMLDocumentScanner::notationDecl(XMLString *, XMLString *, XMLString *, XMLStreamManager *)
{}

inline void XMLDocumentScanner::unparsedEntityDecl(XMLString *, XMLString *, XMLString *, XMLString *,
    XMLStreamManager *)
{}

inline void XMLDocumentScanner::TreatErrorState(XMLParseErrorState state, XMLInputStream *stream)
{
	fErrorProcessor->TreatErrorState(state, stream);
}
