/*
 * EntityDeclManager: the entity declarations of a document in a 256-bucket hash table,
 * preloaded with the five predefined entities; resolution of names and character references.
 */

#include "xml_locator.h"
#include "xml_filedescriptor.h"
#include "xml_scannerstate.h"
#include "xml_inputstream.h"
#pragma implementation "xml_entitydeclmanager.h"
#pragma implementation "xml_declmanager.h"
#include "xml_entitydeclmanager.h"
#include "xml_entitydeclaration.h"
#include "xml_string.h"

EntityDeclManager::EntityDeclManager()
{
	/* The five predefined entities of XML 1.0 (section 4.6), in the order of the specification. */
	const char *ltLit = "lt";
	const char *gtLit = "gt";
	const char *ampLit = "amp";
	const char *aposLit = "apos";
	const char *quotLit = "quot";

	for (int i = 255; i >= 0; i--)
		fBuckets[i] = 0;

	fBuiltinPairs[0].name = new XMLString(ltLit, false);
	fBuiltinPairs[1].name = new XMLString(gtLit, false);
	fBuiltinPairs[2].name = new XMLString(ampLit, false);
	fBuiltinPairs[3].name = new XMLString(aposLit, false);
	fBuiltinPairs[4].name = new XMLString(quotLit, false);
	fBuiltinPairs[0].value = new XMLString("<", false);
	fBuiltinPairs[1].value = new XMLString(">", false);
	fBuiltinPairs[2].value = new XMLString("&", false);
	fBuiltinPairs[3].value = new XMLString("'", false);
	fBuiltinPairs[4].value = new XMLString("\"", false);

	{
		EntityDeclManagerBuiltinDef *p = fBuiltinPairs;
		int count = 4;
		do {
			AddInternalDeclaration(p->name, p->value, false);
			p++;
		} while (--count >= 0);
	}

	fBuiltin[0] = new XMLString(ltLit, false);
	fBuiltin[1] = new XMLString(gtLit, false);
	fBuiltin[2] = new XMLString(ampLit, false);
	fBuiltin[3] = new XMLString(aposLit, false);
	fBuiltin[4] = new XMLString(quotLit, false);

	/*
	 * Replacement texts of the declarations in the specification (<!ENTITY lt "&#38;#60;">...),
	 * plus the doubly escaped forms of gt, apos and quot.
	 */
	fBuiltinRefs[0] = new XMLString("&#38;#60;", false);
	fBuiltinRefs[1] = new XMLString("&#62;", false);
	fBuiltinRefs[2] = new XMLString("&#38;#38;", false);
	fBuiltinRefs[3] = new XMLString("&#39;", false);
	fBuiltinRefs[4] = new XMLString("&#34;", false);
	fBuiltinRefs[5] = new XMLString("&#38;#62;", false);
	fBuiltinRefs[6] = new XMLString("&#38;#39;", false);
	fBuiltinRefs[7] = new XMLString("&#38;#34;", false);
}

EntityDeclManager::~EntityDeclManager()
{
	{
		XMLString **q = &fBuiltinPairs[0].value;
		int count = 4;
		do {
			delete *(q - 1);
			delete *q;
			q += 2;
		} while (--count >= 0);
	}
	{
		EntityDeclaration **b = &fBuckets[0];
		int count = 255;
		do {
			delete *b;
			b++;
		} while (--count >= 0);
	}
	delete fBuiltin[0];
	delete fBuiltin[1];
	delete fBuiltin[2];
	delete fBuiltin[3];
	delete fBuiltin[4];
	delete fBuiltinRefs[0];
	delete fBuiltinRefs[1];
	delete fBuiltinRefs[2];
	delete fBuiltinRefs[3];
	delete fBuiltinRefs[4];
	delete fBuiltinRefs[5];
	delete fBuiltinRefs[6];
	delete fBuiltinRefs[7];
}

EntityDeclaration *EntityDeclManager::ResolveEntity(XMLString *name, bool parameterEnt) const
{
	EntityDeclaration *decl = fBuckets[hash(name)];
	while (decl) {
		if (*decl->GetName() == *name && decl->GetParameterEnt() == parameterEnt)
			return decl;
		decl = (EntityDeclaration *)decl->Next();
	}
	return 0;
}

EntityDeclaration *EntityDeclManager::ResolveEntity(int code, XMLInputStream *stream) const
{
	if (code < 0x9 || code == 0xB || code == 0xC || (code >= 0xE && code <= 0x1F)
	    || (code >= 0xD800 && code <= 0xDFFF) || (code >= 0xFFFE && code <= 0xFFFF) || code > 0x10FFFF)
		return 0;

	XMLString charStr;
	stream->GetXMLReferenceChar(&charStr, code);
	EntityDeclaration *decl = new EntityDeclaration(&charStr, &charStr, false);
	decl->fCharRef = 1;
	return decl;
}

void EntityDeclManager::AddInternalDeclaration(XMLString *name, XMLString *value, bool parameterEnt)
{
	EntityDeclaration *decl = new EntityDeclaration(name, value, parameterEnt);
	push(decl);
}

void EntityDeclManager::push(EntityDeclaration *decl)
{
	unsigned char h = hash(decl->GetName());
	if (fBuckets[h])
		decl->fNext = fBuckets[h];
	fBuckets[h] = decl;
}

int EntityDeclManager::IsBuiltInEntity(XMLString *name)
{
	if (*name == *fBuiltin[0] || *name == *fBuiltin[1] || *name == *fBuiltin[2] || *name == *fBuiltin[3]
	    || *name == *fBuiltin[4])
		return 1;
	return 0;
}

unsigned char EntityDeclManager::hash(XMLString *name) const
{
	unsigned h = 0;
	int len = name->ByteLength();
	const unsigned char *p = name->GetBytes();
	if (len > 0) {
		int i = len;
		do {
			h = h * 31 + *p++;
		} while (--i);
	}
	return (unsigned char)h;
}
