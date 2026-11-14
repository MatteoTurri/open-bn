/*
 * ExifReader: a minimal EXIF/TIFF tag reader over an InputIoFilter stream.
 */

#ifndef EXIFREADER_H
#define EXIFREADER_H

#include <vector>
#include "iofilter.h"

namespace Exif {

enum tags {
	TAG_MAKE = 271,
	TAG_MODEL = 272,
	TAG_ORIENTATION = 274,
	TAG_XRESOLUTION = 282,
	TAG_YRESOLUTION = 283,
	TAG_RESOLUTIONUNIT = 296,
	TAG_DATETIME = 306,
	TAG_YCBCRPOSITIONING = 531,
	TAG_EXIFIFDPOINTER = 34665,
	TAG_EXPOSURETIME = 33434,
	TAG_FNUMBER = 33437,
	TAG_FLASH = 37385,
	TAG_FOCALLENGTH = 37386,
	TAG_ISOSPEEDRATINGS = 34855,
	TAG_DATETIMEORIGINAL = 36867,
	TAG_DATETIMEDIGITIZED = 36868,
	TAG_SHUTTERSPEEDVALUE = 37377,
	TAG_EXPOSUREBIASVALUE = 37380,
	TAG_LIGHTSOURCE = 37384,
};

enum tiff_tag_types {
	TIFF_BYTE = 1,
	TIFF_ASCII = 2,
	TIFF_SHORT = 3,
	TIFF_LONG = 4,
	TIFF_RATIONAL = 5,
	TIFF_SBYTE = 6,
	TIFF_UNDEFINED = 7,
	TIFF_SSHORT = 8,
	TIFF_SLONG = 9,
	TIFF_SRATIONAL = 10,
	TIFF_FLOAT = 11,
	TIFF_DOUBLE = 12
};

} /* namespace Exif */

class ExifReader {
public:
	class TagData {
	public:
		TagData(Exif::tags tag, Exif::tiff_tag_types type, unsigned value);
		TagData(Exif::tags tag, Exif::tiff_tag_types type, unsigned value, unsigned denom);
		TagData(Exif::tags tag, Exif::tiff_tag_types type, const char *str);
		~TagData();

		unsigned GetValue() const;
		float GetRational() const;
		void GetRational(unsigned &num, unsigned &denom) const;
		const char *GetString() const;

		/* 0x00 */ Exif::tags tag;
		/* 0x04 */ Exif::tiff_tag_types type;
		/* 0x08 */ unsigned value;
		/* 0x0C */ unsigned denom;
		/* 0x10 */ char *str;
	}; /* size = 0x14 */

	ExifReader(InputIoFilter *filter);
	virtual ~ExifReader();

	TagData *SearchTag(Exif::tags tag) const;
	static bool IsSupportedTag(Exif::tags tag);

	unsigned char *SetIDFScanpoint(unsigned n);
	void PutBackIDFScanpoint(void *p);

	unsigned char ReadU8(bool bigEndian);
	unsigned short ReadU16(bool bigEndian);
	unsigned ReadU32(bool bigEndian);
	int ReadBytes(unsigned char *buf, unsigned n);

	bool Parse();
	void ParseTag();
	int ParseIDF(unsigned n);
	unsigned ChargeBuffer(unsigned n);

private:
	/* 0x00 */ InputIoFilter *m_ioFilter;
	/* 0x04 */ vector<TagData *> m_tags;
	/* 0x10 */ unsigned char *m_buf;  /* read buffer */
	/* 0x14 */ unsigned m_cap;  /* allocated bytes */
	/* 0x18 */ unsigned char *m_cur;  /* read cursor inside m_buf */
	/* 0x1C */ int m_len;  /* valid bytes in m_buf */
	/* 0x20 */ int m_bigEndian;  /* the TIFF header is big-endian (MM) */
	/* 0x24 */ unsigned char *m_base;  /* start of the current IFD, see SetIDFScanpoint */
}; /* size = 0x2C */

#endif
