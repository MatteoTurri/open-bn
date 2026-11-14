/*
 * sceDNAS2CDL*: the DNAS2 content download queue and its download thread.
 */

#ifndef DNAS2CDL_H
#define DNAS2CDL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Download tag (DLTAG), its fields named after the XML attributes cdlQueAdd copies in; the
   queue keeps a deep copy, with its strings duplicated. */
typedef struct {
	/* 0x00 */ int tag_flag;  /* index of the download method, 1 = installer */
	/* 0x04 */ char *id;
	/* 0x08 */ char *ver;
	/* 0x0C */ int tag_type;
	/* 0x10 */ char *title;
	/* 0x14 */ char *title_id;
	/* 0x18 */ char *title_sub_id;
	/* 0x1C */ int pad_1C;
	/* 0x20 */ long long dldata_size;
	/* 0x28 */ long long inst_size;
	/* 0x30 */ struct {
		int content_type, inst_alloc_type;
	} inst;
	/* 0x38 */ char *do_id;
	/* 0x3C */ char *nextref;
	/* 0x40 */ char *cref;  /* URL of the content */
	/* 0x44 */ char *dist_id;
	/* 0x48 */ int dist_no;
	/* 0x4C */ char *unk_4C;
	/* 0x50 */ int unk_50;  /* cdlQueAdd sets 1 */
	/* 0x54 */ char *cpage;
	/* 0x58 */ char *kref;
	/* 0x5C */ char *kpage;
	/* 0x60 */ char *ackref;
	/* 0x64 */ char *pat_name;
	/* 0x68 */ char *pat_size;
	/* 0x6C */ char *thumbnail_image;
	/* 0x70 */ char *application_name;
	/* 0x74 */ int jump_flag;
} DNAS2DLTag;

/* Public status of the downloader, copied out by sceDNAS2CDLGetStatus. */
typedef struct {
	/* 0x00 */ int state;  /* 1 = download in progress */
	/* 0x04 */ int pad_04[5];
	/* 0x18 */ long long clen;  /* Content-Length of the reply, -1 = unknown */
	/* 0x20 */ long long pos;  /* download position */
} DNAS2CDLStatus;

/* Work buffer passed to the download methods: resume position and counter. */
typedef struct {
	long long pos;
	int n;
	int pad_0C[5];
} DNAS2CDLWork;

int sceDNAS2CDLQueueInsertAt(DNAS2DLTag *tag, int pos);
int sceDNAS2CDLQueueRemove(int idx);
int sceDNAS2CDLQueueGetTotalEntryNum(void);
int sceDNAS2CDLQueueGetEntryInfo(DNAS2DLTag *out, int idx);
int sceDNAS2CDLStop(void);
int sceDNAS2CDLStart(char *name, char *name2, int *cookie);
int sceDNAS2CDLAbort(void);
int sceDNAS2CDLGetThreadID(void);
int sceDNAS2CDLGetStatus(DNAS2CDLStatus *out);

#ifdef __cplusplus
}
#endif

#endif
