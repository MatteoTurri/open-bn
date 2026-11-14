/*
 * InputIoFileFilter methods, forwarding the stream operations to EiclFile.
 */

#pragma implementation "inputiofilefilter.h"
#include "inputiofilefilter.h"

InputIoFileFilter::InputIoFileFilter()
{}

InputIoFileFilter::InputIoFileFilter(const char *path)
{
	if (m_file.Open(path, "rb"))
		ObjectIdentifier(path);
}

int InputIoFileFilter::Open(const char *path)
{
	if (m_file.Open(path, "rb")) {
		ObjectIdentifier(path);
		return 1;
	}
	return 0;
}

InputIoFileFilter::~InputIoFileFilter()
{
	m_file.Close();
}

int InputIoFileFilter::Close()
{
	return m_file.Close();
}

int InputIoFileFilter::Read_(unsigned int n, unsigned char *buf)
{
	return m_file.Read(n, buf);
}

int InputIoFileFilter::Read_(unsigned int pos, unsigned int n, unsigned char *buf)
{
	return m_file.Read(pos, n, buf);
}

int InputIoFileFilter::Seek(unsigned int pos)
{
	if (m_cacheEnabled && m_cacheFilled) {
		m_cachePos = pos;
		return 1;
	}
	return m_file.Seek(pos);
}

int InputIoFileFilter::Seek(unsigned int pos, int whence)
{
	if (m_cacheEnabled && m_cacheFilled) {
		switch (whence) {
		case 0:
			m_cachePos = pos;
			break;
		case 1:
			m_cachePos = m_cachePos + pos;
			break;
		case 2:
			m_cachePos = m_cacheSize - pos;
			break;
		}
		return 1;
	}
	return m_file.Seek(pos, whence);
}

unsigned int InputIoFileFilter::GetCurrentPosition()
{
	return m_file.GetCurrentPosition();
}

const char *InputIoFileFilter::FileName()
{
	return ObjectIdentifier();
}

const char *InputIoFileFilter::GetRealFileName()
{
	return ObjectIdentifier();
}

unsigned int InputIoFileFilter::Size()
{
	return m_file.Size();
}
