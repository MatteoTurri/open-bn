/*
 * Small/amx natives "sysUpdate*" (table smallext_sysup): system update of the
 * BN. Update info and packages are downloaded by a background "TM" thread;
 * the operations on /opt0 run as root (BecomeRoot/RestoreUid).
 */

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib/resuid.h"
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>
#include <string>
#include "amx.h"
#include "natives/smallext.h"
#include "script/script.h"
#include "thread/thposixthread.h"
#include "http/httpfile.h"
#include "dnas/dnas2sysupmanager.h"
#include "lib/scepfsd.h"
#include "fs/bbfile.h"


#include "app/bndownload.h"

#include "ui/navimenu.h"
#include "script/script_abort_entry.h"

/* Flag file: a system update is in progress. */
#define SYSUPDATE_TMP "/opt0/bn/sysconf/sysupdate.tmp"

/* Update info of one package: a = version, b = download URL (empty = nothing new). */
class _upinfo {
public:
	/* 0x00 */ string a;
	/* 0x04 */ string b;
};

/* Credentials and server URL from the BNUPDATE configuration file. */
static struct {
	string user;       /* key "USER"     */
	string password;   /* key "PASSWORD" */
	string url;        /* key "URL"      */
} s_conf;

static string s_bnVersion[3];

static _upinfo s_upinfo[2];

/*
 * Arguments of the TM thread, set by sysUpdateTMStart: 0 = update info URL,
 * 1 = update info file, 2 = package URL, 3 = package destination file.
 */
static string s_tmInfoUrl;
static string s_tmInfoFile;
static string s_tmPkgUrl;
static string s_tmPkgFile;

/* User ids saved by BecomeRoot and restored by RestoreUid. */
static int s_uidResult;
static uid_t s_ruid;
static uid_t s_euid;
static uid_t s_suid;

/* The TM thread and its state. */
static pthread_t s_tmThread;
static int s_tmProgress;       /* result of the last run */
static int s_tmMode;           /* 0 = download the package, nonzero = fetch the update info */
static int s_tmRunning;        /* nonzero while the thread runs */
static int s_tmDone;           /* the update info changed */

/*
 * Creates (or recreates) the APA partition "__linux.<part>" and formats it as
 * reiserfs.
 */
static int MkfsBntmp(int part, const char *size, int nsub, const char *fpwd, const char *rpwd, int mode)
{
	int pfd[2];
	char path[128];
	int status;
	int fd;
	int pid;
	int n = 0;

	if (strlen(size) >= 5 || strlen(fpwd) >= 9 || strlen(rpwd) >= 9)
		return 1;

	scepfsInit();
	sprintf(path, "hdd0:__linux.%d", part);
	fd = scepfsOpen(path, 1);
	if (fd >= 0) {
		n = scepfsIoctl2(fd, 0x6803, 0, 0, 0, 0);  /* number of sub-partitions */
		if (n < 0) {
			scepfsExit();
			return -1;
		}
		scepfsClose(fd);
		if (mode == 0 && n == nsub) {
			scepfsExit();
			return 0;
		}
		if (mode == 2)
			scepfsRemove(path);
	}
	sprintf(path, "hdd0:__linux.%d,%s,%s,%s,REISER", part, fpwd, rpwd, size);
	fd = scepfsOpen(path, 0x203);
	if (fd < 0) {
		scepfsExit();
		return -1;
	}
	while (n < nsub) {
		if (scepfsIoctl2(fd, 0x6801, (void *)size, strlen(size) + 1,  /* add a sub-partition */
		        0, 0)
		    < 0) {
			scepfsClose(fd);
			sprintf(path, "hdd0:__linux.%d", part);
			scepfsRemove(path);
			scepfsExit();
			return -1;
		}
		n++;
	}
	scepfsClose(fd);
	scepfsExit();

	fd = open("/dev/hda", O_RDWR);
	ioctl(fd, BLKRRPART, 0); /* reread the partition table */
	close(fd);

	if (pipe(pfd) < 0)
		return -1;
	pid = fork();
	if (pid < 0)
		return -1;
	if (pid == 0) {
		dup2(pfd[0], 0);
		close(pfd[1]);
		sprintf(path, "/dev/hda%d", part);
		execl("/sbin/mkreiserfs", "/sbin/mkreiserfs", "-q", path, 0);
	} else {
		char yes[] = "y\n";
		close(pfd[0]);
		write(pfd[1], yes, strlen(yes));
		waitpid(pid, &status, 0);
	}
	close(pfd[0]);
	close(pfd[1]);
	return n + 1;
}

/* Switches to root, saving the user ids, and back. */

static int BecomeRoot()
{
	s_uidResult = getresuid(&s_ruid, &s_euid, &s_suid);
	if (s_uidResult == 0)
		s_uidResult = setresuid(0, 0, 0);
	return s_uidResult;
}

static int RestoreUid()
{
	return s_uidResult = setresuid(s_ruid, s_euid, s_suid);
}

static cell AMX_NATIVE_CALL n_err11012(AMX *amx, cell *params)
{
	return BecomeRoot();
}

static cell AMX_NATIVE_CALL n_err11013(AMX *amx, cell *params)
{
	return RestoreUid();
}

/* Setters of the configuration keys, parallel to CONF_KEYS. */

static int SetConfUser(const char *s)
{
	s_conf.user = s;
	return 0;
}

static int SetConfPassword(const char *s)
{
	s_conf.password = s;
	return 0;
}

static int SetConfUrl(const char *s)
{
	s_conf.url = s;
	return 0;
}

/* Index of key in tab, -1 if absent. */
static int FindKey(const char *key, const char *const *tab, int n)
{
	int i;

	for (i = 0; i < n; i++)
		if (strcmp(key, tab[i]) == 0)
			return i;
	return -1;
}

/* Parses "KEY=value" lines, calling the setter of each known key; -1 when a setter fails. */
static int ParseConf(char *text, const char *const *keys, int (*const *funcs)(const char *), int nkeys)
{
	char *line;

	while ((line = strsep(&text, "\r\n")) != 0) {
		char *key = strsep(&line, "=");
		int i;

		if (key == 0 || line == 0)
			continue;
		i = FindKey(key, keys, nkeys);
		if (i < 0)
			continue;
		if (funcs[i](line) < 0)
			return -1;
	}
	return 0;
}

/* Configuration keys and their setters (parallel tables). */
static const char *CONF_KEYS[3] = { "USER", "PASSWORD", "URL" };

static int (*CONF_SETTERS[3])(const char *) = {
	SetConfUser,
	SetConfPassword,
	SetConfUrl,
};

static const char *s_htmlType = "text/html";

static int ParseConfText(char *text)
{
	return ParseConf(text, CONF_KEYS, CONF_SETTERS, 3);
}

/* True when s contains a CR or LF. */
static int HasEol(const char *s)
{
	while (*s != 0) {
		if (*s == '\r' || *s == '\n')
			return 1;
		s++;
	}
	return 0;
}

/* Stores s in s_bnVersion[i] without its non-numeric prefix ("BN-1.02" -> "1.02"). */
static void SetBnVersion(int i, string s)
{
	unsigned int j;

	s_bnVersion[i] = s;
	for (j = 0; j < s_bnVersion[i].length(); j++)
		if (isdigit(s_bnVersion[i][j]))
			break;
	s_bnVersion[i].erase(0, j);
}

/* Clears the update info: s_tmDone and the versions in s_upinfo. */
static void ClearUpInfo()
{
	int i;

	s_tmDone = 0;
	for (i = 0; i < 2; i++)
		s_upinfo[i].a.erase();
}

/* Reads back the two versions written by WriteUpInfoFile, one per line. */
static void ReadUpInfoFile(const char *path)
{
	char buf[256];
	FILE *fp = fopen(path, "r");
	int i;

	if (fp != 0) {
		for (i = 0; i < 2; i++) {
			char *p;

			if (fgets(buf, 256, fp) == 0)
				break;
			p = strchr(buf, '\n');
			if (p != 0)
				*p = 0;
			s_upinfo[i].a = buf;
		}
		fclose(fp);
	}
}

/* Writes the versions in s_upinfo to the file s_tmInfoFile; returns 1 on error. */
static int WriteUpInfoFile()
{
	int r = -1;
	FILE *fp = fopen(s_tmInfoFile.c_str(), "w");
	int i;

	if (fp != 0) {
		for (i = 0; i < 2; i++) {
			r = fprintf(fp, "%s\n", s_upinfo[i].a.c_str());
			if (r == -1)
				break;
		}
		fclose(fp);
		sync();
	}
	return r == -1;
}

/*
 * Parses the update info from the server: "VERSION,URL" lines, one per
 * package. When the version differs from s_upinfo[i].a, a and b are updated
 * and s_tmDone is set; otherwise b is cleared (nothing to download). A line
 * without a comma updates a only and always clears b.
 */
static int ParseUpInfo(char *text)
{
	char *line;
	int i = 0;

	while ((line = strsep(&text, "\r\n")) != 0) {
		char *key = strsep(&line, ",");

		if (key == 0)
			continue;
		if (line != 0) {
			if (strcmp(key, s_upinfo[i].a.c_str()) != 0) {
				s_upinfo[i].a = key;
				s_upinfo[i].b = line;
				s_tmDone = 1;
			} else {
				s_upinfo[i].b.erase();
			}
		} else {
			if (strlen(key) == 0)
				continue;
			if (strcmp(key, s_upinfo[i].a.c_str()) != 0) {
				s_upinfo[i].a = key;
				s_tmDone = 1;
			}
			s_upinfo[i].b.erase();
		}
		i++;  /* NOTE: bug: more than two lines overrun s_upinfo */
	}
	return 0;
}

/*
 * Downloads the update info and parses it. 401/407 retry once with user and
 * pass; a "text/html" answer is an error page and returns 404. Returns the
 * HTTP status, a negative parse result or -1002 when out of memory.
 */
static int FetchUpInfo(const char *url, const char *user, const char *pass)
{
	char *u = 0;
	char *p = 0;
	int ret = -1002;
	HttpFile *hf = new HttpFile;
	int st;

	if (hf != 0) {
		char ctype[128];
		sceHttpFileRequestArg arg;
		int done;

retry:
		ctype[0] = 0;
		memset(&arg, 0, sizeof(arg));
		arg.no_accept = 0;
		arg.done_flag = &done;
		arg.user = u;
		arg.pass = p;
		arg.keep_resp = 1;
		arg.ct_buf = ctype;
		arg.ct_len = 128;
		st = ret = hf->Request(url, 0, 0, &arg);
		if (ret == 200) {
			if (strncmp(ctype, s_htmlType, strlen(s_htmlType)) != 0) {
				char *buf = (char *)malloc(arg.length + 1);

				if (buf != 0) {
					memcpy(buf, (void *)arg.body, arg.length);
					buf[arg.length] = 0;
					ret = ParseUpInfo(buf);
					if (ret >= 0)
						ret = st;
					free(buf);
				} else {
					ret = -1002;
				}
			} else {
				ret = 404;
			}
		} else if (ret == 401 || ret == 407) {
			if (u == 0 && p == 0 && user != 0 && pass != 0) {
				u = (char *)user;
				p = (char *)pass;
				goto retry;
			}
		}
		delete hf;
	}
	return ret;
}

/* I/O callbacks of DownloadToFile: the HTTP layer writes straight into the BB file. */

static int RewindFd(int fd)
{
	return BBlseekCPP(fd, 0, 0);
}

static int WriteFd(int fd, const void *buf, int nbytes)
{
	return BBwriteCPP(fd, buf, nbytes);
}

/*
 * Downloads url into the file dst, written directly by the I/O callbacks.
 * An existing dst is replaced through dst + ".new" and a rename. Returns the
 * HTTP status (401/407 retry once with user/pass; a "text/html" answer counts
 * as 404), -1002 when no HttpFile can be created, -1003 when the file cannot
 * be opened or renamed; on any status other than 200 the file is removed.
 */
static int DownloadToFile(const char *dst, const char *url, const char *user, const char *pass)
{
	char *u = 0;
	char *p = 0;
	int ret = -1002;
	HttpFile *hf = new HttpFile;

	if (hf != 0) {
		string path = dst;
		struct stat st;
		char ctype[128];
		sceHttpFileRequestArg arg;
		int done;
		int fd;

		if (BBlstatCPP(dst, &st) == 0)
			path += ".new";
		fd = BBopenCPP(path.c_str(), 769, 436);
		ret = -1003;
		if (fd >= 0) {
retry:
			ctype[0] = 0;
			memset(&arg, 0, sizeof(arg));
			arg.no_accept = 0;
			arg.done_flag = &done;
			arg.user = u;
			arg.pass = p;
			arg.ct_buf = ctype;
			arg.ct_len = 128;
			arg.fd = fd;
			arg.write_cb = (int)WriteFd;
			arg.done_cb = (void (*)(int))RewindFd;
			ret = hf->Request(url, 0, 0, &arg);
			if (ret == 401 || ret == 407) {
				if (u == 0 && p == 0 && user != 0 && pass != 0) {
					u = (char *)user;
					p = (char *)pass;
					goto retry;
				}
			}
			BBcloseCPP(fd);
			if (ret == 200) {
				if (strncmp(ctype, s_htmlType, strlen(s_htmlType)) == 0)
					ret = 404;
			}
			if (ret != 200) {
				BBremoveCPP(path.c_str());
			} else if (strcmp(path.c_str(), dst) != 0) {
				BBremoveCPP(dst);
				if (BBrenameCPP(path.c_str(), dst) < 0)
					ret = -1003;
			}
		}
		delete hf;
	}
	return ret;
}

/*
 * Writes prefix + the last component of path to out; for a path ending in '/'
 * it takes the part of the previous component after its first '_'. -1 when
 * path has no '/'.
 */
static int MakeTmPath(char *out, const char *prefix, const char *path)
{
	char buf[256];
	char *p;

	strcpy(buf, path);
	p = strrchr(buf, '/');
	if (p == 0)
		return -1;
	if (strlen(p + 1) == 0) {
		*p = 0;
		p = strrchr(buf, '/');
		if (p == 0)
			return -1;
		p = strchr(p, '_');
		if (p == 0)
			return -1;
	}
	strcpy(out, prefix);
	strcat(out, p + 1);
	return 0;
}

/*
 * Writes path with its last component replaced by suffix to out; for a path
 * ending in '/', the previous component is cut after its first '_' and a
 * final '/' is added. -1 when path has no '/'.
 */
static int MakeTmDir(char *out, const char *suffix, const char *path)
{
	char buf[256];
	char *p;

	strcpy(buf, path);
	p = strrchr(buf, '/');
	if (p == 0)
		return -1;
	if (strlen(p + 1) == 0) {
		*p = 0;
		p = strrchr(buf, '/');
		if (p == 0)
			return -1;
		p = strchr(p, '_');
		if (p == 0)
			return -1;
		p[1] = 0;
		strcpy(out, buf);
		strcat(out, suffix);
		strcat(out, "/");
	} else {
		p[1] = 0;
		strcpy(out, buf);
		strcat(out, suffix);
	}
	return 0;
}

/*
 * Body of the TM thread: as root, fetches the update info (s_tmMode != 0) or
 * downloads the package to its file; the result goes to s_tmProgress.
 */
static void *TmThread(void *arg)
{
	int r;

	BecomeRoot();
	if (s_tmMode != 0) {
		ClearUpInfo();
		ReadUpInfoFile(s_tmInfoFile.c_str());
		r = FetchUpInfo(s_tmInfoUrl.c_str(), s_conf.user.c_str(), s_conf.password.c_str());
	} else {
		r = DownloadToFile(s_tmPkgFile.c_str(), s_tmPkgUrl.c_str(), s_conf.user.c_str(),
		    s_conf.password.c_str());
	}
	s_tmProgress = r;
	RestoreUid();
	s_tmRunning = 0;
	return 0;
}

/*
 * sysUpdateTMStart(arg1, arg2, mode): starts the TM thread. With mode != 0
 * fetches the update info (arg1 replaces the last component of the configured
 * URL, arg2 = local info file); with mode 0 downloads a package (arg1 = URL,
 * arg2 = destination file).
 */
static cell AMX_NATIVE_CALL n_sysUpdateTMStart(AMX *amx, cell *params)
{
	char buf[256];
	char path[256];
	cell *cptr;
	int r;

	s_tmMode = params[3] != 0;
	if (amx_GetAddr(amx, params[1], &cptr) == AMX_ERR_NONE) {
		amx_GetString(buf, cptr);
		if (s_tmMode != 0) {
			r = MakeTmDir(path, buf, s_conf.url.c_str());
			if (r < 0)
				return r;
			s_tmInfoUrl = path;
		} else {
			s_tmPkgUrl = buf;
		}
	}
	if (amx_GetAddr(amx, params[2], &cptr) == AMX_ERR_NONE) {
		amx_GetString(buf, cptr);
		if (s_tmMode != 0)
			s_tmInfoFile = buf;
		else
			s_tmPkgFile = buf;
	}
	s_tmRunning = 1;
	r = pthread_create(&s_tmThread, 0, TmThread, 0);
	if (r < 0)
		s_tmRunning = 0;
	return r;
}

/* sysUpdateTMStop(): stops the TM thread and waits for it. */
static cell AMX_NATIVE_CALL n_sysUpdateTMStop(AMX *amx, cell *params)
{
	if (s_tmRunning != 0)
		pthread_kill(s_tmThread, 16);
	pthread_join(s_tmThread, 0);
	return 0;
}

/* sysUpdateTMSync(&result): nonzero while the thread runs; result is set once it is over. */
static cell AMX_NATIVE_CALL n_sysUpdateTMSync(AMX *amx, cell *params)
{
	cell *cptr;
	int r = s_tmRunning;

	if (amx_GetAddr(amx, params[1], &cptr) == AMX_ERR_NONE) {
		if (r != 0)
			*cptr = 0;
		else
			*cptr = s_tmProgress;
	}
	return r;
}

/* sysUpdateTMUrl(i, buf): download URL of package i; 1 when there is none, -1 on a bad argument. */
static cell AMX_NATIVE_CALL n_sysUpdateTMUrl(AMX *amx, cell *params)
{
	cell *cptr;
	int i = params[1];

	if (i < 2 && amx_GetAddr(amx, params[2], &cptr) == AMX_ERR_NONE) {
		if (s_upinfo[i].b.empty())
			return 1;
		amx_SetString(cptr, (char *)s_upinfo[i].b.c_str(), 1);
		return 0;
	}
	return -1;
}

/* sysUpdateTMPath(buf, prefix, path): MakeTmPath on two script strings. */
static cell AMX_NATIVE_CALL n_sysUpdateTMPath(AMX *amx, cell *params)
{
	char out[256];
	char b[256];
	char a[256];
	cell *cptr;

	if (amx_GetAddr(amx, params[3], &cptr) == AMX_ERR_NONE)
		amx_GetString(a, cptr);
	if (amx_GetAddr(amx, params[2], &cptr) == AMX_ERR_NONE)
		amx_GetString(b, cptr);
	MakeTmPath(out, b, a);
	if (amx_GetAddr(amx, params[1], &cptr) == AMX_ERR_NONE)
		amx_SetString(cptr, out, 1);
	return 0;
}

/* sysUpdateTMFinish(): saves the update info when it changed. */
static cell AMX_NATIVE_CALL n_sysUpdateTMFinish(AMX *amx, cell *params)
{
	if (s_tmDone == 0)
		return 0;
	return WriteUpInfoFile();
}

static cell AMX_NATIVE_CALL n_sysUpdateSetup(AMX *amx, cell *params)
{
	NaviMenu::Enable(false);
	return 0;
}

static cell AMX_NATIVE_CALL n_sysUpdateSetFlag(AMX *amx, cell *params)
{
	int fd = open(SYSUPDATE_TMP, O_WRONLY | O_CREAT | O_TRUNC, 0664);

	if (fd < 0)
		return -1;
	close(fd);
	return 0;
}

static cell AMX_NATIVE_CALL n_sysUpdateChkFlag(AMX *amx, cell *params)
{
	struct stat st;

	return lstat(SYSUPDATE_TMP, &st);
}

static cell AMX_NATIVE_CALL n_sysUpdateClrFlag(AMX *amx, cell *params)
{
	return remove(SYSUPDATE_TMP);
}

/* True when [p,q] lies entirely in the script heap or entirely in its stack. */
static inline int amx_addr_ok(AMX *amx, cell p, cell q)
{
	if (p >= 0 && p < amx->hea && q >= 0 && q < amx->hea)
		return 1;
	if (p >= amx->stk && p < amx->stp && q >= amx->stk && q < amx->stp)
		return 1;
	return 0;
}

/*
 * sysUpdateGetUrl(user, password, url): reads and decrypts the configuration
 * file /opt0/bn/sysconf/BNUPDATE. Text without line breaks is the URL alone,
 * otherwise "KEY=value" lines. The three values go to the script buffers.
 */
static cell AMX_NATIVE_CALL n_sysUpdateGetUrl(AMX *amx, cell *params)
{
	static char s_urlBuf[1024];
	cell *cptr;
	unsigned int len;
	int size;
	int fd = open("/opt0/bn/sysconf/BNUPDATE", O_RDONLY);

	if (fd < 0)
		return -1;
	size = lseek(fd, 0, SEEK_END);
	if (size <= 0) {
		close(fd);
		return -2;
	}
	lseek(fd, 0, SEEK_SET);
	if (size >= 1024)
		size = 1023;
	if (read(fd, s_urlBuf, size) != size) {
		close(fd);
		return -3;
	}
	close(fd);
	Script__Abort(size, s_urlBuf, (int *)&len);
	if (len >= 1023)
		return -4;
	s_urlBuf[len] = 0;
	if (HasEol(s_urlBuf)) {
		if (ParseConfText(s_urlBuf) != 0)
			return -5;
	} else {
		s_conf.user.erase();
		s_conf.password.erase();
		s_conf.url = s_urlBuf;
	}
	if (!amx_addr_ok(amx, params[1], strlen(s_conf.user.c_str()) + params[1])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	if (!amx_addr_ok(amx, params[2], strlen(s_conf.password.c_str()) + params[2])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	if (!amx_addr_ok(amx, params[3], strlen(s_conf.url.c_str()) + params[3])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	amx_SetString(cptr, (char *)s_conf.user.c_str(), 1);
	amx_GetAddr(amx, params[2], &cptr);
	amx_SetString(cptr, (char *)s_conf.password.c_str(), 1);
	amx_GetAddr(amx, params[3], &cptr);
	amx_SetString(cptr, (char *)s_conf.url.c_str(), 1);
	return 0;
}

/*
 * sysUpdateChkBnVer(list, buf): looks up the installed BN version (first line
 * of /opt0/bn/sysconf/bnversion.dat) in a "VERSION,value" list read from the
 * PFS partition hdd0:__contents, and returns the value after the comma.
 * Each failure has its own code, -1 to -12.
 */
static cell AMX_NATIVE_CALL n_sysUpdateChkBnVer(AMX *amx, cell *params)
{
	static char s_verBuf[1024];
	static char s_listBuf[32768];
	string path = CellToString(amx, params[1]);
	char *ver, *tok, *p;
	cell *cptr;
	int fd, size, found;

	scepfsInit();
	if (scepfsMount("pfs0:", "hdd0:__contents", 0, 0, 0) < 0) {
		scepfsExit();
		return -1;
	}
	fd = scepfsOpen(path.c_str(), 1);
	if (fd < 0) {
		if (scepfsUmount("pfs0:") < 0) {
			scepfsExit();
			return -2;
		}
		scepfsExit();
		return -3;
	}
	size = scepfsLseek(fd, 0, 2);
	if (size <= 0) {
		scepfsClose(fd);
		scepfsUmount("pfs0:");
		scepfsExit();
		return -4;
	}
	scepfsLseek(fd, 0, 0);
	if (size >= 32768)
		size = 32767;
	if (scepfsRead(fd, s_listBuf, size) < 0) {
		scepfsClose(fd);
		scepfsUmount("pfs0:");
		scepfsExit();
		return -5;
	}
	if (scepfsClose(fd) < 0) {
		if (scepfsUmount("pfs0:") < 0) {
			scepfsExit();
			return -6;
		}
		scepfsExit();
		return -7;
	}
	if (scepfsUmount("pfs0:") < 0) {
		scepfsExit();
		return -8;
	}
	scepfsExit();

	fd = open("/opt0/bn/sysconf/bnversion.dat", O_RDONLY);
	if (fd < 0)
		return -9;
	size = lseek(fd, 0, SEEK_END);
	if (size <= 0) {
		close(fd);
		return -10;
	}
	lseek(fd, 0, SEEK_SET);
	if (size >= 1024)
		size = 1023;
	if (read(fd, s_verBuf, size) != size) {
		close(fd);
		return -11;
	}
	close(fd);

	ver = s_verBuf;
	ver = strsep(&ver, "\n");
	found = 0;
	p = s_listBuf;
	while ((tok = strsep(&p, "\n")) != 0 && strlen(tok) != 0) {
		if (strncmp(ver, tok, strlen(ver)) == 0) {
			strsep(&tok, ",");
			if (tok == 0)
				tok = "";
			found = 1;
			break;
		}
	}
	if (!found)
		return -12;
	if (!amx_addr_ok(amx, params[1], strlen(tok) + params[1])) {
		amx_RaiseError(amx, 5);
		return 0;
	}
	amx_GetAddr(amx, params[1], &cptr);
	amx_SetString(cptr, tok, 1);
	return 0;
}

static cell AMX_NATIVE_CALL n_sysUpdateInit(AMX *amx, cell *params)
{
	BecomeRoot();
	BBFileAccessExitCPP();
	return 0;
}

static cell AMX_NATIVE_CALL n_sysUpdateDlStart(AMX *amx, cell *params)
{
	string user = CellToString(amx, params[1]);
	string password = CellToString(amx, params[2]);
	string uri = CellToString(amx, params[3]);
	string dstPath = CellToString(amx, params[4]);

	return bnDownloadStart((char *)user.c_str(), (char *)password.c_str(), (char *)uri.c_str(),
	    (char *)dstPath.c_str());
}

static cell AMX_NATIVE_CALL n_sysUpdateDlSync(AMX *amx, cell *params)
{
	static int s_dlProgress;
	cell *cptr;
	int r = bnDownloadSync(&s_dlProgress);

	if (amx_GetAddr(amx, params[1], &cptr) == AMX_ERR_NONE)
		*cptr = s_dlProgress;
	return r;
}

static cell AMX_NATIVE_CALL n_sysUpdateDlStop(AMX *amx, cell *params)
{
	bnDownloadCancel();
	return 0;
}

static cell AMX_NATIVE_CALL n_sysUpdateStart(AMX *amx, cell *params)
{
	int ret, r;

	if (scepfsInit() < 0)
		return -1;
	if (sceBNSysupInit(0, 0) < 0)
		return -2;
	ret = sceBNSysupStart(0, 0);
	r = -3;
	if (ret >= 0)
		r = 0;
	return r;
}

/* sysUpdateGetStat(&state, &a, &b, &step): status of the update driver; returns 0. */
static cell AMX_NATIVE_CALL n_sysUpdateGetStat(AMX *amx, cell *params)
{
	static sceBNSysupStatus s_sysupStat;
	cell *cptr;

	sceBNSysupGetStatus(&s_sysupStat);
	if (amx_GetAddr(amx, params[1], &cptr) == AMX_ERR_NONE)
		*cptr = s_sysupStat.state;
	if (amx_GetAddr(amx, params[2], &cptr) == AMX_ERR_NONE)
		*cptr = s_sysupStat.result;
	if (amx_GetAddr(amx, params[3], &cptr) == AMX_ERR_NONE)
		*cptr = s_sysupStat.total_steps;
	if (amx_GetAddr(amx, params[4], &cptr) == AMX_ERR_NONE)
		*cptr = s_sysupStat.step;
	return 0;
}

static cell AMX_NATIVE_CALL n_sysUpdateEnd(AMX *amx, cell *params)
{
	sceBNSysupExit(0, 0);
	scepfsExit();
	return 0;
}

static cell AMX_NATIVE_CALL n_sysUpdateExit(AMX *amx, cell *params)
{
	BBFileAccessInitCPP();
	RestoreUid();
	return 0;
}

/*
 * sysInitBNVersion(file): fills s_bnVersion: slot 0 from
 * /opt0/bn/sysconf/bnversion.dat, slots 1 and 2 from the two lines of file.
 */
static cell AMX_NATIVE_CALL n_sysInitBNVersion(AMX *amx, cell *params)
{
	char buf[256];
	cell *cptr;
	int fd = open("/opt0/bn/sysconf/bnversion.dat", O_RDONLY);

	if (fd >= 0) {
		int n = lseek(fd, 0, SEEK_END);

		if (n > 0) {
			lseek(fd, 0, SEEK_SET);
			if (n >= 256)
				n = 255;
			if (read(fd, buf, n) == n) {
				buf[n] = 0;
				SetBnVersion(0, buf);
			}
		}
		close(fd);
	}
	if (amx_GetAddr(amx, params[1], &cptr) == AMX_ERR_NONE) {
		amx_GetString(buf, cptr);
		ClearUpInfo();
		ReadUpInfoFile(buf);
		SetBnVersion(1, s_upinfo[0].a);
		SetBnVersion(2, s_upinfo[1].a);
	}
	return 0;
}

/* sysGetBNVersion(i, buf): version i, "unknown" when empty; 0 on a bad index. */
static cell AMX_NATIVE_CALL n_sysGetBNVersion(AMX *amx, cell *params)
{
	cell *cptr;

	if (params[1] < 3 && amx_GetAddr(amx, params[2], &cptr) == AMX_ERR_NONE) {
		if (s_bnVersion[params[1]].length() != 0)
			amx_SetString(cptr, (char *)s_bnVersion[params[1]].c_str(), 1);
		else
			amx_SetString(cptr, "unknown", 1);
		return 1;
	}
	return 0;
}

/*
 * sysUpdateGetEura(file, buf, size): reads a file of the PFS partition
 * hdd0:__contents (mounted on pfs0:) into the script buffer, truncated to size
 * cells. Returns 0, or -1..-5 on failure.
 */
static cell AMX_NATIVE_CALL n_sysUpdateGetEura(AMX *amx, cell *params)
{
	char path[256];
	cell *cptr;
	int ret = -1;

	if (amx_GetAddr(amx, params[1], &cptr) == AMX_ERR_NONE) {
		amx_GetString(path, cptr);
		scepfsInit();
		if (scepfsMount("pfs0:", "hdd0:__contents", 0, 0, 0) >= 0) {
			int fd = scepfsOpen(path, 1);

			if (fd >= 0) {
				int len = scepfsLseek(fd, 0, 2);
				int max = params[3] * 4;
				char *buf;

				if (len >= max)
					len = max - 1;
				buf = (char *)malloc(len + 1);
				if (buf != 0) {
					scepfsLseek(fd, 0, 0);
					if (scepfsRead(fd, buf, len) >= 0) {
						buf[len] = 0;
						if (amx_GetAddr(amx, params[2], &cptr) != AMX_ERR_NONE) {
							ret = -5;
						} else {
							amx_SetString(cptr, buf, 1);
							ret = 0;
						}
					} else {
						ret = -4;
					}
					free(buf);
				}
				scepfsClose(fd);
			} else {
				ret = -3;
			}
			scepfsUmount("pfs0:");
		} else {
			ret = -2;
		}
		scepfsExit();
	}
	return ret;
}

/*
 * sysUpdateMkBntmp(): recreates /bntmp (reformats /dev/hda6 as reiserfs and
 * remounts it) as root, with the BB file access stopped.
 */
static cell AMX_NATIVE_CALL n_sysUpdateMkBntmp(AMX *amx, cell *params)
{
	int r;

	BecomeRoot();
	BBFileAccessExitCPP();
	umount("/bntmp");
	r = MkfsBntmp(6, "128M", 0, "", "", 2);
	mount("/dev/hda6", "/bntmp", "reiserfs", 512, 0);
	BBFileAccessInitCPP();
	RestoreUid();
	return r;
}

AMX_NATIVE_INFO smallext_sysup[] = {
	{ "sysUpdateSetFlag", n_sysUpdateSetFlag },
	{ "sysUpdateChkFlag", n_sysUpdateChkFlag },
	{ "sysUpdateClrFlag", n_sysUpdateClrFlag },
	{ "sysUpdateGetUrl", n_sysUpdateGetUrl },
	{ "sysUpdateChkBnVer", n_sysUpdateChkBnVer },
	{ "sysUpdateInit", n_sysUpdateInit },
	{ "sysUpdateDlStart", n_sysUpdateDlStart },
	{ "sysUpdateDlSync", n_sysUpdateDlSync },
	{ "sysUpdateDlStop", n_sysUpdateDlStop },
	{ "sysUpdateStart", n_sysUpdateStart },
	{ "sysUpdateGetStat", n_sysUpdateGetStat },
	{ "sysUpdateEnd", n_sysUpdateEnd },
	{ "sysUpdateExit", n_sysUpdateExit },
	{ "sysUpdateMkBntmp", n_sysUpdateMkBntmp },
	{ "err11012", n_err11012 },
	{ "err11013", n_err11013 },
	{ "sysUpdateSetup", n_sysUpdateSetup },
	{ "sysUpdateTMStart", n_sysUpdateTMStart },
	{ "sysUpdateTMSync", n_sysUpdateTMSync },
	{ "sysUpdateTMStop", n_sysUpdateTMStop },
	{ "sysUpdateTMUrl", n_sysUpdateTMUrl },
	{ "sysUpdateTMPath", n_sysUpdateTMPath },
	{ "sysUpdateTMFinish", n_sysUpdateTMFinish },
	{ "sysInitBNVersion", n_sysInitBNVersion },
	{ "sysGetBNVersion", n_sysGetBNVersion },
	{ "sysUpdateGetEura", n_sysUpdateGetEura },
	{ 0, 0 },
};
