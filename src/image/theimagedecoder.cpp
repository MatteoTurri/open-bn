/*
 * theImageDecoder: decodes an image file with the decoder chosen from its
 * extension and hands the result to a callback. Also theImageEncoder and the
 * ImageInfo table of supported formats.
 */

#pragma implementation "theimagedecoder.h"
#include <new>
#include <string.h>
#include <vector>
#include "theimagedecoder.h"
#include "eiclerror.h"
#include "imagergba.h"
#include "iofilter.h"
#include "inputiofilefilter.h"
#include "pgprops.h"
#include "inputdatafilterpng.h"
#include "inputdatafilterjpeg.h"
#include "imgopzoom.h"
#include "exifreader.h"
#include "outputiofilefilter.h"
#include "outputdatafilter.h"
#include "inputdatafilterbmp.h"
#include "inputdatafiltergif.h"
#include "inputdatafiltertiff.h"
#include "inputdatafiltermng.h"
#include "outputdatafilterbmp.h"
#include "outputdatafiltergif.h"
#include "outputdatafilterjpeg.h"
#include "outputdatafilterpng.h"
#include "outputdatafiltertiff.h"

typedef int BOOL;
#define FALSE 0
#define TRUE 1

/* ImageInfo: the static table of supported formats and its helpers. */
class OutputDataFilter;

class ImageInfo {
public:
	struct FormatInfo {
		/* 0x00 */ const char *ext[4];
		/* 0x10 */ InputDataFilter *(*createDecoder)(InputIoFilter *);
		/* 0x14 */ void *createEncoder;
		/* 0x18 */ int flag18;
		/* 0x1C */ int flag1c;
	};

	static FormatInfo SupportFormats[7];

	static FormatInfo *SearchFormatInfo(const char *filename);
	static InputDataFilter *CreateDecoderBMP(InputIoFilter *filter);
	static InputDataFilter *CreateDecoderGIF(InputIoFilter *filter);
	static InputDataFilter *CreateDecoderJPEG(InputIoFilter *filter);
	static InputDataFilter *CreateDecoderPNG(InputIoFilter *filter);
	static InputDataFilter *CreateDecoderTIFF(InputIoFilter *filter);
	static InputDataFilter *CreateDecoderMNG(InputIoFilter *filter);
	static OutputDataFilter *CreateEncoderBMP(OutputIoFilter *filter);
	static OutputDataFilter *CreateEncoderGIF(OutputIoFilter *filter);
	static OutputDataFilter *CreateEncoderJPEG(OutputIoFilter *filter);
	static OutputDataFilter *CreateEncoderPNG(OutputIoFilter *filter);
	static OutputDataFilter *CreateEncoderTIFF(OutputIoFilter *filter);
};

/* Supported formats: extensions, decoder and encoder factories and two flags
   (the second is set for JPEG and MNG); terminated by a zeroed entry. */
ImageInfo::FormatInfo ImageInfo::SupportFormats[7] = {
	{ { "bmp", 0, 0, 0 }, CreateDecoderBMP, (void *)CreateEncoderBMP, 0, 0 },
	{ { "gif", 0, 0, 0 }, CreateDecoderGIF, (void *)CreateEncoderGIF, 0, 0 },
	{ { "jpg", "jpeg", "jpe", 0 }, CreateDecoderJPEG, (void *)CreateEncoderJPEG, 0, 1 },
	{ { "png", 0, 0, 0 }, CreateDecoderPNG, (void *)CreateEncoderPNG, 0, 0 },
	{ { "tif", "tiff", 0, 0 }, CreateDecoderTIFF, (void *)CreateEncoderTIFF, 0, 0 },
	{ { "mng", 0, 0, 0 }, CreateDecoderMNG, 0, 1, 0 },
	{ { 0, 0, 0, 0 }, 0, 0, 0, 0 },
};

/* Bits of the constructor's `flags` argument. */
const int theImageDecoder::fill_alpha = 1;
const int theImageDecoder::keep_aspect_rate = 2;
const int theImageDecoder::animation = 4;
const int theImageDecoder::format_check = 8;

/* Decodes the file at path and hands the result to cb, on success as on
   failure. */
theImageDecoder::theImageDecoder(Callback cb, const char *path, int width, int height, pixel_format type, int alpha,
    int frame, unsigned int flags, _ImgDecStatus *status)
        : m_cb(cb), m_count(0), m_ptype(PF_AUTO), m_images(0), m_cluts(0), m_delays(0), m_exifData(0)
{
	/* Per pixel_format: ImageRGBA exchange format and bytes per pixel. */
	static const struct PixelFormatInfo {
		int exchangeType;
		int bytesPerPixel;
	} pixelFormat[5] = {
		{ ImageRGBA::RGB888, 3 },   /* PF_AUTO      */
		{ ImageRGBA::ARGB1555, 2 },   /* PF_ARGB1555  */
		{ ImageRGBA::RGB888, 3 },   /* PF_RGB888    */
		{ ImageRGBA::RGBA8888, 4 },   /* PF_RGBA8888  */
		{ ImageRGBA::INDEX8, 1 }    /* PF_INDEX8    */
	};
	int srcWidth;
	int srcHeight;
	InputIoFileFilter *file;
	InputDataFilter *decoder;
	PgProps *props;
	int outWidth;
	int outHeight;
	ImageRGBA **images;
	int decWidth, decHeight;
	int rawWidth, rawHeight;
	int srcType;
	ImageInfo::FormatInfo *fi;
	decoded_result result;
	BOOL known;
	BOOL hasDecoder;
	BOOL isPng;
	BOOL isJpeg;
	BOOL invalid;
	int flag;
	BOOL fits;
	int rc;
	int pt;
	int i;

	file = 0;
	decoder = 0;
	props = 0;
	srcWidth = 0;
	srcHeight = 0;
	outWidth = -1;
	outHeight = -1;
	images = 0;

	if (type < 0) {
		result = RESULT_BAD_FORMAT;
		goto error;
	}
	invalid = (type >= 5);
	if (invalid) {
		result = RESULT_BAD_FORMAT;
		goto error;
	}

	file = new InputIoFileFilter();
	if (file == 0) {
		result = RESULT_NO_MEMORY;
		goto error;
	}
	if (!file->Open(path)) {
		result = RESULT_CANT_OPEN;
		goto error;
	}

	fi = ImageInfo::SearchFormatInfo(path);
	if (fi == 0 || fi->createDecoder == 0)
		hasDecoder = FALSE;
	else
		hasDecoder = TRUE;
	if (hasDecoder == FALSE) {
		result = RESULT_UNSUPPORTED;
		goto error;
	}

	fi = ImageInfo::SearchFormatInfo(path);
	if (fi == 0)
		decoder = 0;
	else
		decoder = fi->createDecoder(file);

	if (flags & format_check) {
		if (decoder->IsSupported() == FALSE) {
			result = RESULT_UNSUPPORTED;
			goto error;
		}
	}

	fi = ImageInfo::SearchFormatInfo(path);
	if (fi == 0
	    || (fi->createDecoder != ImageInfo::CreateDecoderBMP && fi->createDecoder != ImageInfo::CreateDecoderGIF
	        && fi->createDecoder != ImageInfo::CreateDecoderJPEG && fi->createDecoder != ImageInfo::CreateDecoderPNG
	        && fi->createDecoder != ImageInfo::CreateDecoderTIFF
	        && fi->createDecoder != ImageInfo::CreateDecoderMNG))
		known = FALSE;
	else
		known = TRUE;
	if (known == TRUE) {
		decWidth = width;
		decHeight = height;
		fi = ImageInfo::SearchFormatInfo(path);
		flag = 0;
		if (fi)
			flag = fi->flag18;
		if (flag) {
			frame = (flags & animation) ? -1 : frame;
			rc = decoder->GetRawData(&decWidth, &decHeight, type, alpha, flags, m_images, m_cluts, &srcType,
			    &outWidth, &outHeight, status, m_delays, m_count, frame);
			if (m_count == 0) {
				result = RESULT_NO_IMAGE;
				goto error;
			}
			if (rc != 0) {
				result = RESULT_UNSUPPORTED;
				goto error;
			}
		} else {
			m_count = 1;
			m_images = new unsigned char *[m_count];
			m_cluts = new unsigned char *[m_count];
			if (m_images == 0 || m_cluts == 0) {
				result = RESULT_NO_MEMORY;
				goto error;
			}
			m_images[0] = 0;
			m_cluts[0] = 0;
			rc = decoder->GetRawData(&decWidth, &decHeight, type, alpha, flags, m_images, m_cluts, &srcType,
			    &outWidth, &outHeight, status);
		}

		switch (rc) {
		case 0:
			if (m_cluts != 0 && m_cluts[0] == 0) {
				delete[] m_cluts;
				m_cluts = 0;
			}
			srcWidth = decWidth;
			srcHeight = decHeight;
			width = srcWidth;
			height = srcHeight;
			pt = type ? type : srcType;
			goto invoke;
		case EICL_ERR_NO_MEMORY:
			result = RESULT_NO_MEMORY;
			goto error;
		case EICL_ERR_STREAM:
		case EICL_ERR_STREAM + 1:
		case EICL_ERR_STREAM + 2:
		case EICL_ERR_STREAM_WRITE:
			result = RESULT_CANT_OPEN;
			goto error;
		case EICL_ERR_NO_DATA:
		case EICL_ERR_BAD_DATA:
		case EICL_ERR_CODEC:
		case EICL_ERR_CODEC + 1:
		case EICL_ERR_CODEC + 2:
			result = RESULT_DECODE_FAILED;
			goto error;
		case EICL_ERR_BAD_HEADER:
			result = RESULT_UNSUPPORTED;
			goto error;
		case EICL_ERR_FORMAT:
			if (m_images)
				delete[] m_images;
			m_images = 0;
			if (m_cluts)
				delete[] m_cluts;
			m_cluts = 0;
			delete decoder;
			fi = ImageInfo::SearchFormatInfo(path);
			if (fi == 0)
				decoder = 0;
			else
				decoder = fi->createDecoder(file);
			break;
		default:
			result = RESULT_DECODE_FAILED;
			goto error;
		}
	}

	fits = FALSE;
	fi = ImageInfo::SearchFormatInfo(path);
	if (fi == 0 || fi->createDecoder != ImageInfo::CreateDecoderPNG)
		isPng = FALSE;
	else
		isPng = TRUE;
	if (isPng == TRUE) {
		props = new PgProps();
		decoder->GetProps(props);

		EiclSize size;

		props->GetSize(&size);
		srcWidth = size.GetWidth();
		srcHeight = size.GetHeight();
		if (width == 0 || height == 0) {
			width = srcWidth;
			height = srcHeight;
		}
		switch (props->GetBitsPerPixel()) {
		case 8:
			srcType = PF_INDEX8;
			break;
		case 24:
			srcType = PF_RGB888;
			break;
		case 32:
			srcType = PF_RGBA8888;
			break;
		}
		pt = type;
		if (pt == 0)
			pt = srcType;
		if (width == srcWidth && height == srcHeight)
			fits = (pt == srcType);
		if (srcType == PF_INDEX8) {
			if (type == srcType || type == PF_AUTO) {
				pt = PF_INDEX8;
				fits = TRUE;
				width = srcWidth;
				height = srcHeight;
			}
		}
	}

	if (fits) {
		/* Single image: both arrays have one entry. */
		m_count = 1;
		m_images = new unsigned char *[1];
		m_images[0] = new unsigned char[width * height * pixelFormat[pt].bytesPerPixel];
		if (pt == PF_INDEX8) {
			m_cluts = new unsigned char *[1];
			m_cluts[0] = new unsigned char[1024];
		}
		rc = ((InputDataFilterPNG *)decoder)->GetPNGImageClut(m_images, m_cluts);
		if (rc == EICL_ERR_NO_MEMORY) {
			result = RESULT_NO_MEMORY;
			goto error;
		}
		if (rc != 0) {
			result = RESULT_UNSUPPORTED;
			goto error;
		}
		goto finish;
	}

	fi = ImageInfo::SearchFormatInfo(path);
	flag = 0;
	if (fi)
		flag = fi->flag18;
	if (flag) {
		frame = (flags & animation) ? -1 : frame;
		rc = decoder->GetRawData(images, m_delays, m_count, frame, (unsigned char)alpha);
		if (m_count == 0) {
			result = RESULT_NO_IMAGE;
			goto error;
		}
		if (rc != 0) {
			result = RESULT_UNSUPPORTED;
			goto error;
		}
	} else {
		m_count = 1;
		images = new ImageRGBA *[m_count];
		fi = ImageInfo::SearchFormatInfo(path);
		if (fi == 0 || fi->createDecoder != ImageInfo::CreateDecoderJPEG)
			isJpeg = FALSE;
		else
			isJpeg = TRUE;
		if (isJpeg == TRUE) {
			rawWidth = width;
			rawHeight = height;
			rc = ((InputDataFilterJPEG *)decoder)
			         ->GetRawData(&rawWidth, &rawHeight, images, (unsigned char)alpha, &outWidth,
			             &outHeight);
		} else {
			rc = decoder->GetRawData(images, &outWidth, &outHeight, (unsigned char)alpha);
		}
	}
	if (rc == EICL_ERR_NO_MEMORY) {
		result = RESULT_NO_MEMORY;
		goto error;
	}
	if (rc != 0) {
		result = RESULT_UNSUPPORTED;
		goto error;
	}

	fi = ImageInfo::SearchFormatInfo(path);
	flag = 0;
	if (fi)
		flag = fi->flag1c;
	if (flag) {
		m_exifData = new ExifReader(file);
		m_exifData->Parse();
	}

	if (props == 0) {
		props = new PgProps();
		decoder->GetProps(props);
	}
	srcType = PF_RGBA8888;
	if (props->GetColorSpace() == 0 || props->GetColorSpace() == 1) {
		switch (props->GetBitsPerPixel()) {
		case 16:
			srcType = PF_ARGB1555;
			break;
		case 24:
			srcType = PF_RGB888;
			break;
		case 32:
			srcType = PF_RGBA8888;
			break;
		}
	}
	if (images[0]->GetClutBufferPtr())
		srcType = PF_INDEX8;
	srcWidth = images[0]->Width();
	srcHeight = images[0]->Height();
	if (width == 0 || height == 0) {
		width = srcWidth;
		height = srcHeight;
	} else if (flags & keep_aspect_rate) {
		int fitHeight = (int)((float)srcHeight * (float)width / (float)srcWidth);
		int fitWidth = (int)((float)srcWidth * (float)height / (float)srcHeight);

		if (height < fitHeight)
			width = fitWidth;
		else
			height = (width < fitWidth) ? fitHeight : height;
	}

	if (type == PF_RGB888)
		pt = PF_RGB888;
	else if (type == PF_RGBA8888)
		pt = PF_RGBA8888;
	else if (type == PF_ARGB1555)
		pt = PF_ARGB1555;
	else if (type == PF_INDEX8 || type == PF_AUTO) {
		if (images[0]->GetClutBufferPtr()) {
			pt = PF_INDEX8;
			if (width != srcWidth || height != srcHeight)
				pt = PF_RGBA8888;
		} else {
			pt = srcType;
		}
	}

	m_images = new unsigned char *[m_count];
	memset(m_images, 0, m_count * 4);
	if (pt == PF_INDEX8) {
		m_cluts = new unsigned char *[m_count];
		memset(m_cluts, 0, m_count * 4);
	}
	m_ptype = (pixel_format)pt;

	for (i = 0; i < m_count; i++) {
		if (flags & fill_alpha)
			images[i]->FillAlpha((unsigned char)alpha);
		if (width != srcWidth || height != srcHeight) {
			if (images[i]->GetClutBufferPtr()) {
				ImageRGBA *conv =
				    new ImageRGBA(srcWidth, srcHeight, (unsigned char)alpha, ImageRGBA::RGBA8888);
				conv->SetImage(images[i]->GetImageBufferPtr(), ImageRGBA::INDEX8,
				    images[i]->GetClutBufferPtr());
				delete images[i];
				images[i] = conv;
			}
			ImgOpResize::Operate(images[i], width, height, RESIZE_DEFAULT);
		}
		if (pt == PF_INDEX8) {
			m_images[i] = new unsigned char[width * height * pixelFormat[pt].bytesPerPixel];
			m_cluts[i] = new unsigned char[1024];
			images[i]->GetImage(m_images[i], (ImageRGBA::pixel_type)pixelFormat[pt].exchangeType);
			images[i]->GetClut(m_cluts[i]);
			delete images[i];
			images[i] = 0;
		} else {
			m_images[i] = new unsigned char[width * height * pixelFormat[pt].bytesPerPixel];
			images[i]->GetImage(m_images[i], (ImageRGBA::pixel_type)pixelFormat[pt].exchangeType);
			delete images[i];
			images[i] = 0;
		}
	}

finish:
	if (outWidth != -1 && outHeight != -1) {
		srcWidth = outWidth;
		srcHeight = outHeight;
	}
invoke:
	cb(RESULT_OK, 0, m_count, (pixel_format)pt, (void **)m_images, (void **)m_cluts, width, height, m_delays,
	    srcWidth, srcHeight, (pixel_format)srcType, status->userData);
	goto cleanup;

error:
	cb(result, 0, 0, PF_AUTO, 0, 0, 0, 0, 0, 0, 0, PF_AUTO, status->userData);
	if (m_exifData) {
		delete m_exifData;
		m_exifData = 0;
	}
	if (m_images) {
		for (fits = 0; fits < m_count; fits++) {  /* redundant as written */
			if (m_images[fits]) {
				delete[] m_images[fits];
				m_images[fits] = 0;
			}
		}
	}
	if (m_cluts) {
		for (fits = 0; fits < m_count; fits++) {  /* redundant as written */
			if (m_cluts[fits]) {
				delete[] m_cluts[fits];
				m_cluts[fits] = 0;
			}
		}
	}

cleanup:
	delete props;
	if (images) {
		delete[] images;
		images = 0;
	}
	delete decoder;
	delete file;
}

theImageDecoder::~theImageDecoder()
{
	int i;

	for (i = 0; i < m_count; i++) {
		if (m_images) {
			if (m_images[i]) {
				delete[] m_images[i];
				m_images[i] = 0;
			}
		}
		if (m_ptype == PF_INDEX8) {
			if (m_cluts) {
				if (m_cluts[i]) {
					delete[] m_cluts[i];
					m_cluts[i] = 0;
				}
			}
		}
	}
	if (m_images) {
		delete[] m_images;
		m_images = 0;
	}
	if (m_ptype == PF_INDEX8) {
		if (m_cluts) {
			delete[] m_cluts;
			m_cluts = 0;
		}
	}
	if (m_delays) {
		delete[] m_delays;
		m_delays = 0;
	}
	if (m_exifData) {
		delete m_exifData;
		m_exifData = 0;
	}
}

const char *theImageDecoder::getResultString(decoded_result result, int idx)
{
	static const char *const resultStrings[] = { "Success", "ERROR: No enough memory", "ERROR: Unsupported format",
		"ERROR: Can't open file", "ERROR: Not found page", "ERROR: Unsupported pixelformat", "ERROR: Unknown" };
	return resultStrings[result];
}

bool theImageDecoder::isValidExifTags() const
{
	return m_exifData != 0;
}

int theImageDecoder::GetExifUShort(int tag) const
{
	if (!isValidExifTags())
		return 0;
	const ExifReader::TagData *entry = m_exifData->SearchTag((Exif::tags)tag);
	if (!entry)
		return 0;
	return entry->GetValue() & 0xffff;
}

float theImageDecoder::GetExifRational(int tag) const
{
	if (!isValidExifTags())
		return 0.0f;
	const ExifReader::TagData *entry = m_exifData->SearchTag((Exif::tags)tag);
	if (!entry)
		return 0.0f;
	return entry->GetRational();
}

const char *theImageDecoder::GetExifString(int tag) const
{
	if (!isValidExifTags())
		return 0;
	const ExifReader::TagData *entry = m_exifData->SearchTag((Exif::tags)tag);
	if (!entry)
		return 0;
	return entry->GetString();
}

const char *theImageDecoder::getMake() const
{
	return GetExifString(271);
}

const char *theImageDecoder::getModel() const
{
	return GetExifString(272);
}

int theImageDecoder::getOrientation() const
{
	return GetExifUShort(274);
}

const char *theImageDecoder::getDateTime() const
{
	return GetExifString(306);
}

float theImageDecoder::getFnumber() const
{
	return GetExifRational(0x829d);
}

int theImageDecoder::getISOSpeedRatings() const
{
	return GetExifUShort(0x8827);
}

const char *theImageDecoder::getDateTimeOriginal() const
{
	return GetExifString(0x9003);
}

const char *theImageDecoder::getDateTimeDigitized() const
{
	return GetExifString(0x9004);
}

float theImageDecoder::getShutterSpeedValue() const
{
	return GetExifRational(0x9201);
}

float theImageDecoder::getExposureBiasValue() const
{
	return GetExifRational(0x9204);
}

int theImageDecoder::getLightSource() const
{
	return GetExifUShort(0x9208);
}

int theImageDecoder::getFlash() const
{
	return GetExifUShort(0x9209);
}

int theImageDecoder::getFocalLength() const
{
	return GetExifUShort(0x920a);
}

typedef OutputDataFilter *(*CreateEncoderFn)(OutputIoFilter *filter);

int theImageEncoder::encode(const void *data, pixel_format format, unsigned int width, unsigned int height,
    const char *name)
{
	/* Encoder pixel_format -> ImageRGBA::pixel_type */
	static const ImageRGBA::pixel_type fmtMap[3] = { ImageRGBA::RGBA8888, ImageRGBA::RGB888, ImageRGBA::ARGB1555 };
	int result;
	OutputIoFileFilter *file = 0;
	OutputDataFilter *enc = 0;
	ImageRGBA *img = 0;
	ImageInfo::FormatInfo *info;

	int ok;

	info = ImageInfo::SearchFormatInfo(name);
	if (info == 0 || info->createEncoder == 0)
		ok = 0;
	else
		ok = 1;
	if (ok == 0) {
		result = 2;
	} else {
		img = new ImageRGBA(width, height, 255, ImageRGBA::RGBA8888);
		if (img == 0) {
			result = 1;
		} else {
			img->SetImage(data, fmtMap[format], 0);
			file = new OutputIoFileFilter();
			if (file->Open(name) == 0) {
				result = 3;
			} else {
				int ret;

				info = ImageInfo::SearchFormatInfo(name);
				if (info == 0)
					enc = 0;
				else
					enc = ((CreateEncoderFn)info->createEncoder)(file);
				ret = enc->SetRawData(img);
				if (ret == EICL_ERR_STREAM)
					result = 3;
				else
					result = (ret == EICL_ERR_NO_MEMORY);
			}
		}
	}
	delete file;
	delete enc;
	delete img;
	return result;
}

const char *theImageEncoder::getResultString(encoded_result result, int idx)
{
	static const char *const resultStrings[] = { "Success", "ERROR: No enough memory", "ERROR: Unsupported format",
		"ERROR: Can't open file" };
	return resultStrings[result];
}

inline ImageInfo::FormatInfo *ImageInfo::SearchFormatInfo(const char *filename)
{
	const char *ext = strrchr(filename, '.');
	if (ext) {
		ext++;
		for (FormatInfo *fi = SupportFormats; fi->ext[0] != 0; fi++) {
			for (int j = 0; fi->ext[j] != 0 && j < 4; j++) {
				if (strcasecmp(ext, fi->ext[j]) == 0)
					return fi;
			}
		}
	}
	return 0;
}

inline InputDataFilter *ImageInfo::CreateDecoderBMP(InputIoFilter *filter)
{
	return new InputDataFilterBMP(filter);
}

inline InputDataFilter *ImageInfo::CreateDecoderGIF(InputIoFilter *filter)
{
	return new InputDataFilterGIF(filter);
}

inline InputDataFilter *ImageInfo::CreateDecoderJPEG(InputIoFilter *filter)
{
	return new InputDataFilterJPEG(filter);
}

inline InputDataFilter *ImageInfo::CreateDecoderPNG(InputIoFilter *filter)
{
	return new InputDataFilterPNG(filter);
}

inline InputDataFilter *ImageInfo::CreateDecoderTIFF(InputIoFilter *filter)
{
	return new InputDataFilterTIFF(filter);
}

inline InputDataFilter *ImageInfo::CreateDecoderMNG(InputIoFilter *filter)
{
	return new InputDataFilterMNG(filter);
}

inline OutputDataFilter *ImageInfo::CreateEncoderBMP(OutputIoFilter *filter)
{
	return new OutputDataFilterBMP(filter);
}

inline OutputDataFilter *ImageInfo::CreateEncoderGIF(OutputIoFilter *filter)
{
	return new OutputDataFilterGIF(filter);
}

inline OutputDataFilter *ImageInfo::CreateEncoderJPEG(OutputIoFilter *filter)
{
	return new OutputDataFilterJPEG(filter);
}

inline OutputDataFilter *ImageInfo::CreateEncoderPNG(OutputIoFilter *filter)
{
	return new OutputDataFilterPNG(filter);
}

inline OutputDataFilter *ImageInfo::CreateEncoderTIFF(OutputIoFilter *filter)
{
	return new OutputDataFilterTIFF(filter);
}
