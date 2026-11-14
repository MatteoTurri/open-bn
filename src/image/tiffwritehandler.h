/*
 * TIFFWriteHandler: static adapter between the libtiff TIFFClientOpen
 * callbacks and an OutputIoFilter, passed to libtiff as the opaque handle.
 * close() and map() do nothing and return 0.
 */

#ifndef TIFFWRITEHANDLER_H
#define TIFFWRITEHANDLER_H

#include "iofilter.h"

class TIFFWriteHandler {
public:
	/* 0x00 */ OutputIoFilter *m_ioFilter;

	static long read(void *fd, void *buf, long size)
	{
		OutputIoFilter *f = ((TIFFWriteHandler *)fd)->m_ioFilter;
		return f->Read(size, (unsigned char *)buf);
	}

	static long write(void *fd, void *buf, long size)
	{
		OutputIoFilter *f = ((TIFFWriteHandler *)fd)->m_ioFilter;
		return f->Write(size, (const unsigned char *)buf);
	}

	static long seek(void *fd, long offset, int whence)
	{
		if (!((TIFFWriteHandler *)fd)->m_ioFilter->Seek(offset, whence))
			return -1;
		return ((TIFFWriteHandler *)fd)->m_ioFilter->GetCurrentPosition();
	}

	static int close(void *fd) { return 0; }

	static long size(void *fd)
	{
		OutputIoFilter *f = ((TIFFWriteHandler *)fd)->m_ioFilter;
		return f->Size();
	}

	static int map(void *fd, void **base, long *sz) { return 0; }

	static void unmap(void *fd, void *base, long sz) {}
};

#endif
