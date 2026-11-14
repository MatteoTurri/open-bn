/*
 * EntityDeclaration: an entity declaration (name and replacement text); the text of a
 * parameter entity is stored padded with one space on each side.
 */

#include "xml_filedescriptor.h"
#include "xml_locator.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#pragma implementation "xml_entitydeclaration.h"
#include "xml_entitydeclaration.h"
#include "xml_string.h"

EntityDeclaration::EntityDeclaration()
{
	fInternalEntity = 1;
	fCharRef = 0;
	unk_18 = 0;
	fValue = 0;
}

EntityDeclaration::EntityDeclaration(XMLString *name, XMLString *value, bool parameterEnt) : Declaration(name, 0, 0)
{
	fInternalEntity = 1;
	fCharRef = 0;
	unk_18 = 0;
	fValue = 0;
	if (value) {
		if (parameterEnt) {
			fValue = new XMLString();
			fValue->Append((XMLChar)' ');
			fValue->Append(value);
			fValue->Append((XMLChar)' ');
		} else {
			fValue = value->Clone();
		}
	}
	fParameterEnt = parameterEnt;
	fNext = 0;
}

EntityDeclaration::~EntityDeclaration()
{
	if (fValue) {
		delete fValue;
		fValue = 0;
	}
	if (unk_18) {
		delete unk_18;
		unk_18 = 0;
	}
}

XMLString *EntityDeclaration::GetValue()
{
	return fValue;
}

void EntityDeclaration::SetValue(XMLString *value)
{
	if (fValue)
		fValue->Set(value);
	else
		fValue = value->Clone();
}
int EntityDeclaration::GetParameterEnt()
{
	return fParameterEnt;
}
int EntityDeclaration::InternalEntity()
{
	return fInternalEntity;
}
int EntityDeclaration::CharacterRef()
{
	return fCharRef;
}
