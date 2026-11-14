/*
 * Database methods: record add, search and visit, and the rewrite through
 * the ".bak" file.
 */

#include "database.h"

#include <stdio.h>
#include <stdlib.h>

/* Copies the rest of `src` into `dst` in 2 KB blocks. */
static inline void copyRest(FILE *src, FILE *dst)
{
	char buf[2048];
	unsigned int n;

	for (;;) {
		n = fread(buf, 1, 2048, src);
		if (n == 0) {
			feof(src);
			break;
		}
		if (fwrite(buf, 1, n, dst) < n)
			break;
	}
}

/* Writes "<name>.bak" with the new record first, followed by the whole old
   file, then replaces the file with it. The record is passed without its
   newline. */
void Database::Add(const char *rec)
{
	FILE *src;
	FILE *dst = 0;

	string bak = name + ".bak";
	string line = string(rec) + "\n";

	pthread_mutex_lock(&mutex);
	src = fopen(name.c_str(), "a+");
	if (src == 0) {
		perror("fopen");
	} else {
		fseek(src, 0, SEEK_SET);
		dst = fopen(bak.c_str(), "w");
		if (dst == 0) {
			perror("fopen");
		} else {
			if (fwrite(line.c_str(), line.length(), 1, dst) != 0)
				copyRest(src, dst);
		}
	}
	if (src)
		fclose(src);
	if (dst)
		fclose(dst);
	/* NOTE: bug: replaces the file even when the .bak could not be written */
	remove(name.c_str());
	rename(bak.c_str(), name.c_str());
	pthread_mutex_unlock(&mutex);
}

/* Reads one line of any length: counts its characters one at a time, seeks
   back, then rereads it with fgets into a buffer of the right size.
   Returns 0 when a line was read, 1 at end of file, -1 on error. */
static int read_line(FILE *fp, string &out)
{
	char c;
	char *p = 0;
	int len = 0;
	int ret = 0;

	for (;;) {
		if (fread(&c, 1, 1, fp) == 0) {
			if (feof(fp)) {
				ret = 1;
			} else {
				perror("fread");
				ret = -1;
			}
			goto out_;
		}
		len++;
		if (c == '\n')
			break;
	}

	fseek(fp, -len, SEEK_CUR);
	len++;
	p = (char *)malloc(len);
	if (p == 0) {
		perror("malloc");
		ret = -1;
		goto out_;
	}
	if (fgets(p, len, fp) == 0) {
		perror("fgets");
		ret = -1;
		goto out_;
	}
	out = p;
out_:
	if (p)
		free(p);
	return ret;
}

/* Copies the whole of `src` into `dst` in 2 KB blocks. Returns 0 when the
   end of the file was reached, -1 if a read or a write failed. */
static inline int copyAll(FILE *src, FILE *dst)
{
	char buf[2048];
	unsigned int n;

	int r;

	for (;;) {
		n = fread(buf, 1, 2048, src);
		if (n == 0) {
			if (feof(src))
				r = 0;
			else
				r = -1;
			break;
		}
		if (fwrite(buf, 1, n, dst) < n) {
			r = -1;
			break;
		}
	}
	return r;
}

/* Returns the records for which cmp(key, record, arg) == 0. flags & 1 stops
   at the first match; flags & 2 rewrites the file with the matching records
   moved to the front. */
vector<string> Database::Find(const char *key, void *arg, int flags)
{
	vector<string> found;
	FILE *src;
	FILE *dst = 0;
	FILE *rest = 0;

	string bak = name + ".bak";
	string bak2 = name + ".bak2";

	pthread_mutex_lock(&mutex);
	{
		string line;

		src = fopen(name.c_str(), "r");
		if (src == 0) {
			perror("fopen");
			goto done;
		}
		if (flags & 2) {
			dst = fopen(bak.c_str(), "w");
			if (dst == 0) {
				perror("fopen");
				goto done;
			}
			rest = fopen(bak2.c_str(), "w+");
			if (rest == 0) {
				perror("fopen");
				goto done;
			}
		}
		for (;;) {
			if (read_line(src, line))
				goto endloop;

			string t(line, 0, line.find_last_not_of("\r\n") + 1);
			if (cmp(key, t.c_str(), arg) == 0) {
				found.push_back(line);
				if (flags & 2) {
					if (fwrite(line.c_str(), line.length(), 1, dst) == 0)
						goto done;
				}
				if (flags & 1)
					goto endloop;
			} else if (flags & 2) {
				if (fwrite(line.c_str(), line.length(), 1, rest) == 0)
					goto done;
			}
		}
endloop:
		if (flags & 2) {
			fseek(rest, 0, SEEK_SET);
			if (copyAll(rest, dst) == 0)
				copyRest(src, dst);
		}
done:;
	}
	if (src)
		fclose(src);
	if (dst)
		fclose(dst);
	if (rest)
		fclose(rest);
	if (flags & 2) {
		remove(name.c_str());
		rename(bak.c_str(), name.c_str());
		remove(bak2.c_str());
	}
	pthread_mutex_unlock(&mutex);
	return found;
}

/* Calls `fn` on every record (without its line terminators) with a delete
   flag. With flags & 1 the file is rewritten without the records whose flag
   was set. */
int Database::ForEach(Visitor fn, void *arg, int flags)
{
	FILE *src;
	FILE *dst = 0;

	string bak = name + ".bak";

	pthread_mutex_lock(&mutex);
	{
		string line;

		src = fopen(name.c_str(), "r");
		if (src == 0) {
			perror("fopen");
		} else {
			dst = fopen(bak.c_str(), "w");
			if (dst == 0) {
				perror("fopen");
			} else {
				for (;;) {
					if (read_line(src, line))
						goto endloop;

					bool del = false;
					string t(line, 0, line.find_last_not_of("\r\n") + 1);
					fn(t.c_str(), arg, del);
					if (flags & 1) {
						if (!del) {
							if (fwrite(line.c_str(), line.length(), 1, dst) == 0)
								goto endloop;
						}
					}
				}
endloop:;
			}
		}
	}
	if (src)
		fclose(src);
	if (dst)
		fclose(dst);
	if (flags & 1) {
		remove(name.c_str());
		rename(bak.c_str(), name.c_str());
	}
	pthread_mutex_unlock(&mutex);
	return 0;
}

Database::Database(const string &name, Compare cmp) : name(name), cmp(cmp), ready(0)
{
	pthread_mutex_init(&mutex, 0);
}

Database::~Database()
{}
