/*
 * Enumeration of the "<name>,<value>" entries of a system configuration file.
 */

#include <string>
#include <stdlib.h>
#include <string.h>

#include "bnsysconf.h"

#include "fs/bbfile.h"

#include <sys/stat.h>

static char *g_sysconfDir = "/opt0/bn/sysconf/";

/* Splits the buffer into lines, each line into "<name>,<value>", and calls
   cb for every entry (or only for `name` when given). Stops with -1 when cb
   returns a negative value. */
static int bn_sysconf_parse(char *buf, const char *name, SysconfEnumCb cb, void *ctx)
{
	char *line;

	while ((line = strsep(&buf, "\r\n")) != 0) {
		char *k = strsep(&line, ",");

		if (k == 0)
			continue;
		if (line == 0)
			continue;
		if (name != 0 && strcmp(k, name) != 0)
			continue;
		if (cb(k, line, ctx) < 0)
			return -1;
	}
	return 0;
}

/* Reads the whole file <sysconf dir><key> and passes it to the parser
   together with the caller's name, callback and context. */
extern "C" int bnEnumSysconf(const char *key, const char *name, SysconfEnumCb cb, void *ctx)
{
	string path = string(g_sysconfDir) + key;
	struct stat st;
	int r;
	char *buf;
	int fd;

	r = BBlstatCPP(path.c_str(), &st);
	if (r < 0)
		goto out;
	buf = (char *)malloc(st.st_size + 1);
	if (buf == 0)
		goto nomem;
	fd = BBopenCPP(path.c_str(), 0);
	if (fd >= 0) {
		int n = BBreadCPP(fd, buf, st.st_size);

		if (n == st.st_size) {
			buf[n] = 0;
			r = bn_sysconf_parse(buf, name, cb, ctx);
		}
		BBcloseCPP(fd);
	}
	free(buf);
	goto out;
nomem:
	r = -1;
out:
	return r;
}
