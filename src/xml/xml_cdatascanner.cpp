/*
 * CDATAScanner: scanner of the CDATA sections.
 */

#include "xml_scanner.h"
#include "xml_tokenscanner.h"
#include "xml_scanners.h"

CDATAScanner::CDATAScanner(XMLDocumentScanner *ds, Validator *v) : TokenScanner(ds), fEndMark("]]>", false)
{
	fValidator = v;
}

CDATAScanner::~CDATAScanner()
{}

/* Reads the section up to the end mark into fBuf, then hands it to the document handler. */
int CDATAScanner::Scan(XMLInputStream *stream, XMLStreamManager *mgr)
{
	fDocScanner->startCDATA();

	XMLString *buf = &fBuf;

	buf->Clear();
	int r = stream->GetUntil(buf, &fEndMark);
	if (r & 1) {
		fDocScanner->characters(buf, buf->fByteLength, mgr);
		fDocScanner->endCDATA();
	} else {
		fErrorProcessor->TreatErrorState(r == 0x1000 ? XPE_CharRefUnresolved : XPE_CDataUnterminated, stream);
	}
	return r;
}
