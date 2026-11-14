/*
 * XMLStreamManager: the input streams of the XML parser. Owns the stack of open streams
 * (nested entities), a ScannerFactory and the current stream.
 */

#ifndef GUARD_XML_STREAMMANAGER_H
#define GUARD_XML_STREAMMANAGER_H
#pragma interface
#include "xml_mmobject.h"
#include "xml_string.h"

class XMLInputStream;
class XMLDocumentScanner;
class InputStreamStack;
class ScannerFactory;
class XMLInputSource;
class XMLFileDescriptor;

class XMLStreamManager : public XMLMMObject {
	friend class XMLDocumentScanner;  /* parse() reads fFactory */
public:
	XMLStreamManager(XMLDocumentScanner *scanner);
	~XMLStreamManager();

	XMLInputStream *GetCurrentStream();
	int LastInputStream();
	int isEntityParse();
	void ReleaseAllInputStream();
	XMLInputStream *ReleaseInputStream();
	/* Returns 1 on success, 0 on failure. */
	int createInputStream(XMLInputSource *source, XMLCodeType codeType);
	int ApplyInputStream(XMLInputSource *source);
	int ApplyInputStream(XMLFileDescriptor *fd);
	int ApplyInputStream(XMLString *sysId, bool isPublic, bool isEntity);

protected:
	/* 0x04 */ InputStreamStack *fStack;
	/* 0x08 */ ScannerFactory *fFactory;
	/* 0x0C */ int fPad12;  /* never used */
	/* 0x10 */ XMLMMObject *fOwned;
	/* 0x14 */ XMLInputStream *fCurrent;
	/* 0x18 */ int fPad24;  /* never used */
	/* 0x1C */ XMLDocumentScanner *fScanner;
};

#endif // GUARD_XML_STREAMMANAGER_H
