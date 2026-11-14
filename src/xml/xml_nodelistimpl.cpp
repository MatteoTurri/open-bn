/*
 * Methods of NodeListImpl, the list behind the NodeList handle.
 */

#include "xml_nodelist.h"
#include "xml.h"

static inline XMLString nameOf(NodeImpl *node)
{
	return node->fName;
}

unsigned NodeListImpl::Length()
{
	return fLength;
}

int NodeListImpl::contained()
{
	return 0;
}

NodeListImpl::NodeListImpl()
{
	fFirst = 0;
	fLast = 0;
	fLength = 0;
}

NodeImpl *NodeListImpl::insertFirst(NodeImpl *node)
{
	NodeListElement *elem = new NodeListElement(node);
	if (fFirst) {
		fFirst->fPrev = elem;
		elem->fNext = fFirst;
	} else {
		fLast = elem;
	}
	fFirst = elem;
	node->AddOwner();
	fLength++;
	return node;
}

NodeImpl *NodeListImpl::insertLast(NodeImpl *node)
{
	NodeListElement *elem = new NodeListElement(node);
	if (fLast) {
		fLast->fNext = elem;
		elem->fPrev = fLast;
	} else {
		fFirst = elem;
	}
	fLast = elem;
	node->AddOwner();
	fLength++;
	return node;
}

NodeImpl *NodeListImpl::Item(unsigned long index)
{
	if (index >= fLength)
		return 0;
	NodeListElement *elem = fFirst;
	for (unsigned long i = 0; i < index; i++)
		elem = elem->fNext;
	return elem->node();
}

NodeImpl *NodeListImpl::GetNamedItem(XMLString *name)
{
	return findItem(name);
}

NodeImpl *NodeListImpl::SetNamedItem(NodeImpl *node)
{
	NodeListElement *elem = new NodeListElement(node);
	if (fLast) {
		fLast->fNext = elem;
		elem->fPrev = fLast;
	} else {
		fFirst = elem;
	}
	fLast = elem;
	node->AddOwner();
	fLength++;
	return node;
}

NodeImpl *NodeListImpl::RemoveNamedItem(XMLString *name)
{
	NodeImpl *node = findItem(name);
	if (node)
		return removeItem(node);
	return 0;
}

NodeImpl *NodeListImpl::removeItem(NodeImpl *node)
{
	for (NodeListElement *elem = fFirst; fFirst && elem; elem = elem->fNext) {  /* redundant as written */
		NodeImpl *cur = elem->node();
		if (node == cur) {
			if (elem->fPrev)
				elem->fPrev->fNext = elem->fNext;
			else
				fFirst = elem->fNext;
			if (elem->fNext)
				elem->fNext->fPrev = elem->fPrev;
			else
				fLast = elem->fPrev;
			cur->RemoveOwner();
			delete elem;
			fLength--;
			return cur;
		}
	}
	return 0;
}

NodeImpl *NodeListImpl::findItem(XMLString *name)
{
	NodeListElement *elem = fFirst;
	XMLString value;
	for (; elem; elem = elem->fNext) {
		NodeImpl *node = elem->node();
		if (*name == nameOf(node))
			return node;
	}
	return 0;
}

NodeImpl *NodeListImpl::findItem(NodeImpl *node)
{
	for (NodeListElement *elem = fFirst; elem; elem = elem->fNext) {
		NodeImpl *cur = elem->node();
		if (cur == node)
			return cur;
	}
	return 0;
}

NodeListImpl::~NodeListImpl()
{
	if (fFirst) {
		NodeListElement *elem = fFirst;
		do {
			NodeImpl *node = elem->node();
			fFirst = elem->fNext;
			if (!node->RemoveOwner() && !node->Referenced())
				NodeImpl::conditionalDelete(node);
			delete elem;
		} while ((elem = fFirst) != 0);
	}
}

void NodeListImpl::lastReference()
{
	conditionalDelete(this);
}

void NodeListImpl::conditionalDelete(NodeListImpl *node)
{
	delete node;
}

void NodeListImpl::firstReference()
{}

NodeListImpl::NodeListImpl(const NodeListImpl *other)
{
	fFirst = 0;
	fLast = 0;
	fLength = 0;
	for (NodeListElement *elem = other->fFirst; elem; elem = elem->fNext) {
		NodeImpl *clone = elem->node()->CloneNode(true);
		NodeListElement *e = new NodeListElement(clone);
		if (fLast) {
			fLast->fNext = e;
			e->fPrev = fLast;
		} else {
			fFirst = e;
		}
		fLast = e;
		clone->AddOwner();
		fLength++;
	}
}
