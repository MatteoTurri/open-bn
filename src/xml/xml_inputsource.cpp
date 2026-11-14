/*
 * XMLInputSource: the byte source of a parse, over a memory buffer described by an
 * XMLFileDescriptor; position, peek/read/seek and the end-of-chunk handler.
 */

#pragma implementation "xml_inputsource.h"
#include "xml_eochandler.h"
#include "xml_filedescriptor.h"
#include "xml_inputsource.h"
#include "xml_locator.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#include "xml_locatorimpl.h"
#include "xml_string.h"
#include <string.h>

XMLInputSource::XMLInputSource(XMLFileDescriptor *fd)
{
	XMLIOType ioType = fd->fIOType;
	fXMLFileDescriptor = fd;
	fEOCHandler = 0;
	fIsFinal = 1;
	fIOType = ioType;
	unsigned code = fd->fCodeType;
	if (!code)
		fCodeType = XMLCode_UTF8;
	else
		fCodeType = code;
	if (fd->fBuffer) {
		if (fd->fBuffer->GetBufferPointer()) {
			if (fIOType == XMLIOType_Memory) {
				fPos = 0;
				fBufferString = fd->fBuffer->Clone();
				fLimit = fBufferString->ByteLength();
				fBuf = (char *)fBufferString->GetBytes();
				const char *onMemory = "ON_MEMORY";
				fPublicId = new XMLString(onMemory, false);
				fSystemId = new XMLString(onMemory, false);
				fIsFinal = 1;
				fIsOpen = 1;
			} else {
				fBufferString = 0;
				fSystemId = 0;
				fPublicId = 0;
				fLocator = 0;
				fIsOpen = 0;
			}
			fLocator = new LocatorImpl(fPublicId, fSystemId);
		}
	}
}

XMLString *XMLInputSource::getPublicId()
{
	return fPublicId;
}

XMLString *XMLInputSource::getSystemId()
{
	return fSystemId;
}
XMLFileDescriptor *XMLInputSource::getXMLFileDescriptor()
{
	return fXMLFileDescriptor;
}
Locator *XMLInputSource::getLocator()
{
	return fLocator;
}
int XMLInputSource::IsOpen()
{
	return fIsOpen;
}
void XMLInputSource::SetIsFinal(bool isFinal)
{
	fIsFinal = isFinal;
}
int XMLInputSource::GetIsFinal()
{
	return fIsFinal;
}
XMLInputSource::~XMLInputSource()
{
	delete fPublicId;
	delete fSystemId;
	delete fLocator;
	delete fBufferString;
}

void XMLInputSource::Append(const XMLString *str)
{
	if (fIOType == XMLIOType_Memory) {
		if (fPos >= 11) {
			unsigned limit = fLimit;
			fBufferString->DeleteData(0, fBufferString->CharLength() - 10);
			fPos -= limit - fBufferString->ByteLength();
		}
		fBufferString->Append(str);
		fLimit = fBufferString->ByteLength();
		fBuf = (char *)fBufferString->GetBytes();
	}
}

int XMLInputSource::seekg(int off, XMLIOSeekDir dir)
{
	switch (dir) {
	case XMLIOSeek_cur:
		fPos = fPos + off;
		break;
	case XMLIOSeek_beg:
		fPos = off;
		break;
	case XMLIOSeek_end:
		fPos = fLimit - off - 1;
		break;
	}
	return 0;
}

int XMLInputSource::seekg(int pos)
{
	fPos = pos;
	return 0;
}

int XMLInputSource::eof()
{
	while (fPos >= fLimit) {
		if (fIsFinal)
			return -1;
		endOfChunk();
	}
	return 0;
}

int XMLInputSource::peek()
{
	while (fPos >= fLimit) {
		if (fIsFinal)
			return -1;
		endOfChunk();
	}
	return fBuf[fPos];
}

int XMLInputSource::peek(unsigned n)
{
	unsigned result = 0;
	if (n > 4)
		n = 4;
	while (fPos >= fLimit) {
		if (fIsFinal)
			return -1;
		endOfChunk();
	}
	for (unsigned i = 0; i < n; i++) {
		result <<= 8;
		result |= (unsigned char)fBuf[fPos + i];
	}
	return result;
}

int XMLInputSource::read(unsigned char *buf, int len)
{
	while ((int)(fLimit - fPos) < len) {
		if (fIsFinal)
			len = fLimit - fPos;
		endOfChunk();
	}
	memcpy(buf, fBuf + fPos, len);
	fPos += len;
	return len;
}

void XMLInputSource::endOfChunk()
{
	if (fEOCHandler != 0)
		fEOCHandler->endOfChunk();
	else
		fIsFinal = 1;
}

void XMLInputSource::SetEOCHandler(EOCHandler *h)
{
	if (h)
		fEOCHandler = h;
}
