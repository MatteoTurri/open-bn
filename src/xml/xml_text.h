/*
 * Text node: the implementation class and the DOM handle (derives from CharacterData).
 */

#ifndef GUARD_XML_TEXT_H
#define GUARD_XML_TEXT_H
#include "xml.h"
#include "xml_chardata.h"

class TextImpl : public CharacterDataImpl {
public:
	TextImpl(DocumentImpl *ownerDoc, XMLString *data);
	TextImpl(DocumentImpl *ownerDoc, XMLString *name, XMLString *value, unsigned short type);
	TextImpl(TextImpl *other, bool deep);
	virtual ~TextImpl();
	virtual NodeImpl *CloneNode(bool deep);
	virtual void Serialise(XMLOutputSource *out);
	virtual void Serialise(XMLOutputSource *out, OutputDirective *directive);
};

class Text : public CharacterData {
public:
	Text();
	Text(TextImpl *impl);
	Text(const Node &other);
	virtual ~Text();

	Text &operator=(const Text &other);
	Text &operator=(const int *other);
	bool operator==(const Text &other) const;
	bool operator!=(const Text &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;
};

#endif // GUARD_XML_TEXT_H
