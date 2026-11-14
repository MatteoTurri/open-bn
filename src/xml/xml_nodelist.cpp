/*
 * NodeList: the reference-counted handle of a node list; item and length.
 */

#include "xml_filedescriptor.h"
#include "xml_nodelist.h"
#include "xml.h"

NodeList::NodeList(NodeListImpl *impl)
{
	fImpl = impl;
	if (impl)
		GCObject::AddRef(impl);
}

NodeList::NodeList()
{
	fImpl = 0;
}

NodeList::NodeList(const NodeList &other)
{
	fImpl = other.fImpl;
	if (fImpl)
		GCObject::AddRef(fImpl);
}

NodeList::~NodeList()
{
	if (fImpl)
		GCObject::RemoveRef(fImpl);
}

NodeList &NodeList::operator=(const NodeList &other)
{
	if (fImpl != other.fImpl) {
		if (fImpl)
			GCObject::RemoveRef(fImpl);
		fImpl = other.fImpl;
		if (fImpl)
			GCObject::AddRef(fImpl);
	}
	return *this;
}

bool NodeList::operator==(const NodeList &other) const
{
	return fImpl == other.fImpl;
}

bool NodeList::operator!=(const NodeList &other) const
{
	return fImpl != other.fImpl;
}

NodeList &NodeList::operator=(const int *other)
{
	if (other == 0) {
		if (fImpl) {
			GCObject::RemoveRef(fImpl);
			fImpl = 0;
		}
	}
	return *this;
}

bool NodeList::operator==(const int *other) const
{
	if (other == 0)
		return fImpl == 0;
	return false;
}

bool NodeList::operator!=(const int *other) const
{
	if (other == 0)
		return fImpl != 0;
	return true;
}

Node NodeList::item(unsigned long index)
{
	return Node(fImpl->Item(index));
}

unsigned NodeList::get_length() const
{
	if (fImpl)
		return fImpl->fLength;
	return 0;
}
