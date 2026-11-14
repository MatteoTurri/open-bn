/*
 * Methods of XMLString and its free comparison operators.
 */

#pragma implementation "xml_string.h"
#include "xml_string.h"
#include "xml_inputstreamimpl.h"
#include "xml_unicodeutils.h"
#include "xml_utf8utils.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

bool XMLString::IsValidName() const
{
	return isValidNameOrNmtoken(true);
}

bool XMLString::IsValidNmtoken() const
{
	return isValidNameOrNmtoken(false);
}

inline void XMLString::init(unsigned char *buf, unsigned len, bool adopt)
{
	fCodeType = XMLCode_UTF8;
	fByteLength = len;
	if (!adopt) {
		unsigned cap = 128;
		fCapacity = cap;
		fFree = cap;
		if (cap < len + 2) {
			fBuffer = expandStringBuffer(0, len + 2, 0);
			fFree = fCapacity;
		} else {
			fBuffer = new unsigned char[cap];
		}
	} else {
		unsigned cap = len + 2;
		fCapacity = cap;
		fFree = cap;
		fBuffer = new unsigned char[cap];
	}
	memcpy(fBuffer, buf, len);
	fFree -= len;
	switch (codetypeToAligntype((XMLCodeType)fCodeType)) {
	case 1:
		if (GetCodeType() == 2)
			fCharLength = UTF8Utils::GetCharLength(fBuffer, fByteLength);
		else
			fCharLength = len;
		break;
	default:
		fCharLength = fByteLength / 4;
	}
}

/* Takes strlen(s) (0 for a null s). */
XMLString::XMLString(const char *s, bool adopt)
{
	unsigned len = 0;
	if (s) {
		const unsigned char *p = (const unsigned char *)s;
		while (*p) {
			p++;
			len++;
		}
	}
	init((unsigned char *)s, len, adopt);
}

int XMLString::codetypeToAligntype(XMLCodeType codeType)
{
	if (codeType >= XMLCode_16LE_A) {
		if (codeType < XMLCode_32_Byte0)
			return 2;
		if (codeType < XMLCode_11)
			return 4;
	}
	return 1;
}

inline XMLString::XMLString()
{
	fCodeType = XMLCode_UTF8;
	fByteLength = 0;
	fCapacity = 0;
	fFree = 0;
	fBuffer = 0;
	fCharLength = 0;
}

XMLString::XMLString(const XMLString &other)
{
	fByteLength = other.fByteLength;
	fCodeType = other.fCodeType;
	fCharLength = other.fCharLength;
	fCapacity = other.fByteLength;
	fFree = other.fByteLength;
	if (fCapacity < fByteLength) {
		fBuffer = expandStringBuffer(0, fByteLength, 0);
		fFree = fCapacity;
	} else {
		unsigned char *buf;

		if (fCapacity)
			buf = new unsigned char[fCapacity];
		else
			buf = 0;
		fBuffer = buf;
	}
	memcpy(fBuffer, other.fBuffer, fByteLength);
	fFree -= fByteLength;
}

/* Exact copy: no spare room, unlike init and Clone. */
inline XMLString::XMLString(const XMLString *other)
{
	unsigned char *buf;

	fByteLength = other->ByteLength();
	fCodeType = other->GetCodeType();
	fCharLength = other->CharLength();
	fCapacity = other->ByteLength();
	fFree = other->ByteLength();
	if (fCapacity)
		buf = new unsigned char[fCapacity];
	else
		buf = 0;
	fBuffer = buf;
	memcpy(buf, other->GetBytes(), fByteLength);
	fFree -= fByteLength;
}

void XMLString::XMLStringCopy(const XMLString *src)
{
	fByteLength = src->fByteLength;
	fCodeType = src->fCodeType;
	fCharLength = src->fCharLength;
	fCapacity = src->fByteLength + 2;
	fFree = src->fByteLength + 2;
	if (fBuffer)
		delete[] fBuffer;
	fBuffer = new unsigned char[fCapacity];
	memcpy(fBuffer, src->fBuffer, fByteLength);
	fFree = fFree - fByteLength;
}

/* As init with adopt=false, for a length already known. */
XMLString::XMLString(const char *s, int len)
{
	init((unsigned char *)s, (unsigned)len, false);
}

/* Capacity 128 or more; the character count follows the code type. */
XMLString::XMLString(const unsigned char *buf, unsigned byteLen, XMLCodeType codeType)
{
	fByteLength = byteLen;
	fCodeType = codeType;
	unsigned cap = 128;
	fCapacity = cap;
	fFree = cap;
	if (cap < byteLen + 2) {
		fBuffer = expandStringBuffer(0, byteLen + 2, 0);
		fFree = fCapacity;
	} else {
		fBuffer = new unsigned char[cap];
	}
	memcpy(fBuffer, buf, byteLen);
	fFree -= byteLen;
	fCharLength = 0;
	switch (codetypeToAligntype((XMLCodeType)fCodeType)) {
	case 1:
		if (fCodeType == XMLCode_UTF8)
			fCharLength = UTF8Utils::GetCharLength(fBuffer, fByteLength);
		else
			fCharLength = fByteLength;
		break;
	case 2:
		fCharLength = fByteLength >> 1;
		break;
	default:
		fCharLength = fByteLength >> 2;
	}
}

/* Copy through the raw constructor (buffer, length, code type). */
XMLString *XMLString::Clone() const
{
	return new XMLString(fBuffer, fByteLength, (XMLCodeType)fCodeType);
}

XMLString::~XMLString()
{
	if (fBuffer)
		delete[] fBuffer;
}

unsigned char *XMLString::expandStringBuffer(unsigned char *oldBuf, unsigned needed, unsigned keep)
{
	unsigned n = fCapacity * 2;
	if (needed < n)
		needed = n;
	fCapacity = ((needed >> 7) + 1) << 7;
	unsigned char *nb = new unsigned char[fCapacity];
	if (oldBuf && keep)
		memcpy(nb, oldBuf, keep);
	fFree = fCapacity - keep;
	if (oldBuf)
		delete[] oldBuf;
	return nb;
}

/* Appends len raw bytes; returns the new byte length. */
unsigned XMLString::Append(const unsigned char *buf, unsigned len)
{
	if (!buf || len == 0)
		return 0;
	if (fFree < len + 2) {
		fBuffer = expandStringBuffer(fBuffer, fByteLength + len + 2, fByteLength);
		fFree = fCapacity - fByteLength;
	}
	memmove(fBuffer + fByteLength, buf, len);
	fByteLength += len;
	fFree -= len;
	if (codetypeToAligntype((XMLCodeType)fCodeType) == 1) {
		if (fCodeType == XMLCode_UTF8)
			fCharLength += UTF8Utils::GetCharLength(fBuffer + (fByteLength - len), len);
		else
			fCharLength += len;
	} else {
		fCharLength = fByteLength >> 2;  /* NOTE: bug: also for the 2-byte code types */
	}
	return fByteLength;
}

/* Private variant, with the character count already known. */
unsigned XMLString::Append(const unsigned char *buf, unsigned byteLen, unsigned charLen)
{
	if (fFree < byteLen + 2) {
		unsigned keep = fByteLength;
		if (buf == fBuffer) {
			fBuffer = expandStringBuffer(fBuffer, keep + byteLen + 2, keep);
			buf = fBuffer;
		} else
			fBuffer = expandStringBuffer(fBuffer, keep + byteLen + 2, keep);
		fFree = fCapacity - fByteLength;
	}
	memcpy(fBuffer + fByteLength, buf, byteLen);
	fByteLength += byteLen;
	fCharLength += charLen;
	fFree -= byteLen;
	return fByteLength;
}

XMLString &XMLString::operator=(const XMLString &other)
{
	if (&other == this)
		return *this;
	Clear();
	fByteLength = other.ByteLength();
	fCodeType = other.GetCodeType();
	if (fFree < other.ByteLength()) {
		fBuffer = expandStringBuffer(fBuffer, other.ByteLength(), 0);
		fFree = fCapacity;
	}
	memcpy(fBuffer, other.GetBytes(), fByteLength);
	fFree -= fByteLength;
	fCharLength = other.CharLength();
	return *this;
}

/* Returns the buffer terminated by two zero bytes, growing it when needed. */
unsigned char *XMLString::GetBufferPointer()
{
	if (fFree < 2) {
		fBuffer = expandStringBuffer(fBuffer, fByteLength + 2, fByteLength);
		fFree = fCapacity - fByteLength;
	}
	fBuffer[fByteLength] = 0;
	fBuffer[fByteLength + 1] = 0;
	return fBuffer;
}

void XMLString::Append(const char *s, unsigned len)
{
	Append((const unsigned char *)s, len);
}

unsigned XMLString::Append(const char *s)
{
	if (s == 0)
		return 0;
	unsigned len = 0;
	while (s[len])
		len++;
	return Append((const unsigned char *)s, len);
}

unsigned XMLString::Append(const XMLString *str)
{
	if (str == 0)
		return 0;
	Append(str->fBuffer, str->fByteLength, str->fCharLength);
	return fByteLength;
}

unsigned XMLString::Append(XMLChar c)
{
	unsigned char buf = 0;
	if (c == (XMLChar)0x41)
		buf = 0;
	else if (c == (XMLChar)0x30)
		buf = 0;
	else
		buf = (unsigned char)c;
	if (!buf)
		return 0;
	return Append(&buf, 1, 1);
}

void XMLString::Set(const unsigned char *s, int len)
{
	fByteLength = 0;
	fCharLength = 0;
	fFree = fCapacity;
	Append(s, len);
}

void XMLString::Set(const char *s, int len)
{
	fByteLength = 0;
	fCharLength = 0;
	fFree = fCapacity;
	Append((const unsigned char *)s, len);
}

unsigned XMLString::Set(const XMLString *s)
{
	fByteLength = 0;
	fCharLength = 0;
	fFree = fCapacity;
	if (s) {
		Append(s->fBuffer, s->fByteLength, s->fCharLength);
		return fByteLength;
	}
	return 0;
}

unsigned XMLString::Set(XMLChar c)
{
	fByteLength = 0;
	fCharLength = 0;
	fFree = fCapacity;
	unsigned char buf = 0;
	if (c == (XMLChar)0x41)
		buf = 0;
	else if (c == (XMLChar)0x30)
		buf = 0;
	else
		buf = (unsigned char)c;
	if (buf)
		return Append(&buf, 1, 1);
	return 0;
}

int XMLString::CharByte(unsigned char c) const
{
	switch (codetypeToAligntype((XMLCodeType)fCodeType)) {
	case 1:
		if (fCodeType == XMLCode_UTF8)
			return UTF8Utils::GetCharByte(c);
		return 1;
	case 2:
		return 2;
	}
	return 4;
}

/*
 * memcmp when the code types agree and ignoreCase is false; otherwise character by
 * character, and with ignoreCase only the first byte of each character goes through toupper().
 */
bool XMLString::equals(const XMLString &other, bool ignoreCase) const
{
	if (fCharLength != other.fCharLength)
		return false;
	if (fCodeType == other.fCodeType) {
		if (fByteLength != other.fByteLength)
			return false;
		if (!ignoreCase)
			return memcmp(fBuffer, other.fBuffer, fByteLength) == 0;
	}
	unsigned char *p1 = fBuffer;
	unsigned char *p2 = other.fBuffer;
	for (unsigned i = 0; i < fCharLength; i++) {
		int w1 = CharByte(*p1);
		int w2 = other.CharByte(*p2);
		if (ignoreCase) {
			if (toupper(*p1) != toupper(*p2))
				return false;
		} else {
			if (memcmp(p1, p2, w1) != 0)
				return false;
		}
		p1 += w1;
		p2 += w2;
	}
	return true;
}

/* Compares the bytes at the same index, whatever the width of the characters. */
bool operator==(const XMLString &a, const unsigned char *b)
{
	unsigned n = 0;
	unsigned i;
	const unsigned char *p;

	if (b == 0)
		return false;
	p = b;
	while (*p != 0) {
		p++;
		n++;
	}
	if (a.CharLength() != n)
		return false;
	p = a.GetBytes();
	for (i = 0; i < n; i++) {
		unsigned step = a.CharByte(*p);

		if (a.GetBytes()[i] != b[i])
			return false;
		p += step;
	}
	return true;
}

/* Copy of [offset, offset+count), clipped; empty when offset is past the end. */
XMLString XMLString::SubstringData(unsigned long offset, unsigned long count) const
{
	XMLString tmp(this);
	if (fCharLength < offset) {
		tmp.Clear();
		return XMLString(&tmp);
	}
	if (fCharLength - count < offset)
		count = fCharLength - offset;
	if (offset != 0)
		tmp.DeleteData(0, offset);
	if (tmp.fCharLength != count)
		tmp.DeleteData(count, tmp.fCharLength - count);
	return XMLString(&tmp);
}

/* Removes count characters from offset (clipped to the end). */
void XMLString::DeleteData(unsigned long offset, unsigned long count)
{
	if (count == 0)
		return;
	unsigned byteOffset = 0;
	unsigned bytesRemoved = 0;
	if (fCharLength < offset)
		return;
	if (fCharLength < offset + count)
		count = fCharLength - offset;
	unsigned i;
	unsigned char *start = fBuffer;
	for (i = 0; i < offset; i++) {
		byteOffset += CharByte(*start);
		start += CharByte(*start);
	}
	unsigned char *end = start;
	for (i = 0; i < count; i++) {
		bytesRemoved += CharByte(*end);
		end += CharByte(*end);
	}
	if (fByteLength >= byteOffset + bytesRemoved)
		memmove(start, end, fByteLength - (byteOffset + bytesRemoved));
	fByteLength -= bytesRemoved;
	fCharLength -= count;
	fFree = fCapacity - fByteLength;
}

/* Copy with toupper() or tolower() applied to the single-byte characters only. */
XMLString *XMLString::toUpperAndLowerCase(bool toUpper) const
{
	XMLString *result = Clone();
	result->fByteLength = 0;
	result->fCharLength = 0;
	result->fFree = result->fCapacity;
	unsigned char *p = fBuffer;
	for (unsigned i = 0; i < fCharLength; i++) {
		int w = CharByte(*p);
		if (w == 1) {
			unsigned char c;
			if (toUpper)
				c = toupper(*p);
			else
				c = tolower(*p);
			result->Append(&c, 1, 1);
		} else {
			result->Append(p, w, 1);
		}
		p += w;
	}
	return result;
}

/*
 * True if every character is a space, \n, \r or \t; in the wider code types the other
 * bytes of each character must be zero.
 */
bool XMLString::IsWhiteSpace() const
{
	if (fCharLength == 0)
		return false;
	int align;
	int off;
	switch (fCodeType) {
	case XMLCode_16LE_A:
	case XMLCode_16BE_A:
	case XMLCode_16LE_B:
	case XMLCode_16BE_B:
		align = 2;
		off = (fCodeType - XMLCode_16LE_A) % 2;
		break;
	case XMLCode_32_Byte0:
	case XMLCode_32_Byte1:
	case XMLCode_32_Byte2:
	case XMLCode_32_Byte3:
		align = 4;
		off = (fCodeType - XMLCode_32_Byte0) % 4;
		break;
	default:
		off = 0;
		align = 1;
	}
	unsigned i = 0;
	while (i < fByteLength) {
		if ((int)(i % align) == off) {
			unsigned char c = fBuffer[i];
			if (c != ' ' && c != '\n' && c != '\r' && c != '\t')
				return false;
		}
		if ((int)(i % align) != off) {
			if (fBuffer[i] != 0)
				return false;
		}
		i++;
	}
	return true;
}

/* True if every character is a PubidChar: \n, \r, or 0x20-0x7A except " & < > [ \ ] ^ ` */
bool XMLString::IsValidPubidChar() const
{
	bool valid = true;
	int align = codetypeToAligntype((XMLCodeType)fCodeType);
	int off;
	switch (align) {
	case 2:
		off = (fCodeType - XMLCode_16LE_A) % 2;
		break;
	case 4:
		off = (fCodeType - XMLCode_32_Byte0) % 4;
		break;
	default:
		off = 0;
	}
	unsigned char *p = fBuffer + off;
	while (p < fBuffer + fByteLength) {
		int ch = *p;
		unsigned char c = ch;
		if (c == '\n' || c == '\r') {
		} else if (ch >= 32 && ch <= 122) {
			if (c == '"' || c == '&' || c == '<' || c == '>' || c == '[' || c == '\\' || c == ']'
			    || c == '^' || c == '`') {
				valid = false;
				break;
			}
		} else {
			valid = false;
			break;
		}
		p += align;
	}
	return valid;
}

XMLString XMLString::operator[](unsigned i) const
{
	return SubstringData(i, 1);
}

/* The free comparison operators of two strings forward to equals. */

bool operator==(const XMLString &a, const XMLString &b)
{
	return a.equals(b, 0);
}

/* True for a one-character string in a byte-wide code type (not 27) equal to b. */
bool operator==(const XMLString &a, XMLChar b)
{
	if (a.CharLength() != 1)
		return false;
	if (XMLString::codetypeToAligntype((XMLCodeType)a.GetCodeType()) == 1 && a.GetCodeType() != 27)
		return a.GetBytes()[0] == (unsigned char)b;
	return false;
}

bool operator==(const XMLString &a, const char *b)
{
	return a == (const unsigned char *)b;
}

bool operator!=(const XMLString &a, const XMLString &b)
{
	return a.equals(b, 0) == 0;
}

bool operator!=(const XMLString &a, XMLChar b)
{
	return !(a == b);
}

bool operator!=(const XMLString &a, const unsigned char *b)
{
	return (a == b) == 0;
}

bool operator!=(const XMLString &a, const char *b)
{
	return (a == (const unsigned char *)b) == 0;
}

void XMLString::InsertData(unsigned long index, XMLString *str)
{
	if (str)
		ReplaceData(index, 0, str);
}

/* Replaces [offset, offset+count) with *str, which may be this string. */
void XMLString::ReplaceData(unsigned long offset, unsigned long count, XMLString *str)
{
	XMLString *selfCopy = 0;
	if (str == 0)
		return;
	if (fCharLength < offset)
		return;
	if (offset + count > fCharLength)
		count = fCharLength - offset;
	if (this == str)
		selfCopy = str->Clone();  /* NOTE: bug: never deleted */
	XMLString *tail = Clone();
	unsigned oldLen = fCharLength;
	DeleteData(offset, oldLen);
	tail->DeleteData(0, offset + count);
	XMLString *src = selfCopy ? selfCopy : str;
	if (src)
		Append(src->GetBytes(), src->ByteLength(), src->CharLength());
	if (tail)
		Append(tail->GetBytes(), tail->ByteLength(), tail->CharLength());
	delete tail;
}

bool XMLString::EqualsIgnoreCase(const XMLString *other) const
{
	if (other == 0)
		return false;
	return equals(*other, true);
}

XMLString *XMLString::ToUpperCase() const
{
	return toUpperAndLowerCase(true);
}

XMLString *XMLString::ToLowerCase() const
{
	return toUpperAndLowerCase(false);
}

/* Character position of str from start, -1 if absent. */
int XMLString::IndexOf(const XMLString *str, int start) const
{
	if (str == 0)
		return -1;
	if (start < 0)
		start = 0;
	if (fCharLength - (unsigned)start < str->fCharLength)
		return -1;
	unsigned len = str->fCharLength;
	for (int i = start; (unsigned)i <= fCharLength - len; i++) {
		const XMLString &sub = SubstringData(i, len);
		if (sub.equals(*str, false))
			return i;
	}
	return -1;
}

/* Character position of substr from start, -1 if absent or substr is null. */
int XMLString::IndexOf(const char *substr, int start) const
{
	if (substr == 0)
		return -1;
	if (start < 0)
		start = 0;
	unsigned rem = fCharLength - (unsigned)start;
	unsigned len = 0;
	while (substr[len] != 0)
		len++;
	if (rem < len)
		return -1;
	for (int i = start; (unsigned)i <= fCharLength - len; i++) {
		const XMLString &sub = SubstringData(i, len);
		if (sub == (const unsigned char *)substr)
			return i;
	}
	return -1;
}

bool XMLString::IsValidDigit() const
{
	switch (codetypeToAligntype((XMLCodeType)fCodeType)) {
	case 1:
		return UnicodeUtils::IsValidUnicodeDigit(fBuffer[0]);
	case 2: {
		unsigned char *p = fBuffer;
		if (fCodeType != XMLCode_16LE_A) {
			unsigned char *q = fBuffer + 1;
			if (fCodeType != XMLCode_16LE_B)
				p = q;
		}
		return UnicodeUtils::IsValidUnicodeDigit(*p);
	}
	}
	return false;
}

bool XMLString::isValidNameOrNmtoken(bool nmtoken) const
{
	if (fCharLength != 0) {
		if (fCodeType == XMLCode_1)
			return UTF8Utils::IsValidNameOrNmtoken(this, nmtoken);
		if (fCodeType == XMLCode_UTF8)
			return UTF8Utils::IsValidNameOrNmtoken(this, nmtoken);
	}
	return false;
}

/* A valid Name that is not "xml" in any case. */
bool XMLString::IsValidPITarget() const
{
	XMLString xml("xml", false);
	if (IsValidName() && !EqualsIgnoreCase(&xml))
		return true;
	return false;
}

int XMLString::StrToCodetype() const
{
	return 2;
}

/* A Name without ':'. */
bool XMLString::IsValidNCName() const
{
	if (IndexOf(":", 0) == -1 && IsValidName())
		return true;
	return false;
}

/* NCName (':' NCName)? */
bool XMLString::IsValidQName() const
{
	int idx = -1;

	idx = IndexOf(":", 0);
	if (idx == -1)
		return IsValidNCName();
	return SubstringData(0, idx).IsValidNCName() && SubstringData(idx + 1, fCharLength - idx).IsValidNCName();
}

/*
 * Removes the first space-separated name, and the spaces after it, from the string and
 * returns it (the whole string when there is no space).
 */
XMLString XMLString::cutName()
{
	XMLString space(" ", false);
	int idx = IndexOf(&space, 0);
	if (idx == -1)
		idx = fCharLength;
	XMLString prefix = SubstringData(0, idx);
	DeleteData(0, idx);
	while (fCharLength != 0 && SubstringData(0, 1).IsWhiteSpace())
		DeleteData(0, 1);
	return XMLString(&prefix);
}

/* True if every space-separated name is a valid Name (works on a copy). */
bool XMLString::AreValidNames() const
{
	if (fCharLength == 0)
		return false;
	XMLString *work = Clone();
	XMLString name;
	while (work->fCharLength != 0) {
		name = work->cutName();
		if (!name.IsValidName()) {
			delete work;
			return false;
		}
	}
	delete work;
	return true;
}

XMLString *XMLString::Split(const XMLString *delim, int *count) const
{
	int pos;

	if (!delim)
		return 0;
	*count = 1;
	if (fCharLength != 0) {
		int start = 0;
		for (;;) {
			pos = IndexOf(delim, start);
			if (pos == -1)
				break;
			(*count)++;
			start = pos + delim->CharLength();
			if ((unsigned)start >= fCharLength)
				break;
		}
	}
	XMLString **arr = new XMLString *[*count];
	pos = 0;
	for (unsigned n = 0; n < (unsigned)*count; n++) {
		arr[n] = new XMLString(SubstringData(pos, IndexOf(delim, pos) - pos));
		pos = IndexOf(delim, pos) + delim->CharLength();
	}
	return (XMLString *)arr;
}

int XMLString::AtoI() const
{
	if (codetypeToAligntype((XMLCodeType)fCodeType) == 2) {
		unsigned char *buf = new unsigned char[fCharLength + 1];
		int byteOff = 0;
		if (fCodeType != XMLCode_16LE_A)
			byteOff = (fCodeType != XMLCode_16LE_B);
		unsigned len = fCharLength;
		for (unsigned i = 0; i < len; i++)
			buf[i] = fBuffer[byteOff + i * 2];
		buf[fCharLength] = 0;
		int result = strtol((char *)buf, 0, 10);
		if (buf)
			delete[] buf;
		return result;
	}
	unsigned char *buf = new unsigned char[fByteLength + 1];
	memcpy(buf, fBuffer, fByteLength);
	buf[fByteLength] = 0;
	int result = strtol((char *)buf, 0, 10);
	if (buf)
		delete[] buf;
	return result;
}

double XMLString::AtoF() const
{
	if (codetypeToAligntype((XMLCodeType)fCodeType) == 2) {
		unsigned char *buf = new unsigned char[fCharLength + 1];
		int byteOff = 0;
		if (fCodeType != XMLCode_16LE_A)
			byteOff = (fCodeType != XMLCode_16LE_B);
		unsigned len = fCharLength;
		for (unsigned i = 0; i < len; i++)
			buf[i] = fBuffer[byteOff + i * 2];
		buf[fCharLength] = 0;
		double result = strtod((char *)buf, 0);
		if (buf)
			delete[] buf;
		return result;
	}
	unsigned char *buf = new unsigned char[fByteLength + 1];
	memcpy(buf, fBuffer, fByteLength);
	buf[fByteLength] = 0;
	double result = strtod((char *)buf, 0);
	if (buf)
		delete[] buf;
	return result;
}

/* Always "UTF-8": codeType is not used. */
XMLString XMLString::CodetypeToStr(XMLCodeType codeType)
{
	XMLString tmp("UTF-8", false);
	return XMLString(&tmp);
}
