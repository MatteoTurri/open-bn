/*
 * UTF8Utils: UTF-8 conversion and classification. Static methods only.
 */

#ifndef GUARD_XML_UTF8UTILS_H
#define GUARD_XML_UTF8UTILS_H
#include "xml_mmobject.h"
#include "xml_string.h"

class UTF8Utils : public XMLMMObject {
public:
	static unsigned long UTF8ToUnicodeChar(const unsigned char *buf, int nBytes, XMLCodeType codeType);
	static unsigned short UnicodeToUTF8Char(unsigned char *out, unsigned long c);
	static int GetCharLength(unsigned char *buf, int byteLen);
	static int GetCharByte(unsigned char c);
	static int IsValidName(XMLString *s);
	static bool IsValidNameOrNmtoken(const XMLString *s, bool isName);
	static unsigned GetXMLReferenceChar(XMLString *str, XMLChar ch);

	static unsigned long mask[6];
	static unsigned long mask_top[6];
	static unsigned long mask_6bit;
	static unsigned long mask_8bit;
};

#endif // GUARD_XML_UTF8UTILS_H
