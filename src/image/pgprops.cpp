/*
 * PgProps: constructor, defaults and the property accessors.
 */

#include "pgprops.h"

#include <string.h>

PgProps::PgProps()
{
	InitVariables_();
}

PgProps::~PgProps()
{}

void PgProps::InitVariables_()
{
	m_colorSpace = kRGB;
	m_size.SetSize(0, 0);
	m_priRes = 0;
	m_secRes = 0;
	m_rotationAngle = 0;
	m_bitsPerPixel = 24;
	m_format = kFormatDefault;
	m_imageType = kImageTypeDefault;
	m_verticalFlip = 0;
	m_horizontalFlip = 0;
	m_tiffCompression = kTiffCompressionDefault;
	m_compressionQuality = 2;
	m_exifTags = 0;
}

bool PgProps::SetColorSpace(ColorSpace cs)
{
	m_colorSpace = cs;
	return true;
}

PgProps::ColorSpace PgProps::GetColorSpace()
{
	return m_colorSpace;
}

PgProps::ColorSpace PgProps::GetColorSpaceName(char *name)
{
	char table[7][16] = { "RGB", "RGB", "CMYK", "CMYK228", "Grayscale", "Monochrome", "Unknown color" };
	ColorSpace cs = GetColorSpace();
	strcpy(name, table[cs]);
	return cs;
}

bool PgProps::SetSize(EiclSize sz)
{
	m_size.SetSize(sz);
	return true;
}

bool PgProps::SetSize(int w, int h)
{
	m_size.SetSize(w, h);
	return true;
}

bool PgProps::GetSize(EiclSize *out)
{
	if (!out) {
		return false;
	}
	out->SetSize(m_size);
	return true;
}

bool PgProps::SetResolution(double a, double b)
{
	m_priRes = a;
	m_secRes = b;
	return true;
}

bool PgProps::SetResolution(double r)
{
	m_priRes = r;
	m_secRes = r;
	return true;
}

bool PgProps::GetResolution(double *a, double *b)
{
	*a = m_priRes;
	*b = m_secRes;
	return true;
}

bool PgProps::GetResolution(double *a)
{
	*a = m_priRes;
	return true;
}

int PgProps::GetBitsPerPixel()
{
	return m_bitsPerPixel;
}

bool PgProps::SetBitsPerPixel(int bpp)
{
	m_bitsPerPixel = bpp;
	return true;
}

bool PgProps::SetFormat(PgFormat f)
{
	m_format = f;
	return true;
}

PgProps::PgFormat PgProps::GetFormat()
{
	return m_format;
}

bool PgProps::SetImageType(ImageType t)
{
	m_imageType = t;
	return true;
}

PgProps::ImageType PgProps::GetImageType()
{
	return m_imageType;
}

bool PgProps::Copy(PgProps *other)
{
	SetColorSpace(other->GetColorSpace());

	EiclSize sz;
	if (other->GetSize(&sz) == 1) {
		SetSize(EiclSize(&sz));
	}

	double a, b;
	other->GetResolution(&a, &b);
	SetResolution(a, b);

	SetFormat(other->GetFormat());
	SetImageType(other->GetImageType());
	SetBitsPerPixel(other->GetBitsPerPixel());

	return true;
}

bool PgProps::SetRotationAngle(double angle)
{
	m_rotationAngle = angle;
	return true;
}

double PgProps::GetRotationAngle()
{
	return m_rotationAngle;
}

bool PgProps::SetVerticalFlip(int flip)
{
	m_verticalFlip = flip;
	return true;
}

int PgProps::GetVerticalFlip()
{
	return m_verticalFlip;
}

bool PgProps::SetHorizontalFlip(int flip)
{
	m_horizontalFlip = flip;
	return true;
}

int PgProps::GetHorizontalFlip()
{
	return m_horizontalFlip;
}

void PgProps::SetTiffCompression(TiffCompression c)
{
	GetTiffCompression();
	m_tiffCompression = c;
}

PgProps::TiffCompression PgProps::GetTiffCompression()
{
	return m_tiffCompression;
}

bool PgProps::SetCompressionQuality(int q)
{
	m_compressionQuality = q;
	return true;
}

int PgProps::GetCompressionQuality()
{
	return m_compressionQuality;
}

bool PgProps::RegisterExifTags(void *tags)
{
	m_exifTags = tags;
	return true;
}

void *PgProps::GetExifTags()
{
	return m_exifTags;
}
