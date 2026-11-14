/*
 * thImageDecoder: decodes an image on its thread through theImageDecoder,
 * with an on-disk cache of decoded images indexed by a Database, and the
 * BB stdio wrappers it registers as local file I/O.
 */
#define THIMAGEDECODER_IMPLEMENTATION
#include "thimagedecoder.h"
#include "fs/bburl.h"
#include "fs/bbpath.h"
#include "thread/thscopeguard.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "localfileio.h"
#include "app/database.h"
#include <sys/stat.h>
#include "fs/bbfile.h"

/* live instances, returned by InstanceCount */
int thImageDecoder::instance_count = 0;

/* the cache index, created by Init and deleted by Exit */
static Database *g_cacheDatabase;

/* Database::ForEach context of the cache pruning: total size and entry count. */
struct CachePruneCtx {
	int totalSize; /* bytes of the entries seen so far */
	int count;
};

/* Builds the cache key: "size mtime decodeOption defaultAlpha decodePage
 * w h mode " + file name + " " + cache name + " param". */
static string MakeCacheKey(const string &fileName, unsigned int decodeOption, int defaultAlpha, int decodePage,
    int reqWidth, int reqHeight, int imageMode, long size, long mtime, string &cacheName, int param)
{
	char buf[512];
	char suffix[64];

	sprintf(buf, "%d %d %d %d %d %d %d %d ", size, mtime, decodeOption, defaultAlpha, decodePage, reqWidth,
	    reqHeight, imageMode);
	sprintf(suffix, " %d", param);
	return buf + fileName + " " + cacheName + suffix;
}

/* Builds the cache key in the old format, without the cache name and the
 * parameter; ConvCache looks it up to migrate an old entry. */
static string MakeOldCacheKey(const string &fileName, unsigned int decodeOption, int defaultAlpha, int decodePage,
    int reqWidth, int reqHeight, int imageMode, long size, long mtime)
{
	char buf[512];

	sprintf(buf, "%d %d %d %d %d %d %d %d ", size, mtime, decodeOption, defaultAlpha, decodePage, reqWidth,
	    reqHeight, imageMode);
	return buf + fileName + " ";
}

/* Compare function of the cache index: 0 if the first n bytes match,
 * -1 otherwise. */
static int cmpPrefix(const char *a, const char *b, void *n)
{
	return strncmp(a, b, (unsigned int)n) == 0 ? 0 : -1;
}

/* Pruning callback of the cache index: an entry is a line of space
 * separated fields, the 10th being the cache file name and the 11th its
 * size. Adds them up in ctx and, past 32 MB or 512 entries, removes the
 * file and asks for the entry to be dropped. */
static int cachePruneCallback(const char *key, CachePruneCtx *ctx, int &stop)
{
	const char *p = key;
	int i;

	for (i = 0; i < 9; i++) {
		p = strchr(p, ' ');
		if (p == 0)
			return -1;
		p++;
	}
	const char *e = strchr(p, ' ');
	if (e == 0)
		return -1;
	e++;
	ctx->totalSize += strtol(e, 0, 10);
	ctx->count++;
	if (ctx->totalSize <= 0x2000000 && ctx->count <= 512)
		return 0;

	string name = string(p).substr(0, e - p - 1);
	remove(name.c_str());
	stop = 1;
	return 0;
}

thImageDecoder::thImageDecoder(void *userData, DecodeFinishCallback cb) : m_userData(userData), m_finishCallback(cb)
{
	instance_count++;
	init();
}

/* Frees the EXIF/decoder object, the image and CLUT buffers and every page
 * buffer. */
thImageDecoder::~thImageDecoder()
{
	instance_count--;
	if (m_exifInfo) {
		delete m_exifInfo;
		m_imageBuffer = 0;
	}
	if (m_imageBuffer)
		free(m_imageBuffer);
	if (m_clutBuffer)
		free(m_clutBuffer);
	for (unsigned int i = 0; i < (unsigned int)(m_pageBuf.end() - m_pageBuf.begin()); i++)
		if (m_pageBuf[i])
			free(m_pageBuf[i]);
	m_pageBuf.clear();
}

/* Resets every field (default alpha 255, decode image mode 1, cache on,
 * request mode -1) and registers the local file I/O functions. */
void thImageDecoder::init()
{
	m_cacheName.erase(m_cacheName.begin(), m_cacheName.end());
	m_fileName.erase(m_fileName.begin(), m_fileName.end());

	m_width = 0;
	m_height = 0;
	m_requestWidth = 0;
	m_requestHeight = 0;
	m_requestImageMode = (ImageMode)-1;
	m_srcWidth = 0;
	m_srcHeight = 0;
	m_decodeImageMode = (ImageMode)1;
	m_imageBuffer = 0;
	m_clutBuffer = 0;
	unk_26C = 0;
	unk_270 = 0;

	m_pageBuf.clear();

	unk_2C = 0;
	unk_30 = 0;
	m_defaultAlpha = 255;
	m_decodePage = 0;
	m_decodeOption = 0;
	m_cacheFlag = 1;
	m_decodeStatus.cancel = 0;
	m_decodeStatus.percent = 0;
	m_decodeStatus.userData = this;

	theLocalFileIO().RegisterFileIOFunctions(FOPEN, FCLOSE, FSEEK, FREAD, FWRITE, filesize, FTELL);
	m_exifInfo = 0;
}

/* Frees the previous image and CLUT buffers and decodes, unless the
 * thread is aborted. */
int thImageDecoder::threadProc()
{
	if (m_abortFlag)
		return 0;
	if (m_imageBuffer)
		free(m_imageBuffer);
	m_imageBuffer = 0;
	if (m_clutBuffer)
		free(m_clutBuffer);
	m_clutBuffer = 0;
	decode();
	return 0;
}

int thImageDecoder::threadEvent()
{
	if (m_abortFlag)
		return 1;
	if (!m_userData)
		return 1;
	if (!m_finishCallback)
		return 1;
	m_finishCallback(m_userData, m_decodeImageMode, m_imageBuffer, m_clutBuffer, m_width, m_height, m_srcWidth,
	    m_srcHeight);
	return 1;
}

int thImageDecoder::threadFinished()
{
	if (m_abortFlag)
		return 1;
	if (!m_userData)
		return 1;
	if (!m_finishCallback)
		return 1;
	return m_finishCallback(m_userData, m_decodeImageMode, m_imageBuffer, m_clutBuffer, m_width, m_height,
	           m_srcWidth, m_srcHeight)
	    != 0;
}

/* Set by img_callback when decoding failed or was aborted; decode then
 * destroys the decoder. */
int opus_err_flag = 0;

/* Decodes m_fileName (made absolute and prefixed with "file:" if needed)
 * from the cache when possible, otherwise with a new theImageDecoder, and
 * writes the result to the cache. */
void thImageDecoder::decode()
{
	if (m_fileName.length() == 0)
		return;
	if (m_abortFlag != 0)
		return;

	if (m_fileName.find(':', 0) == string::npos) {
		if (m_fileName.find('/', 0) != 0) {
			/* NOTE: bug: getcwd may write up to 511 bytes into a 496-byte buffer. */
			char cwd[496];
			thScopeGuard guard;  /* unused, but part of the layout */
			string url;

			url = "file:";
			url += getcwd(cwd, 511);
			url += "/" + m_fileName;
			m_fileName = url;
		} else {
			m_fileName = "file:" + m_fileName;
		}
	}

	{
		int reqMode = m_requestImageMode;

		BBUrl bbUrl;
		BBPath bbPath;
		string localPath; /* path given to the decoder */
		string dev;  /* the three outputs of getLocalPathString */
		string rest;
		string extra;
		string acc;
		BBFSType fsType;

		bbUrl.setUrlString(m_fileName);
		localPath = bbUrl.getPathString();
		bbPath.setPathString(bbUrl.getPathString());
		bbPath.getLocalPathString(fsType, dev, rest, extra);

		/* NOTE: bug: acc is built but never used; the decoder gets localPath. */
		if (dev.length() != 0)
			acc = "/" + dev;
		if (rest.length() != 0)
			acc += "/" + rest;
		if (extra.length() != 0)
			acc += "/" + extra;

		if ((int)m_decodeOption < 0)
			m_cacheFlag = 0;
		if (m_cacheFlag && cacheread() != 0)
			return;

		m_cacheName = "";

		int fmt;
		switch (reqMode) {
		case -1:
			fmt = 0;
			break;
		case 19:
			fmt = 4;
			break;
		case 1:
			fmt = 2;
			break;
		case 0:
		default:
			fmt = 3;
			break;
		}

		if (m_abortFlag != 0)
			return;

		opus_err_flag = 0;
		m_exifInfo = new theImageDecoder(img_callback, localPath.c_str(), m_requestWidth, m_requestHeight,
		    (theImageDecoder::pixel_format)fmt, m_defaultAlpha, m_decodePage, m_decodeOption, &m_decodeStatus);
		if (opus_err_flag != 0) {
			if (m_exifInfo) {
				delete m_exifInfo;
				m_imageBuffer = 0;
				m_exifInfo = 0;
			}
			return;
		}

		if (m_cacheFlag)
			cachewrite();
	}
}

/* Writes the decoded image to a new temporary cache file ("CACHE", width,
 * height, the two callback extras, decode mode, then the pixels) and
 * registers it with RegistCache(file size). Files under linux:/mnt/ are
 * not cached. */
void thImageDecoder::cachewrite()
{
	char magic[5] = { 'C', 'A', 'C', 'H', 'E' };
	char tmpl[26] = "/opt0/cache/imcacheXXXXXX";
	char *p = (char *)m_imageBuffer;
	BBUrl bbUrl;
	string path;
	int total;
	int fd;

	bbUrl.setUrlString(m_fileName);
	path = bbUrl.getPathString();

	if (strncmp(path.c_str(), "linux:/mnt/", 11) == 0)
		return;

	fd = mkstemp(tmpl);
	if (fd < 0)
		return;

	m_cacheName = tmpl;
	if (m_cacheName.length() == 0)  /* NOTE: bug: leaks fd and the temporary file */
		return;

	if (write(fd, magic, 5) <= 0) {
		close(fd);
		remove(tmpl);
		return;
	}
	if (write(fd, &m_width, 4) <= 0) {
		close(fd);
		remove(tmpl);
		return;
	}
	if (write(fd, &m_height, 4) <= 0) {
		close(fd);
		remove(tmpl);
		return;
	}
	if (write(fd, &m_srcWidth, 4) <= 0) {
		close(fd);
		remove(tmpl);
		return;
	}
	if (write(fd, &m_srcHeight, 4) <= 0) {
		close(fd);
		remove(tmpl);
		return;
	}
	if (write(fd, &m_decodeImageMode, 4) <= 0) {
		close(fd);
		remove(tmpl);
		return;
	}

	total = 25;
	switch (m_decodeImageMode) {
	case 1:
		total = m_height * m_width * 3 + 25;
		for (int y = 0; y < m_height; y++) {
			if (write(fd, p, m_width * 3) <= 0) {
				close(fd);
				remove(tmpl);
				return;
			}
			sleep(0);
			p += m_width * 3;
		}
		break;
	case 0:
		total = m_height * m_width * 4 + 25;
		for (int y = 0; y < m_height; y++) {
			if (write(fd, p, m_width * 4) <= 0) {
				close(fd);
				remove(tmpl);
				return;
			}
			sleep(0);
			p += m_width * 4;
		}
		break;
	case 19:
		total = m_height * m_width + 1049;
		if (write(fd, m_clutBuffer, 1024) <= 0) {
			close(fd);
			remove(tmpl);
			return;
		}
		sleep(0);
		for (int y = 0; y < m_height; y++) {
			if (write(fd, p, m_width) <= 0) {
				close(fd);
				remove(tmpl);
				return;
			}
			sleep(0);
			p += m_width;
		}
		break;
	}

	close(fd);
	RegistCache(total);
}

/* Reads the cache file (migrating an old entry with ConvCache first):
 * "CACHE" magic, then width, height, extras and decode mode straight into
 * the fields, then the pixels by mode (1: RGB, 0: RGBA, 19: 8-bit indexed
 * plus a 1024-byte CLUT). A bad magic also deletes the cache file. */
int thImageDecoder::cacheread()
{
	unsigned char magic[5] = { 'C', 'A', 'C', 'H', 'E' };
	unsigned char buf[5];
	char *p;

	ConvCache();

	if (m_cacheName.empty())
		return 0;

	FILE *fp = fopen(m_cacheName.c_str(), "r");
	if (!fp)
		return 0;

	if ((int)fread(buf, 1, 5, fp) < 5) {
		fclose(fp);
		return 0;
	}
	for (int i = 0; i < 5; i++) {
		if (magic[i] != buf[i]) {
			fclose(fp);
			DeleteCache();
			return 0;
		}
	}

	if ((int)fread(&m_width, 4, 1, fp) <= 0) {
		fclose(fp);
		return 0;
	}
	if ((int)fread(&m_height, 4, 1, fp) <= 0) {
		fclose(fp);
		return 0;
	}
	if ((int)fread(&m_srcWidth, 4, 1, fp) <= 0) {
		fclose(fp);
		return 0;
	}
	if ((int)fread(&m_srcHeight, 4, 1, fp) <= 0) {
		fclose(fp);
		return 0;
	}
	if (m_width >= 4097) {
		fclose(fp);
		return 0;
	}
	if (m_height >= 4097) {
		fclose(fp);
		return 0;
	}
	if ((int)fread(&m_decodeImageMode, 4, 1, fp) <= 0) {
		fclose(fp);
		return 0;
	}

	switch (m_decodeImageMode) {
	case 1: {
		m_imageBuffer = malloc(m_width * m_height * 3);
		if (!m_imageBuffer) {
			fclose(fp);
			return 0;
		}
		p = (char *)m_imageBuffer;
		for (int y = 0; y < m_height; y++) {
			if ((int)fread(p, 1, m_width * 3, fp) < m_width * 3) {
				fclose(fp);
				return 0;
			}
			sleep(0);
			p += m_width * 3;
		}
		break;
	}
	case 0: {
		m_imageBuffer = malloc(m_width * m_height * 4);
		if (!m_imageBuffer) {
			fclose(fp);
			return 0;
		}
		p = (char *)m_imageBuffer;
		for (int y = 0; y < m_height; y++) {
			if ((int)fread(p, 1, m_width * 4, fp) < m_width * 4) {
				fclose(fp);
				return 0;
			}
			sleep(0);
			p += m_width * 4;
		}
		break;
	}
	case 19: {
		m_imageBuffer = malloc(m_width * m_height);
		if (!m_imageBuffer) {
			fclose(fp);
			return 0;
		}
		m_clutBuffer = malloc(1024);
		if (!m_clutBuffer) {
			free(m_imageBuffer);
			m_imageBuffer = 0;
			fclose(fp);
			return 0;
		}
		if ((int)fread(m_clutBuffer, 1, 1024, fp) < 1024) {
			fclose(fp);
			return 0;
		}
		sleep(0);
		p = (char *)m_imageBuffer;
		for (int y = 0; y < m_height; y++) {
			if ((int)fread(p, 1, m_width, fp) < m_width) {
				fclose(fp);
				return 0;
			}
			sleep(0);
			p += m_width;
		}
		break;
	}
	default:
		return 0;
	}

	fclose(fp);
	return 1;
}

int thImageDecoder::imgAverageZoomOut(unsigned char *src)
{
	unsigned int total[4];
	unsigned char pixel[4];
	unsigned int x, y;

	if (m_requestWidth <= 0 || m_requestHeight <= 0)
		return -1;

	unsigned char *newBuf = (unsigned char *)malloc(m_requestWidth * m_requestHeight * 4);
	if (!newBuf)
		return -1;

	int bpp = (m_decodeImageMode != 0) ? 3 : 4;
	unsigned int srcWidth = m_width;
	unsigned int srcHeight = m_height;
	unsigned int reqWidth = m_requestWidth;
	unsigned int reqHeight = m_requestHeight;

	if (!src)
		return -1;

	int srcStride = srcWidth * bpp;

	for (y = 0; y < reqHeight; y++) {
		unsigned char *destRow = newBuf + (y * reqWidth) * bpp;
		unsigned int y0 = (y * srcHeight) / reqHeight;
		unsigned int y1 = ((y + 1) * srcHeight) / reqHeight;

		for (x = 0; x < reqWidth; x++) {
			unsigned int x0 = (x * srcWidth) / reqWidth;
			unsigned int x1 = ((x + 1) * srcWidth) / reqWidth;
			int colStride = (x1 - x0) * bpp;
			unsigned char *p = src + srcStride * y0 + x0 * bpp;
			int rowSkip = srcStride - colStride;

			total[0] = total[1] = total[2] = total[3] = 0;
			for (unsigned int ry = y0; ry < y1; ry++) {
				unsigned char *end = p + colStride;
				while (p < end) {
					total[0] += *p++;
					total[1] += *p++;
					total[2] += *p++;
					if (bpp == 4)
						total[3] += *p++;
				}
				p += rowSkip;
			}

			unsigned int contribs = (y1 - y0) * (x1 - x0);
			if (contribs == 0)
				contribs = 1;
			pixel[0] = total[0] / contribs;
			pixel[1] = total[1] / contribs;
			pixel[2] = total[2] / contribs;
			if (bpp == 4)
				pixel[3] = total[3] / contribs;

			unsigned char *d = destRow + x * bpp;
			d[0] = pixel[0];
			d[1] = pixel[1];
			d[2] = pixel[2];
			if (bpp == 4)
				d[3] = pixel[3];
		}

		if (y & 1)
			sleep(0);
	}

	if (m_imageBuffer)
		free(m_imageBuffer);
	m_imageBuffer = newBuf;
	m_width = m_requestWidth;
	m_height = m_requestHeight;
	return 0;
}

int thImageDecoder::imgAverageZoomOutH(unsigned char *src)
{
	unsigned int total[4];
	unsigned char pixel[4];
	unsigned int x, y;

	if (m_requestWidth <= 0 || m_requestHeight <= 0)
		return -1;

	unsigned char *newBuf = (unsigned char *)malloc(m_requestWidth * m_requestHeight * 4);
	if (!newBuf)
		return -1;

	int bpp = (m_decodeImageMode != 0) ? 3 : 4;
	unsigned int srcWidth = m_width;
	unsigned int srcHeight = m_height;
	unsigned int reqWidth = m_requestWidth;
	unsigned int reqHeight = m_requestHeight;

	if (src) {
		int srcStride = srcWidth * bpp;
		unsigned int top = 0;

		for (y = 0; y < reqHeight; y++) {
			unsigned int offset = (reqHeight - srcHeight) / 2;
			unsigned int limit = reqHeight - offset - 1;

			if (y < offset) {
				unsigned char *destRow = newBuf + (y * reqWidth) * bpp;
				for (x = 0; x < reqWidth; x++) {
					unsigned char *d = destRow + x * bpp;
					d[0] = 0;
					d[1] = 0;
					d[2] = 0;
					if (bpp == 4)
						d[3] = 128;
				}
				top = y;
			} else if (y >= limit) {
				unsigned char *destRow = newBuf + (y * reqWidth) * bpp;
				for (x = 0; x < reqWidth; x++) {
					unsigned char *d = destRow + x * bpp;
					d[0] = 0;
					d[1] = 0;
					d[2] = 0;
					if (bpp == 4)
						d[3] = 128;
				}
			} else {
				unsigned char *destRow = newBuf + (y * reqWidth) * bpp;
				unsigned int row = y - (top + 1);

				if (srcHeight < row) {
					for (x = 0; x < reqWidth; x++) {
						unsigned char *d = destRow + x * bpp;
						d[0] = 0;
						d[1] = 0;
						d[2] = 0;
						if (bpp == 4)
							d[3] = 128;
					}
					continue;
				} else {
					unsigned int y0 = y;
					unsigned int y1 = y + 1;

					for (x = 0; x < reqWidth; x++) {
						unsigned int x0 = (x * srcWidth) / reqWidth;
						unsigned int x1 = ((x + 1) * srcWidth) / reqWidth;

						int colStride = (x1 - x0) * bpp;
						unsigned char *p = src + srcStride * row + x0 * bpp;
						int rowSkip = srcStride - colStride;

						total[0] = total[1] = total[2] = total[3] = 0;

						for (unsigned int ry = y0; ry < y1; ry++) {
							unsigned char *end = p + colStride;
							while (p < end) {
								total[0] += *p++;
								total[1] += *p++;
								total[2] += *p++;
								if (bpp == 4)
									total[3] += *p++;
							}
							p += rowSkip;
						}

						unsigned int contribs = (y1 - y0) * (x1 - x0);
						if (contribs == 0)
							contribs = 1;
						pixel[0] = total[0] / contribs;
						pixel[1] = total[1] / contribs;
						pixel[2] = total[2] / contribs;
						if (bpp == 4)
							pixel[3] = total[3] / contribs;

						unsigned char *d = destRow + x * bpp;
						d[0] = pixel[0];
						d[1] = pixel[1];
						d[2] = pixel[2];
						if (bpp == 4)
							d[3] = pixel[3];
					}
				}
			}

			if (y & 1)
				sleep(0);
		}
	} else {
		return -1;
	}

	if (m_imageBuffer)
		free(m_imageBuffer);
	m_imageBuffer = newBuf;
	m_width = m_requestWidth;
	m_height = m_requestHeight;
	m_srcHeight = m_requestHeight;
	return 0;
}

int thImageDecoder::imgAverageZoomOutV(unsigned char *src)
{
	unsigned int total[4];
	unsigned char pixel[4];

	if (m_requestWidth <= 0 || m_requestHeight <= 0)
		return -1;

	unsigned char *newBuf = (unsigned char *)malloc(m_requestWidth * m_requestHeight * 4);
	if (!newBuf)
		return -1;

	int bpp = (m_decodeImageMode != 0) ? 3 : 4;
	unsigned int srcWidth = m_width;
	unsigned int srcHeight = m_height;
	unsigned int reqWidth = m_requestWidth;
	unsigned int offset = (reqWidth - srcWidth) / 2;
	unsigned int offsetEnd = offset + srcWidth;

	unsigned int reqHeight = m_requestHeight;

	if (src) {
		int srcStride = srcWidth * bpp;

		for (unsigned int y = 0; y < reqHeight; y++) {
			unsigned char *destRow = newBuf + (y * reqWidth) * bpp;
			unsigned int y0 = (y * srcHeight) / reqHeight;
			unsigned int y1 = ((y + 1) * srcHeight) / reqHeight;

			for (unsigned int x = 0; x < reqWidth; x++) {
				if (x < offset) {
					unsigned char *d = destRow + x * bpp;
					d[0] = 0;
					d[1] = 0;
					d[2] = 0;
					if (bpp == 4)
						d[3] = 128;
				} else if (x < offsetEnd) {
					unsigned int x0 = x - offset;
					unsigned int x1 = x0 + 1;
					int colStride = (x1 - x0) * bpp;
					unsigned char *p = src + srcStride * y0 + x0 * bpp;
					int rowSkip = srcStride - colStride;

					total[0] = total[1] = total[2] = total[3] = 0;
					for (unsigned int ry = y0; ry < y1; ry++) {
						unsigned char *end = p + colStride;
						while (p < end) {
							total[0] += *p++;
							total[1] += *p++;
							total[2] += *p++;
							if (bpp == 4)
								total[3] += *p++;
						}
						p += rowSkip;
					}

					unsigned int contribs = (y1 - y0) * (x1 - x0);
					if (contribs == 0)
						contribs = 1;
					pixel[0] = total[0] / contribs;
					pixel[1] = total[1] / contribs;
					pixel[2] = total[2] / contribs;
					if (bpp == 4)
						pixel[3] = total[3] / contribs;

					unsigned char *d = destRow + x * bpp;
					d[0] = pixel[0];
					d[1] = pixel[1];
					d[2] = pixel[2];
					if (bpp == 4)
						d[3] = pixel[3];
				} else {
					unsigned char *d = destRow + x * bpp;
					d[0] = 0;
					d[1] = 0;
					d[2] = 0;
					if (bpp == 4)
						d[3] = 128;
				}
			}

			if (y & 1)
				sleep(0);
		}
	} else {
		return -1;
	}

	if (m_imageBuffer)
		free(m_imageBuffer);
	m_imageBuffer = newBuf;
	m_width = m_requestWidth;
	m_height = m_requestHeight;
	return 0;
}

/* Whether the decoder thread was asked to stop (exported, no caller in the program). */
extern "C" int IsDecodeCancel(thImageDecoder *d)
{
	if (d == 0)
		return 0;
	return d->m_abortFlag != 0;
}

bool thImageDecoder::isValidExifTags() const
{
	if (!m_exifInfo)
		return false;
	return m_exifInfo->isValidExifTags();
}

const char *thImageDecoder::getMake() const
{
	if (!m_exifInfo)
		return 0;
	return m_exifInfo->getMake();
}

const char *thImageDecoder::getModel() const
{
	if (!m_exifInfo)
		return 0;
	return m_exifInfo->getModel();
}

int thImageDecoder::getOrientation() const
{
	if (!m_exifInfo)
		return 0;
	return m_exifInfo->getOrientation();
}

const char *thImageDecoder::getDateTime() const
{
	if (!m_exifInfo)
		return 0;
	return m_exifInfo->getDateTime();
}

float thImageDecoder::getFnumber() const
{
	if (!m_exifInfo)
		return 0.0f;
	return m_exifInfo->getFnumber();
}

int thImageDecoder::getISOSpeedRatings() const
{
	if (!m_exifInfo)
		return 0;
	return m_exifInfo->getISOSpeedRatings();
}

const char *thImageDecoder::getDateTimeOriginal() const
{
	if (!m_exifInfo)
		return 0;
	return m_exifInfo->getDateTimeOriginal();
}

const char *thImageDecoder::getDateTimeDigitized() const
{
	if (!m_exifInfo)
		return 0;
	return m_exifInfo->getDateTimeDigitized();
}

float thImageDecoder::getShutterSpeedValue() const
{
	if (!m_exifInfo)
		return 0.0f;
	return m_exifInfo->getShutterSpeedValue();
}

float thImageDecoder::getExposureBiasValue() const
{
	if (!m_exifInfo)
		return 0.0f;
	return m_exifInfo->getExposureBiasValue();
}

int thImageDecoder::getLightSource() const
{
	if (!m_exifInfo)
		return 0;
	return m_exifInfo->getLightSource();
}

int thImageDecoder::getFlash() const
{
	if (!m_exifInfo)
		return 0;
	return m_exifInfo->getFlash();
}

int thImageDecoder::getFocalLength() const
{
	if (!m_exifInfo)
		return 0;
	return m_exifInfo->getFocalLength();
}

/* Returns the size of the file, 0 if stat fails. */
long filesize(const char *path)
{
	struct stat st;

	if (BBlstatCPP(path, &st) != 0)
		return 0;
	return st.st_size;
}

_IO_FILE *FOPEN(const char *path, const char *mode)
{
	int fp = BBfopenCPP(path, mode);

	if (fp < 0)
		return 0;
	return (_IO_FILE *)(fp + 1);
}

int FCLOSE(_IO_FILE *fp)
{
	return BBfcloseCPP((int)fp - 1);
}

int FSEEK(_IO_FILE *fp, long offset, int whence)
{
	return BBfseekCPP((int)fp - 1, offset, whence);
}

unsigned FREAD(void *buf, unsigned int size, unsigned int nmemb, _IO_FILE *fp)
{
	return BBfreadCPP(buf, size, nmemb, (int)fp - 1);
}

unsigned FWRITE(const void *buf, unsigned int size, unsigned int nmemb, _IO_FILE *fp)
{
	return BBfwriteCPP(buf, size, nmemb, (int)fp - 1);
}

long FTELL(_IO_FILE *fp)
{
	return BBftellCPP((int)fp - 1);
}

/* Decoder callback: forwards to thImageDecoder::OpusCallBack on success
 * and clears opus_err_flag, otherwise sets it. */
void img_callback(theImageDecoder::decoded_result result, int reserved, int count, theImageDecoder::pixel_format format,
    void **planes, void **cluts, int width, int height, int *delays, int srcWidth, int srcHeight,
    theImageDecoder::pixel_format srcType, void *ctx)
{
	if (result != theImageDecoder::RESULT_OK) {
		opus_err_flag = 1;
		return;
	}
	if (!ctx) {
		opus_err_flag = 1;
		return;
	}
	((thImageDecoder *)ctx)
	    ->OpusCallBack(result, reserved, count, format, planes, cluts, width, height, delays, srcWidth, srcHeight,
	        srcType);
	opus_err_flag = 0;
}

#define TID_DPRINTF(args)

/* Decoded image: stores the decode mode (format 3 -> 0, 2 -> 1, 4 -> 19,
 * anything else is ignored), the image and CLUT buffers, the size and the
 * extras; RGBA and CLUT alphas are halved to the 0..128 range. */
void thImageDecoder::OpusCallBack(theImageDecoder::decoded_result result, int reserved, int count,
    theImageDecoder::pixel_format format, void **planes, void **cluts, int width, int height, int *delays, int srcWidth,
    int srcHeight, theImageDecoder::pixel_format srcType)
{
	switch (format) {
	case theImageDecoder::PF_RGBA8888:
		m_decodeImageMode = (ImageMode)0;
		break;
	case theImageDecoder::PF_RGB888:
		m_decodeImageMode = (ImageMode)1;
		break;
	case theImageDecoder::PF_INDEX8:
		m_decodeImageMode = (ImageMode)19;
		break;
	default:
		return;
	}

	if (count > 1)
		for (int i = 0; i < count; i++)
			TID_DPRINTF(("plane %d: %p\n", i, planes[i]));

	if (m_decodeImageMode == 19) {
		m_imageBuffer = planes[0];
		m_clutBuffer = cluts[0];
	} else {
		m_imageBuffer = planes[0];
		m_clutBuffer = 0;
	}

	if (format == theImageDecoder::PF_RGBA8888) {
		unsigned char *p = (unsigned char *)m_imageBuffer + 3;
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				*p = (unsigned int)(*p + 1) >> 1;
				p += 4;
			}
		}
	} else if (format == theImageDecoder::PF_INDEX8) {
		unsigned char *p = (unsigned char *)m_clutBuffer + 3;
		for (int i = 0; i < 256; i++) {
			*p = (unsigned int)(*p + 1) >> 1;
			p += 4;
		}
	}

	m_width = width;
	m_height = height;
	m_srcWidth = srcWidth;
	m_srcHeight = srcHeight;
}

/* Migrates an old cache entry: looks the old key up in the index and
 * takes the cache file name from its 10th field; m_cacheName is cleared if
 * there is none. */
void thImageDecoder::ConvCache()
{
	BBUrl bbUrl;
	string path;

	bbUrl.setUrlString(m_fileName);
	path = bbUrl.getPathString();

	struct stat st;
	if (BBlstatCPP(path.c_str(), &st) == -1)
		return;

	string key = MakeOldCacheKey(m_fileName, m_decodeOption, m_defaultAlpha, m_decodePage, m_requestWidth,
	    m_requestHeight, m_requestImageMode, st.st_size, st.st_mtime);
	vector<string> found = g_cacheDatabase->Find(key.c_str(), (void *)key.length(), 1);

	if (found.size() != 0) {
		unsigned int pos = 0;
		int n;

		for (n = 0; n < 9; n++) {
			pos = found[0].find(" ", pos);
			if (pos == string::npos)
				goto notfound;
			pos++;
		}
		n = found[0].find(" ", pos) - pos;
		string &first = found[0];
		string tmp;
		tmp.assign(first, pos, n);
		m_cacheName = tmp;
		return;
	}
notfound:
	m_cacheName = "";
}

/* registrations left before the next pruning */
static int g_registCacheCount = 0;

/* Adds the entry of the current file to the cache index, and prunes the
 * index every 10 registrations. param is the size of the cache file. */
void thImageDecoder::RegistCache(int param)
{
	BBUrl bbUrl;
	string path;

	bbUrl.setUrlString(m_fileName);
	path = bbUrl.getPathString();

	struct stat st;
	if (BBlstatCPP(path.c_str(), &st) == -1)
		return;

	string key = MakeCacheKey(m_fileName, m_decodeOption, m_defaultAlpha, m_decodePage, m_requestWidth,
	    m_requestHeight, m_requestImageMode, st.st_size, st.st_mtime, m_cacheName, param);
	g_cacheDatabase->Add(key.c_str());
	if (--g_registCacheCount <= 0) {
		CachePruneCtx ctx;
		ctx.totalSize = 0;
		ctx.count = 0;
		g_cacheDatabase->ForEach((int (*)(const char *, void *, bool &))cachePruneCallback, &ctx, 1);
		g_registCacheCount = 10;
	}
}

/* Deletes the cache file and clears m_cacheName. */
void thImageDecoder::DeleteCache()
{
	unlink(m_cacheName.c_str());
	m_cacheName = "";
}

/* Creates the shared cache index on /opt0/cache/index. */
void thImageDecoder::Init()
{
	g_cacheDatabase = new Database("/opt0/cache/index", cmpPrefix);
}

void thImageDecoder::Exit()
{
	if (g_cacheDatabase)
		delete g_cacheDatabase;
	g_cacheDatabase = 0;
}

void thImageDecoder::setFileName(const string &name)
{
	m_fileName = name;
}

void thImageDecoder::setCacheName(const string &name)
{
	m_cacheName = name;
}

int thImageDecoder::getWidth()
{
	return m_width;
}

int thImageDecoder::getHeight()
{
	return m_height;
}

void *thImageDecoder::getImageBuffer()
{
	return m_imageBuffer;
}

void *thImageDecoder::getCLUTBuffer()
{
	return m_clutBuffer;
}

int thImageDecoder::getPageNum()
{
	return m_pageBuf.end() - m_pageBuf.begin();
}

void *thImageDecoder::getPageImageBuffer(unsigned int index)
{
	if (index < (unsigned int)(m_pageBuf.end() - m_pageBuf.begin()))
		return m_pageBuf[index];
	return 0;
}

void thImageDecoder::setRequestSize(int width, int height)
{
	m_requestWidth = width;
	m_requestHeight = height;
}

void thImageDecoder::setRequestImageMode(ImageMode mode)
{
	m_requestImageMode = mode;
}

thImageDecoder::ImageMode thImageDecoder::getDecodeImageMode()
{
	return m_decodeImageMode;
}

void thImageDecoder::setDecodeOption(unsigned int option)
{
	m_decodeOption = option;
}

void thImageDecoder::setDefaultAlpht(int alpha)
{
	m_defaultAlpha = alpha;
}

void thImageDecoder::setDecodePage(int page)
{
	m_decodePage = page;
}

void thImageDecoder::cacheOn()
{
	m_cacheFlag = 1;
}

void thImageDecoder::cacheOff()
{
	m_cacheFlag = 0;
}

void thImageDecoder::setFinishCallback(DecodeFinishCallback cb)
{
	m_finishCallback = cb;
}

int thImageDecoder::getProgress()
{
	return m_decodeStatus.percent;
}

void thImageDecoder::abortProc()
{}

void thImageDecoder::releaseProc()
{}

int thImageDecoder::InstanceCount()
{
	return instance_count;
}
