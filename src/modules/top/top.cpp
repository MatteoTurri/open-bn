/*
 * The Top Menu module. bn dlopens it from /opt0/bn/modules/, calls Init and
 * registers FuncTable, the Small natives "topIsp*" that the pages of the Top
 * Menu call to list the ISP settings files of /opt0/bn/sysconf/ispch and read
 * their key=value pairs.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <map>
#include "amx.h"
#include "app/stringutil.h"
#include "thread/thposixthread.h"
#include "fs/bbfile.h"
#include "ui/navimenu.h"

static const char *ispch_dir = "/opt0/bn/sysconf/ispch";

/* Paths of the files of ispch_dir, in the order BBreaddirCPP gave them. */
static vector<string> isp_files;
/* key=value pairs of the files opened since the last topIspClose; a later file overrides a key. */
static map<string, string> isp_values;
/* Returned by topIspSync; nothing in the module writes it. */
static int isp_sync;

static cell AMX_NATIVE_CALL n_topInit(AMX *amx, cell *params);

/* Fills isp_files with the paths of the entries of dir. */
static int isp_scan(const char *dir)
{
	isp_files.clear();
	int d = BBopendirCPP(dir);
	if (d >= 0) {
		struct dirent *ent;
		while ((ent = BBreaddirCPP(d)) != 0) {
			if (strcmp(ent->d_name, ".") == 0)
				continue;
			if (strcmp(ent->d_name, "..") == 0)
				continue;
			isp_files.push_back(string(dir) + "/" + ent->d_name);
		}
		BBclosedirCPP(d);
	}
	return 0;
}

static void isp_load()
{
	isp_scan(ispch_dir);
}

/* topIspGetCount(): the number of files found by the last scan. */
static cell AMX_NATIVE_CALL n_topIspGetCount(AMX *amx, cell *params)
{
	return isp_files.size();
}

static cell AMX_NATIVE_CALL n_topIspSync(AMX *amx, cell *params)
{
	return isp_sync;
}

/* Splits buf into lines of key=value and stores them in isp_values. */
static int isp_parse(char *buf)
{
	char *line;
	while ((line = strsep(&buf, "\r\n")) != 0) {
		char *key = strsep(&line, "=");
		if (key == 0 || line == 0)
			continue;
		isp_values[key] = line;
	}
	return 0;
}

/*
 * topIspOpen(index): reads the file isp_files[index] and adds its pairs to
 * isp_values. Returns -1 for a bad index, the error of BBlstatCPP, or 0, also
 * when the file cannot be read.
 */
static cell AMX_NATIVE_CALL n_topIspOpen(AMX *amx, cell *params)
{
	int ret = -1;
	if (params[1] < isp_files.size()) {
		struct stat st;
		ret = BBlstatCPP(isp_files[params[1]].c_str(), &st);
		if (ret >= 0) {
			char *buf = (char *)malloc(st.st_size + 1);
			if (buf == 0)
				return ret;
			int fd = BBopenCPP(isp_files[params[1]].c_str(), 0);
			if (fd >= 0) {
				int n = BBreadCPP(fd, buf, st.st_size);
				if (n == st.st_size) {
					buf[n] = '\0';
					ret = isp_parse(buf);
				}
				BBcloseCPP(fd);
			}
			free(buf);
		}
	}
	return ret;
}

/* topIspClose(): forgets the pairs read by topIspOpen. */
static cell AMX_NATIVE_CALL n_topIspClose(AMX *amx, cell *params)
{
	isp_values.clear();
	return 0;
}

/* topIspGetValue(key[], value[]): copies the value of key from isp_values; 1 if found, else 0. */
static cell AMX_NATIVE_CALL n_topIspGetValue(AMX *amx, cell *params)
{
	map<string, string>::iterator it;
	char key[32];
	cell *cstr;

	if (amx_GetAddr(amx, params[1], &cstr) != AMX_ERR_NONE)
		return 0;
	amx_GetString(key, cstr);
	if ((it = isp_values.find(key)) != isp_values.end()) {
		cell *dest;
		if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE)
			return 0;
		amx_SetString(dest, (char *)(*it).second.c_str(), 1);
		return 1;
	}
	return 0;
}

AMX_NATIVE_INFO FuncTable[] = {
	{ "topInit", n_topInit },
	{ "topIspGetCount", n_topIspGetCount },
	{ "topIspSync", n_topIspSync },
	{ "topIspOpen", n_topIspOpen },
	{ "topIspClose", n_topIspClose },
	{ "topIspGetValue", n_topIspGetValue },
	{ 0, 0 },
};

/* Entry points that bn resolves by name after dlopen (app/module.cpp). */
extern "C" void Init()
{
	isp_files.clear();
	isp_values.clear();
}

extern "C" void Exit()
{
	isp_files.clear();
	isp_values.clear();
}

/* topInit(): enables the navigation menu and scans ispch_dir. */
static cell AMX_NATIVE_CALL n_topInit(AMX *amx, cell *params)
{
	NaviMenu::Enable(true);
	isp_load();
	return 0;
}
