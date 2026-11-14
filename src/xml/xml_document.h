/*
 * Document node: DocumentImpl and the DOM handle Document.
 */

#ifndef GUARD_XML_DOCUMENT_H
#define GUARD_XML_DOCUMENT_H
#include "xml.h"
#include "xml_element.h"
#include "xml_domimpl.h"
#include "xml_docfrag.h"
#include "xml_text.h"
#include "xml_comment.h"
#include "xml_cdata.h"
#include "xml_pi.h"
#include "xml_nodelist.h"
#include "xml_attr.h"

class XMLFileDescriptor;

class DocumentImpl : public NodeImpl {
public:
	DocumentImpl();
	DocumentImpl(DocumentImpl *other, bool deep);
	virtual ~DocumentImpl();
	virtual void SerialiseToBuffer(XMLString *buffer);
	virtual NodeImpl *CloneNode(bool deep);
	/* Own virtuals of DocumentImpl, after those of NodeImpl. */
	virtual void Serialise(char *fileName);
	virtual void Serialise(XMLFileDescriptor *fd);
	virtual void Serialise(XMLString *buffer, OutputDirective *directive);
	virtual void SerialiseToBuffer(XMLString *buffer, OutputDirective *directive);

	DOMImplementationImpl *Implementation();
	NodeImpl *DocumentElement();
	DocumentFragmentImpl *CreateDocumentFragment();
	ElementImpl *CreateElement(XMLString *name);
	TextImpl *CreateTextNode(XMLString *data);
	CommentImpl *CreateComment(XMLString *data);
	CDATASectionImpl *CreateCDATASection(XMLString *data);
	ProcessingInstructionImpl *CreateProcessingInstruction(XMLString *target, XMLString *data);
	AttrImpl *CreateAttribute(XMLString *name);
	NodeImpl *ImportNode(NodeImpl *source, bool deep);

	/* 0x70 */ DOMImplementationImpl *fImplementation;
	/* 0x74 */ DOMImplObject *unk_74;  /* deleted by the destructor */
};

class OutputDirective;

class Document : public Node {
public:
	Document();
	Document(DocumentImpl *impl);
	Document(const Node &other);
	Document(const Document &other);
	virtual ~Document();

	static Document createDocument();

	Document &operator=(const Document &other);
	Document &operator=(const int *other);
	bool operator==(const Document &other) const;
	bool operator!=(const Document &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;

	DocumentImpl *implementation() const;
	DOMImplementation get_implementation() const;
	Element get_documentElement() const;
	DocumentFragment createDocumentFragment();
	Element createElement(XMLString name);
	Text createTextNode(XMLString data);
	Comment createComment(XMLString data);
	CDATASection createCDATASection(XMLString data);
	ProcessingInstruction createProcessingInstruction(XMLString target, XMLString data);
	Attr createAttribute(XMLString name);
	NodeList getElementsByTagName(XMLString name);
	Node importNode(Node other, bool deep);
	/* Serialise(OutputDirective*) is the only non-virtual overload. */
	virtual void Serialise(char *fileName);
	virtual void Serialise(XMLFileDescriptor *fd);
	virtual XMLString Serialise();
	virtual void Serialise(XMLString *buffer, OutputDirective *directive);
	XMLString Serialise(OutputDirective *directive);
};

#endif // GUARD_XML_DOCUMENT_H
