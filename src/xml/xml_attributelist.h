/*
 * AttributeList: the SAX attribute list interface (pure virtual).
 */

#ifndef GUARD_XML_ATTRIBUTELIST_H
#define GUARD_XML_ATTRIBUTELIST_H

class XMLString;

class AttributeList {
public:
	virtual ~AttributeList() {}
	virtual unsigned getLength() = 0;
	virtual XMLString *getName(unsigned index) = 0;
	virtual XMLString *getType(unsigned index) = 0;
	virtual XMLString *getType(XMLString *name) = 0;
	virtual XMLString *getValue(unsigned index) = 0;
	virtual XMLString *getValue(XMLString *name) = 0;
};

#endif // GUARD_XML_ATTRIBUTELIST_H
