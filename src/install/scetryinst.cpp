/*
 * sceTRYInst*: C layer over TRYInstManager and TRYInstaller; most functions
 * forward to a method of the static TRYInstManager singleton g_manager.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tryinstmanager.h"
#include "tryinsthistory.h"
#include "trycreatefs.h"
#include "lib/scepfsd.h"

extern "C" {

static TRYInstManager *g_manager = 0;
static int g_managerFlag = 0;
static int g_verFlag = 0;

/* Compares the string at offset 8 of pkg with "1.00". */
int sceTRYLibVersionCheck(void *pkg)
{
	char *ver = *(char **)((char *)pkg + 8);

	return strcmp(ver, "1.00") == 0 ? 0 : -27713;
}

/* Mounts partition devPath of "hdd0" on a TRYAbstFS (mode 1), calls
   try_parseTitleSubID (fills two local TRYInstPkg), then by mode (1/2/other)
   picks ".BINPKG.VER" or ".DATAPKG.VER" and the corresponding package (both 0 for
   "other") and calls try_checkInstHistory. Sets g_verFlag if either package has isPlus set.
   Errors: mount == -27402 -> return 0 at once; any other mount failure ->
   -27710; parseTitleSubID/checkInstHistory < 0 -> UMount, then that code. */
int sceTRYInstVerCheck(const char *devPath, char *titleSubId, int mode)
{
	TRYAbstFS *fs;
	int ret;
	TRYInstPkg binPkg, dataPkg;
	char *modeStr = 0;
	TRYInstPkg *pkgArg = 0;

	fs = TRYCreateFS(1);

	ret = fs->Mount(0, 0, 0, "hdd0", devPath);

	if (ret == -27402) {
		delete fs;
		return 0;
	}

	if (ret < 0) {
		ret = -27710;
		goto closeNoUmount;
	}

	ret = try_parseTitleSubID(titleSubId, &binPkg, &dataPkg);
	if (ret < 0)
		goto closeWithUmount;

	switch (mode) {
	case 1:
		modeStr = ".BINPKG.VER";
		pkgArg = &binPkg;
		break;
	case 2:
		modeStr = ".DATAPKG.VER";
		pkgArg = &dataPkg;
		break;
	}

	ret = try_checkInstHistory(fs, modeStr, pkgArg);
	if (ret < 0)
		goto closeWithUmount;

	fs->UMount();
	delete fs;

	if (binPkg.isPlus == 1 || dataPkg.isPlus == 1)
		g_verFlag = 1;
	return ret;

closeWithUmount:
	fs->UMount();
closeNoUmount:
	delete fs;
	return ret;
}

/* Creates the singleton on first use, then MountContentsPat ->
   SetInstPkgFS_HDD -> PreInstCheck. */
int sceTRYInstPreCheck()
{
	int r;

	if (g_manager == 0)
		g_manager = new TRYInstManager;
	if (g_manager->MountContentsPat() < 0)
		return -27701;
	g_manager->SetInstPkgFS_HDD();
	r = g_manager->PreInstCheck();
	if (r < 0)
		return r;
	return 0;
}

/* Checks that hdd0 has room for the installation: mounts the work partition
   __contents on pfs10, measures block size and free blocks with two devctl
   calls (0x5001 = block size, 0x5002 = free blocks) and compares them with the
   requested size plus a fixed 128 MB margin. Then tries to mount the content
   partition on pfs11: if it does not exist (mount == -2) counts the free
   partitions, otherwise measures its space. 128 = sectors per partition.
   Error codes -27701 to -27711. */
int sceTRYInstSizeCheck(const char *totalSize, const char *name, const char *dataSize, const char *otherSize)
{
	char path[128];
	int mounted10;
	int mounted11;
	int r;
	int blockSize;
	int freeBlocks;
	int parts;
	unsigned int total;

	mounted11 = 0;
	mounted10 = 0;
	sprintf(path, "%s:%s", "hdd0", "__contents");
	if (scepfsMount("pfs10:", path, 1, 0, 0) < 0) {
		r = -27701;
		goto out;
	}
	mounted10 = 1;
	scepfsRemove("pfs10:/dlcontents/dnas_no_key/content.dat");

	blockSize = scepfsDevctl("pfs10:", 20481, 0, 0, 0, 0);
	freeBlocks = scepfsDevctl("pfs10:", 20482, 0, 0, 0, 0);
	if (blockSize < 0 || freeBlocks < 0) {
		r = -27704;
		goto out;
	}
	total = atoi(totalSize) - 1;
	if ((unsigned int)freeBlocks < (total + blockSize) / blockSize + (0x7ffffff + blockSize) / blockSize) {
		r = -27705;
		goto out;
	}

	sprintf(path, "%s:%s", "hdd0", name);
	r = scepfsMount("pfs11:", path, 1, 0, 0);
	switch (-r) {
	case 0:
		r = 0;
		mounted11 = 1;
		break;
	case 2:
		r = 1;
		break;
	case 1:  /* redundant as written */
	case 13:  /* redundant as written */
	case 24:  /* redundant as written */
	default:
		break;
	}
	if (r < 0) {
		r = -27710;
		goto out;
	}
	if (r == 1) {
		int needParts = atoi(dataSize) / 128;
		int have;
		int want;

		r = try_GetUnusedPartitionCount("hdd0:");
		parts = r;
		if (r < 0) {
			r = -27706;
			goto out;
		}
		r = try_GetEmptyPartitionCount("hdd0:");
		parts += r;
		if (r < 0) {
			r = -27707;
			goto out;
		}
		if (parts < needParts) {
			r = -27708;
			goto out;
		}
		want = (unsigned int)atoi(otherSize) >> 17;
		have = ((unsigned int)atoi(dataSize) >> 7) << 10;
		if (have < want) {
			r = -27709;
			goto out;
		}
	} else if (g_verFlag == 0) {
		blockSize = scepfsDevctl("pfs11:", 20481, 0, 0, 0, 0);
		freeBlocks = scepfsDevctl("pfs11:", 20482, 0, 0, 0, 0);
		if (blockSize < 0 || freeBlocks < 0) {
			r = -27711;
			goto out;
		}
		if ((unsigned int)freeBlocks < (unsigned int)atoi(otherSize) / blockSize) {
			r = -27709;
			goto out;
		}
	}
	g_verFlag = 0;
	scepfsUmount("pfs10:");
	if (mounted11)
		scepfsUmount("pfs11:");
	return 0;

out:
	g_verFlag = 0;
	if (mounted10)
		scepfsUmount("pfs10:");
	if (mounted11)
		scepfsUmount("pfs11:");
	return r;
}

/* Initialises the manager (once, via g_managerFlag), then loads info,
   DNAS archive and def in sequence. */
int sceTRYInstInit()
{
	int r;

	if (g_managerFlag != 0)
		return -27751;

	r = g_manager->LoadInstInfo();
	if (r < 0)
		return -27754;

	r = g_manager->Initdnas_ar();
	if (r < 0)
		return -27752;

	r = g_manager->LoadInstDef();
	if (r < 0)
		return -27753;

	g_managerFlag = 1;
	return 0;
}

int sceTRYInstStart()
{
	return g_manager->StartInstThread();
}

/* Stops the thread, frees the resources, deletes the manager if it still
   exists, then clears g_manager and g_managerFlag. */
int sceTRYInstExit()
{
	if (g_manager != 0) {
		int r = g_manager->StopInstThread();
		if (r < 0)
			return -27852;
		g_manager->FreeResource();
		delete g_manager;
	}
	g_manager = 0;
	g_managerFlag = 0;
	return 0;
}

int sceTRYInstGetLicMessage(sceTRYInstLicMessage *out)
{
	return g_manager->GetLicMessage(out);
}

int sceTRYInstGetStatus(sceTRYInstStatus *out)
{
	return g_manager->GetStatus(out);
}

/* Before initialisation: outBuf[0] = 0 and returns -27953. */
int sceTRYInstGetDNASDatapath(char *outBuf)
{
	if (g_manager != 0)
		return g_manager->GetDNASDatapath(outBuf);
	outBuf[0] = 0;
	return -27953;
}

} /* extern "C" */
