/*
 * Attr: the reference-counted handle of an attribute node (name, value).
 */

#include "xml_attr.h"

AttrImpl *Attr::implementation() const
{
	return (AttrImpl *)fImpl;
}

Attr::Attr(AttrImpl *impl) : Node(impl)
{}

Attr::Attr()
{}

Attr::Attr(const Node &other) : Node(other)
{}

Attr::~Attr()
{}

Attr &Attr::operator=(const Attr &other)
{
	return (Attr &)Node::operator=(other);
}

bool Attr::operator==(const Attr &other) const
{
	return Node::operator==(other);
}

bool Attr::operator!=(const Attr &other) const
{
	return Node::operator!=(other);
}

Attr &Attr::operator=(const int *other)
{
	return (Attr &)Node::operator=(other);
}

bool Attr::operator==(const int *other) const
{
	return Node::operator==(other);
}

bool Attr::operator!=(const int *other) const
{
	return Node::operator!=(other);
}

XMLString Attr::get_name() const
{
	return fImpl->fName;
}

XMLString Attr::get_value() const
{
	return fImpl->NodeValue();
}

void Attr::set_value(XMLString value)
{
	((AttrImpl *)fImpl)->AttributeValue(&value);
}
