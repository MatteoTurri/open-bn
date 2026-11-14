/*
 * DNAS1 "AR nokey" layer: archive open, header and body reads and
 * personalization, mostly thin wrappers over DNAS2 functions.
 */

#include <string.h>

#include "lib/scepfsd.h"
#include "dnas_ar.h"
#include "dnas2read.h"

static int g_ar_fd = -1; /* -1: no file open */
static unsigned char g_ar_buf[16];
static const char g_ar_key_scei[] = "SCEI";
static const char g_ar_key_title[] = "title";
static const char g_ar_key_subtitle[] = "sub_title";

static int ar_nokey_read_raw(void *buf, int len, int flag);

/* Reads the 16-byte archive header of the open file fd, loads the profile (from
 * prof when given, else from fd: __sce_dnas2_symbol65037) and returns the 64-bit
 * field at header + 8 in *size. */
int _sce_dnas_ar_nokey_init(int fd, unsigned char *hdr, void *hdrArg, unsigned char *prof, unsigned int profLen,
    void *unused5, void *unused6, void *unused7, long long *size)
{
	g_ar_fd = fd;
	memset(g_ar_buf, 0, 16);
	if (scepfsRead(g_ar_fd, g_ar_buf, 16) != 16)
		return -2;
	if (__sce_dnas2_symbol65037(prof, profLen, hdr, hdrArg, g_ar_fd, g_ar_key_scei, g_ar_key_title,
	        g_ar_key_subtitle, 1)
	    < 0)
		return -1;
	*size = *(long long *)(g_ar_buf + 8);
	return 0;
}

/* Reads a 24-byte member header into `fmt` and the name that follows (at most
 * 1024 bytes) into `name`; -10000 at the end of the archive. */
int _sce_dnas_ar_nokey_read_format(char *name, struct dnas_archive_format *fmt)
{
	int n;

	n = ar_nokey_read_raw(fmt, 24, 0);
	if (n == 0)
		return -10000;
	if (n != 24)
		return -2;
	if (fmt->namelen >= 1025)
		return -2;
	n = ar_nokey_read_raw(name, fmt->namelen, 0);
	if (n != fmt->namelen)
		return -2;
	name[n] = 0;
	return 0;
}

/* Skips `len` bytes of entry body, 1 KB at a time. */
int _sce_dnas_ar_nokey_skip_body(int len)
{
	unsigned char buf[1024];
	int chunk;
	int n;

	if (len == 0)
		return 0;
	do {
		chunk = (len < 1025) ? len : 1024;
		n = ar_nokey_read_raw(buf, chunk, 0);
		len -= n;
		if (n != chunk)
			return -1;
	} while (len != 0);
	return 0;
}

/* Reads `len` bytes of entry body. */
int _sce_dnas_ar_nokey_read_body(void *buf, int len, int flag)
{
	return ar_nokey_read_raw(buf, len, flag);
}

/* Reads through the DNAS2 record reader (__sce_dnas2_symbol65039). */
static int ar_nokey_read_raw(void *buf, int len, int flag)
{
	return __sce_dnas2_symbol65039(buf, len);
}

/* Opens the personalization session (__sce_dnas2_symbol65038). */
int _sce_dnas_ar_nokey_init_personalize(void)
{
	return __sce_dnas2_symbol65038();
}

/* Returns the total length of the personalized data. */
int _sce_dnas_ar_nokey_get_personalize_length(void)
{
	return __sce_dnas2_symbol65036();
}

/* Closes the personalization session. */
int _sce_dnas_ar_nokey_exit_personalize(void)
{
	return __sce_dnas2_symbol65035();
}
