/*
 * EiclFile: a file on disk accessed through the LocalFileIO function table.
 * Every call goes through theLocalFileIO() first.
 */

#ifndef EICLFILE_H
#define EICLFILE_H

#include "localfileio.h"

class EiclFile {
public:
	EiclFile();
	EiclFile(const char *path, const char *mode);
	~EiclFile();

	int InitializeVariables();

	int Open(const char *path, const char *mode);
	int Close();

	int GetCurrentPosition_();
	int GetCurrentPosition();

	int Read(unsigned count, unsigned char *buf);
	int Read(unsigned offset, unsigned count, unsigned char *buf);
	int Write(unsigned count, const unsigned char *buf);

	int Seek(unsigned offset);
	int Seek(unsigned offset, int whence);

	int Size();

	_IO_FILE *GetpFILE();
	const char *GetFileName() const;

	static int touch_file(const char *name);

private:
	/* 0x00 */ _IO_FILE *m_pFILE;
	/* 0x04 */ char *m_fileName;
	/* 0x08 */ int m_unused;  /* unused, but part of the layout */
}; /* size = 0x0C */

#endif
