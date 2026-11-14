/*
 * STagScanner: scanner of the start tags <name attr="value" ...> and <name/>.
 */

#include "xml_scanner.h"
#include "xml_tokenscanner.h"
#include "xml_entityscanner.h"
#include "xml_entitydeclaration.h"
#include "xml_validator.h"
#include "xml_streammanager.h"
#include "xml_scanners.h"
#include "xml_locatorimpl.h"
#include "xml_inputsource.h"

/*
 * Reads the name and the attributes, resolving the entity references in the values.
 * Once the value of a predefined entity has become the current stream, entity resolving
 * is disabled on it, so that the text "&lt;" is not read again as a reference.
 */
int STagScanner::Scan(XMLInputStream *stream, XMLStreamManager *mgr)
{
	EntityDeclaration *decl;
	XMLInputStream *first = 0;
	XMLString *frag = 0;
	int r = 0;
	int ws = 0;
	int slash = 0;

	fAttrs.Clear();
	r = stream->GetName(&fName);
	if ((r & 1) == 0) {
		fErrorProcessor->TreatErrorState(XPE_ETagNameExpected, stream);
		return r;
	}
	ws = stream->SkipWhiteSpace();
	stream->SenseNextChar(&fChar);

	do {
		if ((ws & 1) != 0 && fChar != (XMLChar)'>' && fChar != (XMLChar)'/') {
			r = stream->GetName(&fAttrName);
			if ((r & 1) == 0) {
				fErrorProcessor->TreatErrorState(XPE_AttrNameExpected, stream);
				return r;
			}
			if (fAttrs.Declared(&fAttrName)) {
				fErrorProcessor->TreatErrorState(XPE_AttrDuplicate, stream);
				return 0;
			}
			stream->SkipWhiteSpace();
			stream->GetNextChar(&fChar);
			if (fChar != (XMLChar)'=') {
				fErrorProcessor->TreatErrorState(XPE_AttrEqExpected, stream);
				return 0;
			}
			stream->SkipWhiteSpace();
			stream->GetNextChar(&fQuote);
			first = stream;
			if (fQuote != (XMLChar)'\'' && fQuote != (XMLChar)'"') {
				fErrorProcessor->TreatErrorState(XPE_AttrQuoteExpected, stream);
				return 0;
			}
			XMLString *val = &fValue;
			val->Clear();
			r = stream->GetNextPortion(val, stream == first ? &fQuote : &fStopAlt, &fStopSet);
			while ((r & 1) == 0) {
				if (r == 16) {
					stream = mgr->ReleaseInputStream();
					if (stream == 0) {
						fErrorProcessor->TreatErrorState(XPE_UnexpectedEndOfInput, 0);
						return 16;
					}
				} else if (r == 4096) {
					fErrorProcessor->TreatErrorState(XPE_CharRefUnresolved, stream);
					return 4096;
				}
				stream->SenseNextChar(&fChar);
				if (fChar == (XMLChar)'&') {
					stream->SkipNextChar();
					decl = 0;
					r = fEntityScan->ScanAndResolve(&decl, stream, mgr, false, false);
					if ((r & 1) == 0) {
						if (decl == 0)
							return r;
						if (decl->CharacterRef()) {
							delete decl;
						}
						return r;
					}
					if (decl->InternalEntity()) {
						r = mgr->ApplyInputStream(decl->GetValue(), false, true);
						if (r == 257) {
							stream = mgr->GetCurrentStream();
							if (fEntityScan->IsBuiltInEntity(decl->GetName()))
								stream->DisableEntityResolving();
						} else {
							if (decl == 0)
								return r;
							if (decl->CharacterRef()) {
								delete decl;
							}
							return r;
						}
					} else {
						fErrorProcessor->TreatErrorState(XPE_AttrExternalEntity, stream);
						return 0;
					}
					if (decl != 0) {
						if (decl->CharacterRef()) {
							delete decl;
						}
					}
				}
				if (fChar == (XMLChar)'<') {
					fErrorProcessor->TreatErrorState(XPE_AttrLtInValue, stream);
					return r;
				}
				frag = &fFrag;
				frag->Clear();
				r = stream->GetNextPortion(frag, stream == first ? &fQuote : &fStopAlt, &fStopSet);
				fValue.Append(frag);
				stream->SenseNextChar(&fChar);
				if (fChar != (XMLChar)'&' && fChar != (XMLChar)'<' && mgr->LastInputStream() == 0) {
					stream = mgr->ReleaseInputStream();
					frag->Clear();
					r = stream->GetNextPortion(frag, stream == first ? &fQuote : &fStopAlt,
					    &fStopSet);
					fValue.Append(frag);
					stream->SenseNextChar(&fChar);
					if (fChar == fQuote)
						r = 1;
				}
			}
			r = stream->SkipNextChar();
			if ((r & 1) == 0) {
				fErrorProcessor->TreatErrorState(XPE_UnexpectedEndOfInput, stream);
				return r;
			}
			fType.Set(&fCDATAType);
			fAttrs.AddAttribute(&fAttrName, &fValue, &fType, !fStandalone);
			ws = stream->SkipWhiteSpace();
			r = stream->SenseNextChar(&fChar);
		}
	} while ((ws & 1) != 0 && fChar != (XMLChar)'>' && fChar != (XMLChar)'/' && r != 16);
	r = stream->SenseNextChar(&fChar);
	if (mgr->LastInputStream() == 0 && r == 16) {
		stream = mgr->ReleaseInputStream();
		r = stream->SenseNextChar(&fChar);
	}
	slash = (fChar == (XMLChar)'/');
	if (slash)
		stream->SkipNextChar();
	stream->GetNextChar(&fChar);
	if (fChar == (XMLChar)'>') {
		int st = 0;
		if (fValidator != 0)
			st = fValidator->ValidateTag(XMLToken_STag, &fName);
		if (st == 0) {
			fDocScanner->startElement(&fName, fAttrs, mgr);
			if (slash) {
				fValidator->ValidateTag(XMLToken_ETag, &fName);
				fDocScanner->endElement(&fName, mgr);
				if (fValidator != 0) {
					if (fValidator->EofAcceptable(true))
						r = XSR_RootClosed;
				}
			}
		} else {
			fErrorProcessor->TreatErrorState((XMLParseErrorState)st, stream);
			r = 0;
		}
	} else {
		fErrorProcessor->TreatErrorState(XPE_ETagGtExpected, stream);
		r = 0;
	}
	return r;
}

STagScanner::STagScanner(XMLDocumentScanner *ds, Validator *v, EntityScanner *es)
        : TokenScanner(ds), fStopSet("<&", false), fStopAlt("", false), fCDATAType("CDATA", false)
{
	fValidator = v;
	fEntityScan = es;
	fStandalone = false;
}

STagScanner::~STagScanner()
{}

/* With a validator, marks the current position of the stream. */
void STagScanner::InitPE(XMLInputStream *stream)
{
	if (fValidator != 0)
		fValidator->Mark(stream->GetLocator());
}
