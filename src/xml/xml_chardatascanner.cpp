/*
 * CharacterDataScanner: scanner of the text between tags.
 */

#include "xml_scanner.h"
#include "xml_tokenscanner.h"
#include "xml_inputstreamimpl.h"
#include "xml_scanners.h"
#include "xml_inputsource.h"

/*
 * Reads the text, replacing the five predefined entities and reporting the forbidden
 * sequence "]]>".
 */

int CharacterDataScanner::Scan(XMLInputStream *stream, XMLStreamManager *mgr)
{
	int done = 0;

	fBuf.Clear();

	XMLInputSource *src = (XMLInputSource *)stream->GetSource();
	int r;

	do {
		r = stream->GetNextPortion(&fBuf, &fStopA, &fStopB);
		if ((r & 1) == 0) {
			if (r == 16 && fBuf.CharLength() == 0)
				return 16;

			r = stream->SenseNextChar(&fChar);
			if (fChar == (XMLChar)'<') {
				if (fBuf.CharLength() == 0) {
					fErrorProcessor->TreatErrorState(XPE_CharDataBadStart, stream);
					return 0;
				}
			} else if (fChar == (XMLChar)']') {
				r = stream->SkipNextChar();
				if (r & 1) {
					stream->SenseNextChar(&fChar);
					if (fChar == (XMLChar)']') {
						stream->SkipNextChar();
						stream->SenseNextChar(&fChar);
						if (fChar == (XMLChar)'>') {
							fErrorProcessor->TreatErrorState(XPE_CharDataCDataEnd, stream);
							return 0;
						}
						fBuf.Append((XMLChar)']');
					}
					fBuf.Append((XMLChar)']');
				} else if (r != 16) {
					fDocScanner->characters(&fBuf, fBuf.ByteLength(), mgr);
					fErrorProcessor->TreatErrorState(XPE_PIBadContent, stream);
					return r;
				}
			} else if (r == 0x1000) {
				fErrorProcessor->TreatErrorState(XPE_CharRefUnresolved, stream);
				return 0x1000;
			}
		}
		if (r != 0) {
			stream->SenseNextChar(&fChar);
			if (fChar == (XMLChar)'&') {
				unsigned back = fChar.ByteLength();
				src->seekg(back, XMLIOSeek_cur);
				fChar.Clear();
				stream->GetNextPortion(&fChar, ((XMLInputStreamImpl *)stream)->fSemi, 0);

				if (fChar == XMLString(((XMLInputStreamImpl *)stream)->fLt)) {
					fBuf.Append((XMLChar)'<');
					r = stream->SkipNextChar();
				} else if (fChar == XMLString(((XMLInputStreamImpl *)stream)->fGt)) {
					fBuf.Append((XMLChar)'>');
					r = stream->SkipNextChar();
				} else if (fChar == XMLString(((XMLInputStreamImpl *)stream)->fQuot)) {
					fBuf.Append((XMLChar)'"');
					r = stream->SkipNextChar();
				} else if (fChar == XMLString(((XMLInputStreamImpl *)stream)->fAmp)) {
					fBuf.Append((XMLChar)'&');
					r = stream->SkipNextChar();
				} else if (fChar == XMLString(((XMLInputStreamImpl *)stream)->fApos)) {
					fBuf.Append((XMLChar)'\'');
					r = stream->SkipNextChar();
				} else {
					fDocScanner->characters(&fBuf, fBuf.ByteLength(), mgr);
					r = 1;
					done = 1;
					src->seekg(-back - fChar.ByteLength(), XMLIOSeek_cur);
				}
			} else {
				fDocScanner->characters(&fBuf, fBuf.ByteLength(), mgr);
				r = 1;
				done = 1;
			}
		} else {
			fErrorProcessor->TreatErrorState((XMLParseErrorState)0, stream);
			return 0;
		}
	} while (!done);
	return r;
}

CharacterDataScanner::CharacterDataScanner(XMLDocumentScanner *ds, Validator *v)
        : TokenScanner(ds), fStopB("]<", false), fStopA("&", false)
{
	fValidator = v;
}

CharacterDataScanner::~CharacterDataScanner()
{}
