/*
 * NamedNodeMap: the reference-counted handle of a node map (the attributes of an
 * element); lookup by name or index, insertion and removal.
 */

#include "xml_nnmap.h"
#include "xml.h"

NamedNodeMap::NamedNodeMap(NamedNodeMapImpl *impl)
{
	fImpl = impl;
	if (impl)
		GCObject::AddRef(impl);
}

NamedNodeMap::NamedNodeMap()
{
	fImpl = 0;
}

NamedNodeMap::NamedNodeMap(const NamedNodeMap &other)
{
	fImpl = other.fImpl;
	if (fImpl)
		GCObject::AddRef(fImpl);
}

NamedNodeMap::~NamedNodeMap()
{
	if (fImpl)
		GCObject::RemoveRef(fImpl);
}

NamedNodeMap &NamedNodeMap::operator=(const NamedNodeMap &other)
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

bool NamedNodeMap::operator==(const NamedNodeMap &other) const
{
	return fImpl == other.fImpl;
}

bool NamedNodeMap::operator!=(const NamedNodeMap &other) const
{
	return fImpl != other.fImpl;
}

NamedNodeMap &NamedNodeMap::operator=(const int *other)
{
	if (other == 0) {
		if (fImpl) {
			GCObject::RemoveRef(fImpl);
			fImpl = 0;
		}
	}
	return *this;
}

bool NamedNodeMap::operator==(const int *other) const
{
	if (other == 0)
		return fImpl == 0;
	return false;
}

bool NamedNodeMap::operator!=(const int *other) const
{
	if (other == 0)
		return fImpl != 0;
	return true;
}

Node NamedNodeMap::getNamedItem(XMLString name)
{
	return Node(fImpl->GetNamedItem(&name));
}

Node NamedNodeMap::setNamedItem(Node node)
{
	return Node(fImpl->SetNamedItem(node.fImpl));
}

Node NamedNodeMap::removeNamedItem(XMLString name)
{
	return Node(fImpl->RemoveNamedItem(&name));
}

Node NamedNodeMap::item(unsigned long index) const
{
	return Node(fImpl->Item(index));
}

unsigned NamedNodeMap::get_length() const
{
	return fImpl->Length();
}
