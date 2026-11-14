/*
 * LocalFileIO singleton, default function pointers (stdio) and
 * RegisterFileIOFunctions.
 */

#include <stdio.h>
#include <sys/stat.h>

#include "localfileio.h"

/* Defaults: stdio, and stat() for the file size. */
_IO_FILE *(*LocalFileIO::openFuncPtr)(const char *, const char *) = fopen;
int (*LocalFileIO::closeFuncPtr)(_IO_FILE *) = fclose;
int (*LocalFileIO::seekFuncPtr)(_IO_FILE *, long, int) = fseek;
unsigned (*LocalFileIO::readFuncPtr)(void *, unsigned, unsigned, _IO_FILE *) = fread;
unsigned (*LocalFileIO::writeFuncPtr)(const void *, unsigned, unsigned, _IO_FILE *) = fwrite;
long (*LocalFileIO::sizeFuncPtr)(const char *) = LocalFileIO::DefaultSizeFunc;
long (*LocalFileIO::tellFuncPtr)(_IO_FILE *) = ftell;

LocalFileIO &theLocalFileIO()
{
	static LocalFileIO instance;
	return instance;
}

LocalFileIO::LocalFileIO()
{}

long LocalFileIO::DefaultSizeFunc(const char *path)
{
	struct stat st;

	if (stat(path, &st))
		return 0;
	return st.st_size;
}

void LocalFileIO::RegisterFileIOFunctions(FILE *(*open_f)(const char *, const char *), int (*close_f)(FILE *),
    int (*seek_f)(FILE *, long, int), unsigned (*read_f)(void *, unsigned, unsigned, FILE *),
    unsigned (*write_f)(const void *, unsigned, unsigned, FILE *), long (*size_f)(const char *), long (*tell_f)(FILE *))
{
	openFuncPtr = open_f;
	closeFuncPtr = close_f;
	seekFuncPtr = seek_f;
	readFuncPtr = read_f;
	writeFuncPtr = write_f;
	sizeFuncPtr = size_f;
	tellFuncPtr = tell_f;
}
