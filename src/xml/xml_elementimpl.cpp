/*
 * Methods of ElementImpl. The attributes are kept in fAttributes.
 */

#pragma implementation "xml_elementimpl.h"
#include "xml_element.h"
#include "xml_attr.h"
#include "xml_nnmap.h"
#include "xml_document.h"
#include "xml_outputsource.h"
#include "xml_outputdirective.h"
#include "xml_filedescriptor.h"

inline XMLString ElementImpl::TagName()
{
	return fName;
}

inline ElementImpl::ElementImpl(DocumentImpl *ownerDoc, XMLString *name) : NodeImpl(ownerDoc, name, 0, XML_NODE_ELEMENT)
{
	fAttributes = new NamedNodeMapImpl(this);
}

inline ElementImpl::~ElementImpl()
{}

inline XMLString ElementImpl::GetAttribute(XMLString *name)
{
	NodeImpl *attr = fAttributes->GetNamedItem(name);
	if (attr)
		return attr->NodeValue();
	return XMLString();
}

void ElementImpl::SetAttribute(XMLString *name, XMLString *value)
{
	resetException();
	if (!name->isValidNameOrNmtoken(true)) {
		treatException(DOMException::INVALID_CHARACTER_ERR);
		return;
	}
	if (readOnly()) {
		treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
		return;
	}
	AttrImpl *attr = GetAttributeNode(name);
	if (!attr) {
		attr = fOwnerDocument->CreateAttribute(name);
		fAttributes->SetNamedItem(attr);
	} else {
		NodeImpl *child = attr->fFirstChild;
		while (child) {
			attr->RemoveChild(child, true);
			if (!child->Referenced())
				conditionalDelete(child);
			child = attr->fFirstChild;
		}
	}
	attr->AttributeValue(value);
	XMLString id("id", false);
	if (name->EqualsIgnoreCase(&id))
		fNodePtr = attr;
}

inline AttrImpl *ElementImpl::GetAttributeNode(XMLString *name)
{
	return (AttrImpl *)fAttributes->GetNamedItem(name);
}

inline AttrImpl *ElementImpl::SetAttributeNode(AttrImpl *newAttr, bool create)
{
	resetException();
	if (Node::PerformCheck) {
		if (fOwnerDocument && fOwnerDocument != newAttr->fOwnerDocument) {
			treatException(DOMException::WRONG_DOCUMENT_ERR);
			return 0;
		}
		if (readOnly()) {
			treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
			return 0;
		}
		if (((ElementImpl *)newAttr)->fOwnerCount) {
			treatException(DOMException::INUSE_ATTRIBUTE_ERR);
			return 0;
		}
	}
	XMLString name(newAttr->fName);
	AttrImpl *old = 0;
	if (Node::PerformCheck) {
		if (fAttributes->GetNamedItem(&name))
			old = (AttrImpl *)fAttributes->RemoveNamedItem(&name);
	}
	XMLString id("id", false);
	if (name.EqualsIgnoreCase(&id))
		fNodePtr = newAttr;
	fAttributes->SetNamedItem(newAttr);
	return old;
}

void ElementImpl::Normalize()
{
	NodeImpl *node = fFirstChild;
	while (node) {
		if (node->fNodeType == XML_NODE_TEXT) {
			NodeImpl *next = node->fNextSibling;
			if (next && next->fNodeType == XML_NODE_TEXT) {
				do {
					XMLString a = node->NodeValue();
					XMLString b = next->NodeValue();
					a.Append(&b);
					node->NodeValue(&a);
					next = RemoveChild(next, false);
					if (!next->Referenced())
						conditionalDelete(next);
					next = node->fNextSibling;
				} while (next && next->fNodeType == XML_NODE_TEXT);
			}
		}
		node = node->fNextSibling;
	}
}

inline void ElementImpl::RemoveAttribute(XMLString *name)
{
	resetException();
	if (readOnly()) {
		treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
		return;
	}
	NodeImpl *attr = fAttributes->GetNamedItem(name);
	if (attr) {
		XMLString id("id", false);
		if (name->EqualsIgnoreCase(&id)) {
			if (fNodePtr)
				fNodePtr = 0;
		}
		RemoveAttributeNode((AttrImpl *)attr);
		if (!attr->Referenced())
			conditionalDelete(attr);
	}
}

inline AttrImpl *ElementImpl::RemoveAttributeNode(AttrImpl *oldAttr)
{
	resetException();
	if (readOnly()) {
		treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
		return 0;
	}
	XMLString name(oldAttr->fName);
	oldAttr = (AttrImpl *)fAttributes->RemoveNamedItem(&name);
	if (!oldAttr) {
		treatException(DOMException::NOT_FOUND_ERR);
		return 0;
	}
	XMLString id("id", false);
	if (name.EqualsIgnoreCase(&id)) {
		if (fNodePtr)
			fNodePtr = 0;
	}
	return oldAttr;
}

/* <tag attr1="v1" attr2="v2">children</tag>; each attribute writes itself (AttrImpl::Serialise). */
inline void ElementImpl::Serialise(XMLOutputSource *out)
{
	*out << "<" << &fName;
	NamedNodeMapImpl *attrs = fAttributes;
	unsigned n = attrs->Length();
	for (unsigned i = 0; i < n; i++)
		attrs->Item(i)->Serialise(out);
	*out << ">";
	NodeImpl::Serialise(out);
	*out << "</" << &fName << ">";
}

/*
 * With indentation off, as Serialise(out) with the children through the directive.
 * With indentation on, a new indented line before the open tag unless it follows text
 * (always for an empty HTML element), and before the close tag unless the last child
 * was text; the empty HTML elements (<br>...) get no close tag.
 */
void ElementImpl::Serialise(XMLOutputSource *out, OutputDirective *directive)
{
	if (directive->GetIndentBool()) {
		int oldCount = directive->IncrementIndentCount();
		int empty = directive->IsHTML40ElementName(fName);
		if (empty || directive->GetPrevNode() != OutputDirective::SNS_Text) {
			*out << XMLIOStr_Newline;
			for (int k = oldCount; k > 0; k--)
				*out << "\t";
		}
		*out << "<" << &fName;
		NamedNodeMapImpl *attrs = fAttributes;
		unsigned n = attrs->Length();
		for (unsigned i = 0; i < n; i++)
			attrs->Item(i)->Serialise(out);
		*out << ">";
		if (empty) {
			directive->SetPrevNode(OutputDirective::SNS_EndTag);
			directive->DecrementIndentCount();
		} else {
			directive->SetPrevNode(OutputDirective::SNS_StartTag);
		}
		NodeImpl::Serialise(out, directive);
		if (!empty) {
			OutputDirective::SerialiseNodeState prev = directive->GetPrevNode();
			directive->DecrementIndentCount();
			switch (prev) {
			case OutputDirective::SNS_StartTag:
			case OutputDirective::SNS_EndTag:
			case OutputDirective::SNS_4:
			case OutputDirective::SNS_Other:
				*out << XMLIOStr_Newline;
				for (int k = oldCount; k > 0; k--)
					*out << "\t";
				break;
			default:
				break;
			}
			*out << "</" << &fName << ">";
		}
		directive->SetPrevNode(OutputDirective::SNS_EndTag);
	} else {
		*out << "<" << &fName;
		NamedNodeMapImpl *attrs = fAttributes;
		unsigned n = attrs->Length();
		for (unsigned i = 0; i < n; i++)
			attrs->Item(i)->Serialise(out);
		*out << ">";
		NodeImpl::Serialise(out, directive);
		*out << "</" << &fName << ">";
	}
}

inline NodeImpl *ElementImpl::CloneNode(bool deep)
{
	return new ElementImpl(this, deep);
}

inline ElementImpl::ElementImpl(ElementImpl *other, bool deep) : NodeImpl(other, deep)
{}
