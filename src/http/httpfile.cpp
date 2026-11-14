/*
 * HttpFile and its File base class: requests, authentication and redirect
 * caches, cookies and the on-disk cache index.
 */

#include "httpfile.h"
#include <sys/stat.h>
#include <iostream.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include "thread/scesema.h"
#include "natives/timeextension.h"

static int write_all(int fd, const char *buf, int len);
static int copy_fd(int dst, int src, int len);

File::~File()
{
	Close();
}

/*
 * Recursively deletes dir/name: a directory is emptied first and removed only if
 * remove_dir > 0, a regular file is removed, anything else is left alone.
 */
int File::RmDir(const String &dir, const String &name, int remove_dir)
{
	struct stat st;
	String path;

	if (dir.length() != 0 && dir[dir.length() - 1] != '/')
		path = dir + "/" + name;
	else
		path = dir + name;

	if (stat(path.c_str(), &st) < 0)
		return -1;

	if ((st.st_mode & S_IFMT) == S_IFDIR) {
		DIR *dir = opendir(path.c_str());
		if (dir == 0)
			return -1;
		struct dirent *ent;
		while ((ent = readdir(dir)) != 0) {
			if (ent->d_ino == 0)
				continue;
			if (strcmp(".", ent->d_name) == 0)
				continue;
			if (strcmp("..", ent->d_name) == 0)
				continue;
			RmDir(path.c_str(), ent->d_name, 1);
		}
		closedir(dir);
		if (remove_dir > 0)
			remove(path.c_str());
	} else if ((st.st_mode & S_IFMT) == S_IFREG) {
		remove(path.c_str());
	}
	return 0;
}

int File::Open(const String &mode)
{
	String tmp = name_;  /* unused, but part of the layout */
	Close();
	fd_ = fopen(name_.c_str(), mode.c_str());
	return fd_ == 0;
}

void File::Close()
{
	if (fd_) {
		fclose(fd_);
		fd_ = 0;
	}
}

int File::Seek(long offset, int whence)
{
	return fseek(fd_, offset, whence) != 0;
}

long File::Tell()
{
	return ftell(fd_);
}

int File::Read(void *buf, unsigned int size)
{
	return fread(buf, 1, size, fd_);
}

int File::Write(void *buf, unsigned int size)
{
	return fwrite(buf, 1, size, fd_);
}

const char *File::GetFileName()
{
	return name_.c_str();
}

HttpListHead<sceHttpFileAuthList> HttpFile::auth_header_list;
HttpListHead<sceHttpFileRedList> HttpFile::red_list;
sceHttpFileRequestArg *HttpFile::watch_root;
pthread_mutex_t HttpFile::watch_lock;
String HttpFile::proxy_uri;
sceHTTPCookieList *HttpFile::cookie_list_top;
sceHTTPCookieList *HttpFile::cookie_list_tail;
int HttpFile::cookie_entry_num;
int HttpFile::cookie_list_sema;
String HttpFile::cache_path;
String HttpFile::user_agent;
int HttpFile::rand_prefix_num;
int HttpFile::netbusy_count = 0;
int big_sema;

/* Returns a NULL-terminated array of "path/name" for the entries of a certificate directory. */
static char **sceHTTPSGetCertHandle(const char *path)
{
	DIR *dir;
	struct dirent *ent;
	char **list, **p;
	int len, n;

	list = 0;
	dir = opendir(path);
	if (dir != 0) {
		list = (char **)malloc(256 * sizeof(char *));
		if (list != 0) {
			len = strlen(path);
			n = 256;
			p = list;
			while ((ent = readdir(dir)) != 0 && n > 0) {
				char *name = ent->d_name;
				if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
					continue;
				*p = (char *)malloc(len + strlen(name) + 2);
				if (*p == 0)
					continue;
				strcpy(*p, path);
				strcat(*p, "/");
				strcat(*p, name);
				p++;
				n--;
			}
			*p = 0;
		}
		closedir(dir);
	}
	return list;
}

/* Frees the list returned by sceHTTPSGetCertHandle. */
static void sceHTTPSUseCertHandle(char **list)
{
	char **p;

	if (list == 0)
		return;
	for (p = list; *p; p++)
		free(*p);
	free(list);
}

/* Creates the semaphore of a list and empties it. */
#define HTTP_LIST_INIT(list) do {					\
	SemaParam sema_param;						\
	memset(&sema_param, 0, sizeof(sema_param));			\
	sema_param.initCount = 1;					\
	sema_param.maxCount = 1;					\
	(list).notify = sce_sema_createsema(&sema_param);	\
	(list).head = 0;						\
	(list).tail = 0;						\
} while (0)

/* Creates a binary semaphore. */
#define HTTP_SEMA_CREATE(res) do {					\
	SemaParam sema_param;						\
	memset(&sema_param, 0, sizeof(sema_param));			\
	sema_param.initCount = 1;					\
	sema_param.maxCount = 1;					\
	(res) = sce_sema_createsema(&sema_param);			\
} while (0)

/* Appends e to a list under the list semaphore. */
#define HTTP_LIST_APPEND(list, e) do {					\
	sce_sema_waitsema((list).notify);				\
	(e)->next = 0;							\
	(e)->prev = (list).tail;					\
	(list).tail = (e);						\
	if (!(list).head)						\
		(list).head = (e);					\
	if ((e)->prev)							\
		(e)->prev->next = (e);					\
	(list).count++;							\
	sce_sema_signalsema((list).notify);				\
} while (0)

/* Unlinks e from a list under the list semaphore. */
#define HTTP_LIST_REMOVE(list, e) do {					\
	sce_sema_waitsema((list).notify);				\
	if (!(e)->next)							\
		(list).tail = (e)->prev;				\
	else								\
		(e)->next->prev = (e)->prev;				\
	if (!(e)->prev)							\
		(list).head = (e)->next;				\
	else								\
		(e)->prev->next = (e)->next;				\
	(list).count--;							\
	sce_sema_signalsema((list).notify);				\
} while (0)

/* Takes a reference on an entry under its semaphore. */
#define HTTP_REF_INCR(e) do {						\
	sce_sema_waitsema((e)->res);					\
	(e)->ref_count++;						\
	sce_sema_signalsema((e)->res);					\
} while (0)

/* Drops a reference; dropping the last one of an invalidated entry runs RELEASE. */
#define HTTP_REF_DECR(e, RELEASE) do {					\
	sce_sema_waitsema((e)->res);					\
	if (--(e)->ref_count == 0 && (e)->last_used > 0) {		\
		sce_sema_signalsema((e)->res);				\
		RELEASE;						\
	} else {							\
		sce_sema_signalsema((e)->res);				\
	}								\
} while (0)

/*
 * Initializes the shared HTTP state: cache path, list semaphores, an empty
 * disk cache, the HTTP and HTTPS libraries and the certificates. Returns 1 on
 * failure, 0 otherwise.
 */
int HttpFile::Init(const String &path)
{
	SemaParam sema_param2;

	srand(getpid() & time(0));
	rand_prefix_num = 0;

	cache_path = path;
	if (cache_path[cache_path.length() - 1] != '/')
		cache_path += "/";

	HTTP_LIST_INIT(auth_header_list);

	cookie_list_top = 0;
	cookie_entry_num = 0;

	memset(&sema_param2, 0, sizeof(sema_param2));
	sema_param2.initCount = 1;
	sema_param2.maxCount = 1;
	{
		int rc = sce_sema_createsema(&sema_param2);
		big_sema = rc;
		if (rc < 0)
			return 1;
	}

	memset(&sema_param2, 0, sizeof(sema_param2));
	sema_param2.initCount = 1;
	sema_param2.maxCount = 1;
	{
		int rc = sce_sema_createsema(&sema_param2);
		cookie_list_sema = rc;
		if (rc < 0)
			return 1;
	}

	HTTP_LIST_INIT(red_list);

	pthread_mutex_init(&watch_lock, 0);

	File::RmDir("", cache_path, 0);

	if (sceHTTPInit() < 0)
		return 1;

	{
		char **handle = sceHTTPSGetCertHandle("certs");
		if (sceHTTPSInit(0, handle, 0, 0) < 0)
			return 1;
		sceHTTPSUseCertHandle(handle);
	}
	return 0;
}

void HttpFile::Shutdown()
{
	sce_sema_deletesema(big_sema);
	sce_sema_deletesema(cookie_list_sema);
	File::RmDir("", cache_path, 0);
	sceHTTPSTerminate();
	sceHTTPTerminate();
}

int HttpFile::setProxyURI(const String &uri)
{
	proxy_uri = uri;
	return 0;
}

HttpFile::HttpFile() : auth_list(0)
{
	name_ = "";
	proxy_uri = "";
	cache_dir = "default/";
	handle = (sceHTTPClient *)sceHTTPCreate();
	if (handle) {
		auth_list = 0;  /* redundant as written */
		cur_cookie = 0;
		sceHTTPSetOption(handle, SCE_HTTP_OPT_USER_AGENT, (int)(long)"SCEI PlayStation BB Navigator 1.0");
		sceHTTPSetOption(handle, SCE_HTTP_OPT_HTTP11, 0);
		sceHTTPSetOption(handle, SCE_HTTP_OPT_SSL_FLAGS, 5);
		if (!proxy_uri.empty()) {
			sceHTTPParsedURI *p = sceHTTPParseURI(proxy_uri.c_str(), 96);
			if (p) {
				sceHTTPSetOption(handle, SCE_HTTP_OPT_PROXY, (int)(long)p);
				sceHTTPFreeURI(p);
			}
		}
	}
}

int HttpFile::SetClassCacheDir(const char *dir)
{
	if (dir)
		cache_dir = dir;
	if (cache_dir[cache_dir.length() - 1] != '/')
		cache_dir += "/";
	return 0;
}

int HttpFile::FlushClassCacheDir()
{
	File::RmDir(cache_path, cache_dir, 1);
	return 0;
}

HttpFile::~HttpFile()
{
	if (auth_list)
		sceHTTPFreeAuthList(auth_list);
	if (cur_cookie)
		sceHTTPFreeCookieList(cur_cookie);
	if (handle)
		sceHTTPDestroy(handle);
}

int HttpFile::get_libhttp_error()
{
	int sslerr = SCE_HTTP_ERR_SSL;
	int err = sceHTTPGetClientError(handle);
	if (err == sslerr) {
		err = sceHTTPGetSSLError(handle);
		return err ? err : sslerr;
	}
	if ((unsigned int)(err - SCE_HTTP_ERR_CONNECT) < 2) /* SCE_HTTP_ERR_CONNECT or SCE_HTTP_ERR_SOCKET */
		err = -sceHTTPGetSocketError(handle);
	return err;
}

char *HttpFile::copy_str(const char *s)
{
	char *p = (char *)malloc(strlen(s) + 1);
	if (!p)
		return 0;
	strncpy(p, s, strlen(s) + 1);
	return p;
}

int HttpFile::setUserAgent(const String &agent)
{
	return sceHTTPSetOption(handle, SCE_HTTP_OPT_USER_AGENT, agent.c_str()) < 0;
}

int HttpFile::getAuth(String &user, String &pass)
{
	return 0;
}

sceHTTPHeaderList *HttpFile::search_header(sceHTTPHeaderList *list, const char *name)
{
	while (list && strcmp(list->name, name) != 0)
		list = list->next;
	return list;
}

int HttpFile::free_user_pass(user_pass *p)
{
	if (p->user)
		free(p->user);
	if (p->pass)
		free(p->pass);
	return 0;
}

int HttpFile::copy_user_pass(user_pass *dst, user_pass *src)
{
	dst->user = src->user;
	dst->pass = src->pass;
	if (src->user) {
		char *p = copy_str(src->user);
		dst->user = p;
		if (!p)
			goto fail;
	}
	if (src->pass) {
		char *p = copy_str(src->pass);
		dst->pass = p;
		if (!p)
			goto fail;
	}
	return 0;
fail:
	free_user_pass(dst);
	return HTTPFILE_ERR_NOMEM;
}

int HttpFile::free_digest(sceHTTPDigest *d)
{
	if (d->username)
		free(d->username);
	if (d->realm)
		free(d->realm);
	if (d->password)
		free(d->password);
	if (d->uri)
		free(d->uri);
	if (d->nonce)
		free(d->nonce);
	if (d->cnonce)
		free(d->cnonce);
	if (d->opaque)
		free(d->opaque);
	if (d->entity)
		free(d->entity);
	return 0;
}

int HttpFile::copy_digest(sceHTTPDigest *dst, sceHTTPDigest *src)
{
	*dst = *src;
	if (src->username) {
		char *p = copy_str(src->username);
		dst->username = p;
		if (!p)
			goto fail;
	}
	if (src->realm) {
		char *p = copy_str(src->realm);
		dst->realm = p;
		if (!p)
			goto fail;
	}
	if (src->password) {
		char *p = copy_str(src->password);
		dst->password = p;
		if (!p)
			goto fail;
	}
	if (src->uri) {
		char *p = copy_str(src->uri);
		dst->uri = p;
		if (!p)
			goto fail;
	}
	if (src->nonce) {
		char *p = copy_str(src->nonce);
		dst->nonce = p;
		if (!p)
			goto fail;
	}
	if (src->cnonce) {
		char *p = copy_str(src->cnonce);
		dst->cnonce = p;
		if (!p)
			goto fail;
	}
	if (src->opaque) {
		char *p = copy_str(src->opaque);
		dst->opaque = p;
		if (!p)
			goto fail;
	}
	if (src->entity) {
		char *p = copy_str(src->entity);
		dst->entity = p;
		if (!p)
			goto fail;
	}
	return 0;
fail:
	free_digest(dst);
	return HTTPFILE_ERR_NOMEM;
}

int HttpFile::remove_auth_entry(sceHttpFileAuthList *e)
{
	HTTP_LIST_REMOVE(auth_header_list, e);

	switch (e->type) {
	case 0:
		if (e->data) {
			free_user_pass((user_pass *)e->data);
			free(e->data);
			e->data = 0;
		}
		break;
	case 1:
		if (e->data) {
			free_digest((sceHTTPDigest *)e->data);
			free(e->data);
			e->data = 0;
		}
		break;
	}

	sce_sema_deletesema(e->res);
	if (e->extra)
		free(e->extra);
	if (e)
		free(e);
	return 0;
}

int HttpFile::cmp_auth_entry(sceHttpFileAuthList *e, void *query)
{
	int result = 0;
	switch (e->type) {
	case 0: {
		user_pass *up = (user_pass *)e->data;
		user_pass *q = (user_pass *)query;
		result = strcmp(up->user, q->user) | strcmp(up->pass, q->pass);
		break;
	}
	case 1: {
		sceHTTPDigest *d = (sceHTTPDigest *)e->data;
		sceHTTPDigest *q = (sceHTTPDigest *)query;
		/* NOTE: bug: the digest fields are compared as pointers and the strcmp results are ignored */
		if (d->username != q->username) {
			result = 1;
			break;
		}
		strcmp(d->username, q->username);
		if (d->password != q->password) {
			result = 1;
			break;
		}
		strcmp(d->password, q->password);
		if (d->realm != q->realm) {
			result = 1;
			break;
		}
		strcmp(d->realm, q->realm);
		if (d->uri != q->uri) {
			result = 1;
			break;
		}
		strcmp(d->uri, q->uri);
		if (d->nonce != q->nonce) {
			result = 1;
			break;
		}
		strcmp(d->nonce, q->nonce);
		if (d->cnonce != q->cnonce) {
			result = 1;
			break;
		}
		strcmp(d->cnonce, q->cnonce);
		if (d->opaque != q->opaque) {
			result = 1;
			break;
		}
		strcmp(d->opaque, q->opaque);
		break;
	}
	}
	return result;
}

/*
 * Refreshes the digest entry of the current challenge after a "stale" reply,
 * keyed by the host of the current or the redirect URI.
 */
int HttpFile::stale_entry()
{
	sceHTTPDigest local;
	memset(&local, 0, sizeof(local));

	sceHTTPAuthList *ch = auth_list;
	local.realm = ch->auth.realm;
	local.nonce = ch->auth.nonce;
	local.opaque = ch->auth.opaque;
	local.algorithm = ch->auth.algorithm;
	local.nc = 1;
	local.qop = ch->auth.qop;
	local.method = handle->method;
	local.entity = 0;
	local.entity_len = 0;

	if (ch->auth.proxy) {
		sceHTTPParsedURI *u = handle->proxy_uri;
		keep_auth(1, u->host, &local, ch->auth.proxy, 1);
	} else {
		sceHTTPParsedURI *u = handle->uri;
		keep_auth(1, u->host, &local, 0, 1);
	}

	if (local.username)
		free(local.username);
	if (local.password)
		free(local.password);
	return 0;
}

int HttpFile::invalidate_auth_entry(sceHttpFileAuthList *e)
{
	if (e->ref_count > 0) {
		e->last_used++;
		sce_sema_signalsema(e->res);
	} else {
		sce_sema_signalsema(e->res);
		remove_auth_entry(e);
	}
	return 0;
}

int HttpFile::invalidate_red_entry(sceHttpFileRedList *e)
{
	if (e->ref_count > 0) {
		e->last_used++;
		sce_sema_signalsema(e->res);
	} else {
		HTTP_LIST_REMOVE(red_list, e);
		sceHTTPFreeURI(e->to);
		free(e->from);
		free(e);
	}
	return 0;
}

/*
 * Stores credentials in auth_header_list under `key` (type: 0 = basic, 1 =
 * digest). An entry with the same key is replaced, unless mode == 0 and the
 * credentials are unchanged; with mode > 0 fields 0 and 2 of its data are
 * copied out to cred. The oldest entry is evicted past 200 entries.
 */
int HttpFile::keep_auth(int type, const char *key, void *cred, int proxy, int mode)
{
	int rc;
	sceHttpFileAuthList *p = auth_header_list.head;
	while (p && strcmp(p->extra, key) != 0)
		p = p->next;

	if (p) {
		sce_sema_waitsema(p->res);
		if (!mode && !cmp_auth_entry(p, cred)) {
			sce_sema_signalsema(p->res);
			return 0;
		}
		if (mode > 0) {
			char **out = (char **)cred;
			out[0] = copy_str(((char **)p->data)[0]);
			if (!out[0]) {
				rc = HTTPFILE_ERR_NOMEM;
				goto done;
			}
			out[2] = copy_str(((char **)p->data)[2]);
			if (!out[2]) {
				rc = HTTPFILE_ERR_NOMEM;
				goto done;
			}
		}
		rc = invalidate_auth_entry(p);
		if (rc < 0)
			goto done;
	}

	if (auth_header_list.count >= 200) {
		sce_sema_waitsema(auth_header_list.head->res);
		rc = invalidate_auth_entry(auth_header_list.head);
		if (rc < 0)
			goto done;
	}

	p = (sceHttpFileAuthList *)malloc(sizeof(sceHttpFileAuthList));
	if (!p) {
		rc = HTTPFILE_ERR_NOMEM;
		goto done;
	}
	memset(p, 0, sizeof(sceHttpFileAuthList));
	memset(p, 0, 24);  /* redundant as written */

	HTTP_SEMA_CREATE(p->res);
	p->type = type;
	p->proxy = proxy;

	switch (type) {
	case 0: {
		user_pass *up = (user_pass *)malloc(sizeof(user_pass));
		if (!up) {
			rc = HTTPFILE_ERR_NOMEM;
			goto fail_cleanup;
		}
		memset(up, 0, sizeof(user_pass));
		p->data = up;
		rc = copy_user_pass(up, (user_pass *)cred);
		break;
	}
	case 1: {
		sceHTTPDigest *dg = (sceHTTPDigest *)malloc(sizeof(sceHTTPDigest));
		if (!dg) {
			rc = HTTPFILE_ERR_NOMEM;
			goto fail_cleanup;
		}
		memset(dg, 0, sizeof(sceHTTPDigest));
		p->data = dg;
		rc = copy_digest(dg, (sceHTTPDigest *)cred);
		break;
	}
	default:
		/* NOTE: bug: p and its semaphore are leaked */
		rc = -1;
		goto done;
	}

	if (rc < 0)
		goto fail_cleanup;

	p->extra = copy_str(key);
	if (!p->extra) {
		rc = HTTPFILE_ERR_NOMEM;
		goto fail_cleanup;
	}

	HTTP_LIST_APPEND(auth_header_list, p);
	return 0;

fail_cleanup:
	if (p->extra)
		free(p->extra);
	switch (type) {
	case 0:
		if (p->data) {
			free_user_pass((user_pass *)p->data);
			free(p->data);
		}
		/* NOTE: bug: no break, so case 1 frees p->data a second time */
	case 1:
		if (p->data) {
			free_digest((sceHTTPDigest *)p->data);
			free(p->data);
		}
		break;
	}
	if (p)
		free(p);
done:
	return rc;
}

/*
 * Cookie filter for sceHTTPFilterCookieList, which passes the data part of a
 * list node and the parsed URI: a cookie applies unless it is secure and the
 * scheme is not https, it has expired, or its domain or path do not match.
 */
static int http_cookie_uri_match(sceHTTPCookie *c, sceHTTPParsedURI *uri)
{
	if (c->secure && strcmp(uri->scheme, "https"))
		return 0;
	if (c->expires > 0 && c->expires < time(0))
		return 0;
	if (!uri)
		return 0;
	if (c->domain && uri->host && !sceHTTPLibMatchDomain(uri->host, c->domain))
		return 0;
	if (c->path && uri->path && sceHTTPLibStrsub(uri->path, c->path))
		return 0;
	return 1;
}

sceHTTPCookieList *HttpFile::get_cookies_from_uri(const char *uri)
{
	sceHTTPParsedURI *parsed = sceHTTPParseURI(uri, 96);
	if (parsed) {
		sce_sema_waitsema(cookie_list_sema);
		sceHTTPCookieList *result =
		    sceHTTPFilterCookieList(cookie_list_top, (http_cookie_filter_fn)http_cookie_uri_match, parsed);
		sce_sema_signalsema(cookie_list_sema);
		sceHTTPFreeURI(parsed);
		return result;
	}
	return 0;
}

/* Returns 0 if two cookies have the same first eight fields (name to max_age), 1 otherwise. */
static int cmp_cookie(sceHTTPCookie *a, sceHTTPCookie *b)
{
	if (strcmp(a->name, b->name) || strcmp(a->value, b->value) || strcmp(a->domain, b->domain)
	    || strcmp(a->path, b->path) || a->expires != b->expires || a->secure != b->secure
	    || a->version != b->version || a->max_age != b->max_age)
		return 1;
	return 0;
}

/*
 * Moves to the tail of cookie_list_top every cookie that applies to the
 * current URI.
 */
int HttpFile::sort_cookie_list()
{
	sceHTTPCookieList *list =
	    sceHTTPFilterCookieList(cookie_list_top, (http_cookie_filter_fn)http_cookie_uri_match, handle->uri);
	if (!list)
		return 0;

	sceHTTPCookieList *cur = list;
	do {
		sceHTTPCookieList *node = cookie_list_top;
		while (node) {
			if (!cmp_cookie(&node->cookie, &cur->cookie)) {
				if (!node->prev) {
					cookie_list_top = node->next;
					if (node->next)
						node->next->prev = 0;
				} else {
					node->prev->next = node->next;
					if (node->next)
						node->next->prev = node->prev;
				}
				node->next = 0;
				node->prev = cookie_list_tail;
				cookie_list_tail->next = node;
				cookie_list_tail = node;
				break;
			}
			node = node->next;
		}
		cur = cur->next;
	} while (cur);

	sceHTTPFreeCookieList(list);
	return 0;
}

int HttpFile::save_each_cookie(sceHTTPCookieList *list)
{
	sceHTTPCookieList *last = 0;
	sceHTTPCookieList *p = list;
	if (!p)
		return -1;
	sce_sema_waitsema(cookie_list_sema);
	do {
		if (cookie_entry_num >= 300) {
			cookie_list_top =
			    (sceHTTPCookieList *)sceHTTPDeleteCookieListEntry(cookie_list_top, cookie_list_top);
			cookie_entry_num--;
		}
		void *added = sceHTTPAddCookieList(cookie_list_top, &p->cookie);
		if (!added) {
			fprintf(stderr, "sceHTTPAddCookieList() failed.\n");
		} else {
			sceHTTPCookieList *cur = cookie_list_top;
			int count = 0;
			while (cur) {
				last = cur;
				cur = cur->next;
				count++;
			}
			cookie_entry_num = count;
			if (!cookie_list_top)
				cookie_list_top = (sceHTTPCookieList *)added;
			cookie_list_tail = last;
		}
		p = p->next;
	} while (p);
	sce_sema_signalsema(cookie_list_sema);
	return 0;
}

int HttpFile::overwrite_cookies(sceHTTPCookieList *list)
{
	if (save_each_cookie(list) < 0)
		return -1;
	sceHTTPFreeCookieList(list);
	return 0;
}

int HttpFile::add_cookie_from_strings(const char *url, const char *cookie)
{
	sceHTTPResponse resp;
	sceHTTPClient client;
	memset(&resp, 0, sizeof(resp));

	int result;
	resp.date = time(0);
	resp.headers = sceHTTPAddHeaderList(0, "Set-Cookie", cookie);
	if (!resp.headers) {
		result = -1;
		goto cleanup;
	}

	client.uri = sceHTTPParseURI(url, 32);
	if (!client.uri) {
		result = 1;
		goto cleanup;
	}

	{
		sceHTTPCookieList *parsed = (sceHTTPCookieList *)sceHTTPParseCookie(&client, &resp);
		if (save_each_cookie(parsed) < 0)
			return -1;
		sceHTTPFreeCookieList(parsed);
		result = 0;
	}

cleanup:
	if (client.uri)
		sceHTTPFreeURI(client.uri);
	if (resp.headers)
		sceHTTPFreeHeaderList(resp.headers);
	return result;
}

int HttpFile::save_cookie()
{
	if (save_each_cookie(cur_cookie) < 0)
		return -1;
	sceHTTPFreeCookieList(cur_cookie);
	cur_cookie = 0;
	return 0;
}

int HttpFile::decr_red_ref_count(sceHttpFileRedList *e)
{
	HTTP_REF_DECR(e, {
		sce_sema_deletesema(e->res);
		sceHTTPFreeURI(e->to);
		free(e->from);
		free(e);
	});
	return 0;
}

int HttpFile::decr_auth_ref_count(sceHttpFileAuthList *e)
{
	HTTP_REF_DECR(e, remove_auth_entry(e));
	return 0;
}

/*
 * Applies the stored credentials matching the host: at most one entry for
 * the server and one for the proxy, via the basic or digest calls.
 */
int HttpFile::setAuth()
{
	int found_proxy = 0;
	int found_normal = 0;
	sceHttpFileAuthList *p;

	for (p = auth_header_list.head; p; p = p->next) {
		if (!handle->proxy_uri) {
			if (p->proxy) {
				continue;
			}
			if (strncmp(handle->uri->host, p->extra, strlen(handle->uri->host)) != 0) {
				continue;
			}
		} else if (!p->proxy) {
			if (found_normal) {
				continue;
			}
			if (strncmp(handle->uri->host, p->extra, strlen(handle->uri->host)) != 0) {
				continue;
			}
		} else {
			if (found_proxy) {
				continue;
			}
			if (strncmp(handle->proxy_uri->host, (char *)p->extra, strlen(handle->proxy_uri->host)) != 0) {
				continue;
			}
		}
		if (p->last_used > 0) {
			continue;
		}

		HTTP_REF_INCR(p);

		sceHTTPSetOption(handle, SCE_HTTP_OPT_DEL_HEADER, p->proxy ? "Proxy-Authorization" : "Authorization");

		switch (p->type) {
		case 0: {
			user_pass *up = (user_pass *)p->data;
			sceHTTPSetBasicAuth(handle, up->user, up->pass, p->proxy);
			break;
		}
		case 1: {
			sceHTTPDigest local = *(sceHTTPDigest *)p->data;
			char nonce_buf[16];
			local.uri = handle->uri->path;

			if (local.qop) {
				int sum = time(0) + rand();
				sprintf(nonce_buf, "%08x", sum);
				local.cnonce = nonce_buf;
			} else {
				local.cnonce = 0;
			}
			local.method = handle->method;
			local.entity = 0;
			local.entity_len = 0;

			int rc = sceHTTPSetDigestAuth(handle, &local, p->proxy);
			if (rc < 0)
				cout << "sceHTTPSetDigestAuth() failed." << endl;

			((sceHTTPDigest *)p->data)->nc++;
			break;
		}
		}

		decr_auth_ref_count(p);

		if (!handle->proxy_uri)
			break;

		if (found_proxy > 0 && found_normal > 0)  /* redundant as written */
			break;

		if (!p->proxy)
			found_normal = 1;
		else
			found_proxy = 1;
	}
	return 0;
}

/*
 * Stores `user` and `pass` for the current challenge; returns 1 when
 * there is no challenge.
 */
int HttpFile::add_auth_list(const char *user, const char *pass)
{
	sceHTTPAuthList *ch = auth_list;
	if (!ch)
		return 1;

	if (!ch->auth.type) {
		user_pass local;
		memset(&local, 0, sizeof(local));
		local.user = (char *)user;
		local.pass = (char *)pass;

		sceHTTPAuthList *ch2 = auth_list;
		if (ch2->auth.proxy) {
			sceHTTPParsedURI *u = handle->proxy_uri;
			keep_auth(0, u->host, &local, ch2->auth.proxy, 0);
			return 0;
		} else {
			sceHTTPParsedURI *u = handle->uri;
			keep_auth(0, u->host, &local, 0, 0);
			return 0;
		}
	}

	sceHTTPDigest local;
	memset(&local, 0, sizeof(local));
	local.username = (char *)user;
	local.password = (char *)pass;

	sceHTTPAuthList *ch3 = auth_list;
	local.realm = ch3->auth.realm;
	local.nonce = ch3->auth.nonce;
	local.opaque = ch3->auth.opaque;
	local.algorithm = ch3->auth.algorithm;
	local.nc = 1;
	local.qop = ch3->auth.qop;
	local.method = handle->method;
	local.entity = 0;
	local.entity_len = 0;

	if (ch3->auth.proxy) {
		sceHTTPParsedURI *u = handle->proxy_uri;
		keep_auth(1, u->host, &local, ch3->auth.proxy, 0);
	} else {
		sceHTTPParsedURI *u = handle->uri;
		keep_auth(1, u->host, &local, 0, 0);
	}
	return 0;
}

int HttpFile::remove_redirect(sceHttpFileRedList *e)
{
	HTTP_LIST_REMOVE(red_list, e);

	if (e->from)
		free(e->from);
	if (e->to)
		sceHTTPFreeURI(e->to);
	if (e)
		free(e);
	return 0;
}

/*
 * Records a permanent redirect from `from` to `to` in red_list, replacing an entry for
 * the same URI; the oldest entry is evicted past 200 entries.
 */
int HttpFile::keep_redirect(sceHTTPParsedURI *from, sceHTTPParsedURI *to)
{
	char *unparsed = sceHTTPUnparseURI(from);
	int result;
	sceHttpFileRedList *n;

	if (!unparsed) {
		result = HTTPFILE_ERR_NOMEM;
		goto cleanup;
	}

	n = red_list.head;
	while (n && strcmp(n->from, unparsed) != 0)
		n = n->next;

	if (n) {
		sce_sema_waitsema(n->res);
		result = invalidate_red_entry(n);
		if (result < 0)
			goto cleanup;
	}

	if (red_list.count >= 200) {
		sce_sema_waitsema(red_list.head->res);
		result = invalidate_red_entry(red_list.head);
		if (result < 0)
			goto cleanup;
	}

	n = (sceHttpFileRedList *)malloc(32);
	if (!n) {
		result = HTTPFILE_ERR_NOMEM;
		goto cleanup;
	}
	memset(n, 0, 32);
	memset(n, 0, 24);  /* redundant as written */

	HTTP_SEMA_CREATE(n->res);
	n->from = unparsed;
	unparsed = 0;
	n->to = sceHTTPCloneURI(to);
	if (!n->to) {
		result = HTTPFILE_ERR_NOMEM;
		goto cleanup_n;
	}

	HTTP_LIST_APPEND(red_list, n);
	return 0;

cleanup_n:
	if (n->from)
		free(n->from);
	if (n)
		free(n);
	goto cleanup;

cleanup:
	if (unparsed)
		free(unparsed);
	return result;
}

/* Follows a redirect; returns the parsed target URI on success, else 0. */
sceHTTPParsedURI *HttpFile::get_redirect()
{
	sceHTTPResponse *sub = &handle->response;
	char **loc = sceHTTPParseLocations(sub);
	sceHTTPParsedURI *parsed = 0;
	int rc = 0;
	if (loc) {
		parsed = sceHTTPParseURI(*loc, 96);
		if (parsed) {
			sceHTTPFreeLocations(loc);
			if (handle->response.status_code == 301)
				keep_redirect(handle->uri, parsed);
			int flag = (sub->status_code ^ 0x131) < 1u;
			rc = sceHTTPSetRedirection(handle, parsed, flag);
			if (rc >= 0)
				return parsed;
		}
	}
	fprintf(stderr, "get_redirect: error.\n");
	/* NOTE: bug: loc was already freed if sceHTTPSetRedirection failed */
	if (loc)
		sceHTTPFreeLocations(loc);
	if (parsed)
		sceHTTPFreeURI(parsed);
	return 0;
}

int HttpFile::set_redirect()
{
	char *unparsed = sceHTTPUnparseURI(handle->uri);
	if (!unparsed)
		return HTTPFILE_ERR_NOMEM;
	sceHttpFileRedList *e = red_list.head;
	while (e) {
		if (strcmp(e->from, unparsed) == 0)
			break;
		e = e->next;
	}
	free(unparsed);
	if (!e)
		return 0;
	HTTP_REF_INCR(e);
	sceHTTPParsedURI *clone = (sceHTTPParsedURI *)sceHTTPCloneURI(e->to);
	if (clone) {
		decr_red_ref_count(e);
		sceHTTPSetOption(handle, SCE_HTTP_OPT_URI, (int)(long)clone);
		sceHTTPFreeURI(clone);
		return 0;
	}
	/* NOTE: bug: unparsed was already freed, and the reference on e is never dropped */
	if (unparsed)
		free(unparsed);
	return HTTPFILE_ERR_NOMEM;
}

int HttpFile::verify_auth_info()
{
	return 0;
}

int HttpFile::get_content_type(char *buf, unsigned int size)
{
	sceHTTPResponse *sub = &handle->response;
	int ret = 0;
	if (buf) {
		sceHTTPHeaderList *h = sub->headers;
		while (h) {
			if (strcmp("Content-Type", h->name) == 0) {
				strncpy(buf, h->value, size);
				if (size < strlen(h->value) + 1)
					buf[size - 1] = 0;
				goto out;
			}
			h = h ? h->next : 0;
		}
		ret = -1;
	}
out:
	return ret;
}

int HttpFile::lock_index(const char *path)
{
	String tmp = path;
	int fd = open(tmp.c_str(), O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return fd;
	flock(fd, 2);
	return fd;
}

int HttpFile::unlock_index(int fd)
{
	flock(fd, 8);
	close(fd);
	return 0;
}

/*
 * Splits the tail of an index line at p: "[name] size [rest]\n". The name is
 * optional (absent when the field starts with a digit), so is the rest; the
 * separators become terminators.
 */
#define HF_PARSE_TAIL(p, name, size, rest, FAIL) do {			\
	if ((unsigned)((unsigned char)(p)[0] - '0') < 10u)		\
		(size) = (p);						\
	else {								\
		(name) = (p);						\
		(p) = strchr((name), ' ');				\
		if (!(p))						\
			FAIL;						\
		(size) = (p) + 1;					\
		*(p) = 0;						\
	}								\
	(p) = strchr((size), ' ');					\
	if (!(p)) {							\
		(p) = strchr((size), '\n');				\
		if (!(p))						\
			FAIL;						\
		(rest) = 0;						\
		*(p) = 0;						\
	} else {							\
		*(p) = 0;						\
		(rest) = (p) + 1;					\
		(p) = strchr((rest), '\n');				\
		if (!(p))						\
			FAIL;						\
		*(p) = 0;						\
	}								\
} while (0)

/*
 * Looks up `uri` in the index file open on fd. On a match sets name_ and
 * last_modified, stores the size in *size_out and the content type in type_buf (at most type_len
 * bytes). Returns 1 if found, 0 at end of file, -1 on error.
 */
int HttpFile::search_uri(int fd, const char *uri, char *type_buf, unsigned int type_len, long long *size_out)
{
	char *line = 0;
	FILE *f = fdopen(fd, "r");
	int result;
	char *value;
	char *content_type;
	char *digits;
	char *trailing;

	if (!f) {
		perror("fdopen");
		fprintf(stderr, "fdopen1: ifd = %d\n", fd);
		result = -1;
		goto end;
	}

next_line:
	{
		int cnt = 0;
		char ch;
		for (;;) {
			result = fread(&ch, 1, 1, f);
			if (result < 0) {
				perror("fread");
				result = -1;
				goto end;
			}
			if (result == 0)
				goto end;
			cnt++;
			if (ch == '\n')
				break;
		}

		fseek(f, -cnt, SEEK_CUR);
		int size = cnt + 1;
		line = (char *)malloc(size);
		if (!line) {
			perror("malloc");
			result = -1;
			goto end;
		}
		if (!fgets(line, size, f)) {
			perror("fgets");
			result = -1;
			goto end;
		}

		char *p = strchr(line, ' ');
		if (!p) {
			result = -1;
			goto end;
		}
		*p = 0;
		if (strcmp(line, uri) == 0) {
			value = p + 1;
			p = strchr(value, ' ');
			if (!p) {
				result = -1;
				goto end;
			}
			*p = 0;
			p++;

			content_type = 0;
			HF_PARSE_TAIL(p, content_type, digits, trailing, {
				result = -1;
				goto end;
			});
			goto found;
		}
		free(line);
		line = 0;
	}
	goto next_line;

found:
	name_ = value;
	{
		unsigned long numval = strtoul(digits, 0, 10);
		*size_out = (unsigned int)numval;
	}
	last_modified = trailing ? trailing : "";
	if (type_buf) {
		strncpy(type_buf, content_type, type_len);
		unsigned int len = strlen(content_type);
		if (type_len < len + 1)
			type_buf[type_len - 1] = 0;
	}
	result = 1;

end:
	if (line)
		free(line);
	if (f)
		fclose(f);
	return result;
}

int HttpFile::get_random_filename()
{
	char buf[16];

	int num = rand_prefix_num++;
	if (num == -2) {
		rand_prefix_num = 0;
		num = 0;
	}
	sprintf(buf, "%u", num);

	unsigned int pos = request_uri.rfind('/');
	if (pos != String::npos)
		request_uri.erase(0, pos + 1);
	pos = request_uri.find('?');
	if (pos != String::npos)
		request_uri.erase(pos);
	if (request_uri.length() == 0)
		request_uri = "start.xml";

	if (cache_dir[cache_dir.length() - 1] != '/')
		name_ = cache_path + cache_dir + "/" + buf + request_uri;
	else
		name_ = cache_path + cache_dir + buf + request_uri;
	return 0;
}

/* Creates the directory containing s (s is cut at its last separator c). */
static inline void mkdir_parent(String &s, char c)
{
	unsigned int pos = s.rfind(c);
	if (pos != String::npos) {
		s.erase(pos, s.length() - pos);
		mkdir(s.c_str(), 493);
	}
}

int HttpFile::get_localfile_name(const char *uri_str, char *type_buf, unsigned int type_len, int force_new)
{
	String uri = uri_str;
	String dir;
	String unused; /* unused, but part of the layout */
	struct stat st;

	name_ = uri;

	dir = cache_path + cache_dir;
	mkdir_parent(dir, '/');

	if (cache_dir[cache_dir.length() - 1] != '/') {
		index_path = cache_path + cache_dir + "/" + "index";
		lock_path = cache_path + cache_dir + "/" + "lock";
	} else {
		index_path = cache_path + cache_dir + "index";
		lock_path = cache_path + cache_dir + "lock";
	}

	long long size;
	int failed;
	int lockfd = lock_index(lock_path.c_str());
	if (lockfd < 0) {
		failed = 1;
	} else {
		int fd = open(index_path.c_str(), O_RDONLY, 0644);
		if (fd < 0) {
			failed = 1;
		} else {
			failed = search_uri(fd, uri_str, type_buf, type_len, &size) < 1;
			close(fd);
		}
	}
	if (lockfd > 0)
		unlock_index(lockfd);

	if (!force_new && !failed) {
		if (lstat(name_.c_str(), &st) != 0 || st.st_size <= 0 || (long long)st.st_size != size) {
			last_modified = "";
			get_random_filename();
			goto out;
		}
		if (force_new)  /* redundant as written */
			goto out;
		return 1;
	}
	get_random_filename();
out:
	return 0;
}

static inline void strip_from(String &s, char c)
{
	unsigned int pos = s.find(c);
	if (pos != String::npos)
		s.erase(pos);
}

int HttpFile::add_index(const char *url_str, const char *type_str, int size)
{
	int fd = 0;
	FILE *f = 0;
	int result;

	String url = url_str;
	String type = type_str;
	String line;
	char buf[32];

	sprintf(buf, "%d", size);

	strip_from(type, ';');
	strip_from(type, ' ');
	if (type.length() != 0)
		type = " " + type;
	if (last_modified.length() != 0)
		last_modified = " " + last_modified;

	line = url + " " + name_ + type + " " + buf + last_modified + "\n";

	int lockfd = lock_index(lock_path.c_str());
	if (lockfd < 0) {
		perror("lockfile");
		result = -1;
		goto end;
	}

	fd = open(index_path.c_str(), O_RDWR | O_APPEND | O_CREAT, 0644);
	if (fd < 0) {
		result = -1;
		goto end;
	}

	f = fdopen(fd, "a+");
	if (!f) {
		perror("fdopen");
		fprintf(stderr, "fdopen2: ifd = %d\n", fd);
		result = -1;
		goto end;
	}

	result = fputs(line.c_str(), f);
	if (result == -1) {
		flock(fd, 8);
		perror("fputs");
	}

end:
	if (lockfd > 0)
		unlock_index(lockfd);
	if (f)
		fclose(f);
	if (fd > 0)
		close(fd);
	return result;
}

static int write_all(int fd, const char *buf, int len)
{
	int left = len;

	while (left > 0) {
		/* NOTE: bug: every write is passed len instead of the bytes left */
		int n = write(fd, buf, len);
		if (n < 0)
			return n;
		if (n == 0)
			break;
		left -= n;
		buf += n;
	}
	return len - left;
}

/* Copies src to dst in 2 KB blocks; len == 0 copies up to end of file. */
static int copy_fd(int dst, int src, int len)
{
	char buf[2048];
	int left = len;
	int eof = 0;

	while (eof <= 0 && (len == 0 || left > 0)) {
		int n = sizeof(buf);
		if (left != 0) {
			if (left > sizeof(buf))
				n = sizeof(buf);
			else
				n = left;
		}
		n = read(src, buf, n);
		if (n < 0)
			return n;
		left -= n;
		if (n == 0)
			eof = 1;
		n = write_all(dst, buf, n);
		if (n < 0)
			return n;
	}
	return len - left;
}

/*
 * Moves the index line of (path, name_) to the top of the index file, unless
 * it already is the first. Returns -1 on error.
 */
int HttpFile::sort_index(const char *path)
{
	int fd1 = 0;
	int fd2 = 0;
	FILE *f = 0;
	String bak_name = index_path + ".bak";
	String outline;
	char *line = 0;
	int ret;
	int lockfd;
	int skipped;
	char ch;
	char *pch;
	int cnt;

	lockfd = lock_index(lock_path.c_str());
	if (lockfd < 0) {
		perror("lockfile");
		ret = -1;
		goto cleanup;
	}

	fd1 = open(index_path.c_str(), O_RDWR);
	if (fd1 < 0) {
		ret = -1;
		goto cleanup;
	}

	fd2 = open(bak_name.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0664);
	if (fd2 < 0) {
		perror("index.bak open");
		ret = -1;
		goto cleanup;
	}

	f = fdopen(fd1, "r");
	skipped = 0;
	if (!f) {
		perror("fdopen");
		fprintf(stderr, "fdopen3: ifd = %d\n", fd1);
		ret = -1;
		goto cleanup;
	}

	pch = &ch;
next_line:
	{
		line = 0;
		cnt = 0;
		for (;;) {
			ret = fread(pch, 1, 1, f);
			if (ret < 0)
				goto fread_err;
			if (ret == 0)
				goto cleanup;
			cnt++;
			if (ch == '\n')
				break;
		}

		fseek(f, -cnt, SEEK_CUR);
		int size = cnt + 1;
		line = (char *)malloc(size);
		if (!line) {
			perror("malloc");
			ret = -1;
			goto cleanup;
		}
		if (!fgets(line, size, f)) {
			perror("fgets");
			ret = -1;
			goto cleanup;
		}

		char *sp = strchr(line, ' ');
		if (!sp) {
			ret = -1;
			goto cleanup;
		}
		*sp = 0;
		if (strcmp(line, path) == 0) {
			*sp = ' ';
			outline = line;

			sp = strchr(line, ' ');
			if (!sp) {
				ret = -1;
				goto cleanup;
			}
			char *nm = sp + 1;
			sp = strchr(nm, ' ');
			if (!sp) {
				ret = -1;
				goto cleanup;
			}
			*sp = 0;
			if (strcmp(nm, name_.c_str()) == 0)
				goto found;
		}
		free(line);
		skipped++;
		goto next_line;
	}

found:
	if (skipped == 0) {
		ret = 0;
		goto cleanup;
	}

	fseek(f, -cnt, SEEK_CUR);
	{
		int pos = ftell(f);
		fseek(f, 0, SEEK_SET);
		lseek(fd1, 0, SEEK_SET);
		if (write_all(fd2, outline.c_str(), outline.length()) < 0) {
			ret = -1;
			goto cleanup;
		}
		if (copy_fd(fd2, fd1, pos) < 0) {
			ret = -1;
			goto cleanup;
		}
	}
	lseek(fd1, outline.length(), SEEK_CUR);
	if (copy_fd(fd2, fd1, 0) >= 0)
		goto copy_back;
	ret = -1;
	goto cleanup;

fread_err:
	perror("fread");
	ret = -1;
	goto cleanup;

copy_back:
	lseek(fd1, 0, SEEK_SET);
	lseek(fd2, 0, SEEK_SET);
	ret = copy_fd(fd1, fd2, 0) < 0 ? -1 : 0;
cleanup:
	if (lockfd > 0)
		unlock_index(lockfd);
	if (f)
		fclose(f);
	if (fd1 > 0)
		close(fd1);
	if (fd2 > 0)
		close(fd2);
	if (line)
		free(line);
	return ret;
}

void HttpFile::add_watch_list(sceHttpFileRequestArg *arg)
{
	pthread_mutex_lock(&watch_lock);
	arg->next = watch_root;
	arg->thread = pthread_self();
	arg->start_tick = tickcount();
	watch_root = arg;
	pthread_mutex_unlock(&watch_lock);
}

void HttpFile::remove_watch_list(sceHttpFileRequestArg *arg)
{
	pthread_mutex_lock(&watch_lock);
	sceHttpFileRequestArg *prev = 0;
	sceHttpFileRequestArg *cur = watch_root;
	while (cur) {
		if (cur == arg) {
			if (prev)
				prev->next = cur->next;
			else
				watch_root = cur->next;
			break;
		}
		prev = cur;
		cur = cur->next;
	}
	pthread_mutex_unlock(&watch_lock);
}
/* Write callback (SetOption 18) that discards the data. */
static int discard_write(void *h, const void *buf, int len)
{
	return len;
}

typedef void (*request_progress_fn)(void *h, long long n, sceHttpFileRequestArg *arg);

/*
 * Progress callback (SetOption 15): restarts the timeout, forwards to the
 * user callback and discards the body of a reply other than 200.
 */
static void request_progress_cb(sceHTTPClient *h, long long n, sceHttpFileRequestArg *arg)
{
	arg->start_tick = tickcount();
	if (arg->progress_cb)
		((request_progress_fn)arg->progress_cb)(h, n, arg);
	sceHTTPResponse *resp = sceHTTPGetResponse(h);
	if (n == 0) {
		int ok = (resp->status_code == 200);
		if (!ok)
			sceHTTPSetOption(h, SCE_HTTP_OPT_WRITE_FN, (int)discard_write);
	}
}

void HttpFile::Sync()
{
	if (pthread_mutex_trylock(&watch_lock) < 0)
		return;
	sceHttpFileRequestArg *cur = watch_root;
	while (cur) {
		if (cur->timeout) {
			unsigned int elapsed = (unsigned)tickcount() - (unsigned)cur->start_tick;
			if ((unsigned)cur->timeout < elapsed)
				pthread_kill(cur->thread, SIGUSR1);
		}
		cur = cur->next;
	}
	pthread_mutex_unlock(&watch_lock);
}

/*
 * Performs a request for uri_str as a state machine (st = 1..11, 0 leaves the
 * loop), using the disk cache when possible. Returns an HTTP status or a
 * negative error.
 */
int HttpFile::Request(const String &uri_str, void *post_data, unsigned int post_len, sceHttpFileRequestArg *arg)
{
	char ctbuf[128];
	int auth_failed = 0;
	sceHTTPParsedURI *uri = 0;
	int cached = 0;
	int clen = 0;
	int st;
	int ret = 200;
	sceHTTPResponse *resp;
	sceHTTPHeaderList *h;

	request_uri = uri_str;
	unsigned int pos = uri_str.find(':');
	if (pos == String::npos) {
		name_ = uri_str;
		return 200;
	}
	String scheme(uri_str, 0, pos);
	if (scheme == "file") {
		name_ = uri_str;
		name_.erase(0, 5);
		return 200;
	}
	if (scheme != "http" && scheme != "https")
		return SCE_HTTP_ERR_INVALID;
	if (!arg || (!arg->write_cb && !arg->keep_resp)) {
		last_modified = "";
		if (arg && arg->mode == 2) {
			if (get_localfile_name(uri_str.c_str(), arg ? arg->ct_buf : 0, arg ? arg->ct_len : 0, 0) > 0)
				return 200;
		}
		get_localfile_name(uri_str.c_str(), arg ? arg->ct_buf : 0, arg ? arg->ct_len : 0, 1);
	}
	if (arg->user && arg->pass) {
		sce_sema_waitsema(big_sema);
		add_auth_list(arg->user, arg->pass);
		sce_sema_signalsema(big_sema);
	}
	add_watch_list(arg);

	resp = 0;
	st = 1;
	while (st) {
		switch (st) {
		case 1:
			uri = sceHTTPParseURI(uri_str.c_str(), 96);
			if (!uri) {
				ret = get_libhttp_error();
				st = 8;
				break;
			}
			/* FALLTHROUGH */
		case 2: {
			sceHTTPSetOption(handle, SCE_HTTP_OPT_URI, (int)uri);
			sceHTTPFreeURI(uri);
			uri = 0;
			if (post_data && post_len) {
				sceHTTPSetOption(handle, SCE_HTTP_OPT_METHOD, SCE_HTTP_METHOD_POST);
				if (sceHTTPSetOption(handle, SCE_HTTP_OPT_POST_DATA, post_data, post_len,
				        arg->post_flag)
				    < 0) {
					ret = get_libhttp_error();
					st = 8;
					break;
				}
				/* the Content-Type header is sent only with a body */
				if (arg && arg->content_type) {
					h = (sceHTTPHeaderList *)sceHTTPAddHeaderList(0, "Content-Type",
					    arg->content_type);
					if (!h || sceHTTPSetOption(handle, SCE_HTTP_OPT_HEADERS, (int)h, 2) < 0) {
						sceHTTPFreeHeaderList(h);
						ret = get_libhttp_error();
						st = 8;
						break;
					}
					sceHTTPFreeHeaderList(h);
				}
			} else {
				sceHTTPSetOption(handle, SCE_HTTP_OPT_METHOD, SCE_HTTP_METHOD_GET);
			}
			sce_sema_waitsema(big_sema);
			set_redirect();
			sce_sema_signalsema(big_sema);
			sceHTTPSetOption(handle, SCE_HTTP_OPT_PROGRESS_CB, (int)request_progress_cb, (int)arg);
			if (arg->no_accept > 0) {
				if (sceHTTPSetOption(handle, SCE_HTTP_OPT_STD_HEADERS, 4) < 0) {
					ret = get_libhttp_error();
					st = 8;
					break;
				}
			} else {
				h = (sceHTTPHeaderList *)sceHTTPAddHeaderList(0, "Accept",
				    "text/psxml, binary/psscript, image/png, image/jpeg, */*");
				if (!h || !sceHTTPAddHeaderList(h, "Accept-Charset", "utf-8,*")
				    || !sceHTTPAddHeaderList(h, "Accept-Language", "ja, en")
				    || sceHTTPSetOption(handle, SCE_HTTP_OPT_HEADERS, (int)h, 2) < 0) {
					sceHTTPFreeHeaderList(h);
					ret = get_libhttp_error();
					st = 8;
					break;
				}
				sceHTTPFreeHeaderList(h);
			}
			if (arg && !arg->mode && last_modified.length()) {
				h = (sceHTTPHeaderList *)sceHTTPAddHeaderList(0, "If-Modified-Since",
				    last_modified.c_str());
				if (!h || sceHTTPSetOption(handle, SCE_HTTP_OPT_HEADERS, (int)h, 2) < 0) {
					sceHTTPFreeHeaderList(h);
					ret = get_libhttp_error();
					st = 8;
					break;
				}
				sceHTTPFreeHeaderList(h);
			}
			sce_sema_waitsema(cookie_list_sema);
			sceHTTPSetCookie(handle, cookie_list_top);
			sce_sema_signalsema(cookie_list_sema);
			sce_sema_waitsema(big_sema);
			setAuth();
			sce_sema_signalsema(big_sema);
		}
			/* FALLTHROUGH */
		case 3: {
			if (!arg || (!arg->write_cb && !arg->keep_resp)) {
				if (Open(String("wb")) != 0) {
					ret = -errno;
					st = 0;
					break;
				}
				if (sceHTTPSetOption(handle, SCE_HTTP_OPT_OUT_FD, fd_->_fileno) < 0) {
					ret = get_libhttp_error();
					st = 8;
					break;
				}
				if (sceHTTPSetOption(handle, SCE_HTTP_OPT_WRITE_FN, (int)write) < 0) {
					ret = get_libhttp_error();
					st = 8;
					break;
				}
			} else if (arg->fd >= 0 && arg->write_cb) {
				if (sceHTTPSetOption(handle, SCE_HTTP_OPT_OUT_FD, arg->fd) < 0) {
					ret = get_libhttp_error();
					st = 8;
					break;
				}
				if (sceHTTPSetOption(handle, SCE_HTTP_OPT_WRITE_FN, arg->write_cb) < 0) {
					ret = get_libhttp_error();
					st = 8;
					break;
				}
			} else {
				if (sceHTTPSetOption(handle, SCE_HTTP_OPT_OUT_FD, -1) < 0) {
					ret = get_libhttp_error();
					st = 8;
					break;
				}
			}
		}
			/* FALLTHROUGH */
		case 4:
			sce_sema_waitsema(big_sema);
			netbusy_count++;
			sce_sema_signalsema(big_sema);
			if (sceHTTPOpen(handle) < 0) {
				ret = get_libhttp_error();
				if (ret == SCE_HTTP_ERR_ABORTED && arg && arg->timeout
				    && (unsigned int)arg->timeout < (unsigned int)(tickcount() - arg->start_tick))
					ret = HTTPFILE_ERR_TIMEOUT;
				sceHTTPClose(handle);
				st = 8;
				sce_sema_waitsema(big_sema);
				netbusy_count--;
				sce_sema_signalsema(big_sema);
				break;
			} else {
				if (sceHTTPRequest(handle) < 0) {
					ret = get_libhttp_error();
					if (ret == SCE_HTTP_ERR_WRITE) {
						if (!arg || (!arg->write_cb && !arg->keep_resp))
							ret = -errno;
					} else if (ret == SCE_HTTP_ERR_ABORTED && arg && arg->timeout
					    && (unsigned int)arg->timeout
					        < (unsigned int)(tickcount() - arg->start_tick))
						ret = HTTPFILE_ERR_TIMEOUT;
					st = 8;
				} else {
					st = 5;
				}
				sceHTTPClose(handle);
			}
			sce_sema_waitsema(big_sema);
			netbusy_count--;
			sce_sema_signalsema(big_sema);
			break;
		case 5:
			resp = sceHTTPGetResponse(handle);
			clen = handle->response.content_length;
			if (resp->status_code == 200 && clen > 0 && (long long)clen != resp->length) {
				ret = HTTPFILE_ERR_LENGTH;
				st = 8;
				break;
			}
			ret = resp->status_code;
			switch (ret) {
			case 401:
			case 407:
				st = 6;
				break;
			case 301:
			case 302:
			case 303:
			case 307:
				auth_failed = 0;
				st = 11;
				break;
			case 200:
			default:
				auth_failed = 0;
				cached = (ret == 200) ? 1 : cached;
				st = 7;
				if (!arg || (!arg->write_cb && !arg->keep_resp)) {
					get_content_type(ctbuf, 128);
					if (arg->ct_buf)
						strncpy(arg->ct_buf, ctbuf, arg->ct_len);
					clen = handle->response.content_length;
				} else if (arg->ct_buf) {
					get_content_type(arg->ct_buf, arg->ct_len);
				}
				break;
			}
			break;
		case 6: {
			sceHTTPAuthList *ai;
			if (auth_list)
				sceHTTPFreeAuthList(auth_list);
			ai = (sceHTTPAuthList *)sceHTTPParseAuth(resp);
			auth_list = ai;
			if (!ai) {
				ret = HTTPFILE_ERR_AUTH;
				st = 8;
				break;
			}
			if (ai->auth.stale == 1) {
				auth_failed = 0;
				sce_sema_waitsema(big_sema);
				stale_entry();
				sce_sema_signalsema(big_sema);
				st = 9;
			} else {
				auth_failed = 1;
				st = 8;
			}
		} break;
		case 11: {
			sceHTTPParsedURI *r;
			sce_sema_waitsema(big_sema);
			r = get_redirect();
			sce_sema_signalsema(big_sema);
			if (!r) {
				st = 8;
			} else {
				uri = r;
				st = 9;
			}
		} break;
		case 7:
		case 8:
		case 9:
			if (!auth_failed && auth_list) {
				sceHTTPFreeAuthList(auth_list);
				auth_list = 0;
			}
			if (resp) {
				if (cached > 0) {
					if (!arg || (!arg->write_cb && !arg->keep_resp)) {
						last_modified = "";
						for (h = resp->headers; h; h = h->next) {
							if (strcmp("Last-Modified", h->name) == 0) {
								last_modified = h->value;
								break;
							}
						}
					}
					if (cur_cookie)
						sceHTTPFreeCookieList(cur_cookie);
					cur_cookie = sceHTTPParseCookie(handle, resp);
					if (cur_cookie) {
						if (arg && arg->done_flag)
							*arg->done_flag = 1;
					} else {
						if (arg && arg->done_flag)
							*arg->done_flag = 0;
					}
					verify_auth_info();
				} else {
					if (cur_cookie)
						sceHTTPFreeCookieList(cur_cookie);
					cur_cookie = sceHTTPParseCookie(handle, resp);
					if (cur_cookie)
						save_cookie();
				}
				sceHTTPSetOption(handle, SCE_HTTP_OPT_HEADERS, 0, 1);
				if (arg && arg->keep_resp > 0) {
					arg->body = (int)resp->body;
					arg->length = (int)resp->length;
				} else {
					sceHTTPCleanUpResponse(handle);
				}
			}
			if (st == 9) {
				if (!arg || (!arg->write_cb && !arg->keep_resp))
					Seek(0, 0);
				if (arg->fd >= 0 && arg->done_cb)
					arg->done_cb(arg->fd);
				if (!cached)
					remove(name_.c_str());
				st = 2;
				break;
			}
			/* FALLTHROUGH */
		case 10:
			if (!arg || (!arg->write_cb && !arg->keep_resp)) {
				if (cached > 0)
					add_index(uri_str.c_str(), ctbuf, clen);
				else
					remove(name_.c_str());
				Close();
				if (ret == 304)
					ret = get_localfile_name(uri_str.c_str(), arg ? arg->ct_buf : 0,
					          arg ? arg->ct_len : 0, 0)
					        < 0
					    ? HTTPFILE_ERR_NO_CACHE
					    : 200;
				if (ret == 200)
					sort_index(uri_str.c_str());
			}
			st = 0;
			break;
		}
	}
	remove_watch_list(arg);
	return ret;
}

String HttpFile::UriToHost(const String &uri)
{
	sceHTTPParsedURI *parsed = sceHTTPParseURI(uri.c_str(), 0);
	String host = parsed->host;
	sceHTTPFreeURI(parsed);
	return host;
}

/*
 * Trims the cache index to max_entries entries and max_bytes bytes, removing the cached files
 * past either limit.
 */
unsigned int HttpFile::LimitCache(unsigned int max_entries, unsigned int max_bytes)
{
	FILE *f = 0;
	unsigned int nline = 0;
	unsigned int nbyte = 0;
	int src = 0;
	int dst = 0;
	char ch;
	char *line;
	int lockfd;
	String whole_line;

	String dir = cache_path + cache_dir;
	mkdir_parent(dir, '/');

	if (cache_dir[cache_dir.length() - 1] != '/') {
		index_path = cache_path + cache_dir + "/" + "index";
		lock_path = cache_path + cache_dir + "/" + "lock";
	} else {
		index_path = cache_path + cache_dir + "index";
		lock_path = cache_path + cache_dir + "lock";
	}

	String bak = index_path + ".tmp";

	lockfd = lock_index(lock_path.c_str());
	if (lockfd < 0) {
		perror("lockfile");
		goto out;
	}
	src = open(index_path.c_str(), O_RDWR);
	if (src < 0)
		goto out;
	dst = open(bak.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0664);
	if (dst < 0) {
		perror("index.tmp open");
		goto out;
	}
	f = fdopen(src, "r");
	line = 0;
	if (!f) {
		perror("fdopen");
		fprintf(stderr, "fdopen1: src_fd = %d\n", src);
		goto end;
	}

next_line:
	{
		line = 0;
		int cnt = 0;
		for (;;) {
			int nread = fread(&ch, 1, 1, f);
			if (nread < 0) {
				perror("fread");
				goto end;
			}
			if (nread == 0)
				goto end;
			cnt++;
			if (ch == '\n')
				break;
		}

		fseek(f, -cnt, SEEK_CUR);
		int size = cnt + 1;
		line = (char *)malloc(size);
		if (!line) {
			perror("malloc");
			goto end;
		}
		if (!fgets(line, size, f)) {
			perror("fgets");
			goto end;
		}

		char *p = strchr(line, ' ');
		if (!p)
			goto end;
		whole_line = line;
		char *value = p + 1;
		*p = 0;

		p = strchr(value, ' ');
		if (!p)
			goto end;
		*p = 0;
		p++;

		char *digits;
		char *name, *rest;
		HF_PARSE_TAIL(p, name, digits, rest, goto end);

		nline++;
		nbyte += strtoul(digits, 0, 10);

		if (max_entries < nline || max_bytes < nbyte)
			remove(value);
		else if (write_all(dst, whole_line.c_str(), whole_line.length()) < 0)
			goto end;

		free(line);
	}
	goto next_line;

end:
	if (line)
		free(line);
out:
	if (f)
		fclose(f);
	if (src > 0)
		close(src);
	if (dst > 0)
		close(dst);
	remove(index_path.c_str());
	rename(bak.c_str(), index_path.c_str());
	if (lockfd > 0)
		unlock_index(lockfd);
}

/*
 * Follows the chain of permanent redirects recorded in red_list from uri and
 * returns the final URI.
 */
String HttpFile::CheckRedirectList(const char *uri)
{
	String tmp = uri;

	sceHTTPParsedURI *parsed = sceHTTPParseURI(uri, 96);
	if (parsed) {
		char *unparsed = sceHTTPUnparseURI(parsed);
		if (unparsed) {
			tmp = unparsed;
			free(unparsed);
		}
		sceHTTPFreeURI(parsed);
	}

	sceHttpFileRedList *found = 0;
	sce_sema_waitsema(big_sema);
	int matched;
	do {
		matched = 0;
		sceHttpFileRedList *e = red_list.head;
		while (e) {
			if (strcmp(e->from, tmp.c_str()) == 0) {
				char *unparsed = sceHTTPUnparseURI(e->to);
				if (unparsed) {
					tmp = unparsed;
					free(unparsed);
				}
				found = e;
				matched = 1;
				break;
			}
			e = e->next;
		}
	} while (matched);
	sce_sema_signalsema(big_sema);

	if (!found)
		return uri;
	return tmp;
}
