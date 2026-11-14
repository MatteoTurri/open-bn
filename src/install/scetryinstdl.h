/*
 * sceTRYInstDL*: download wrappers of the TRY installer module
 * (scetryinstdl.cpp), the installer methods of the DNAS2 download queue
 * (dnas2cdl.cpp). The tag and work arguments are not used.
 */

#ifndef SCETRYINSTDL_H
#define SCETRYINSTDL_H

#include "dnas/dnas2cdl.h"

extern "C" {

extern int try_instdl_errno;

int sceTRYInstDLFirstCheck(DNAS2DLTag *tag, int z, DNAS2CDLWork *w);
int sceTRYInstDLRangeCheck(DNAS2DLTag *tag, DNAS2CDLWork *w, int z);
int sceTRYInstDLWrite(int fd, void *buf, int nbyte);
int sceTRYInstDLRewind(int fd);
int sceTRYInstDLClose(int fd);
int sceTRYInstDLComplete(DNAS2DLTag *tag, int z);
int sceTRYInstDLFail(int fd, DNAS2DLTag *tag, int z);
}

#endif
