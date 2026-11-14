/*
 * Constructor and destructor of InputStreamStackElement.
 */

#include "xml_inputstreamstack.h"
#include "xml_inputsource.h"
#include "xml_locator.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"

InputStreamStackElement::InputStreamStackElement(XMLInputStream *stream, XMLFileDescriptor fd,
    InputStreamStackElement *next)
{
	fStream = stream;
	fFileDescriptor = fd;
	fFileDescriptor.fBuffer = fd.fBuffer->Clone();
	fNext = next;
}

InputStreamStackElement::~InputStreamStackElement()
{
	delete fFileDescriptor.fBuffer;
}
