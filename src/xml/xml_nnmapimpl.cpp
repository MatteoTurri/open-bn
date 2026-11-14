/*
 * Methods of NamedNodeMapImpl, the node map behind the NamedNodeMap handle.
 */

#include "xml_nnmap.h"
#include "xml.h"
#include "xml_filedescriptor.h"

NamedNodeMapImpl::NamedNodeMapImpl(NodeImpl *owner)
{
	resetException();
	fOwnerNode = owner;
	fList = new NodeListImpl();
}

NodeImpl *NamedNodeMapImpl::GetNamedItem(XMLString *name)
{
	return fList->GetNamedItem(name);
}

NodeImpl *NamedNodeMapImpl::SetNamedItem(NodeImpl *node)
{
	resetException();
	if (Node::PerformCheck) {
		if (fOwnerNode->fOwnerDocument != node->fOwnerDocument)
			treatException(DOMException::WRONG_DOCUMENT_ERR);
		if (fOwnerNode->readOnly())
			treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
		if (node->contained())
			treatException(DOMException::INUSE_ATTRIBUTE_ERR);
	}
	NodeImpl *old = 0;
	if (Node::PerformCheck) {
		XMLString tmp(node->fName);
		old = fList->RemoveNamedItem(&tmp);
	}
	fList->SetNamedItem(node);
	return old;
}

NodeImpl *NamedNodeMapImpl::RemoveNamedItem(XMLString *name)
{
	resetException();
	NodeImpl *old = fList->RemoveNamedItem(name);
	if (old == 0)
		treatException(DOMException::NOT_FOUND_ERR);
	return old;
}

NodeImpl *NamedNodeMapImpl::Item(unsigned long index) const
{
	return fList->Item(index);
}

unsigned NamedNodeMapImpl::Length() const
{
	return fList->fLength;
}

NamedNodeMapImpl::~NamedNodeMapImpl()
{
	delete fList;
}

void NamedNodeMapImpl::lastReference()
{
	conditionalDelete(this);
}

void NamedNodeMapImpl::firstReference()
{}

void NamedNodeMapImpl::conditionalDelete(NamedNodeMapImpl *node)
{
	if (node && !node->contained())
		delete node;
}

int NamedNodeMapImpl::contained()
{
	return fOwnerNode != 0;
}

NamedNodeMapImpl::NamedNodeMapImpl(const NamedNodeMapImpl *other, NodeImpl *owner)
{
	fList = new NodeListImpl(other->fList);
	fOwnerNode = owner;
}
