/*
 * Methods of TagNameStack: an intrusive stack of NameElement, linked through
 * NameElement::fNext.
 */

#include "xml_locator.h"
#include "xml_filedescriptor.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#include "xml_validator.h"

TagNameStack::TagNameStack()
{
	fHead = 0;
}

TagNameStack::~TagNameStack()
{
	while (fHead) {
		NameElement *node = Pop();
		delete node;
	}
}

void TagNameStack::Push(NameElement *e)
{
	e->fNext = fHead;
	fHead = e;
}

void TagNameStack::Push(XMLString *name)
{
	NameElement *e = new NameElement(name);
	e->fNext = fHead;
	fHead = e;
}

NameElement *TagNameStack::Pop()
{
	NameElement *e = fHead;
	if (e)
		fHead = e->fNext;
	return e;
}
