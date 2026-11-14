/*
 * NodeListElement: node of the doubly linked list of NodeListImpl.
 */

#ifndef GUARD_XML_NODELISTELEMENT_H
#define GUARD_XML_NODELISTELEMENT_H

class NodeImpl;

class NodeListElement {
public:
	NodeListElement(NodeImpl *node);
	virtual ~NodeListElement();
	NodeImpl *node();

	/* 0x00 */ NodeImpl *fNode;
	/* 0x04 */ NodeListElement *fNext;
	/* 0x08 */ NodeListElement *fPrev;
	/* 0x0C: vtable pointer */
};

#endif // GUARD_XML_NODELISTELEMENT_H
