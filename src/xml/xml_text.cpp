/*
 * Text: the handle of a text node; construction, assignment and comparison only.
 */

#include "xml_text.h"

Text::Text(TextImpl *impl) : CharacterData(impl)
{}

Text::Text()
{}

Text::Text(const Node &other) : CharacterData(other)
{}

Text::~Text()
{}

Text &Text::operator=(const Text &other)
{
	return (Text &)CharacterData::operator=(other);
}

bool Text::operator==(const Text &other) const
{
	return Node::operator==(other);
}

bool Text::operator!=(const Text &other) const
{
	return Node::operator!=(other);
}

Text &Text::operator=(const int *other)
{
	return (Text &)Node::operator=(other);
}

bool Text::operator==(const int *other) const
{
	return Node::operator==(other);
}

bool Text::operator!=(const int *other) const
{
	return Node::operator!=(other);
}
