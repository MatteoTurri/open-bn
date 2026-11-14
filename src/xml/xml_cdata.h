/*
 * CDATASection node: the implementation class and the DOM handle (derives from Text).
 */

#ifndef GUARD_XML_CDATA_H
#define GUARD_XML_CDATA_H
#include "xml.h"
#include "xml_text.h"

class CDATASectionImpl : public TextImpl {
public:
	CDATASectionImpl(DocumentImpl *ownerDoc, XMLString *data);
	CDATASectionImpl(CDATASectionImpl *other, bool deep);
	virtual ~CDATASectionImpl();
	virtual NodeImpl *CloneNode(bool deep);
	virtual void Serialise(XMLOutputSource *out);
};

class CDATASection : public Text {
public:
	CDATASection();
	CDATASection(CDATASectionImpl *impl);
	CDATASection(const Node &other);
	virtual ~CDATASection();

	CDATASection &operator=(const CDATASection &other);
	CDATASection &operator=(const int *other);
	bool operator==(const CDATASection &other) const;
	bool operator!=(const CDATASection &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;
};

#endif // GUARD_XML_CDATA_H
