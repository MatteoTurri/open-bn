/*
 * OutputDirective: the serialisation options (method, version, encoding, indentation...).
 * Without a polymorphic base, its vtable pointer is at the end (0xF0).
 */

#ifndef GUARD_XML_OUTPUTDIRECTIVE_H
#define GUARD_XML_OUTPUTDIRECTIVE_H
#pragma interface
#include "xml_string.h"

class XMLOutputSource;

class OutputDirective {
public:
	/*
	 * State left by the last serialised node: after a start tag, after a close tag (or an
	 * empty HTML element), after text, after another non-element node (SNS_0, SNS_4 unused).
	 */
	enum SerialiseNodeState {
		SNS_0,
		SNS_StartTag,
		SNS_EndTag,
		SNS_Text,
		SNS_4,
		SNS_Other
	};
	OutputDirective();
	virtual ~OutputDirective();
	virtual void Serialise(XMLOutputSource *out);
	void OutputMethod(XMLString *s);
	void OutputVersion(XMLString *s);
	void OutputEncoding(XMLString *s);
	void OutputStandalone(XMLString *s);
	void OutputDoctypePublic(XMLString *s);
	void OutputDoctypeSystem(XMLString *s);
	void OutputCDATASectionElements(XMLString *s);
	void OutputMediaType(XMLString *s);
	int GetCodeType();
	int GetIndentBool();
	int GetIndentCount();
	int IncrementIndentCount();
	int DecrementIndentCount();
	SerialiseNodeState GetPrevNode();
	void SetPrevNode(SerialiseNodeState state);
	void OutputOmitXMLDeclaration(bool b);
	void OutputIndent(bool b);
	int IsMethodHTML();
	int IsHTML40ElementName(const XMLString &name);
	void SerialiseHTMLMETA(XMLOutputSource *out);

	/* The 13 empty elements of HTML 4.0 (no close tag), NULL-terminated. */
	static const char *const html40openTagNames[];

protected:
	/* 0x00 */ XMLString fMethod;
	/* 0x1C */ XMLString fVersion;
	/* 0x38 */ XMLString fEncoding;
	/* 0x54 */ int fOmitXMLDecl;
	/* 0x58 */ XMLString fStandalone;
	/* 0x74 */ XMLString fDoctypePublic;
	/* 0x90 */ XMLString fDoctypeSystem;
	/* 0xAC */ XMLString fCDATASectionElements;
	/* 0xC8 */ int fIndent;
	/* 0xCC */ XMLString fMediaType;
	/* 0xE8 */ int fIndentCount;
	/* 0xEC */ SerialiseNodeState fPrevNode;
};

inline int OutputDirective::GetIndentBool()
{
	return fIndent;
}

inline int OutputDirective::IncrementIndentCount()
{
	int old = fIndentCount;
	fIndentCount = old + 1;
	return old;
}

inline int OutputDirective::DecrementIndentCount()
{
	int old = fIndentCount;
	fIndentCount = old - 1;
	return old;
}

inline int OutputDirective::GetIndentCount()
{
	return fIndentCount;
}

inline void OutputDirective::SetPrevNode(SerialiseNodeState state)
{
	fPrevNode = state;
}

inline OutputDirective::SerialiseNodeState OutputDirective::GetPrevNode()
{
	return fPrevNode;
}

#endif // GUARD_XML_OUTPUTDIRECTIVE_H
