/*
 * Variables: the name -> value tables (multimaps, so a name can have several
 * values) and their mutex.
 */

#pragma implementation "variables.h"
#include "variables.h"
#include <sys/stat.h>
#include "fs/bbfile.h"

multimap<string, string> variable_table;
multimap<string, string> sub_variable_table;
pthread_mutex_t table_mutex;

Variables::Variables()
{}

Variables variables;

void Variables::Initialize()
{
	pthread_mutex_init(&table_mutex, 0);
	variable_table.clear();
	sub_variable_table.clear();
}

/* SetVariable replaces every value of `name`; AddVariable adds one more. */
void Variables::SetVariable(const string &name, const string &value)
{
	pthread_mutex_lock(&table_mutex);
	variable_table.erase(name);
	variable_table.insert(pair<string, string>(name, value));
	pthread_mutex_unlock(&table_mutex);
}

void Variables::SetVariable(const string &name, const list<string> &values)
{
	pthread_mutex_lock(&table_mutex);
	variable_table.erase(name);
	for (list<string>::const_iterator it = values.begin(); it != values.end(); ++it)
		variable_table.insert(pair<string, string>(name, *it));
	pthread_mutex_unlock(&table_mutex);
}

void Variables::AddVariable(const string &name, const string &value)
{
	pthread_mutex_lock(&table_mutex);
	variable_table.insert(pair<string, string>(name, value));
	pthread_mutex_unlock(&table_mutex);
}

void Variables::AddVariable(const string &name, const list<string> &values)
{
	pthread_mutex_lock(&table_mutex);
	for (list<string>::const_iterator it = values.begin(); it != values.end(); ++it)
		variable_table.insert(pair<string, string>(name, *it));
	pthread_mutex_unlock(&table_mutex);
}

/* Fills `values` with every value of `name` and returns how many there are.
   A leading "$" escape is removed first ("$$FOO" -> "$FOO"). */
int Variables::GetVariable(const string &name, list<string> &values)
{
	values.clear();

	string namecopy = name;
	string::size_type idx = namecopy.find_first_not_of('$');
	if (idx != 0 && idx != string::npos)
		namecopy.erase(idx - 1, 1);

	pthread_mutex_lock(&table_mutex);
	multimap<string, string>::const_iterator it = variable_table.find(namecopy);
	if (it != variable_table.end()) {
		multimap<string, string>::const_iterator last = variable_table.upper_bound(namecopy);
		int count = 0;
		do {
			values.push_back(it->second);
			count++;
			++it;
		} while (it != last);
		pthread_mutex_unlock(&table_mutex);
		return count;
	} else {
		pthread_mutex_unlock(&table_mutex);
		return 0;
	}
}

void Variables::DeleteVariable(const string &name)
{
	pthread_mutex_lock(&table_mutex);
	variable_table.erase(name);
	pthread_mutex_unlock(&table_mutex);
}

int Variables::ReadFile()
{
	return 0;
}

int Variables::ReadFileSub()
{
	return 0;
}

int Variables::WriteFile()
{
	return 0;
}

void Variables::DeleteFile()
{
	BBremoveCPP("pfs:__contents/bn.conf/bnaval.dat");
}

/* Checks whether bnaval.dat exists: a missing file (-2) is not an error.
   Returns 0 or the negative error code. */
int Variables::CheckFile(bool &result)
{
	struct stat st;
	result = false;
	int r = BBlstatCPP("pfs:__contents/bn.conf/bnaval.dat", &st);
	if (-2 == r) {
		result = false;
		return 0;
	}
	if (r < 0) {
		result = false;
		return r;
	}
	result = true;
	return 0;
}

/* Replaces the main table with the entries of the sub table that have both a
   non-empty name and a non-empty value. */
int Variables::Sub2Main()
{
	pthread_mutex_lock(&table_mutex);
	variable_table.clear();
	for (multimap<string, string>::iterator it = sub_variable_table.begin(); it != sub_variable_table.end(); it++) {
		int keylen = (*it).first.length();
		if (keylen > 0) {
			int vallen = (*it).second.length();
			if (vallen > 0)
				variable_table.insert(pair<string, string>((*it).first, (*it).second));
		}
	}
	pthread_mutex_unlock(&table_mutex);
	return 0;
}
