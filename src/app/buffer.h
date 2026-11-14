/*
 * Buffer: a reference-counted block of bytes, used for the data of a loaded
 * page.
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <string>
#include <stdlib.h>
#include <string.h>

/* Passed by value: the copy constructor shares the Rep and bumps its
   reference count. */
class Buffer {
public:
	struct Rep {
		/* 0x00 */ void *data;  /* released with free() */
		/* 0x04 */ int length;  /* in bytes */
		/* 0x08 */ int ref;  /* reference count */
		void *GetData() const { return data; }
		int GetSize() const { return length; }
		unsigned int size() const { return length; }
	};
	Buffer() : m_p(0) {}
	Buffer(const Buffer &o) : m_p(o.m_p)
	{
		if (m_p)
			m_p->ref++;
	}
	~Buffer()
	{
		Rep *p = m_p;
		if (p && --p->ref <= 0) {
			if (p->data)
				free(p->data);
			delete p;
		}
	}
	Buffer(const string &s)
	{
		Rep *r = new Rep;
		const char *p = s.c_str();

		r->length = s.length();
		r->data = 0;
		r->ref = 1;
		if (p != 0 && r->length != 0) {
			r->data = malloc(r->length);
			memcpy(r->data, p, r->length);
		}
		m_p = r;
	}
	Buffer(const void *src, int n) : m_p(0)
	{
		if (src != 0) {
			Rep *r = new Rep;

			r->data = 0;
			r->length = n;
			r->ref = 1;
			if (n != 0) {
				r->data = malloc(n);
				memcpy(r->data, src, r->length);
			}
			m_p = r;
		}
	}
	Buffer &operator=(const Buffer &o)
	{
		Rep *n = o.m_p;
		Rep *old = m_p;

		m_p = n;
		if (n)
			n->ref++;
		if (old && --old->ref <= 0) {
			if (old->data)
				free(old->data);
			delete old;
		}
		return *this;
	}
	void *GetData() const { return m_p ? m_p->GetData() : 0; }
	int GetSize() const { return m_p ? m_p->GetSize() : 0; }
	unsigned int size() const { return m_p ? m_p->size() : 0; }
	operator string() const;

	Rep *m_p;
};

#endif
