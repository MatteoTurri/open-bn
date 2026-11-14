/*
 * XMLString: the string class of the XML toolkit, in one of several code types.
 */

#ifndef GUARD_XML_STRING_H
#define GUARD_XML_STRING_H
#pragma interface
#include "xml_mmobject.h"

/*
 * Code type (encoding) of a string. The code shows only the width of the code unit
 * (XMLString::codetypeToAligntype) and where its low byte lies, which is how the
 * enumerators are named: two byte-wide types (1 is validated as UTF-8 too), two pairs of
 * 16-bit types (little and big endian), four 32-bit types (low byte at offset 0 to 3).
 * 11 to 15 are byte-wide and never used.
 */
enum XMLCodeType {
	XMLCode_None,
	XMLCode_1,
	XMLCode_UTF8,
	XMLCode_16LE_A,
	XMLCode_16BE_A,
	XMLCode_16LE_B,
	XMLCode_16BE_B,
	XMLCode_32_Byte0,
	XMLCode_32_Byte1,
	XMLCode_32_Byte2,
	XMLCode_32_Byte3,
	XMLCode_11,
	XMLCode_12,
	XMLCode_13,
	XMLCode_14,
	XMLCode_15
};

class XMLInputStreamImpl;
class UTF8Utils;
enum XMLChar {
	XMLChar_zero,
	XMLChar_hash = 35,
	XMLChar_semi = 59
};

class XMLString : public XMLMMObject {
	friend class XMLInputStreamImpl;
	friend class UTF8Utils;
	friend class XMLUTF8Stream;
	friend class CDATAScanner;
	friend class CommentScanner;
	friend class CharacterDataScanner;
	friend class STagScanner;

public:
	XMLString();
	XMLString(const XMLString &other);
	XMLString(const char *s, bool adopt);
	XMLString(const char *s, int len);
	XMLString(const XMLString *other);
	XMLString(const unsigned char *buf, unsigned byteLen, XMLCodeType codeType);
	~XMLString();
	XMLString &operator=(const XMLString &other);

	XMLString *Clone() const;
	unsigned Set(const XMLString *s);
	unsigned Set(XMLChar c);
	void Set(const char *s, int len);
	void Set(const unsigned char *s, int len);
	void Append(const char *s, unsigned len);
	unsigned Append(const unsigned char *buf, unsigned len);
	unsigned Append(const char *s);
	unsigned Append(const XMLString *str);
	unsigned Append(XMLChar c);
	unsigned Append(const unsigned char *buf, unsigned byteLen, unsigned charLen);
	XMLString operator[](unsigned i) const;
	void InsertData(unsigned long index, XMLString *str);
	void ReplaceData(unsigned long offset, unsigned long count, XMLString *str);
	XMLString SubstringData(unsigned long offset, unsigned long count) const;
	void DeleteData(unsigned long offset, unsigned long count);
	int StrToCodetype() const;
	int AtoI() const;
	bool EqualsIgnoreCase(const XMLString *other) const;
	bool isValidNameOrNmtoken(bool nmtoken) const;
	bool IsValidName() const;
	bool IsValidNmtoken() const;
	bool IsValidPITarget() const;
	bool IsValidDigit() const;
	bool IsWhiteSpace() const;
	int CharByte(unsigned char c) const;
	XMLString *ToUpperCase() const;
	XMLString *ToLowerCase() const;
	XMLString *toUpperAndLowerCase(bool toUpper) const;
	void XMLStringCopy(const XMLString *src);
	bool equals(const XMLString &other, bool caseSensitive) const;
	bool IsValidPubidChar() const;
	bool IsValidNCName() const;
	bool IsValidQName() const;
	bool AreValidNames() const;
	int IndexOf(const char *substr, int start) const;
	int IndexOf(const XMLString *str, int start) const;
	XMLString *Split(const XMLString *delim, int *count) const;
	XMLString cutName();
	static XMLString CodetypeToStr(XMLCodeType codeType);
	double AtoF() const;
	unsigned char *GetBytes() const { return fBuffer; }
	unsigned ByteLength() const { return fByteLength; }
	unsigned CharLength() const { return fCharLength; }
	bool IsEmpty() const { return fCharLength == 0; }
	int GetCodeType() const { return fCodeType; }
	void SetCodeType(XMLCodeType codeType) { fCodeType = codeType; }
	void Clear()
	{
		fByteLength = 0;
		fCharLength = 0;
		fFree = fCapacity;
	}
	static int codetypeToAligntype(XMLCodeType codeType);

	unsigned char *expandStringBuffer(unsigned char *oldBuf, unsigned needed, unsigned keep);
	unsigned char *GetBufferPointer();

private:
	/*
	 * Shared by the constructors from a raw buffer: adopt=true allocates exactly len+2,
	 * adopt=false starts from a capacity of 128 and grows with expandStringBuffer.
	 */
	inline void init(unsigned char *buf, unsigned len, bool adopt);

	/* 0x04 */ unsigned char *fBuffer;
	/* 0x08 */ unsigned fCapacity;  /* capacity of fBuffer */
	/* 0x0C */ unsigned fFree;  /* free bytes left in fBuffer */
	/* 0x10 */ unsigned fByteLength;
	/* 0x14 */ unsigned fCharLength;
	/* 0x18 */ int fCodeType;
};

bool operator==(const XMLString &a, const XMLString &b);
bool operator!=(const XMLString &a, const XMLString &b);
bool operator==(const XMLString &a, const char *b);
bool operator!=(const XMLString &a, const char *b);
bool operator!=(const XMLString &a, const unsigned char *b);
bool operator==(const XMLString &a, XMLChar b);
bool operator!=(const XMLString &a, XMLChar b);
bool operator==(const XMLString &a, const unsigned char *b);

#endif // GUARD_XML_STRING_H
