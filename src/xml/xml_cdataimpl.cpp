/*
 * Methods of CDATASectionImpl (name "#cdata-section"), derived from TextImpl.
 */

#include "xml_cdata.h"
#include "xml_outputsource.h"

CDATASectionImpl::CDATASectionImpl(DocumentImpl *ownerDoc, XMLString *data)
        : TextImpl(ownerDoc, 0, data, XML_NODE_CDATA_SECTION)
{
	fName.Set("#cdata-section", 14);
}

CDATASectionImpl::CDATASectionImpl(CDATASectionImpl *other, bool deep) : TextImpl(other, deep)
{}

CDATASectionImpl::~CDATASectionImpl()
{}

NodeImpl *CDATASectionImpl::CloneNode(bool deep)
{
	return new CDATASectionImpl(this, deep);
}

void CDATASectionImpl::Serialise(XMLOutputSource *out)
{
	*out << "<![CDATA[";
	*out << &fValue;
	*out << "]]>";
}
