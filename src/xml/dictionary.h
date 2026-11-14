/*
 * Dictionary: hash table keyed by the value itself (GetKey), with an extra word per entry.
 * The slot of a key is (key * 7) % capacity, then the next free one (wrapping around);
 * the table doubles when it gets half full. Used with Node values by XmlTagTable.
 */

#ifndef GUARD_DICTIONARY_H
#define GUARD_DICTIONARY_H

#include "xml.h"
#include <stdio.h>

template <class T, class Key, class GetKey> class Dictionary {
public:
	struct Entry {
		int flag;  /* 0 = free, 1 = used */
		unsigned extra;
		T value;
		Entry() : flag(0) {}
	};

	/* 0x00 */ int limit;  /* maximum count, none if <= 0 */
	/* 0x04 */ int capacity;
	/* 0x08 */ Entry *table;
	/* 0x0C */ int count;

	Dictionary(int capacity, int limit);
	~Dictionary();

	int FindAvailableSlot(int start) const;
	void Grow();
	int Regist(const T &value, const Key &extra);
	int Get(const T &value, Key *extra) const;
	void Remove(const T &value);
	void Dump() const;
};

template <class T, class Key, class GetKey> Dictionary<T, Key, GetKey>::Dictionary(int cap, int lim)
{
	limit = lim;
	capacity = cap;
	table = 0;
	count = 0;
	table = new Entry[cap];
}

template <class T, class Key, class GetKey> Dictionary<T, Key, GetKey>::~Dictionary()
{
	if (table)
		delete[] table;
}

template <class T, class Key, class GetKey> int Dictionary<T, Key, GetKey>::FindAvailableSlot(int start) const
{
	int i = start;
	if (i < capacity) {
		for (; i < capacity; i++)
			if (table[i].flag == 0)
				return i;
	}
	for (int j = 0; j < start; j++)
		if (table[j].flag == 0)
			return j;
	return 0; /* NOTE: bug: returns 0, a valid slot, when no slot is free */
}

/* Hashing functor: the key of a Node is its NodeImpl pointer. */
struct foGetIntReprNode {
	int operator()(const Node &node) const { return (int)node.fImpl; }
};

template <class T, class Key, class GetKey> void Dictionary<T, Key, GetKey>::Grow()
{
	int oldCapacity = capacity;
	Entry *oldTable = table;
	int i;

	capacity = oldCapacity * 2;
	table = new Entry[capacity];

	for (i = 0; i < oldCapacity; i++) {
		if (oldTable[i].flag) {
			int slot = FindAvailableSlot(GetKey()(oldTable[i].value) * 7 % capacity);
			table[slot] = oldTable[i];
		}
	}
	delete[] oldTable;
}

template <class T, class Key, class GetKey> int Dictionary<T, Key, GetKey>::Regist(const T &value, const Key &extra)
{
	if (limit > 0) {
		if (limit < count + 1)
			return 0;
	}
	if (count + 1 > capacity / 2)
		Grow();

	int slot = FindAvailableSlot(GetKey()(value) * 7 % capacity);

	table[slot].flag = 1;
	table[slot].extra = extra;
	table[slot].value = value;
	count++;
	return 1;
}

template <class T, class Key, class GetKey> int Dictionary<T, Key, GetKey>::Get(const T &value, Key *extra) const
{
	int start = GetKey()(value) * 7 % capacity;
	int i;

	if (start < capacity) {
		for (i = start; i < capacity; i++) {
			if (table[i].flag) {
				if (table[i].value == value) {
					*extra = table[i].extra;
					return 1;
				}
			}
		}
	}
	for (i = 0; i < start; i++) {
		if (table[i].flag) {
			if (table[i].value == value) {
				*extra = table[i].extra;
				return 1;
			}
		}
	}
	return 0;
}

template <class T, class Key, class GetKey> void Dictionary<T, Key, GetKey>::Remove(const T &value)
{
	int start = GetKey()(value) * 7 % capacity;
	int i;

	if (start < capacity) {
		for (i = start; i < capacity; i++) {
			if (table[i].flag) {
				if (table[i].value == value) {
					count--;
					table[i].flag = 0;
					return;
				}
			}
		}
	}
	for (i = 0; i < start; i++) {
		if (table[i].flag) {
			if (table[i].value == value) {
				count--;
				table[i].flag = 0;
				return;
			}
		}
	}
}

template <class T, class Key, class GetKey> void Dictionary<T, Key, GetKey>::Dump() const
{
	for (int i = 0; i < capacity; i++) {
		printf("dictionary [%d] = %x, %x, %x \n", i, table[i].flag, GetKey()(table[i].value), table[i].extra);
	}
}

#endif /* GUARD_DICTIONARY_H */
