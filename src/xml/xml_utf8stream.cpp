/*
 * Methods of XMLUTF8Stream. Read results: 1 = ok, 16 = end of input (or a truncated
 * sequence), 4096 = a character not allowed in XML, 0 = failure.
 */

#pragma implementation "xml_utf8stream.h"
#include "xml_utf8stream.h"
#include "xml_utf8utils.h"
#include "xml_unicodeutils.h"
#include "xml_inputsource.h"
#include <ctype.h>

/* debug trace of the XML library, empty in the release build */
#define XML_TRACE(args)	do { } while (0)

/*
 * Reads the characters of a Name (or of an Nmtoken when isName is false; only the first
 * character follows the Name start rules), flushing a 128-byte buffer into str. The first
 * invalid character is put back. Returns 1 if anything was read, 16 at end of input,
 * 0 if the first character is invalid.
 */
int XMLUTF8Stream::getNameAndNmtoken(XMLString *str, bool isName)
{
	/* 128-byte flush threshold plus room for the last multibyte character */
	unsigned char buf[136];
	unsigned byteLen = 0;
	unsigned charLen = 0;
	int nBytes;
	unsigned char *p;

	str->Clear();
	for (;;) {
		p = buf + byteLen;
		if (fSource->read(p, 1) <= 0) {
			nBytes = 1;
			break;
		}
		nBytes = UTF8Utils::GetCharByte(*p);
		if (nBytes != 1) {
			if (nBytes >= 2)
				fSource->read(p + 1, nBytes - 1);
			if (nBytes == 0)
				return 0;
		}
		if (!UnicodeUtils::IsValidUnicodeName(UTF8Utils::UTF8ToUnicodeChar(buf + byteLen, nBytes, XMLCode_UTF8),
		        isName)) {
			fSource->seekg(-nBytes, XMLIOSeek_cur);
			nBytes = 0;
			break;
		}
		byteLen += nBytes;
		charLen++;
		isName = 0;
		if (byteLen >= 128) {
			str->Append(buf, byteLen, charLen);
			charLen = 0;
			byteLen = 0;
		}
	}
	if (byteLen != 0)
		str->Append(buf, byteLen, charLen);
	if (str->fCharLength != 0)
		return 1;
	return nBytes << 4;
}

/*
 * Reads into dst up to the string marker (excluded). Only the first byte of the marker is
 * compared first; the rest is read and compared when it agrees, and put back otherwise.
 * Returns 1 when the marker is found, 16 at end of input, 0 for a null argument or a
 * truncated marker.
 */
int XMLUTF8Stream::GetUntil(XMLString *dst, XMLString *marker)
{
	unsigned char *curBuf;
	unsigned char *markerBuf;
	XMLString *cur;
	int markerLen;

	if (marker == 0)
		return 0;
	if (dst == 0)
		return 0;
	markerBuf = marker->GetBufferPointer();
	XML_TRACE(("GetUntil: marker \"%s\"\n", markerBuf));
	XMLString *str = &fUntilBuf;
	curBuf = str->GetBufferPointer();
	cur = str;
	markerLen = marker->fByteLength;
	for (;;) {
		int r;

		if (fSource->eof())
			return 16;
		r = GetNextChar(cur);
		if (r != 1) {
			fSource->seekg(-(int)cur->fByteLength, XMLIOSeek_cur);
			return r;
		}
		if (*markerBuf == *curBuf) {
			unsigned saved = cur->fByteLength;

			str = &fMatchBuf;
			str->Clear();
			str->Append(cur);
			int i = 1;
			while (i < markerLen) {
				if (GetNextChar(cur) == 16)
					return 0;
				str->Append(cur);
				i++;
			}
			if (*str == *marker)
				return 1;
			fSource->seekg(saved - str->fByteLength, XMLIOSeek_cur);
		}
		dst->Append(cur);
	}
}

/*
 * Reads into dst until a character whose first byte is in stopSet (returns 1) or in
 * notSet (returns 0); the sets are checked only while fEntityResolving is set. The
 * stopping character is put back.
 */
int XMLUTF8Stream::GetNextPortion(XMLString *dst, XMLString *stopSet, XMLString *notSet)
{
	unsigned char buf[8];
	int cont = 1;
	int matched;
	int stopLen;
	unsigned char *stopBuf;
	int notLen;
	unsigned char *notBuf;
	int total;
	int nBytes;

	stopLen = stopSet->fByteLength;
	stopBuf = stopSet->GetBufferPointer();
	notLen = 0;
	notBuf = 0;
	if (notSet != 0) {
		notLen = notSet->fByteLength;
		notBuf = notSet->GetBufferPointer();
	}
	matched = 0;
	for (;;) {
		total = fSource->read(buf, 1);
		if (total <= 0)
			return 16;
		nBytes = UTF8Utils::GetCharByte(buf[0]);
		if (nBytes != 1) {
			if (nBytes >= 2) {
				total = fSource->read(buf + 1, nBytes - 1) + 1;
				if (total < nBytes)
					return 16;
			} else if (nBytes == 0)
				return 0;
		}
		if (!UnicodeUtils::IsValidUnicodeXMLCharacter(
		        UTF8Utils::UTF8ToUnicodeChar(buf, nBytes, XMLCode_UTF8))) {
			fSource->seekg(-total, XMLIOSeek_cur);
			return 4096;
		}
		if (fEntityResolving) {
			int i;

			for (i = 0; i < stopLen; i++) {
				if (buf[0] == stopBuf[i]) {
					matched = 1;
					cont = 0;
				}
			}
			if (cont) {
				for (i = 0; i < notLen; i++) {
					if (buf[0] == notBuf[i])
						cont = 0;
				}
			}
		}
		if (cont != 1) {
			fSource->seekg(-total, XMLIOSeek_cur);
			return matched ? 1 : cont;
		}
		dst->Append(buf, nBytes, 1);
	}
}

/*
 * Recognises the next token without consuming it (every branch seeks back): '<' followed
 * by '/', '?' or '!' and a keyword ("[CDATA[", "[", "--", "DOCTYPE", "ATTLIST", "ENTITY",
 * "ELEMENT", "NOTATION"), '&' with an entity reference (the five predefined ones count
 * as text), '%', "]]>", ']' + spaces + '>', '>'.
 */
int XMLUTF8Stream::GetNextToken()
{
	unsigned char buf[256];
	unsigned char c;
	long count;
	int got;
	int n;
	int nBytes;
	int m;
	int r;
	int len;

	c = 0;
	len = 1;
	got = fSource->read(&c, len);
	if (UnicodeUtils::IsValidUnicodeSpaceCharacter(c)) {
		fSource->seekg(-1, XMLIOSeek_cur);
		return XMLToken_Space;
	}
	if (c == '<') {
		if (fEntityResolving == 0) {
			fSource->seekg(-1, XMLIOSeek_cur);
			return XMLToken_Text;
		}
		n = fSource->read(buf, len);
		if (n <= 0)
			return XMLToken_EOF;
		nBytes = UTF8Utils::GetCharByte(buf[0]);
		if (nBytes != len) {
			if (nBytes >= 2) {
				n = fSource->read(buf + 1, nBytes - 1) + 1;
				if (n < nBytes) {
					fSource->seekg(-n, XMLIOSeek_cur);
					return XMLToken_EOF;
				}
			}
			if (nBytes == 0)
				return XMLToken_Text;
		}
		if (UnicodeUtils::IsValidUnicodeName(UTF8Utils::UTF8ToUnicodeChar(buf, nBytes, XMLCode_UTF8), 1)) {
			fSource->seekg(-n, XMLIOSeek_cur);
			return XMLToken_STag;
		}
		if (buf[0] == '/')
			return XMLToken_ETag;
		if (buf[0] == '?')
			return XMLToken_PI;
		if (buf[0] == '!') {
			m = fSource->read(buf, 8);
			if (buf[0] == '[' && memcmp(buf, "[CDATA[", 7) == 0) {
				fSource->seekg(-m + 7, XMLIOSeek_cur);
				return XMLToken_CDataSect;
			}
			if (buf[0] == '[' && memcmp(buf, "[", 1) == 0) {
				fSource->seekg(-m + 1, XMLIOSeek_cur);
				return XMLToken_CondSect;
			}
			if (buf[0] == '-' && memcmp(buf, "--", 2) == 0) {
				fSource->seekg(-m + 2, XMLIOSeek_cur);
				return XMLToken_Comment;
			}
			if (buf[0] == 'D' && memcmp(buf, "DOCTYPE", 7) == 0) {
				fSource->seekg(-m + 7, XMLIOSeek_cur);
				return XMLToken_Doctype;
			}
			if (buf[0] == 'A' && memcmp(buf, "ATTLIST", 7) == 0) {
				fSource->seekg(-m + 7, XMLIOSeek_cur);
				return XMLToken_AttlistDecl;
			}
			if (buf[0] == 'E' && memcmp(buf, "ENTITY", 6) == 0) {
				fSource->seekg(-m + 6, XMLIOSeek_cur);
				return XMLToken_EntityDecl;
			}
			if (buf[0] == 'E' && memcmp(buf, "ELEMENT", 7) == 0) {
				fSource->seekg(-m + 7, XMLIOSeek_cur);
				return XMLToken_ElementDecl;
			}
			if (buf[0] == 'N' && memcmp(buf, "NOTATION", 8) == 0) {
				fSource->seekg(-m + 8, XMLIOSeek_cur);
				return XMLToken_NotationDecl;
			}
			fSource->seekg(~m, XMLIOSeek_cur);
		} else {
			fSource->seekg(~n, XMLIOSeek_cur);
			return XMLToken_Text;
		}
	}
	if (c == '&') {
		if (fEntityResolving == 0) {
			fSource->seekg(-1, XMLIOSeek_cur);
			return XMLToken_Text;
		}
		XMLString *ref = &fCharBuf;

		ref->Clear();
		GetNextPortion(ref, fSemi, 0);
		if (*ref == *fLt || *ref == *fGt || *ref == *fQuot || *ref == *fAmp || *ref == *fApos) {
			XMLString *s = &fCharBuf;
			fSource->seekg(~(int)s->fByteLength, XMLIOSeek_cur);
			return XMLToken_Text;
		}
		fSource->seekg(-(int)ref->fByteLength, XMLIOSeek_cur);
		return XMLToken_EntityRef;
	}
	if (c == '%') {
		fSource->seekg(-1, XMLIOSeek_cur);
		return XMLToken_PERef;
	}
	if (c == ']') {
		c = (unsigned char)fSource->peek();
		if (c == ']') {
			fSource->seekg(1, XMLIOSeek_cur);
			c = (unsigned char)fSource->peek();
			if (c != '>') {
				fSource->seekg(-1, XMLIOSeek_cur);
				return XMLToken_Text;
			}
			fSource->seekg(1, XMLIOSeek_cur);
			return XMLToken_CDataEnd;
		}
		count = 0;
		r = skipWhiteSpace(&count);
		if (r < 0) {
			fSource->seekg(-1, XMLIOSeek_cur);
			return XMLToken_Text;
		}
		if (r >= 2) {
			if (r == 16)
				fSource->seekg(-count, XMLIOSeek_cur);
		} else {
			c = (unsigned char)fSource->peek();
			if (c == '>') {
				fSource->seekg(1, XMLIOSeek_cur);
				return XMLToken_SubsetEnd;
			}
			fSource->seekg(-count, XMLIOSeek_cur);
		}
		fSource->seekg(-1, XMLIOSeek_cur);
		return XMLToken_Text;
	}
	if (c == '>') {
		fSource->seekg(-1, XMLIOSeek_cur);
		return fEntityResolving ? XMLToken_DeclEnd : XMLToken_Text;
	}
	if (got <= 0)
		return XMLToken_EOF;
	fSource->seekg(-1, XMLIOSeek_cur);
	return XMLToken_Text;
}

XMLUTF8Stream::XMLUTF8Stream(XMLCodeType codeType, XMLInputSource *source) : XMLInputStreamImpl(codeType, source)
{}

XMLUTF8Stream::~XMLUTF8Stream()
{}

/* Skips Unicode white space, counting it in *count; 16 if the source has ended. */
int XMLUTF8Stream::skipWhiteSpace(long *count)
{
	int consumed = 0;
	unsigned char c;

	if (fSource->eof()) {
		*count = 0;
		return 16;
	}
	c = fSource->peek();
	while (UnicodeUtils::IsValidUnicodeSpaceCharacter(c)) {
		fSource->read(&c, 1);
		(*count)++;
		consumed = 1;
		c = fSource->peek();
	}
	return consumed;
}

/* Reads one UTF-8 character into str; with rewind set, the stream is put back. */
int XMLUTF8Stream::readChar(XMLString *str, bool rewind)
{
	unsigned char buf[8];
	int result = 1;
	int total = 1;
	int nBytes;

	str->Clear();
	if (fSource->read(buf, 1) <= 0)
		return 16;
	nBytes = UTF8Utils::GetCharByte(buf[0]);
	if (nBytes != 1) {
		if (nBytes >= 2) {
			total = fSource->read(buf + 1, nBytes - 1) + 1;
			if (total < nBytes)
				result = 16;
		}
		if (nBytes == 0)
			return 0;
	}
	if (!UnicodeUtils::IsValidUnicodeXMLCharacter(UTF8Utils::UTF8ToUnicodeChar(buf, nBytes, XMLCode_UTF8)))
		result = 4096;
	str->Append(buf, nBytes, 1);
	if (rewind)
		fSource->seekg(-total, XMLIOSeek_cur);
	return result;
}

/*
 * Reads a decimal integer with an optional '-'. Returns 16 at end of input, 1 if a digit
 * was read, 0 otherwise (with *result = 0).
 */
int XMLUTF8Stream::GetNextInteger(int *result)
{
	int sign = 1;
	int accum = 0;
	int found = 0;
	unsigned char ch;

	if (fSource->eof())
		return 16;
	ch = (unsigned char)fSource->peek();
	if (ch == '-') {
		fSource->read(&ch, 1);
		sign = -1;
		ch = (unsigned char)fSource->peek();
	}
	if (isdigit(ch)) {
		found = 1;
		do {
			fSource->read(&ch, 1);
			accum = accum * 10 + (ch - '0');
			ch = (unsigned char)fSource->peek();
		} while (isdigit(ch));
	}
	*result = sign * accum;
	return found;
}

int XMLUTF8Stream::GetNextHex(int *result)
{
	int accum = 0;
	int consumed = 0;
	if (fSource->eof())
		return 16;
	unsigned char ch = (unsigned char)fSource->peek();
	if (isxdigit(ch)) {
		consumed = 1;
		do {
			fSource->read(&ch, 1);
			if (ch >= '0' && ch <= '9')
				accum = accum * 16 + (ch - '0');
			if (ch >= 'a' && ch <= 'f')
				accum = accum * 16 + (ch - 'a' + 10);
			if (ch >= 'A' && ch <= 'F')
				accum = accum * 16 + (ch - 'A' + 10);
			ch = (unsigned char)fSource->peek();
		} while (isxdigit(ch));
	}
	*result = accum;
	return consumed;
}

void XMLUTF8Stream::GetXMLReservedChar(XMLString *str, XMLChar ch)
{
	unsigned char c = ch;
	str->Append(&c, 1, 1);
}

/* As UTF8Utils::GetXMLReferenceChar, for a numeric reference: UTF-8 encodes code. */
void XMLUTF8Stream::GetXMLReferenceChar(XMLString *str, unsigned long code)
{
	str->Clear();
	unsigned char buf[8];
	unsigned len;
	if (!(code & 0xff80)) {
		len = 1;
		buf[0] = (unsigned char)code;
	} else
		len = UTF8Utils::UnicodeToUTF8Char(buf, code);
	str->Append(buf, len, 1);
}

unsigned XMLUTF8Stream::GetXMLReferenceChar(XMLString *str, XMLChar ch)
{
	return UTF8Utils::GetXMLReferenceChar(str, ch);
}
