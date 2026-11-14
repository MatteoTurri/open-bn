/*
 * Comment node: the implementation class and the DOM handle (derives from CharacterData).
 */

#ifndef GUARD_XML_COMMENT_H
#define GUARD_XML_COMMENT_H
#include "xml.h"
#include "xml_chardata.h"

class CommentImpl : public CharacterDataImpl {
public:
	CommentImpl(DocumentImpl *ownerDoc, XMLString *data);
	CommentImpl(CommentImpl *other, bool deep);
	virtual ~CommentImpl();
	virtual NodeImpl *CloneNode(bool deep);
	virtual void Serialise(XMLOutputSource *out);
};

class Comment : public CharacterData {
public:
	Comment();
	Comment(CommentImpl *impl);
	Comment(const Node &other);
	virtual ~Comment();

	Comment &operator=(const Comment &other);
	Comment &operator=(const int *other);
	bool operator==(const Comment &other) const;
	bool operator!=(const Comment &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;
};

#endif // GUARD_XML_COMMENT_H
