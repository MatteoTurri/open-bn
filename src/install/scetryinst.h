/*
 * sceTRYInst*: the C interface of the title installer (scetryinst.cpp).
 */

#ifndef SCETRYINST_H
#define SCETRYINST_H

/* License message (260 bytes), copied by value by GetLicMessage. */
struct sceTRYInstLicMessage {
	int data[0x104 / 4];
};

/* Installation progress, updated by SetStatus and read by GetStatus. */
struct sceTRYInstStatus {
	/* 0x00 */ int state;  // 1 ready, 2 installing, 3 done, 4 failed (SetStatus 1)
	/* 0x04 */ int result;  // progress step or error code (SetStatus 2)
	/* 0x08 */ int totalCount;
	/* 0x0C */ int currentCount;  // partitions done so far
	/* 0x10 */ int unk_10;  // SetStatus 7
	/* 0x14 */ int pad_14;
	/* 0x18 */ long long bytesTotal;  // SetStatus 5: added while m_state is 100
	/* 0x20 */ long long accum2;  // SetStatus 4, and 6 while m_state is 100
	/* 0x28 */ long long trailerTotal;  // set by LoadInstInfo
	/* 0x30 */ long long trailer;  // SetStatus 6 while m_state is 101
}; /* size = 0x38 */

extern "C" {
int sceTRYLibVersionCheck(void *pkg);
int sceTRYInstVerCheck(const char *devPath, char *titleSubId, int mode);
int sceTRYInstPreCheck();
int sceTRYInstSizeCheck(const char *totalSize, const char *name, const char *dataSize, const char *otherSize);
int sceTRYInstInit();
int sceTRYInstStart();
int sceTRYInstExit();
int sceTRYInstGetLicMessage(sceTRYInstLicMessage *out);
int sceTRYInstGetStatus(sceTRYInstStatus *out);
int sceTRYInstGetDNASDatapath(char *outBuf);
}

#endif
