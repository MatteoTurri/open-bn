/*
 * BB file access wrappers: extern "C" *CPP entry points, each forwarding to
 * the C function of the same name without the suffix.
 */

#include <stdarg.h>
#include "bbfile.h"
#include "bbfile_dir.h"
#include "bbfileaccess.h"

extern "C" {

/* SIGTERM handler of the smbumount path: does nothing. */
void sigterm_smbumount(int sig)
{
	(void)sig;
}

/* --- wrappers --- */

int BBFileAccessInitCPP()
{
	return BBFileAccessInit();
}

int BBFileAccessExitCPP()
{
	return BBFileAccessExit();
}

int BBfopenCPP(const char *path, const char *mode)
{
	return BBfopen(path, mode);
}

int BBfcloseCPP(int fp)
{
	return BBfclose(fp);
}

int BBfreadCPP(void *buf, int size, int nmemb, int fp)
{
	return BBfread(buf, size, nmemb, fp);
}

int BBfwriteCPP(const void *buf, int size, int nmemb, int fp)
{
	return BBfwrite(buf, size, nmemb, fp);
}

int BBfseekCPP(int fp, int offset, int whence)
{
	return BBfseek(fp, offset, whence);
}

int BBftellCPP(int fp)
{
	return BBftell(fp);
}

/* mode is read only when flags has 0x100 (create); it defaults to 0664. */
int BBopenCPP(const char *path, int flags, ...)
{
	int mode = 436;
	if (flags & 0x100) {
		va_list ap;
		va_start(ap, flags);
		mode = va_arg(ap, int);
		va_end(ap);
	}
	return BBopen(path, flags, mode);
}

int BBcloseCPP(int fd)
{
	return BBclose(fd);
}

int BBreadCPP(int fd, void *buf, int nbytes)
{
	return BBread(fd, buf, nbytes);
}

int BBwriteCPP(int fd, const void *buf, int nbytes)
{
	return BBwrite(fd, buf, nbytes);
}

int BBlseekCPP(int fd, int offset, int whence)
{
	return BBlseek(fd, offset, whence);
}

int BBmkdirCPP(const char *path, int mode)
{
	return BBmkdir(path, mode);
}

int BBrmdirCPP(const char *path)
{
	return BBrmdir(path);
}

int BBrenameCPP(const char *oldpath, const char *newpath)
{
	return BBrename(oldpath, newpath);
}

int BBremoveCPP(const char *path)
{
	return BBremove(path);
}

int BBlstatCPP(const char *path, void *buf)
{
	return BBlstat(path, (struct stat *)buf);
}

int BBlstatSVCPP(const char *path, void *buf)
{
	return BBlstatSV(path, (BBStatSV *)buf);
}

int BBopendirCPP(const char *path)
{
	return BBopendir(path);
}

int BBclosedirCPP(int dirp)
{
	return BBclosedir(dirp);
}

struct dirent *BBreaddirCPP(int dirp)
{
	return BBreaddir(dirp);
}

iox_dirent_t BBgetPFSdirbufCPP(int index)
{
	return BBgetPFSdirbuf(index);
}

int BBgetPFSattrCPP(const char *path, void *attr)
{
	return BBgetPFSattr(path, (int *)attr);
}

int BBsetPFSattrCPP(const char *path, int attr)
{
	return BBsetPFSattr(path, attr);
}

int BBgetMCattrCPP(const char *path, void *attr)
{
	return BBgetMCattr(path, (unsigned short *)attr);
}

int BBsetMCattrCPP(const char *path, unsigned short attr)
{
	return BBsetMCattr(path, attr);
}

int BBgetMCInfoCPP(int port, int slot, void *type, void *freeBlocks, void *formatted)
{
	return BBgetMCInfo(port, slot, (int *)type, (int *)freeBlocks, (int *)formatted);
}

int BBgetPS1VMCInfoCPP(int port, int slot, void *outFree, void *outAttached, void *outSize)
{
	return BBgetPS1VMCInfo(port, slot, (int *)outFree, (int *)outAttached, (int *)outSize);
}

int BBsetPFSmodeCPP(const char *path, int mode)
{
	return BBsetPFSmode(path, mode);
}

int BBgetPFSPartitionFreeSizeCPP(const char *path, unsigned long long *out)
{
	return BBgetPFSPartitionFreeSize(path, out);
}

int BBgetPFSPartitionFreeZoneCPP(const char *path, int *out1, int *out2)
{
	return BBgetPFSPartitionFreeZone(path, out1, out2);
}

int BBgetBNPFSPartitionFreeSizeCPP(const char *path, unsigned long long *out)
{
	return BBgetBNPFSPartitionFreeSize(path, out);
}

int BBgetBNPFSPartitionFreeZoneCPP(const char *path, int *freeZones, int *zoneSize)
{
	return BBgetBNPFSPartitionFreeZone(path, freeZones, zoneSize);
}

int BBsetPFSstatCPP(const char *path, void *st, int mode)
{
	return BBsetPFSstat(path, st, mode);
}

int BBgetPFSstatCPP(const char *path, void *st)
{
	return BBgetPFSstat(path, st);
}

int BBMCFormatCPP(int port, int slot)
{
	return BBMCFormat(port, slot);
}

int BBpfsFileAttrReadCPP(int fd, void *attr)
{
	return BBpfsFileAttrRead(fd, attr);
}

int BBpfsFileAttrWriteCPP(int fd, void *attr)
{
	return BBpfsFileAttrWrite(fd, attr);
}

} /* extern "C" */
