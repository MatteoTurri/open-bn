/*
 * Variables: the global name -> value store of the Broadband Navigator
 * (persisted in bnaval.dat). All the state is in file-scope globals; the
 * class only groups the static accessors.
 */

#ifndef VARIABLES_H
#define VARIABLES_H

#include <pthread.h>
#include <map>
#include <string>
#include <list>

class Variables {
public:
	Variables();
	virtual ~Variables() {}

	static void Initialize();

	static int ReadFile();
	static int ReadFileSub();
	static int WriteFile();
	static void DeleteFile();
	static int CheckFile(bool &result);

	static void DeleteVariable(const string &name);
	static void SetVariable(const string &name, const string &value);
	static void SetVariable(const string &name, const list<string> &values);
	static void AddVariable(const string &name, const string &value);
	static void AddVariable(const string &name, const list<string> &values);
	static int GetVariable(const string &name, list<string> &values);
	/* Like GetVariable, but if the variable is missing `values` receives `def`
		   (the returned count is still the real one). */
	static int GetVariable(const string &name, list<string> &values, const string &def)
	{
		int n = GetVariable(name, values);
		if (n <= 0)
			values.push_back(def);
		return n;
	}

	static int Sub2Main();
};

#endif
