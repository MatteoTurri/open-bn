/*
 * OutputIoFileFilter methods, forwarding the stream operations to EiclFile.
 */

#pragma implementation "outputiofilefilter.h"
#include "outputiofilefilter.h"

OutputIoFileFilter::OutputIoFileFilter()
{
	m_file = 0;
}

OutputIoFileFilter::OutputIoFileFilter(const char *path)
{
	ObjectIdentifier(path);
	m_file = new EiclFile(ObjectIdentifier(), "w");
}

int OutputIoFileFilter::Open(const char *path)
{
	ObjectIdentifier(path);
	m_file = new EiclFile();
	return m_file->Open(path, "w") != 0;
}

int OutputIoFileFilter::Write(unsigned int n, const unsigned char *buf)
{
	return Write_(n, buf);
}

int OutputIoFileFilter::Read(unsigned int n, unsigned char *buf)
{
	return Read_(n, buf);
}

int OutputIoFileFilter::Seek(unsigned int pos, int whence)
{
	return m_file->Seek(pos, whence);
}

unsigned int OutputIoFileFilter::GetCurrentPosition()
{
	return m_file->GetCurrentPosition();
}

unsigned int OutputIoFileFilter::Size()
{
	return m_file->Size();
}

int OutputIoFileFilter::Close()
{
	if (m_file) {
		delete m_file;
		m_file = 0;
	}
	return 1;
}

/* Close() deletes m_file. */
OutputIoFileFilter::~OutputIoFileFilter()
{
	Close();
}

int OutputIoFileFilter::Write_(unsigned int n, const unsigned char *buf)
{
	if (!m_file)
		return 0;
	return m_file->Write(n, buf);
}

int OutputIoFileFilter::Read_(unsigned int n, unsigned char *buf)
{
	if (!m_file)
		return 0;
	return m_file->Read(n, buf);
}
