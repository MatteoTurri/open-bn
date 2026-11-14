/*
 * EiclFile: open/read/write/seek on a file through the LocalFileIO table.
 */

#include <stdlib.h>
#include <string.h>

#include "eiclfile.h"

EiclFile::EiclFile()
{
	InitializeVariables();
}

EiclFile::EiclFile(const char *path, const char *mode)
{
	InitializeVariables();
	Open(path, mode);
}

EiclFile::~EiclFile()
{
	Close();
	if (m_fileName) {
		free(m_fileName);
		m_fileName = 0;
	}
}

int EiclFile::InitializeVariables()
{
	m_pFILE = 0;
	m_fileName = 0;
	return 1;
}

int EiclFile::Open(const char *path, const char *mode)
{
	theLocalFileIO();
	m_pFILE = LocalFileIO::openFuncPtr(path, mode);
	if (m_fileName) {
		free(m_fileName);
		m_fileName = 0;
	}
	m_fileName = (char *)malloc(strlen(path) + 1);
	if (m_fileName) {
		strcpy(m_fileName, path);
	} else {
		theLocalFileIO();
		LocalFileIO::closeFuncPtr(m_pFILE);
		m_pFILE = 0;
	}
	return m_pFILE != 0;
}

int EiclFile::Close()
{
	int ok = 1;
	if (m_pFILE) {
		theLocalFileIO();
		if (LocalFileIO::closeFuncPtr(m_pFILE) == 0) {
			m_pFILE = 0;
		} else {
			ok = 0;
		}
	}
	return ok;
}

int EiclFile::GetCurrentPosition_()
{
	if (!m_pFILE)
		return -1;
	theLocalFileIO();
	return LocalFileIO::tellFuncPtr(m_pFILE);
}

int EiclFile::Read(unsigned count, unsigned char *buf)
{
	if (!m_pFILE)
		return 0;
	theLocalFileIO();
	return LocalFileIO::readFuncPtr(buf, 1, count, m_pFILE);
}

int EiclFile::Read(unsigned offset, unsigned count, unsigned char *buf)
{
	if (!Seek(offset))
		return 0;
	return Read(count, buf);
}

int EiclFile::Write(unsigned count, const unsigned char *buf)
{
	if (!m_pFILE)
		return 0;
	theLocalFileIO();
	return LocalFileIO::writeFuncPtr(buf, 1, count, m_pFILE);
}

int EiclFile::Seek(unsigned offset)
{
	if (!m_pFILE)
		return 0;
	theLocalFileIO();
	return LocalFileIO::seekFuncPtr(m_pFILE, offset, 0) == 0;
}

int EiclFile::Seek(unsigned offset, int whence)
{
	if (!m_pFILE)
		return 0;
	theLocalFileIO();
	return LocalFileIO::seekFuncPtr(m_pFILE, offset, whence) == 0;
}

int EiclFile::Size()
{
	theLocalFileIO();
	return LocalFileIO::sizeFuncPtr(m_fileName);
}

_IO_FILE *EiclFile::GetpFILE()
{
	return m_pFILE;
}

int EiclFile::GetCurrentPosition()
{
	return GetCurrentPosition_();
}

int EiclFile::touch_file(const char *name)
{
	EiclFile f;
	int ok;
	if (f.Open(name, "w")) {
		f.Close();
		ok = 1;
	} else {
		ok = 0;
	}
	return ok;
}

const char *EiclFile::GetFileName() const
{
	return m_fileName;
}
