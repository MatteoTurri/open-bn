/*
 * Methods of ProcessingInstructionImpl (node type 7): the target is the node name,
 * the data the node value.
 */

#include "xml_pi.h"
#include "xml_outputsource.h"

ProcessingInstructionImpl::ProcessingInstructionImpl(DocumentImpl *ownerDoc, XMLString *target, XMLString *data)
        : NodeImpl(ownerDoc, target, data, XML_NODE_PROCESSING_INSTRUCTION)
{}

ProcessingInstructionImpl::ProcessingInstructionImpl(ProcessingInstructionImpl *other, bool deep)
        : NodeImpl(other, deep)
{}

ProcessingInstructionImpl::~ProcessingInstructionImpl()
{}

NodeImpl *ProcessingInstructionImpl::CloneNode(bool deep)
{
	return new ProcessingInstructionImpl(this, deep);
}

void ProcessingInstructionImpl::Serialise(XMLOutputSource *out)
{
	*out << "<?" << &fName;
	if (fValue.ByteLength() != 0)
		*out << " " << &fValue;
	*out << "?>";
}
