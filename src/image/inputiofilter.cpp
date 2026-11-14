/*
 * InputIoFilter: typed reads (big- and little-endian), whole-file cache and
 * object identifier, on top of the subclass's Read_/Seek/Size.
 */

#pragma implementation "iofilter.h"
#include "iofilter.h"

int InputIoFilter::ReadInt16(short *out)
{
	signed char buf[8];
	int result;
	if (Read(2, (unsigned char *)buf)) {
		short v = (buf[0] << 8) | (unsigned char)buf[1];
		*out = v;
		result = 1;
	} else {
		result = 0;
	}
	return result;
}

int InputIoFilter::ReadInt32(int *out)
{
	unsigned char buf[8];
	int result;
	if (Read(4, buf)) {
		/* NOTE: bug: buf[1] is used twice and buf[2] is never read. */
		int b0, b1, b2, b3;
		b0 = buf[0];
		b0 <<= 24;
		b1 = buf[1];
		b1 <<= 16;
		b2 = buf[1];
		b2 <<= 8;
		b3 = buf[3];
		*out = b0 | b1 | b2 | b3;
		result = 1;
	} else {
		result = 0;
	}
	return result;
}

int InputIoFilter::ReadInt16Reverse(short *out)
{
	signed char buf[8];
	int result;
	if (Read(2, (unsigned char *)buf)) {
		*out = (buf[1] << 8) | (unsigned char)buf[0];
		result = 1;
	} else {
		result = 0;
	}
	return result;
}

int InputIoFilter::ReadInt32Reverse(int *out)
{
	unsigned char buf[8];
	int result;
	if (Read(4, buf)) {
		int b0, b1, b2, b3;
		b0 = buf[0];
		b0 &= 0xff;
		b1 = buf[1];
		b1 <<= 8;
		b1 &= 0xff00;
		b2 = buf[2];
		b2 <<= 16;
		b2 &= 0xff0000;
		b3 = buf[3];
		b3 <<= 24;
		b3 &= 0xff000000;
		*out = b0 | b1 | b2 | b3;
		result = 1;
	} else {
		result = 0;
	}
	return result;
}

int InputIoFilter::Read(unsigned int n, unsigned char *buf)
{
	int result;

	if (m_cacheEnabled) {
		if (!m_cacheBuf && !ReadAllDataToCache())
			return Read_(n, buf);
		memcpy(buf, m_cacheBuf + m_cachePos, n);
		m_cachePos += n;
		result = n;
	} else {
		result = Read_(n, buf);
	}
	return result;
}

int InputIoFilter::Read(unsigned int pos, unsigned int n, unsigned char *buf)
{
	if (m_cacheEnabled) {
		if (!m_cacheFilled) {
			if (ReadAllDataToCache()) {
				memcpy(buf, m_cacheBuf + pos, n);
				m_cachePos = pos + n;
			} else {
				Read_(pos, n, buf);
			}
		}

		memcpy(buf, m_cacheBuf + pos, n);
		m_cachePos = pos + n;
		return n;
	}
	return Read_(pos, n, buf);
}

int InputIoFilter::ReadAllDataToCache()
{
	m_cacheSize = Size();
	unsigned char *buf = (unsigned char *)malloc(m_cacheSize);
	m_cacheBuf = buf;
	if (buf) {
		m_cacheFilled = 1;
		m_cachePos = 0;
		return Read_(0, m_cacheSize, buf);
	}
	m_cacheFilled = 0;
	return 0;
}

InputIoFilter::InputIoFilter()
{
	m_cacheBuf = 0;
	m_cacheFilled = 0;
	m_cacheEnabled = 0;
	m_objectIdentifier = 0;
}

int InputIoFilter::EnableCache(int flag)
{
	int old = m_cacheEnabled;
	m_cacheEnabled = flag;
	return old;
}

int InputIoFilter::DisableCache()
{
	int old = m_cacheEnabled;
	m_cacheEnabled = 0;
	return old;
}

int InputIoFilter::CacheMaxSize(int size)
{
	m_cacheMaxSize = size;
	return 1;
}

InputIoFilter::~InputIoFilter()
{
	if (m_cacheBuf) {
		free(m_cacheBuf);
		m_cacheBuf = 0;
	}
	if (m_objectIdentifier)
		delete[] m_objectIdentifier;
	m_objectIdentifier = 0;
}

const char *InputIoFilter::ObjectIdentifier()
{
	return m_objectIdentifier;
}

int InputIoFilter::ObjectIdentifier(const char *id)
{
	m_objectIdentifier = new char[strlen(id) + 1];
	strcpy(m_objectIdentifier, id);
	return 1;
}
