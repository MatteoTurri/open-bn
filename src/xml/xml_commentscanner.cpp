/*
 * CommentScanner: scanner of the comments <!-- ... -->.
 */

#include "xml_scanner.h"
#include "xml_tokenscanner.h"
#include "xml_scanners.h"

/*
 * Reads up to "--" and hands the comment to the document handler before checking
 * the result. After the final '>' a CR, LF or CR LF is skipped.
 */
int CommentScanner::Scan(XMLInputStream *stream, XMLStreamManager *mgr)
{
	XMLString *buf = &fBuf;

	buf->Clear();

	int r = stream->GetUntil(buf, &fEndMark);

	fDocScanner->comment(buf, buf->fByteLength, mgr);
	if ((r & 1) == 0) {
		fErrorProcessor->TreatErrorState(r == 0x1000 ? XPE_CharRefUnresolved : XPE_CommentUnterminated, stream);
		return r;
	}

	XMLString *ch = &fChar;

	stream->SenseNextChar(ch);
	if (*ch != (XMLChar)'>') {
		fErrorProcessor->TreatErrorState(XPE_CommentGtExpected, stream);
		r = 0;
	} else {
		r = stream->SkipNextChar();
		stream->SenseNextChar(ch);
		if (*ch == (XMLChar)'\n' || *ch == (XMLChar)'\r') {
			stream->SkipNextChar();
			if (*ch == (XMLChar)'\r') {
				stream->SenseNextChar(ch);
				if (*ch == (XMLChar)'\n')
					stream->SkipNextChar();
			}
		}
	}
	return r;
}

CommentScanner::CommentScanner(XMLDocumentScanner *ds) : TokenScanner(ds), fEndMark("--", false)
{}

CommentScanner::~CommentScanner()
{}
