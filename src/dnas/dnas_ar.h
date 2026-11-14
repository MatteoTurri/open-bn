/*
 * DNAS1 "AR nokey" layer: archive open, header and body reads and personalization.
 */

#ifndef DNAS_AR_H
#define DNAS_AR_H

/* Header of a member of the DNAS archive, read by _sce_dnas_ar_nokey_read_format
   before the member name. */
struct dnas_archive_format {
	/* 0x00 */ int type;     /* 1 directory, 2 file */
	/* 0x04 */ int attr;     /* 0 plain file, 1..2 personalized */
	/* 0x08 */ int namelen;  /* length of the name that follows */
	/* 0x0C */ int reserved;
	/* 0x10 */ long long size;  /* member size */
}; /* size = 0x18 */

#ifdef __cplusplus
extern "C" {
#endif

int _sce_dnas_ar_nokey_init(int fd, unsigned char *hdr, void *hdrArg, unsigned char *prof, unsigned int profLen,
    void *unused5, void *unused6, void *unused7, long long *size);
int _sce_dnas_ar_nokey_read_format(char *name, struct dnas_archive_format *fmt);
int _sce_dnas_ar_nokey_skip_body(int len);
int _sce_dnas_ar_nokey_read_body(void *buf, int len, int flag);
int _sce_dnas_ar_nokey_init_personalize(void);
int _sce_dnas_ar_nokey_get_personalize_length(void);
int _sce_dnas_ar_nokey_exit_personalize(void);

#ifdef __cplusplus
}
#endif

#endif
