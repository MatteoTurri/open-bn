/*
 * ExifReader: finds the EXIF APP1 segment of a JPEG stream and collects the
 * supported tags of its TIFF IFDs.
 */

#pragma implementation "exifreader.h"
#include "exifreader.h"

/* throw() as in the declarations of the kit <string.h>. */
#include <string.h>

#include <iostream.h>

const char *ExifIdentifierCode = "Exif\0";

bool ExifReader::IsSupportedTag(Exif::tags tag)
{
	Exif::tags table[13] = {
		Exif::TAG_MAKE,
		Exif::TAG_MODEL,
		Exif::TAG_ORIENTATION,
		Exif::TAG_DATETIME,
		Exif::TAG_FNUMBER,
		Exif::TAG_ISOSPEEDRATINGS,
		Exif::TAG_DATETIMEORIGINAL,
		Exif::TAG_DATETIMEDIGITIZED,
		Exif::TAG_SHUTTERSPEEDVALUE,
		Exif::TAG_EXPOSUREBIASVALUE,
		Exif::TAG_LIGHTSOURCE,
		Exif::TAG_FLASH,
		Exif::TAG_FOCALLENGTH,
	};

	for (unsigned i = 0; i < 13; i++) {
		if (tag == table[i])
			return true;
	}
	return false;
}

/* m_base is set later by SetIDFScanpoint; the byte order defaults to
 * big-endian until the TIFF header is read. */
ExifReader::ExifReader(InputIoFilter *filter) : m_ioFilter(filter)
{
	m_buf = 0;
	m_cap = 0;
	m_cur = 0;
	m_len = 0;
	m_bigEndian = 1;
}

// Frees the read buffer and every TagData in m_tags.
ExifReader::~ExifReader()
{
	if (m_buf) {
		delete[] m_buf;
		m_buf = 0;
	}
	TagData **end = m_tags.end();
	for (TagData **it = m_tags.begin(); it < end; it++)
		delete *it;
}

/* ===== ExifReader ===== */

ExifReader::TagData *ExifReader::SearchTag(Exif::tags tag) const
{
	TagData *const *it = m_tags.begin();
	TagData *const *end = m_tags.end();
	while (it < end) {
		if ((*it)->tag == tag)
			return *it;
		it++;
	}
	return 0;
}

/* Checks for the JPEG SOI and an EXIF APP1 segment, then parses the TIFF
 * header and walks the IFD chain. */
bool ExifReader::Parse()
{
	unsigned char buf[6];
	unsigned len;
	unsigned order;
	unsigned offset;

	m_ioFilter->Seek(0);
	if (ChargeBuffer(12) < 12)
		return false;
	if (ReadU16(true) != 0xffd8)          /* SOI */
		return false;
	if (ReadU16(true) != 0xffe1)          /* APP1 */
		return false;
	len = ReadU16(true);
	ReadBytes(buf, 6);
	if (memcmp(buf, ExifIdentifierCode, 6) != 0)
		return false;
	if (ChargeBuffer(len + 4) < len + 4)
		return false;

	m_base = m_cur;
	order = ReadU16(true);
	if (order != 0x4d4d && order != 0x4949) {
		cerr << "Unknown TIFF byte order identifier: " << order << endl;
		return false;
	}
	m_bigEndian = (order == 0x4d4d);

	if (ReadU16(false) != 42) {
		cerr << "Broken TIFF header" << endl;
		return false;
	}
	if (ReadU32(false) != 8) {
		cerr << "Broken TIFF header" << endl;
		return false;
	}
	offset = 8;
	do {
		offset = ParseIDF(offset);
	} while (offset != 0);
	return true;
}

/* Reads the IFD at offset n (tag count, the tags, offset of the next IFD)
 * and returns the next offset; the read position is restored. */
int ExifReader::ParseIDF(unsigned n)
{
	unsigned char *old = SetIDFScanpoint(n);
	int num = ReadU16(false);
	for (int i = 0; i < num; i++)
		ParseTag();
	unsigned next = ReadU32(false);
	PutBackIDFScanpoint(old);
	return next;
}

/* Reads one 12-byte IFD entry: tag, type, count, then the value field. The
   whole field is consumed but only its first value is kept; count is
   unused. ExifIFDPointer recurses into its IFD; a supported tag becomes a
   TagData (scalar, rational or string, the last two read at the offset). */
void ExifReader::ParseTag()
{
	Exif::tags tag = (Exif::tags)ReadU16(false);
	Exif::tiff_tag_types type = (Exif::tiff_tag_types)ReadU16(false);
	unsigned value;

	ReadU32(false);
	if (type == Exif::TIFF_BYTE) {
		value = ReadU8(false);
		ReadU8(false);
		ReadU8(false);
		ReadU8(false);
	} else if (type == Exif::TIFF_SHORT) {
		value = ReadU16(false);
		ReadU16(false);
	} else {
		value = ReadU32(false);
	}

	if (tag == Exif::TAG_EXIFIFDPOINTER) {
		ParseIDF(value);
		return;
	}
	if (!IsSupportedTag(tag))
		return;

	if (type == Exif::TIFF_BYTE || type == Exif::TIFF_SHORT || type == Exif::TIFF_LONG
	    || type == Exif::TIFF_SLONG) {
		m_tags.push_back(new TagData(tag, type, value));
	} else if (type == Exif::TIFF_RATIONAL || type == Exif::TIFF_SRATIONAL) {
		unsigned char *old = SetIDFScanpoint(value);
		unsigned num = ReadU32(false);
		unsigned den = ReadU32(false);

		m_tags.push_back(new TagData(tag, type, num, den));
		PutBackIDFScanpoint(old);
	} else if (type == Exif::TIFF_ASCII) {
		unsigned char *old = SetIDFScanpoint(value);

		m_tags.push_back(new TagData(tag, type, (const char *)m_cur));
		PutBackIDFScanpoint(old);
	}
}

unsigned char *ExifReader::SetIDFScanpoint(unsigned n)
{
	unsigned char *old = m_cur;
	m_cur = m_base + n;
	return old;
}

void ExifReader::PutBackIDFScanpoint(void *p)
{
	m_cur = (unsigned char *)p;
}

/* Refills the read buffer with n bytes, keeping the unread tail: grows the
 * buffer if needed, otherwise compacts it in place, then reads the missing
 * bytes from the filter. Returns the valid byte count. */
unsigned ExifReader::ChargeBuffer(unsigned n)
{
	unsigned remaining;
	unsigned char *dst;

	if (m_cur != 0)
		remaining = m_len - (m_cur - m_buf);
	else
		remaining = 0;

	if (m_cap < n) {
		unsigned char *base;

		dst = new unsigned char[n];
		m_cap = n;
		base = dst;
		if (dst == 0)
			return 0;
		if (remaining != 0) {
			memcpy(dst, m_cur, remaining);
			dst += remaining;
		}
		if (m_buf != 0) {
			delete[] m_buf;
			m_buf = 0;
		}
		m_len = m_ioFilter->Read(n - remaining, dst) + remaining;
		m_buf = base;
		m_cur = base;
	} else {
		dst = m_buf;
		if (remaining != 0) {
			memmove(m_buf, m_cur, remaining);
			dst = m_buf + remaining;
		}
		m_len = m_ioFilter->Read(n - remaining, dst) + remaining;
		m_cur = m_buf;
	}
	return m_len;
}

unsigned char ExifReader::ReadU8(bool bigEndian)
{
	return *m_cur++;
}

/* Big-endian when asked or when the TIFF header says so (m_bigEndian). */
unsigned short ExifReader::ReadU16(bool bigEndian)
{
	unsigned short r = *m_cur++;
	if (bigEndian || m_bigEndian) {
		r <<= 8;
		r |= *m_cur++;
	} else {
		r |= *m_cur++ << 8;
	}
	return r;
}

unsigned ExifReader::ReadU32(bool bigEndian)
{
	unsigned r = *m_cur++;

	if (bigEndian || m_bigEndian) {
		r <<= 8;
		r |= *m_cur++;
		r <<= 8;
		r |= *m_cur++;
		r <<= 8;
		r |= *m_cur++;
	} else {
		r |= *m_cur++ << 8;
		r |= *m_cur++ << 16;
		r |= *m_cur++ << 24;
	}
	return r;
}

/* NOTE: bug: declared int but returns no value. */
int ExifReader::ReadBytes(unsigned char *buf, unsigned n)
{
	memcpy(buf, m_cur, n);
	m_cur += n;
}

/* ===== TagData ===== */

ExifReader::TagData::TagData(Exif::tags t, Exif::tiff_tag_types ty, unsigned v)
{
	tag = t;
	type = ty;
	value = v;
	str = 0;
}

ExifReader::TagData::TagData(Exif::tags t, Exif::tiff_tag_types ty, unsigned v, unsigned d)
{
	tag = t;
	type = ty;
	value = v;
	str = 0;
	denom = d;
}

ExifReader::TagData::TagData(Exif::tags t, Exif::tiff_tag_types ty, const char *s)
{
	tag = t;
	type = ty;
	str = new char[strlen(s) + 1];
	strcpy(str, s);
}

ExifReader::TagData::~TagData()
{
	if (str) {
		delete[] str;
		str = 0;
	}
}

unsigned ExifReader::TagData::GetValue() const
{
	return value;
}

void ExifReader::TagData::GetRational(unsigned &num, unsigned &den) const
{
	num = value;
	den = denom;
}

float ExifReader::TagData::GetRational() const
{
	if (type == Exif::TIFF_SRATIONAL)
		return (float)(int)value / (float)(int)denom;
	return (float)value / (float)denom;
}

const char *ExifReader::TagData::GetString() const
{
	return str;
}
