/*
 * sceTRYInstDL*: download wrappers of the TRY installer module, sharing one
 * TRYAbstFS (g_dlFs), the path buffer and the last error (try_instdl_errno).
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "trycontentinfo.h"
#include "scetryinstdl.h"

extern "C" {

/* TRYAbstFS of the download in progress. try_instdl_errno holds the last
   error: DLWrite sets it, DLFirstCheck clears it. */
int try_instdl_errno;
static TRYAbstFS *g_dlFs;
static char g_dlPathBuf[1024];

/* Mounts the content partition, builds the path "dlcontents/dnas_no_key"
   (g_dlPathBuf) and "<path>/content.dat", then Mkdir (-EEXIST is fine)
   and Open; on failure does UMount and returns the error code, otherwise
   returns the fd. */
int sceTRYInstDLFirstCheck(DNAS2DLTag *tag, int z, DNAS2CDLWork *w)
{
	char localbuf[1012];
	int status;

	g_dlFs = 0;
	try_instdl_errno = 0;
	g_dlFs = try_mountContentPat();
	if (g_dlFs == 0)
		return -101;

	sprintf(g_dlPathBuf, "%s/%s", "dlcontents", "dnas_no_key");
	sprintf(localbuf, "%s/%s", g_dlPathBuf, "content.dat");

	status = g_dlFs->Mkdir(g_dlPathBuf, 511);
	if (status < 0 && status != -EEXIST) {
		status = -104;
		goto closeFs;
	}

	{
		int r = g_dlFs->Open(localbuf, 1538, 438);
		if (r >= 0)
			return r;
		status = -103;
	}

closeFs:
	g_dlFs->UMount();
	return status;
}

/* Stub: ignores its arguments and returns 0. */
int sceTRYInstDLRangeCheck(DNAS2DLTag *tag, DNAS2CDLWork *w, int z)
{
	return 0;
}

/* On failure stores the error in try_instdl_errno (-99 for 0xfffefffb,
   otherwise the raw code) and returns -401. */
int sceTRYInstDLWrite(int fd, void *buf, int nbyte)
{
	int r = g_dlFs->Write(fd, buf, nbyte);
	if (r < 0) {
		if (r == 0xfffefffb)
			try_instdl_errno = -99;
		else
			try_instdl_errno = r;
		return -401;
	}
	return r;
}

int sceTRYInstDLRewind(int fd)
{
	int r = g_dlFs->Seek(fd, 0, 0);
	int result = -501;
	if (r >= 0)
		result = 0;
	return result;
}

int sceTRYInstDLClose(int fd)
{
	int r = g_dlFs->Close(fd);
	int result = -201;
	if (r >= 0)
		result = 0;
	return result;
}

/* Checks the downloaded content (try_checkDLContentInfo on a cleared local
   header), then UMount. On success deletes the fs, clears g_dlFs and returns
   0; otherwise takes the same cleanup path and returns -203 if the UMount
   failed, -202 if only the check failed. */
int sceTRYInstDLComplete(DNAS2DLTag *tag, int z)
{
	sceTRYInstDLContHeader hdr;
	int r;
	int ret;

	memset(&hdr, 0, 4264);
	r = try_checkDLContentInfo(g_dlFs, g_dlPathBuf, &hdr);

	if (r >= 0) {
		int umountRet = g_dlFs->UMount();
		if (umountRet >= 0) {
			delete g_dlFs;
			g_dlFs = 0;
			return 0;
		}
		ret = -203;
	} else {
		int umountRet = g_dlFs->UMount();
		ret = -202;
		if (umountRet < 0)
			ret = -203;
	}

	delete g_dlFs;
	g_dlFs = 0;
	return ret;
}

/* Closes fd, removes "dlcontents/dnas_no_key/content.dat", UMount; deletes
   the fs in both cases, then returns -203 if UMount failed, 0 otherwise. */
int sceTRYInstDLFail(int fd, DNAS2DLTag *tag, int z)
{
	char localbuf[1024];
	int r;

	g_dlFs->Close(fd);

	sprintf(localbuf, "%s/%s/%s", "dlcontents", "dnas_no_key", "content.dat");

	g_dlFs->Remove(localbuf);

	r = g_dlFs->UMount();

	if (r < 0) {
		delete g_dlFs;
		return -203;
	}
	delete g_dlFs;
	return 0;
}

} /* extern "C" */
