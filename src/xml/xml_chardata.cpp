/*
 * CharacterData: the handle shared by text, comment and CDATA nodes; the DOM string
 * operations (get/set, append, insert, delete, replace, substring) on its data.
 */

#include "xml_chardata.h"

CharacterDataImpl *CharacterData::implementation() const
{
	return (CharacterDataImpl *)fImpl;
}

CharacterData::CharacterData(CharacterDataImpl *impl) : Node(impl)
{}

CharacterData::CharacterData()
{}

CharacterData::CharacterData(const Node &other) : Node(other)
{}

CharacterData::~CharacterData()
{}

CharacterData &CharacterData::operator=(const CharacterData &other)
{
	return (CharacterData &)Node::operator=(other);
}

bool CharacterData::operator==(const CharacterData &other) const
{
	return Node::operator==(other);
}

bool CharacterData::operator!=(const CharacterData &other) const
{
	return Node::operator!=(other);
}

CharacterData &CharacterData::operator=(const int *other)
{
	return (CharacterData &)Node::operator=(other);
}

bool CharacterData::operator==(const int *other) const
{
	return Node::operator==(other);
}

bool CharacterData::operator!=(const int *other) const
{
	return Node::operator!=(other);
}

XMLString CharacterData::substringData(unsigned long offset, unsigned long count)
{
	return ((CharacterDataImpl *)fImpl)->SubstringData(offset, count);
}

void CharacterData::appendData(XMLString data)
{
	((CharacterDataImpl *)fImpl)->AppendData(&data);
}

void CharacterData::insertData(unsigned long index, XMLString data)
{
	((CharacterDataImpl *)fImpl)->InsertData(index, &data);
}

void CharacterData::deleteData(unsigned long offset, unsigned long count)
{
	((CharacterDataImpl *)fImpl)->DeleteData(offset, count);
}

void CharacterData::replaceData(unsigned long offset, unsigned long count, XMLString data)
{
	((CharacterDataImpl *)fImpl)->ReplaceData(offset, count, &data);
}

XMLString CharacterData::get_data() const
{
	return ((CharacterDataImpl *)fImpl)->Data();
}

void CharacterData::set_data(XMLString data)
{
	((CharacterDataImpl *)fImpl)->Data(&data);
}

unsigned CharacterData::get_length() const
{
	return ((CharacterDataImpl *)fImpl)->Length();
}
