/*
 * NodeListElement: one link of the doubly linked list behind NodeListImpl.
 */

#include "xml_nodelistelement.h"

NodeImpl *NodeListElement::node()
{
	return fNode;
}

NodeListElement::NodeListElement(NodeImpl *node)
{
	fPrev = 0;
	fNext = 0;
	fNode = node;
}

NodeListElement::~NodeListElement()
{}
