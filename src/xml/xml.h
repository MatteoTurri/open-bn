/*
 * The DOM core: NodeImpl, the implementation of a node, and Node, the handle that refers to it.
 * Each handle class holds a reference-counted pointer to its Impl.
 */

#ifndef GUARD_XML_H
#define GUARD_XML_H
#include "xml_gc.h"
#include "xml_string.h"
#include "xml_domimplobject.h"

class NodeListImpl;
class NamedNodeMapImpl;
class DocumentImpl;
class NodeList;
class NamedNodeMap;
class Document;
class XMLOutputSource;
class OutputDirective;

/* The DOM node types (the nodeType values of the DOM specification). */
enum XMLNodeType {
	XML_NODE_ELEMENT = 1,
	XML_NODE_ATTRIBUTE = 2,
	XML_NODE_TEXT = 3,
	XML_NODE_CDATA_SECTION = 4,
	XML_NODE_ENTITY_REFERENCE = 5,
	XML_NODE_ENTITY = 6,
	XML_NODE_PROCESSING_INSTRUCTION = 7,
	XML_NODE_COMMENT = 8,
	XML_NODE_DOCUMENT = 9,
	XML_NODE_DOCUMENT_TYPE = 10,
	XML_NODE_DOCUMENT_FRAGMENT = 11,
	XML_NODE_NOTATION = 12
};

/* Bit of a node type in NodeImpl::fAllowedChildren. */
#define XML_NODE_BIT(type) (1 << ((type) - 1))

class NodeImpl : public DOMImplObject {
public:
	/* 0x10 */ NodeImpl *fParentNode;
	/* 0x14 */ XMLString fName;
	/* 0x30 */ XMLString fValue;
	/* 0x4C */ unsigned short fNodeType;
	/* 0x4E */ char fPad78[2];
	/* 0x50 */ NodeImpl *fFirstChild;
	/* 0x54 */ NodeImpl *fLastChild;
	/* 0x58 */ NodeImpl *fPreviousSibling;
	/* 0x5C */ NodeImpl *fNextSibling;
	/* 0x60 */ NamedNodeMapImpl *fAttributes;
	/* 0x64 */ DocumentImpl *fOwnerDocument;
	/* 0x68 */ short fAllowedChildren;  /* mask of the allowed child types */
	/* 0x6A */ char fPad106[2];
	/* 0x6C */ void *fNodePtr;

	NodeImpl();
	NodeImpl(DocumentImpl *ownerDoc, XMLString *name, XMLString *value, unsigned short type);
	NodeImpl(const NodeImpl *other, bool deep);
	virtual ~NodeImpl();

	XMLString NodeValue();
	void NodeValue(XMLString *value);
	NodeListImpl *ChildNodes();
	NodeImpl *ParentNode();
	NodeImpl *FirstChild();
	NodeImpl *LastChild();
	NodeImpl *PreviousSibling();
	NodeImpl *NextSibling();
	NamedNodeMapImpl *Attributes();
	DocumentImpl *OwnerDocument();
	unsigned short NodeType();
	int HasChildNodes();
	int contained();
	XMLString NodeName();
	int readOnly();
	int In(NodeImpl *other);
	int isAllowedChild(NodeImpl *child);
	XMLString GetNodePtrValue();
	NodeListImpl *GetElementsByTagName(XMLString *name, NodeListImpl *result);
	NodeImpl *AppendChild(NodeImpl *newChild);
	NodeImpl *RemoveChild(NodeImpl *oldChild, bool release);
	NodeImpl *doInsertBefore(NodeImpl *newChild, NodeImpl *refChild, bool replace);
	NodeImpl *InsertBefore(NodeImpl *newChild, NodeImpl *refChild, bool replace);
	NodeImpl *ReplaceChild(NodeImpl *newChild, NodeImpl *oldChild);
	NodeImpl *GetFirstChild(XMLString *name);
	NodeImpl *GetLastChild(XMLString *name);
	NodeImpl *GetNextNode(XMLString *name);
	NodeImpl *GetPrevNode(XMLString *name);
	void AppendEntityValueNode(NodeImpl *node);
	void SetOwnerOnAll(DocumentImpl *doc);
	static void conditionalDelete(NodeImpl *node);
	virtual void firstReference();
	virtual void lastReference();
	virtual void Serialise(XMLOutputSource *out);
	virtual void Serialise(XMLOutputSource *out, OutputDirective *directive);
	virtual void SerialiseToBuffer(XMLString *buffer);
	virtual NodeImpl *CloneNode(bool deep);
};

/* Base DOM handle: the NodeImpl pointer at 0x00, the vtable pointer at 0x04. */
class Node {
public:
	Node();
	Node(NodeImpl *impl);
	Node(const Node &other);
	virtual ~Node();

	static int PerformCheck;

	int GetHash() const;

	Node &operator=(const Node &other);
	Node &operator=(const int *other);
	bool operator==(const Node &other) const;
	bool operator!=(const Node &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;

	XMLString get_nodeName();
	XMLString get_nodeValue();
	unsigned short get_nodeType();
	Node get_firstChild();
	Node get_lastChild();
	Node get_previousSibling();
	Node get_nextSibling();
	NodeList get_childNodes();
	NamedNodeMap get_attributes();
	Document get_ownerDocument();
	int hasChildNodes();
	void *GetNodePtr();
	void SetNodePtr(void *ptr);
	XMLString GetNodePtrValue();
	DOMException Status();
	void set_nodeValue(XMLString data);

	Node insertBefore(Node newChild, Node refChild);
	Node replaceChild(Node newChild, Node oldChild);
	Node removeChild(Node oldChild);
	Node appendChild(Node newChild);
	Node appendEntityValueNode(Node newChild);
	Node cloneNode(bool deep);
	Node get_parentNode();
	Node getFirstChild(XMLString name);
	Node getLastChild(XMLString name);
	Node getNextNode(XMLString name);
	Node getPrevNode(XMLString name);
	void SerialiseToBuffer(XMLString *buffer);

	friend class NamedNodeMap;
	friend class Document;
	/* hashing functor of Dictionary (dictionary.h): fImpl is the key */
	friend struct foGetIntReprNode;

protected:
	NodeImpl *fImpl;
};

#endif // GUARD_XML_H
