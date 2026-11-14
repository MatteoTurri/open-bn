/*
 * Methods of AttrImpl, the node behind the Attr handle.
 */

#include "xml_attr.h"
#include "xml_document.h"
#include "xml_outputsource.h"

AttrImpl::AttrImpl(DocumentImpl *ownerDoc, XMLString *name) : NodeImpl(ownerDoc, name, 0, XML_NODE_ATTRIBUTE)
{}

AttrImpl::~AttrImpl()
{}

void AttrImpl::AttributeValue(XMLString *value)
{
	NodeValue(value);
	if (fFirstChild != 0) {
		NodeImpl *child = fFirstChild;
		do {
			RemoveChild(child, 1);
			if (!child->Referenced())
				conditionalDelete(child);
			child = fFirstChild;
		} while (child != 0);
	}
	AppendChild(fOwnerDocument->CreateTextNode(value));
}

void AttrImpl::Serialise(XMLOutputSource *out)
{
	*out << " " << &fName;
	*out << "=\"";
	if (fFirstChild != 0) {
		for (NodeImpl *n = fFirstChild; n != 0; n = n->fNextSibling)
			n->Serialise(out);
	} else {
		*out << &fValue;
	}
	*out << "\"";
}

NodeImpl *AttrImpl::CloneNode(bool deep)
{
	return new AttrImpl(this, deep);
}

AttrImpl::AttrImpl(AttrImpl *other, bool deep) : NodeImpl(other, deep)
{
	fSpecified = other->fSpecified;
}
