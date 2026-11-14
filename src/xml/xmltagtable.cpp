/*
 * XmlTagTable and the three DOM-glue helpers that register and remove element
 * subtrees.
 */

#include "xmltagtable.h"

/* Reference-counted body shared by the copies of an XmlTagTable. */
struct XmlTagTable::Rep {
	/* 0x00 */ int refcount;
	/* 0x04 */ Dictionary<Node, unsigned, foGetIntReprNode> dict; /* node -> handle */
	/* 0x14 */ vector<Node> nodes; /* handle -> node */
	/* Slot 0 holds a null Node: handle 0 means "no tag". */
	Rep() : refcount(1), dict(153, -1) { nodes.push_back(Node((NodeImpl *)0)); }
	~Rep() {}
};  /* size = 0x20 */

XmlTagTable::XmlTagTable()
{
	rep = new Rep;
}

XmlTagTable::~XmlTagTable()
{
	Rep *r = rep;
	if (r) {
		if (--r->refcount == 0)
			delete r;
	}
}

XmlTagTable::XmlTagTable(const XmlTagTable &other)
{
	rep = other.rep;
	rep->refcount++;
}

void XmlTagTable::operator=(const XmlTagTable &other)
{
	/* Assigning an empty table clears rep. */
	Rep *old = rep;
	Rep *src = other.rep;
	rep = src;
	if (src)
		src->refcount++;
	if (old) {
		if (--old->refcount == 0)
			delete old;
	}
}

/* Stores node in the first free (null) slot from 1, or appends it; the slot
   index is the handle, also recorded in the dictionary (node -> handle). */
unsigned XmlTagTable::Regist(Node node)
{
	unsigned i;
	for (i = 1; i < rep->nodes.size(); i++)
		if (rep->nodes[i] == 0)
			break;
	if (i < rep->nodes.size())
		rep->nodes[i] = node;
	else
		rep->nodes.push_back(node);
	rep->dict.Regist(node, i);
	return i;
}

Node XmlTagTable::GetNode(unsigned handle)
{
	if (handle != 0 && handle < rep->nodes.size())
		return rep->nodes[handle];
	return Node((NodeImpl *)0);
}

unsigned XmlTagTable::GetHandle(Node node)
{
	unsigned handle;
	return rep->dict.Get(node, &handle) ? handle : 0;
}

void XmlTagTable::Remove(unsigned handle)
{
	if (handle != 0 && handle < rep->nodes.size()) {
		Node node = rep->nodes[handle];
		rep->dict.Remove(node);
		rep->nodes[handle] = 0;
	}
}

void XmlTagTable::Dump() const
{
	for (unsigned i = 1; i < rep->nodes.size(); i++)
		if (rep->nodes[i] != 0)
			printf("tag handle [%d] = %x\n", i, foGetIntReprNode()(rep->nodes[i]));
	rep->dict.Dump();
}

unsigned FindTagFromList(XmlTagTable &table, Node node)
{
	if (node == 0)
		return 0;
	return table.GetHandle(node);
}

/* Registers node and, recursively, all its element children (type 1). */
unsigned TagList_Add(XmlTagTable &table, Node node)
{
	if (node == 0)
		return 0;
	unsigned handle = table.Regist(node);
	Node child = node.get_firstChild();
	while (child != 0) {
		if (child.get_nodeType() == 1)
			TagList_Add(table, child);
		child = child.get_nextSibling();
	}
	return handle;
}

void TagList_Remove(XmlTagTable &table, int handle)
{
	if (handle == 0)
		return;
	for (Node child = GetTagNode(&table, handle).get_firstChild(); child != 0; child = child.get_nextSibling()) {
		if (child != 0 && child.get_nodeType() == 1)
			TagList_Remove(table, FindTagFromList(table, child));
	}
	table.Remove(handle);
}
