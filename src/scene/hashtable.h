/*
 * HashTable<T>: open-addressing hash table with linear probing, keyed by the
 * ids handed out by Regist (ObjectTable, ModelTable).
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdio.h>

/*
 * Keys are increasing ids starting from 1 (key 0 marks a free slot), hashed
 * as (id * 7) % capacity; the table doubles when it gets more than half full.
 */
template <class T> class HashTable {
public:
	struct Entry {
		/* 0x00 */ T value;
		/* 0x04 */ int key;
		/* 0x08 */ int extra;
	};

	/* 0x00 */ int limit;  /* maximum entries, <= 0 = unlimited */
	/* 0x04 */ int capacity;  /* buckets */
	/* 0x08 */ Entry *table;
	/* 0x0C */ int next_id;  /* never reused */
	/* 0x10 */ int count;  /* entries in use */

	HashTable(int capacity, int limit);
	~HashTable();

	int FindAvailableSlot(int start) const;
	void Grow();
	int Regist(const T &value, int extra);
	T Get(int key, int *extra) const;
	int Remove(int key, int *extra);
	void Dump() const;
};

template <class T> HashTable<T>::HashTable(int n, int max)
{
	limit = max;
	capacity = n;
	table = 0;
	next_id = 1;
	count = 0;

	Entry *newtable = new Entry[n];
	Entry *p = newtable;
	for (int i = n - 1; i != -1; p++, i--) {
		p->value = 0;
		p->key = 0;
		p->extra = 0;
	}
	table = newtable;
}

template <class T> HashTable<T>::~HashTable()
{
	if (table)
		delete[] table;
}

template <class T> int HashTable<T>::FindAvailableSlot(int start) const
{
	int i = start;
	if (i < capacity) {
		for (; i < capacity; i++)
			if (table[i].key == 0)
				return i;
	}
	for (int j = 0; j < start; j++)
		if (table[j].key == 0)
			return j;
	return -1;
}

template <class T> void HashTable<T>::Grow()
{
	int n = capacity;
	int newcap = n * 2;
	Entry *oldtable = table;
	capacity = newcap;

	Entry *newtable = new Entry[newcap];
	Entry *q = newtable;
	for (int i = newcap - 1; i != -1; q++, i--) {
		q->value = 0;
		q->key = 0;
		q->extra = 0;
	}
	table = newtable;
	for (int j = 0; j < n; j++) {
		if (oldtable[j].key != 0) {
			int esz = sizeof(Entry);
			int slot = FindAvailableSlot((oldtable[j].key * 7) % capacity);
			Entry *dst = (Entry *)((char *)table + slot * esz);
			dst->value = oldtable[j].value;
			dst->key = oldtable[j].key;
			dst->extra = oldtable[j].extra;
		}
	}

	if (oldtable)
		delete[] oldtable;
}

template <class T>
/* Inserts `value` and returns its key (even if no slot was free), or 0 if the limit is reached. */
int HashTable<T>::Regist(const T &value, int extra)
{
	if (limit > 0 && count + 1 > limit)
		return 0;

	if (count + 1 > capacity / 2)
		Grow();

	int id = next_id;
	int slot = FindAvailableSlot((id * 7) % capacity);
	if (slot >= 0) {
		table[slot].value = value;
		table[slot].key = id;
		table[slot].extra = extra;
		next_id++;
		count++;
	}
	return id;
}

template <class T> T HashTable<T>::Get(int key, int *extra) const
{
	if (key == 0)
		return 0;
	int start = (key * 7) % capacity;
	for (int i = start; i < capacity; i++)
		if (table[i].key == key) {
			if (extra)
				*extra = table[i].extra;
			return table[i].value;
		}
	for (int i = 0; i < start; i++)
		if (table[i].key == key) {
			if (extra)
				*extra = table[i].extra;
			return table[i].value;
		}
	return 0;
}

template <class T> int HashTable<T>::Remove(int key, int *extra)
{
	if (key == 0)
		return 0;
	int start = (key * 7) % capacity;
	for (int i = start; i < capacity; i++)
		if (table[i].key == key) {
			count--;
			if (extra)
				*extra = table[i].extra;
			table[i].key = 0;
			table[i].value = 0;
			table[i].extra = 0;
			return 1;
		}
	for (int i = 0; i < start; i++)
		if (table[i].key == key) {
			count--;
			if (extra)
				*extra = table[i].extra;
			table[i].key = 0;
			table[i].value = 0;
			table[i].extra = 0;
			return 1;
		}
	return 0;
}

template <class T> void HashTable<T>::Dump() const
{
	for (int i = 0; i < capacity; i++)
		printf("table[%d] = ( %x, %p )\n", i, table[i].key, table[i].value);
}

#endif
