/*
 * EntityScanner: scanner of the entity references &name; and %name;.
 */

#pragma implementation "xml_entityscanner.h"
#include "xml_entityscanner.h"
#include "xml_entitydeclmanager.h"
#include "xml_entitydeclaration.h"
#include "xml_inputstream.h"
#include "xml_streammanager.h"
#include "xml_inputsource.h"
#include "xml_parseerrorprocessor.h"
#include "xml_scanner.h"

int EntityScanner::Resolve(XMLInputStream *stream, XMLStreamManager *mgr, bool parameterEnt)
{
	EntityDeclaration *decl = 0;
	int result = scan(&decl, stream, mgr, parameterEnt, false);

	if (result & 1) {
		if (decl) {
			if (decl->CharacterRef()) {
				XMLString *value = decl->GetValue();
				if (value)
					fDocScanner->characters(value, value->ByteLength(), mgr);
				delete decl;
				return result;
			}

			XMLString *name = decl->GetName();
			fDocScanner->startEntity(name, mgr);

			if (decl->InternalEntity()) {
				XMLString *value = decl->GetValue();
				result = mgr->ApplyInputStream(value, false, true);
				XMLString *name2 = decl->GetName();
				if (fEntityDeclManager->IsBuiltInEntity(name2))
					mgr->GetCurrentStream()->DisableEntityResolving();
			} else {
				fErrorProcessor->TreatErrorState(XPE_EntityNotDeclared, stream);
				return 0;
			}
		}
	}

	XMLInputStream *cur = mgr->GetCurrentStream();
	if (cur) {
		if (decl)
			cur->EntityReference(decl->GetName());
	}
	return result;
}

int EntityScanner::scan(EntityDeclaration **declOut, XMLInputStream *stream, XMLStreamManager *mgr, bool parameterEnt,
    bool nameOnly)
{
	bool charRef = false;

	stream->SenseNextChar(&fChar);
	int code;
	int result;

	if (!parameterEnt && fChar == XMLChar_hash) {
		stream->SkipNextChar();
		stream->SenseNextChar(&fChar);
		charRef = true;
		if (fChar == fHexMarker) {
			stream->SkipNextChar();
			result = stream->GetNextHex(&code);
		} else {
			result = stream->GetNextInteger(&code);
		}
		if ((result & 1) == 0 || code < 0) {
			if (result == 16)
				fErrorProcessor->TreatErrorState(XPE_CharRefBadDigit, stream);
			else
				fErrorProcessor->TreatErrorState(XPE_CharRefInvalid, stream);
			return result;
		}
	} else {
		result = stream->GetName(&fName);
		if ((result & 1) == 0) {
			fErrorProcessor->TreatErrorState(XPE_EntityNameNoSemi, stream);
			return result;
		}
	}

	stream->GetNextChar(&fChar);
	if (fChar != XMLChar_semi) {
		fErrorProcessor->TreatErrorState(XPE_CharRefNoSemi, stream);
		return 0;
	}

	if (charRef) {
		*declOut = fEntityDeclManager->ResolveEntity(code, stream);
		if (!*declOut) {
			fErrorProcessor->TreatErrorState(XPE_CharRefUnresolved, stream);
			return 0;
		}
	} else if (!nameOnly) {
		*declOut = fEntityDeclManager->ResolveEntity(&fName, parameterEnt);
		if (!*declOut && !parameterEnt) {
			fErrorProcessor->TreatErrorState(XPE_EntityUnresolved, stream);
			return 0;
		}
	} else {
		if (!*declOut)
			*declOut = new EntityDeclaration();
		(*declOut)->SetValue(&fName);
	}
	return result;
}

EntityScanner::EntityScanner(XMLDocumentScanner *ds, EntityDeclManager *edm) : TokenScanner(ds), fHexMarker("x", false)
{
	fEntityDeclManager = edm;
}

EntityScanner::~EntityScanner()
{}

int EntityScanner::Scan(XMLInputStream *stream, XMLStreamManager *mgr)
{
	return Resolve(stream, mgr, false);
}

int EntityScanner::ScanAndResolve(EntityDeclaration **decl, XMLInputStream *stream, XMLStreamManager *mgr,
    bool parameterEnt, bool nameOnly)
{
	return scan(decl, stream, mgr, parameterEnt, nameOnly);
}

int EntityScanner::IsBuiltInEntity(XMLString *name)
{
	return fEntityDeclManager->IsBuiltInEntity(name);
}
