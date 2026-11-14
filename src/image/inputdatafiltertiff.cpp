/*
 * TIFF decoder (InputDataFilterTIFF) on top of libtiff's TIFFReadRGBAImage,
 * reading through InputIoFilter via the TIFFReadHandler client callbacks.
 */

#include "inputdatafiltertiff.h"
#include "eiclerror.h"
#include <new>
#include <stdio.h>

#include "../../ref/tiff-3.4beta037/tiffio.h"

#include "theimagedecoder.h"

/* Bridge between the libtiff client I/O callbacks and InputIoFilter. The
   handle is the address of the variable holding the filter (m_handle).
   write/close/map are stubs (read-only stream), unmap does nothing. */
class TIFFReadHandler {
public:
	static tsize_t read(thandle_t fd, tdata_t buf, tsize_t size)
	{
		InputIoFilter *filter = *(InputIoFilter **)fd;
		return filter->Read((unsigned int)size, (unsigned char *)buf);
	}
	static tsize_t write(thandle_t fd, tdata_t buf, tsize_t size) { return 0; }
	static toff_t seek(thandle_t fd, toff_t off, int whence)
	{
		if (!(*(InputIoFilter **)fd)->Seek((unsigned int)off, whence))
			return -1;
		return (*(InputIoFilter **)fd)->GetCurrentPosition();
	}
	static int close(thandle_t fd) { return 0; }
	static toff_t size(thandle_t fd)
	{
		InputIoFilter *filter = *(InputIoFilter **)fd;
		return filter->Size();
	}
	static int map(thandle_t fd, tdata_t *base, toff_t *sz) { return 0; }
	static void unmap(thandle_t fd, tdata_t base, toff_t sz) {}
};

/* Rewinds the filter and opens the client stream in mode "rm" (read, no mmap). */
static inline TIFF *OpenTIFF(InputIoFilter **handle)
{
	(*handle)->Seek(0);
	return TIFFClientOpen((*handle)->GetRealFileName(), "rm", handle, TIFFReadHandler::read, TIFFReadHandler::write,
	    TIFFReadHandler::seek, TIFFReadHandler::close, TIFFReadHandler::size, TIFFReadHandler::map,
	    TIFFReadHandler::unmap);
}

InputDataFilterTIFF::InputDataFilterTIFF() : InputDataFilter()
{
	m_ioFilter = 0;
	m_handle = 0;
}

InputDataFilterTIFF::InputDataFilterTIFF(InputIoFilter *ioFilter) : InputDataFilter()
{
	m_ioFilter = ioFilter;
	m_handle = new InputIoFilter *;
	/* NOTE: bug: m_handle is used before the NULL check. */
	*m_handle = m_ioFilter;
	if (!m_handle)
		printf("Memory InputDataFilterTIFF Error\n");
}

InputDataFilterTIFF::~InputDataFilterTIFF()
{
	::operator delete(m_handle);
	m_handle = 0;
}

/* Asks libtiff whether the image can be read as RGBA. */
int InputDataFilterTIFF::IsSupported()
{
	int ok;
	if (m_ioFilter) {
		TIFF *tif = OpenTIFF(m_handle);
		if (tif) {
			char emsg[1024];
			ok = TIFFRGBAImageOK(tif, emsg) != 0;
			TIFFClose(tif);
		} else {
			ok = 0;
		}
	} else {
		ok = 0;
	}
	return ok;
}

/* Decodes with TIFFReadRGBAImage (bottom-up raster) into a new ImageRGBA.
   With the DOTRANGE tag each channel is rescaled to 0..255 from the tag's
   range; otherwise the raster is copied as is. */
int InputDataFilterTIFF::GetRawData(ImageRGBA **out, int *outWidth, int *outHeight, unsigned char alpha)
{
	int ret;
	if (m_ioFilter) {
		TIFF *tif = OpenTIFF(m_handle);
		if (tif) {
			unsigned short dot[2];
			char emsg[1024];
			int width, height;
			int hasDot;
			uint32 *raster;
			uint32 *p;
			int x, y;
			double range;

			TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
			TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
			hasDot = TIFFGetField(tif, TIFFTAG_DOTRANGE, &dot[0], &dot[1]) != 0;
			*outWidth = width;
			*outHeight = height;
			/* NOTE: bug: ret is left uninitialized when TIFFRGBAImageOK fails. */
			if (TIFFRGBAImageOK(tif, emsg)) {
				raster = (uint32 *)_TIFFmalloc(width * height * 4);
				if (raster == 0)
					goto bad;
				if (TIFFReadRGBAImage(tif, width, height, raster, 0, 0) == 0)
					goto bad;
				*out = new ImageRGBA(width, height, alpha, ImageRGBA::RGBA8888);
				p = raster;
				if (*out == 0)
					goto noImage;
				if (hasDot != 1) {
					for (y = height - 1; y >= 0; y--) {
						for (x = 0; x < width; x++) {
							unsigned char *q = (unsigned char *)p;

							(*out)->SetPixel(x, y, q[0], q[1], q[2], q[3]);
							p++;
						}
					}
				} else {
					range = (double)(dot[1] - dot[0]);
					for (y = height - 1; y >= 0; y--) {
						for (x = 0; x < width; x++) {
							unsigned int pixel = *p;
							double d0, d1, d2, d3;
							unsigned char c0, c1, c2, c3;

							d0 = ((double)(pixel & 0xff) - (double)dot[0]) * 255.0 / range;
							d1 = ((double)((pixel >> 8) & 0xff) - (double)dot[0]) * 255.0
							    / range;
							d2 = ((double)((pixel >> 16) & 0xff) - (double)dot[0]) * 255.0
							    / range;
							d3 = ((double)(pixel >> 24) - (double)dot[0]) * 255.0 / range;
							p++;
							if (d0 > 255.0)
								c0 = 255;
							else if (d0 < 0.0)
								c0 = 0;
							else
								c0 = (unsigned char)(unsigned long)d0;
							if (d1 > 255.0)
								c1 = 255;
							else if (d1 < 0.0)
								c1 = 0;
							else
								c1 = (unsigned char)(unsigned long)d1;
							if (d2 > 255.0)
								c2 = 255;
							else if (d2 < 0.0)
								c2 = 0;
							else
								c2 = (unsigned char)(unsigned long)d2;
							if (d3 > 255.0)
								c3 = 255;
							else if (d3 < 0.0)
								c3 = 0;
							else
								c3 = (unsigned char)(unsigned long)d3;
							(*out)->SetPixel(x, y, c0, c1, c2, c3);
						}
					}
				}
				ret = 0;
				goto freeRaster;
noImage:
				ret = EICL_ERR_NO_MEMORY;
				goto freeRaster;
bad:
				ret = raster == 0 ? 16 : 4097;
freeRaster:
				if (raster)
					_TIFFfree(raster);
			}
			TIFFClose(tif);
		} else {
			ret = EICL_ERR_STREAM;
		}
	} else {
		ret = EICL_ERR_STREAM;
	}
	return ret;
}

/* Fills props from the TIFF tags: format 512, resolution -1 when the
   resolution tags are missing, 72 dpi for RESUNIT_NONE, centimeters
   converted to inches. */
int InputDataFilterTIFF::GetProps(PgProps *props)
{
	int ret;
	if (m_ioFilter) {
		TIFF *tif = OpenTIFF(m_handle);
		if (tif) {
			int width, height;
			float xres, yres;
			int resUnit;

			props->SetFormat((PgProps::PgFormat)512);
			props->SetImageType(PgProps::kImageTypeDefault);

			if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width) != 1)
				goto badSize;
			if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height) != 1)
				goto badSize;
			props->SetSize(width, height);
			goto sizeDone;
badSize:
			ret = 0;
sizeDone:

			if (TIFFGetFieldDefaulted(tif, TIFFTAG_XRESOLUTION, &xres) == 1
			    && TIFFGetFieldDefaulted(tif, TIFFTAG_YRESOLUTION, &yres) == 1
			    && TIFFGetFieldDefaulted(tif, TIFFTAG_RESOLUTIONUNIT, &resUnit) == 1) {
				double dpiX = xres;
				double dpiY = yres;
				if (resUnit == RESUNIT_NONE) {
					dpiX = 72.0;
					dpiY = 72.0;
				} else if (resUnit == RESUNIT_CENTIMETER) {
					dpiX *= 2.54;
					dpiY *= 2.54;
				}
				props->SetResolution(dpiX, dpiY);
			} else {
				props->SetResolution(-1.0, -1.0);
			}
			TIFFClose(tif);
		} else {
			ret = 0;
		}
	} else {
		ret = 0;
	}
	/* NOTE: bug: ret is never set on success. */
	return ret;
}

/* Raw-buffer variant: decodes with TIFFReadRGBAImage (32-bit ABGR raster,
   bottom-up) and flips it into *images. dstWidth, dstHeight: requested size
   (0 = image size); format: 0 = 3; 3 = 32 bpp, 2 = RGB888; alpha: alpha value;
   flags: option mask; srcFormat: format produced; srcWidth, srcHeight: image
   size; status: the TIFFProgress (cancel == 1 aborts).
   Returns EICL_ERR_NO_MEMORY (cancelled or out of memory), EICL_ERR_FORMAT (bad format or size),
   EICL_ERR_STREAM (no stream or TIFFClientOpen failed), EICL_ERR_BAD_DATA (TIFFReadRGBAImage failed),
   0 on success. */
int InputDataFilterTIFF::GetRawData(int *dstWidth, int *dstHeight, int format, int alpha, unsigned int flags,
    unsigned char **images, unsigned char **cluts, int *srcFormat, int *srcWidth, int *srcHeight, void *status)
{
	int ret;
	unsigned short dot[2];
	char emsg[1024];
	int width, height, planarConfig;
	TIFF *tif = 0;
	uint32 *raster = 0;
	unsigned char *out = 0;
	TIFFProgress *progress = (TIFFProgress *)status;

	(void)cluts;

	if (progress->cancel == 1)
		return EICL_ERR_NO_MEMORY;
	if (format == 0)
		format = 3;
	progress->percent = 0;
	*srcFormat = 3;
	if (format != 2 && format != 3)
		return EICL_ERR_FORMAT;

	if (m_ioFilter) {
		tif = OpenTIFF(m_handle);
		if (tif) {
			int hasDot;

			TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
			TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
			TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &planarConfig);
			*srcWidth = width;
			*srcHeight = height;
			if (*dstWidth == 0 || *dstHeight == 0) {
				*dstWidth = width;
				*dstHeight = height;
			} else {
				if (*dstWidth != width) {
					TIFFClose(tif);
					return EICL_ERR_FORMAT;
				}
				if (*dstHeight != height) {
					TIFFClose(tif);
					return EICL_ERR_FORMAT;
				}
			}
			hasDot = TIFFGetField(tif, TIFFTAG_DOTRANGE, &dot[0], &dot[1]) != 0;
			/* NOTE: bug: ret is left uninitialized when TIFFRGBAImageOK fails. */
			if (TIFFRGBAImageOK(tif, emsg)) {
				if (format == 3) {
					raster = new uint32[width * height];
					out = (unsigned char *)new unsigned int[width * height];
					if (raster != 0 && TIFFReadRGBAImage(tif, width, height, raster, 0, progress)) {
						int x, y;
						unsigned int *dst = 0;
						uint32 *src = 0;

						src = raster;
						if (hasDot != 1) {
							if (flags & theImageDecoder::fill_alpha) {
								/* NOTE: bug: format 3 stores 0 or 1 per pixel instead of the pixel. */
								for (y = height - 1; y >= 0; y--) {
									dst = (unsigned int *)out + width * y;
									for (x = 0; x < width; x++) {
										int v = 0;
										unsigned int *q = dst++;
										unsigned int p = *src++;

										if ((p & 0x00ffffff) || (alpha << 24))
											v = 1;
										*q = v;
									}
								}
							} else {
								for (y = height - 1; y >= 0; y--) {
									dst = (unsigned int *)out + width * y;
									for (x = 0; x < width; x++)
										*dst++ = *src++;
								}
							}
						} else {
							double range = (double)(dot[1] - dot[0]);

							for (y = height - 1; y >= 0; y--) {
								unsigned int p;
								double dr, dg, db, da;
								int cr, cg, cb, ca;
								int fill = flags & theImageDecoder::fill_alpha;

								dst = (unsigned int *)out + width * y;
								for (x = 0; x < width; x++) {
									/* NOTE: bug: always reads the first raster pixel; the source never advances. */
									p = *raster;
									dr = ((double)(p & 0xff) - (double)dot[0])
									    * 255.0 / range;
									dg =
									    ((double)((p >> 8) & 0xff) - (double)dot[0])
									    * 255.0 / range;
									db = ((double)((p >> 16) & 0xff)
									         - (double)dot[0])
									    * 255.0 / range;
									da = ((double)(p >> 24) - (double)dot[0])
									    * 255.0 / range;

									if (dr > 255.0)
										cr = 255;
									else if (dr < 0.0)
										cr = 0;
									else
										cr = (unsigned char)dr;
									if (dg > 255.0)
										cg = 255;
									else if (dg < 0.0)
										cg = 0;
									else
										cg = (unsigned char)dg;
									if (db > 255.0)
										cb = 255;
									else if (db < 0.0)
										cb = 0;
									else
										cb = (unsigned char)db;
									if (da > 255.0)
										ca = 255;
									else if (da < 0.0)
										ca = 0;
									else
										ca = (unsigned char)da;

									if (fill) {
										int v = 0;
										unsigned int *q = dst++;

										if ((alpha << 24) || cb || cg || cr)
											v = 1;
										*q = v;
									} else {
										int v = 0;
										unsigned int *q = dst++;

										if (ca || cb || cg || cr)
											v = 1;
										*q = v;
									}
								}
							}
						}
						*images = out;
						out = 0;
						ret = 0;
					} else {
						ret = (raster == 0) ? 16 : 4097;
					}
				} else if (format == 2) {
					raster = new uint32[width * height];
					out = new unsigned char[width * height * 3];
					if (raster != 0 && out != 0
					    && TIFFReadRGBAImage(tif, width, height, raster, 0, progress)) {
						int x, y;

						if (hasDot != 1) {
							unsigned char *dst = 0;
							unsigned char *src = 0;

							src = (unsigned char *)raster;
							for (y = height - 1; y >= 0; y--) {
								dst = out + width * 3 * y;
								for (x = 0; (unsigned int)x < (unsigned int)width;
								    x++) {
									*dst++ = *src++;
									*dst++ = *src++;
									*dst++ = *src++;
								}
							}
						} else {
							double range = (double)(dot[1] - dot[0]);

							for (y = height - 1; y >= 0; y--) {
								unsigned char *dst = out + width * 3 * y;
								for (x = 0; x < width; x++) {
									/* NOTE: bug: always reads the first raster pixel; the source never advances. */
									unsigned int p = *raster;
									double dr =
									    ((double)(p & 0xff) - (double)dot[0])
									    * 255.0 / range;
									double dg =
									    ((double)((p >> 8) & 0xff) - (double)dot[0])
									    * 255.0 / range;
									double db = ((double)((p >> 16) & 0xff)
									                - (double)dot[0])
									    * 255.0 / range;
									double da = ((double)(p >> 24) - (double)dot[0])
									    * 255.0 / range;
									int cr, cg, cb, ca;

									if (dr > 255.0)
										cr = 255;
									else if (dr < 0.0)
										cr = 0;
									else
										cr = (unsigned char)dr;
									if (dg > 255.0)
										cg = 255;
									else if (dg < 0.0)
										cg = 0;
									else
										cg = (unsigned char)dg;
									if (db > 255.0)
										cb = 255;
									else if (db < 0.0)
										cb = 0;
									else
										cb = (unsigned char)db;
									if (da > 255.0)
										ca = 255;
									else if (da < 0.0)
										ca = 0;
									else
										ca = (unsigned char)da;

									*dst++ = cr;
									*dst++ = cg;
									*dst++ = cb;
								}
							}
						}
						*images = out;
						out = 0;
						ret = 0;
					} else {
						ret = (raster == 0) ? 16 : 4097;
					}
				} else {
					TIFFClose(tif);
					return EICL_ERR_FORMAT;
				}
				delete[] raster;
				delete[] out;
			}
			TIFFClose(tif);
			goto done;
		}
	}
	ret = EICL_ERR_STREAM;
done:
	progress->percent = 100;
	return ret;
}
