/*
 * Database: a text file of one record per line, keyed through a caller
 * supplied comparison function. Updates rewrite the file through "<name>.bak".
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <pthread.h>

class Database {
public:
	typedef int (*Compare)(const char *, const char *, void *);

	Database(const string &name, Compare cmp);
	~Database();

	typedef int (*Visitor)(const char *, void *, bool &);

	void Add(const char *rec);
	vector<string> Find(const char *key, void *arg, int flags);
	int ForEach(Visitor fn, void *arg, int flags);

	/* 0x00 */ string name;
	/* 0x04 */ Compare cmp;
	/* 0x08 */ pthread_mutex_t mutex;
	/* 0x20 */ int ready;
};

#endif
