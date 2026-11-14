/*
 * Memory card library of the PS2 Linux Kit (sceMc*, scemc.c): types and functions.
 */

#ifndef SCEMC_H
#define SCEMC_H

/* Open flags of sceMcOpen (the IOP file flags of sifdev.h, also taken by scepfsOpen). */
#define SCE_RDONLY 0x0001
#define SCE_WRONLY 0x0002
#define SCE_RDWR   0x0003
#define SCE_CREAT  0x0200
#define SCE_TRUNC  0x0400

/* Origins of sceMcSeek (and scepfsLseek). */
#define SCE_SEEK_SET 0
#define SCE_SEEK_CUR 1
#define SCE_SEEK_END 2

/* Bits of sceMcTblGetDir::AttrFile. */
#define sceMcFileAttrReadable   0x0001
#define sceMcFileAttrWriteable  0x0002
#define sceMcFileAttrExecutable 0x0004
#define sceMcFileAttrSubdir     0x0020
#define sceMcFileAttrPDAExec    0x0800 /* PocketStation application */
#define sceMcFileAttrPS1        0x1000 /* PS1 save */

/* Date and time of a memory card directory entry. */
typedef struct sceMcStDateTime {
	unsigned char Resv2;
	unsigned char Sec;
	unsigned char Min;
	unsigned char Hour;
	unsigned char Day;
	unsigned char Month;
	unsigned short Year;
} sceMcStDateTime; /* size = 0x8 */

/* Directory entry filled by sceMcGetDir. */
typedef struct sceMcTblGetDir {
	/* 0x00 */ sceMcStDateTime _Create;
	/* 0x08 */ sceMcStDateTime _Modify;
	/* 0x10 */ unsigned int FileSizeByte;
	/* 0x14 */ unsigned short AttrFile;
	/* 0x16 */ unsigned short Reserve1;
	/* 0x18 */ unsigned int Reserve2;
	/* 0x1C */ unsigned int PdaAplNo;
	/* 0x20 */ unsigned char EntryName[32];
} sceMcTblGetDir; /* size = 0x40 */

#ifdef __cplusplus
extern "C" {
#endif

int sceMcInit(void);
int sceMcExit(void);
int sceMcChangeThreadPriority(int prio);
int sceMcGetSlotMax(int port);
int sceMcGetInfo(int port, int slot, int *type, int *free, int *format);
int sceMcOpen(int port, int slot, const char *name, int mode);
int sceMcClose(int fd);
int sceMcSeek(int fd, int offset, int whence);
int sceMcRead(int fd, void *buf, int count);
int sceMcWrite(int fd, const void *buf, int count);
int sceMcFlush(int fd);
int sceMcMkdir(int port, int slot, const char *name);
int sceMcChdir(int port, int slot, const char *name, char *curDir);
/* Asynchronous: returns 0 once the command is started; the entry count
   comes from sceMcSync. */
int sceMcGetDir(int port, int slot, const char *name, unsigned int mode, int maxEntries, sceMcTblGetDir *table);
int sceMcSetFileInfo(int port, int slot, const char *name, const void *info, unsigned int flags);
int sceMcDelete(int port, int slot, const char *name);
int sceMcRename(int port, int slot, const char *name, const char *newName);
int sceMcFormat(int port, int slot);
int sceMcUnformat(int port, int slot);
int sceMcGetEntSpace(int port, int slot, const char *name);
int sceMcSync(int mode, int *cmd, int *result);

/* Internal: the request senders behind the calls above. */
int sceSifCallRpc(void *client, int fno, int mode, void *sendbuf, int ssize, void *recvbuf, int rsize, void *endfunc,
    void *efarg);
int sceMcRead_sub(int fd, void *buf, int size);
int sceMcWrite_sub(int fd, void *buf, int size);
int sceMcGetDir_sub(int port, int slot, const char *name, int mode, int count, void *buf);

#ifdef __cplusplus
}
#endif

#endif
