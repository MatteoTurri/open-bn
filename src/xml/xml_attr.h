/*
 * Attr node: the implementation class and the DOM handle.
 */

#ifndef GUARD_XML_ATTR_H
#define GUARD_XML_ATTR_H
#include "xml.h"

class AttrImpl : public NodeImpl {
public:
	AttrImpl(DocumentImpl *ownerDoc, XMLString *name);
	AttrImpl(AttrImpl *other, bool deep);
	virtual ~AttrImpl();

	void AttributeValue(XMLString *value);

	virtual void Serialise(XMLOutputSource *out);
	virtual NodeImpl *CloneNode(bool deep);

private:
	/* 0x70 */ bool fSpecified;
};

class Attr : public Node {
public:
	Attr();
	Attr(AttrImpl *impl);
	Attr(const Node &other);
	virtual ~Attr();

	Attr &operator=(const Attr &other);
	Attr &operator=(const int *other);
	bool operator==(const Attr &other) const;
	bool operator!=(const Attr &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;

	AttrImpl *implementation() const;
	XMLString get_name() const;
	XMLString get_value() const;
	void set_value(XMLString value);
};

#endif // GUARD_XML_ATTR_H
