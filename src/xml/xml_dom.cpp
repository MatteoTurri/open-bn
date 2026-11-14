/*
 * Node: the reference-counted handle of every DOM node (navigation, child list
 * editing, value, serialisation), and the Node::PerformCheck switch.
 */

#include "xml.h"
#include "xml_nodelist.h"
#include "xml_nnmap.h"
#include "xml_document.h"
#include "xml_filedescriptor.h"

/* While nonzero the Impl classes validate names and arguments (see DOMUnit::InternalCheck). */
int Node::PerformCheck = 1;

int Node::GetHash() const
{
	return (int)fImpl;
}

Node::Node(NodeImpl *impl)
{
	fImpl = impl;
	if (impl)
		GCObject::AddRef(impl);
}

Node::Node()
{
	fImpl = 0;
}

Node::~Node()
{
	if (fImpl)
		GCObject::RemoveRef(fImpl);
}

void Node::SerialiseToBuffer(XMLString *buffer)
{
	fImpl->SerialiseToBuffer(buffer);
}

void Node::SetNodePtr(void *ptr)
{
	fImpl->fNodePtr = ptr;
}

void *Node::GetNodePtr()
{
	return fImpl->fNodePtr;
}

XMLString Node::GetNodePtrValue()
{
	return fImpl->GetNodePtrValue();
}

Node Node::getFirstChild(XMLString name)
{
	return Node(fImpl->GetFirstChild(&name));
}

Node Node::getLastChild(XMLString name)
{
	return Node(fImpl->GetLastChild(&name));
}

Node Node::getNextNode(XMLString name)
{
	return Node(fImpl->GetNextNode(&name));
}

Node Node::getPrevNode(XMLString name)
{
	return Node(fImpl->GetPrevNode(&name));
}

Node Node::appendEntityValueNode(Node newChild)
{
	return Node(fImpl->InsertBefore(newChild.fImpl, 0, true));
}

Node Node::insertBefore(Node newChild, Node refChild)
{
	return Node(fImpl->InsertBefore(newChild.fImpl, refChild.fImpl, false));
}

Node Node::replaceChild(Node newChild, Node oldChild)
{
	return Node(fImpl->ReplaceChild(newChild.fImpl, oldChild.fImpl));
}

Node Node::removeChild(Node oldChild)
{
	return Node(fImpl->RemoveChild(oldChild.fImpl, false));
}

Node Node::appendChild(Node newChild)
{
	return Node(fImpl->AppendChild(newChild.fImpl));
}

int Node::hasChildNodes()
{
	return fImpl->fFirstChild != 0;
}

Node Node::cloneNode(bool deep)
{
	return Node(fImpl->CloneNode(deep));
}

Node Node::get_parentNode()
{
	if (!fImpl)
		return *new Node();
	return Node(fImpl->fParentNode);
}

NodeList Node::get_childNodes()
{
	return NodeList(fImpl->ChildNodes());
}

NamedNodeMap Node::get_attributes()
{
	return NamedNodeMap(fImpl->fAttributes);
}

Document Node::get_ownerDocument()
{
	return Document(fImpl->fOwnerDocument);
}

Node Node::get_firstChild()
{
	return Node(fImpl->fFirstChild);
}

Node Node::get_lastChild()
{
	return Node(fImpl->fLastChild);
}

Node Node::get_previousSibling()
{
	return Node(fImpl->fPreviousSibling);
}

Node Node::get_nextSibling()
{
	return Node(fImpl->fNextSibling);
}

unsigned short Node::get_nodeType()
{
	return fImpl->fNodeType;
}

XMLString Node::get_nodeName()
{
	return fImpl->fName;
}

XMLString Node::get_nodeValue()
{
	return fImpl->NodeValue();
}

void Node::set_nodeValue(XMLString data)
{
	fImpl->NodeValue(&data);
}

Node &Node::operator=(const Node &other)
{
	if (fImpl != other.fImpl) {
		if (fImpl)
			GCObject::RemoveRef(fImpl);
		fImpl = other.fImpl;
		if (fImpl)
			GCObject::AddRef(fImpl);
	}
	return *this;
}

bool Node::operator==(const Node &other) const
{
	return fImpl == other.fImpl;
}

bool Node::operator!=(const Node &other) const
{
	return fImpl != other.fImpl;
}

Node &Node::operator=(const int *other)
{
	if (other == 0) {
		if (fImpl) {
			GCObject::RemoveRef(fImpl);
			fImpl = 0;
		}
	}
	return *this;
}

bool Node::operator==(const int *other) const
{
	if (other == 0)
		return fImpl == 0;
	return false;
}

bool Node::operator!=(const int *other) const
{
	if (other == 0)
		return fImpl != 0;
	return true;
}

Node::Node(const Node &other)
{
	fImpl = other.fImpl;
	if (fImpl)
		GCObject::AddRef(fImpl);
}

DOMException Node::Status()
{
	return DOMException((DOMException::ExceptionCode)fImpl->Status());
}
