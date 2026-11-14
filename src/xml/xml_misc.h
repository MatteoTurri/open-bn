/*
 * ScannerFactory: creates on demand, and owns, the token scanners of the XML parser.
 */

#ifndef GUARD_XML_MISC_H
#define GUARD_XML_MISC_H
#pragma interface
#include "xml_mmobject.h"
#include "xml_validator.h"

class EntityDeclManager;
class XMLDocumentScanner;
class EntityScanner;
class CDATAScanner;
class CharacterDataScanner;
class CommentScanner;
class ETagScanner;
class PIScanner;
class STagScanner;
class SScanner;

class ScannerFactory : public XMLMMObject {
public:
	ScannerFactory();
	virtual ~ScannerFactory();

	Validator *GetValidator();
	void RegisterDocScanner(XMLDocumentScanner *s);
	void *GetSTagScanner(bool create);
	void *Create(XMLToken tok);

	void *createEntityScanner();
	void *createStagScanner();
	void *createEtagScanner();
	void *createCharacterdataScanner();
	void *createCommentScanner();
	void *createPIScanner();
	void *createCdataScanner();
	void *createSScanner();
	void *createValidator();

protected:
	/* 0x04 */ void *fPad4;  /* cleared by the constructor, never read */
	/* 0x08 */ CDATAScanner *fCdataScanner;
	/* 0x0C */ CharacterDataScanner *fCharacterdataScanner;
	/* 0x10 */ CommentScanner *fCommentScanner;
	/* 0x14 */ EntityScanner *fEntityScanner;
	/* 0x18 */ ETagScanner *fEtagScanner;
	/* 0x1C */ PIScanner *fPIScanner;
	/* 0x20 */ STagScanner *fStagScanner;
	/* 0x24 */ SScanner *fSScanner;
	/* 0x28 */ void *fPad40[6];  /* cleared by the constructor, never read */
	/* 0x40 */ Validator *fValidator;
	/* 0x44 */ EntityDeclManager *fEntityDeclManager;
	/* 0x48 */ void *fPad72[3];  /* cleared by the constructor, never read */
	/* 0x54 */ XMLDocumentScanner *fDocScanner;  /* not owned */
};

#endif // GUARD_XML_MISC_H
