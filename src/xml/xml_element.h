/*
 * Element node: the DOM handle (ElementImpl is in xml_elementimpl.h).
 */

#ifndef GUARD_XML_ELEMENT_H
#define GUARD_XML_ELEMENT_H
#include "xml.h"
#include "xml_elementimpl.h"

class AttrImpl;
class Attr;
class NodeList;

class Element : public Node {
public:
	Element();
	Element(ElementImpl *impl);
	Element(const Node &other);
	virtual ~Element();

	Element &operator=(const Element &other);
	Element &operator=(const int *other);
	bool operator==(const Element &other) const;
	bool operator!=(const Element &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;

	ElementImpl *implementation() const;
	XMLString get_tagName() const;
	XMLString getAttribute(XMLString name);
	void setAttribute(XMLString name, XMLString value);
	void removeAttribute(XMLString name);
	Attr getAttributeNode(XMLString name);
	Attr setAttributeNode(Attr newAttr);
	Attr setAttributeNode(Attr newAttr, bool create);
	Attr removeAttributeNode(Attr oldAttr);
	NodeList getElementsByTagName(XMLString name);
	void normalize();
};

#endif // GUARD_XML_ELEMENT_H
