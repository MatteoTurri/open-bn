/*
 * Methods of CharacterDataImpl, the node behind the CharacterData handle.
 */

#include "xml_chardata.h"

CharacterDataImpl::CharacterDataImpl(DocumentImpl *ownerDoc, XMLString *name, XMLString *value, unsigned short type)
        : NodeImpl(ownerDoc, name, value, type)
{}

CharacterDataImpl::CharacterDataImpl(CharacterDataImpl *other, bool deep) : NodeImpl(other, deep)
{}

CharacterDataImpl::~CharacterDataImpl()
{}

NodeImpl *CharacterDataImpl::CloneNode(bool deep)
{
	return new CharacterDataImpl(this, deep);
}

XMLString CharacterDataImpl::Data()
{
	resetException();
	return fValue;
}

void CharacterDataImpl::Data(XMLString *value)
{
	resetException();
	if (readOnly())
		treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
	else
		fValue.Set(value);
}

unsigned CharacterDataImpl::Length()
{
	return fValue.CharLength();
}

XMLString CharacterDataImpl::SubstringData(unsigned long offset, unsigned long count)
{
	resetException();
	unsigned len = fValue.ByteLength();
	if (len != 0 && len < offset) {
		treatException(DOMException::INDEX_SIZE_ERR);
		return XMLString();
	}
	return fValue.SubstringData(offset, count);
}

void CharacterDataImpl::AppendData(XMLString *data)
{
	resetException();
	if (readOnly())
		treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
	else
		fValue.Append(data);
}

void CharacterDataImpl::InsertData(unsigned long index, XMLString *data)
{
	resetException();
	unsigned len = fValue.ByteLength();
	if (len != 0 && len < index)
		treatException(DOMException::INDEX_SIZE_ERR);
	else if (readOnly())
		treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
	else
		fValue.InsertData(index, data);
}

void CharacterDataImpl::DeleteData(unsigned long offset, unsigned long count)
{
	resetException();
	unsigned len = fValue.ByteLength();
	if (len != 0 && len < offset)
		treatException(DOMException::INDEX_SIZE_ERR);
	else if (readOnly())
		treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
	else
		fValue.DeleteData(offset, count);
}

void CharacterDataImpl::ReplaceData(unsigned long offset, unsigned long count, XMLString *data)
{
	resetException();
	unsigned len = fValue.ByteLength();
	if (len != 0 && len < offset)
		treatException(DOMException::INDEX_SIZE_ERR);
	else if (readOnly())
		treatException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
	else
		fValue.ReplaceData(offset, count, data);
}
