/*
 * DNAS2SysupManager methods: loading of the update definition from the
 * contents partition and the worker thread that runs it.
 */

#include "dnas2sysupmanager.h"

#include "dnas2sysupdater.h"

DNAS2SysupManager::DNAS2SysupManager()
{
	pthread_mutex_init(&m_mutex, 0);
	memset(&m_status, 0, sizeof(m_status));
	m_status.state = 1;
	m_cmdHead = 0;
	m_cmdTail = 0;
}

DNAS2SysupManager::~DNAS2SysupManager()
{
	sysupCommandList *node = m_cmdHead;
	while (node) {
		sysupCommandList *next = node->next;
		free(node->data);
		free(node);
		node = next;
	}
}

int DNAS2SysupManager::MountContentsPart()
{
	memcpy(m_pfsName, "pfs0:", 6);
	int r = -1;
	for (unsigned int i = 0; i < 10; i++) {
		r = scepfsMount(m_pfsName, "hdd0:__contents", 2, 0, 0);
		if (r >= 0)
			break;
		m_pfsName[3]++;
	}
	if (r >= 0)
		return 0;
	return -1003;
}

int DNAS2SysupManager::UnMountContentsPart()
{
	if (scepfsUmount(m_pfsName) >= 0)
		return 0;
	return -1003;
}

// content.dat: a SysupDefHeader followed by count records, each a
// SysupDefRecord (command type, data size) and size bytes of data.
struct SysupDefHeader {
	/* 0x00 */ char magic[16];        // "SceBNSysupDef"
	/* 0x10 */ char version[16];      // "1.0"
	/* 0x20 */ unsigned int count;    // number of records
	/* 0x24 */ unsigned int unk_24;
	/* 0x28 */ unsigned int count_hi;
	/* 0x2C */ unsigned int unk_2C;
}; /* size = 0x30 */

struct SysupDefRecord {
	/* 0x00 */ unsigned int type;
	/* 0x04 */ unsigned int size;
}; /* size = 0x8 */

int DNAS2SysupManager::LoadSysupDef()
{
	strcpy(m_defPath, m_pfsName);
	strcat(m_defPath, "/dlcontents/BN/content.dat");
	int fd = scepfsOpen(m_defPath, 1);
	int r;
	if (fd < 0) {
		r = -1004;
	} else {
		SysupDefHeader hdr;
		if (scepfsRead(fd, &hdr, sizeof(hdr)) != (int)sizeof(hdr)) {
			r = -1004;
		} else if (strncmp(hdr.magic, "SceBNSysupDef", 13) != 0) {
			r = -1005;
		} else if (strncmp(hdr.version, "1.0", 3) != 0) {
			r = -1005;
		} else {
			unsigned int i = 0;
			m_status.step = 0;
			m_status.total_steps = hdr.count_hi << 1;
			for (; i < hdr.count; i++) {
				SysupDefRecord rec;
				if (scepfsRead(fd, &rec, sizeof(rec)) != (int)sizeof(rec)) {
					r = -1004;
					goto fail;
				}
				if (rec.type < 1000 || rec.type > 4000) {
					r = -1005;
					goto fail;
				}
				void *data;
				if (rec.size != 0) {
					data = malloc(rec.size);
					if (!data) {
						r = -1001;
						goto fail;
					}
					if (rec.size != (unsigned int)scepfsRead(fd, data, rec.size)) {
						r = -1004;
						goto fail;
					}
				} else {
					data = 0;
				}
				addSysupCommand(rec.type, data);
			}
			scepfsClose(fd);
			return 0;
		}
	}
fail:
	if (fd >= 0)
		scepfsClose(fd);
	return r;
}

int DNAS2SysupManager::GetStatus(sceBNSysupStatus *out)
{
	int r = pthread_mutex_trylock(&m_mutex);
	if (r == 0) {
		*out = m_status;
		r = pthread_mutex_unlock(&m_mutex);
	}
	return r;
}

void DNAS2SysupManager::SetStatus(unsigned int type, unsigned int value, char *unused1, unsigned long long unused2)
{
	pthread_mutex_lock(&m_mutex);
	switch (type) {
	case 1:
		m_status.state = value;
		break;
	case 2:
		m_status.result = value;
		break;
	case 3:
		m_status.step = value;
		break;
	}
	pthread_mutex_unlock(&m_mutex);
}

void DNAS2SysupManager::SysupdateStateControl()
{
	int r = startSysupdate();
	if (r == 0) {
		pthread_mutex_lock(&m_mutex);
		m_status.state = 3;
		m_status.result = 2;
		pthread_mutex_unlock(&m_mutex);
	} else if (r == 1) {
		pthread_mutex_lock(&m_mutex);
		m_status.state = 3;
		m_status.result = 3;
		pthread_mutex_unlock(&m_mutex);
	} else {
		pthread_mutex_lock(&m_mutex);
		m_status.state = 4;
		m_status.result = r;
		pthread_mutex_unlock(&m_mutex);
	}
}

int DNAS2SysupManager::StartSysupThread()
{
	m_status.state = 2;
	m_status.result = 1;
	if (pthread_create(&m_threadId, 0, _DNASSysupdateThreadEntry, this) < 0)
		return -1001;
	return 0;
}

void *_DNASSysupdateThreadEntry(void *arg)
{
	((DNAS2SysupManager *)arg)->SysupdateStateControl();
	return 0;
}

// startSysupdate: run the command list. Types 1001-1004 are the RPM, kernel,
// MBR and PFS steps, 1005 a marker. Returns the first error, else 1 if a
// marker was seen, else 0.
int DNAS2SysupManager::startSysupdate()
{
	bool sawNoop = false;
	sysupCommandList *node = m_cmdHead;
	DNAS2Sysupdater *updater = new DNAS2Sysupdater(this);
	int r;
	for (; node; node = node->next) {
		switch (node->tag) {
		case 1001:
			r = updater->SysupRPM(node);
			if (r < 0)
				goto fail;
			break;
		case 1002:
			r = updater->SysupKernel(node);
			if (r < 0)
				goto fail;
			break;
		case 1003:
			r = updater->SysupMBR(node);
			if (r < 0)
				goto fail;
			break;
		case 1004:
			r = updater->SysupPFS(node);
			if (r < 0)
				goto fail;
			break;
		case 1005:
			sawNoop = true;
			break;
		}
	}
	delete updater;
	return (int)sawNoop;
fail:
	delete updater;
	return r;
}

void DNAS2SysupManager::addSysupCommand(unsigned int type, void *data)
{
	sysupCommandList *node = (sysupCommandList *)malloc(sizeof(sysupCommandList));
	node->tag = type;
	node->data = data;
	node->next = 0;
	if (m_cmdHead == 0)
		m_cmdHead = node;
	else
		m_cmdTail->next = node;
	m_cmdTail = node;
}

void DNAS2SysupManager::CountUpStep()
{
	pthread_mutex_lock(&m_mutex);
	m_status.step++;
	pthread_mutex_unlock(&m_mutex);
}
