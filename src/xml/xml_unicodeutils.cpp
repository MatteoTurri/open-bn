/*
 * UnicodeUtils: the character classes of XML 1.0 Appendix B (letters, digits,
 * combining characters, extenders) and the name and white space checks.
 */

#include "xml_unicodeutils.h"

int UnicodeUtils::IsValidUnicodeIdeographic(unsigned long c)
{
	return (c >= 0x4E00 && c <= 0x9FA5) || c == 0x3007 || (c >= 0x3021 && c <= 0x3029);
}

int UnicodeUtils::IsValidUnicodeLetter(unsigned long c)
{
	return IsValidUnicodeBaseChar(c) || IsValidUnicodeIdeographic(c);
}

bool UnicodeUtils::IsValidUnicodeDigit(unsigned long c)
{
	return (c >= 0x0030 && c <= 0x0039) || (c >= 0x0660 && c <= 0x0669) || (c >= 0x06F0 && c <= 0x06F9)
	    || (c >= 0x0966 && c <= 0x096F) || (c >= 0x09E6 && c <= 0x09EF) || (c >= 0x0A66 && c <= 0x0A6F)
	    || (c >= 0x0AE6 && c <= 0x0AEF) || (c >= 0x0B66 && c <= 0x0B6F) || (c >= 0x0BE7 && c <= 0x0BEF)
	    || (c >= 0x0C66 && c <= 0x0C6F) || (c >= 0x0CE6 && c <= 0x0CEF) || (c >= 0x0D66 && c <= 0x0D6F)
	    || (c >= 0x0E50 && c <= 0x0E59) || (c >= 0x0ED0 && c <= 0x0ED9) || (c >= 0x0F20 && c <= 0x0F29);
}

int UnicodeUtils::IsValidUnicodeExtender(unsigned long c)
{
	return c == 0x00B7 || c == 0x02D0 || c == 0x02D1 || c == 0x0387 || c == 0x0640 || c == 0x0E46 || c == 0x0EC6
	    || c == 0x3005 || (c >= 0x3031 && c <= 0x3035) || (c >= 0x309D && c <= 0x309E)
	    || (c >= 0x30FC && c <= 0x30FE);
}

int UnicodeUtils::IsValidUnicodeXMLCharacter(unsigned long c)
{
	if (c < 0x9 || c == 0xB || c == 0xC || (c >= 0xE && c <= 0x1F) || (c >= 0xD800 && c <= 0xDFFF)
	    || (c >= 0xFFFE && c <= 0xFFFF) || c > 0x10FFFF)
		return 0;
	return 1;
}

int UnicodeUtils::IsValidUnicodeSpaceCharacter(unsigned long c)
{
	return c == 0x20 || c == 0x0A || c == 0x0D || c == 0x09;
}

int UnicodeUtils::IsValidUnicodeBaseChar(unsigned long c)
{
	if (c < 0x80)
		return (c - 'a' < 26) || (c - 'A' < 26);
	switch (c / 1000) {
	case 0:
		if (c < 256)
			return c - 0xC0 < 23 || c - 0xD8 < 31 || c - 0xF8 < 8;
		if (c < 383)
			return c - 0x100 < 50 || c - 0x134 < 11 || c - 0x141 < 8 || c - 0x14A < 53;
		if (c < 536)
			return c - 0x180 < 68 || c - 0x1CD < 36 || c - 0x1F4 < 2 || c - 0x1FA < 30;
		if (c < 930)
			return c - 0x250 < 89 || c - 0x2BB < 7 || c == 0x386 || c - 0x388 < 3 || c == 0x38C
			    || c - 0x38E < 20;
		return c - 0x3A3 < 44 || c - 0x3D0 < 7 || c == 0x3DA || c == 0x3DC || c == 0x3DE || c == 0x3E0
		    || c - 0x3E2 < 6;
	case 1:
		if (c < 1154)
			return c - 0x3E8 < 12 || c - 0x401 < 12 || c - 0x40E < 66 || c - 0x451 < 12 || c - 0x45E < 36;
		if (c < 1272)
			return c - 0x490 < 53 || c - 0x4C7 < 2 || c - 0x4CB < 2 || c - 0x4D0 < 28 || c - 0x4EE < 8;
		if (c < 1569)
			return c - 0x4F8 < 2 || c - 0x531 < 38 || c == 0x559 || c - 0x561 < 38 || c - 0x5D0 < 27
			    || c - 0x5F0 < 3;
		return c - 0x621 < 26 || c - 0x641 < 10 || c - 0x671 < 71 || c - 0x6BA < 5 || c - 0x6C0 < 15
		    || c - 0x6D0 < 4 || c == 0x6D5 || c - 0x6E5 < 2;
	case 2:
		if (c < 2451)
			return c - 0x905 < 53 || c == 0x93D || c - 0x958 < 10 || c - 0x985 < 8 || c - 0x98F < 2;
		if (c < 2527)
			return c - 0x993 < 22 || c - 0x9AA < 7 || c == 0x9B2 || c - 0x9B6 < 4 || c - 0x9DC < 2;
		if (c < 2602)
			return c - 0x9DF < 3 || c - 0x9F0 < 2 || c - 0xA05 < 6 || c - 0xA0F < 2 || c - 0xA13 < 22;
		if (c < 2654)
			return c - 0xA2A < 7 || c - 0xA32 < 2 || c - 0xA35 < 2 || c - 0xA38 < 2 || c - 0xA59 < 4;
		if (c < 2707)
			return c == 0xA5E || c - 0xA72 < 3 || c - 0xA85 < 7 || c == 0xA8D || c - 0xA8F < 3;
		if (c < 2821)
			return c - 0xA93 < 22 || c - 0xAAA < 7 || c - 0xAB2 < 2 || c - 0xAB5 < 5 || c == 0xABD
			    || c == 0xAE0;
		if (c < 2870)
			return c - 0xB05 < 8 || c - 0xB0F < 2 || c - 0xB13 < 22 || c - 0xB2A < 7 || c - 0xB32 < 2;
		if (c < 2958)
			return c - 0xB36 < 4 || c == 0xB3D || c - 0xB5C < 2 || c - 0xB5F < 3 || c - 0xB85 < 6;
		if (c < 2979)
			return c - 0xB8E < 3 || c - 0xB92 < 4 || c - 0xB99 < 2 || c == 0xB9C || c - 0xB9E < 2;
		return c - 0xBA3 < 2 || c - 0xBA8 < 3 || c - 0xBAE < 8 || c == 0xBB7;
	case 3:
		if (c < 3125)
			return c - 0xBB8 < 2 || c - 0xC05 < 8 || c - 0xC0E < 3 || c - 0xC12 < 23 || c - 0xC2A < 10;
		if (c < 3242)
			return c - 0xC35 < 5 || c - 0xC60 < 2 || c - 0xC85 < 8 || c - 0xC8E < 3 || c - 0xC92 < 23;
		if (c < 3342)
			return c - 0xCAA < 10 || c - 0xCB5 < 5 || c == 0xCDE || c - 0xCE0 < 2 || c - 0xD05 < 8;
		if (c < 3632)
			return c - 0xD0E < 3 || c - 0xD12 < 23 || c - 0xD2A < 16 || c - 0xD60 < 2 || c - 0xE01 < 46;
		if (c < 3719)
			return c == 0xE30 || c - 0xE32 < 2 || c - 0xE40 < 6 || c - 0xE81 < 2 || c == 0xE84;
		if (c < 3745)
			return c - 0xE87 < 2 || c == 0xE8A || c == 0xE8D || c - 0xE94 < 4 || c - 0xE99 < 7;
		if (c < 3760)
			return c - 0xEA1 < 3 || c == 0xEA5 || c == 0xEA7 || c - 0xEAA < 2 || c - 0xEAD < 2;
		return c == 0xEB0 || c - 0xEB2 < 2 || c == 0xEBD || c - 0xEC0 < 5 || c - 0xF40 < 8 || c - 0xF49 < 33;
	case 4:
		if (c < 4361)
			return c - 0x10A0 < 38 || c - 0x10D0 < 39 || c == 0x1100 || c - 0x1102 < 2 || c - 0x1105 < 3;
		if (c < 4428)
			return c == 0x1109 || c - 0x110B < 2 || c - 0x110E < 5 || c == 0x113C || c == 0x113E
			    || c == 0x1140;
		if (c < 4447)
			return c == 0x114C || c == 0x114E || c == 0x1150 || c - 0x1154 < 2 || c == 0x1159;
		if (c < 4466)
			return c - 0x115F < 3 || c == 0x1163 || c == 0x1165 || c == 0x1167 || c == 0x1169
			    || c - 0x116D < 2;
		if (c < 4535)
			return c - 0x1172 < 2 || c == 0x1175 || c == 0x119E || c == 0x11A8 || c == 0x11AB
			    || c - 0x11AE < 2;
		return c - 0x11B7 < 2 || c == 0x11BA || c - 0x11BC < 7 || c == 0x11EB || c == 0x11F0 || c == 0x11F9;
	case 7:
		return c - 0x1E00 < 156 || c - 0x1EA0 < 90 || c - 0x1F00 < 22 || c - 0x1F18 < 6 || c - 0x1F20 < 32;
	case 8:
		if (c < 8031)
			return c - 0x1F40 < 6 || c - 0x1F48 < 6 || c - 0x1F50 < 8 || c == 0x1F59 || c == 0x1F5B
			    || c == 0x1F5D;
		if (c < 8134)
			return c - 0x1F5F < 31 || c - 0x1F80 < 53 || c - 0x1FB6 < 7 || c == 0x1FBE || c - 0x1FC2 < 3;
		if (c < 8182)
			return c - 0x1FC6 < 7 || c - 0x1FD0 < 4 || c - 0x1FD6 < 6 || c - 0x1FE0 < 13 || c - 0x1FF2 < 3;
		return c - 0x1FF6 < 7 || c == 0x2126 || c - 0x212A < 2 || c == 0x212E || c - 0x2180 < 3;
	case 12:
		return c - 0x3041 < 84 || c - 0x30A1 < 90 || c - 0x3105 < 40;
	case 44:
		if (c > 0xABFF)
			return 1;
		return 0;
	case 45:
		return 1;
	case 46:
		return 1;
	case 47:
		return 1;
	case 48:
		return 1;
	case 49:
		return 1;
	case 50:
		return 1;
	case 51:
		return 1;
	case 52:
		return 1;
	case 53:
		return 1;
	case 54:
		return 1;
	case 55:
		return c <= 0xD7A3;
	}
	return 0;
}

int UnicodeUtils::IsValidUnicodeCombiningChar(unsigned long c)
{
	switch (c / 768) {
	case 1:
		if (c < 1467)
			return c - 0x300 < 70 || c - 0x360 < 2 || c - 0x483 < 4 || c - 0x591 < 17 || c - 0x5A3 < 23;
		return c - 0x5BB < 3 || c == 0x5BF || c - 0x5C1 < 2 || c == 0x5C4;
	case 2:
		return c - 0x64B < 8 || c == 0x670 || c - 0x6D6 < 7 || c - 0x6DD < 3 || c - 0x6E0 < 5 || c - 0x6E7 < 2
		    || c - 0x6EA < 4;
	case 3:
		if (c < 2433)
			return c - 0x901 < 3 || c == 0x93C || c - 0x93E < 16 || c - 0x951 < 4 || c - 0x962 < 2;
		if (c < 2503)
			return c - 0x981 < 3 || c == 0x9BC || c == 0x9BE || c == 0x9BF || c - 0x9C0 < 5;
		if (c < 2620)
			return c - 0x9C7 < 2 || c - 0x9CB < 3 || c == 0x9D7 || c - 0x9E2 < 2 || c == 0xA02;
		if (c < 2689)
			return c == 0xA3C || c - 0xA3E < 5 || c - 0xA47 < 2 || c - 0xA4B < 3 || c - 0xA70 < 2;
		if (c < 2817)
			return c - 0xA81 < 3 || c == 0xABC || c - 0xABE < 8 || c - 0xAC7 < 3 || c - 0xACB < 3;
		if (c < 2902)
			return c - 0xB01 < 3 || c == 0xB3C || c - 0xB3E < 6 || c - 0xB47 < 2 || c - 0xB4B < 3;
		return c - 0xB56 < 2 || c - 0xB82 < 2 || c - 0xBBE < 5 || c - 0xBC6 < 3 || c - 0xBCA < 4 || c == 0xBD7;
	case 4:
		if (c < 3202)
			return c - 0xC01 < 3 || c - 0xC3E < 7 || c - 0xC46 < 3 || c - 0xC4A < 4 || c - 0xC55 < 2;
		if (c < 3330)
			return c - 0xC82 < 2 || c - 0xCBE < 7 || c - 0xCC6 < 3 || c - 0xCCA < 4 || c - 0xCD5 < 2;
		if (c < 3633)
			return c - 0xD02 < 2 || c - 0xD3E < 6 || c - 0xD46 < 3 || c - 0xD4A < 4 || c == 0xD57;
		return c == 0xE31 || c - 0xE34 < 7 || c - 0xE47 < 8 || c == 0xEB1 || c - 0xEB4 < 6 || c - 0xEBB < 2
		    || c - 0xEC8 < 6;
	case 5:
		if (c < 3953)
			return c - 0xF18 < 2 || c == 0xF35 || c == 0xF37 || c == 0xF39 || c == 0xF3E || c == 0xF3F;
		return c - 0xF71 < 20 || c - 0xF86 < 6 || c - 0xF90 < 6 || c == 0xF97 || c - 0xF99 < 21 || c - 0xFB1 < 7
		    || c == 0xFB9;
	case 10:
		return c - 0x20D0 < 13 || c == 0x20E1;
	case 16:
		return c - 0x302A < 6 || c == 0x3099 || c == 0x309A;
	}
	return 0;
}

int UnicodeUtils::IsValidUnicodeName(unsigned long c, bool firstChar)
{
	if (firstChar)
		return IsValidUnicodeLetter(c) || c == '_' || c == ':';
	return IsValidUnicodeLetter(c) || c == ':' || IsValidUnicodeDigit(c) || c == '.' || c == '-' || c == '_'
	    || IsValidUnicodeCombiningChar(c) || IsValidUnicodeExtender(c);
}
