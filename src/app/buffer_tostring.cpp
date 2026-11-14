/*
 * Buffer::operator string(): copies the buffer bytes into a NUL-terminated
 * string.
 */

#include "buffer.h"

#include <stdlib.h>
#include <string.h>

Buffer::operator string() const
{
	string tmp;
	int n = m_p->length;
	char *buf = (char *)malloc(n + 1);

	if (buf) {
		memcpy(buf, m_p->data, n);
		buf[n] = 0;
		tmp = buf;
		free(buf);
	}
	return tmp;
}
