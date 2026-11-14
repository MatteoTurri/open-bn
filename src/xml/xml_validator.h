/*
 * Validator: checks the nesting of the tags with the stack of the open tag names
 * (TagNameStack of NameElement).
 */

#ifndef GUARD_XML_VALIDATOR_H
#define GUARD_XML_VALIDATOR_H
#include "xml_mmobject.h"
#include "xml_string.h"
#include "xml_scannerstate.h"

class Locator;
class ParseErrorProcessor;
class AttListDeclManager;
class EntityDeclManager;

/* Node of the tag stack; fNext is the link used by TagNameStack. */
class NameElement : public XMLMMObject {
public:
	NameElement();
	NameElement(XMLString *name);
	virtual ~NameElement();
	/* 0x04 */ NameElement *fNext;
	/* 0x08 */ XMLString *fName;
	/* 0x0C */ int fFlag;
};

/* Intrusive stack of NameElement. */
class TagNameStack : public XMLMMObject {
public:
	TagNameStack();
	~TagNameStack();
	void Push(NameElement *e);
	void Push(XMLString *name);
	NameElement *Pop();
	/* 0x04 */ NameElement *fHead;
};

#include "xml_errorhandler.h"
#include "xml_attributelist.h"

class Validator : public XMLMMObject {
public:
	Validator(ParseErrorProcessor *pep, AttListDeclManager *adm, EntityDeclManager *edm);
	virtual ~Validator();
	virtual XMLToken ValidateTag(XMLToken token, XMLString *name);
	int EofAcceptable(bool b);
	virtual int PcdataAcceptable();
	void Mark(Locator *loc);
	void RemoveMark(XMLString *name);

protected:
	/* 0x04 */ ParseErrorProcessor *fProcessor;
	/* 0x08 */ TagNameStack fStack;
	/* 0x10 */ XMLString fString;
	/* 0x2C */ TagNameStack fStack2;
};

#endif // GUARD_XML_VALIDATOR_H
