/*
 * ElementImpl: the element node behind the Element handle.
 */

#ifndef XML_ELEMENTIMPL_H
#define XML_ELEMENTIMPL_H
#pragma interface
#include "xml.h"

class AttrImpl;

class ElementImpl : public NodeImpl {
public:
	ElementImpl(DocumentImpl *ownerDoc, XMLString *name);
	ElementImpl(ElementImpl *other, bool deep);
	virtual ~ElementImpl();
	virtual NodeImpl *CloneNode(bool deep);
	virtual void Serialise(XMLOutputSource *out);
	virtual void Serialise(XMLOutputSource *out, OutputDirective *directive);

	XMLString TagName();
	XMLString GetAttribute(XMLString *name);
	void SetAttribute(XMLString *name, XMLString *value);
	void RemoveAttribute(XMLString *name);
	void Normalize();
	AttrImpl *GetAttributeNode(XMLString *name);
	AttrImpl *SetAttributeNode(AttrImpl *newAttr, bool create);
	AttrImpl *RemoveAttributeNode(AttrImpl *oldAttr);
};

#endif
