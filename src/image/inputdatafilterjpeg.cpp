/*
 * JPEG decoder (InputDataFilterJPEG) on top of libjpeg, with a custom source
 * manager reading through InputIoFilter and a hand-written marker scanner.
 */

#include "pgprops.h"
#include "eiclerror.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>
extern "C" {
#include "jpeglib.h"
#include "jerror.h"
}

#include "imagergba.h"
#include "rect.h"

#include "inputdatafilterjpeg.h"
#include "iofilter.h"
#include "rawimggenerator.h"
#include "theimagedecoder.h"

/* The libjpeg source manager (as in jdatasrc.c), reading through
   owner->m_ioFilter; at end of stream it inserts a fake EOI marker. */
class InputDataFilterJPEG::PrivateJPEGReader {
public:
	/* 0x00 */ jpeg_source_mgr pub;
	/* 0x1C */ InputDataFilterJPEG *owner;
	/* 0x20 */ unsigned char *buffer;
	/* 0x24 */ boolean start_of_file;

	static void init_source(j_decompress_ptr cinfo)
	{
		PrivateJPEGReader *src = (PrivateJPEGReader *)cinfo->src;
		src->start_of_file = TRUE;
	}

	static boolean fill_input_buffer(j_decompress_ptr cinfo)
	{
		PrivateJPEGReader *src = (PrivateJPEGReader *)cinfo->src;
		int nbytes = src->owner->m_ioFilter->Read(4096, src->buffer);
		if (nbytes == 0) {
			src->buffer[0] = (unsigned char)0xFF;
			src->buffer[1] = (unsigned char)0xD9;
			nbytes = 2;
		}
		src->pub.bytes_in_buffer = nbytes;
		src->pub.next_input_byte = src->buffer;
		src->start_of_file = FALSE;
		return TRUE;
	}

	static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
	{
		PrivateJPEGReader *src = (PrivateJPEGReader *)cinfo->src;

		if (num_bytes > 0) {
			while (num_bytes > (long)src->pub.bytes_in_buffer) {
				num_bytes -= (long)src->pub.bytes_in_buffer;
				(void)fill_input_buffer(cinfo);
			}
			src->pub.next_input_byte += (size_t)num_bytes;
			src->pub.bytes_in_buffer -= (size_t)num_bytes;
		}
	}

	static void term_source(j_decompress_ptr cinfo) {}
};

InputDataFilterJPEG::InputDataFilterJPEG() : InputDataFilter()
{
	m_ioFilter = 0;
	m_useExifTk = 1;
	m_useExifTag = 0;
	m_srcIsCorrect = 0;
	InputDataFilterJPEG **selfBox = new InputDataFilterJPEG *;
	*selfBox = this;
	m_self = selfBox;
	return;  /* redundant as written */
}

InputDataFilterJPEG::InputDataFilterJPEG(InputIoFilter *ioFilter) : InputDataFilter()
{
	m_ioFilter = ioFilter;
	m_useExifTk = 1;
	m_useExifTag = 0;
	m_srcIsCorrect = 0;
	InputDataFilterJPEG **selfBox = new InputDataFilterJPEG *;
	*selfBox = this;
	m_self = selfBox;
	return;  /* redundant as written */
}

InputDataFilterJPEG::~InputDataFilterJPEG()
{
	delete m_self;
}

/* Checks the file name extension (".jpg" or ".jpeg", case-insensitive), then
   unless m_srcIsCorrect is set reads the header through GetProps with EXIF
   tags disabled, and finally probes the stream with CheckFileFormat. */
int InputDataFilterJPEG::IsSupported()
{
	int ok;
	const char *fileName;

	if (m_ioFilter
	    && (fileName = m_ioFilter->GetRealFileName(),
	        strcasecmp(fileName + strlen(fileName) - 4, ".jpg") == 0
	            || strcasecmp(fileName + strlen(fileName) - 5, ".jpeg") == 0)) {
		ok = 1;
		PgProps localProps;
		int savedUseExifTag = m_useExifTag;
		m_useExifTag = 0;
		if (m_srcIsCorrect == 0)
			ok = GetProps(&localProps);
		m_useExifTag = savedUseExifTag;
	} else
		ok = 0;
	if (ok == 1) {
		unsigned long width, height;
		if (CheckFileFormat(0, 0, 0, &width, &height) < 0)
			ok = 0;
	}
	return ok;
}

/* Installs the InputIoFilter source manager on cinfo (cf. jpeg_stdio_src). */
static inline void JpegInputIoFilterSrc(j_decompress_ptr cinfo, InputDataFilterJPEG **self)
{
	InputDataFilterJPEG::PrivateJPEGReader *src;

	if (cinfo->src == NULL) {
		cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
		    sizeof(InputDataFilterJPEG::PrivateJPEGReader));
		src = (InputDataFilterJPEG::PrivateJPEGReader *)cinfo->src;
		src->buffer = (unsigned char *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, 4096);
	}

	(*self)->m_ioFilter->Seek(0);

	src = (InputDataFilterJPEG::PrivateJPEGReader *)cinfo->src;
	src->pub.init_source = InputDataFilterJPEG::PrivateJPEGReader::init_source;
	src->pub.fill_input_buffer = InputDataFilterJPEG::PrivateJPEGReader::fill_input_buffer;
	src->pub.skip_input_data = InputDataFilterJPEG::PrivateJPEGReader::skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart;
	src->pub.term_source = InputDataFilterJPEG::PrivateJPEGReader::term_source;
	InputDataFilterJPEG *ownerPtr = *self;
	src->pub.bytes_in_buffer = 0;
	src->pub.next_input_byte = 0;
	src->owner = ownerPtr;
}

/* JFIF densities are unsigned 16-bit values. */
static inline double JpegDensityValue(short v)
{
	double d = (double)v;
	if (v < 0)
		d += 65536.0;
	return d;
}

/* libjpeg error manager that longjmps back to the decoder (as in example.c). */
struct JpegErrorMgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

static void JpegErrorExit(j_common_ptr cinfo)
{
	JpegErrorMgr *myerr = (JpegErrorMgr *)cinfo->err;
	(*cinfo->err->output_message)(cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

/* Raw-buffer variant: decodes into a buffer allocated here and returned in
   *images. dstWidth, dstHeight: requested size (0 = image size); format: pixel
   format (0 = automatic, 2); alpha: alpha for format 3; flags: unused; cluts:
   palette (always 0); srcFormat: format produced; srcWidth, srcHeight: image
   size; status: _ImgDecStatus.
   Returns EICL_ERR_STREAM (no stream), EICL_ERR_BAD_DATA (libjpeg error or cancelled), EICL_ERR_NO_MEMORY (out of
   memory) or EICL_ERR_FORMAT (unsupported format). */
int InputDataFilterJPEG::GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
    unsigned char **images, unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status)
{
	static const int kJpegOutColorCode[5] = { 0, 7, 0, 3, 8 };  /* ImageRGBA pixel type by output_components */
	static const int kJpegPixelType[5] = { 2, 4, 1, 2,
		3 };  /* format code for *srcFormat: 2 RGB, 3 RGBA, 4 indexed */
	struct jpeg_decompress_struct cinfo;
	JpegErrorMgr jerr;
	unsigned char *buffer = 0;
	int ret = 0;

	(void)flags;

	if (!m_ioFilter)
		goto noIoFilter;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = JpegErrorExit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		if (buffer)
			delete[] buffer;
		*images = 0;
		if (ret != EICL_ERR_FORMAT)
			ret = EICL_ERR_BAD_DATA;
		return ret;
	}
	jpeg_CreateDecompress(&cinfo, JPEG_LIB_VERSION, (size_t)sizeof(struct jpeg_decompress_struct));

	JpegInputIoFilterSrc(&cinfo, m_self);

	if (jpeg_read_header(&cinfo, TRUE) != 1)
		goto headerFail;
	cinfo.dct_method = (J_DCT_METHOD)0;
	jpeg_start_decompress(&cinfo);
	*srcWidth = cinfo.image_width;
	*srcHeight = cinfo.image_height;

	if (kJpegOutColorCode[cinfo.output_components] != 3)
		goto badColorSpace;
	if (format == 0)
		format = 2;
	if (format != 2 && format != 3)
		goto badPixelType;
	if (*dstWidth == 0 || *dstHeight == 0) {
		*dstWidth = cinfo.output_width;
		*dstHeight = cinfo.output_height;
	}
	if (*dstWidth != (int)cinfo.output_width)
		goto badSize;
	if (*dstHeight != (int)cinfo.output_height)
		goto badSize;
	if (format == 2)
		buffer = new unsigned char[*dstWidth * *dstHeight * 3];
	else
		buffer = new unsigned char[*dstWidth * *dstHeight * 4];
	if (buffer == 0)
		goto allocFail;
	{
		JSAMPARRAY rows;
		unsigned char *dst;
		int row;

		rows = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE,
		    cinfo.output_width * cinfo.output_components, cinfo.output_height);
		dst = buffer;
		ret = 0;
		row = 0;
		while (cinfo.output_scanline < cinfo.output_height) {
			if (((_ImgDecStatus *)status)->cancel == 1) {
				ret = EICL_ERR_BAD_DATA;
				break;
			}
			jpeg_read_scanlines(&cinfo, rows, 1);
			if (format == 2) {
				memcpy(dst, rows[0], cinfo.output_width * 3);
				dst += cinfo.output_width * 3;
			} else {
				unsigned char *src = rows[0];
				unsigned int x;
				for (x = 0; x < cinfo.output_width; x++) {
					*dst++ = *src++;
					*dst++ = *src++;
					*dst++ = *src++;
					*dst++ = (unsigned char)alpha;
				}
			}
			((_ImgDecStatus *)status)->percent = (row + 1) * 100 / cinfo.output_height;
			row++;
		}
	}
	if (ret == 0)
		*images = buffer;
	else
		*images = 0;
	*cluts = 0;
	*srcFormat = kJpegPixelType[cinfo.output_components];
	goto finish;
allocFail:
	ret = EICL_ERR_NO_MEMORY;
	goto finish;
badSize:
	ret = EICL_ERR_FORMAT;
	goto finish;
badPixelType:
	ret = EICL_ERR_FORMAT;
	goto finish;
badColorSpace:
	ret = EICL_ERR_FORMAT;
finish:
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	goto out;
headerFail:
	ret = EICL_ERR_BAD_DATA;
	goto out;
noIoFilter:
	ret = EICL_ERR_STREAM;
out:
	if (ret != 0 && buffer)
		delete[] buffer;
	return ret;
}

/* Like GetRawData(ImageRGBA **, ...) below, but rawWidth and rawHeight are
   in/out: when both are nonzero they are the requested size and libjpeg DCT
   scaling picks a scale_denom of 8, 4, 2 or 1; otherwise they receive the
   image size. outWidth and outHeight optionally receive the image size. */
int InputDataFilterJPEG::GetRawData(int *rawWidth, int *rawHeight, ImageRGBA **images, unsigned char alpha,
    int *outWidth, int *outHeight)
{
	static const int kJpegOutColorCode[5] = { 0, 7, 0, 3, 8 };  /* ImageRGBA pixel type by output_components */
	struct jpeg_decompress_struct cinfo;
	JpegErrorMgr jerr;
	unsigned char *buffer;
	int ret;

	if (!m_ioFilter)
		goto noIoFilter;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = JpegErrorExit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		return EICL_ERR_BAD_DATA;
	}
	jpeg_CreateDecompress(&cinfo, JPEG_LIB_VERSION, (size_t)sizeof(struct jpeg_decompress_struct));

	JpegInputIoFilterSrc(&cinfo, m_self);

	if (jpeg_read_header(&cinfo, TRUE) != 1) {
		ret = EICL_ERR_BAD_DATA;
		goto out;
	}
	cinfo.dct_method = (J_DCT_METHOD)0;
	if (outWidth)
		*outWidth = cinfo.image_width;
	if (outHeight)
		*outHeight = cinfo.image_height;
	if (*rawWidth == 0 || *rawHeight == 0) {
		*rawWidth = cinfo.image_width;
		*rawHeight = cinfo.image_height;
	} else {
		cinfo.scale_num = 1;
		if (cinfo.image_width >= (unsigned int)(*rawWidth * 8)
		    && cinfo.image_height >= (unsigned int)(*rawHeight * 8))
			cinfo.scale_denom = 8;
		else if (cinfo.image_width >= (unsigned int)(*rawWidth * 4)
		    && cinfo.image_height >= (unsigned int)(*rawHeight * 4))
			cinfo.scale_denom = 4;
		else if (cinfo.image_width >= (unsigned int)(*rawWidth * 2)
		    && cinfo.image_height >= (unsigned int)(*rawHeight * 2))
			cinfo.scale_denom = 2;
		else
			cinfo.scale_denom = 1;
		jpeg_calc_output_dimensions(&cinfo);
	}
	jpeg_start_decompress(&cinfo);
	buffer = new unsigned char[cinfo.output_width * 3 * cinfo.output_height];
	if (buffer == 0)
		goto allocFail;
	{
		JSAMPARRAY rows;

		rows = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE,
		    cinfo.output_width * cinfo.output_components, 1);
		if (m_callback) {
			tagRECT rect;
			int row;

			rect.left = 0;
			rect.right = cinfo.output_width;
			for (row = 0; cinfo.output_scanline < cinfo.output_height; row++) {
				jpeg_read_scanlines(&cinfo, rows, 1);
				memcpy(buffer + row * cinfo.output_width * cinfo.output_components, rows[0],
				    cinfo.output_width * cinfo.output_components);
				rect.bottom = row;
				rect.top = row;
			}
		} else {
			int row;

			for (row = 0; cinfo.output_scanline < cinfo.output_height; row++) {
				jpeg_read_scanlines(&cinfo, rows, 1);
				memcpy(buffer + row * cinfo.output_width * cinfo.output_components, rows[0],
				    cinfo.output_width * cinfo.output_components);
			}
		}
		ret = 0;
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	*images = new ImageRGBA(cinfo.output_width, cinfo.output_height, alpha, ImageRGBA::RGBA8888);
	if (*images == 0)
		ret = EICL_ERR_NO_MEMORY;
	else if ((*images)->GetImageBufferPtr() != 0)
		(*images)->SetImage(buffer, (ImageRGBA::pixel_type)kJpegOutColorCode[cinfo.output_components], 0);
	else
		ret = EICL_ERR_NO_MEMORY;
	if (buffer)
		delete[] buffer;
	return ret;
allocFail:
	ret = EICL_ERR_NO_MEMORY;
	goto out;
noIoFilter:
	ret = EICL_ERR_STREAM;
out:
	return ret;
}

/* Decodes into a flat RGB buffer, then copies it into a new ImageRGBA.
   outWidth, outHeight receive the image size; alpha is the default alpha. Returns EICL_ERR_STREAM (no
   stream), EICL_ERR_BAD_DATA (libjpeg error or bad header), EICL_ERR_NO_MEMORY (out of memory), 0 on
   success. The m_callback branch fills a rect that is never used. */
int InputDataFilterJPEG::GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha)
{
	static const int kJpegOutColorCode[5] = { 0, 7, 0, 3, 8 };  /* ImageRGBA pixel type by output_components */
	struct jpeg_decompress_struct cinfo;
	JpegErrorMgr jerr;
	unsigned char *buffer;
	int ret;

	if (!m_ioFilter)
		goto noIoFilter;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = JpegErrorExit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		return EICL_ERR_BAD_DATA;
	}
	jpeg_CreateDecompress(&cinfo, JPEG_LIB_VERSION, (size_t)sizeof(struct jpeg_decompress_struct));

	JpegInputIoFilterSrc(&cinfo, m_self);

	if (jpeg_read_header(&cinfo, TRUE) != 1) {
		ret = EICL_ERR_BAD_DATA;
		goto out;
	}
	cinfo.dct_method = (J_DCT_METHOD)0;
	jpeg_start_decompress(&cinfo);
	*outWidth = cinfo.image_width;
	*outHeight = cinfo.image_height;
	buffer = new unsigned char[cinfo.output_width * 3 * cinfo.output_height];
	if (buffer == 0)
		goto allocFail;
	{
		JSAMPARRAY rows;

		rows = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE,
		    cinfo.output_width * cinfo.output_components, 1);
		if (m_callback) {
			tagRECT rect;
			int row;

			rect.left = 0;
			rect.right = cinfo.output_width;
			for (row = 0; cinfo.output_scanline < cinfo.output_height; row++) {
				jpeg_read_scanlines(&cinfo, rows, 1);
				memcpy(buffer + row * cinfo.output_width * cinfo.output_components, rows[0],
				    cinfo.output_width * cinfo.output_components);
				rect.bottom = row;
				rect.top = row;
			}
		} else {
			int row;

			for (row = 0; cinfo.output_scanline < cinfo.output_height; row++) {
				jpeg_read_scanlines(&cinfo, rows, 1);
				memcpy(buffer + row * cinfo.output_width * cinfo.output_components, rows[0],
				    cinfo.output_width * cinfo.output_components);
			}
		}
		ret = 0;
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	*out = new ImageRGBA(cinfo.output_width, cinfo.output_height, alpha, ImageRGBA::RGBA8888);
	if (*out == 0)
		ret = EICL_ERR_NO_MEMORY;
	else if ((*out)->GetImageBufferPtr() != 0)
		(*out)->SetImage(buffer, (ImageRGBA::pixel_type)kJpegOutColorCode[cinfo.output_components], 0);
	else
		ret = EICL_ERR_NO_MEMORY;
	if (buffer)
		delete[] buffer;
	return ret;
allocFail:
	ret = EICL_ERR_NO_MEMORY;
	goto out;
noIoFilter:
	ret = EICL_ERR_STREAM;
out:
	return ret;
}

unsigned int InputDataFilterJPEG::CalcScaleDenom(unsigned int origWidth, unsigned int origHeight, unsigned int reqWidth,
    unsigned int reqHeight)
{
	unsigned int denom;
	if (reqWidth != 0 && reqHeight != 0) {
		unsigned int wratio = origWidth / reqWidth;
		unsigned int hratio = origHeight / reqHeight;
		denom = (wratio < hratio) ? wratio : hratio;
		if (denom >= 8) {
			denom = 8;
		} else if (denom >= 4) {
			denom = 4;
		} else if (denom >= 2) {
			denom = 2;
		} else {
			denom = 1;
		}
	} else {
		denom = 1;
	}
	return denom;
}

/* Decodes with DCT scaling (CalcScaleDenom) into a RawImg created by
   RawImgGenerator, reporting each row through CallCallback when a callback is
   registered. Returns EICL_ERR_STREAM (no stream), EICL_ERR_BAD_DATA (libjpeg error or bad header),
   EICL_ERR_NO_MEMORY (GenerateSizedImage failed), 0 on success. */
int InputDataFilterJPEG::GetRawDataFast(unsigned int width, unsigned int height, RawImg **out)
{
	struct jpeg_decompress_struct cinfo;
	JpegErrorMgr jerr;

	if (!m_ioFilter)
		goto noIoFilter;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = JpegErrorExit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		return EICL_ERR_BAD_DATA;
	}
	jpeg_CreateDecompress(&cinfo, JPEG_LIB_VERSION, (size_t)sizeof(struct jpeg_decompress_struct));

	JpegInputIoFilterSrc(&cinfo, m_self);

	if (jpeg_read_header(&cinfo, TRUE) != 1)
		return EICL_ERR_BAD_DATA;

	cinfo.dct_method = JDCT_IFAST;
	cinfo.do_fancy_upsampling = FALSE;
	cinfo.do_block_smoothing = FALSE;
	cinfo.rec_outbuf_height = 4;
	cinfo.quantize_colors = FALSE;
	cinfo.dither_mode = JDITHER_NONE;
	cinfo.scale_denom = CalcScaleDenom(cinfo.image_width, cinfo.image_height, width, height);
	/* NOTE: bug: divides by scale_num instead of the scale_denom just computed. */
	cinfo.output_width = cinfo.image_width / cinfo.scale_num;
	cinfo.output_height = cinfo.image_height / cinfo.scale_num;
	jpeg_start_decompress(&cinfo);

	*out = RawImgGenerator::GenerateSizedImage(RawImg::TYPE_DIB, cinfo.output_width, cinfo.output_height);
	if (*out == 0)
		goto allocFail;
	{
		JSAMPARRAY rows;

		rows = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE,
		    cinfo.output_width * cinfo.output_components, 1);
		if (m_callback) {
			tagRECT rect;
			int row;

			rect.left = 0;
			rect.right = cinfo.output_width;
			for (row = 0; cinfo.output_scanline < cinfo.output_height; row++) {
				jpeg_read_scanlines(&cinfo, rows, 1);
				(*out)->SetLine(row, rows[0], cinfo.output_components);
				rect.bottom = row;
				rect.top = row;
				CallCallback(row * 100 / cinfo.output_height, (*out)->GetDIB(), &rect);
			}
		} else {
			int row;

			for (row = 0; cinfo.output_scanline < cinfo.output_height; row++) {
				jpeg_read_scanlines(&cinfo, rows, 1);
				(*out)->SetLine(row, rows[0], cinfo.output_components);
			}
		}
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	return 0;
allocFail:
	return EICL_ERR_NO_MEMORY;
noIoFilter:
	return EICL_ERR_STREAM;
}

/* Opens libjpeg on m_ioFilter and fills props from the header. Color spaces
   other than grayscale, RGB and CMYK are rejected. Resolution: density_unit 1
   is dpi, 2 is dots per cm (converted and truncated), otherwise 72 dpi. */
int InputDataFilterJPEG::GetProps(PgProps *props)
{
	struct jpeg_decompress_struct cinfo;
	JpegErrorMgr jerr;
	int ret = 1;

	if (!m_ioFilter)
		goto noIoFilter;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = JpegErrorExit;
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		return EICL_ERR_BAD_DATA;
	}
	jpeg_CreateDecompress(&cinfo, JPEG_LIB_VERSION, (size_t)sizeof(struct jpeg_decompress_struct));

	JpegInputIoFilterSrc(&cinfo, m_self);

	if (jpeg_read_header(&cinfo, TRUE) != 1)
		goto headerFail;

	props->SetFormat((PgProps::PgFormat)1024);
	props->SetImageType(PgProps::kImageTypeDefault);
	props->SetSize(cinfo.image_width, cinfo.image_height);

	switch (cinfo.out_color_space) {
	case JCS_GRAYSCALE:
		props->SetColorSpace(PgProps::kGrayscale);
		break;
	case JCS_RGB:
		props->SetColorSpace(PgProps::kRGB);
		break;
	case JCS_CMYK:
		props->SetColorSpace(PgProps::kCMYK);
		break;
	case JCS_UNKNOWN:
	case JCS_YCbCr:
	case JCS_YCCK:
	default:
		ret = 0;
		break;
	}
	if (ret != 1)
		goto destroy;

	props->SetBitsPerPixel(cinfo.num_components * 8);

	if (cinfo.density_unit == 1) {
		props->SetResolution(JpegDensityValue((short)cinfo.X_density),
		    JpegDensityValue((short)cinfo.Y_density));
	} else if (cinfo.density_unit == 2) {
		props->SetResolution((double)(int)(JpegDensityValue((short)cinfo.X_density) * 2.54),
		    (double)(int)(JpegDensityValue((short)cinfo.Y_density) * 2.54));
	} else {
		props->SetResolution(72.0, 72.0);
	}
	ret = 1;
	goto destroy;
headerFail:
	ret = 0;
destroy:
	jpeg_destroy_decompress(&cinfo);
	goto out;
noIoFilter:
	ret = 0;
out:
	return ret;
}

int InputDataFilterJPEG::use_exif_tk(int v)
{
	int old = m_useExifTk;
	m_useExifTk = v;
	return old;
}

int InputDataFilterJPEG::use_exif_tag(int v)
{
	int old = m_useExifTag;
	m_useExifTag = v;
	return old;
}

int InputDataFilterJPEG::src_is_correct(int v)
{
	int old = m_srcIsCorrect;
	m_srcIsCorrect = v;
	return old;
}

/* Hand-written JPEG marker scanner, same contract as
   InputDataFilterPNG::CheckFileFormat. Looks for SOI in the first 129 bytes,
   then walks the segments: APP0 (JFIF), SOF (precision 8, size, components),
   COM, and SOS, after which the entropy-coded data is scanned for the next
   real marker (FF 00 stuffing and RST markers are skipped).
   Returns -1 no stream, -2 EOF before SOI, -3 seek failed, -4 EOF inside a
   segment, -10 no SOI, -11 size outside the limits b/c (0 = no limit),
   -30 non-conforming segment (only when bit 0 of a is set). */
/* Debug trace of the marker scanner, compiled out. */
#define JPEG_CHECK_TRACE 0
#define JPEG_TRACE if (JPEG_CHECK_TRACE) printf

long InputDataFilterJPEG::CheckFileFormat(unsigned long a, unsigned long b, unsigned long c, unsigned long *width,
    unsigned long *height)
{
	unsigned char buf[1024];
	unsigned char m0, m1, l0, l1;
	unsigned long imgWidth = 0, imgHeight = 0;
	long found;
	int i;
	unsigned int pos;
	unsigned int segStart;
	unsigned int len;
	unsigned int ncomp;
	unsigned int n;
	unsigned int max;
	int bad;

	if (!m_ioFilter)
		return -1;

	if (b == 0)
		b = 0xffffffff;
	if (c == 0)
		c = 0xffffffff;

	JPEG_TRACE("-- Find SOI Marker...\n");
	m_ioFilter->Seek(0, 0);

	found = -1;
	for (i = 0; i < 129; i++) {
		pos = m_ioFilter->GetCurrentPosition();
		if (!m_ioFilter->Read(1, &m0))
			goto err2;
		JPEG_TRACE("   $%08lX $%02X\n", pos, m0);
		if (m0 != 255)
			continue;
		if (!m_ioFilter->Read(1, &m1))
			goto err2;
		JPEG_TRACE("   Found Marker ($%02X$%02X) at $%08lX.\n", m0, m1, pos);
		if (m1 == 216) {
			JPEG_TRACE("-- Found SOI Marker ($FFD8) at $%08lX\n", pos);
			found = i;
			break;
		}
		JPEG_TRACE("   ? Skip junk data($%02X) at $%08lX\n", m1, pos + 1);
		if (!m_ioFilter->Seek((unsigned int)-1, 1))
			goto err3;
	}

	if (found == -1)
		goto err10;

	for (;;) {
		pos = m_ioFilter->GetCurrentPosition();
		segStart = pos;
		if (!m_ioFilter->Read(1, &m0))
			break;
		if (m0 != 255)
			continue;
		if (!m_ioFilter->Read(1, &m1))
			goto err4;
haveMarker:
		if (m1 < 192) {
			JPEG_TRACE("-- ! Found Invalid Marker ($%02X%02X) at $%08lX\n", m0, m1, segStart);
			continue;
		}
		if (!m_ioFilter->Read(1, &l0))
			goto err4;
		if (!m_ioFilter->Read(1, &l1))
			goto err4;
		len = (l0 << 8) + l1;
		switch (m1) {
		case 0xE0:
			JPEG_TRACE("-- Found APP0 Marker ($%02X%02X) at $%08lX\n", m0, m1, segStart);
			JPEG_TRACE("   Chunk Size = %d\n", len);
			if (len < 16) {
				JPEG_TRACE("   ! Invalid Chunk Size %d (minimum 16).\n", len);
				if (a & 1)
					goto err30;
				JPEG_TRACE("   ! Skip this chunk.\n");
				break;
			}
			n = len - 2;
			if (n > 1024) {
				max = 1024;
				JPEG_TRACE(
				    "   ? Larger Size of following data, %d (maximum %d). Round chunk size into %d.\n",
				    n, max, max);
				n = max;
			}
			if (!m_ioFilter->Read(n, buf))
				goto err4;
			if (memcmp("JFIF", buf, 5) != 0) {
				JPEG_TRACE("   ? Not JFIF Header (%c%c%c%c). Skip this chunk.\n", buf[0], buf[1],
				    buf[2], buf[3]);
			} else {
				JPEG_TRACE("   Version=%04lX, Unit=%d, Resolution-X=%d, Resolution-Y=%d\n",
				    (buf[5] << 8) + buf[6], buf[7], (buf[8] << 8) + buf[9], (buf[10] << 8) + buf[11]);
				JPEG_TRACE("   ThumbWidth=%d, ThumbHeight=%d\n", buf[12], buf[13]);
			}
			break;
		case 0xE1:
		case 0xE2:
		case 0xE3:
		case 0xE4:
		case 0xE5:
		case 0xE6:
		case 0xE7:
		case 0xE8:
		case 0xE9:
		case 0xEA:
		case 0xEB:
		case 0xEC:
		case 0xED:
		case 0xEE:
		case 0xEF:
			JPEG_TRACE("-- Found APP%d Marker ($%02X%02X) at $%08lX\n", m1 - 0xE0, m0, m1, segStart);
			JPEG_TRACE("   Chunk Size = %d\n", len);
			break;
		case 0xDB:
			JPEG_TRACE("-- Found DQT Marker ($%02X%02X) at $%08lX\n", m0, m1, segStart);
			JPEG_TRACE("   Chunk Size = %d\n", len);
			break;
		case 0xC4:
			JPEG_TRACE("-- Found DHT Marker ($%02X%02X) at $%08lX\n", m0, m1, segStart);
			JPEG_TRACE("   Chunk Size = %d\n", len);
			break;
		case 0xC0:
		case 0xC1:
		case 0xC2:
		case 0xC3:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0xCD:
		case 0xCE:
		case 0xCF:
			JPEG_TRACE("-- Found SOF%d Marker ($%02X%02X) at $%08lX\n", m1 - 0xC0, m0, m1, segStart);
			JPEG_TRACE("   Chunk Size = %d\n", len);
			if (len < 11) {
				JPEG_TRACE("   ! Invalid Chunk Size %d (minimum 11).\n", len);
				if (a & 1)
					goto err30;
				JPEG_TRACE("   ! Skip this chunk.\n");
				break;
			}
			n = len - 2;
			if (n > 1024) {
				max = 1024;
				JPEG_TRACE(
				    "   ! Invalid Size of following data, %d (maximum %d). Round chunk size into %d.\n",
				    n, max, max);
				n = max;
			}
			if (!m_ioFilter->Read(n, buf))
				goto err4;
			if (buf[0] != 8) {
				JPEG_TRACE("   ! Invalid Sample Quality ($%02X).\n", buf[0]);
				if (a & 1)
					goto err30;
				JPEG_TRACE("   ! Skip this chunk.\n");
				break;
			}
			imgHeight = (buf[1] << 8) + buf[2];
			imgWidth = (buf[3] << 8) + buf[4];
			ncomp = buf[5];
			JPEG_TRACE("   Width=%d, Height=%d, NCompo=%d\n", imgWidth, imgHeight, ncomp);
			bad = 0;
			if (imgWidth == 0 || b < imgWidth) {
				JPEG_TRACE("   ! Over Picture Width (%ld).\n", imgWidth);
				bad = 1;
			}
			if (imgHeight == 0 || c < imgHeight) {
				JPEG_TRACE("   ! Over Picture Height (%ld).\n", imgHeight);
				bad = 1;
			}
			if (bad)
				goto err11;
			if (len != ncomp * 3 + 8) {
				JPEG_TRACE("   ! Invalid Chunk Size %d (6+NCompo*3+2=%d).\n", len, ncomp * 3 + 8);
				if (a & 1)
					goto err30;
				JPEG_TRACE("   ! Skip this chunk.\n");
				break;
			}
			i = 0;
			if (ncomp != 0) {
				do {
					JPEG_TRACE("   Compo %d: CompoID=%02lX, SampFactor=%d:%d, Quant=%d\n", i,
					    buf[6 + i * 3], buf[7 + i * 3] >> 4, buf[7 + i * 3] & 15, buf[8 + i * 3]);
				} while (++i < (int)ncomp);
			}
			break;
		case 0xFE:
			JPEG_TRACE("-- Found COM Marker ($%02X%02X) at $%08lX\n", m0, m1, segStart);
			n = len - 2;
			if (n > 1023) {
				max = 1023;
				n = max;
			}
			if (!m_ioFilter->Read(n, buf))
				goto err4;
			buf[1023] = 0;
			JPEG_TRACE("   Comment: [%s]\n", buf);
			break;
		case 0xDD:
			JPEG_TRACE("-- Found DRI Marker ($%02X%02X) at $%08lX\n", m0, m1, segStart);
			break;
		case 0xDA:
			JPEG_TRACE("-- Found SOS Marker ($%02X%02X) at $%08lX\n", m0, m1, segStart);
			if (!m_ioFilter->Read(1, &m0))
				goto err4;
			if (m0 * 2 + 6 != len) {
				JPEG_TRACE("   ! Invalid Following Data (%d).\n", m0);
				if (a & 1)
					goto err30;
				JPEG_TRACE("   ! Skip this chunk.\n");
				break;
			}
			if (!m_ioFilter->Seek(segStart + (len + 2), 0))
				goto err3;
			for (;;) {
				if (!m_ioFilter->Read(1, &m0))
					goto err4;
				if (m0 != 255)
					continue;
				pos = m_ioFilter->GetCurrentPosition() - 1;
				segStart = pos;
				if (!m_ioFilter->Read(1, &m1))
					goto err4;
				if (m1 == 0)
					continue;
				if (m1 == 217) {
					JPEG_TRACE("-- Found EOI Marker ($%02X%02X) at $%08lX\n", m0, m1, segStart);
					segStart = m_ioFilter->GetCurrentPosition();
					goto nextChunk;
				}
				if ((unsigned char)(m1 + 48) >= 8)
					goto haveMarker;
				JPEG_TRACE("-- Found RST%d Marker ($%02X%02X) at $%08lX\n", m1 - 0xD0, m0, m1,
				    segStart);
			}
			JPEG_TRACE("   Data Size is %ld bytes.\n", segStart);
			break;
		default:
			JPEG_TRACE("-- Found Marker ($%02X%02X) at $%08lX\n", m0, m1, segStart);
			JPEG_TRACE("   Skip this chunk.\n");
			break;
		}
nextChunk:
		if (!m_ioFilter->Seek(segStart + (len + 2), 0))
			goto err3;
	}

	JPEG_TRACE("-- EOF at $%08lX\n", segStart);
	*width = imgWidth;
	*height = imgHeight;
	if (!m_ioFilter->Seek(0, 0))
		goto err3;
	return 0;
err2:
	m_ioFilter->Seek(0, 0);
	return -2;
err3:
	return -3;
err4:
	m_ioFilter->Seek(0, 0);
	return -4;
err10:
	m_ioFilter->Seek(0, 0);
	return -10;
err11:
	m_ioFilter->Seek(0, 0);
	return -11;
err30:
	m_ioFilter->Seek(0, 0);
	return -30;
}
