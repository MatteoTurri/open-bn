/*
 * Methods of NodeImpl: node fields, the tree of children and the serialisation.
 */

#include "xml.h"
#include "xml_nodelist.h"
#include "xml_nnmap.h"
#include "xml_outputsource.h"
#include "xml_outputdirective.h"
#include "xml_document.h"
#include "xml_filedescriptor.h"

int NodeImpl::isAllowedChild(NodeImpl *child)
{
	return (fAllowedChildren >> (child->fNodeType - 1)) & 1;
}

void NodeImpl::AppendEntityValueNode(NodeImpl *node)
{
	InsertBefore(node, 0, true);
}

int NodeImpl::HasChildNodes()
{
	return fFirstChild != 0;
}

DocumentImpl *NodeImpl::OwnerDocument()
{
	return fOwnerDocument;
}

unsigned short NodeImpl::NodeType()
{
	return fNodeType;
}

NodeImpl *NodeImpl::NextSibling()
{
	return fNextSibling;
}

NodeImpl *NodeImpl::ParentNode()
{
	return fParentNode;
}

NamedNodeMapImpl *NodeImpl::Attributes()
{
	return fAttributes;
}

NodeImpl *NodeImpl::FirstChild()
{
	return fFirstChild;
}

NodeImpl *NodeImpl::LastChild()
{
	return fLastChild;
}

NodeImpl *NodeImpl::PreviousSibling()
{
	return fPreviousSibling;
}

XMLString NodeImpl::NodeName()
{
	return fName;
}

NodeImpl::NodeImpl(DocumentImpl *ownerDoc, XMLString *name, XMLString *value, unsigned short type)
{
	fStatus = 0;
	switch (type) {
	case XML_NODE_ATTRIBUTE:
		/* NOTE: bug: the DOM allows EntityReference children of an Attr, not Entity */
		fAllowedChildren = XML_NODE_BIT(XML_NODE_TEXT) | XML_NODE_BIT(XML_NODE_ENTITY);
		break;
	case XML_NODE_DOCUMENT:
		fAllowedChildren = XML_NODE_BIT(XML_NODE_ELEMENT) | XML_NODE_BIT(XML_NODE_PROCESSING_INSTRUCTION)
		    | XML_NODE_BIT(XML_NODE_COMMENT) | XML_NODE_BIT(XML_NODE_DOCUMENT_TYPE);
		break;
	case XML_NODE_DOCUMENT_TYPE:
		fAllowedChildren = XML_NODE_BIT(XML_NODE_ENTITY) | XML_NODE_BIT(XML_NODE_NOTATION);
		break;
	case XML_NODE_ELEMENT:
	case XML_NODE_ENTITY_REFERENCE:
	case XML_NODE_ENTITY:
	case XML_NODE_DOCUMENT_FRAGMENT:
		fAllowedChildren = XML_NODE_BIT(XML_NODE_ELEMENT) | XML_NODE_BIT(XML_NODE_TEXT)
		    | XML_NODE_BIT(XML_NODE_CDATA_SECTION) | XML_NODE_BIT(XML_NODE_ENTITY_REFERENCE)
		    | XML_NODE_BIT(XML_NODE_PROCESSING_INSTRUCTION) | XML_NODE_BIT(XML_NODE_COMMENT);
		break;
	case XML_NODE_TEXT:
	case XML_NODE_CDATA_SECTION:
	case XML_NODE_PROCESSING_INSTRUCTION:
	case XML_NODE_COMMENT:
	case XML_NODE_NOTATION:
		fAllowedChildren = 0;
		break;
	}
	fParentNode = 0;
	if (name)
		fName.XMLStringCopy(name);
	if (value)
		fValue.XMLStringCopy(value);
	fNodeType = type;
	fFirstChild = 0;
	fLastChild = 0;
	fPreviousSibling = 0;
	fNextSibling = 0;
	fAttributes = 0;
	fOwnerDocument = ownerDoc;
	fOwnerCount = 0;
	fNodePtr = 0;
}

NodeImpl::~NodeImpl()
{
	delete fAttributes;
}

void NodeImpl::conditionalDelete(NodeImpl *node)
{
	if (node && node->fParentNode == 0 && node->fOwnerCount == 0) {
		NodeImpl *n = node->fFirstChild;
		while (n) {
			NodeImpl *next = n->fNextSibling;
			node->RemoveChild(n, 1);
			if (n->fRefCount == 0)
				conditionalDelete(n);
			n = next;
		}
		delete node;
	}
}

NodeImpl *NodeImpl::RemoveChild(NodeImpl *oldChild, bool release)
{
	resetException();
	if (!release) {
		if (readOnly()) {
			treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
			return 0;
		}
	}
	if (oldChild && oldChild->fParentNode != this) {
		if (!fParentNode)
			return 0;
		treatException(DOMException::NOT_FOUND_ERR);
		return 0;
	}
	NodeImpl *parent = oldChild->fParentNode;
	NodeImpl *prev, *next;
	if (!parent) {
		prev = 0;
		next = 0;
	}
	prev = oldChild->fPreviousSibling;
	next = oldChild->fNextSibling;
	if (prev)
		prev->fNextSibling = next;
	else if (parent)
		parent->fFirstChild = next;
	if (next)
		next->fPreviousSibling = prev;
	else if (parent)
		parent->fLastChild = prev;
	oldChild->fParentNode = 0;
	oldChild->fPreviousSibling = 0;
	oldChild->fNextSibling = 0;
	return oldChild;
}

NodeImpl *NodeImpl::doInsertBefore(NodeImpl *newChild, NodeImpl *refChild, bool replace)
{
	if (Node::PerformCheck) {
		if (!isAllowedChild(newChild)) {
			treatException(DOMException::HIERARCHY_REQUEST_ERR);
			return 0;
		}
		if (In(newChild)) {
			treatException(DOMException::HIERARCHY_REQUEST_ERR);
			return 0;
		}
	}
	if (Node::PerformCheck) {
		if (fOwnerDocument && fOwnerDocument != newChild->fOwnerDocument) {
			treatException(DOMException::WRONG_DOCUMENT_ERR);
			return 0;
		}
		if (!replace && readOnly()) {
			treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
			return 0;
		}
	}
	if (refChild && this != refChild->fParentNode) {
		treatException(DOMException::NOT_FOUND_ERR);
		return 0;
	}
	if (fNodeType != XML_NODE_DOCUMENT) {
		NodeImpl *docElem = fOwnerDocument->DocumentElement();
		if (newChild->In(docElem))
			newChild->fParentNode->RemoveChild(newChild, false);
	}
	if (newChild->fNodeType == XML_NODE_DOCUMENT_FRAGMENT)
		return newChild;
	NodeImpl *oldParent = newChild->fParentNode;
	if (oldParent) {
		if (!newChild->fPreviousSibling && !newChild->fNextSibling) {
			oldParent->fFirstChild = 0;
			newChild->fParentNode->fLastChild = 0;
		} else if (oldParent->fFirstChild == newChild) {
			oldParent->fFirstChild = newChild->fNextSibling;
			newChild->fParentNode->fFirstChild->fPreviousSibling = 0;
		} else {
			newChild->fPreviousSibling->fNextSibling = newChild->fNextSibling;
			if (newChild->fNextSibling)
				newChild->fNextSibling->fPreviousSibling = newChild->fPreviousSibling;
		}
	}
	newChild->fParentNode = this;
	newChild->fNextSibling = refChild;
	newChild->fPreviousSibling = refChild ? refChild->fPreviousSibling : fLastChild;
	if (refChild)
		refChild->fPreviousSibling = newChild;
	else
		fLastChild = newChild;
	if (newChild->fPreviousSibling)
		newChild->fPreviousSibling->fNextSibling = newChild;
	if (fFirstChild == refChild)
		fFirstChild = newChild;
	return newChild;
}

NodeImpl *NodeImpl::ReplaceChild(NodeImpl *newChild, NodeImpl *oldChild)
{
	resetException();
	if (newChild->fNodeType == XML_NODE_DOCUMENT_FRAGMENT) {
		NodeImpl *n = newChild->fFirstChild;
		if (n) {
			do {
				doInsertBefore(newChild->RemoveChild(n, false), oldChild, false);
				n = newChild->fFirstChild;
			} while (n);
		}
	} else {
		doInsertBefore(newChild, oldChild, false);
	}
	if (Status())
		return 0;
	return RemoveChild(oldChild, false);
}

/* A document fragment inserts its children one by one (detaching each), not itself. */
NodeImpl *NodeImpl::InsertBefore(NodeImpl *newChild, NodeImpl *refChild, bool replace)
{
	resetException();
	if (newChild->fNodeType == XML_NODE_DOCUMENT_FRAGMENT) {
		NodeImpl *n = newChild->fFirstChild;
		if (n) {
			do {
				doInsertBefore(newChild->RemoveChild(n, false), refChild, replace);
				n = newChild->fFirstChild;
			} while (n);
		}
		return newChild;
	}
	return doInsertBefore(newChild, refChild, replace);
}

/* appendChild(n) is insertBefore(n, null). */
NodeImpl *NodeImpl::AppendChild(NodeImpl *newChild)
{
	resetException();
	if (!newChild)
		return 0;
	return InsertBefore(newChild, 0, false);
}

NodeImpl *NodeImpl::CloneNode(bool deep)
{
	return new NodeImpl(this, deep);
}

NodeListImpl *NodeImpl::ChildNodes()
{
	NodeListImpl *list = new NodeListImpl();
	for (NodeImpl *n = fFirstChild; n; n = n->fNextSibling)
		list->SetNamedItem(n);
	return list;
}

NodeListImpl *NodeImpl::GetElementsByTagName(XMLString *name, NodeListImpl *result)
{
	if (result == 0) {
		result = new NodeListImpl();
	} else {
		if (fName == *name || *name == "*")
			result->SetNamedItem(this);
	}
	for (NodeImpl *n = fFirstChild; n; n = n->fNextSibling)
		if (n->fNodeType == XML_NODE_ELEMENT)
			n->GetElementsByTagName(name, result);
	return result;
}

/*
 * Serialises the children with the directive: elements and text through their own
 * two-argument Serialise (an element named "head" also gets the <meta> charset line),
 * every other node through its one-argument Serialise (a comment on a new line),
 * recording SNS_Other.
 */
void NodeImpl::Serialise(XMLOutputSource *out, OutputDirective *directive)
{
	NodeImpl *n = FirstChild();
	while (n) {
		XMLString unused;
		switch (n->NodeType()) {
		case XML_NODE_ELEMENT: {
			XMLString *lower = n->NodeName().ToLowerCase();
			if (*lower == "head")
				directive->SerialiseHTMLMETA(out);
			delete lower;
		}
			/* fall through */
		case XML_NODE_TEXT:
			n->Serialise(out, directive);
			n = n->NextSibling();
			break;
		default:
			if (n->NodeType() == XML_NODE_COMMENT)
				*out << XMLIOStr_Newline;
			n->Serialise(out);
			directive->SetPrevNode(OutputDirective::SNS_Other);
			n = n->NextSibling();
			break;
		}
	}
}

XMLString NodeImpl::NodeValue()
{
	resetException();
	return fValue;
}

NodeImpl::NodeImpl(const NodeImpl *other, bool deep)
{
	fOwnerCount = 0;
	fParentNode = 0;
	XMLString name = ((NodeImpl *)other)->NodeName();
	XMLString value = ((NodeImpl *)other)->NodeValue();
	fName.Append(&name);
	fValue.Append(&value);
	fNodeType = other->fNodeType;
	fOwnerDocument = other->fOwnerDocument;
	fAllowedChildren = other->fAllowedChildren;
	if (other->fAttributes)
		fAttributes = new NamedNodeMapImpl(other->fAttributes, this);
	else
		fAttributes = 0;
	fPreviousSibling = 0;
	fNextSibling = 0;
	fFirstChild = 0;
	fLastChild = 0;
	if (fAttributes) {
		XMLString id("id", false);
		fNodePtr = fAttributes->GetNamedItem(&id);
	} else {
		fNodePtr = 0;
	}
	if (other->fFirstChild && deep) {
		for (NodeImpl *child = other->fFirstChild; child; child = child->fNextSibling)
			AppendEntityValueNode(child->CloneNode(deep));
	}
}

void NodeImpl::NodeValue(XMLString *value)
{
	resetException();
	if (Node::PerformCheck && readOnly())
		treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
	else
		fValue.XMLStringCopy(value);
}

/* A plain node has no text form of its own: only the children are written. */
void NodeImpl::Serialise(XMLOutputSource *out)
{
	for (NodeImpl *n = fFirstChild; n; n = n->fNextSibling)
		n->Serialise(out);
}

/* Serialises into the given buffer through a temporary XMLOutputSource. */
void NodeImpl::SerialiseToBuffer(XMLString *buffer)
{
	XMLOutputSource *out = new XMLOutputSource(buffer, XMLIOType_Memory, XMLCode_UTF8, 1024);
	Serialise(out);
	delete out;
}

void NodeImpl::firstReference()
{
	if (fOwnerDocument)
		GCObject::AddRef((GCObject *)fOwnerDocument);
}

void NodeImpl::lastReference()
{
	if (fOwnerDocument)
		GCObject::RemoveRef((GCObject *)fOwnerDocument);
	conditionalDelete(this);
}

int NodeImpl::In(NodeImpl *other)
{
	NodeImpl *n = fParentNode;
	while (n) {
		if (n == other)
			return 1;
		n = n->fParentNode;
	}
	return 0;
}

int NodeImpl::readOnly()
{
	NodeImpl *n = this;
	while (n) {
		/* inside an entity reference or an entity */
		if ((unsigned)(n->fNodeType - XML_NODE_ENTITY_REFERENCE) < 2)
			return 1;
		n = n->fParentNode;
	}
	return 0;
}

int NodeImpl::contained()
{
	return fParentNode != 0 || fOwnerCount != 0;
}

void NodeImpl::SetOwnerOnAll(DocumentImpl *doc)
{
	unsigned i, n;

	fOwnerDocument = doc;
	if (fAttributes) {
		n = fAttributes->Length();
		for (i = 0; i < n; i++)
			fAttributes->Item(i)->SetOwnerOnAll(doc);
	}
	NodeListImpl *list = new NodeListImpl();
	NodeImpl *child = fFirstChild;
	while (child) {
		list->SetNamedItem(child);
		child = child->fNextSibling;
	}
	n = list->fLength;
	for (i = 0; i < n; i++)
		list->Item(i)->SetOwnerOnAll(doc);
	delete list;
}

XMLString NodeImpl::GetNodePtrValue()
{
	if (fNodePtr) {
		NodeImpl *p = (NodeImpl *)fNodePtr;
		p->resetException();
		return p->fValue;
	}
	return XMLString();
}

NodeImpl *NodeImpl::GetFirstChild(XMLString *name)
{
	NodeImpl *n = fFirstChild;
	while (n) {
		if (n->fNodeType == XML_NODE_ELEMENT && (name == 0 || *name == XMLString(n->fName)))
			return n;
		n = n->fNextSibling;
	}
	return 0;
}

NodeImpl *NodeImpl::GetLastChild(XMLString *name)
{
	NodeImpl *n = fLastChild;
	while (n) {
		if (n->fNodeType == XML_NODE_ELEMENT && (name == 0 || *name == XMLString(n->fName)))
			return n;
		n = n->fPreviousSibling;
	}
	return 0;
}

NodeImpl *NodeImpl::GetNextNode(XMLString *name)
{
	NodeImpl *n = fNextSibling;
	while (n) {
		if (n->fNodeType == XML_NODE_ELEMENT && (name == 0 || *name == XMLString(n->fName)))
			return n;
		n = n->fNextSibling;
	}
	return 0;
}

NodeImpl *NodeImpl::GetPrevNode(XMLString *name)
{
	NodeImpl *n = fPreviousSibling;
	while (n) {
		if (n->fNodeType == XML_NODE_ELEMENT && (name == 0 || *name == XMLString(n->fName)))
			return n;
		n = n->fPreviousSibling;
	}
	return 0;
}
