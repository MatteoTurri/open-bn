/*
 * PNG decoder (InputDataFilterPNG) on top of libpng, plus a hand-written
 * chunk scanner (CheckFileFormat) that validates signature, CRCs and IHDR.
 */

#include "inputdatafilterpng.h"
#include "eiclerror.h"
#include "png.h"

#include <string.h>
#include "theimagedecoder.h"

/* 32-bit CRC, the sample code of the PNG specification (Annex D). */
static unsigned long crc_table[256];
static int crc_table_computed = 0;

/* png_rw_ptr for png_set_read_fn: the io pointer is the InputIoFilter. */
static void PngReadDataFn(png_structp png_ptr, png_bytep data, png_size_t length)
{
	InputIoFilter *io = (InputIoFilter *)png_get_io_ptr(png_ptr);
	if (io)
		io->Read(length, data);
}

InputDataFilterPNG::InputDataFilterPNG() : InputDataFilter()
{
	m_ioFilter = 0;
}

InputDataFilterPNG::InputDataFilterPNG(InputIoFilter *ioFilter) : InputDataFilter()
{
	m_ioFilter = ioFilter;
}

InputDataFilterPNG::~InputDataFilterPNG()
{}

int InputDataFilterPNG::IsSupported()
{
	int ok;
	if (m_ioFilter) {
		unsigned char sig[9];
		ok = m_ioFilter->Read(0, 9, sig) == 9 ? (png_check_sig(sig, 9) != 0) : 0;
	} else {
		ok = 0;
	}
	if (ok == 1) {
		unsigned long width, height;
		if (CheckFileFormat(0, 0, 0, &width, &height) < 0)
			ok = 0;
	}
	return ok;
}

/* Raw-buffer variant: returns bare buffers (*images pixels, *cluts palette,
   *srcFormat format) instead of an ImageRGBA. dstWidth, dstHeight: requested
   size (0 = image size); format: pixel format (0 = automatic); alpha: alpha
   value; flags: option mask; srcWidth, srcHeight: image size; status:
   _ImgDecStatus. Formats: 4 = 8-bit indexed + palette, 3 = RGBA, 2 = RGB;
   when format differs from the native format the pixels are converted.
   Returns EICL_ERR_NO_DATA (no stream), EICL_ERR_BAD_DATA (libpng error or cancelled), EICL_ERR_CODEC
   (png_create_info_struct failed), EICL_ERR_NO_MEMORY (out of memory), EICL_ERR_FORMAT
   (unsupported format or size mismatch). */
int InputDataFilterPNG::GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
    unsigned char **images, unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status)
{
	int ret = 0;
	unsigned char *rawBuf = 0;
	unsigned char *convBuf = 0;
	unsigned char *clutBuf = 0;
	unsigned char **rowPointers = 0;
	png_structp png_ptr = 0;
	png_infop info_ptr;
	png_colorp palette = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	int numPalette;
	png_bytep trans;
	int numTrans;
	png_color_16p transValues;
	png_uint_32 rowbytes;
	int passes;
	int fmt;
	unsigned int row;

	*images = 0;
	*cluts = 0;

	if (!m_ioFilter)
		return EICL_ERR_NO_DATA;
	m_ioFilter->Seek(0);

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_ptr)
		return EICL_ERR_BAD_DATA;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, 0, 0);
		return EICL_ERR_CODEC;
	}

	if (setjmp(png_ptr->jmpbuf)) {
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		if (rowPointers)
			free(rowPointers);
		if (rawBuf)
			delete[] rawBuf;
		if (clutBuf)
			delete[] clutBuf;
		return EICL_ERR_BAD_DATA;
	}

	png_set_read_fn(png_ptr, m_ioFilter, PngReadDataFn);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, 0, 0);

	if (bit_depth < 8)
		png_set_packing(png_ptr);
	if (color_type == 0 || color_type == 4)
		png_set_gray_to_rgb(png_ptr);
	passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
	rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	*srcWidth = (int)width;
	*srcHeight = (int)height;
	if (*dstWidth == 0 || *dstHeight == 0) {
		*dstWidth = (int)width;
		*dstHeight = (int)height;
	}
	if (*dstWidth != (int)width) {
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		return EICL_ERR_FORMAT;
	}
	if (*dstHeight != (int)height) {
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		return EICL_ERR_FORMAT;
	}

	if (color_type == 3)
		fmt = 4;
	else if (color_type & 4)
		fmt = 3;
	else
		fmt = 2;

	if (format == 0)
		format = fmt;
	if (fmt == 4) {
		if (format != fmt) {
			png_destroy_read_struct(&png_ptr, &info_ptr, 0);
			return EICL_ERR_FORMAT;
		}
	} else {
		if (format != 2 && format != 3) {
			png_destroy_read_struct(&png_ptr, &info_ptr, 0);
			return EICL_ERR_FORMAT;
		}
	}

	if (color_type == 3) {
		png_get_PLTE(png_ptr, info_ptr, &palette, &numPalette);
		clutBuf = new unsigned char[1024];
		rawBuf = new unsigned char[*dstWidth * *dstHeight];
		if (clutBuf == 0 || rawBuf == 0) {
			ret = EICL_ERR_NO_MEMORY;
		} else {
			unsigned char *dst = clutBuf;
			unsigned char *src = (unsigned char *)palette;
			int p;

			for (p = 0; p < numPalette; p++) {
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = (unsigned char)alpha;
			}
			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
				unsigned char *alphaPtr;
				unsigned char *tp;
				int t;

				png_get_tRNS(png_ptr, info_ptr, &trans, &numTrans, &transValues);
				alphaPtr = clutBuf + 3;
				tp = trans;
				if (flags & theImageDecoder::fill_alpha) {
					for (t = 0; t < numTrans; t++) {
						*alphaPtr = (unsigned char)alpha;
						alphaPtr += 4;
					}
				} else {
					for (t = 0; t < numTrans; t++) {
						*alphaPtr = *tp++;
						alphaPtr += 4;
					}
				}
			}
		}
	} else {
		rawBuf = new unsigned char[rowbytes * *dstHeight];
		if (rawBuf == 0)
			ret = EICL_ERR_NO_MEMORY;
	}

	rowPointers = (unsigned char **)malloc(*dstHeight * 4);
	if (rowPointers == 0)
		ret = EICL_ERR_NO_MEMORY;

	if (ret == 0) {
		unsigned char *rowPtr;
		int stride;
		int pass;

		if (color_type == 3)
			stride = *dstWidth;
		else
			stride = rowbytes;
		rowPtr = rawBuf;
		for (row = 0; row < (unsigned int)*dstHeight; row++, rowPtr += stride)
			rowPointers[row] = rowPtr;

		for (pass = 0; pass < passes; pass++) {
			for (row = 0; row < (unsigned int)*dstHeight; row++) {
				if (((_ImgDecStatus *)status)->cancel == 1) {
					png_destroy_read_struct(&png_ptr, &info_ptr, 0);
					if (rowPointers)
						free(rowPointers);
					if (rawBuf)
						delete[] rawBuf;
					if (clutBuf)
						delete[] clutBuf;
					return EICL_ERR_BAD_DATA;
				}
				png_read_rows(png_ptr, &rowPointers[row], 0, 1);
				((_ImgDecStatus *)status)->percent = (row + 1) * 100 / *dstHeight;
			}
		}

		if (fmt != format) {
			if (format == 3) {
				int nbytes = *dstHeight * 4;

				convBuf = new unsigned char[*dstWidth * nbytes];
				if (convBuf == 0) {
					ret = EICL_ERR_NO_MEMORY;
				} else if (fmt == 2) {
					unsigned char *dst = convBuf;

					for (row = 0; row < (unsigned int)*dstHeight; row++) {
						unsigned char *src = rowPointers[row];
						int x;

						for (x = 0; x < *dstWidth; x++) {
							*dst++ = *src++;
							*dst++ = *src++;
							*dst++ = *src++;
							*dst++ = (unsigned char)alpha;
						}
					}
				} else {
					ret = EICL_ERR_FORMAT;
				}
			} else if (format == 2) {
				int nbytes = *dstHeight * 3;

				convBuf = new unsigned char[*dstWidth * nbytes];
				if (convBuf == 0) {
					ret = EICL_ERR_NO_MEMORY;
				} else if (fmt == 3) {
					unsigned char *dst = convBuf;

					for (row = 0; row < (unsigned int)*dstHeight; row++) {
						unsigned char *src = rowPointers[row];
						int x;

						for (x = 0; x < *dstWidth; x++) {
							*dst++ = *src++;
							*dst++ = *src++;
							*dst++ = *src++;
							src++;
						}
					}
				} else {
					ret = EICL_ERR_FORMAT;
				}
			}
			if (ret == 0) {
				if (rawBuf)
					delete[] rawBuf;
				rawBuf = convBuf;
				convBuf = 0;
			}
			if (convBuf)
				delete[] convBuf;
		} else if (fmt == 3) {
			if (flags & theImageDecoder::fill_alpha) {
				for (row = 0; row < (unsigned int)*dstHeight; row++) {
					unsigned char *p = rowPointers[row];
					int x;

					p += 3;
					for (x = 0; x < *dstWidth; x++) {
						*p = (unsigned char)alpha;
						p += 4;
					}
				}
			}
		}
	}

	if (ret == 0) {
		*images = rawBuf;
		rawBuf = 0;
		*cluts = clutBuf;
		clutBuf = 0;
		/* NOTE: bug: reports the native format even when the pixels were converted to format. */
		*srcFormat = fmt;
	}
	png_destroy_info_struct(png_ptr, &info_ptr);
	png_destroy_read_struct(&png_ptr, 0, 0);
	if (rowPointers)
		free(rowPointers);
	if (rawBuf)
		delete[] rawBuf;
	if (clutBuf)
		delete[] clutBuf;
	return ret;
}

/* Like GetPNGImageClut, but allocates the pixel and palette buffers itself
   and returns them in a new ImageRGBA (*out), filled with SetImage. */
int InputDataFilterPNG::GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha)
{
	int ret = 0;
	unsigned char *rawBuf = 0;
	unsigned char *clutBuf = 0;
	unsigned char **rowPointers = 0;
	png_structp png_ptr = 0;
	png_colorp palette = 0;

	if (!m_ioFilter)
		return EICL_ERR_NO_DATA;
	m_ioFilter->Seek(0);

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_ptr)
		return EICL_ERR_BAD_DATA;

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, 0, 0);
		return EICL_ERR_CODEC;
	}

	if (setjmp(png_ptr->jmpbuf)) {
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		if (rowPointers)
			free(rowPointers);
		if (rawBuf)
			free(rawBuf);
		if (clutBuf)
			free(clutBuf);
		return EICL_ERR_BAD_DATA;
	}

	png_set_read_fn(png_ptr, m_ioFilter, PngReadDataFn);
	png_read_info(png_ptr, info_ptr);

	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, 0, 0);

	if (bit_depth < 8)
		png_set_packing(png_ptr);
	if (color_type == 0 || color_type == 4)
		png_set_gray_to_rgb(png_ptr);

	*outWidth = (int)width;
	*outHeight = (int)height;

	png_read_update_info(png_ptr, info_ptr);
	png_uint_32 rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	if (color_type == 3) {
		int numPalette;
		png_get_PLTE(png_ptr, info_ptr, &palette, &numPalette);

		clutBuf = (unsigned char *)malloc(1024);
		rawBuf = (unsigned char *)malloc(width * height);
		if (clutBuf == 0 || rawBuf == 0) {
			ret = EICL_ERR_NO_MEMORY;
		} else {
			unsigned char *dst = clutBuf;
			unsigned char *src = (unsigned char *)palette;
			memset(clutBuf, 255, 1024);
			int i;
			for (i = 0; i < numPalette; i++) {
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = 255;
			}

			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
				png_bytep trans;
				int numTrans;
				png_color_16p transValues;
				png_get_tRNS(png_ptr, info_ptr, &trans, &numTrans, &transValues);

				unsigned char *alphaPtr = clutBuf + 3;
				unsigned char *t = trans;
				int j;
				for (j = 0; j < numTrans; j++) {
					*alphaPtr = *t++;
					alphaPtr += 4;
				}
			}
		}
	} else {
		rawBuf = (unsigned char *)malloc(rowbytes * height);
		if (rawBuf == 0)
			ret = EICL_ERR_NO_MEMORY;
	}

	rowPointers = (unsigned char **)malloc(height * 4);
	if (rowPointers == 0)
		ret = EICL_ERR_NO_MEMORY;

	if (color_type == 3)
		*out = new ImageRGBA(width, height, alpha, ImageRGBA::INDEX8);
	else
		*out = new ImageRGBA(width, height, alpha, ImageRGBA::RGBA8888);
	if (*out == 0)
		ret = EICL_ERR_NO_MEMORY;

	if (ret == 0) {
		unsigned char *rowPtr;
		unsigned int row;
		int stride;
		if (color_type == 3)
			stride = width;
		else
			stride = rowbytes;
		for (row = 0, rowPtr = rawBuf; row < height; row++, rowPtr += stride)
			rowPointers[row] = rowPtr;
		png_read_image(png_ptr, rowPointers);

		if (color_type == 3)
			(*out)->SetImage(rawBuf, ImageRGBA::INDEX8, clutBuf);
		else if (color_type & 4)
			(*out)->SetImage(rawBuf, ImageRGBA::RGBA8888, 0);
		else
			(*out)->SetImage(rawBuf, ImageRGBA::RGB888, 0);
		ret = 0;
	}

	png_destroy_info_struct(png_ptr, &info_ptr);
	png_destroy_read_struct(&png_ptr, 0, 0);
	if (rowPointers)
		free(rowPointers);
	if (rawBuf)
		free(rawBuf);
	if (clutBuf)
		free(clutBuf);
	return ret;
}

/* Reads the image into the caller's buffers: row pointers over the indexed or
   RGB(A) pixels in *a and, for paletted images, the 256-entry RGBA palette
   in *b (alpha from tRNS). Returns EICL_ERR_NO_DATA (no stream), EICL_ERR_BAD_DATA (libpng error),
   EICL_ERR_CODEC (png_create_info_struct failed), EICL_ERR_NO_MEMORY (bad arguments or out of
   memory). */
int InputDataFilterPNG::GetPNGImageClut(unsigned char **a, unsigned char **b)
{
	int ret = 0;
	png_structp png_ptr = 0;
	png_colorp palette = 0;

	if (!m_ioFilter)
		return EICL_ERR_NO_DATA;
	m_ioFilter->Seek(0);

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_ptr)
		return EICL_ERR_BAD_DATA;

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, 0, 0);
		return EICL_ERR_CODEC;
	}

	if (setjmp(png_ptr->jmpbuf)) {
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		return EICL_ERR_BAD_DATA;
	}

	png_set_read_fn(png_ptr, m_ioFilter, PngReadDataFn);
	png_read_info(png_ptr, info_ptr);

	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, 0, 0);

	if (bit_depth < 8)
		png_set_packing(png_ptr);
	if (color_type == 0 || color_type == 4)
		png_set_gray_to_rgb(png_ptr);

	png_read_update_info(png_ptr, info_ptr);
	png_uint_32 rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	unsigned char *rawBuf;
	if (color_type == 3) {
		int numPalette;
		png_get_PLTE(png_ptr, info_ptr, &palette, &numPalette);

		unsigned char *clutBuf = *b;
		rawBuf = *a;
		if (clutBuf == 0 || rawBuf == 0) {
			ret = EICL_ERR_NO_MEMORY;
		} else {
			unsigned char *dst = clutBuf;
			unsigned char *src = (unsigned char *)palette;
			memset(clutBuf, 255, 1024);
			int i;
			for (i = 0; i < numPalette; i++) {
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = 255;
			}

			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
				png_bytep trans;
				int numTrans;
				png_color_16p transValues;
				png_get_tRNS(png_ptr, info_ptr, &trans, &numTrans, &transValues);

				unsigned char *alphaPtr = clutBuf + 3;
				unsigned char *t = trans;
				int j;
				for (j = 0; j < numTrans; j++) {
					*alphaPtr = *t++;
					alphaPtr += 4;
				}
			}
		}
	} else {
		rawBuf = *a;
		if (rawBuf == 0)
			ret = EICL_ERR_NO_MEMORY;
	}

	unsigned char **rowPointers = (unsigned char **)malloc(height * 4);
	if (rowPointers == 0)
		ret = EICL_ERR_NO_MEMORY;

	if (ret == 0) {
		unsigned char *rowPtr;
		unsigned int row;
		int stride;
		if (color_type == 3)
			stride = width;
		else
			stride = rowbytes;
		for (row = 0, rowPtr = rawBuf; row < height; row++, rowPtr += stride)
			rowPointers[row] = rowPtr;
		png_read_image(png_ptr, rowPointers);
	}

	png_destroy_info_struct(png_ptr, &info_ptr);
	png_destroy_read_struct(&png_ptr, 0, 0);
	if (rowPointers)
		free(rowPointers);
	return ret;
}

/* Reads the IHDR through libpng and fills props: 8 bpp for paletted images,
   32 with alpha, otherwise 24; resolution from pHYs converted to dpi, or
   72 dpi. Returns 0 if libpng cannot be set up, EICL_ERR_BAD_DATA on a libpng error. */
int InputDataFilterPNG::GetProps(PgProps *props)
{
	png_structp png_ptr = 0;
	if (!m_ioFilter)
		return 0;
	m_ioFilter->Seek(0);

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_ptr)
		return 0;

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, 0, 0);
		return 0;
	}

	if (setjmp(png_ptr->jmpbuf)) {
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		return EICL_ERR_BAD_DATA;
	}

	png_set_read_fn(png_ptr, m_ioFilter, PngReadDataFn);
	png_read_info(png_ptr, info_ptr);

	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, 0, 0);

	props->SetFormat((PgProps::PgFormat)64);
	props->SetImageType(PgProps::kImageTypeDefault);
	props->SetSize((int)width, (int)height);
	props->SetColorSpace(PgProps::kRGBA);
	if (color_type == 3)
		props->SetBitsPerPixel(8);
	else if (color_type & 4)
		props->SetBitsPerPixel(32);
	else
		props->SetBitsPerPixel(24);

	unsigned int xres = png_get_x_pixels_per_meter(png_ptr, info_ptr);
	unsigned int yres = png_get_y_pixels_per_meter(png_ptr, info_ptr);
	unsigned int res = png_get_pixels_per_meter(png_ptr, info_ptr);
	double aspect = png_get_pixel_aspect_ratio(png_ptr, info_ptr);

	if (xres == 0)
		goto no_res;
	if (yres == 0)
		goto no_res;
	if (res == 0)
		goto no_res;
	if (aspect == 0.0)
		goto no_res;
	{
		double dpix = xres * 254 / 10000;
		double dpiy = yres * 254 / 10000;
		props->SetResolution(dpix, dpiy);
	}
	goto res_done;
no_res:
	props->SetResolution(72.0, 72.0);
res_done:;

	png_destroy_info_struct(png_ptr, &info_ptr);
	png_destroy_read_struct(&png_ptr, 0, 0);
	return 1;
}

static void make_crc_table(void)
{
	unsigned long c;
	int n, k;

	for (n = 0; n < 256; n++) {
		c = (unsigned long)n;
		for (k = 0; k < 8; k++) {
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}
		crc_table[n] = c;
	}
	crc_table_computed = 1;
}

static unsigned long update_crc(unsigned long crc, unsigned char *buf, int len)
{
	unsigned long c = crc;
	int n;

	if (!crc_table_computed)
		make_crc_table();

	for (n = 0; n < len; n++)
		c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
	return c;
}

static unsigned long update_crc_byte(unsigned long crc, int b)
{
	if (!crc_table_computed)
		make_crc_table();

	return crc_table[(crc ^ b) & 0xff] ^ (crc >> 8);
}

/* Hand-written PNG validator (no libpng): looks for the signature in the
   first 129 bytes, then walks the chunks checking their CRC and the IHDR
   fields. Bit 0 of a is the strict flag: without it a non-conforming IHDR is
   ignored instead of rejected. b, c: maximum width and height (0 = no limit).
   Returns -1 no stream, -2 EOF, -3 seek failed, -4 no IEND, -10 no signature,
   -11 size out of limits, -12 bad chunk CRC, -30 non-conforming IHDR. */
long InputDataFilterPNG::CheckFileFormat(unsigned long a, unsigned long b, unsigned long c, unsigned long *width,
    unsigned long *height)
{
	unsigned char buf[1024];
	unsigned char byte;
	unsigned long imgWidth = 0, imgHeight = 0;
	int sawIEND = 0;
	long found;
	int i;
	unsigned long crc = 0;
	unsigned int pos, chunkStart;
	unsigned long chunkLen, chunkType;
	unsigned long rest;
	unsigned long k;
	int bad;
	unsigned int bitDepth, colorType, compression, filter, interlace;

	if (!m_ioFilter)
		return -1;

	if (b == 0)
		b = 0xffffffff;
	if (c == 0)
		c = 0xffffffff;

	m_ioFilter->Seek(0, 0);

	found = -1;
	for (i = 0; i < 129; i++) {
		pos = m_ioFilter->GetCurrentPosition();
		if (!m_ioFilter->Read(1, &byte))
			goto err2;
		if (byte != 137)
			continue;
		if (!m_ioFilter->Read(7, buf))
			goto err2;
		if (memcmp("PNG\r\n\032\n", buf, 7) == 0) {
			found = i;
			break;
		}
		if (!m_ioFilter->Seek(++pos, 0))
			goto err3;
	}

	if (found == -1)
		goto err10;

	for (;;) {
		chunkStart = m_ioFilter->GetCurrentPosition();
		if (!m_ioFilter->Read(8, buf)) {
			if (!sawIEND)
				goto err4;
			break;
		}

		chunkLen = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
		chunkType = (buf[4] << 24) + (buf[5] << 16) + (buf[6] << 8) + buf[7];
		crc = update_crc(0xffffffff, buf + 4, 4);

		switch (chunkType) {
		case 0x49454e44:
			sawIEND = 1;
			break;
		case 0x49484452:
			if (!m_ioFilter->Read(13, buf))
				goto err4;
			crc = update_crc(crc, buf, 13);

			bad = 0;
			imgWidth = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
			imgHeight = (buf[4] << 24) + (buf[5] << 16) + (buf[6] << 8) + buf[7];
			bitDepth = buf[8];
			colorType = buf[9];
			compression = buf[10];
			filter = buf[11];
			interlace = buf[12];

			if (imgWidth == 0 || b < imgWidth)
				bad = 1;
			if (imgHeight == 0 || c < imgHeight)
				bad = 1;
			if (bad)
				goto err11;

			switch (colorType) {
			case 0:
				if (bitDepth == 1 || bitDepth == 2 || bitDepth == 4 || bitDepth == 8 || bitDepth == 16)
					break;
				if (a & 1)
					goto err30;
				goto chunk_done;
			case 2:
				if (bitDepth == 8 || bitDepth == 16)
					break;
				if (a & 1)
					goto err30;
				goto chunk_done;
			case 3:
				if (bitDepth == 1 || bitDepth == 2 || bitDepth == 4 || bitDepth == 8)
					break;
				if (a & 1)
					goto err30;
				goto chunk_done;
			case 4:
				/* NOTE: bug: gray + alpha accepts bit depths 1, 2 and 4 too (the spec allows 8 and 16). */
				if (bitDepth == 1 || bitDepth == 2 || bitDepth == 4 || bitDepth == 8)
					break;
				if (a & 1)
					goto err30;
				goto chunk_done;
			case 6:
				if (bitDepth == 8 || bitDepth == 16)
					break;
				if (a & 1)
					goto err30;
				goto chunk_done;
			}
			if (compression != 0) {
				if (a & 1)
					goto err30;
				break;
			}
			if (filter != 0) {
				if (a & 1)
					goto err30;
				break;
			}
			if (interlace >= 2) {
				if (a & 1)
					goto err30;
				break;
			}
			break;
		}
chunk_done:

		pos = m_ioFilter->GetCurrentPosition();
		rest = chunkStart + 8 + chunkLen - pos;
		if (rest != 0) {
			for (k = 0; k < rest; k++) {
				if (!m_ioFilter->Read(1, &byte))
					goto err2;
				crc = update_crc_byte(crc, byte);
			}
		}

		if (!m_ioFilter->Read(4, buf)) {
			if (sawIEND)
				break;
			goto err4;
		}
		crc = ~crc;
		if (crc != (unsigned long)((buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3]))
			goto err12;
	}

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
err12:
	m_ioFilter->Seek(0, 0);
	return -12;
err30:
	m_ioFilter->Seek(0, 0);
	return -30;
}
