/*
 * Methods of CommentImpl (name "#comment").
 */

#include "xml_comment.h"
#include "xml_outputsource.h"

CommentImpl::CommentImpl(DocumentImpl *ownerDoc, XMLString *data)
        : CharacterDataImpl(ownerDoc, 0, data, XML_NODE_COMMENT)
{
	fName.Set("#comment", 8);
}

CommentImpl::CommentImpl(CommentImpl *other, bool deep) : CharacterDataImpl(other, deep)
{}

CommentImpl::~CommentImpl()
{}

NodeImpl *CommentImpl::CloneNode(bool deep)
{
	return new CommentImpl(this, deep);
}

void CommentImpl::Serialise(XMLOutputSource *out)
{
	*out << "<!--" << &fValue << "-->";
}
