/*
 * InputIoFileFilter: InputIoFilter over a file opened with EiclFile. The file
 * name is stored as the filter's object identifier.
 */

#ifndef INPUTIOFILEFILTER_H
#define INPUTIOFILEFILTER_H

#include "iofilter.h"
#include "eiclfile.h"

class InputIoFileFilter : public InputIoFilter {
public:
	InputIoFileFilter();
	InputIoFileFilter(const char *path);

	int Open(const char *path);

	virtual const char *GetRealFileName();
	virtual int Seek(unsigned int pos);
	virtual int Seek(unsigned int pos, int whence);
	virtual unsigned int GetCurrentPosition();
	virtual int Close();
	virtual unsigned int Size();

	virtual ~InputIoFileFilter();

protected:
	virtual int Read_(unsigned int n, unsigned char *buf);
	virtual int Read_(unsigned int pos, unsigned int n, unsigned char *buf);

public:
	const char *FileName();

private:
	/* 0x20 */ EiclFile m_file;
};  /* size = 0x2C */

#endif
