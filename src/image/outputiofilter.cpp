/*
 * OutputIoFilter base class: typed writes and the object identifier.
 */

#pragma implementation "iofilter.h"
#include "iofilter.h"

OutputIoFilter::OutputIoFilter()
{
	m_objectIdentifier = 0;
}

OutputIoFilter::~OutputIoFilter()
{
	if (m_objectIdentifier) {
		delete[] m_objectIdentifier;
		m_objectIdentifier = 0;
	}
}

int OutputIoFilter::Write(unsigned int n, const unsigned char *buf)
{
	return Write_(n, buf);
}

int OutputIoFilter::WriteInt32(int v)
{
	unsigned char buf[8];
	buf[0] = (unsigned char)((unsigned int)v >> 24);
	buf[1] = (unsigned char)((unsigned int)v >> 16);
	buf[2] = (unsigned char)((unsigned int)v >> 8);
	buf[3] = (unsigned char)v;
	return Write(4, buf) == 4;
}

int OutputIoFilter::WriteInt16(short v)
{
	unsigned char buf[8];
	unsigned int uv = v;
	buf[0] = (unsigned char)(uv >> 8);
	buf[1] = (unsigned char)v;
	return Write(2, buf) == 2;
}

int OutputIoFilter::Read(unsigned int n, unsigned char *buf)
{
	return Read_(n, buf);
}

const char *OutputIoFilter::ObjectIdentifier() const
{
	return m_objectIdentifier;
}

int OutputIoFilter::ObjectIdentifier(const char *id)
{
	m_objectIdentifier = new char[strlen(id) + 1];
	strcpy(m_objectIdentifier, id);
	return 1;
}
