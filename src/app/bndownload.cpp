/*
 * Background download of a content file over HTTP into the __contents HDD
 * partition, with progress, cancellation and content removal.
 */

#include <string>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include "http/httpfile.h"

#include <string.h>
#include "bndownload.h"
#include "lib/scepfsd.h"

/* Debug trace of dlRun, disabled. */
static const int dl_debug = 0;

static long long g_dlTotal;
static long long g_dlDone;
static int g_dlCancel;
static int g_dlStatus;
static char g_dlUser[256];
static char g_dlPassword[256];
static char g_dlUri[2048];
static char g_dlDstPath[2048];
static pthread_t g_dlThread;

/* Wrappers of the scepfs primitives. Each one runs with signal 16 blocked:
   bnDownloadCancel uses that signal to interrupt the download thread. */
static int pfsInit()
{
	sigset_t set;
	sigset_t old;
	int r;

	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(1, &set, &old);
	r = scepfsInit();
	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(2, &set, &old);
	return r;
}

static void pfsExit()
{
	sigset_t set;
	sigset_t old;

	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(1, &set, &old);
	scepfsExit();
	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(2, &set, &old);
}

static int pfsOpen(const char *path, int flags, int mode)
{
	sigset_t set;
	sigset_t old;
	int r;

	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(1, &set, &old);
	r = scepfsOpen(path, flags, mode);
	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(2, &set, &old);
	return r;
}

static int pfsLseek(int fd, int off, int whence)
{
	sigset_t set;
	sigset_t old;
	int r;

	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(1, &set, &old);
	r = scepfsLseek(fd, off, whence);
	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(2, &set, &old);
	return r;
}

static int pfsWrite(int fd, const void *buf, int n)
{
	sigset_t set;
	sigset_t old;
	int r;

	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(1, &set, &old);
	r = scepfsWrite(fd, buf, n);
	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(2, &set, &old);
	return r;
}

static int pfsClose(int fd)
{
	sigset_t set;
	sigset_t old;
	int r;

	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(1, &set, &old);
	r = scepfsClose(fd);
	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(2, &set, &old);
	return r;
}

static int pfsMkdir(const char *path, int mode)
{
	sigset_t set;
	sigset_t old;
	int r;

	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(1, &set, &old);
	r = scepfsMkdir(path, mode);
	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(2, &set, &old);
	return r;
}

static int cdRemoveTree(char *p)
{
	sigset_t set;
	sigset_t old;
	int r;

	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(1, &set, &old);
	r = scepfsRemove(p);
	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(2, &set, &old);
	return r;
}

static int cdMount(char *a, char *b, int c, void *d, unsigned int e)
{
	sigset_t set;
	sigset_t old;
	int r;

	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(1, &set, &old);
	r = scepfsMount(a, b, c, d, e);
	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(2, &set, &old);
	return r;
}

static int cdUnmount(char *a)
{
	sigset_t set;
	sigset_t old;
	int r;

	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(1, &set, &old);
	r = scepfsUmount(a);
	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(2, &set, &old);
	return r;
}

static int pfsWriteFwd(int fd, const void *buf, int n)
{
	return pfsWrite(fd, buf, n);
}

static int pfsRewind(int fd)
{
	pfsLseek(fd, 0, 0);
	return 0;
}

/* Progress callback. g_dlTotal is -1 until the content length is known. */
static void dlProgress(sceHTTPClient *h, long long done)
{
	if (g_dlTotal == -1)
		g_dlTotal = sceHTTPGetContentLength(h);
	g_dlDone = done;
}

static const char *g_htmlType = "text/html";

/* One request: mounts the contents partition, creates the directory and
   downloads `uri` into the pfs0: file `dst`. A 200 answer of type text/html
   is a server error page and counts as 404. */
static int dlRequest(HttpFile *f, char *user, char *pass, void *uri, void *dst)
{
	char ctype[128];
	sceHttpFileRequestArg req;
	int fd;
	int rc;
	int r;

	g_dlDone = 0;
	g_dlTotal = -1;
	if (cdMount("pfs0:", "hdd0:__contents", 0, 0, 0) < 0)
		return -1;
	pfsMkdir("pfs0:/dlcontents/BN", 0777);
	fd = pfsOpen((char *)dst, 0x602, 0666);
	if (fd < 0) {
		if (cdUnmount("pfs0:") < 0)
			return -3;
		return -4;
	}

	memset(&req, 0, sizeof(req));
	strcpy(ctype, "binary/pscontent");
	req.no_accept = 1;
	req.progress_cb = (int)dlProgress;
	req.ct_buf = ctype;
	req.ct_len = sizeof(ctype);
	req.user = user;
	req.pass = pass;
	req.fd = fd;
	req.write_cb = (int)pfsWriteFwd;
	req.done_cb = (void (*)(int))pfsRewind;

	rc = f->Request(string((char *)uri), 0, 0, &req);
	if (rc == 200 && strncmp(ctype, g_htmlType, strlen(g_htmlType)) == 0)
		rc = 404;

	if (pfsClose(fd) < 0) {
		if (cdUnmount("pfs0:") < 0)
			return -5;
		return -6;
	}
	/* -1014: the write callback failed. On any failure the partial file is
		 * removed. */
	if (rc == 200) {
		r = cdUnmount("pfs0:");
		if (r < 0)
			r = -7;
		if (r)
			return r;
		return rc;
	} else if (rc == -1014) {
		cdRemoveTree((char *)dst);
		r = cdUnmount("pfs0:");
		if (r < 0)
			r = -7;
		if (r)
			return r;
		return rc;
	} else {
		cdRemoveTree((char *)dst);
		r = cdUnmount("pfs0:");
		if (r < 0)
			r = -7;
		if (r)
			return r;
		return rc;
	}
}

/* Runs the request up to three times: a 401 answer is retried with the
   credentials in g_dlUser/g_dlPassword, any other answer ends the loop. */
static int dlRun(void *uri, void *dst)
{
	HttpFile *f;
	char *user;
	char *pass;
	int rc;
	int i;

	rc = 0;
	f = new HttpFile();
	user = 0;
	pass = 0;
	pfsInit();
	i = 0;
	do {
		if (i >= 3)
			break;
		rc = dlRequest(f, user, pass, uri, dst);
		if (rc == 200)
			break;
		if (rc != 401)
			break;
		i++;
		user = g_dlUser;
		pass = g_dlPassword;
	} while (1);
	if (dl_debug) {
		char who[32];
		strcpy(who, user ? user : "");
		puts(who);
	}
	pfsExit();
	if (f)
		delete f;
	return rc;
}

static void *bnDownloadThreadEntry(void *arg)
{
	(void)arg;
	g_dlStatus = dlRun(g_dlUri, g_dlDstPath);
	return 0;
}

/* Copies the four string parameters into static buffers and starts the
   download thread (detached). Returns -2/-4 if user/password is too long,
   -5/-7 if uri/dstPath is empty, -6/-8 if uri/dstPath is too long, otherwise
   the result of pthread_create. */
int bnDownloadStart(char *user, char *password, char *uri, char *dstPath)
{
	pthread_attr_t attr;

	g_dlCancel = 0;
	g_dlStatus = 0;

	if (strlen(user) >= 256)
		return -2;
	memset(g_dlUser, 0, 256);
	memmove(g_dlUser, user, strlen(user));

	if (strlen(password) >= 256)
		return -4;
	memset(g_dlPassword, 0, 256);
	memmove(g_dlPassword, password, strlen(password));

	if (strlen(uri) == 0)
		return -5;
	if (strlen(uri) >= 2048)
		return -6;
	memset(g_dlUri, 0, 2048);
	memmove(g_dlUri, uri, strlen(uri));

	if (strlen(dstPath) == 0)
		return -7;
	if (strlen(dstPath) >= 2048)
		return -8;
	memset(g_dlDstPath, 0, 2048);
	memmove(g_dlDstPath, dstPath, strlen(dstPath));

	g_dlDone = 0;
	g_dlTotal = -1;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, 1);
	return pthread_create(&g_dlThread, &attr, bnDownloadThreadEntry, 0);
}

/* Stores the progress percentage in *out (0 while the total is unknown)
   and returns the download status. */
int bnDownloadSync(int *out)
{
	if (g_dlTotal == -1)
		*out = 0;
	else if (g_dlTotal > 0)
		*out = g_dlDone * 100 / g_dlTotal;
	else
		*out = 0;
	return g_dlStatus;
}

/* Interrupts the download thread with signal 16, once. */
void bnDownloadCancel()
{
	if (g_dlCancel == 0) {
		pthread_kill(g_dlThread, 16);
		g_dlCancel = 1;
	}
}

/* Removes the downloaded content files from the contents partition.
   Returns the unmount result, -4 if it failed. */
int bnContentDelete()
{
	int ret;

	pfsInit();
	if (cdMount("pfs0:", "hdd0:__contents", 0, 0, 0) < 0) {
		pfsExit();
		return -1;
	}
	cdRemoveTree("pfs0:/dlcontents/BN/content.dat");
	cdRemoveTree("pfs0:/dlcontents/BN/bnversion.svr");
	ret = cdUnmount("pfs0:");
	if (ret < 0)
		ret = -4;
	pfsExit();
	return ret;
}
