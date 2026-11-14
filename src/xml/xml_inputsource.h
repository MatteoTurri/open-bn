/*
 * XMLInputSource: a source of XML input (a buffer or a file descriptor) with its locator.
 */

#ifndef GUARD_XML_INPUTSOURCE_H
#define GUARD_XML_INPUTSOURCE_H
#pragma interface
#include "xml_eochandler.h"
#include "xml_mmobject.h"
#include "xml_outputsource.h"

class XMLString;
class Locator;
class XMLFileDescriptor;

enum XMLIOSeekDir {
	XMLIOSeek_0,
	XMLIOSeek_beg,
	XMLIOSeek_cur,
	XMLIOSeek_end
};

class XMLInputStreamImpl;

class XMLInputSource : public XMLMMObject {
	friend class XMLInputStreamImpl;
	friend class XMLDocumentScanner;
	friend class XMLStreamManager;
	friend class InputStreamStack;

public:
	XMLString *getPublicId();
	XMLString *getSystemId();
	XMLFileDescriptor *getXMLFileDescriptor();
	Locator *getLocator();
	int IsOpen();
	int GetIsFinal();
	void SetIsFinal(bool isFinal);
	int seekg(int off, XMLIOSeekDir dir);
	int seekg(int pos);
	void SetEOCHandler(EOCHandler *h);
	void endOfChunk();
	int eof();
	int peek();
	int peek(unsigned n);
	int read(unsigned char *buf, int len);
	void Append(const XMLString *str);
	XMLInputSource(XMLFileDescriptor *fd);
	~XMLInputSource();

protected:
	/* 0x04 */ XMLIOType fIOType;  /* copy of XMLFileDescriptor::fIOType */
	/* 0x08 */ XMLFileDescriptor *fXMLFileDescriptor;
	/* 0x0C */ unsigned fCodeType;  /* XMLCode_UTF8 when the descriptor gives none */
	/* 0x10 */ XMLString *fPublicId;
	/* 0x14 */ XMLString *fSystemId;
	/* 0x18 */ XMLString *fBufferString;  /* owned copy of the source buffer */
	/* 0x1C */ Locator *fLocator;
	/* 0x20 */ int fIsOpen;
	/* 0x24 */ int fIsFinal;
	/* 0x28 */ unsigned fPos;
	/* 0x2C */ EOCHandler *fEOCHandler;
	/* 0x30 */ unsigned fLimit;
	/* 0x34 */ char *fBuf;
};

#endif // GUARD_XML_INPUTSOURCE_H
