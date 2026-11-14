/*
 * thMediaChecker: the media check thread. Polls the CDVD drive type and the USB
 * device list (/proc/bus/usb/devices), loads the USB modules and mounts or
 * unmounts USB mass storage, publishing the state in Variables.
 */

#define THMEDIACHECKER_IMPLEMENTATION
#include "thmediachecker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include "app/variables.h"
#include <sys/stat.h>
#include <scsi/sg.h>

/* Buffers of the CDVD type ioctl and of the SCSI commands: an sg header (36
 * bytes) followed by the 96 bytes of the INQUIRY reply. */
static int g_cdvdIoctlBuf;
static unsigned char g_modesenseOutBuf[36 + 96];

thMediaChecker *pMediaChecker;  /* the singleton instance */

int thMediaChecker::fd = 0;
int thMediaChecker::usb_status = 0;
int thMediaChecker::ms_status = 0;
int thMediaChecker::mount_dev_no = -1;
int thMediaChecker::external_unmount = 0;
int thMediaChecker::PORT = 0;
int thMediaChecker::mount_port = 0;

pthread_mutex_t thMediaChecker::skipusb_flag_mutex;
pthread_mutex_t thMediaChecker::skipcdvd_flag_mutex;
pthread_mutex_t thMediaChecker::cdvd_type_mutex;
int thMediaChecker::skipusb_flag;
int thMediaChecker::skipcdvd_flag;
int thMediaChecker::cdvd_type;
int thMediaChecker::illegal_flag;

/* SCSI command block shared by TestForMedium/Inquiry/Modesense, a byte image
 * of a struct sg_header (<scsi/sg.h>) followed by the CDB: the additional sense
 * code and qualifier (sense_buffer[12], [13]) at +32/+33, the CDB at +36. */
struct Cdb6 {
	unsigned char b[6];
};
struct Cdb10 {
	unsigned char b[10];
};
struct ScsiCmdBlock {
	/* 0x00 */ char pad_00[32];
	/* 0x20 */ unsigned char asc;
	/* 0x21 */ unsigned char ascq;
	/* 0x22 */ char pad_22[2];
	union {
		/* 0x24 */ Cdb6 cdb;  /* 6-byte CDB */
		/* 0x24 */ Cdb10 cdb10;  /* 10-byte CDB */
	};
	/* 0x2E */ char pad_2E[8];
};  /* size = 0x36 */
ScsiCmdBlock cmd;

thMediaChecker::~thMediaChecker()
{
	pMediaChecker = 0;
}

void thMediaChecker::init()
{
	m_discEventPending = 0;
	cdvd_type = 0;
	illegal_flag = 0;
	pMediaChecker = this;
	m_usbUserData = 0;
	m_discUserData = 0;
	m_usbCallback = 0;
	skipusb_flag = 1;
	m_usbChanged = 0;
	m_usbStorageSeen = 0;
	skipcdvd_flag = 0;
}

void thMediaChecker::setUSBCallback(void *userData, MediaCallback cb)
{
	m_usbUserData = userData;
	m_usbCallback = cb;
}

void thMediaChecker::setDISCCallback(void *userData, MediaCallback cb)
{
	m_discUserData = userData;
	m_discCallback = cb;
}

int thMediaChecker::getCDVDType()
{
	pthread_mutex_lock(&cdvd_type_mutex);
	int result = illegal_flag ? 255 : cdvd_type;
	pthread_mutex_unlock(&cdvd_type_mutex);
	return result;
}

/* Disables the USB media check and saves the setting (BB_USB_MEDIACHECK =
 * "DISABLE"); if USB storage is mounted, unmounts it. */
int thMediaChecker::skipusb()
{
	string name("BB_USB_MEDIACHECK");

	pthread_mutex_lock(&skipusb_flag_mutex);
	skipusb_flag = 1;
	Variables::SetVariable(name, "DISABLE");
	pthread_mutex_unlock(&skipusb_flag_mutex);

	if (usb_status == 1) {
		external_unmount = 1;
		m_usbChanged = 0;
		mounter();
		m_pendingEvent = 1;
		waitEventProc();
	}
	return 0;
}

int thMediaChecker::restartusb()
{
	pthread_mutex_lock(&skipusb_flag_mutex);
	skipusb_flag = 0;
	pthread_mutex_unlock(&skipusb_flag_mutex);
	return 0;
}

int thMediaChecker::getSkipUSBFlag()
{
	pthread_mutex_lock(&skipusb_flag_mutex);
	int result = skipusb_flag;
	pthread_mutex_unlock(&skipusb_flag_mutex);
	return result;
}

int thMediaChecker::skipcdvd()
{
	pthread_mutex_lock(&skipcdvd_flag_mutex);
	skipcdvd_flag = 1;
	pthread_mutex_unlock(&skipcdvd_flag_mutex);
	return 0;
}

int thMediaChecker::restartcdvd()
{
	pthread_mutex_lock(&skipcdvd_flag_mutex);
	skipcdvd_flag = 0;
	pthread_mutex_unlock(&skipcdvd_flag_mutex);
	return 0;
}

int thMediaChecker::getSkipCDVDFlag()
{
	pthread_mutex_lock(&skipcdvd_flag_mutex);
	int result = skipcdvd_flag;
	pthread_mutex_unlock(&skipcdvd_flag_mutex);
	return result;
}

int thMediaChecker::threadProc()
{
	if (m_abortFlag == 0) {
		MediaCheck();
		return 0;
	}
	return 0;
}

int thMediaChecker::threadFinished()
{
	m_pendingEvent = 0;
	return 1;
}

/* Rereads the persistent BB_USB_MEDIACHECK variable and updates the skip state. */
int thMediaChecker::controlUSBCheck()
{
	string name("BB_USB_MEDIACHECK");
	list<string> values;
	list<string>::iterator it;
	char buf[128];

	int ret = Variables::GetVariable(name, values);
	it = values.begin();
	if (ret != 0) {
		sprintf(buf, "%s", (*it).c_str());
		if (strncmp(buf, "ENABLE", 6) == 0) {
			if (skipusb_flag == 1)
				restartusb();
		} else if (strncmp(buf, "DISABLE", 7) == 0) {
			if (skipusb_flag == 0) {
				skipusb();
				m_usbChanged = 0;
			}
		}
	}
	return 0;
}

/* Publishes the media state in Variables: BB_CDVD_MEDIA (disc type) when
 * m_discEventPending is set, BB_USB_MEDIA / _PRODUCT / _MANUFAC and BB_MS_STATUS
 * when m_usbChanged is set. */
int thMediaChecker::threadEvent()
{
	if (m_abortFlag)
		return 1;

	if (m_discEventPending != 0) {
		string name("BB_CDVD_MEDIA");
		string value;
		pthread_mutex_lock(&cdvd_type_mutex);
		switch (cdvd_type) {
		default:
		case 0:
			value = "NoDisc";
			break;
		case 1:
		case 2:
		case 3:
		case 4:
			value = "Checking";
			break;
		case 5:
			value = "Unknown";
			break;
		case 16:
			value = "PSCD";
			break;
		case 17:
			value = "PSCDAudio";
			break;
		case 18:
			value = "PS2CD";
			break;
		case 19:
			value = "PS2CDAudio";
			break;
		case 20:
			value = "PS2DVD";
			break;
		case 111:
			value = "SAWCD";
			break;
		case 112:
			value = "SAWCDAudio";
			break;
		case 113:
			value = "SAWOCDAudio";
			break;
		case 253:
			value = "CDDA";
			break;
		case 254:
			value = "DVDVideo";
			break;
		case 255:
			value = "Illegal";
			break;
		}
		pthread_mutex_unlock(&cdvd_type_mutex);
		Variables::SetVariable(name, value);
		m_discEventPending = 0;

		list<string> values;
		list<string>::iterator vit;
		for (vit = values.begin(); vit != values.end(); vit++)
			;
		if (m_discUserData != 0 && m_discCallback != 0)
			m_discCallback(m_discUserData);
	}

	if (m_usbChanged != 0) {
		list<bb_usb_entry *>::iterator it;

		for (it = m_usbSrcList.begin(); it != m_usbSrcList.end(); it++)
			;
		for (it = m_usbAddedList.begin(); it != m_usbAddedList.end(); it++)
			;
		for (it = m_usbDestList.begin(); it != m_usbDestList.end(); it++)
			;

		string name;
		string value;

		name = "BB_USB_MEDIA";
		value = "NONE";
		Variables::SetVariable(name, value);

		if (usb_status == 1) {
			for (it = m_usbAddedList.begin(); it != m_usbAddedList.end(); it++) {
				if ((*it)->cls != 8)
					continue;
				name = "BB_USB_MEDIA";
				value = "STORAGE";
				Variables::SetVariable(name, value);
				name = "BB_USB_MEDIA_PRODUCT";
				value = (*it)->product;
				Variables::SetVariable(name, value);
				name = "BB_USB_MEDIA_MANUFAC";
				value = (*it)->manufacturer;
				Variables::SetVariable(name, value);
			}
			if (ms_status == 3) {
				name = "BB_MS_STATUS";
				value = "BB_MS_LOCKED";
				Variables::SetVariable(name, value);
			} else if (ms_status == 5) {
				name = "BB_MS_STATUS";
				value = "BB_MS_BUSY";
				Variables::SetVariable(name, value);
			} else {
				name = "BB_MS_STATUS";
				value = "BB_MS_READY";
				Variables::SetVariable(name, value);
			}
		}

		if (usb_status == 0) {
			for (it = m_usbDestList.begin(); it != m_usbDestList.end(); it++)
				delete *it;

			name = "BB_USB_MEDIA";
			value = "NONE";
			Variables::SetVariable(name, value);

			if (ms_status == 3) {
				name = "BB_MS_STATUS";
				value = "BB_MS_LOCKED";
				Variables::SetVariable(name, value);
			} else if (ms_status != 1) {
				name = "BB_MS_STATUS";
				value = "BB_MS_CANT_OPEN";
				Variables::SetVariable(name, value);
			}

			m_usbSrcList.clear();
			m_usbAddedList.clear();
			m_usbDestList.clear();

			if (m_usbUserData != 0 && m_usbCallback != 0)
				m_discCallback(m_usbUserData);
		}
	}
	return 1;
}

/* SCSI device nodes. */
static const char kDevSd[16][10] = {
	"/dev/sda",
	"/dev/sdb",
	"/dev/sdc",
	"/dev/sdd",
	"/dev/sde",
	"/dev/sdf",
	"/dev/sdg",
	"/dev/sdh",
	"/dev/sdi",
	"/dev/sdj",
	"/dev/sdk",
	"/dev/sdl",
	"/dev/sdm",
	"/dev/sdn",
	"/dev/sdo",
	"/dev/sdp",
};
static const char kDevSg[16][10] = {
	"/dev/sga",
	"/dev/sgb",
	"/dev/sgc",
	"/dev/sgd",
	"/dev/sge",
	"/dev/sgf",
	"/dev/sgg",
	"/dev/sgh",
	"/dev/sgi",
	"/dev/sgj",
	"/dev/sgk",
	"/dev/sgl",
	"/dev/sgm",
	"/dev/sgn",
	"/dev/sgo",
	"/dev/sgp",
};

/* Main loop of the thread: check the CDVD every 500 ms, and the USB devices
 * every 4 rounds. */
void thMediaChecker::MediaCheck()
{
	char path[16];
	int n;

	for (;;) {
		n = 0;
		for (;;) {
			usleep(500000);
			if (m_abortFlag != 0)
				return;
			if (getSkipCDVDFlag() == 0)
				MediaCheckCDVD();
			if (m_abortFlag != 0)
				return;
			n++;
			if (m_discEventPending != 0)
				goto do_mount;
			if (n < 4)
				continue;
			controlUSBCheck();
			if (getSkipUSBFlag() == 0)
				break;
			n = 0;
		}
		MediaCheckUSBStorage();
		if (m_usbChanged != 0)
			goto tail;
		if (m_usbStorageSeen == 0)
			goto tail;

		if (usb_status != 1)
			goto not_mounted;

		if (fd != 0)
			fd = 0;
		errno = 0;
		sprintf(path, "%s", kDevSg[mount_port]);
		fd = open(path, O_RDWR);
		if (fd >= 0)
			goto check_media;
		if (errno == EROFS)
			goto check_media;
		external_unmount = 1;
		goto tail;

check_media:
		ms_status = MS_Check_Media();
		if (ms_status != 2)
			goto do_close;
		if (mount_port != PORT)
			goto do_close;
		external_unmount = 1;
		goto tail;

do_close:
		close(fd);
		goto tail;

not_mounted:
		if (usb_status != 0)
			goto tail;
		ms_status = MS_Check_Device(-1);
		if (ms_status != 0)
			goto tail;
		close(fd);
		m_usbChanged = 1;
		m_usbSrcList.clear();

tail:
		if (m_abortFlag != 0)
			return;
		if (m_discEventPending != 0)
			goto do_mount;
		if (m_usbChanged != 0)
			goto do_mount;
		if (external_unmount == 0)
			goto wait;
do_mount:
		if (mounter() != 0)
			m_pendingEvent = 1;
		external_unmount = 0;
wait:
		if (waitEventProc() == 0)
			return;
	}
}

/* Reads the CDVD disc type from /dev/ps2cdvdstat into cdvd_type. */
int thMediaChecker::MediaCheckCDVD()
{
	int fd = open("/dev/ps2cdvdstat", O_RDONLY);
	if (fd < 0)
		return fd;

	int ret = ioctl(fd, 0x40045602, &g_cdvdIoctlBuf);
	if (ret < 0)
		return close(fd);
	close(fd);

	pthread_mutex_lock(&cdvd_type_mutex);
	int type = g_cdvdIoctlBuf;
	if (type == 255)
		illegal_flag = 1;

	if (type != cdvd_type) {
		m_discEventPending = 0;
		cdvd_type = type;
	}
	return pthread_mutex_unlock(&cdvd_type_mutex);
}

/* Parses /proc/bus/usb/devices: 'T' (topology) starts a new entry, 'P'
 * Vendor/ProdID, 'S' Product/Manufacturer/SerialNumber, 'I' Cls/Sub/Prot/Driver,
 * 'E' (endpoint, last line of the block) adds it with usb_add_entry. */
void thMediaChecker::MediaCheckUSBStorage()
{
	char buf[256];
	list<bb_usb_entry *>::iterator it;
	bb_usb_entry *entry;
	int state;
	FILE *fp;

	m_usbAddedList.clear();
	m_usbDestList.clear();
	m_usbChanged = 0;
	m_usbStorageSeen = 0;

	for (it = m_usbSrcList.begin(); it != m_usbSrcList.end(); it++)
		(*it)->keepFlag = 0;

	fp = fopen("/proc/bus/usb/devices", "r");
	if (fp == 0)
		return;

	state = 0;
	entry = 0;
	while (fgets(buf, 255, fp) != 0) {
		if (m_abortFlag != 0) {
			fclose(fp);
			return;
		}
		switch (buf[0]) {
		case 'T':
			entry = new bb_usb_entry;
			state = 1;
			if (entry == 0) {
				fclose(fp);
				return;
			}
			entry->vendor = 0;
			entry->prodId = 0;
			entry->cls = 0;
			entry->sub = 0;
			entry->prot = 0;
			entry->product = "";
			entry->manufacturer = "";
			entry->serialNumber = "";
			entry->driver = "";
			entry->keepFlag = 0;
			usb_check_top(buf, entry);
			break;
		case 'E':
			if (state == 1) {
				usb_add_entry(entry);
				entry = 0;
			}
			state = 0;
			break;
		case 'P':
			if (state == 1)
				usb_check_product(buf, entry);
			break;
		case 'S':
			if (state == 1)
				usb_check_string(buf, entry);
			break;
		case 'I':
			if (state == 1)
				usb_check_info(buf, entry);
			break;
		}
	}
	usb_remove_entry();
	fclose(fp);
}

void thMediaChecker::usb_check_top(char *buf, bb_usb_entry *entry)
{
	char *p = buf + 4;
	char *tok;
	for (;;) {
		tok = strtok(p, " =");
		p = 0;
		if (tok == 0)
			break;
		if (strcmp(tok, "Dev#") != 0)
			continue;
		char *val = strtok(0, " \t");
		if (!val)
			break;
		unsigned int v;
		sscanf(val, "%4x", &v);
		entry->dev = (unsigned short)v;
	}
}

void thMediaChecker::usb_check_product(char *buf, bb_usb_entry *entry)
{
	char *p = buf + 4;
	char *tok;
	for (;;) {
		tok = strtok(p, " =");
		p = 0;
		if (tok == 0)
			break;
		if (strcmp(tok, "Vendor") == 0) {
			tok = strtok(0, " \t");
			if (!tok)
				break;
			unsigned int v;
			sscanf(tok, "%4x", &v);
			entry->vendor = (unsigned short)v;
			continue;
		}
		if (strcmp(tok, "ProdID") == 0) {
			tok = strtok(0, " \t");
			if (!tok)
				break;
			unsigned int v;
			sscanf(tok, "%4x", &v);
			entry->prodId = (unsigned short)v;
			continue;
		}
	}
}

void thMediaChecker::usb_check_string(char *buf, bb_usb_entry *entry)
{
	char *p = buf + 4;
	char *tok;
	for (;;) {
		tok = strtok(p, " =");
		p = 0;
		if (tok == 0)
			break;
		if (strcmp(tok, "Product") == 0) {
			tok = strtok(0, "\n");
			if (!tok)
				break;
			entry->product = tok;
			continue;
		}
		if (strcmp(tok, "Manufacturer") == 0) {
			tok = strtok(0, "\n");
			if (!tok)
				break;
			entry->manufacturer = tok;
			continue;
		}
		if (strcmp(tok, "SerialNumber") == 0) {
			tok = strtok(0, "\n");
			if (!tok)
				break;
			entry->serialNumber = tok;
			continue;
		}
	}
}

void thMediaChecker::usb_check_info(char *buf, bb_usb_entry *entry)
{
	char *p = buf + 4;
	char *tok;
	for (;;) {
		tok = strtok(p, " =");
		p = 0;
		if (tok == 0)
			break;
		if (strcmp(tok, "Cls") == 0) {
			tok = strtok(0, " \t");
			if (!tok)
				break;
			unsigned int v;
			sscanf(tok, "%2x", &v);
			unsigned char b = (unsigned char)v;
			entry->cls = b;
			if (b == 8)
				m_usbStorageSeen = 1;
			continue;
		}
		if (strcmp(tok, "Sub") == 0) {
			tok = strtok(0, " \t");
			if (!tok)
				break;
			unsigned int v;
			sscanf(tok, "%2x", &v);
			entry->sub = (unsigned char)v;
			continue;
		}
		if (strcmp(tok, "Prot") == 0) {
			tok = strtok(0, " \t");
			if (!tok)
				break;
			unsigned int v;
			sscanf(tok, "%2x", &v);
			entry->prot = (unsigned char)v;
			continue;
		}
		if (strcmp(tok, "Driver") == 0) {
			tok = strtok(0, "\n");
			if (!tok)
				break;
			entry->driver = tok;
			continue;
		}
	}
}

/* Adds `entry` to m_usbSrcList and m_usbAddedList unless an equal one is already
 * listed; either way it is marked as still present. A new entry sets
 * m_usbChanged when no USB storage is mounted. */
void thMediaChecker::usb_add_entry(bb_usb_entry *entry)
{
	int found = 0;
	list<bb_usb_entry *>::iterator it;

	for (it = m_usbSrcList.begin(); it != m_usbSrcList.end(); it++) {
		if ((*it)->vendor != entry->vendor)
			continue;
		if ((*it)->prodId != entry->prodId)
			continue;
		if ((*it)->product.compare(entry->product) != 0)
			continue;
		if ((*it)->manufacturer.compare(entry->manufacturer) != 0)
			continue;
		if ((*it)->serialNumber.compare(entry->serialNumber) != 0)
			continue;
		if ((*it)->cls != entry->cls)
			continue;
		if ((*it)->sub != entry->sub)
			continue;
		if ((*it)->prot != entry->prot)
			continue;
		if ((*it)->dev != entry->dev)
			continue;
		(*it)->keepFlag = 1;
		found = 1;
		break;
	}
	if (found)
		return;

	entry->keepFlag = 1;
	m_usbSrcList.push_back(entry);
	m_usbAddedList.push_back(entry);
	if (usb_status != 1)
		m_usbChanged = 1;
}

/* Moves the entries of m_usbSrcList not marked by usb_add_entry
 * (keepFlag == 0) to m_usbDestList. */
void thMediaChecker::usb_remove_entry()
{
	list<bb_usb_entry *>::iterator it = m_usbSrcList.begin();
	while (it != m_usbSrcList.end()) {
		if ((*it)->keepFlag == 0) {
			m_usbChanged = 1;
			m_usbDestList.push_back(*it);
			it = m_usbSrcList.erase(it);
		} else {
			it++;
		}
	}
}

static const char kDevSdPart[16][16] = {
	"/dev/sda1",
	"/dev/sdb1",
	"/dev/sdc1",
	"/dev/sdd1",
	"/dev/sde1",
	"/dev/sdf1",
	"/dev/sdg1",
	"/dev/sdh1",
	"/dev/sdi1",
	"/dev/sdj1",
	"/dev/sdk1",
	"/dev/sdl1",
	"/dev/sdm1",
	"/dev/sdn1",
	"/dev/sdo1",
	"/dev/sdp1",
};

#define MS_MGC_VAL 0xC0ED0000

/* Mounts newly attached USB mass storage (class 8) on /mnt/storage0 (vfat,
 * read-only when write protected) and unmounts the removed devices; with no USB
 * change pending, serve an external unmount request. */
int thMediaChecker::mounter()
{
	struct stat st;
	char dev[16];
	int ret;
	list<bb_usb_entry *>::iterator it;
	int i;
	int r;
	int msStatus;
	unsigned long flags;

	ret = 0;
	if (m_usbChanged != 0) {
		if (lstat("/mnt/storage0", &st) != 0)
			return 0;

		for (it = m_usbAddedList.begin(); it != m_usbAddedList.end(); it++) {
			if (usb_status != 0)
				continue;
			if ((*it)->cls != 8)
				continue;
			flags = MS_MGC_VAL | 8;
			msStatus = MS_Check();
			ms_status = msStatus;
			if (msStatus == 3)
				flags = MS_MGC_VAL | 9;
			else if (msStatus != 0) {
				ms_status = 1;
				continue;
			}
			for (i = 0; i < 20; i++) {
				sprintf(dev, "%s", kDevSdPart[PORT]);
				r = mount(dev, "/mnt/storage0", "vfat", flags, "codepage=932,iocharset=utf8");
				if (r == 0) {
					mount_dev_no = (*it)->dev;
					usb_status = 1;
					mount_port = PORT;
					ret = 1;
					break;
				}
			}
			if (r != -1)
				continue;
			if (mount_dev_no == -1) {
				umount("/mnt/storage0");
				m_usbSrcList.clear();
				ret = 1;
				continue;
			}
			if (mount_dev_no != 0) {
				ret = 0;
				continue;
			}
			usleep(2000000);
			if (mount(dev, "/mnt/storage0", "vfat", flags, "codepage=932,iocharset=utf8") == 0) {
				usb_status = 1;
				ret = 1;
			} else {
				ret = 0;
			}
		}

		for (it = m_usbDestList.begin(); it != m_usbDestList.end(); it++) {
			if (usb_status != 1)
				continue;
			if ((*it)->cls != 8)
				continue;
			while ((r = umount("/mnt/storage0")) != 0)
				sleep(1);
			usb_status = 0;
			mount_dev_no = 0;
			if (r == -1) {  /* redundant as written */
				for (i = 0; i < 20; i++) {
					sleep(1);
					r = umount("/mnt/storage0");
					if (r == 0) {
						usb_status = 0;
						mount_dev_no = 0;
						break;
					}
				}
				if (r != 0) {
					ms_status = 5;
					break;
				}
			}
			ret = 1;
		}
		return ret;
	}

	if (external_unmount == 1 && usb_status == 1) {
		r = umount("/mnt/storage0");
		if (r == 0) {
			m_usbSrcList.clear();
			usb_status = 0;
			mount_dev_no = 0;
		} else if (r == -1) {
			ms_status = 5;
		}
		m_usbChanged = 1;
		external_unmount = 0;
		return 1;
	}
	return 1;
}

/* Loads the three USB modules with fork + execlp, waiting for each to exit. */
void thMediaChecker::LoadModules()
{
	const char *modules[3] = { "sd_mod", "msdos", "usb-storage" };
	int status;

	for (unsigned int i = 0; i < 3; i++) {
		switch (fork()) {
		case -1:
			break;
		case 0:
			execlp("/sbin/modprobe", "/sbin/modprobe", modules[i], 0);
			exit(0);
		default:
			wait(&status);
			break;
		}
	}
}

int thMediaChecker::unmountusb()
{
	external_unmount = 1;
	return 0;
}

/* The old sg interface: a struct sg_header before the CDB. handle_scsi_cmd is
 * the one of the SCSI Generic HOWTO. */
#define SCSI_OFF sizeof(struct sg_header)

int thMediaChecker::handle_scsi_cmd(unsigned int cmd_len, unsigned int in_size, unsigned char *i_buff,
    unsigned int out_size, unsigned char *o_buff)
{
	int status = 0;
	struct sg_header *sg_hd;

	/* safety checks */
	if (!cmd_len)
		return -1;            /* need a cmd_len != 0 */
	if (!i_buff)
		return -1;             /* need an input buffer != NULL */
	if (SCSI_OFF + cmd_len + in_size > SG_BIG_BUFF)
		return -1;
	if (SCSI_OFF + out_size > SG_BIG_BUFF)
		return -1;

	if (!o_buff)
		out_size = 0;      /* no output buffer, no output size */

	/* generic SCSI device header construction */
	sg_hd = (struct sg_header *)i_buff;
	sg_hd->reply_len = SCSI_OFF + out_size;
	sg_hd->twelve_byte = cmd_len == 12;
	sg_hd->result = 0;

	/* send command */
	status = write(fd, i_buff, SCSI_OFF + cmd_len + in_size);
	if (status != SCSI_OFF + cmd_len + in_size || sg_hd->result) {
		/* some error happened */
		fprintf(stderr, "write(generic) result = 0x%x cmd = 0x%x\n", sg_hd->result, i_buff[SCSI_OFF]);
		perror("");
		return status;
	}

	if (!o_buff)
		o_buff = i_buff;       /* buffer pointer check */

	/* retrieve result */
	status = read(fd, o_buff, SCSI_OFF + out_size);
	if (status != SCSI_OFF + out_size || sg_hd->result) {
		/* some error happened */
		fprintf(stderr, "read(generic) result = 0x%x cmd = 0x%x  SCSI_OFF+out_size=%d status = %d\n",
		    sg_hd->result, o_buff[SCSI_OFF], SCSI_OFF + out_size, status);
		fprintf(stderr, "read(generic) sense %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
		    sg_hd->sense_buffer[0], sg_hd->sense_buffer[1], sg_hd->sense_buffer[2], sg_hd->sense_buffer[3],
		    sg_hd->sense_buffer[4], sg_hd->sense_buffer[5], sg_hd->sense_buffer[6], sg_hd->sense_buffer[7],
		    sg_hd->sense_buffer[8], sg_hd->sense_buffer[9], sg_hd->sense_buffer[10], sg_hd->sense_buffer[11],
		    sg_hd->sense_buffer[12], sg_hd->sense_buffer[13], sg_hd->sense_buffer[14], sg_hd->sense_buffer[15]);
	}
	/* Look if we received what we expected */
	if (status == SCSI_OFF + out_size)
		status = 0; /* received them all */

	return status;  /* 0 means no error */
}

/* Checks the USB storage device: 1 or 3 from MS_Check_Device, 2 no medium,
 * 4 MODE SENSE failed, 3 write protected, 0 ready. */
int thMediaChecker::MS_Check()
{
	int deviceStatus = MS_Check_Device(-1);
	if (deviceStatus != 0) {
		if (deviceStatus == 1)
			return 1;
		if (deviceStatus == 3)
			return 3;
	}

	if (MS_Check_Media() == 2)
		return 2;

	int senseRet = Modesense(g_modesenseOutBuf);
	if (senseRet != 0) {
		close(fd);
		return 4;
	}

	unsigned char b = g_modesenseOutBuf[39];
	if ((b & 0x80) != 0) {
		close(fd);
		return 3;
	}
	close(fd);
	return 0;
}

/* Finds the first /dev/sdX that opens (devno == -1: scans; otherwise only
 * devno, < 16), stores its index in PORT and opens the corresponding /dev/sgX in fd.
 * errno 30 (EROFS) counts as found. */
int thMediaChecker::MS_Check_Device(int devno)
{
	char path[16];
	int start;
	int last;
	int i;

	errno = 0;
	if (devno == -1) {
		start = 0;
		/* NOTE: bug: the scan stops before the last device (/dev/sdp). */
		last = 15;
	} else if ((unsigned int)devno < 16) {
		start = devno;
		last = start + 1;
	} else {
		return 1;
	}

	for (i = start; i < last; i++) {
		sprintf(path, "%s", kDevSd[i]);
		errno = 0;
		int fd = open(path, O_RDWR);
		if (fd >= 0) {
			PORT = i;
			close(fd);
			break;
		}
		if (errno == EROFS) {
			PORT = i;
			goto found;
		}
	}
found:
	if (errno != 0 && errno != EROFS)
		return 1;

	errno = 0;
	sprintf(path, "%s", kDevSg[PORT]);
	fd = open(path, O_RDWR);
	if (fd >= 0)
		return 0;
	if (errno == EROFS)
		return 3;
	fprintf(stderr, "Need read/write permissions for /dev/sga. %d\n", fd);
	return 1;
}

int thMediaChecker::MS_Check_Media()
{
	if (TestForMedium() != 0)
		return 0;
	close(fd);
	return 2;
}

/* MODE SENSE(10), all pages (0x3F), allocation length 8. */
int thMediaChecker::Modesense(unsigned char *buf)
{
	Cdb10 cdb = { { 0x5A, 0, 0x3F, 0, 0, 0, 0, 0, 0x08, 0 } };
	cmd.cdb10 = cdb;
	int ret = handle_scsi_cmd(10, 0, (unsigned char *)&cmd, 8, buf);
	if (ret != 0) {
		/* NOTE: bug: wrong message, copied from Inquiry. */
		fprintf(stderr, "Inquiry failed\n");
		return 4;
	}
	return 0;
}

/* INQUIRY, allocation length 96. */
int thMediaChecker::Inquiry(unsigned char *buf)
{
	Cdb6 cdb = { { 0x12, 0, 0, 0, 0x60, 0 } };
	cmd.cdb = cdb;
	int ret = handle_scsi_cmd(6, 0, (unsigned char *)&cmd, 96, buf);
	if (ret != 0) {
		fprintf(stderr, "Inquiry failed\n");
		return 0;
	}
	return (int)(buf + 36);
}

int thMediaChecker::TestForMedium()
{
	Cdb6 cdb;
	memset(&cdb, 0, sizeof(cdb));
	cmd.cdb = cdb;
	int ret = handle_scsi_cmd(6, 0, (unsigned char *)&cmd, 0, 0);
	if (ret != 0) {
		fprintf(stderr, "Test unit ready failed\n");
		return 0;
	}
	int result = 0;
	if (cmd.asc != 58)
		goto notmedium;
	if (cmd.ascq == 0)
		goto done;
notmedium:
	result = 1;
done:
	return result;
}

thMediaChecker::thMediaChecker()
{
	init();
}

void thMediaChecker::abortProc()
{}

void thMediaChecker::releaseProc()
{}
