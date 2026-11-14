/*
 * BMP decoder: BITMAPFILEHEADER/BITMAPINFOHEADER parsing, decoding of 1, 4, 8,
 * 16, 24 and 32 bpp images into an ImageRGBA or into raw caller buffers.
 */

#include <stdlib.h>
#include <string.h>
#include "inputdatafilterbmp.h"
#include "eiclerror.h"

#include "theimagedecoder.h"

int InputDataFilterBMP::IsSupported()
{
	if (!m_ioFilter)
		return 0;
	unsigned char sig[2];
	m_ioFilter->Read(0, 2, sig);
	if (sig[0] != 'B')
		return 0;
	if (sig[1] != 'M')
		return 0;
	return GetBitmapHeaders_();
}

/* 24 bpp: each row is converted with ImageRGBA::SetLine (BGR888). On failure
   the output image itself is deleted. */
int InputDataFilterBMP::GetRawData24Bit(ImageRGBA *img)
{
	int err;
	int width = m_bih.biWidth;
	int height = m_bih.biHeight;
	if (img) {
		int stride = ((width * 3) + 3) & ~3;
		unsigned char *rowbuf = (unsigned char *)malloc(stride);
		if (rowbuf) {
			int dataOffset = 40;
			int rowStep = (stride + 3) & ~3;
			if (m_hasFileHeader)
				dataOffset = m_bfh.bfOffBits;
			int row = height - 1;
			if (row >= 0) {
				do {
					m_ioFilter->Read(dataOffset, stride, rowbuf);
					img->SetLine(row, rowbuf, ImageRGBA::BGR888, 0);
					row--;
					dataOffset += rowStep;
				} while (row >= 0);
			}
			err = 0;
			free(rowbuf);
		} else {
			err = EICL_ERR_NO_MEMORY;
		}
	} else {
		err = EICL_ERR_NO_MEMORY;
	}
	if (err)
		delete img;
	return err;
}

/* 8 bpp indexed. The RGBQUAD palette (biClrUsed entries, or 1 << biBitCount)
   is read before img is checked. */
int InputDataFilterBMP::GetRawData8Bit(ImageRGBA *img)
{
	int paletteCount = m_bih.biClrUsed;
	if (!paletteCount)
		paletteCount = 1 << m_bih.biBitCount;
	unsigned char palette[1024];
	m_ioFilter->Read(paletteCount * 4, palette);
	int width = m_bih.biWidth;
	int height = m_bih.biHeight;
	int err;
	if (img) {
		int roundedStride = (width + 3) & ~3;
		unsigned char *rowbuf = (unsigned char *)malloc(roundedStride);
		if (rowbuf) {
			int row = height - 1;
			err = 0;
			for (; row >= 0; row--) {
				if (m_ioFilter->Read(roundedStride, rowbuf) == roundedStride) {
					unsigned int col = 0;
					while (col < (unsigned int)width) {
						unsigned char idx = rowbuf[col];
						unsigned char *entry = palette + idx * 4;
						img->SetPixel(col, row, entry[2], entry[1], entry[0]);
						col++;
					}
				} else {
					err = 258;
					break;
				}
			}
			free(rowbuf);
		} else {
			err = EICL_ERR_NO_MEMORY;
		}
	}
	/* NOTE: bug: err is returned uninitialized when img is NULL. */
	return err;
}

/* 4 bpp indexed, read one byte at a time; the padding at the end of each row
   is read and discarded. Always returns 0. */
int InputDataFilterBMP::GetRawData4Bit(ImageRGBA *img)
{
	int paletteCount = m_bih.biClrUsed;
	if (!paletteCount)
		paletteCount = 1 << m_bih.biBitCount;
	unsigned char palette[64];
	m_ioFilter->Read(paletteCount * 4, palette);
	int height = m_bih.biHeight;
	int width = m_bih.biWidth;
	int roundedStride = ((width / 2) + (width & 1) + 3) & ~3;
	unsigned char b;
	unsigned char *e;
	for (int row = height - 1; row >= 0; row--) {
		int col = 0;
		int nbytes = 0;
		while (col < width - (width & 1)) {
			m_ioFilter->Read(1, &b);
			e = &palette[(b >> 4) * 4];
			img->SetPixel(col++, row, e[2], e[1], e[0]);
			e = &palette[(b & 0xf) * 4];
			img->SetPixel(col++, row, e[2], e[1], e[0]);
			nbytes++;
		}
		while (col < width) {
			m_ioFilter->Read(1, &b);
			e = &palette[(b >> 4) * 4];
			img->SetPixel(col, row, e[2], e[1], e[0]);
			col++;
			nbytes++;
		}
		while (nbytes < roundedStride) {
			m_ioFilter->Read(1, &b);
			nbytes++;
		}
	}
	return 0;
}

/* 16 bpp (5:5:5), read sequentially from the current stream position. */
int InputDataFilterBMP::GetRawData16Bit(ImageRGBA *img)
{
	int err = 0;
	int width = m_bih.biWidth;
	int height = m_bih.biHeight;
	int stride = width * 2;
	if (img) {
		int roundedStride = (stride + 3) & ~3;
		unsigned char *rowbuf = (unsigned char *)malloc(stride);
		if (rowbuf) {
			int row = height - 1;
			if (row >= 0) {
				do {
					if (m_ioFilter->Read(stride, rowbuf) == stride) {
						int col = 0;
						unsigned char *p = rowbuf;
						while (col < width) {
							{
								int pixel = (p[1] << 8) + p[0];
								int r5 = (pixel & 0x7c00) >> 10;
								int g5 = (pixel & 0x3e0) >> 5;
								int b5 = pixel & 0x1f;
								img->SetPixel(col, row,
								    (unsigned char)((r5 * 255) / 32),
								    (unsigned char)((g5 * 255) / 32),
								    (unsigned char)((b5 * 255) / 32));
								col++;
								p += 2;
							}
						}
						if (col * 2 < roundedStride) {
							if (m_ioFilter->Read(2, rowbuf) != 2) {
								err = 258;
								break;
							}
						}
						row--;
					} else {
						err = 258;
						break;
					}
				} while (row >= 0);
			}
			free(rowbuf);
		}
	}
	/* NOTE: bug: returns 0 when img is NULL or the row buffer cannot be allocated. */
	return err;
}

/* 32 bpp BI_BITFIELDS: three channel masks, each shifted down to bit 0. A
   short read abandons only the current row. */
int InputDataFilterBMP::GetRawData32BitMasked(ImageRGBA *img)
{
	int err = 0;
	unsigned int mask[3];
	unsigned char buf[4];
	if (m_ioFilter->Read(4, (unsigned char *)&mask[0]) != 4 || m_ioFilter->Read(4, (unsigned char *)&mask[1]) != 4
	    || m_ioFilter->Read(4, (unsigned char *)&mask[2]) != 4) {
		err = 258;
	} else {
		int sh0 = 0;
		if ((mask[0] & 1) == 0) {
			unsigned int m = mask[0];
			do {
				sh0++;
				m >>= 1;
			} while ((m & 1) == 0);
			mask[0] = m;
		}
		int sh1 = 0;
		if ((mask[1] & 1) == 0) {
			unsigned int m = mask[1];
			do {
				sh1++;
				m >>= 1;
			} while ((m & 1) == 0);
			mask[1] = m;
		}
		int sh2 = 0;
		if ((mask[2] & 1) == 0) {
			unsigned int m = mask[2];
			do {
				sh2++;
				m >>= 1;
			} while ((m & 1) == 0);
			mask[2] = m;
		}
		if (img) {
			int height = m_bih.biHeight;
			int width = m_bih.biWidth;
			for (int row = height - 1; row >= 0; row--) {
				for (int col = 0; col < width; col++) {
					if (m_ioFilter->Read(4, buf) != 4) {
						err = 258;
						goto nextRow;
					}
					int pixel = (buf[2] << 16) + (buf[1] << 8) + buf[0];
					/* NOTE: bug: the masks of the first and third channel are swapped in the AND. */
					img->SetPixel(col, row,
					    (unsigned char)(((pixel >> sh0) & mask[2]) * 255 / mask[0]),
					    (unsigned char)(((pixel >> sh1) & mask[1]) * 255 / mask[1]),
					    (unsigned char)(((pixel >> sh2) & mask[0]) * 255 / mask[2]));
				}
nextRow:;
			}
		} else {
			err = EICL_ERR_CODEC;
		}
	}
	return err;
}

/* 16 bpp BI_BITFIELDS: the mask handling of GetRawData32BitMasked with the
   row padding of GetRawData16Bit. */
int InputDataFilterBMP::GetRawData16BitMasked(ImageRGBA *img)
{
	int err = 0;
	unsigned int mask[3];
	unsigned char buf[4];
	if (m_ioFilter->Read(4, (unsigned char *)&mask[0]) == 4) {
		if (m_ioFilter->Read(4, (unsigned char *)&mask[1]) != 4) {
			err = 258;
		} else if (m_ioFilter->Read(4, (unsigned char *)&mask[2]) != 4) {
			err = 258;
		} else {
			int sh0 = 0;
			if ((mask[0] & 1) == 0) {
				unsigned int m = mask[0];
				do {
					sh0++;
					m >>= 1;
				} while ((m & 1) == 0);
				mask[0] = m;
			}
			int sh1 = 0;
			if ((mask[1] & 1) == 0) {
				unsigned int m = mask[1];
				do {
					sh1++;
					m >>= 1;
				} while ((m & 1) == 0);
				mask[1] = m;
			}
			int sh2 = 0;
			if ((mask[2] & 1) == 0) {
				unsigned int m = mask[2];
				do {
					sh2++;
					m >>= 1;
				} while ((m & 1) == 0);
				mask[2] = m;
			}
			if (img) {
				int height = m_bih.biHeight;
				int width = m_bih.biWidth;
				int roundedStride = (width * 2 + 3) & ~3;
				for (int row = height - 1; row >= 0; row--) {
					int col = 0;
					if (width > 0) {
						do {
							if (m_ioFilter->Read(2, buf) != 2) {
								err = 258;
								break;
							}
							int pixel = (buf[1] << 8) + buf[0];
							/* NOTE: bug: the masks of the first and third channel are swapped in the AND. */
							img->SetPixel(col, row,
							    (unsigned char)(((pixel >> sh0) & mask[2]) * 255 / mask[0]),
							    (unsigned char)(((pixel >> sh1) & mask[1]) * 255 / mask[1]),
							    (unsigned char)(((pixel >> sh2) & mask[0]) * 255
							        / mask[2]));
							col++;
						} while (col < width);
					}
					if (col * 2 < roundedStride) {
						if (m_ioFilter->Read(2, buf) != 2) {
							err = 258;
							break;
						}
					}
				}
			} else {
				err = EICL_ERR_CODEC;
			}
		}
	} else {
		err = 258;
	}
	return err;
}

/* 1 bpp: two-entry palette, eight pixels per byte, most significant bit first. */
int InputDataFilterBMP::GetRawData1Bit(ImageRGBA *img)
{
	int err = 0;
	unsigned char palette[8];
	if (m_ioFilter->Read(8, palette) == 8) {
		if (img) {
			int width = m_bih.biWidth;
			int bytes = width / 8;
			bytes += ((width & 7) != 0);
			int roundedStride = (bytes + 3) & ~3;
			int height = m_bih.biHeight;
			unsigned char *rowbuf = (unsigned char *)malloc(roundedStride);
			if (rowbuf) {
				for (int row = height - 1; row >= 0; row--) {
					if (m_ioFilter->Read(roundedStride, rowbuf) == roundedStride) {
						int col = 0;
						while (col < width) {
							unsigned char *entry =
							    palette + (((rowbuf[col / 8] >> (7 - col % 8)) & 1) * 4);
							img->SetPixel(col, row, entry[2], entry[1], entry[0]);
							col++;
						}
					} else {
						err = 258;
						break;
					}
				}
				free(rowbuf);
			} else {
				err = EICL_ERR_NO_MEMORY;
			}
		} else {
			err = EICL_ERR_CODEC;
		}
	} else {
		err = 258;
	}
	return err;
}

/* 32 bpp: the fourth byte of each pixel is ignored. A short read stops
   decoding but still returns 0. */
int InputDataFilterBMP::GetRawData32Bit(ImageRGBA *img)
{
	int err = 0;
	if (img) {
		int width = m_bih.biWidth;
		int height = m_bih.biHeight;
		int stride = width * 4;
		int dataOffset = 40;
		if (m_hasFileHeader)
			dataOffset = m_bfh.bfOffBits;
		unsigned char *rowbuf = (unsigned char *)malloc(stride);
		if (rowbuf) {
			int row = height - 1;
			for (; row >= 0; row--, dataOffset += stride) {
				if (m_ioFilter->Read(dataOffset, stride, rowbuf) == stride) {
					unsigned char *p = rowbuf;
					int col = 0;
					while (col < width) {
						img->SetPixel(col, row, p[2], p[1], p[0]);
						p += 4;
						col++;
					}
				} else {
					err = 0;
					break;
				}
			}
		} else {
			err = EICL_ERR_NO_MEMORY;
		}
	}
	/* NOTE: bug: the row buffer is never freed. */
	return err;
}

/* Creates the image and hands it to the decoder for (biCompression,
   biBitCount). Unsupported combinations fill the image with black and
   return EICL_ERR_BAD_HEADER. */
int InputDataFilterBMP::GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha)
{
	if (!m_ioFilter)
		goto noIoFilter;
	if (GetBitmapHeaders_() != 1) {
fail:
		return EICL_ERR_BAD_HEADER;
	}
	*outWidth = m_bih.biWidth;
	*outHeight = m_bih.biHeight;
	*out = new ImageRGBA(m_bih.biWidth, m_bih.biHeight, alpha, ImageRGBA::RGBA8888);
	/* NOTE: bug: tests out instead of *out. */
	if (!out)
		goto noMem;
	if (m_bih.biCompression == 0) {
		if (m_bih.biBitCount == 24)
			return GetRawData24Bit(*out);
		if (m_bih.biBitCount == 8)
			return GetRawData8Bit(*out);
		if (m_bih.biBitCount == 4)
			return GetRawData4Bit(*out);
		if (m_bih.biBitCount == 16)
			return GetRawData16Bit(*out);
	}
	if (m_bih.biCompression == 3 && m_bih.biBitCount == 16)
		return GetRawData16BitMasked(*out);
	if (m_bih.biBitCount == 1)
		return GetRawData1Bit(*out);
	if (m_bih.biCompression == 0 && m_bih.biBitCount == 32)
		return GetRawData32Bit(*out);
	if (m_bih.biCompression == 3 && m_bih.biBitCount == 32)
		return GetRawData32BitMasked(*out);
	{
		int y;
		int x;

		for (y = 0; y < m_bih.biHeight; y++)
			for (x = 0; x < m_bih.biWidth; x++)
				(*out)->SetPixel(x, y, 0, 0, 0);
	}
	goto fail;
noIoFilter:
	return EICL_ERR_STREAM;
noMem:
	return EICL_ERR_NO_MEMORY;
}

/* Returns 1 without reading anything when there is no file header,
   otherwise 1 only if all five fields are read. */
int InputDataFilterBMP::GetBitmapFileHeader_()
{
	int flag = m_hasFileHeader;
	int ok;
	if (flag == 1) {
		ok = 0;
		if (m_ioFilter->ReadInt16Reverse(&m_bfh.bfType) && m_ioFilter->ReadInt32Reverse(&m_bfh.bfSize)
		    && m_ioFilter->ReadInt16Reverse(&m_bfh.bfReserved1)
		    && m_ioFilter->ReadInt16Reverse(&m_bfh.bfReserved2)
		    && m_ioFilter->ReadInt32Reverse(&m_bfh.bfOffBits))
			ok = flag;
	} else {
		ok = 1;
	}
	return ok;
}

/* Accepts only BI_RGB (0) and BI_BITFIELDS (3). On success the stream is left
   just past the info header. */
int InputDataFilterBMP::GetBitmapInfoHeader_()
{
	int ok = 0;
	if (m_ioFilter->ReadInt32Reverse(&m_bih.biSize) && m_ioFilter->ReadInt32Reverse(&m_bih.biWidth)
	    && m_ioFilter->ReadInt32Reverse(&m_bih.biHeight) && m_ioFilter->ReadInt16Reverse(&m_bih.biPlanes)
	    && m_ioFilter->ReadInt16Reverse(&m_bih.biBitCount) && m_ioFilter->ReadInt32Reverse(&m_bih.biCompression)
	    && m_ioFilter->ReadInt32Reverse(&m_bih.biSizeImage) && m_ioFilter->ReadInt32Reverse(&m_bih.biXPelsPerMeter)
	    && m_ioFilter->ReadInt32Reverse(&m_bih.biYPelsPerMeter) && m_ioFilter->ReadInt32Reverse(&m_bih.biClrUsed))
		ok = m_ioFilter->ReadInt32Reverse(&m_bih.biClrImportant) != 0;

	if (ok) {
		if (m_bih.biCompression != 0 && m_bih.biCompression != 3) {
			ok = 0;
		} else if (m_hasFileHeader) {
			m_ioFilter->Seek(m_bih.biSize + 14);
		} else {
			m_ioFilter->Seek(m_bih.biSize);
		}
	}
	return ok;
}

int InputDataFilterBMP::GetProps(PgProps *props)
{
	int ok = GetBitmapHeaders_();
	if (ok != 1)
		return ok;
	props->SetFormat((PgProps::PgFormat)2048);
	props->SetImageType(PgProps::kImageTypeDefault);
	props->SetSize(m_bih.biWidth, m_bih.biHeight);
	double xdpi = (double)m_bih.biXPelsPerMeter * 2.54 / 100.0;
	double ydpi = (double)m_bih.biYPelsPerMeter * 2.54 / 100.0;
	props->SetResolution(xdpi, ydpi);
	switch (m_bih.biBitCount) {
	case 0:
		ok = 0;
		break;
	case 1:
		props->SetColorSpace(PgProps::kMonochrome);
		props->SetBitsPerPixel(1);
		break;
	case 4:
		props->SetColorSpace(PgProps::kRGB);
		props->SetBitsPerPixel(4);
		break;
	case 8:
		props->SetColorSpace(PgProps::kRGB);
		props->SetBitsPerPixel(8);
		break;
	case 16:
		props->SetColorSpace(PgProps::kRGB);
		props->SetBitsPerPixel(16);
		break;
	case 24:
		props->SetColorSpace(PgProps::kRGB);
		props->SetBitsPerPixel(24);
		break;
	case 32:
		props->SetColorSpace(PgProps::kRGBA);
		props->SetBitsPerPixel(32);
		break;
	}
tail:
	return ok;
}

int InputDataFilterBMP::GetBitmapHeaders_()
{
	m_bfh.bfOffBits = 40;
	m_ioFilter->Seek(0);
	int ret = GetBitmapFileHeader_();
	if (ret)
		ret = GetBitmapInfoHeader_();
	return ret;
}

InputDataFilterBMP::InputDataFilterBMP() : InputDataFilter()
{
	m_ioFilter = 0;
	m_hasFileHeader = 1;
}

InputDataFilterBMP::InputDataFilterBMP(InputIoFilter *ioFilter) : InputDataFilter()
{
	m_ioFilter = ioFilter;
	m_hasFileHeader = 1;
}

InputDataFilterBMP::~InputDataFilterBMP()
{}

int InputDataFilterBMP::IgnoreFileHeader()
{
	int old = m_hasFileHeader;
	m_hasFileHeader = 0;
	return old;
}

/* Raw-buffer variant: the decoders allocate the output buffers.
   dstWidth, dstHeight: requested size, in/out (0 = image size); format: requested
   pixel format (0 = automatic); srcFormat: format produced; srcWidth, srcHeight:
   image size; status: progress.
   Accepted (biCompression, biBitCount): (0,24), (0,32), (3,32), (0,8).
   Returns EICL_ERR_STREAM without a stream, EICL_ERR_BAD_HEADER for bad headers, EICL_ERR_FORMAT otherwise. */
int InputDataFilterBMP::GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
    unsigned char **images, unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status)
{
	if (m_ioFilter) {
		*srcWidth = m_bih.biWidth;
		*srcHeight = m_bih.biHeight;
		if (GetBitmapHeaders_() != 1)
			return EICL_ERR_BAD_HEADER;
		if (*dstWidth == 0 || *dstHeight == 0) {
			*dstWidth = m_bih.biWidth;
			*dstHeight = m_bih.biHeight;
		}
		if (*dstWidth != m_bih.biWidth)
			return EICL_ERR_FORMAT;
		if (*dstHeight != m_bih.biHeight)
			return EICL_ERR_FORMAT;
		if (m_bih.biCompression == 0) {
			if (m_bih.biBitCount == 24) {
				if (format == 0)
					format = 2;
				*srcFormat = 2;
				if (format != 2 && format != 3)
					return EICL_ERR_FORMAT;
				return GetRawData24Bit(dstWidth, dstHeight, format, alpha, flags, images, status);
			}
			if (m_bih.biBitCount == 32) {
				if (format == 0)
					format = 3;
				*srcFormat = 3;
				if (format != 2 && format != 3)
					return EICL_ERR_FORMAT;
				return GetRawData32Bit(dstWidth, dstHeight, format, alpha, flags, images, status);
			}
		}
		if (m_bih.biCompression == 3 && m_bih.biBitCount == 32) {
			if (format == 0)
				format = 3;
			*srcFormat = 3;
			if (format != 2 && format != 3)
				return EICL_ERR_FORMAT;
			return GetRawData32BitMasked(dstWidth, dstHeight, format, alpha, flags, images, status);
		}
		if (m_bih.biCompression == 0 && m_bih.biBitCount == 8) {
			if (format == 0)
				format = 4;
			*srcFormat = 4;
			if (format != 4)
				return EICL_ERR_FORMAT;
			return GetRawData8Bit(dstWidth, dstHeight, 4, alpha, flags, images, cluts, status);
		}
		return EICL_ERR_FORMAT;
	}
	return EICL_ERR_STREAM;
}

/* 24 bpp into *images: format == 2 gives 3 bytes per pixel (RGB), otherwise 4
   with alpha as the fourth byte. */
int InputDataFilterBMP::GetRawData24Bit(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
    unsigned char **images, void *status)
{
	int err;
	int width = m_bih.biWidth;
	int height = m_bih.biHeight;
	_ImgDecStatus *prog = (_ImgDecStatus *)status;
	unsigned char *image;
	int dataOffset;
	int stride;
	int row;
	unsigned char *src;
	unsigned char *dst;

	if (format == 2) {
		int rowBytes = width * 3;
		stride = (rowBytes + 3) & ~3;
		unsigned char *rowbuf = (unsigned char *)malloc(stride);
		image = new unsigned char[rowBytes * height];
		if (rowbuf && image) {
			dataOffset = 40;
			if (m_hasFileHeader)
				dataOffset = m_bfh.bfOffBits;
			for (row = height - 1; row >= 0; row--, dataOffset += stride) {
				if (prog->cancel == 1) {
					free(rowbuf);
					if (image)
						delete[] image;
					*images = 0;
					return EICL_ERR_BAD_DATA;
				}
				m_ioFilter->Read(dataOffset, stride, rowbuf);
				src = rowbuf;
				dst = image + row * width * 3;
				for (int col = 0; col < width; col++) {
					*dst++ = src[2];
					*dst++ = src[1];
					*dst++ = src[0];
					src += 3;
				}
				prog->percent = (height - row) * 100 / height;
			}
			err = 0;
			free(rowbuf);
		} else {
			if (rowbuf)
				free(rowbuf);
			if (image)
				delete[] image;
			image = 0;
			err = EICL_ERR_NO_MEMORY;
		}
	} else {
		stride = (width * 3 + 3) & ~3;
		unsigned char *rowbuf = (unsigned char *)malloc(stride);
		int rowBytes = width * 4;
		image = new unsigned char[rowBytes * height];
		if (rowbuf && image) {
			dataOffset = 40;
			if (m_hasFileHeader)
				dataOffset = m_bfh.bfOffBits;
			for (row = height - 1; row >= 0; row--, dataOffset += stride) {
				if (prog->cancel == 1) {
					free(rowbuf);
					if (image)
						delete[] image;
					*images = 0;
					return EICL_ERR_BAD_DATA;
				}
				m_ioFilter->Read(dataOffset, stride, rowbuf);
				src = rowbuf;
				dst = image + row * width * 4;
				for (int col = 0; col < width; col++) {
					*dst++ = src[2];
					*dst++ = src[1];
					*dst++ = src[0];
					*dst++ = (unsigned char)alpha;
					src += 3;
				}
				prog->percent = (height - row) * 100 / height;
			}
			err = 0;
			free(rowbuf);
		} else {
			if (rowbuf)
				free(rowbuf);
			if (image) {
				delete[] image;
				image = 0;
			}
			err = EICL_ERR_NO_MEMORY;
		}
	}
	if (err) {
		if (image)
			delete[] image;
		*images = 0;
	} else {
		*images = image;
	}
	return err;
}

/* 32 bpp into *images: format == 2 gives 3 bytes per pixel (RGB); otherwise 4
   bytes, src[3], src[2], src[1], then alpha if flags has fill_alpha set, else
   src[0]. */
int InputDataFilterBMP::GetRawData32Bit(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
    unsigned char **images, void *status)
{
	int err;
	int width = m_bih.biWidth;
	int height = m_bih.biHeight;
	_ImgDecStatus *prog = (_ImgDecStatus *)status;
	unsigned char *rowbuf;
	unsigned char *image;
	int dataOffset;
	int stride;
	int row;
	unsigned char *src;
	unsigned char *dst;

	if (format == 2) {
		stride = width * 4;
		rowbuf = (unsigned char *)malloc(stride);
		int rowBytes = width * 3;
		image = new unsigned char[rowBytes * height];
		if (rowbuf && image) {
			dataOffset = 40;
			if (m_hasFileHeader)
				dataOffset = m_bfh.bfOffBits;
			for (row = height - 1; row >= 0; row--, dataOffset += stride) {
				if (prog->cancel == 1) {
					free(rowbuf);
					if (image)
						delete[] image;
					*images = 0;
					return EICL_ERR_BAD_DATA;
				}
				m_ioFilter->Read(dataOffset, stride, rowbuf);
				src = rowbuf;
				dst = image + row * width * 3;
				for (int col = 0; col < width; col++) {
					*dst++ = src[2];
					*dst++ = src[1];
					*dst++ = src[0];
					src += 4;
				}
				prog->percent = (height - row) * 100 / height;
			}
			err = 0;
			free(rowbuf);
		} else {
			if (rowbuf)
				free(rowbuf);
			if (image)
				delete[] image;
			err = EICL_ERR_NO_MEMORY;
		}
	} else {
		stride = width * 4;
		rowbuf = (unsigned char *)malloc(stride);
		image = new unsigned char[stride * height];
		if (rowbuf && image) {
			dataOffset = 40;
			if (m_hasFileHeader)
				dataOffset = m_bfh.bfOffBits;
			for (row = height - 1; row >= 0; row--, dataOffset += stride) {
				int fill = flags & theImageDecoder::fill_alpha;

				m_ioFilter->Read(dataOffset, stride, rowbuf);
				if (prog->cancel == 1) {
					free(rowbuf);
					if (image)
						delete[] image;
					*images = 0;
					return EICL_ERR_BAD_DATA;
				}
				src = rowbuf;
				dst = image + row * stride;

				if (fill) {
					for (int col = 0; col < width; col++) {
						*dst++ = src[3];
						*dst++ = src[2];
						*dst++ = src[1];
						*dst++ = (unsigned char)alpha;
						src += 4;
					}
				} else {
					for (int col = 0; col < width; col++) {
						*dst++ = src[3];
						*dst++ = src[2];
						*dst++ = src[1];
						*dst++ = src[0];
						src += 4;
					}
				}
				prog->percent = (height - row) * 100 / height;
			}
			err = 0;
			free(rowbuf);
		} else {
			if (rowbuf)
				free(rowbuf);
			if (image)
				delete[] image;
			err = EICL_ERR_NO_MEMORY;
		}
	}
	if (err) {
		if (image)
			delete[] image;
		*images = 0;
	} else {
		*images = image;
	}
	return err;
}

/* 32 bpp BI_BITFIELDS into *images (status is not used). A short read abandons only the
   current row. */
int InputDataFilterBMP::GetRawData32BitMasked(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
    unsigned char **images, void *status)
{
	unsigned int mask[3];
	unsigned char px[4];
	int pix;
	int err = 0;
	unsigned char *image = 0;
	int shiftR;
	int shiftG;
	int shiftB;
	int n;
	int m;

	n = m_ioFilter->Read(4, (unsigned char *)&mask[0]);
	if (n == 4 && (m = m_ioFilter->Read(4, (unsigned char *)&mask[1])) == n
	    && m_ioFilter->Read(4, (unsigned char *)&mask[2]) == m) {
		{
			int width;
			int height;
			unsigned int v;

			v = mask[0];
			shiftR = 0;
			if ((v & 1) == 0) {
				do {
					shiftR++;
					v >>= 1;
				} while ((v & 1) == 0);
				mask[0] = v;
			}
			v = mask[1];
			shiftG = 0;
			if ((v & 1) == 0) {
				do {
					shiftG++;
					v >>= 1;
				} while ((v & 1) == 0);
				mask[1] = v;
			}
			v = mask[2];
			shiftB = 0;
			if ((v & 1) == 0) {
				do {
					shiftB++;
					v >>= 1;
				} while ((v & 1) == 0);
				mask[2] = v;
			}
			width = m_bih.biWidth;
			height = m_bih.biHeight;
			if (format == 2) {
				int rowBytes = width * 3;

				image = new unsigned char[rowBytes * height];
				if (image != 0) {
					for (int row = height - 1; row >= 0; row--) {
						unsigned char *dst = image + row * rowBytes;

						for (int col = 0; col < width; col++) {
							if (m_ioFilter->Read(4, px) != 4) {
								err = 258;
								goto nextRow;
							}
							pix = (px[2] << 16) + (px[1] << 8) + px[0];
							/* NOTE: bug: the masks of the first and third channel are swapped in the AND. */
							*dst++ = ((pix >> shiftR) & mask[2]) * 255 / mask[0];
							*dst++ = ((pix >> shiftG) & mask[1]) * 255 / mask[1];
							*dst++ = ((pix >> shiftB) & mask[0]) * 255 / mask[2];
						}
nextRow:;
					}
				} else {
					err = EICL_ERR_NO_MEMORY;
				}
			} else {
				int rowBytes = width * 4;

				image = new unsigned char[rowBytes * height];
				if (image != 0) {
					for (int row = height - 1; row >= 0; row--) {
						unsigned char *dst = image + row * rowBytes;

						for (int col = 0; col < width; col++) {
							if (m_ioFilter->Read(4, px) != 4) {
								err = 258;
								goto nextRow2;
							}
							pix = (px[2] << 16) + (px[1] << 8) + px[0];
							*dst++ = ((pix >> shiftR) & mask[2]) * 255 / mask[0];
							*dst++ = ((pix >> shiftG) & mask[1]) * 255 / mask[1];
							*dst++ = ((pix >> shiftB) & mask[0]) * 255 / mask[2];
							*dst++ = (unsigned char)alpha;
						}
nextRow2:;
					}
				} else {
					err = EICL_ERR_NO_MEMORY;
				}
			}
		}
	} else {
		err = 258;
	}
	if (err) {
		if (image)
			delete[] image;
		*images = 0;
	} else {
		*images = image;
	}
	return err;
}

/* 8 bpp indexed: *images receives the pixel indices (top-down), *cluts the
   1024-byte RGBQUAD palette. On error all buffers are freed and both cleared. */
int InputDataFilterBMP::GetRawData8Bit(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
    unsigned char **images, unsigned char **cluts, void *status)
{
	int err;
	int paletteCount;
	if (m_bih.biClrUsed)
		paletteCount = m_bih.biClrUsed;
	else
		paletteCount = 1 << m_bih.biBitCount;
	unsigned char *palette = new unsigned char[1024];
	if (!palette)
		return EICL_ERR_NO_MEMORY;
	m_ioFilter->Read(paletteCount * 4, palette);
	int width = m_bih.biWidth;
	int height = m_bih.biHeight;
	int roundedStride = (width + 3) & ~3;
	unsigned char *rowbuf = (unsigned char *)malloc(roundedStride);
	unsigned char *image = new unsigned char[width * height];
	if (rowbuf && image) {
		err = 0;
		for (int row = height - 1; row >= 0; row--) {
			if (m_ioFilter->Read(roundedStride, rowbuf) == roundedStride) {
				memcpy(image + row * width, rowbuf, width);
			} else {
				err = 258;
				break;
			}
		}
		free(rowbuf);
		rowbuf = 0;
	} else {
		if (rowbuf) {
			free(rowbuf);
			rowbuf = 0;
		}
		if (image) {
			delete[] image;
			image = 0;
		}
		if (palette) {
			delete[] palette;
			palette = 0;
		}
		err = EICL_ERR_NO_MEMORY;
	}
	if (err) {
		if (rowbuf)
			free(rowbuf);
		if (image)
			delete[] image;
		if (palette)
			delete[] palette;
		*images = 0;
		*cluts = 0;
	} else {
		*images = image;
		*cluts = palette;
	}
	return err;
}
