/*
 * UnicodeUtils: Unicode character classes of the XML 1.0 productions (appendix B).
 * Static methods only.
 */

#ifndef GUARD_XML_UNICODEUTILS_H
#define GUARD_XML_UNICODEUTILS_H
#include "xml_mmobject.h"

class UnicodeUtils : public XMLMMObject {
public:
	static int IsValidUnicodeBaseChar(unsigned long c);
	static int IsValidUnicodeCombiningChar(unsigned long c);
	static int IsValidUnicodeIdeographic(unsigned long c);
	static bool IsValidUnicodeDigit(unsigned long c);
	static int IsValidUnicodeExtender(unsigned long c);
	static int IsValidUnicodeLetter(unsigned long c);
	static int IsValidUnicodeName(unsigned long c, bool firstChar);
	static int IsValidUnicodeXMLCharacter(unsigned long c);
	static int IsValidUnicodeSpaceCharacter(unsigned long c);
};

#endif // GUARD_XML_UNICODEUTILS_H
