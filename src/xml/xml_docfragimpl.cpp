/*
 * Methods of DocumentFragmentImpl (node type 11, name "#document-fragment").
 */

#include "xml_docfrag.h"
#include "xml_outputsource.h"

DocumentFragmentImpl::DocumentFragmentImpl(DocumentImpl *ownerDoc)
        : NodeImpl(ownerDoc, 0, 0, XML_NODE_DOCUMENT_FRAGMENT)
{
	fName.Set("#document-fragment", 18);
}

DocumentFragmentImpl::DocumentFragmentImpl(DocumentFragmentImpl *other, bool deep) : NodeImpl(other, deep)
{}

DocumentFragmentImpl::~DocumentFragmentImpl()
{}

NodeImpl *DocumentFragmentImpl::CloneNode(bool deep)
{
	return new DocumentFragmentImpl(this, deep);
}

/* A fragment has no text form: writes a marker comment, then the children. */
void DocumentFragmentImpl::Serialise(XMLOutputSource *out)
{
	*out << "<!-- Document fragment node not printed -->";
	NodeImpl::Serialise(out);
}
