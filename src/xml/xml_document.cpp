/*
 * Document: the handle of the document node; the DOM factory methods (createElement,
 * createTextNode...), getElementsByTagName and serialisation to a file or a buffer.
 */

#include "xml_filedescriptor.h"
#include "xml_document.h"

DocumentImpl *Document::implementation() const
{
	return (DocumentImpl *)fImpl;
}

Document::Document(DocumentImpl *impl) : Node(impl)
{}

Document::Document()
{}

Document::Document(const Document &other) : Node(other)
{}

Document::Document(const Node &other) : Node(other)
{}

Document::~Document()
{}

Document Document::createDocument()
{
	return Document(new DocumentImpl());
}

Element Document::createElement(XMLString name)
{
	return Element(((DocumentImpl *)fImpl)->CreateElement(&name));
}

DocumentFragment Document::createDocumentFragment()
{
	return DocumentFragment(((DocumentImpl *)fImpl)->CreateDocumentFragment());
}

Text Document::createTextNode(XMLString data)
{
	return Text(((DocumentImpl *)fImpl)->CreateTextNode(&data));
}

Comment Document::createComment(XMLString data)
{
	return Comment(((DocumentImpl *)fImpl)->CreateComment(&data));
}

CDATASection Document::createCDATASection(XMLString data)
{
	return CDATASection(((DocumentImpl *)fImpl)->CreateCDATASection(&data));
}

ProcessingInstruction Document::createProcessingInstruction(XMLString target, XMLString data)
{
	return ProcessingInstruction(((DocumentImpl *)fImpl)->CreateProcessingInstruction(&target, &data));
}

Attr Document::createAttribute(XMLString name)
{
	return Attr(((DocumentImpl *)fImpl)->CreateAttribute(&name));
}

NodeList Document::getElementsByTagName(XMLString name)
{
	return NodeList(fImpl->GetElementsByTagName(&name, 0));
}

DOMImplementation Document::get_implementation() const
{
	return DOMImplementation(((DocumentImpl *)fImpl)->Implementation());
}

Element Document::get_documentElement() const
{
	return Element((ElementImpl *)((DocumentImpl *)fImpl)->DocumentElement());
}

Document &Document::operator=(const Document &other)
{
	return (Document &)Node::operator=(other);
}

bool Document::operator==(const Document &other) const
{
	return Node::operator==(other);
}

bool Document::operator!=(const Document &other) const
{
	return Node::operator!=(other);
}

Document &Document::operator=(const int *other)
{
	return (Document &)Node::operator=(other);
}

bool Document::operator==(const int *other) const
{
	return Node::operator==(other);
}

bool Document::operator!=(const int *other) const
{
	return Node::operator!=(other);
}

void Document::Serialise(char *fileName)
{
	((DocumentImpl *)fImpl)->Serialise(fileName);
}

void Document::Serialise(XMLFileDescriptor *fd)
{
	((DocumentImpl *)fImpl)->Serialise(fd);
}

/* Returns the serialised document by value. */
XMLString Document::Serialise()
{
	XMLString buffer;
	((DocumentImpl *)fImpl)->SerialiseToBuffer(&buffer);
	return buffer;
}

void Document::Serialise(XMLString *buffer, OutputDirective *directive)
{
	((DocumentImpl *)fImpl)->Serialise(buffer, directive);
}

XMLString Document::Serialise(OutputDirective *directive)
{
	XMLString buffer;
	((DocumentImpl *)fImpl)->SerialiseToBuffer(&buffer, directive);
	return buffer;
}

Node Document::importNode(Node other, bool deep)
{
	return Node(((DocumentImpl *)fImpl)->ImportNode(other.fImpl, deep));
}
