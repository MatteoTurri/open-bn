/*
 * PgProps: image properties (size, resolution, format, color space, rotation,
 * flips, TIFF compression, quality, EXIF tags).
 */

#ifndef PGPROPS_H
#define PGPROPS_H

#include "eiclsize.h"

class PgProps {
public:
	enum ColorSpace {
		kRGB = 0,
		kRGBA = 1,
		kCMYK = 2,
		kCMYK32 = 3,
		kGrayscale = 4,
		kMonochrome = 5,
		kUnknownColorSpace = 6
	};

	enum PgFormat {
		kFormatDefault = (int)0x80000000
	};

	enum ImageType {
		kImageTypeDefault = 0
	};

	enum TiffCompression {
		kTiffCompressionDefault = 0
	};

	/* 0x00 */ EiclSize m_size;
	/* 0x08 */ double m_secRes;  /* second SetResolution argument */
	/* 0x10 */ double m_priRes;  /* first SetResolution argument, also used by the one-argument accessors */
	/* 0x18 */ int m_bitsPerPixel;  /* default 24 */
	/* 0x1C */ PgFormat m_format;  /* default kFormatDefault (unset) */
	/* 0x20 */ ImageType m_imageType;
	/* 0x24 */ ColorSpace m_colorSpace;
	/* 0x28 */ double m_rotationAngle;
	/* 0x30 */ int m_verticalFlip;
	/* 0x34 */ int m_horizontalFlip;
	/* 0x38 */ TiffCompression m_tiffCompression;
	/* 0x3C */ int m_compressionQuality;  /* default 2 */
	/* 0x40 */ void *m_exifTags;

	PgProps();
	~PgProps();
	void InitVariables_();

	bool SetColorSpace(ColorSpace cs);
	ColorSpace GetColorSpace();
	ColorSpace GetColorSpaceName(char *name);

	bool SetSize(EiclSize sz);
	bool SetSize(int w, int h);
	bool GetSize(EiclSize *out);

	bool SetResolution(double a, double b);
	bool SetResolution(double r);
	bool GetResolution(double *a, double *b);
	bool GetResolution(double *a);

	int GetBitsPerPixel();
	bool SetBitsPerPixel(int bpp);

	bool SetFormat(PgFormat f);
	PgFormat GetFormat();

	bool SetImageType(ImageType t);
	ImageType GetImageType();

	bool Copy(PgProps *other);

	bool SetRotationAngle(double angle);
	double GetRotationAngle();

	bool SetVerticalFlip(int flip);
	int GetVerticalFlip();
	bool SetHorizontalFlip(int flip);
	int GetHorizontalFlip();

	void SetTiffCompression(TiffCompression c);
	TiffCompression GetTiffCompression();

	bool SetCompressionQuality(int q);
	int GetCompressionQuality();

	bool RegisterExifTags(void *tags);
	void *GetExifTags();
};

#endif
