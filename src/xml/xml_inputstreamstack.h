/*
 * InputStreamStack: the stack of open input streams (nested entities) of XMLStreamManager.
 */

#ifndef GUARD_XML_INPUTSTREAMSTACK_H
#define GUARD_XML_INPUTSTREAMSTACK_H
#include "xml_mmobject.h"
#include "xml_filedescriptor.h"

class XMLInputStream;

class InputStreamStackElement : public XMLMMObject {
public:
	InputStreamStackElement(XMLInputStream *stream, XMLFileDescriptor fd, InputStreamStackElement *next);
	virtual ~InputStreamStackElement();

	/* 0x04 */ XMLInputStream *fStream;
	/* 0x08 */ XMLFileDescriptor fFileDescriptor;
	/* 0x24 */ InputStreamStackElement *fNext;
};

class InputStreamStack : public XMLMMObject {
public:
	InputStreamStack();
	virtual ~InputStreamStack();
	void Push(XMLInputStream *stream);
	void *Pop();
	void *Top() const;
	int IsLastInputStream() const;
	int IsOpen(XMLFileDescriptor *fd) const;
	int IsEmpty() const;
	int IsEntity();

protected:
	/* 0x04 */ InputStreamStackElement *fTop;
};

#endif // GUARD_XML_INPUTSTREAMSTACK_H
