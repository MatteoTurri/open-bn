/*
 * Abstract byte streams used by the image filters: InputIoFilter (typed reads
 * and an optional whole-file cache) and OutputIoFilter.
 */

#ifndef IOFILTER_H
#define IOFILTER_H

#include <stdlib.h>
#include <string.h>

typedef unsigned char UByte;

class InputIoFilter {
public:
	InputIoFilter();

	virtual int Read(unsigned int n, unsigned char *buf);
	virtual int ReadInt16(short *out);
	virtual int ReadInt32(int *out);
	virtual int ReadInt16Reverse(short *out);
	virtual int ReadInt32Reverse(int *out);

	virtual const char *GetRealFileName() = 0;
	virtual int Seek(unsigned int pos) = 0;
	virtual int Seek(unsigned int pos, int whence) = 0;
	virtual unsigned int GetCurrentPosition() = 0;
	virtual int Close() = 0;
	virtual unsigned int Size() = 0;

	virtual ~InputIoFilter();

protected:
	virtual int Read_(unsigned int n, unsigned char *buf) = 0;
	virtual int Read_(unsigned int pos, unsigned int n, unsigned char *buf) = 0;

public:
	int Read(unsigned int pos, unsigned int n, unsigned char *buf);
	int ReadAllDataToCache();

	int EnableCache(int flag);
	int DisableCache();
	int CacheMaxSize(int size);

	const char *ObjectIdentifier();
	int ObjectIdentifier(const char *id);

protected:
	/* The constructor leaves m_cacheMaxSize, m_cacheSize and m_cachePos unset. */
	/* 0x00 */ int m_cacheEnabled;
	/* 0x04 */ int m_cacheFilled;
	/* 0x08 */ unsigned char *m_cacheBuf;
	/* 0x0C */ int m_cacheMaxSize;
	/* 0x10 */ unsigned int m_cacheSize;
	/* 0x14 */ unsigned int m_cachePos;
	/* 0x18 */ char *m_objectIdentifier;
	/* 0x1C: vtable pointer */
};  /* size = 0x20 */

class OutputIoFilter {
public:
	OutputIoFilter();

	virtual int Write(unsigned int n, const unsigned char *buf);
	virtual int WriteInt32(int v);
	virtual int WriteInt16(short v);
	virtual int Read(unsigned int n, unsigned char *buf);

	virtual int Seek(unsigned int pos, int whence) = 0;
	virtual unsigned int GetCurrentPosition() = 0;
	virtual unsigned int Size() = 0;
	virtual int Close() = 0;

	virtual ~OutputIoFilter();

protected:
	virtual int Write_(unsigned int n, const unsigned char *buf) = 0;
	virtual int Read_(unsigned int n, unsigned char *buf) = 0;

public:
	const char *ObjectIdentifier() const;
	int ObjectIdentifier(const char *id);

private:
	/* 0x00 */ char *m_objectIdentifier;
	/* 0x04: vtable pointer */
};

#endif
