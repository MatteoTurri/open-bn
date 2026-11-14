/*
 * OutputIoFileFilter: OutputIoFilter over a file, holding a heap EiclFile. The
 * file name is stored as the filter's object identifier.
 */

#ifndef OUTPUTIOFILEFILTER_H
#define OUTPUTIOFILEFILTER_H

#include "iofilter.h"
#include "eiclfile.h"

class OutputIoFileFilter : public OutputIoFilter {
public:
	OutputIoFileFilter();
	OutputIoFileFilter(const char *path);

	int Open(const char *path);

	virtual int Write(unsigned int n, const unsigned char *buf);
	virtual int Read(unsigned int n, unsigned char *buf);

	virtual int Seek(unsigned int pos, int whence);
	virtual unsigned int GetCurrentPosition();
	virtual unsigned int Size();
	virtual int Close();

	virtual ~OutputIoFileFilter();

protected:
	virtual int Write_(unsigned int n, const unsigned char *buf);
	virtual int Read_(unsigned int n, unsigned char *buf);

private:
	/* 0x08 */ EiclFile *m_file;
};

#endif
