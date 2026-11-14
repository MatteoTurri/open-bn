/*
 * thMediaChecker: thread that watches the CDVD drive and USB mass storage
 * devices (disc type, USB driver list, mounting) and reports changes through
 * callbacks.
 */

#ifndef THMEDIACHECKER_H
#define THMEDIACHECKER_H

#include <list>
#include <string>

typedef bool (*MediaCallback)(void *);

/* One entry of the USB device list, parsed from "key =value" text. */
struct bb_usb_entry {
	/* 0x00 */ unsigned short vendor;  /* "Vendor" %4x */
	/* 0x02 */ unsigned short prodId;  /* "ProdID" %4x */
	/* 0x04 */ string product;  /* "Product" */
	/* 0x08 */ string manufacturer;  /* "Manufacturer" */
	/* 0x0C */ string serialNumber;  /* "SerialNumber" */
	/* 0x10 */ unsigned char cls;  /* "Cls" %2x */
	/* 0x11 */ unsigned char sub;  /* "Sub" %2x */
	/* 0x12 */ unsigned char prot;  /* "Prot" %2x */
	/* 0x13 */ char pad_13[1];
	/* 0x14 */ string driver;  /* "Driver" */
	/* 0x18 */ int keepFlag;  /* 1 = still present */
	/* 0x1C */ unsigned short dev;  /* "Dev#" %4x */
};

/* Node of a circular doubly linked list (the head has the same layout). */
struct UsbNode {
	/* 0x00 */ UsbNode *next;
	/* 0x04 */ UsbNode *prev;
	/* 0x08 */ void *info;
	/* 0x0C */ char pad_12[4];
};

#include "thread/thposixthread.h"

class thMediaChecker : public thPosixThread {
public:
	thMediaChecker();
	virtual ~thMediaChecker();

	/* Flags shared with the other threads, each guarded by its mutex. */
	static pthread_mutex_t skipusb_flag_mutex;
	static pthread_mutex_t skipcdvd_flag_mutex;
	static pthread_mutex_t cdvd_type_mutex;
	static int skipusb_flag;
	static int skipcdvd_flag;
	static int cdvd_type;       /* cached CDVD type */
	static int illegal_flag;    /* != 0: getCDVDType ignores the cache */

	/* 0x20 */ int m_discEventPending;
	/* 0x24 */ int m_usbChanged;
	/* 0x28 */ int m_usbStorageSeen;

	/* 0x2C */ list<bb_usb_entry *> m_usbSrcList;
	/* 0x30 */ list<bb_usb_entry *> m_usbAddedList;
	/* 0x34 */ list<bb_usb_entry *> m_usbDestList;

	/* 0x38 */ void *m_usbUserData;
	/* 0x3C */ void *m_discUserData;
	/* 0x40 */ MediaCallback m_usbCallback;
	/* 0x44 */ MediaCallback m_discCallback;

	void init();
	void setUSBCallback(void *userData, MediaCallback cb);
	void setDISCCallback(void *userData, MediaCallback cb);

	static int getCDVDType();

	int skipcdvd();
	int restartcdvd();
	static int getSkipCDVDFlag();

	int skipusb();
	int controlUSBCheck();
	static int restartusb();
	static int getSkipUSBFlag();

	int threadProc();
	int threadFinished();
	int threadEvent();

	void abortProc();
	void releaseProc();

	int MS_Check_Media();
	int unmountusb();

	void MediaCheck();
	void MediaCheckUSBStorage();
	int TestForMedium();

	void usb_check_top(char *buf, bb_usb_entry *entry);
	void usb_check_product(char *buf, bb_usb_entry *entry);
	void usb_check_string(char *buf, bb_usb_entry *entry);
	void usb_check_info(char *buf, bb_usb_entry *entry);

	void LoadModules();

	int handle_scsi_cmd(unsigned int cdbLen, unsigned int dir, unsigned char *cmdBlock, unsigned int dataLen,
	    unsigned char *dataBuf);

	int Inquiry(unsigned char *buf);
	int Modesense(unsigned char *buf);

	int MediaCheckCDVD();

	int MS_Check_Device(int mode);

	int MS_Check();

	int mounter();

	void usb_add_entry(bb_usb_entry *entry);
	void usb_remove_entry();

	static int fd;               /* fd of the open sg device */
	static int usb_status;       /* 1 = USB storage mounted */
	static int ms_status;        /* state of the last MS_Check_* */
	static int mount_dev_no;
	static int external_unmount;
	static int PORT;             /* index of the SCSI device found */
	static int mount_port;       /* index of the mounted USB device */
};

/* thmediachecker.cpp defines THMEDIACHECKER_IMPLEMENTATION and provides the
   out-of-line copy. */
#ifndef THMEDIACHECKER_IMPLEMENTATION
inline thMediaChecker::thMediaChecker()
{
	init();
}
#endif

#endif
