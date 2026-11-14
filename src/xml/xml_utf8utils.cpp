/*
 * UTF8Utils: UTF-8 encoding and decoding of one character and the name/NMTOKEN checks
 * on UTF-8 strings.
 */

#include "xml_inputstreamimpl.h"
#include "xml_utf8utils.h"
#include "xml_unicodeutils.h"

unsigned long UTF8Utils::mask[6] = {
	0x0000007F,
	0x000007FF,
	0x0000FFFF,
	0x001FFFFF,
	0x03FFFFFF,
	0x7FFFFFFF,
};
unsigned long UTF8Utils::mask_top[6] = {
	0x00,
	0xC0,
	0xE0,
	0xF0,
	0xF8,
	0xFC,
};
unsigned long UTF8Utils::mask_6bit = 0x3F;
unsigned long UTF8Utils::mask_8bit = 0x80;

unsigned long UTF8Utils::UTF8ToUnicodeChar(const unsigned char *buf, int nBytes, XMLCodeType codeType)
{
	if (buf == 0)
		return 0x3013;
	unsigned long m = 0;
	if (nBytes == 1)
		return buf[0];
	int i = 7 - nBytes;
	if (i > 0) {
		do {
			m <<= 1;
			m |= 1;
		} while (--i);
	}
	unsigned long result = buf[0] & m;
	for (int j = 1; j < nBytes; j++) {
		result <<= 6;
		result |= buf[j] & 0x3F;
	}
	return result;
}

unsigned short UTF8Utils::UnicodeToUTF8Char(unsigned char *out, unsigned long c)
{
	if (out == 0)
		return 0;
	int nBytes = 1;
	unsigned long *pm = mask;
	while (nBytes < 7 && *pm < c) {
		nBytes++;
		pm++;
	}
	for (int i = 1; i < nBytes; i++) {
		unsigned long b = (c & mask_6bit) | mask_8bit;
		out[nBytes - i] = b;
		c >>= 6;
	}
	out[0] = mask_top[nBytes - 1] | c;
	return nBytes;
}

int UTF8Utils::GetCharLength(unsigned char *buf, int byteLen)
{
	if (buf == 0)
		return 0;
	int charLen = 0;
	for (int i = 0; i < byteLen;) {
		int nBytes;
		if ((signed char)*buf >= 0)
			nBytes = 1;
		else
			nBytes = GetCharByte(*buf);
		if (nBytes == 0)
			break;
		charLen++;
		i += nBytes;
		buf += nBytes;
	}
	return charLen;
}

int UTF8Utils::GetCharByte(unsigned char c)
{
	if ((signed char)c >= 0)
		return 1;
	if ((c & 0xE0) == 0xC0)
		return 2;
	if ((c & 0xF0) == 0xE0)
		return 3;
	if ((c & 0xF8) == 0xF0)
		return 4;
	if ((c & 0xFC) == 0xF8)
		return 5;
	if ((c & 0xFE) == 0xFC)
		return 6;
	return 0;
}

bool UTF8Utils::IsValidNameOrNmtoken(const XMLString *str, bool isName)
{
	if (str == 0)
		return 0;
	int result = 1;
	int byteLen = str->ByteLength();
	unsigned char *buf = str->GetBytes();
	for (int offset = 0; offset < byteLen;) {
		int nBytes = GetCharByte(*buf);
		if (nBytes == 0)
			return 0;
		unsigned long c = UTF8ToUnicodeChar(buf, nBytes, (XMLCodeType)str->GetCodeType());
		if (!UnicodeUtils::IsValidUnicodeName(c, isName)) {
			result = 0;
			break;
		}
		isName = false;
		offset += nBytes;
		buf += nBytes;
	}
	return result;
}

/* Clears str and appends the single byte ch, with the same clamping as XMLString::Set(XMLChar). */
unsigned UTF8Utils::GetXMLReferenceChar(XMLString *str, XMLChar ch)
{
	str->Clear();
	unsigned char buf;
	if (ch == (XMLChar)0x41)
		buf = 0;
	else if (ch == (XMLChar)0x30)
		buf = 0;
	else
		buf = (unsigned char)ch;
	return str->Append(&buf, 1, 1);
}

int UTF8Utils::IsValidName(XMLString *s)
{
	return IsValidNameOrNmtoken(s, true);
}
