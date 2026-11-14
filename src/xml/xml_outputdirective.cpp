/*
 * Methods of OutputDirective and the tables of the HTML 4.0 output method.
 */

#pragma implementation "xml_outputdirective.h"
#include "xml_outputdirective.h"
#include "xml_outputsource.h"

/* clang-format off */
const char *const OutputDirective::html40openTagNames[] = {
	"area", "base", "basefont", "br", "col", "frame", "hr", "img",
	"input", "isindex", "link", "meta", "param", 0,
};
/* clang-format on */

/*
 * Writes the XML prolog (<?xml ...?>) or the HTML DOCTYPE, by fMethod ("xml" when empty).
 * Default version "1.0" for XML, "4.0" for HTML; with no encoding, the default code type
 * in lower case.
 */
void OutputDirective::Serialise(XMLOutputSource *out)
{
	if (fMethod.ByteLength() == 0)
		fMethod = XMLString("xml", false);

	if (fMethod == "xml") {
		if (fVersion.ByteLength() == 0)
			fVersion = XMLString("1.0", false);
		*out << "<?xml version=\"" << &fVersion << "\"";
		if (fEncoding.ByteLength() != 0) {
			*out << " encoding=\"" << &fEncoding << "\"";
		} else {
			XMLString defaultCodeType;
			defaultCodeType = XMLString::CodetypeToStr((XMLCodeType)defaultCodeType.GetCodeType());
			XMLString *lower = defaultCodeType.ToLowerCase();
			*out << " encoding=\"" << lower << "\"";
			delete lower;
		}
		if (fStandalone.ByteLength() != 0)
			*out << " standalone=\"" << &fStandalone << "\"";
		*out << "?>";
	} else if (fMethod == "html") {
		if (fVersion.ByteLength() == 0)
			fVersion = XMLString("4.0", false);
		if (fDoctypePublic.ByteLength() == 0) {
			fDoctypePublic = XMLString("-//W3C//DTD HTML ", false);
			fDoctypePublic.Append(&fVersion);
			fDoctypePublic.Append(" Transitional//EN");
		}
		*out << "<!DOCTYPE HTML PUBLIC \"" << &fDoctypePublic << "\"";
		*out << " \"http://www.w3.org/TR/REC-html40/loose.dtd\">" << XMLIOStr_Newline;
	}
}

inline OutputDirective::OutputDirective()
{
	fOmitXMLDecl = 0;
	fIndent = 1;
	fIndentCount = 0;
	fPrevNode = SNS_EndTag;
}

inline OutputDirective::~OutputDirective()
{}

inline void OutputDirective::OutputMethod(XMLString *s)
{
	fMethod.Set(s);
}
inline void OutputDirective::OutputVersion(XMLString *s)
{
	fVersion.Set(s);
}
inline void OutputDirective::OutputEncoding(XMLString *s)
{
	fEncoding.Set(s);
}
inline void OutputDirective::OutputOmitXMLDeclaration(bool b)
{
	fOmitXMLDecl = b;
}
inline void OutputDirective::OutputStandalone(XMLString *s)
{
	fStandalone.Set(s);
}
inline void OutputDirective::OutputDoctypePublic(XMLString *s)
{
	fDoctypePublic.Set(s);
}
inline void OutputDirective::OutputDoctypeSystem(XMLString *s)
{
	fDoctypeSystem.Set(s);
}
inline void OutputDirective::OutputCDATASectionElements(XMLString *s)
{
	fCDATASectionElements.Set(s);
}
inline void OutputDirective::OutputIndent(bool b)
{
	fIndent = b;
}
inline void OutputDirective::OutputMediaType(XMLString *s)
{
	fMediaType.Set(s);
}

inline int OutputDirective::GetCodeType()
{
	return fEncoding.StrToCodetype();
}

/*
 * Writes <meta HTTP-EQUIV="Content-Type" Content="text/html[; charset=ENC]">, on a new line
 * indented by fIndentCount tabs when fIndent is set.
 */
inline void OutputDirective::SerialiseHTMLMETA(XMLOutputSource *out)
{
	if (!IsMethodHTML())
		return;
	if (fIndent) {
		*out << XMLIOStr_Newline;
		int n = fIndentCount;
		while (n > 0) {
			*out << "\t";
			n--;
		}
	}
	*out << "<meta HTTP-EQUIV=\"Content-Type\" Content=\"text/html";
	if (fEncoding.ByteLength() != 0)
		*out << "; charset=" << &fEncoding;
	*out << "\">";
}

inline int OutputDirective::IsHTML40ElementName(const XMLString &name)
{
	XMLString *lowerName = name.ToLowerCase();
	XMLString temp(lowerName);
	if (IsMethodHTML()) {
		if (html40openTagNames[0]) {
			const char *const *p = html40openTagNames;
			do {
				if (temp == *p) {
					delete lowerName;
					return 1;
				}
			} while (*++p);
		}
	}
	delete lowerName;
	return 0;
}

inline int OutputDirective::IsMethodHTML()
{
	XMLString *lower = fMethod.ToLowerCase();
	int result = 0;
	if (*lower == "html")
		result = (fVersion == "4.0") != 0;
	delete lower;
	return result;
}
