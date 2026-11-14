/*
 * Small/amx natives "file*" (table smallext_file): file copy, move, delete and
 * rename run as a background thFileOperation polled by fileSync; mkdir, lstat,
 * free size and USB status are immediate. The *USB variants first check the
 * memory stick status.
 */

#include "../../ref/small-fmt6/amx.h"
#include "natives/smallext.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string>
#include <list>

#include "fs/bbfile.h"

#include <sys/stat.h>

#include "script/script.h"

#include "fs/bburl.h"

#include "thread/thposixthread.h"

#include "memcard/scemc.h"

#include "fs/fmfileinfo.h"

#include "fs/mcsaveproperty.h"

#include "fs/thfileoperation.h"

#include "app/variables.h"

/* Current operation: 1 copy, 2 move, 3 delete, 4 rename, 6 lstat; 0 none. */
static int g_fileState = 0;
static thFileOperation *g_fileJob = 0;

/* Filled by ioctl 0x53E0 on /dev/sda (80 bytes). */
struct UsbStat {
	/* 0x00 */ char pad0[20];
	/* 0x14 */ int present;
	/* 0x18 */ char pad24[56];
}; /* size = 0x50 */

/*
 * fileCopy(src, dst): starts a background copy; -1 when a job is already
 * running or an argument is empty. fileMove, fileDelete and fileRename work
 * the same way.
 */
static cell AMX_NATIVE_CALL n_fileCopy(AMX *amx, cell *params)
{
	if (g_fileJob != 0)
		return -1;

	string srcUrl = CellToString(amx, params[1]);

	if (srcUrl.length() == 0)
		return -1;

	string dstUrl = CellToString(amx, params[2]);

	if (dstUrl.length() == 0)
		return -1;

	g_fileJob = 0;

	string srcPath;
	string dstPath;
	BBUrl u;

	u.setUrlString(srcUrl);
	srcPath = u.getPathString();
	u.setUrlString(dstUrl);
	dstPath = u.getPathString();
	g_fileState = 1;

	thFileOperation *job = new thFileOperation;
	g_fileJob = job;
	job->setSrcPath(srcPath);
	g_fileJob->m_dstPath = dstPath;
	g_fileJob->m_overwriteFlag = 1;
	g_fileJob->m_oprCommand = thFileOperation::OPR_COPY;
	g_fileJob->start();
	return 0;
}

static cell AMX_NATIVE_CALL n_fileMove(AMX *amx, cell *params)
{
	if (g_fileJob != 0)
		return -1;

	string srcUrl = CellToString(amx, params[1]);

	if (srcUrl.length() == 0)
		return -1;

	string dstUrl = CellToString(amx, params[2]);

	if (dstUrl.length() == 0)
		return -1;

	g_fileJob = 0;

	string srcPath;
	string dstPath;
	BBUrl u;

	u.setUrlString(srcUrl);
	srcPath = u.getPathString();
	u.setUrlString(dstUrl);
	dstPath = u.getPathString();
	g_fileState = 2;

	thFileOperation *job = new thFileOperation;
	g_fileJob = job;
	job->setSrcPath(srcPath);
	g_fileJob->m_dstPath = dstPath;
	g_fileJob->m_overwriteFlag = 1;
	g_fileJob->m_oprCommand = thFileOperation::OPR_MOVE;
	g_fileJob->start();
	return 0;
}

static cell AMX_NATIVE_CALL n_fileDelete(AMX *amx, cell *params)
{
	if (g_fileJob != 0)
		return -1;

	string url = CellToString(amx, params[1]);

	if (url.length() == 0)
		return -1;

	g_fileJob = 0;

	string path;
	BBUrl u;

	u.setUrlString(url);
	path = u.getPathString();
	g_fileState = 3;

	thFileOperation *job = new thFileOperation;
	g_fileJob = job;
	job->setSrcPath(path);
	g_fileJob->m_oprCommand = thFileOperation::OPR_REMOVE;
	g_fileJob->start();
	return 0;
}

static cell AMX_NATIVE_CALL n_fileRename(AMX *amx, cell *params)
{
	if (g_fileJob != 0)
		return -1;

	string url = CellToString(amx, params[1]);

	if (url.length() == 0)
		return -1;

	string name = CellToString(amx, params[2]);

	if (name.length() == 0)
		return -1;

	g_fileJob = 0;

	string path;
	BBUrl u;

	u.setUrlString(url);
	path = u.getPathString();
	g_fileState = 4;

	thFileOperation *job = new thFileOperation;
	g_fileJob = job;
	job->setSrcPath(path);
	g_fileJob->m_newName = name;
	g_fileJob->m_oprCommand = thFileOperation::OPR_RENAME;
	g_fileJob->start();
	return 0;
}

static cell AMX_NATIVE_CALL n_fileMkdir(AMX *amx, cell *params)
{
	string url = CellToString(amx, params[1]);

	if (url.length() == 0)
		return -1;

	string path;
	BBUrl u;

	u.setUrlString(url);
	path = u.getPathString();
	return BBmkdirCPP(path.c_str(), 0777);
}

/* fileLstat(url, &mode, &size): returns the result of BBlstatCPP. */
static cell AMX_NATIVE_CALL n_fileLstat(AMX *amx, cell *params)
{
	cell *cptr;
	string url = CellToString(amx, params[1]);

	if (url.length() == 0)
		return -1;

	string path;
	BBUrl u;

	u.setUrlString(url);
	path = u.getPathString();

	struct stat st;

	g_fileState = 6;

	int ret = BBlstatCPP(path.c_str(), &st);

	if (amx_GetAddr(amx, params[2], &cptr) == 0)
		*cptr = st.st_mode;
	if (amx_GetAddr(amx, params[3], &cptr) == 0)
		*cptr = st.st_size;
	return ret;
}

/*
 * fileSync(&percent, &status): polls the background job. Returns 0 while it
 * runs, 1 when it is over (or for a finished immediate operation), -1 when
 * there is no operation.
 */
static cell AMX_NATIVE_CALL n_fileSync(AMX *amx, cell *params)
{
	cell *cptr;
	int state;

	state = g_fileState;
	if (state <= 0)
		goto bad;
	if (state >= 5) {
		if (state >= 7)
			goto bad;
		goto done;
	}
	if (g_fileJob == 0)
		return 1;
	if (g_fileJob->m_abortFlag != 0) {
		g_fileJob->release();
		g_fileJob = 0;
		return 1;
	}
	if (g_fileJob->IsRelease() != 0) {
		g_fileJob = 0;
		return 1;
	}
	if (amx_GetAddr(amx, params[1], &cptr) == 0) {
		int percent = g_fileJob->m_percent;

		*cptr = percent;
	}
	if (g_fileJob->IsEnd() == 0) {
		if (amx_GetAddr(amx, params[2], &cptr) == 0)
			*cptr = 0;
		return 0;
	}
	if (amx_GetAddr(amx, params[2], &cptr) == 0) {
		int endStatus = g_fileJob->m_endStatus;

		*cptr = endStatus;
	}
	if (g_fileJob->IsEnd() == 0)
		return 0;
	g_fileJob->release();
	g_fileJob = 0;
	goto done;
bad:
	return -1;
done:
	g_fileState = 0;
	return 1;
}

/* fileGetFreeSize(url, &kb): free space in KB, clamped to 32 bits. */
static cell AMX_NATIVE_CALL n_fileGetFreeSize(AMX *amx, cell *params)
{
	cell *cptr;
	string path = CellToString(amx, params[1]);
	BBUrl url;
	unsigned long long free;

	url.setUrlString(path);
	path = url.getPathString();
	if (thFileOperation::getFreeSize(0, 0, 0, path, free) < 0)
		return -1;
	if (amx_GetAddr(amx, params[2], &cptr) == 0) {
		free >>= 10;
		if (free > 0xFFFFFFFFu)
			free = 0xFFFFFFFF;
		*cptr = free;
	}
	return 0;
}

/* fileGetUSBStat(&present): 1 when a USB storage device is attached. */
static cell AMX_NATIVE_CALL n_fileGetUSBStat(AMX *amx, cell *params)
{
	UsbStat st;
	cell *cptr;
	const char *dev;
	int stat;
	int fd;

	dev = "/dev/sda";
	stat = 0;
	fd = open(dev, O_RDONLY | O_NONBLOCK);
	if (fd > 0) {
		ioctl(fd, 0x53E0, &st);
		stat = st.present != 0;
	}
	if (amx_GetAddr(amx, params[1], &cptr) == 0)
		*cptr = stat;
	fd = open(dev, O_RDONLY);
	if (fd > 0)
		close(fd);
	return 0;
}

/*
 * Translates the BB_MS_STATUS variable into the error code of the *USB
 * natives: 1 locked, 2 unloaded, 3 cannot open, 4 busy, 0 ready.
 */
static cell usb_prepare()
{
	string name = "BB_MS_STATUS";
	list<string> values;
	string value;

	Variables::GetVariable(name, values);
	if (values.size() != 0) {
		value = *values.begin();
		if (value == "BB_MS_LOCKED")
			return 1;
		if (value == "BB_MS_UNLOAD")
			return 2;
		if (value == "BB_MS_CANT_OPEN")
			return 3;
		if (value == "BB_MS_BUSY")
			return 4;
		if (value == "BB_MS_READY")
			return 0;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_fileCopy2USB(AMX *amx, cell *params)
{
	cell r;

	r = usb_prepare();
	if (r != 0)
		return r;
	return n_fileCopy(amx, params);
}

static cell AMX_NATIVE_CALL n_fileMove2USB(AMX *amx, cell *params)
{
	cell r;

	r = usb_prepare();
	if (r != 0)
		return r;
	return n_fileMove(amx, params);
}

static cell AMX_NATIVE_CALL n_fileDeleteUSB(AMX *amx, cell *params)
{
	cell r;

	r = usb_prepare();
	if (r != 0)
		return r;
	return n_fileDelete(amx, params);
}

static cell AMX_NATIVE_CALL n_fileRenameUSB(AMX *amx, cell *params)
{
	cell r;

	r = usb_prepare();
	if (r != 0)
		return r;
	return n_fileRename(amx, params);
}

static cell AMX_NATIVE_CALL n_fileMkdirUSB(AMX *amx, cell *params)
{
	cell r;

	r = usb_prepare();
	if (r != 0)
		return r;
	return n_fileMkdir(amx, params);
}

AMX_NATIVE_INFO smallext_file[] = {
	{ "fileCopy", n_fileCopy },
	{ "fileMove", n_fileMove },
	{ "fileDelete", n_fileDelete },
	{ "fileRename", n_fileRename },
	{ "fileMkdir", n_fileMkdir },
	{ "fileLstat", n_fileLstat },
	{ "fileSync", n_fileSync },
	{ "fileGetFreeSize", n_fileGetFreeSize },
	{ "fileGetUSBStat", n_fileGetUSBStat },
	{ "fileCopy2USB", n_fileCopy2USB },
	{ "fileMove2USB", n_fileMove2USB },
	{ "fileDeleteUSB", n_fileDeleteUSB },
	{ "fileRenameUSB", n_fileRenameUSB },
	{ "fileMkdirUSB", n_fileMkdirUSB },
	{ 0, 0 },
};
