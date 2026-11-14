/*
 * Image decoder thread (thImageDecoder) and the decoder object it drives
 * (theImageDecoder).
 */

#ifndef THIMAGEDECODER_H
#define THIMAGEDECODER_H

#include <string>
#include <vector>
#include "theimagedecoder.h"
#include "thread/thposixthread.h"

typedef bool (*DecodeFinishCallback)(void *userData, int mode, void *imageBuffer, void *clutBuffer, int width,
    int height, int cbExtra1, int cbExtra2);

/* Thread that decodes one image file (or reads it back from the on-disk
 * cache) and reports the result to the finish callback. */
class thImageDecoder : public thPosixThread {
public:
	thImageDecoder(void *userData, DecodeFinishCallback cb);
	virtual ~thImageDecoder();

	enum ImageMode {
		IMAGE_MODE_DEFAULT = 0
		/* other values unknown */
	};

	/* 0x20 */ void *m_userData;  /* argument of the finish callback */
	/* 0x24 */ DecodeFinishCallback m_finishCallback;

	/* 0x28 */ theImageDecoder *m_exifInfo;
	/* 0x2C */ int unk_2C;
	/* 0x30 */ int unk_30;
	/* 0x34 */ char pad_34[0x234 - 0x34];
	/* 0x234 */ _ImgDecStatus m_decodeStatus;  /* userData = this (init()) */
	/* 0x240 */ string m_cacheName;
	/* 0x244 */ string m_fileName;
	/* 0x248 */ int m_width;
	/* 0x24C */ int m_height;
	/* 0x250 */ int m_requestWidth;
	/* 0x254 */ int m_requestHeight;
	/* 0x258 */ int m_srcWidth;  /* source image size, also passed to the finish callback */
	/* 0x25C */ int m_srcHeight;
	/* 0x260 */ void *m_imageBuffer;
	/* 0x264 */ void *m_clutBuffer;
	/* 0x268 */ int m_cacheFlag;  /* 1 = use the cache */
	/* 0x26C */ int unk_26C;
	/* 0x270 */ int unk_270;
	/* 0x274 */ vector<void *> m_pageBuf;  /* one image buffer per page */
	/* 0x280 */ ImageMode m_requestImageMode;
	/* 0x284 */ ImageMode m_decodeImageMode;
	/* 0x288 */ int m_defaultAlpha;
	/* 0x28C */ int m_decodePage;
	/* 0x290 */ unsigned int m_decodeOption;  /* negative: do not use the cache */

	void abortProc();
	void releaseProc();

	void cacheOn();
	void cacheOff();

	int getWidth();
	int getHeight();
	void *getImageBuffer();
	void *getCLUTBuffer();
	int getPageNum();
	void *getPageImageBuffer(unsigned int index);
	int getProgress();
	int InstanceCount();

	void setRequestSize(int width, int height);
	void setRequestImageMode(ImageMode mode);
	ImageMode getDecodeImageMode();
	void setDecodeOption(unsigned int option);
	void setDefaultAlpht(int alpha);
	void setDecodePage(int page);
	void setFinishCallback(DecodeFinishCallback cb);

	bool isValidExifTags() const;
	const char *getMake() const;
	const char *getModel() const;
	int getOrientation() const;
	const char *getDateTime() const;
	float getFnumber() const;
	int getISOSpeedRatings() const;
	const char *getDateTimeOriginal() const;
	const char *getDateTimeDigitized() const;
	float getShutterSpeedValue() const;
	float getExposureBiasValue() const;
	int getLightSource() const;
	int getFlash() const;
	int getFocalLength() const;

	void setCacheName(const string &name);
	void setFileName(const string &name);

	static void Exit();

	int threadEvent();
	int threadFinished();

	void OpusCallBack(theImageDecoder::decoded_result result, int reserved, int count,
	    theImageDecoder::pixel_format format, void **planes, void **cluts, int width, int height, int *delays,
	    int srcWidth, int srcHeight, theImageDecoder::pixel_format srcType);

	void decode();
	int threadProc();
	void init();
	static void Init();
	void ConvCache();
	void RegistCache(int);
	void DeleteCache();
	int cacheread();
	void cachewrite();
	int imgAverageZoomOut(unsigned char *);
	int imgAverageZoomOutH(unsigned char *);
	int imgAverageZoomOutV(unsigned char *);

	static int instance_count;
};

/* thimagedecoder.cpp defines THIMAGEDECODER_IMPLEMENTATION and provides the
   out-of-line copies. */
#ifndef THIMAGEDECODER_IMPLEMENTATION
inline void thImageDecoder::setFileName(const string &name)
{
	m_fileName = name;
}

inline void thImageDecoder::setRequestSize(int width, int height)
{
	m_requestWidth = width;
	m_requestHeight = height;
}

inline void thImageDecoder::setRequestImageMode(ImageMode mode)
{
	m_requestImageMode = mode;
}

inline void thImageDecoder::setDecodeOption(unsigned int option)
{
	m_decodeOption = option;
}
#endif

struct _IO_FILE;

/* The BB stdio wrappers registered as local file I/O: the FILE * they return
   is the BB file descriptor plus one, so that fd 0 does not collide with the
   NULL of an error. */
long filesize(const char *path);
_IO_FILE *FOPEN(const char *path, const char *mode);
int FCLOSE(_IO_FILE *fp);
int FSEEK(_IO_FILE *fp, long offset, int whence);
unsigned FREAD(void *buf, unsigned int size, unsigned int nmemb, _IO_FILE *fp);
unsigned FWRITE(const void *buf, unsigned int size, unsigned int nmemb, _IO_FILE *fp);
long FTELL(_IO_FILE *fp);

/* Completion callback of theImageDecoder. */
void img_callback(theImageDecoder::decoded_result result, int reserved, int count, theImageDecoder::pixel_format format,
    void **planes, void **cluts, int width, int height, int *delays, int srcWidth, int srcHeight,
    theImageDecoder::pixel_format srcType, void *ctx);

#endif
