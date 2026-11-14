/*
 * DocumentFragment node: the implementation class and the DOM handle.
 */

#ifndef GUARD_XML_DOCFRAG_H
#define GUARD_XML_DOCFRAG_H
#include "xml.h"

class DocumentFragmentImpl : public NodeImpl {
public:
	DocumentFragmentImpl(DocumentImpl *ownerDoc);
	DocumentFragmentImpl(DocumentFragmentImpl *other, bool deep);
	virtual ~DocumentFragmentImpl();
	virtual NodeImpl *CloneNode(bool deep);
	virtual void Serialise(XMLOutputSource *out);
};

class DocumentFragment : public Node {
public:
	DocumentFragment();
	DocumentFragment(DocumentFragmentImpl *impl);
	DocumentFragment(const Node &other);
	virtual ~DocumentFragment();

	DocumentFragment &operator=(const DocumentFragment &other);
	DocumentFragment &operator=(const int *other);
	bool operator==(const DocumentFragment &other) const;
	bool operator!=(const DocumentFragment &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;
};

#endif // GUARD_XML_DOCFRAG_H
