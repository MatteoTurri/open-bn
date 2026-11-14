/*
 * NodeList: reference-counted DOM handle, with NodeListImpl (a doubly linked list of nodes).
 */

#ifndef GUARD_XML_NODELIST_H
#define GUARD_XML_NODELIST_H
#include "xml_domimplobject.h"
#include "xml_nodelistelement.h"

class Node;
class NodeImpl;
class XMLString;

class NodeListImpl : public DOMImplObject {
public:
	NodeListImpl();
	NodeListImpl(const NodeListImpl *other);
	virtual ~NodeListImpl();
	virtual void firstReference();
	virtual void lastReference();
	unsigned Length();
	int contained();
	NodeImpl *Item(unsigned long index);
	NodeImpl *insertFirst(NodeImpl *node);
	NodeImpl *insertLast(NodeImpl *node);
	NodeImpl *SetNamedItem(NodeImpl *node);
	NodeImpl *GetNamedItem(XMLString *name);
	NodeImpl *RemoveNamedItem(XMLString *name);
	NodeImpl *removeItem(NodeImpl *node);
	NodeImpl *findItem(XMLString *name);
	NodeImpl *findItem(NodeImpl *node);
	static void conditionalDelete(NodeListImpl *node);

	/* 0x10 */ NodeListElement *fFirst;
	/* 0x14 */ NodeListElement *fLast;
	/* 0x18 */ unsigned fLength;
};

class NodeList {
public:
	NodeList();
	NodeList(NodeListImpl *impl);
	NodeList(const NodeList &other);
	virtual ~NodeList();

	NodeList &operator=(const NodeList &other);
	NodeList &operator=(const int *other);
	bool operator==(const NodeList &other) const;
	bool operator!=(const NodeList &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;
	unsigned get_length() const;
	Node item(unsigned long index);

protected:
	NodeListImpl *fImpl;
};

#endif // GUARD_XML_NODELIST_H
