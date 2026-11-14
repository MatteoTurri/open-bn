/*
 * ProcessingInstruction node: the implementation class and the DOM handle.
 */

#ifndef GUARD_XML_PI_H
#define GUARD_XML_PI_H
#include "xml.h"

class ProcessingInstructionImpl : public NodeImpl {
public:
	ProcessingInstructionImpl(DocumentImpl *ownerDoc, XMLString *target, XMLString *data);
	ProcessingInstructionImpl(ProcessingInstructionImpl *other, bool deep);
	virtual ~ProcessingInstructionImpl();
	virtual NodeImpl *CloneNode(bool deep);
	virtual void Serialise(XMLOutputSource *out);
};

class ProcessingInstruction : public Node {
public:
	ProcessingInstruction();
	ProcessingInstruction(ProcessingInstructionImpl *impl);
	ProcessingInstruction(const Node &other);
	virtual ~ProcessingInstruction();

	ProcessingInstruction &operator=(const ProcessingInstruction &other);
	ProcessingInstruction &operator=(const int *other);
	bool operator==(const ProcessingInstruction &other) const;
	bool operator!=(const ProcessingInstruction &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;

	XMLString get_target() const;
	XMLString get_data() const;
	void set_data(XMLString &data);
};

#endif // GUARD_XML_PI_H
