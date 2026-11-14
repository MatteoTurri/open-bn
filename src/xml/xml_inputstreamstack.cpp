/*
 * InputStreamStack: the stack of open input streams (document and entities), each
 * with a copy of its file descriptor; IsOpen tells whether a source is already open.
 */

#include "xml_inputstreamstack.h"
#include "xml_locator.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#include "xml_inputsource.h"

InputStreamStack::InputStreamStack()
{
	fTop = 0;
}

InputStreamStack::~InputStreamStack()
{}

void InputStreamStack::Push(XMLInputStream *stream)
{
	/* The source of a stream is an XMLInputSource: its descriptor is read directly. */
	XMLInputSource *src = (XMLInputSource *)stream->GetSource();
	XMLFileDescriptor *fd = src->fXMLFileDescriptor;
	InputStreamStackElement *node = new InputStreamStackElement(stream, *fd, fTop);
	fTop = node;
}

void *InputStreamStack::Pop()
{
	if (fTop) {
		InputStreamStackElement *node = fTop;
		void *result = node->fStream;
		fTop = node->fNext;
		delete node;
		return result;
	}
	return 0;
}

void *InputStreamStack::Top() const
{
	void *result = 0;
	if (fTop)
		result = fTop->fStream;
	return result;
}

int InputStreamStack::IsEmpty() const
{
	return fTop == 0;
}
int InputStreamStack::IsLastInputStream() const
{
	int result = 0;
	if (fTop)
		result = fTop->fNext == 0;
	return result;
}

int InputStreamStack::IsEntity()
{
	return fTop->fFileDescriptor.fIsEntity;
}

int InputStreamStack::IsOpen(XMLFileDescriptor *fd) const
{
	for (InputStreamStackElement *node = fTop; node; node = node->fNext) {
		if (*fd->fBuffer == *node->fFileDescriptor.fBuffer && fd->fIOType == node->fFileDescriptor.fIOType)
			return 1;
	}
	return 0;
}
