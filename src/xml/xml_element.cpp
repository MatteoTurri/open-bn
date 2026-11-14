/*
 * Methods of the Element handle.
 */

#include "xml_element.h"
#include "xml_attr.h"
#include "xml_nodelist.h"
#include "xml_filedescriptor.h"

ElementImpl *Element::implementation() const
{
	return (ElementImpl *)fImpl;
}

Element::Element(ElementImpl *impl) : Node(impl)
{}

Element::Element()
{}

Element::Element(const Node &other) : Node(other)
{}

Element::~Element()
{}

Element &Element::operator=(const Element &other)
{
	return (Element &)Node::operator=(other);
}

bool Element::operator==(const Element &other) const
{
	return Node::operator==(other);
}

bool Element::operator!=(const Element &other) const
{
	return Node::operator!=(other);
}

Element &Element::operator=(const int *other)
{
	return (Element &)Node::operator=(other);
}

bool Element::operator==(const int *other) const
{
	return Node::operator==(other);
}

bool Element::operator!=(const int *other) const
{
	return Node::operator!=(other);
}

XMLString Element::getAttribute(XMLString name)
{
	return ((ElementImpl *)fImpl)->GetAttribute(&name);
}

void Element::setAttribute(XMLString name, XMLString value)
{
	((ElementImpl *)fImpl)->SetAttribute(&name, &value);
}

void Element::removeAttribute(XMLString name)
{
	((ElementImpl *)fImpl)->RemoveAttribute(&name);
}

Attr Element::getAttributeNode(XMLString name)
{
	return Attr(((ElementImpl *)fImpl)->GetAttributeNode(&name));
}

Attr Element::setAttributeNode(Attr newAttr)
{
	return Attr(((ElementImpl *)fImpl)->SetAttributeNode(*(AttrImpl **)&newAttr, true));
}

Attr Element::setAttributeNode(Attr newAttr, bool create)
{
	return Attr(((ElementImpl *)fImpl)->SetAttributeNode(*(AttrImpl **)&newAttr, create));
}

Attr Element::removeAttributeNode(Attr oldAttr)
{
	return Attr(((ElementImpl *)fImpl)->RemoveAttributeNode(*(AttrImpl **)&oldAttr));
}

NodeList Element::getElementsByTagName(XMLString name)
{
	return NodeList(fImpl->GetElementsByTagName(&name, 0));
}

void Element::normalize()
{
	((ElementImpl *)fImpl)->Normalize();
}

XMLString Element::get_tagName() const
{
	return fImpl->fName;
}
