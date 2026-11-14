/*
 * TRYInstaller methods: the install steps (partition, jacket, registry,
 * pat info, files) and the copy helpers.
 */

#include "tryinstaller.h"
#include "tryinsthistory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include "lib/scepfsd.h"

/* Copies the ContentInfo and GameContentInfo blobs by value; mkpl becomes the
   m_patList list. */
TRYInstaller::TRYInstaller(TRYInstManager *mgr, pthread_cond_t *cond, pthread_mutex_t *mutex, int arg4, TRYAbstFS *fs1,
    int srcFd, TRYAbstFS *fs2, int jacketOffset, int readOffset, ContentInfo *ci, GameContentInfo *gci,
    instMkPartitionList *mkpl)
{
	m_cond = cond;
	m_mgr = mgr;
	m_mutex = mutex;
	unk_0C = arg4;
	m_srcFs = fs1;
	m_srcFd = srcFd;
	m_contentsFs = fs2;
	m_jacketOffset = jacketOffset;
	m_readOffset = readOffset;
	m_contentInfo = *ci;
	m_gameContentInfo = *gci;
	m_patList = mkpl;
	m_allocCount = 0;
	m_allocArray = 0;
}

/* Frees every element of m_allocArray, then the array. */
TRYInstaller::~TRYInstaller()
{
	for (int i = 0; i < m_allocCount; i++) {
		void *p = m_allocArray[i];
		if (p)
			free(p);
	}
	if (m_allocArray)
		free(m_allocArray);
}

/* Dispatches the install step by command (TRYINST_ENTRY_*). */
int TRYInstaller::Install(int cmd, void *arg)
{
	switch (cmd) {
	case TRYINST_ENTRY_MAINPAT:
		return MainPat((sceTRYInstDefMainPat *)arg);
	case TRYINST_ENTRY_EXTJACKET:
		return ExtJacket((sceTRYInstDefExtJacket *)arg);
	case TRYINST_ENTRY_REGISTDB:
		return RegistDB((sceTRYInstDefRegistDB *)arg);
	case TRYINST_ENTRY_WRITEPATINFO:
		return WritePatInfoInit((sceTRYInstDefWritePatInfo *)arg);
	case TRYINST_ENTRY_PFSINSTALL:
		return PFSInstall((sceTRYInstDefPFSIntall *)arg);
	case TRYINST_ENTRY_ALL:
		return All((sceTRYInstDefAll *)arg);
	case TRYINST_ENTRY_FILE:
		return File((sceTRYInstDefFile *)arg);
	default:
		return -1;
	case TRYINST_ENTRY_NOP:
		return 0;
	}
}

/* Looks up the pattern by name and stores the result; returns -27601 if not
   found, otherwise records the name in m_patName. */
int TRYInstaller::MainPat(sceTRYInstDefMainPat *def)
{
	TRYAbstFS *found = SearchFSObj(def->name, &m_searchType, m_searchPathBuf, m_buf1, m_buf2);
	m_foundFsHandle = found;
	if (!found)
		return -27601;
	strcpy(m_patName, def->name);
	return 0;
}

/* Propagates a negative result unchanged (pending error in ret). */
#define JACKET_CHECK(e) do { if ((e) < 0) goto out; } while (0)

/*
 * Extracts the "jacket" archive (the title cover images) into the `res`
 * directory of the destination PFS. The archive is read from the package
 * source (m_srcFs/m_srcFd, from m_jacketOffset): a 16-byte header "TJP" +
 * "1.00" + total size, then records {type, ?, name length, size} + name.
 * Only type-2 records produce a file and count against the remaining size.
 */
int TRYInstaller::ExtJacket(sceTRYInstDefExtJacket *def)
{
	char dir[1024];
	char unused[1024];  /* unused, but part of the layout */
	struct {
		char magic[4];
		char version[4];
		unsigned int pad;
		unsigned int size;
	} hdr;
	char path[1024];
	char name[128];
	struct {
		int type;
		int pad;
		int namelen;
		unsigned int size;
	} ent;
	int ret;
	int n;
	unsigned int remain;
	TRYAbstFS *fs;

	unused[0] = 0;
	sprintf(dir, "res");

	fs = m_foundFsHandle;
	ret = fs->Mkdir(dir, 0777);
	if (ret < 0) {
		if (ret != -EEXIST) {
			ret = -27607;
			goto out;
		}
	}
	changeUID_GID(fs, dir);

	if (m_jacketOffset == 0)
		return 0;

	if (m_srcFs->Seek(m_srcFd, m_jacketOffset, 0) < 0) {
		ret = -27608;
		goto out;
	}

	ret = m_srcFs->Read(m_srcFd, &hdr, 16);
	if (ret != 16) {
		ret = -27609;
		goto out;
	}

	if (strncmp(hdr.magic, "TJP", 3) != 0 || strncmp(hdr.version, "1.00", 4) != 0) {
		ret = -27614;
		goto out;
	}

	remain = hdr.size;
	while (remain != 0) {
		if (m_srcFs->Read(m_srcFd, &ent, 16) != 16)  /* NOTE: bug: returns ret, still 16 */
			goto out;

		n = m_srcFs->Read(m_srcFd, name, ent.namelen);
		if (n != ent.namelen)
			goto out;

		name[n] = 0;
		sprintf(path, "%s/%s", dir, name);

		if (ent.type == 2) {
			JACKET_CHECK(createJacketFile(fs, path, ent.size));  /* redundant as written */
			changeUID_GID(fs, path);
			remain -= ent.size;
		}
	}

	sprintf(path, "pfs://%s/res", m_patName);
	strcpy(m_gameContentInfo.jacketPath, path);

	return 0;
out:
	return ret;
}

/* Splits the package title id into its BIN/DATA halves (nothing to do, and
   success, if that fails), then writes the history of each half (".BINPKG.VER",
   ".DATAPKG.VER") on the found filesystem, each with its own error code. */
int TRYInstaller::RegistDB(sceTRYInstDefRegistDB *)
{
	TRYInstPkg binPkg;
	TRYInstPkg dataPkg;
	if (try_parseTitleSubID(m_contentInfo.titleSubId, &binPkg, &dataPkg) >= 0) {
		if (try_writeInstHistory(m_foundFsHandle, ".BINPKG.VER", &binPkg) < 0)
			return -27623;
		if (try_writeInstHistory(m_foundFsHandle, ".DATAPKG.VER", &dataPkg) < 0)
			goto err;
	}
	return 0;
err:
	return -27624;
}

/* Prepares the buffers for WritePatInfo: allocates the block array, copies
 * the 9-byte "PS2ICON3D" header, and for each block reads its format, rounds
 * the length up to a multiple of 512, allocates (memalign 64), reads the body
 * and reports progress to the manager under the mutex. The {offset, length}
 * records go to m_patRec.
 * Returns -27615 (array malloc), -27605 (read_format or memalign),
 * -27606 (read_body). */
int TRYInstaller::WritePatInfoInit(sceTRYInstDefWritePatInfo *def)
{
	char fmtbuf[1024];
	dnas_archive_format info;
	unsigned int off = 512;
	unsigned int size;
	int i;
	int ret;

	m_allocCount = def->count;
	memset(m_patInfoHdr, 0, 512);

	if ((m_allocArray = (void **)malloc(def->count * 4)) == 0) {
		ret = -27615;
		goto error;
	}

	memset(m_allocArray, 0, m_allocCount * 4);

	memcpy(m_patInfoHdr, "PS2ICON3D", 9);

	for (i = 0; i < m_allocCount; i++) {
		m_allocArray[i] = 0;
		if (_sce_dnas_ar_nokey_read_format(fmtbuf, &info) < 0) {
			ret = -27605;
			goto error;
		}

		size = (((int)info.size + 511) / 512) * 512;
		m_allocArray[i] = memalign(64, size);
		if (m_allocArray[i] == 0) {
			ret = -27605;
			goto error;
		}
		memset(m_allocArray[i], 0, size);

		if (_sce_dnas_ar_nokey_read_body(m_allocArray[i], info.size, 0) < 0) {
			ret = -27606;
			goto error;
		}

		pthread_mutex_lock(m_mutex);
		m_mgr->SetStatus(TRYINST_ADD_ACCUM2, info.size);
		pthread_mutex_unlock(m_mutex);

		m_patRec[i].off = off;
		m_patRec[i].size = info.size;
		off += (((int)info.size + 511) / 512) * 512;
	}

	return 0;

error:
	return ret;
}

/* For a type-1 partition unmounts m_foundFsHandle first, then writes the
 * "pat info" header and all allocated blocks to the file named by
 * m_searchPathBuf, closes it and mounts the partition again. Returns -27617
 * (UMount failed), -27602 (open failed), -27604 (short write), -27616 (Mount
 * failed). */
int TRYInstaller::WritePatInfo()
{
	int fd = -1;
	int ret;
	int i;

	if (m_searchType == 1) {
		if (m_foundFsHandle->UMount() < 0) {
			ret = -27617;
			goto err;
		}
	}

	fd = scepfsOpen(m_searchPathBuf, 3);
	if (fd < 0) {
		ret = -27602;
		goto err;
	}

	if (copyFileHddAttr(fd, m_patInfoHdr, 512) != 512) {
		ret = -27604;
		goto err;
	}

	for (i = 0; i < m_allocCount; i++) {
		if ((unsigned int)copyFileHddAttr(fd, m_allocArray[i], m_patRec[i].size) != m_patRec[i].size) {
			ret = -27604;
			goto err;
		}
	}

	scepfsClose(fd);

	if (m_foundFsHandle->Mount(0, m_buf1, 0, "hdd0", m_patName) >= 0)
		return 0;

	ret = -27616;

err:
	/* On -27616 Mount is called a second time. */
	if (ret == -27604)
		scepfsClose(fd);
	if (ret != -27617)
		m_foundFsHandle->Mount(0, m_buf1, 0, "hdd0", m_patName);
	return ret;
}

/* Reads the format of m_pfsName into m_format.type; on failure clears the name
   and returns -27605. */
int TRYInstaller::PFSInstall(sceTRYInstDefPFSIntall *)
{
	if (_sce_dnas_ar_nokey_read_format(m_pfsName, &m_format) >= 0)
		return 0;
	m_pfsName[0] = 0;
	return -27605;
}

/* "All" install loop: reads the next entry format (updates m_format.type) and
 * dispatches on it (1 = create directory, 2 = copy file), then reads again.
 * Returns -27622 (mkdir failed, not EEXIST), -27605 (read_format < 0), 0 when
 * read_format returns -10000 (end of list). */
int TRYInstaller::All(sceTRYInstDefAll *def)
{
	int r;
	char *path;

again:
	switch (m_format.type) {
	default:
		path = m_pfsName;
		break;
	case 1:
		r = (m_foundFsHandle)->Mkdir(m_pfsName, 0777);
		path = m_pfsName;
		if (r < 0 && r != -EEXIST) {
			r = -27622;
			goto out;
		}
		goto chown;
	case 2:
		if (m_format.attr == 0) {
			m_mgr->SetStatus(TRYINST_SET_UNK10, 1);
			r = copyFile(m_foundFsHandle, m_pfsName, m_format.size, 0);
			path = m_pfsName;
			if (r != (int)m_format.size)
				goto out;
		} else {
			int len;

			path = m_pfsName;
			if (m_format.attr < 0)
				break;
			if (m_format.attr >= 3)
				break;
			m_mgr->SetStatus(TRYINST_SET_UNK10, 2);
			_sce_dnas_ar_nokey_init_personalize();
			len = _sce_dnas_ar_nokey_get_personalize_length();
			m_mgr->SetStatus(TRYINST_ADD_BYTES_TOTAL, len);
			r = copyFile(m_foundFsHandle, path, len, 1);
			if (r != len)
				goto out;
			_sce_dnas_ar_nokey_exit_personalize();
		}
chown:
		r = changeUID_GID(m_foundFsHandle, path);
		if (r < 0)
			goto out;
		break;
	}
	r = _sce_dnas_ar_nokey_read_format(path, &m_format);
	if (r == -10000)
		return 0;
	if (r >= 0)
		goto again;

	r = -27605;
out:
	return r;
}

/* Installs a single file: reuses m_foundFsHandle when the pattern name
   equals m_patName, otherwise looks it up with SearchFSObj; copies the file if
   m_format.type == 2, then reads the next format (any result >= 0 or -10000 is
   success). */
int TRYInstaller::File(sceTRYInstDefFile *def)
{
	int r;
	TRYAbstFS *fs;
	char *defName;

	if (strcmp((char *)def, m_pfsName) != 0) {
		r = -27618;
		goto out;
	}
	defName = (char *)def + 128;
	if (strcmp(defName, m_patName) == 0) {
		fs = m_foundFsHandle;
	} else {
		char pathBuf[64];
		char name1[16];
		char name2[16];
		int foundType;
		fs = SearchFSObj(defName, &foundType, pathBuf, name1, name2);
		if (!fs) {
			r = -27601;
			goto out;
		}
		if (foundType == 2) {
			r = -27619;
			goto out;
		}
	}
	if (m_format.type == 2) {
		r = copyFile(fs, m_pfsName, m_format.size, 0);
		if (r != (int)m_format.size)
			goto out;
	}
	r = _sce_dnas_ar_nokey_read_format(m_pfsName, &m_format);
	if (r == -10000)
		return 0;
	if (r >= 0)
		return 0;
	r = -27605;
out:
	return r;
}

/* Finds the m_patList node named `name`; stores its type, builds pathBuf
   "hdd0:<name>,<name1>", copies data->name1/name2 (NULL unless type == 1) to
   outName1/outName2 and returns the node fs. */
TRYAbstFS *TRYInstaller::SearchFSObj(char *name, int *outType, char *pathBuf, char *outName1, char *outName2)
{
	instMkPartitionList *node = m_patList;
	while (node) {
		if (strcmp(node->name, name) == 0) {
			char *name1 = 0;
			char *name2 = 0;
			int type = node->type;
			if (type == 1) {
				name1 = node->data->name1;
				name2 = node->data->name2;
			}
			*outType = type;
			sprintf(pathBuf, "hdd0:%s,%s", node->name, name1);
			strcpy(outName1, name1);
			strcpy(outName2, name2);
			return node->fs;
		}
		node = node->next;
	}
	return 0;
}

/* Unmounts and removes the partition of every node that has a filesystem;
   frees nothing (the dtor does). */
int TRYInstaller::RemoveAllPat()
{
	instMkPartitionList *node = m_patList;
	if (node) {
		do {
			char *partName = node->name;
			char *password = 0;
			if (node->type == 1) {
				password = node->data->name1;
			}
			TRYAbstFS *fs = node->fs;
			if (fs) {
				fs->UMount();
				fs->RmPart(0, partName, password);
			}
			node = node->next;
		} while (node);
	}
	return 0;
}

/* Writes nbyte rounded up to a multiple of 512; returns nbyte, or -27604 on
   a short write. */
int TRYInstaller::copyFileHddAttr(int fd, void *buf, unsigned int nbyte)
{
	unsigned int aligned = ((nbyte + 511) >> 9) << 9;
	int r = scepfsWrite(fd, buf, aligned);
	int ret = -27604;
	if ((unsigned int)r == aligned)
		ret = (int)nbyte;
	return ret;
}

/* Creates the file (Open 0x602, 0666) and copies `size` bytes into it in
   8 KB chunks read with _sce_dnas_ar_nokey_read_body, reporting progress to
   the manager under the mutex (status 4 for content, 6 for flag 1).
   Returns -27620 open, -27606 short read, -27621 short write. */
int TRYInstaller::copyFile(TRYAbstFS *fs, char *path, unsigned long long size, int flag)
{
	char buf[8192];
	int total = 0;
	int fd, n, len;

	fd = fs->Open(path, 0x602, 0666);
	if (fd < 0)
		return -27620;

	while (size != 0) {
		len = (size > 8192) ? 8192 : (int)size;
		n = _sce_dnas_ar_nokey_read_body(buf, len, flag);
		if (n != len) {
			n = -27606;
			goto error;
		}
		n = fs->Write(fd, buf, n);
		if (n != len) {
			n = -27621;
			goto error;
		}
		total += n;
		size -= n;
		pthread_mutex_lock(m_mutex);
		switch (flag) {
		case 0:
			m_mgr->SetStatus(TRYINST_ADD_ACCUM2, n);
			break;
		case 1:
			m_mgr->SetStatus(TRYINST_ADD_DONE, n);
			break;
		}
		pthread_mutex_unlock(m_mutex);
	}
	fs->Close(fd);
	return total;

error:
	fs->Close(fd);
	return n;
}

/* Copies `size` bytes from the package source (m_srcFs/m_srcFd) to fs:path,
   opened O_WRONLY|O_CREAT|O_TRUNC 0666, in chunks of at most 8192 bytes.
   Returns -27611/-27613 on a short read/write (closing the file) and -27612
   if the open fails. */
int TRYInstaller::createJacketFile(TRYAbstFS *fs, char *path, unsigned long long size)
{
	char buf[8192];
	int ret;

	int fd = fs->Open(path, 0x602, 438);
	if (fd < 0)
		return -27612;

	while (size != 0) {
		int chunk = (size < 8192) ? size : 8192;
		ret = m_srcFs->Read(m_srcFd, buf, chunk);
		if (ret != chunk) {
			ret = -27611;
			goto error;
		}
		ret = fs->Write(fd, buf, ret);
		if (ret != chunk) {
			ret = -27613;
			goto error;
		}
		size -= ret;
	}
	fs->Close(fd);
	return 0;

error:
	fs->Close(fd);
	return ret;
}

/* Sets uid = gid = 0xffff (statmask 0x40) through Getstat/Chstat. */
int TRYInstaller::changeUID_GID(TRYAbstFS *fs, char *path)
{
	TRY_stat st;
	fs->Getstat(path, &st);
	st.uid = 0xffff;
	st.gid = 0xffff;
	return fs->Chstat(path, &st, 0x40);
}
