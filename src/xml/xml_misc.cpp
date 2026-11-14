/*
 * ScannerFactory: creates on first use, and owns, the scanner of each token kind and
 * the validator of a document scan.
 */

#include "xml_locator.h"
#include "xml_filedescriptor.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#include "xml_scanner.h"
#include "xml_entityscanner.h"
#include "xml_scanners.h"
#pragma implementation "xml_misc.h"
#include "xml_misc.h"
#include "xml_streammanager.h"
#include "xml_inputsource.h"

ScannerFactory::~ScannerFactory()
{
	if (fCdataScanner) {
		delete fCdataScanner;
		fCdataScanner = 0;
	}
	if (fCharacterdataScanner) {
		delete fCharacterdataScanner;
		fCharacterdataScanner = 0;
	}
	if (fCommentScanner) {
		delete fCommentScanner;
		fCommentScanner = 0;
	}
	if (fPIScanner) {
		delete fPIScanner;
		fPIScanner = 0;
	}
	if (fEntityScanner) {
		delete fEntityScanner;
		fEntityScanner = 0;
	}
	if (fEntityDeclManager) {
		delete fEntityDeclManager;
		fEntityDeclManager = 0;
	}
	if (fEtagScanner) {
		delete fEtagScanner;
		fEtagScanner = 0;
	}
	if (fStagScanner) {
		delete fStagScanner;
		fStagScanner = 0;
	}
	if (fSScanner) {
		delete fSScanner;
		fSScanner = 0;
	}
	if (fValidator) {
		delete fValidator;
		fValidator = 0;
	}
}

/* Get-or-create of the scanner for a token; tokens without a scanner return null. */
void *ScannerFactory::Create(XMLToken tok)
{
	switch ((int)tok) {
	case XMLToken_CharData:
		if (fCharacterdataScanner)
			return fCharacterdataScanner;
		return createCharacterdataScanner();
	case XMLToken_ETag:
		if (fEtagScanner)
			return fEtagScanner;
		return createEtagScanner();
	case XMLToken_STag:
		if (fStagScanner)
			return fStagScanner;
		return createStagScanner();
	case XMLToken_Space:
		if (fSScanner)
			return fSScanner;
		return createSScanner();
	case XMLToken_EntityRef: {
		EntityScanner *s = fEntityScanner;
		if (!s) {
			if (!fEntityDeclManager)
				fEntityDeclManager = new EntityDeclManager();
			s = fEntityScanner = new EntityScanner(fDocScanner, fEntityDeclManager);
		}
		return s;
	}
	case XMLToken_PI:
		if (fPIScanner)
			return fPIScanner;
		return createPIScanner();
	case XMLToken_Comment:
		if (fCommentScanner)
			return fCommentScanner;
		return createCommentScanner();
	case XMLToken_CDataSect:
		if (fCdataScanner)
			return fCdataScanner;
		return createCdataScanner();
	case XMLToken_Doctype:
	case XMLToken_ElementDecl:
	case 7:
	case XMLToken_DeclEnd:
	case XMLToken_CDataEnd:
	case XMLToken_SubsetEnd:
	case XMLToken_EntityDecl:
	case 13:
	case 15:
	case 16:
	case 17:
	case 18:
	case XMLToken_CondSect:
	case 20:
	case 21:
	case 22:
	case 23:
	case XMLToken_NotationDecl:
	case XMLToken_PERef:
	case 27:
	case 28:
	case 31:
	case XMLToken_Text:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38:
	case 39:
	case 40:
	case 42:
	case 43:
	case XMLToken_EOF:
	default:
		return 0;
	}
}

ScannerFactory::ScannerFactory()
{
	fPad4 = 0;
	fCdataScanner = 0;
	fCharacterdataScanner = 0;
	fCommentScanner = 0;
	fEntityScanner = 0;
	fEtagScanner = 0;
	fPIScanner = 0;
	fStagScanner = 0;
	fSScanner = 0;
	fPad40[0] = 0;
	fPad40[1] = 0;
	fPad40[2] = 0;
	fPad40[3] = 0;
	fPad40[4] = 0;
	fPad40[5] = 0;
	fValidator = 0;
	fEntityDeclManager = 0;
	fPad72[0] = 0;
	fPad72[1] = 0;
	fPad72[2] = 0;
}
void ScannerFactory::RegisterDocScanner(XMLDocumentScanner *s)
{
	fDocScanner = s;
}

void *ScannerFactory::createEntityScanner()
{
	if (!fEntityDeclManager)
		fEntityDeclManager = new EntityDeclManager();
	EntityScanner *es = new EntityScanner(fDocScanner, fEntityDeclManager);
	fEntityScanner = es;
	return es;
}

void *ScannerFactory::GetSTagScanner(bool create)
{
	if (!fStagScanner && create)
		return createStagScanner();
	return fStagScanner;
}

Validator *ScannerFactory::GetValidator()
{
	return fValidator;
}

void *ScannerFactory::createValidator()
{
	Validator *v = new Validator(fDocScanner->GetErrorProcessor(), 0, 0);
	fValidator = v;
	return v;
}

void *ScannerFactory::createEtagScanner()
{
	ETagScanner *etag = new ETagScanner(fDocScanner,
	    fValidator ? fValidator : (fValidator = new Validator(fDocScanner->GetErrorProcessor(), 0, 0)));
	fEtagScanner = etag;
	return etag;
}

void *ScannerFactory::createStagScanner()
{
	if (!fEntityDeclManager)
		fEntityDeclManager = new EntityDeclManager();
	if (!fEntityScanner)
		fEntityScanner = new EntityScanner(fDocScanner, fEntityDeclManager);
	STagScanner *stag = new STagScanner(fDocScanner,
	    fValidator ? fValidator : (fValidator = new Validator(fDocScanner->GetErrorProcessor(), 0, 0)),
	    fEntityScanner);
	fStagScanner = stag;
	return stag;
}

void *ScannerFactory::createCharacterdataScanner()
{
	CharacterDataScanner *cds = new CharacterDataScanner(fDocScanner,
	    fValidator ? fValidator : (fValidator = new Validator(fDocScanner->GetErrorProcessor(), 0, 0)));
	fCharacterdataScanner = cds;
	return cds;
}

void *ScannerFactory::createCommentScanner()
{
	CommentScanner *cs = new CommentScanner(fDocScanner);
	fCommentScanner = cs;
	return cs;
}

void *ScannerFactory::createPIScanner()
{
	PIScanner *pi = new PIScanner(fDocScanner);
	fPIScanner = pi;
	return pi;
}

void *ScannerFactory::createCdataScanner()
{
	CDATAScanner *cdata = new CDATAScanner(fDocScanner,
	    fValidator ? fValidator : (fValidator = new Validator(fDocScanner->GetErrorProcessor(), 0, 0)));
	fCdataScanner = cdata;
	return cdata;
}

void *ScannerFactory::createSScanner()
{
	SScanner *s = new SScanner(fDocScanner);
	fSScanner = s;
	return s;
}
