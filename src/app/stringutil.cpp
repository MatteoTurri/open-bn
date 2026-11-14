/*
 * StringUtil: string with upper-casing, replace-all and split helpers.
 */

#include "strutil.h"

void StringUtil::MakeUpper()
{
	for (unsigned int i = 0; i < length(); i++) {
		if ((*this)[i] >= 'a' && (*this)[i] <= 'z')
			(*this)[i] -= 32;
	}
}

/* Replaces every occurrence of `from` with `to` and returns the count.
   Scanning resumes after the inserted text. */
int StringUtil::Replace(const StringUtil &from, const StringUtil &to)
{
	int count = 0;
	unsigned int pos = 0;

	while (pos < length()) {
		pos = find(from, pos);
		if (pos == npos)
			return count;
		replace(pos, from.length(), to);
		pos += to.length();
		count++;
	}
	return count;
}

/* Splits *this at every `sep`. The tail after the last separator is always
   added, so the result has at least one element. */
vector<StringUtil> StringUtil::Separate(const string &sep) const
{
	vector<StringUtil> list;
	unsigned int pos = 0;
	unsigned int start = 0;

	while ((pos = find(sep, pos)) != npos) {
		list.push_back(substr(start, pos - start));
		start = pos + sep.length();
		pos = start;
	}
	list.push_back(substr(start, length() - start));
	return list;
}
