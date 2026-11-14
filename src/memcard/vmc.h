/*
 * Virtual memory card library (sceVmc*, vmc.c and card.c): the
 * PS1 memory card emulation.
 */

#ifndef VMC_H
#define VMC_H

/* Directory entry filled by sceVmcGetDir (_card_get_dir). */
typedef struct DIRENTRY {
	/* 0x00 */ char EntryName[20];
	/* 0x14 */ unsigned int AttrFile;
	/* 0x18 */ unsigned int FileSizeByte;
	/* 0x1C */ char *EntryNamePtr;  /* EntryName; 0 in the entry after the last */
	/* 0x20 */ unsigned int Reserve[2];
} DIRENTRY; /* size = 0x28 */

#ifdef __cplusplus
extern "C" {
#endif

/* image: the memory card image mapped in memory. */
int sceVmcInit(void);
int sceVmcEnd(void);
int sceVmcAttach(void *image);
int sceVmcGetInfo(void *image, int *outFree, int *outAttached, int *outSize);
int sceVmcReadFile(void *image, const char *name, void *buf, int offset, int size);
int sceVmcWriteFile(void *image, const char *name, const void *buf, int offset, int size);
int sceVmcDeleteFile(void *image, const char *name);
int sceVmcCreateFile(void *image, const char *name, int nblocks);
int sceVmcGetDir(void *image, int a1, int a2, DIRENTRY *ent);
int sceVmcFormat(void *image);

#ifdef __cplusplus
}
#endif

#endif
