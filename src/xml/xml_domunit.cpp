/*
 * DOMUnit: the SAX document handler that builds a Document from the parser events
 * and keeps the last error reported.
 */

#pragma implementation "xml_domunit.h"
#include "xml_domunit.h"
#include "xml_attributelistimpl.h"

void DOMUnit::characters(XMLString *chars, unsigned length)
{
	if (chars->IsWhiteSpace())
		return;
	if (fInCDATA) {
		CDATASection tmpCDATA(fDocument.createCDATASection(XMLString(chars)));
		if (fEntityValue) {
			fCurrentNode.appendEntityValueNode(Node(tmpCDATA));
		} else {
			fCurrentNode.appendChild(Node(tmpCDATA));
		}
		/* NOTE: bug: the CDATA section is appended a second time */
		fCurrentNode.appendChild(Node(tmpCDATA));
	} else {
		Text tmpText(fDocument.createTextNode(XMLString(chars)));
		if (fEntityValue) {
			fCurrentNode.appendEntityValueNode(Node(tmpText));
		} else {
			fCurrentNode.appendChild(Node(tmpText));
		}
	}
}

void DOMUnit::startElement(XMLString *name, AttributeList &attrs)
{
	Element element;
	element = fDocument.createElement(XMLString(name));

	int length = attrs.getLength();
	for (int i = 0; i < length; i++) {
		Attr tmpAttr = fDocument.createAttribute(XMLString(attrs.getName(i)));
		tmpAttr.set_value(XMLString(attrs.getValue(i)));
		bool specified = ((AttributeListImpl &)attrs).getSpecified(i);
		element.setAttributeNode(tmpAttr, specified);
	}

	if (fEntityValue) {
		fCurrentNode.appendEntityValueNode(Node(element));
	} else {
		fCurrentNode.appendChild(Node(element));
	}
	fCurrentNode = element;
}

DOMUnit::DOMUnit()
{
	fDocument = Document::createDocument();
	fCurrentNode = fDocument;
	fInCDATA = 0;
	fEntityValue = 0;
	fPreserveWhiteSpace = 1;
}

SAXParseException DOMUnit::Status()
{
	return fStatus;
}

void DOMUnit::comment(XMLString *chars, unsigned long start, unsigned long length)
{
	Comment tmpComment(fDocument.createComment(XMLString(*chars)));
	if (fEntityValue) {
		fCurrentNode.appendEntityValueNode(Node(tmpComment));
	} else if (fCurrentNode.get_nodeType() != 10) {
		fCurrentNode.appendChild(Node(tmpComment));
	}
}

void DOMUnit::startCDATA()
{
	fInCDATA = 1;
}

void DOMUnit::endCDATA()
{
	fInCDATA = 0;
}

void DOMUnit::endDocument()
{}

void DOMUnit::endElement(XMLString *name)
{
	if (fCurrentNode.get_nodeType() == 1) {
		Element(fCurrentNode).normalize();
	}
	fCurrentNode = fCurrentNode.get_parentNode();
}

void DOMUnit::ignorableWhitespace(XMLString *chars, unsigned length)
{
	Text tmpText(fDocument.createTextNode(XMLString(chars)));
	if (fEntityValue) {
		fCurrentNode.appendEntityValueNode(Node(tmpText));
	} else {
		fCurrentNode.appendChild(Node(tmpText));
	}
}

void DOMUnit::processingInstruction(XMLString *target, XMLString *data)
{
	ProcessingInstruction tmpPI(fDocument.createProcessingInstruction(XMLString(*target), XMLString(*data)));
	if (fEntityValue) {
		fCurrentNode.appendEntityValueNode(Node(tmpPI));
	} else if (fCurrentNode.get_nodeType() != 10) {
		fCurrentNode.appendChild(Node(tmpPI));
	}
}

void DOMUnit::startDocument()
{}

void DOMUnit::fatalError(SAXParseException &exc)
{
	fStatus = exc;
}

void DOMUnit::error(SAXParseException &exc)
{
	fStatus = exc;
}

Document DOMUnit::getDocument()
{
	return fDocument;
}

void DOMUnit::Reset()
{
	fDocument = 0;
	fDocument = Document::createDocument();
	fCurrentNode = fDocument;
	fInCDATA = 0;
	fEntityValue = 0;
}

void DOMUnit::PreserveWhiteSpace(bool b)
{
	fPreserveWhiteSpace = b;
}

void DOMUnit::InternalCheck(bool b)
{
	Node::PerformCheck = b;
}

DOMUnit::~DOMUnit()
{}
