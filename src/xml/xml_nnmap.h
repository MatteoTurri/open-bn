/*
 * NamedNodeMap: reference-counted DOM handle, with NamedNodeMapImpl (nodes by name).
 */

#ifndef GUARD_XML_NNMAP_H
#define GUARD_XML_NNMAP_H
#include "xml_domimplobject.h"
#include "xml_nodelist.h"

class Node;
class XMLString;

class NamedNodeMapImpl : public DOMImplObject {
public:
	NamedNodeMapImpl(NodeImpl *owner);
	NamedNodeMapImpl(const NamedNodeMapImpl *other, NodeImpl *owner);
	virtual ~NamedNodeMapImpl();
	virtual void firstReference();
	virtual void lastReference();
	unsigned Length() const;
	int contained();
	NodeImpl *Item(unsigned long index) const;
	NodeImpl *GetNamedItem(XMLString *name);
	NodeImpl *SetNamedItem(NodeImpl *node);
	NodeImpl *RemoveNamedItem(XMLString *name);
	static void conditionalDelete(NamedNodeMapImpl *node);

	/* 0x10 */ NodeListImpl *fList;
	/* 0x14 */ NodeImpl *fOwnerNode;
};

class NamedNodeMap {
public:
	NamedNodeMap();
	NamedNodeMap(NamedNodeMapImpl *impl);
	NamedNodeMap(const NamedNodeMap &other);
	virtual ~NamedNodeMap();

	NamedNodeMap &operator=(const NamedNodeMap &other);
	NamedNodeMap &operator=(const int *other);
	bool operator==(const NamedNodeMap &other) const;
	bool operator!=(const NamedNodeMap &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;
	unsigned get_length() const;
	Node item(unsigned long index) const;
	Node getNamedItem(XMLString name);
	Node setNamedItem(Node node);
	Node removeNamedItem(XMLString name);

protected:
	NamedNodeMapImpl *fImpl;
};

#endif // GUARD_XML_NNMAP_H
