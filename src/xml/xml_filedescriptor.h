/*
 * XMLFileDescriptor: describes an XML input or output (I/O type, buffer, code type).
 */

#ifndef GUARD_XML_FILEDESCRIPTOR_H
#define GUARD_XML_FILEDESCRIPTOR_H
#include "xml_string.h"
#include "xml_mmobject.h"
#include "xml_outputsource.h"

class XMLFileDescriptor : public XMLMMObject {
public:
	/* empty descriptor */
	XMLFileDescriptor()
	{
		fHasBufferCode = 0;
		fIsEntity = 0;
		fIOType = XMLIOType_Memory;
		fBuffer = 0;
		fCodeType = XMLCode_None;
	}
	/* the internal writer of XMLOutputSource */
	XMLFileDescriptor(XMLString *buffer, XMLIOType type, XMLCodeType codeType, int size)
	{
		fIOType = type;
		fIsEntity = 0;
		fHasBufferCode = 0;
		if (buffer != 0 && size == 0)
			fSize = buffer->ByteLength();
		else
			fSize = size;
		fBuffer = buffer;
		fCodeType = codeType;
	}
	virtual ~XMLFileDescriptor() {}
	/* 0x04 */ XMLIOType fIOType;
	/* 0x08 */ int fSize;  /* size in bytes */
	/* 0x0C */ XMLString *fBuffer;
	/* 0x10 */ int fIsEntity;
	/* 0x14 */ int fHasBufferCode;
	/* 0x18 */ XMLCodeType fCodeType;
};

#endif // GUARD_XML_FILEDESCRIPTOR_H
