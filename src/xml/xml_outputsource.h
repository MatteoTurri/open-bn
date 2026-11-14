/*
 * XMLOutputSource: destination of the XML serialisation (a buffer, through an internal writer).
 */

#ifndef GUARD_XML_OUTPUTSOURCE_H
#define GUARD_XML_OUTPUTSOURCE_H
#pragma interface
#include "xml_string.h"
#include "xml_mmobject.h"

enum XMLIOStrCtrl {
	XMLIOStr_0,
	XMLIOStr_Newline,
	XMLIOStr_2,
	XMLIOStr_3
};
/* Mode of the internal writer: XMLIOType_File for a file, XMLIOType_Memory for a buffer (SerialiseToBuffer). */
enum XMLIOType {
	XMLIOType_0,
	XMLIOType_File,
	XMLIOType_2,
	XMLIOType_Memory
};

class XMLOutputSource : public XMLMMObject {
public:
	XMLOutputSource(XMLString *buffer, XMLIOType type, XMLCodeType codeType, int size);
	virtual ~XMLOutputSource();
	XMLString *getPublicId();
	XMLCodeType getEncoding();
	void setEncoding(XMLCodeType encoding);
	XMLOutputSource &operator<<(const char *s);
	XMLOutputSource &operator<<(const unsigned char *s);
	XMLOutputSource &operator<<(char c);
	XMLOutputSource &operator<<(unsigned char c);
	XMLOutputSource &operator<<(XMLString *s);
	XMLOutputSource &operator<<(int n);
	XMLOutputSource &operator<<(unsigned n);
	XMLOutputSource &operator<<(XMLIOStrCtrl c);

protected:
	/* 0x04 */ XMLIOType fIOType;
	/* 0x08 */ XMLMMObject *fWriter;  /* internal writer, an XMLFileDescriptor */
	/* 0x0C */ XMLString *fPublicId;
	/* 0x10 */ XMLCodeType fEncoding;
	/* 0x14 */ XMLString *fBuffer;
	/* 0x18 */ int unk_18;
	/* 0x1C */ XMLString *fCtrl;  /* line terminator, "\n" */
};

#endif // GUARD_XML_OUTPUTSOURCE_H
