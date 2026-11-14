/*
 * LocalFileIO: indirection layer for file I/O. The seven stdio-style function
 * pointers are static members, set with RegisterFileIOFunctions. _IO_FILE is
 * forward-declared so that <stdio.h> is not required.
 */

#ifndef LOCALFILEIO_H
#define LOCALFILEIO_H

struct _IO_FILE;

class LocalFileIO {
public:
	LocalFileIO();
	static long DefaultSizeFunc(const char *path);
	void RegisterFileIOFunctions(_IO_FILE *(*open_f)(const char *, const char *), int (*close_f)(_IO_FILE *),
	    int (*seek_f)(_IO_FILE *, long, int), unsigned (*read_f)(void *, unsigned, unsigned, _IO_FILE *),
	    unsigned (*write_f)(const void *, unsigned, unsigned, _IO_FILE *), long (*size_f)(const char *),
	    long (*tell_f)(_IO_FILE *));

	static _IO_FILE *(*openFuncPtr)(const char *, const char *);
	static int (*closeFuncPtr)(_IO_FILE *);
	static int (*seekFuncPtr)(_IO_FILE *, long, int);
	static unsigned (*readFuncPtr)(void *, unsigned, unsigned, _IO_FILE *);
	static unsigned (*writeFuncPtr)(const void *, unsigned, unsigned, _IO_FILE *);
	static long (*sizeFuncPtr)(const char *);
	static long (*tellFuncPtr)(_IO_FILE *);
};

LocalFileIO &theLocalFileIO();

#endif
