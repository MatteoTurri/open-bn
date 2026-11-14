/*
 * ETagScanner: scanner of the end tags </name>.
 */

#include "xml_filedescriptor.h"
#include "xml_locator.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#include "xml_scanner.h"
#include "xml_tokenscanner.h"
#include "xml_validator.h"
#include "xml_scanners.h"

int ETagScanner::Scan(XMLInputStream *stream, XMLStreamManager *mgr)
{
	XMLString *name = &fName;

	if ((stream->GetName(name) & 1) == 0) {
		fErrorProcessor->TreatErrorState(XPE_ETagNameExpected, stream);
		return 0;
	}
	stream->SkipWhiteSpace();

	XMLString *ch = &fChar;
	int r = stream->GetNextChar(ch);

	if ((r & 1) == 0 || *ch != (XMLChar)'>') {
		fErrorProcessor->TreatErrorState(XPE_ETagGtExpected, stream);
		return 0;
	}
	if (fValidator != 0) {
		int st = fValidator->ValidateTag(XMLToken_ETag, name);

		if (st != 0) {
			if (st == XPE_ETagNotAllowed)  /* redundant as written */
				fErrorProcessor->TreatErrorState(XPE_ETagNotAllowed, stream);
			else
				fErrorProcessor->TreatErrorState((XMLParseErrorState)st, stream);
			return 0;
		}
		fDocScanner->endElement(name, mgr);
		if (fValidator->EofAcceptable(true))
			return XSR_RootClosed;
	}
	return r;
}

ETagScanner::ETagScanner(XMLDocumentScanner *ds, Validator *v) : TokenScanner(ds)
{
	fValidator = v;
}

ETagScanner::~ETagScanner()
{}
