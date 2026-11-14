/*
 * EntityDeclaration: an entity declaration of the DTD.
 */

#ifndef GUARD_XML_ENTITYDECLARATION_H
#define GUARD_XML_ENTITYDECLARATION_H
#pragma interface
#include "xml_declaration.h"

class EntityDeclaration : public Declaration {
	friend class EntityDeclManager;

public:
	EntityDeclaration();
	EntityDeclaration(XMLString *name, XMLString *value, bool parameterEnt);
	virtual ~EntityDeclaration();
	XMLString *GetValue();
	void SetValue(XMLString *value);
	int GetParameterEnt();
	int InternalEntity();
	int CharacterRef();

protected:
	/* 0x14 */ XMLString *fValue;
	/* 0x18 */ XMLString *unk_18;
	/* 0x1C */ int fParameterEnt;
	/* 0x20 */ int fInternalEntity;
	/* 0x24 */ int fCharRef;
};

#endif // GUARD_XML_ENTITYDECLARATION_H
