/*
 * XMLStreamManager: opens the input streams of a parse (document, external entities),
 * keeps them on an InputStreamStack and releases them.
 */

#include "xml_utf8stream.h"
#include "xml_scanner.h"
#include "xml_entityscanner.h"
#include "xml_scanners.h"
#include "xml_misc.h"
#include "xml_inputstreamstack.h"
#include "xml_inputsource.h"
#pragma implementation "xml_streammanager.h"
#include "xml_streammanager.h"
#include "xml_locatorimpl.h"
#include "xml_validator.h"
#include <string.h>

/* XML declaration filled by createInputStream, never read. */
struct XMLDeclInfo {
	/* 0x000 */ int fHasDecl;
	/* 0x004 */ int fPad4;
	/* 0x008 */ char fVersion[256];
	/* 0x108 */ int fCodeType;
	/* 0x10C */ int fStandalone;
};

int XMLStreamManager::ApplyInputStream(XMLFileDescriptor *fd)
{
	XMLScannerState state = XSS_Prolog;
	if (fCurrent)
		state = fCurrent->GetState()->GetState();
	XMLInputSource *source = new XMLInputSource(fd);
	if (!source->fIsOpen) {
		if (source)
			delete source;
		fScanner->TreatErrorState(XPE_CannotOpenSource, 0);
		return 0;
	}
	unsigned codeType;
	if (fd->fHasBufferCode && fd->fIsEntity) {
		codeType = fd->fBuffer->GetCodeType();
	} else {
		codeType = XMLCode_None;
	}
	if (!createInputStream(source, (XMLCodeType)codeType))
		return 0;
	fStack->Push(fCurrent);
	fCurrent->GetState()->SetState(state);
	STagScanner *stag = (STagScanner *)fFactory->GetSTagScanner(false);
	if (stag)
		stag->InitPE(fCurrent);
	return 257;
}

/*
 * Reads the XML declaration of the source, if any: only the UTF-8 encoding is accepted
 * (codeType is not used). Sets fCurrent to a new XMLUTF8Stream and returns 1, or 0.
 */
int XMLStreamManager::createInputStream(XMLInputSource *source, XMLCodeType codeType)
{
	LocatorImpl *loc;
	int hasDecl;
	int n;

	if (!source->fIsOpen) {
		if (source)
			delete source;
		fCurrent = 0;
		return 0;
	}

	XMLDeclInfo decl;  /* unused, but part of the layout */
	unsigned char buf[16];
	char sig[6] = "<?xml";
	char value[64];

	hasDecl = 1;
	source->read(buf, 5);
	loc = (LocatorImpl *)source->fLocator;
	for (int i = 0; i < 5; i++) {
		if (buf[i] ^ ((unsigned char *)sig)[i])
			hasDecl = 0;
	}

	if (hasDecl) {
		if (source->fXMLFileDescriptor->fCodeType != XMLCode_None) {
			do {
				if (!source->read(buf, 1))
					break;
			} while (buf[0] != '>');
		} else {
			while (source->read(buf, 1)) {
				if (buf[0] == 'e') {
					if (source->read(buf, 7) == 7) {
						if (memcmp(buf, "ncoding", 7) == 0) {
							do {
								if (!source->read(buf, 1))
									break;
							} while (buf[0] != '"' && buf[0] != '\'');
							n = 0;
							for (;;) {
								if (!source->read(buf, 1))
									break;
								if (buf[0] == '"' || buf[0] == '\'')
									break;
								value[n] = buf[0];
								n++;
							}
							XMLString name(value, n);
							if (name != XMLString::CodetypeToStr(XMLCode_UTF8)) {
								fScanner->TreatErrorState(XPE_InvalidTokenState, 0);
								ReleaseInputStream();
								if (source)
									delete source;
								return 0;
							}
						} else {
							source->seekg(-7, XMLIOSeek_cur);
						}
					}
				}
				if (buf[0] == '>')
					break;
			}
		}
	} else {
		source->seekg(0);
	}

	loc->fLineNumber = 1;
	loc->fColumnNumber = 1;
	decl.fHasDecl = hasDecl;
	decl.fCodeType = XMLCode_UTF8;
	strncpy(decl.fVersion, "1.0", 4);
	decl.fStandalone = 0;
	fCurrent = new XMLUTF8Stream(XMLCode_UTF8, source);
	return 1;
}

XMLStreamManager::XMLStreamManager(XMLDocumentScanner *scanner)
{
	fCurrent = 0;
	fStack = new InputStreamStack();
	fFactory = new ScannerFactory();
	fOwned = 0;
	fFactory->RegisterDocScanner(scanner);
	fScanner = scanner;
}

XMLStreamManager::~XMLStreamManager()
{
	delete fFactory;
	delete fStack;
	delete fOwned;
}

/* Opens a stream on a copy of sysId, through a temporary descriptor. */
int XMLStreamManager::ApplyInputStream(XMLString *sysId, bool isPublic, bool isEntity)
{
	XMLFileDescriptor fd;
	int ret;

	fd.fIsEntity = isEntity;
	fd.fHasBufferCode = isEntity;
	if (isPublic || isEntity) {
		fd.fIOType = XMLIOType_Memory;
		fd.fSize = sysId->ByteLength();
		if (isEntity)
			fd.fCodeType = XMLCode_UTF8;
	} else {
		fd.fIOType = XMLIOType_File;
		fd.fCodeType = XMLCode_None;
	}
	fd.fBuffer = sysId->Clone();
	if (fStack->IsOpen(&fd)) {
		fScanner->TreatErrorState(XPE_SourceAlreadyOpen, fCurrent);
		if (fd.fBuffer)
			delete fd.fBuffer;
		return 0;
	}
	ret = ApplyInputStream(&fd);
	if (fd.fBuffer)
		delete fd.fBuffer;
	return ret;
}

int XMLStreamManager::ApplyInputStream(XMLInputSource *source)
{
	unsigned codeType = XMLCode_None;
	XMLScannerState state = XSS_Prolog;
	if (fCurrent)
		state = fCurrent->GetState()->GetState();
	XMLFileDescriptor *fd = source->fXMLFileDescriptor;
	if (fd->fIOType == XMLIOType_Memory)
		codeType = fd->fBuffer->GetCodeType();
	if (!createInputStream(source, (XMLCodeType)codeType))
		return 0;
	fStack->Push(fCurrent);
	fCurrent->GetState()->SetState(state);
	STagScanner *stag = (STagScanner *)fFactory->GetSTagScanner(false);
	if (stag)
		stag->InitPE(fCurrent);
	return 257;
}

XMLInputStream *XMLStreamManager::GetCurrentStream()
{
	return fCurrent;
}
int XMLStreamManager::LastInputStream()
{
	return fStack->IsLastInputStream();
}
int XMLStreamManager::isEntityParse()
{
	return fStack->IsEntity();
}

void XMLStreamManager::ReleaseAllInputStream()
{
	do {
		if (!fCurrent) {
			if (fStack->IsEmpty())
				continue;
		} else {
			delete fCurrent;
			if (fStack->IsEmpty())
				continue;  /* NOTE: bug: tests and deletes the stale fCurrent again */
			fStack->Pop();
		}
		fCurrent = (XMLInputStream *)fStack->Top();
	} while (fCurrent);
	fCurrent = 0;
}

XMLInputStream *XMLStreamManager::ReleaseInputStream()
{
	Validator *val = fFactory->GetValidator();
	if (val) {
		Locator *loc = fCurrent->GetLocator();
		if (loc) {
			XMLString *sysId = loc->getSystemId();
			val->RemoveMark(sysId);
		}
	}
	if (fCurrent) {
		XMLString *ent = (XMLString *)fCurrent->IsEntityReference();
		if (ent)
			fScanner->endEntity(ent, this);
		delete fCurrent;
	}
	if (!fStack->IsEmpty()) {
		fCurrent = (XMLInputStream *)fStack->Pop();
		fCurrent = (XMLInputStream *)fStack->Top();
	} else {
		fCurrent = 0;
	}
	return fCurrent;
}
