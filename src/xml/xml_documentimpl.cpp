/*
 * Methods of DocumentImpl (node type 9, name "#document").
 * Names are validated while Node::PerformCheck is set; errors go to treatException.
 */

#include "xml_filedescriptor.h"
#include "xml_document.h"
#include "xml_attr.h"
#include "xml_outputsource.h"
#include "xml_outputdirective.h"

DocumentImpl::DocumentImpl() : NodeImpl(0, 0, 0, XML_NODE_DOCUMENT)
{
	XMLString name("#document", true);
	fName.XMLStringCopy(&name);
	fImplementation = 0;
	unk_74 = 0;
}

DocumentImpl::~DocumentImpl()
{
	delete fImplementation;
	delete unk_74;
}

DocumentImpl::DocumentImpl(DocumentImpl *other, bool deep) : NodeImpl(other, deep)
{
	fImplementation = 0;
	unk_74 = 0;
}

DOMImplementationImpl *DocumentImpl::Implementation()
{
	if (!fImplementation)
		fImplementation = new DOMImplementationImpl();
	return fImplementation;
}

NodeImpl *DocumentImpl::DocumentElement()
{
	NodeImpl *n = fFirstChild;
	while (n && n->fNodeType != XML_NODE_ELEMENT)
		n = n->fNextSibling;
	return n;
}

ElementImpl *DocumentImpl::CreateElement(XMLString *name)
{
	resetException();
	if (Node::PerformCheck && !name->isValidNameOrNmtoken(true)) {
		treatException(DOMException::INVALID_CHARACTER_ERR);
		return 0;
	}
	return new ElementImpl(this, name);
}

DocumentFragmentImpl *DocumentImpl::CreateDocumentFragment()
{
	return new DocumentFragmentImpl(this);
}

TextImpl *DocumentImpl::CreateTextNode(XMLString *data)
{
	return new TextImpl(this, data);
}

CommentImpl *DocumentImpl::CreateComment(XMLString *data)
{
	return new CommentImpl(this, data);
}

CDATASectionImpl *DocumentImpl::CreateCDATASection(XMLString *data)
{
	resetException();
	return new CDATASectionImpl(this, data);
}

ProcessingInstructionImpl *DocumentImpl::CreateProcessingInstruction(XMLString *target, XMLString *data)
{
	resetException();
	XMLString xml("xml", false);
	if (!target->isValidNameOrNmtoken(true) || target->EqualsIgnoreCase(&xml)) {
		treatException(DOMException::INVALID_CHARACTER_ERR);
		return 0;
	}
	return new ProcessingInstructionImpl(this, target, data);
}

AttrImpl *DocumentImpl::CreateAttribute(XMLString *name)
{
	resetException();
	if (Node::PerformCheck && !name->isValidNameOrNmtoken(true)) {
		treatException(DOMException::INVALID_CHARACTER_ERR);
		return 0;
	}
	return new AttrImpl(this, name);
}

/* Writes to a file through a temporary XMLOutputSource (XMLIOType_File). */
void DocumentImpl::Serialise(char *fileName)
{
	XMLString *name = new XMLString(fileName, false);
	XMLOutputSource *out = new XMLOutputSource(name, XMLIOType_File, XMLCode_UTF8, 1024);
	NodeImpl::Serialise(out);
	delete name;
	delete out;
}

void DocumentImpl::SerialiseToBuffer(XMLString *buffer)
{
	XMLOutputSource *out = new XMLOutputSource(buffer, XMLIOType_Memory, XMLCode_UTF8, 1024);
	NodeImpl::Serialise(out);
	delete out;
}

/* Writes to an open descriptor; the buffer belongs to the descriptor. */
void DocumentImpl::Serialise(XMLFileDescriptor *fd)
{
	XMLOutputSource *out = new XMLOutputSource(fd->fBuffer, fd->fIOType, fd->fCodeType, 1024);
	NodeImpl::Serialise(out);
	delete out;
}

/*
 * Writes to a copy of buffer with the code type of the directive: first the directive
 * itself (prolog or DOCTYPE), then the children, then a final line break.
 */
void DocumentImpl::Serialise(XMLString *buffer, OutputDirective *directive)
{
	XMLString *clone = buffer->Clone();
	XMLOutputSource *out = new XMLOutputSource(clone, XMLIOType_File, (XMLCodeType)directive->GetCodeType(), 1024);
	directive->Serialise(out);
	NodeImpl::Serialise(out, directive);
	*out << XMLIOStr_Newline;
	delete clone;
	delete out;
}

/* As above, but directly into the buffer of the caller. */
void DocumentImpl::SerialiseToBuffer(XMLString *buffer, OutputDirective *directive)
{
	XMLOutputSource *out =
	    new XMLOutputSource(buffer, XMLIOType_Memory, (XMLCodeType)directive->GetCodeType(), 1024);
	directive->Serialise(out);
	NodeImpl::Serialise(out, directive);
	*out << XMLIOStr_Newline;
	delete out;
}

NodeImpl *DocumentImpl::ImportNode(NodeImpl *source, bool deep)
{
	NodeImpl *clone = source->CloneNode(deep);
	clone->SetOwnerOnAll(this);
	clone->fParentNode = 0;
	return clone;
}

NodeImpl *DocumentImpl::CloneNode(bool deep)
{
	return new DocumentImpl(this, deep);
}
