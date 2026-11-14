/*
 * CharacterData node: the implementation class and the DOM handle.
 */

#ifndef GUARD_XML_CHARDATA_H
#define GUARD_XML_CHARDATA_H
#include "xml.h"

class CharacterDataImpl : public NodeImpl {
public:
	CharacterDataImpl(DocumentImpl *ownerDoc, XMLString *name, XMLString *value, unsigned short type);
	CharacterDataImpl(CharacterDataImpl *other, bool deep);
	virtual ~CharacterDataImpl();
	virtual NodeImpl *CloneNode(bool deep);

	XMLString Data();
	void Data(XMLString *value);
	unsigned Length();
	void AppendData(XMLString *data);
	void InsertData(unsigned long index, XMLString *data);
	void DeleteData(unsigned long offset, unsigned long count);
	void ReplaceData(unsigned long offset, unsigned long count, XMLString *data);
	XMLString SubstringData(unsigned long offset, unsigned long count);
};

class CharacterData : public Node {
public:
	CharacterData();
	CharacterData(CharacterDataImpl *impl);
	CharacterData(const Node &other);
	virtual ~CharacterData();

	CharacterData &operator=(const CharacterData &other);
	CharacterData &operator=(const int *other);
	bool operator==(const CharacterData &other) const;
	bool operator!=(const CharacterData &other) const;
	bool operator==(const int *other) const;
	bool operator!=(const int *other) const;

	CharacterDataImpl *implementation() const;
	XMLString get_data() const;
	unsigned get_length() const;
	void set_data(XMLString data);
	void appendData(XMLString data);
	void insertData(unsigned long index, XMLString data);
	void deleteData(unsigned long offset, unsigned long count);
	void replaceData(unsigned long offset, unsigned long count, XMLString data);
	XMLString substringData(unsigned long offset, unsigned long count);
};

#endif // GUARD_XML_CHARDATA_H
