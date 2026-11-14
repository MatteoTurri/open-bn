/*
 * TRYInstManager methods: the content.dat checks, the install definition read
 * from the DNAS archive, the partition and command queues, and the install
 * thread with its status.
 */

#include "tryinstmanager.h"
#include "tryinstaller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "trycreatefs.h"

TRYInstManager::TRYInstManager()
{
	m_contentsFs = 0;
	m_fs = 0;
	m_contentsMounted = 0;
	m_pkgOnHdd = 0;
	memset(unk_10, 0, 1024);
	m_fd = -1;
	m_hasDatapath = 0;
	pthread_cond_init(&m_cond, 0);
	pthread_mutex_init(&m_mutex, 0);
	memset(&m_status, 0, sizeof(m_status));
	m_threadRunning = 0;
	m_defEntryCount = 0;
	m_partitionSeq = 0;
	memset(&m_licMessage, 0, sizeof(m_licMessage));
	m_mkPartCount = 0;
	m_mkPartHead = 0;
	m_mkPartTail = 0;
	m_thirdListCount = 0;
	m_thirdListHead = 0;
	m_thirdListTail = 0;
	m_cmdCount = 0;
	m_cmdHead = 0;
	m_cmdTail = 0;
}

TRYInstManager::~TRYInstManager()
{}

/* Opens dlcontents/dnas_no_key/content.dat on m_fs and reads its 4264-byte
   header into m_contHeader; the version sets m_state (TRYINST_CONTENT_*). */
int TRYInstManager::PreInstCheck()
{
	char path[1024];
	sprintf(path, "%s/%s/%s", "dlcontents", "dnas_no_key", "content.dat");
	int fd = m_fs->Open(path, 1, 0600);
	m_fd = fd;
	if (fd < 0)
		return -27702;
	int r = m_fs->Read(fd, &m_contHeader, 4264);
	char *ver = m_contHeader.version;
	if (r != 4264) {
		m_fs->Close(m_fd);
		return -27703;
	}
	int state;
	if (strncmp("1.00", ver, 4) == 0) {
		state = TRYINST_CONTENT_V100;
	} else {
		if (strncmp("1.01", ver, 4) != 0)
			return -27712;
		state = TRYINST_CONTENT_V101;
	}
	m_state = state;
	return 0;
}

int TRYInstManager::MountContentsPat()
{
	if (m_contentsMounted != 0)
		return 0;
	TRYAbstFS *fs = TRYCreateFS(1);
	m_contentsFs = fs;
	int r = fs->Mount(0, 0, 0, "hdd0", "__contents");
	if (r < 0)
		return -1;
	m_contentsMounted = 1;
	return 0;
}

int TRYInstManager::SetInstPkgFS_HDD()
{
	if (m_pkgOnHdd != 0)
		return 0;
	m_pkgOnHdd = 1;
	m_fs = m_contentsFs;
	return 0;
}

/* Closes and removes content.dat, unmounts m_fs and deletes m_contentsFs, then frees
   the partition queue (unmounting and deleting each file system), the third queue
   and the command queue. */
void TRYInstManager::FreeResource()
{
	if (m_fs) {
		m_fs->Close(m_fd);
		char path[1024];
		sprintf(path, "%s/%s/%s", "dlcontents", "dnas_no_key", "content.dat");
		m_fs->Remove(path);
		m_fs->UMount();
		if (m_contentsFs) {
			delete m_contentsFs;
		}
	}
	instMkPartitionList *node = m_mkPartHead;
	while (node) {
		instMkPartitionList *next = node->next;
		if (node->fs) {
			node->fs->UMount();
		}
		free(node->data);
		if (node->fs) {
			delete node->fs;
		}
		free(node);
		node = next;
	}
	ThirdNode *t = m_thirdListHead;
	while (t) {
		ThirdNode *tn = t->next;
		free(t->data);
		free(t);
		t = tn;
	}
	CmdNode *c = m_cmdHead;
	while (c) {
		CmdNode *cn = c->next;
		free(c->data);
		free(c);
		c = cn;
	}
}

/* Seeks to the archive offset of the header and opens the DNAS archive there; with content.dat 1.00
   the 64-bit value it returns becomes m_status.bytesTotal. */
int TRYInstManager::Initdnas_ar()
{
	long long result;
	m_fs->Seek(m_fd, m_contHeader.archiveOffset, 0);
	int r = _sce_dnas_ar_nokey_init(m_fd, 0, 0, 0, 0, 0, 0, 0, &result);
	if (r < 0)
		return -1;
	if (m_state == TRYINST_CONTENT_V100)
		m_status.bytesTotal = result;
	return 0;
}

/*
 * Reads the "TRYInstDef" member of the DNAS archive and turns its entries
 * into commands. A leading "dnas.dat" member is extracted by loadDNASdat
 * first. The member size and the dnas.dat size are taken off
 * m_status.bytesTotal; type 100 entries (license message) are not counted.
 */
int TRYInstManager::LoadInstDef()
{
	char name[1024];
	dnas_archive_format fmt;
	instDefHeader hdr;
	instDefEntry ent;
	int skipped = 0;

	if (_sce_dnas_ar_nokey_read_format(name, &fmt) != 0)
		goto fail;
	if (strncmp("dnas.dat", name + 3, 8) == 0) {
		if (loadDNASdat(&fmt) < 0)
			goto fail;
		skipped = fmt.size;
		if (_sce_dnas_ar_nokey_read_format(name, &fmt) != 0)
			goto fail;
	}
	if (_sce_dnas_ar_nokey_read_body(&hdr, 20, 0) != 20)
		goto fail;
	m_status.bytesTotal -= fmt.size + skipped;
	if (strncmp(hdr.magic, "TRYInstDef", 10) != 0)
		goto fail;
	if (strncmp(hdr.ver, "1.00", 4) != 0)
		goto fail;
	m_defEntryCount = hdr.count;
	m_status.totalCount = hdr.count;
	for (int i = 0; i < m_defEntryCount; i++) {
		if (_sce_dnas_ar_nokey_read_body(&ent, 8, 0) != 8)
			goto fail;
		void *buf = malloc(ent.size);
		int r = _sce_dnas_ar_nokey_read_body(buf, ent.size, 0);
		if (r != ent.size)
			goto fail;
		if (ent.type == TRYINST_ENTRY_LICMESSAGE)
			i--;  /* NOTE: bug: the payload of a skipped entry is never freed */
		else
			createAndAddCommandList(ent.type, buf, r);
	}
	pthread_mutex_lock(&m_mutex);
	SetStatus(TRYINST_SET_STATE, TRYINST_STATE_READY);
	pthread_mutex_unlock(&m_mutex);
	return 0;
fail:
	return -1;
}

/* With content.dat 1.01, reads the last 512 bytes (right before the jacket offset of the header) into
   m_instInfo and takes m_status.bytesTotal and trailerTotal from them. */
int TRYInstManager::LoadInstInfo()
{
	if (m_state == TRYINST_CONTENT_V100)
		goto success;
	if (m_state != TRYINST_CONTENT_V101)
		return 0;
	{
		unsigned int off = m_contHeader.jacketOffset - 512;
		m_fs->Seek(m_fd, off, 0);
		int r = m_fs->Read(m_fd, m_instInfo, 512);
		if (r != 512) {
			m_fs->Close(m_fd);
			return -1;
		}
		m_status.bytesTotal = *(long long *)(m_instInfo + 16);
		m_status.trailerTotal = *(long long *)(m_instInfo + 24);
	}
success:
	return 0;
}

/*
 * The whole installation, run by the install thread: removes and creates the
 * partitions, builds a TRYInstaller and passes it the queued commands one by
 * one, counting them in m_status. The state ends at TRYINST_STATE_DONE (result
 * 2) or TRYINST_STATE_FAILED (result: the error code).
 */
int TRYInstManager::StartInstall()
{
	CmdNode *cmd = m_cmdHead;
	TRYInstaller *inst = 0;
	int r;

	pthread_mutex_lock(&m_mutex);
	SetStatus(TRYINST_SET_STATE, TRYINST_STATE_INSTALLING);
	SetStatus(TRYINST_SET_RESULT, 1);
	pthread_mutex_unlock(&m_mutex);

	r = removePartition();
	if (r < 0)
		goto fail;
	r = makePartition();
	if (r < 0)
		goto fail;

	inst = new TRYInstaller(this, &m_cond, &m_mutex, unk_410, m_fs, m_fd, m_contentsFs,
	    m_contHeader.hasJacket ? m_contHeader.jacketOffset : 0, m_contHeader.archiveOffset,
	    &m_contHeader.contentInfo, &m_contHeader.gameContentInfo, m_mkPartHead);
	while (cmd) {
		r = inst->Install(cmd->type, cmd->data);
		if (r < 0)
			goto fail;
		pthread_mutex_lock(&m_mutex);
		m_partitionSeq++;
		m_status.currentCount = m_partitionSeq;
		pthread_mutex_unlock(&m_mutex);
		cmd = cmd->next;
	}
	r = inst->WritePatInfo();
	if (r < 0)
		goto fail;
	delete inst;
	pthread_mutex_lock(&m_mutex);
	SetStatus(TRYINST_SET_STATE, TRYINST_STATE_DONE);
	SetStatus(TRYINST_SET_RESULT, 2);
	pthread_mutex_unlock(&m_mutex);
	goto done;
fail:
	inst->RemoveAllPat();  /* NOTE: bug: inst is still 0 when removePartition or makePartition fail */
	delete inst;
	pthread_mutex_lock(&m_mutex);
	SetStatus(TRYINST_SET_STATE, TRYINST_STATE_FAILED);
	SetStatus(TRYINST_SET_RESULT, r);
	pthread_mutex_unlock(&m_mutex);
done:;
}

int TRYInstManager::GetLicMessage(sceTRYInstLicMessage *out)
{
	*out = m_licMessage;
}

int TRYInstManager::GetStatus(sceTRYInstStatus *out)
{
	pthread_mutex_lock(&m_mutex);
	*out = m_status;
	pthread_mutex_unlock(&m_mutex);
}

/* Sets or adds to a field of m_status (TRYINST_SET_*, TRYINST_ADD_*). */
void TRYInstManager::SetStatus(int cmd, int value)
{
	switch (cmd) {
	case TRYINST_SET_STATE:
		m_status.state = value;
		break;
	case TRYINST_SET_RESULT:
		m_status.result = value;
		break;
	case TRYINST_SET_CURRENT_COUNT:
		m_status.currentCount = value;
		break;
	case TRYINST_ADD_ACCUM2:
		m_status.accum2 += value;
		break;
	case TRYINST_ADD_BYTES_TOTAL:
		if (m_state == TRYINST_CONTENT_V100)
			m_status.bytesTotal += value;
		break;
	case TRYINST_ADD_DONE:
		switch (m_state) {
		case TRYINST_CONTENT_V100:
			m_status.accum2 += value;
			break;
		case TRYINST_CONTENT_V101:
			m_status.trailer += value;
			break;
		}
		break;
	case TRYINST_SET_UNK10:
		m_status.unk_10 = value;
		break;
	}
}

int TRYInstManager::GetDNASDatapath(char *outBuf)
{
	int ret;
	switch (m_status.state) {
	case TRYINST_STATE_DONE:
		if (m_hasDatapath != 0) {
			strcpy(outBuf, m_datapath);
			ret = 0;
		} else {
			outBuf[0] = 0;
			ret = -27951;
		}
		break;
	case TRYINST_STATE_FAILED:
		outBuf[0] = 0;
		ret = m_hasDatapath ? -27952 : -27951;
		break;
	default:
		ret = -27954;
		break;
	}
	return ret;
}

int TRYInstManager::StartInstThread()
{
	if (m_threadRunning)
		return 0;
	pthread_attr_t attr;
	if (pthread_attr_init(&attr) != 0)
		return -27801;
	unsigned int stackSize;
	if (pthread_attr_getstacksize(&attr, &stackSize) != 0)
		return -27802;
	if (stackSize <= 0x1ffff && pthread_attr_setstacksize(&attr, 0x20000) != 0)
		return -27803;
	if (pthread_create(&m_threadId, &attr, (void *(*)(void *))_TRYInstallThreadEntry, this) != 0)
		return -27804;
	m_threadRunning = 1;
	pthread_mutex_lock(&m_mutex);
	SetStatus(TRYINST_SET_STATE, TRYINST_STATE_READY);
	SetStatus(TRYINST_SET_RESULT, 0);
	pthread_mutex_unlock(&m_mutex);
	return 0;
}

int TRYInstManager::StopInstThread()
{
	if (m_threadRunning) {
		pthread_cancel(m_threadId);
	}
	pthread_cond_destroy(&m_cond);
	pthread_mutex_destroy(&m_mutex);
	return 0;
}

/* Thread entry started by StartInstThread. */
extern "C" void *_TRYInstallThreadEntry(void *arg)
{
	((TRYInstManager *)arg)->StartInstall();
	return 0;
}

/* License message: copied to m_licMessage and freed; partition: queued with
   addMkPartitionList; anything else: an install command. */
void TRYInstManager::createAndAddCommandList(int type, void *data, int size)
{
	switch (type) {
	case TRYINST_ENTRY_LICMESSAGE:
		memcpy(m_licMessage.data, data, size);
		free(data);
		break;
	case TRYINST_ENTRY_MKPARTITION:
		addMkPartitionList(1, data);
		break;
	default:
		addInstCommand(type, data);
		break;
	}
}

/* Appends a zeroed 72-byte partition node to the queue. */
void TRYInstManager::addMkPartitionList(int type, void *data)
{
	instMkPartitionList *node = (instMkPartitionList *)malloc(72);
	memset(node, 0, 72);
	node->type = type;
	node->data = (instMkPartData *)data;
	node->next = 0;
	if (m_mkPartHead == 0) {
		m_mkPartHead = node;
		m_mkPartTail = node;
	} else {
		m_mkPartTail->next = node;
		m_mkPartTail = node;
	}
	m_mkPartCount++;
}

/* Appends a {type, data} node to the command queue (m_cmdHead stays on the first). */
void TRYInstManager::addInstCommand(int type, void *data)
{
	CmdNode *node = (CmdNode *)malloc(12);
	node->type = type;
	node->data = data;
	node->next = 0;
	if (m_cmdHead == 0) {
		m_cmdHead = node;
		m_cmdTail = node;
	} else {
		m_cmdTail->next = node;
		m_cmdTail = node;
	}
	m_cmdCount++;
}

/* Creates and mounts the partition of every type 1 node of the queue. */
int TRYInstManager::makePartition()
{
	instMkPartitionList *node = m_mkPartHead;
	while (node) {
		if (node->type == 1) {
			int r = createAndMountPFS(node);
			if (r < 0)
				return r;
		}
		pthread_mutex_lock(&m_mutex);
		m_partitionSeq++;
		m_status.currentCount = m_partitionSeq;
		pthread_mutex_unlock(&m_mutex);
		node = node->next;
	}
	return 0;
}

/* Creates the file system of the partition and runs MkMainPart, AddSubPart,
   Format (when MkMainPart returned 0) and Mount; on success copies the mount
   point and the name into the node, on failure removes the partition. */
int TRYInstManager::createAndMountPFS(instMkPartitionList *p)
{
	instMkPartData *d = p->data;
	TRYAbstFS *fs = TRYCreateFS(1);
	int r;
	p->fs = fs;
	r = fs->MkMainPart(d->unit, d->size, d->name1, d->name2, 0, d->name0);
	if (r < 0)
		goto fail;
	{
		int mainRet = r;
		r = fs->AddSubPart(d->subUnit, d->subPart, d->size, d->name1, d->name2, 0, d->name0);
		if (r < 0)
			goto fail;
		if (mainRet == 0) {
			r = fs->Format(d->fmtUnit, d->size, 0, d->name0);
			if (r < 0)
				goto fail;
		}
	}
	r = fs->Mount(0, d->name1, 0, "hdd0", d->name0);
	if (r < 0)
		goto fail;
	strcpy(p->mountPoint, fs->GetMountPoint());
	strcpy(p->name, d->name0);
	return 0;
fail:
	if (fs)
		fs->RmPart(0, d->name0, d->name1);
	return r;
}

int TRYInstManager::loadDNASdat(dnas_archive_format *fmt)
{
	FILE *fp = 0;
	void *buf = malloc(fmt->size);
	if (buf == 0)
		goto fail;
	{
		int r = _sce_dnas_ar_nokey_read_body(buf, fmt->size, 0);
		if (r == (int)fmt->size) {
			struct timeval tv;
			gettimeofday(&tv, 0);
			sprintf(m_datapath, "/tmp/%08x", ((unsigned char)tv.tv_sec << 24) | (tv.tv_usec << 4));
			fp = fopen(m_datapath, "w+");
			if (fp) {
				unsigned int w = fwrite(buf, 1, fmt->size, fp);
				if ((int)w == (int)fmt->size) {
					free(buf);
					fclose(fp);
					m_hasDatapath = 1;
					return 0;
				}
			}
		}
	}
fail:
	if (buf)
		free(buf);
	if (fp)
		fclose(fp);
	return -1;
}

/* Nothing to remove: always succeeds. */
int TRYInstManager::removePartition()
{
	return 0;
}
