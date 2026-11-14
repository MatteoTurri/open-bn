/*
 * DocumentFragment: the handle of a document fragment node; construction, assignment
 * and comparison only.
 */

#include "xml_docfrag.h"

DocumentFragment::DocumentFragment(DocumentFragmentImpl *impl) : Node(impl)
{}

DocumentFragment::DocumentFragment()
{}

DocumentFragment::DocumentFragment(const Node &other) : Node(other)
{}

DocumentFragment::~DocumentFragment()
{}

DocumentFragment &DocumentFragment::operator=(const DocumentFragment &other)
{
	return (DocumentFragment &)Node::operator=(other);
}

bool DocumentFragment::operator==(const DocumentFragment &other) const
{
	return Node::operator==(other);
}

bool DocumentFragment::operator!=(const DocumentFragment &other) const
{
	return Node::operator!=(other);
}

DocumentFragment &DocumentFragment::operator=(const int *other)
{
	return (DocumentFragment &)Node::operator=(other);
}

bool DocumentFragment::operator==(const int *other) const
{
	return Node::operator==(other);
}

bool DocumentFragment::operator!=(const int *other) const
{
	return Node::operator!=(other);
}
