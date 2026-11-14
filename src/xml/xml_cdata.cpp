/*
 * CDATASection: the handle of a CDATA section node; construction, assignment and
 * comparison only, the data is reached through CharacterData.
 */

#include "xml_cdata.h"

CDATASection::CDATASection(CDATASectionImpl *impl) : Text(impl)
{}

CDATASection::CDATASection()
{}

CDATASection::CDATASection(const Node &other) : Text(other)
{}

CDATASection::~CDATASection()
{}

CDATASection &CDATASection::operator=(const CDATASection &other)
{
	return (CDATASection &)Text::operator=(other);
}

bool CDATASection::operator==(const CDATASection &other) const
{
	return Node::operator==(other);
}

bool CDATASection::operator!=(const CDATASection &other) const
{
	return Node::operator!=(other);
}

CDATASection &CDATASection::operator=(const int *other)
{
	return (CDATASection &)Node::operator=(other);
}

bool CDATASection::operator==(const int *other) const
{
	return Node::operator==(other);
}

bool CDATASection::operator!=(const int *other) const
{
	return Node::operator!=(other);
}
