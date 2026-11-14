/*
 * Comment: the handle of a comment node; construction, assignment and comparison only.
 */

#include "xml_comment.h"

Comment::Comment(CommentImpl *impl) : CharacterData(impl)
{}

Comment::Comment()
{}

Comment::Comment(const Node &other) : CharacterData(other)
{}

Comment::~Comment()
{}

Comment &Comment::operator=(const Comment &other)
{
	return (Comment &)CharacterData::operator=(other);
}

bool Comment::operator==(const Comment &other) const
{
	return Node::operator==(other);
}

bool Comment::operator!=(const Comment &other) const
{
	return Node::operator!=(other);
}

Comment &Comment::operator=(const int *other)
{
	return (Comment &)Node::operator=(other);
}

bool Comment::operator==(const int *other) const
{
	return Node::operator==(other);
}

bool Comment::operator!=(const int *other) const
{
	return Node::operator!=(other);
}
