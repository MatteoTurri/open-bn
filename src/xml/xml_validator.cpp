/*
 * Validator: the open element stack of a scan; checks that each end tag closes the
 * element on top and that the input may end.
 */

#include "xml_locator.h"
#include "xml_filedescriptor.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#include "xml_parseerrorprocessor.h"
#include "xml_validator.h"
#include "xml_entitydeclmanager.h"

int Validator::EofAcceptable(bool b)
{
	if (fStack.fHead == 0)
		return 1;
	if (fStack.fHead->fFlag == 0)
		return 0;
	if (b) {
		NameElement *e = fStack.Pop();
		delete e;
	}
	return 0;
}

Validator::Validator(ParseErrorProcessor *pep, AttListDeclManager *adm, EntityDeclManager *edm)
{
	fProcessor = pep;
}

void Validator::RemoveMark(XMLString *name)
{
	if (fStack.fHead) {
		NameElement *e = fStack.Pop();
		if (e) {
			if (e->fFlag == 0 || *e->fName != *name)
				fStack.Push(e);
			else
				delete e;
		}
	}
}

XMLToken Validator::ValidateTag(XMLToken token, XMLString *name)
{
	XMLToken result = XMLToken_None;
	if (token != XMLToken_ETag) {
		if (token == XMLToken_STag) {
			NameElement *e = new NameElement(name);
			fStack.Push(e);
		}
	} else {
		if (fStack.fHead) {
			NameElement *e = fStack.Pop();
			if (e && *e->fName == *name) {
				delete e;
			} else {
				result = (XMLToken)XPE_ETagNotAllowed;
				fStack.Push(e);
			}
		}
	}
	return result;
}

int Validator::PcdataAcceptable()
{
	return 0;
}

Validator::~Validator()
{
	while (fStack.fHead) {
		NameElement *e = fStack.Pop();
		delete e;
	}
}

void Validator::Mark(Locator *loc)
{
	NameElement *e = new NameElement();
	e->fFlag = 1;
	e->fName = loc->getSystemId()->Clone();
	fStack.Push(e);
}
