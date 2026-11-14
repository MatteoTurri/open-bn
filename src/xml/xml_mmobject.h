/*
 * XMLMMObject: polymorphic root of the XML toolkit classes (only a virtual destructor).
 */

#ifndef GUARD_XML_MMOBJECT_H
#define GUARD_XML_MMOBJECT_H
#pragma interface

class XMLMMObject {
public:
	XMLMMObject();
	virtual ~XMLMMObject();
};

#endif // GUARD_XML_MMOBJECT_H
