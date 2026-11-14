/*
 * PIScanner: scanner of the processing instructions <?target data?>.
 */

#include "xml_scanner.h"
#include "xml_tokenscanner.h"
#include "xml_scanners.h"

/*
 * Reads the target and the data up to "?>". Data that does not start after white space
 * is an error (0x8109).
 */
int PIScanner::Scan(XMLInputStream *stream, XMLStreamManager *mgr)
{
	int r = stream->GetName(&fTarget);

	if ((r & 1) == 0) {
		fErrorProcessor->TreatErrorState(XPE_PITargetExpected, stream);
		return r;
	}
	if (!fTarget.IsValidPITarget()) {
		fErrorProcessor->TreatErrorState(XPE_PITargetInvalid, stream);
		return 0;
	}

	int ws = stream->SkipWhiteSpace();

	r = stream->GetUntil(&fBuf, &fEndMark);
	if (r & 1) {
		if (fBuf.CharLength() != 0 && (ws & 1) == 0) {
			fErrorProcessor->TreatErrorState(XPE_PINoSpaceAfterTgt, stream);
			return 0;
		}
		fDocScanner->processingInstruction(&fTarget, &fBuf, mgr);

		stream->SenseNextChar(&fChar);
		if (fChar == (XMLChar)'\n' || fChar == (XMLChar)'\r') {
			stream->SkipNextChar();
			if (fChar == (XMLChar)'\r') {
				stream->SenseNextChar(&fChar);
				if (fChar == (XMLChar)'\n')
					stream->SkipNextChar();
			}
		}
		return 1;
	}
	switch (r) {
	case 16:
		fErrorProcessor->TreatErrorState(XPE_PIUnterminated, stream);
		return 0;
	case 0x1000:
		fErrorProcessor->TreatErrorState(XPE_CharRefUnresolved, stream);
		break;
	default:
		fErrorProcessor->TreatErrorState(XPE_PIBadContent, stream);
		break;
	}
	return r;
}

PIScanner::PIScanner(XMLDocumentScanner *ds) : TokenScanner(ds), fEndMark("?>", false)
{}

PIScanner::~PIScanner()
{}
