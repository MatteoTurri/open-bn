/*
 * StringUtil: a string with upper-casing, replace-all, split and right-part
 * helpers.
 */

#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <string>
#include <vector>

class StringUtil : public string {
public:
	StringUtil() {}
	StringUtil(const string &s) : string(s) {}
	StringUtil(const char *s) { assign(s); }
	void MakeUpper();
	int Replace(const StringUtil &from, const StringUtil &to);
	vector<StringUtil> Separate(const string &sep) const;
};

#endif
