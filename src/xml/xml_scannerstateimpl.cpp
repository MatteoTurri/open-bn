/*
 * ScannerStateImpl: the scanner state (prolog, content, epilog) and the check of each
 * token against it.
 */

#include "xml_filedescriptor.h"
#include "xml_locator.h"
#include "xml_inputstream.h"
#include "xml_scannerstate.h"

/* Three-state automaton (prolog, content, epilog) over the token values; any other state returns 0. */
int ScannerStateImpl::Update(XMLToken *tok)
{
	switch (fState) {
	case XSS_Prolog: {
		int t = *(int *)tok;
		switch (t) {
		case XMLToken_PI:
		case XMLToken_Space:
		case XMLToken_Comment:
			return 1;
		case XMLToken_Doctype:
			fState = XSS_Unsupported;
			return 0;
		case XMLToken_STag:
			fState = XSS_Content;
			return 1;
		default:
			fState = XSS_Rejected;
			return 0;
		}
	}

	case XSS_Content: {
		int t = *(int *)tok;
		switch (t) {
		case XMLToken_CDataSect:
		case XMLToken_CharData:
		case XMLToken_EntityRef:
		case XMLToken_ETag:
		case XMLToken_PI:
		case XMLToken_STag:
		case XMLToken_Comment:
		case XMLToken_EOF:
			return 1;
		case XMLToken_DeclEnd:
			fState = XSS_Unsupported;
			return 0;
		case XMLToken_PERef:
		case XMLToken_Space:
		case XMLToken_Text:
			*(int *)tok = XMLToken_CharData;
			return 1;
		default:
			fState = XSS_Rejected;
			return 0;
		}
	}

	case XSS_Epilog: {
		int t = *(int *)tok;
		switch (t) {
		case XMLToken_PI:
		case XMLToken_Space:
		case XMLToken_Comment:
		case XMLToken_EOF:
			return 1;
		case XMLToken_STag: /* redundant as written */
		default:
			fState = XSS_Rejected;
			return 0;
		}
	}

	default:
		return 0;
	}
}

XMLScannerState ScannerStateImpl::GetState()
{
	return (XMLScannerState)fState;
}
void ScannerStateImpl::SetState(XMLScannerState s)
{
	fState = s;
}

ScannerStateImpl::ScannerStateImpl()
{
	fState = XSS_Prolog;
}

ScannerStateImpl::~ScannerStateImpl()
{}

/* After the close tag of the root element the state becomes XSS_Epilog. */
int ScannerStateImpl::Update(XMLScanResult r)
{
	if (r == XSR_RootClosed)
		fState = XSS_Epilog;
	return 1;
}
