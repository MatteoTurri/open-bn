/*
 * Methods of TextImpl (name "#text").
 */

#include "xml_text.h"
#include "xml_outputsource.h"
#include "xml_outputdirective.h"

TextImpl::TextImpl(DocumentImpl *ownerDoc, XMLString *data) : CharacterDataImpl(ownerDoc, 0, data, XML_NODE_TEXT)
{
	XMLString name("#text", true);
	fName.XMLStringCopy(&name);
}

TextImpl::TextImpl(DocumentImpl *ownerDoc, XMLString *name, XMLString *value, unsigned short type)
        : CharacterDataImpl(ownerDoc, name, value, type)
{}

TextImpl::~TextImpl()
{}

TextImpl::TextImpl(TextImpl *other, bool deep) : CharacterDataImpl(other, deep)
{}

NodeImpl *TextImpl::CloneNode(bool deep)
{
	return new TextImpl(this, deep);
}

void TextImpl::Serialise(XMLOutputSource *out)
{
	*out << &fValue;
}

/*
 * With indentation on and the HTML output method, a value of 3+ characters ending in
 * white space is trimmed to its core plus one space, and a new indented line is started
 * when the previous node asked for it (SNS_Other). Always records a text node (SNS_Text).
 */
void TextImpl::Serialise(XMLOutputSource *out, OutputDirective *directive)
{
	if (directive->GetIndentBool() && directive->IsMethodHTML()) {
		unsigned n = fValue.CharLength();
		XMLString word;
		unsigned i;
		for (i = 0; i < n - 1 && fValue.SubstringData(i, 1).IsWhiteSpace(); i++)
			;
		if (n >= 3 && fValue.SubstringData(n - 1, 1).IsWhiteSpace()) {
			unsigned j = n - 1;
			while (j > 0 && fValue.SubstringData(j, 1).IsWhiteSpace())
				j--;
			word = fValue.SubstringData(i, j - i + 1);
			word.Append(" ");
		} else {
			word = fValue;
		}
		if (directive->GetPrevNode() == OutputDirective::SNS_Other) {
			*out << XMLIOStr_Newline;
			int k = directive->GetIndentCount();
			while (k > 0) {
				*out << "\t";
				k--;
			}
		}
		*out << &word;
	} else {
		*out << &fValue;
	}
	directive->SetPrevNode(OutputDirective::SNS_Text);
}
