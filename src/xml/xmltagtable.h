/*
 * XmlTagTable: maps DOM element nodes to the integer handles used by scripts.
 */

#ifndef GUARD_XMLTAGTABLE_H
#define GUARD_XMLTAGTABLE_H
#include <vector>
#include "xml.h"

#include "dictionary.h"

/*
 * Table of element nodes exposed to scripts as integer handles. Copies share
 * one reference-counted Rep; a handle is an index into Rep::nodes and handle 0
 * means "no tag".
 */
class XmlTagTable {
public:
	/* Defined in xmltagtable.cpp. */
	struct Rep;

	Rep *rep;

	XmlTagTable();
	XmlTagTable(const XmlTagTable &other);
	~XmlTagTable();
	void operator=(const XmlTagTable &other);

	unsigned Regist(Node node);
	Node GetNode(unsigned handle);
	unsigned GetHandle(Node node);
	void Remove(unsigned handle);
	void Dump() const;
};

/* Resolves a script handle to its node; clients use this, not GetNode(). */
inline Node GetTagNode(XmlTagTable *list, unsigned handle)
{
	return list->GetNode(handle);
}

unsigned FindTagFromList(XmlTagTable &table, Node node);
unsigned TagList_Add(XmlTagTable &table, Node node);
void TagList_Remove(XmlTagTable &table, int handle);

#endif /* GUARD_XMLTAGTABLE_H */
