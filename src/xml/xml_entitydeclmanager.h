/*
 * EntityDeclManager: hash table (256 buckets) of the entity declarations, with the five
 * predefined entities of XML (lt, gt, amp, apos, quot).
 */

#ifndef GUARD_XML_ENTITYDECLMANAGER_H
#define GUARD_XML_ENTITYDECLMANAGER_H
#pragma interface
#include "xml_declmanager.h"

class XMLString;
class EntityDeclaration;
class XMLInputStream;

/* Name and value of a predefined entity, registered by the constructor. */
struct EntityDeclManagerBuiltinDef {
	XMLString *name;
	XMLString *value;
};

class EntityDeclManager : public DeclManager {
public:
	EntityDeclManager();
	virtual ~EntityDeclManager();
	EntityDeclaration *ResolveEntity(XMLString *name, bool parameterEnt) const;
	EntityDeclaration *ResolveEntity(int code, XMLInputStream *stream) const;
	void AddInternalDeclaration(XMLString *name, XMLString *value, bool parameterEnt);
	void push(EntityDeclaration *decl);
	int IsBuiltInEntity(XMLString *name);
	unsigned char hash(XMLString *name) const;

protected:
	/* 0x04 */ EntityDeclaration *fBuckets[256];
	/* 0x404 */ EntityDeclManagerBuiltinDef fBuiltinPairs[5];
	/* 0x42C */ XMLString *fBuiltin[5];  /* names, for IsBuiltInEntity */
	/* replacement texts of the predefined entities */
	/* 0x440 */ XMLString *fBuiltinRefs[8];
};

#endif // GUARD_XML_ENTITYDECLMANAGER_H
