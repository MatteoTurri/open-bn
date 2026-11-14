/*
 * ProcessingInstruction: the handle of a processing instruction node (target, data).
 */

#include "xml_pi.h"

ProcessingInstruction::~ProcessingInstruction()
{}

ProcessingInstruction::ProcessingInstruction(ProcessingInstructionImpl *impl) : Node(impl)
{}

ProcessingInstruction::ProcessingInstruction()
{}

ProcessingInstruction::ProcessingInstruction(const Node &other) : Node(other)
{}

ProcessingInstruction &ProcessingInstruction::operator=(const ProcessingInstruction &other)
{
	return (ProcessingInstruction &)Node::operator=(other);
}

bool ProcessingInstruction::operator==(const ProcessingInstruction &other) const
{
	return Node::operator==(other);
}

bool ProcessingInstruction::operator!=(const ProcessingInstruction &other) const
{
	return Node::operator!=(other);
}

ProcessingInstruction &ProcessingInstruction::operator=(const int *other)
{
	return (ProcessingInstruction &)Node::operator=(other);
}

bool ProcessingInstruction::operator==(const int *other) const
{
	return Node::operator==(other);
}

bool ProcessingInstruction::operator!=(const int *other) const
{
	return Node::operator!=(other);
}

XMLString ProcessingInstruction::get_target() const
{
	return fImpl->fName;
}

XMLString ProcessingInstruction::get_data() const
{
	return fImpl->NodeValue();
}

void ProcessingInstruction::set_data(XMLString &data)
{
	fImpl->NodeValue(&data);
}
