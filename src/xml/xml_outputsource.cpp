/*
 * XMLOutputSource: the output stream of the serialiser; the << operators append text
 * to an XMLString buffer.
 */

#pragma implementation "xml_outputsource.h"
#include "xml_outputsource.h"
#include "xml_filedescriptor.h"
#include "xml_inputstreamimpl.h"
#include <stdio.h>

XMLString *XMLOutputSource::getPublicId()
{
	return fPublicId;
}
void XMLOutputSource::setEncoding(XMLCodeType encoding)
{
	fEncoding = encoding;
}
XMLCodeType XMLOutputSource::getEncoding()
{
	return fEncoding;
}

/*
 * Creates the internal writer (an XMLFileDescriptor on the buffer) from the parameters;
 * fEncoding is always XMLCode_UTF8, the codeType parameter goes to the writer only.
 */
XMLOutputSource::XMLOutputSource(XMLString *buffer, XMLIOType type, XMLCodeType codeType, int size)
{
	fWriter = new XMLFileDescriptor(buffer, type, codeType, size);
	fIOType = type;
	fEncoding = XMLCode_UTF8;
	fPublicId = new XMLString("buf", true);
	fBuffer = buffer;
	unk_18 = 1;
	fCtrl = new XMLString("\n", true);
}

XMLOutputSource::~XMLOutputSource()
{
	delete fPublicId;
	delete fWriter;
	delete fCtrl;
}

XMLOutputSource &XMLOutputSource::operator<<(const char *s)
{
	XMLString tmp(s, true);
	*this << &tmp;
	return *this;
}

XMLOutputSource &XMLOutputSource::operator<<(const unsigned char *s)
{
	XMLString tmp((const char *)s, true);
	*this << &tmp;
	return *this;
}

/* Shared body of operator<< for char, unsigned char, int and unsigned. */
static inline XMLOutputSource &putCStr(XMLOutputSource &os, const char *s)
{
	XMLString tmp(s, true);
	os << &tmp;
	return os;
}

XMLOutputSource &XMLOutputSource::operator<<(char c)
{
	char buf[2];
	buf[0] = c;
	buf[1] = '\0';
	return putCStr(*this, buf);
}

XMLOutputSource &XMLOutputSource::operator<<(unsigned char c)
{
	char buf[2];
	buf[0] = c;
	buf[1] = '\0';
	return putCStr(*this, buf);
}

XMLOutputSource &XMLOutputSource::operator<<(XMLString *s)
{
	fBuffer->Append(s);
	return *this;
}

XMLOutputSource &XMLOutputSource::operator<<(int n)
{
	char buf[16];
	sprintf(buf, "%ld", n);
	return putCStr(*this, buf);
}

XMLOutputSource &XMLOutputSource::operator<<(unsigned n)
{
	char buf[16];
	sprintf(buf, "%ld", n);
	return putCStr(*this, buf);
}

XMLOutputSource &XMLOutputSource::operator<<(XMLIOStrCtrl c)
{
	if (c == XMLIOStr_Newline) {
		if (fCtrl)
			fBuffer->Append(fCtrl);
	}
	return *this;
}
