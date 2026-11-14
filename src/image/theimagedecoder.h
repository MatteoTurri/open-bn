/*
 * theImageDecoder (image file decoding with a result callback, and EXIF
 * getters) and theImageEncoder (saves a pixel buffer to a file).
 */

#ifndef THEIMAGEDECODER_H
#define THEIMAGEDECODER_H

class ExifReader;

/* Decoding status shared with the caller: the decoders poll cancel and update
   percent (the `k` argument of the GetRawData functions); userData is handed
   to the result callback. */
struct _ImgDecStatus {
	/* 0x00 */ int cancel;  /* 1 = abort the decoding */
	/* 0x04 */ int percent;  /* progress, 0..100 */
	/* 0x08 */ void *userData;
};

class theImageDecoder {
public:
	virtual ~theImageDecoder();

	bool isValidExifTags() const;

	int GetExifUShort(int tag) const;
	float GetExifRational(int tag) const;
	const char *GetExifString(int tag) const;

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

	enum decoded_result {
		RESULT_OK = 0,
		RESULT_NO_MEMORY = 1,
		RESULT_UNSUPPORTED = 2,
		RESULT_CANT_OPEN = 3,
		RESULT_NO_IMAGE = 4,
		RESULT_BAD_FORMAT = 5,
		RESULT_DECODE_FAILED = 6
	};

	/* Format of the buffers handed over (callback arguments 4 and 12, constructor
		   argument 5); PF_AUTO keeps the decoded format. */
	enum pixel_format {
		PF_AUTO = 0,
		PF_ARGB1555 = 1,
		PF_RGB888 = 2,
		PF_RGBA8888 = 3,
		PF_INDEX8 = 4
	};

	typedef void (*Callback)(decoded_result result, int reserved, int count, pixel_format type, void **images,
	    void **cluts, int width, int height, int *delays, int srcWidth, int srcHeight, pixel_format srcType,
	    void *userData);

	theImageDecoder(Callback cb, const char *path, int width, int height, pixel_format type, int alpha, int frame,
	    unsigned int flags, _ImgDecStatus *status);

	static const char *getResultString(decoded_result result, int idx);

	static const int fill_alpha;
	static const int keep_aspect_rate;
	static const int animation;
	static const int format_check;

private:
	/* 0x00 */ Callback m_cb;  /* called on success and on failure */
	/* 0x04 */ int m_count;  /* number of decoded images */
	/* 0x08 */ pixel_format m_ptype;  /* pixel format of the buffers */
	/* 0x0C */ unsigned char **m_images;  /* one pixel buffer per image */
	/* 0x10 */ unsigned char **m_cluts;  /* one palette per image, only for PF_INDEX8 */
	/* 0x14 */ int *m_delays;  /* frame durations */
	/* 0x18 */ ExifReader *m_exifData;
	/* 0x1C: vtable pointer */
};  /* size = 0x20 */

/* Static members only: encode writes a pixel buffer to a file. */
class theImageEncoder {
public:
	enum pixel_format {
		RGBA8888 = 0,
		RGB888 = 1,
		ARGB1555 = 2
	};
	enum encoded_result {
		ENC_OK
	};

	static int encode(const void *data, pixel_format format, unsigned int width, unsigned int height,
	    const char *name);
	static const char *getResultString(encoded_result result, int idx);
};

#endif
