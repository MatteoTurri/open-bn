/*
 * HttpFile: HTTP client of the Broadband Navigator on top of the system sceHTTP
 * library, with authentication, redirects, cookies and a disk cache.
 */

#ifndef HTTPFILE_H
#define HTTPFILE_H

#include <stdio.h>
#include <string>
#include <pthread.h>
#include "http/scehttp.h"
#include "app/bnstring.h"

/* The errors of HttpFile itself (the library ones are SCE_HTTP_ERR_*). */
#define HTTPFILE_ERR_NOMEM (-10000)    /* out of memory */
#define HTTPFILE_ERR_AUTH (-10002)     /* the authentication challenge cannot be parsed */
#define HTTPFILE_ERR_NO_CACHE (-10003) /* 304 Not Modified, but the cached file is missing */
#define HTTPFILE_ERR_LENGTH (-10004)   /* the body is not as long as its Content-Length */
#define HTTPFILE_ERR_TIMEOUT (-10006)  /* aborted after the timeout of the request */

struct user_pass {
	char *user;
	char *pass;
};

struct sceHttpFileAuthList {
	/* 0x00 */ int res;  /* semaphore guarding the entry */
	/* 0x04 */ sceHttpFileAuthList *next;
	/* 0x08 */ sceHttpFileAuthList *prev;
	/* 0x0C */ int ref_count;
	/* 0x10 */ int last_used;
	/* 0x14 */ int pad_14;
	/* 0x18 */ int type;  /* 0 = basic (user_pass *), 1 = digest (sceHTTPDigest *) */
	/* 0x1C */ int proxy;  /* credentials of the proxy, not of the server */
	/* 0x20 */ char *extra;  /* freed with free() */
	/* 0x24 */ void *data;  /* user_pass * or sceHTTPDigest *, by type */
};

struct sceHttpFileRedList {
	/* 0x00 */ int res;  /* semaphore guarding the entry */
	/* 0x04 */ sceHttpFileRedList *next;
	/* 0x08 */ sceHttpFileRedList *prev;
	/* 0x0C */ int ref_count;
	/* 0x10 */ int last_used;
	/* 0x14 */ int pad_14;
	/* 0x18 */ char *from;  /* unparsed URI that was redirected */
	/* 0x1C */ sceHTTPParsedURI *to;  /* where it was redirected */
};

/* Head of an intrusive list kept as a class static (auth_header_list, red_list). */
template <class T> struct HttpListHead {
	/* 0x00 */ int count;
	/* 0x04 */ int notify;  /* semaphore guarding the list */
	/* 0x08 */ T *head;
	/* 0x0C */ T *tail;
};

/* Optional arguments of HttpFile::Request. */
struct sceHttpFileRequestArg {
	/* 0x00 */ int no_accept;  /* > 0 sends SetOption(20, 4) instead of the Accept headers */
	/* 0x04 */ int *done_flag;  /* if set, receives 1 when cookies are present, else 0 */
	/* 0x08 */ char *user;  /* credentials for add_auth_list */
	/* 0x0C */ char *pass;
	/* 0x10 */ int progress_cb;  /* user progress callback */
	/* 0x14 */ char *content_type;  /* Content-Type header of a POST */
	/* 0x18 */ char *ct_buf;  /* output buffer for the response Content-Type */
	/* 0x1C */ unsigned int ct_len;  /* its size */
	/* 0x20 */ int unk_20;  /* read by the content downloader after a timeout */
	/* 0x24 */ int post_flag;  /* fifth argument of SetOption(6, ...) */
	/* 0x28 */ long long resume_pos;  /* set by the content downloader */
	/* 0x30 */ long long unk_30;
	/* 0x38 */ int fd;  /* output descriptor (SetOption 17) */
	/* 0x3C */ int write_cb;  /* write callback (SetOption 18) */
	/* 0x40 */ void (*done_cb)(int);  /* called with fd when the transfer ends */
	/* 0x44 */ int keep_resp;  /* > 0 keeps the response and copies the two fields below */
	/* 0x48 */ int body;  /* sceHTTPResponse.body */
	/* 0x4C */ int length;  /* low word of the response length */
	/* 0x50 */ int mode;  /* 2 = cache only */
	/* 0x54 */ int pad_54;
	/* 0x58 */ sceHttpFileRequestArg *next;  /* watch-list link */
	/* 0x5C */ pthread_t thread;  /* requesting thread, signalled on timeout */
	/* 0x60 */ int start_tick;  /* tick of the last progress */
	/* 0x64 */ int timeout;  /* in ticks, 0 = none */
}; /* size = 0x68 */

/* Base class of HttpFile: a named file. */
class File {
protected:
	/* 0x00 */ FILE *fd_;  /* 0 when closed */
	/* 0x04 */ String name_;

public:
	File() : fd_(0) {}
	virtual ~File();
	virtual int Open(const String &name);
	virtual void Close();
	virtual int Seek(long off, int whence);
	virtual long Tell();
	virtual int Read(void *buf, unsigned int n);
	virtual int Write(void *buf, unsigned int n);
	virtual const char *GetFileName();
	operator const String();
	operator const char *();
	operator char *();
	String operator=(const String &name);
	static int RmDir(const String &dir, const String &name, int remove_dir);
};

inline String File::operator=(const String &name)
{
	return name_ = name;
}

inline File::operator const String()
{
	return name_;
}

inline File::operator const char *()
{
	return name_.c_str();
}

inline File::operator char *()
{
	return (char *)name_.c_str();
}

class HttpFile : public File {
public:
	static HttpListHead<sceHttpFileAuthList> auth_header_list;
	static HttpListHead<sceHttpFileRedList> red_list;
	static sceHttpFileRequestArg *watch_root;
	static pthread_mutex_t watch_lock;
	static String proxy_uri;
	static sceHTTPCookieList *cookie_list_top;
	static sceHTTPCookieList *cookie_list_tail;
	static int cookie_entry_num;
	static int cookie_list_sema;
	static String cache_path;
	static String user_agent;
	static int rand_prefix_num;
	static int netbusy_count;

	/* fd_ and name_ (+0x00, +0x04) and the vtable pointer (+0x08) come from File. */
	/* 0x0C */ sceHTTPClient *handle;
	/* 0x10 */ sceHTTPAuthList *auth_list;  /* challenges of the last 401/407 reply */
	/* 0x14 */ sceHTTPCookieList *cur_cookie;
	/* 0x18 */ String cache_dir;
	/* 0x1C */ String index_path;
	/* 0x20 */ String lock_path;
	/* 0x24 */ String last_modified;
	/* 0x28 */ String str28;
	/* 0x2C */ String request_uri;

	char *copy_str(const char *s);
	int FlushClassCacheDir();
	int SetClassCacheDir(const char *dir);
	int get_libhttp_error();
	int setUserAgent(const String &agent);
	sceHTTPHeaderList *search_header(sceHTTPHeaderList *list, const char *name);
	int free_user_pass(user_pass *p);
	int copy_user_pass(user_pass *dst, user_pass *src);
	int free_digest(sceHTTPDigest *d);
	int copy_digest(sceHTTPDigest *dst, sceHTTPDigest *src);
	int remove_auth_entry(sceHttpFileAuthList *e);
	int cmp_auth_entry(sceHttpFileAuthList *e, void *query);
	int stale_entry();
	int invalidate_auth_entry(sceHttpFileAuthList *e);
	int invalidate_red_entry(sceHttpFileRedList *e);
	int keep_auth(int type, const char *key, void *cred, int proxy, int mode);
	static sceHTTPCookieList *get_cookies_from_uri(const char *uri);
	int sort_cookie_list();
	static int save_each_cookie(sceHTTPCookieList *list);
	static int overwrite_cookies(sceHTTPCookieList *list);
	static int add_cookie_from_strings(const char *url, const char *cookie);
	int save_cookie();
	int decr_red_ref_count(sceHttpFileRedList *e);
	int decr_auth_ref_count(sceHttpFileAuthList *e);
	int setAuth();
	int add_auth_list(const char *user, const char *pass);
	int remove_redirect(sceHttpFileRedList *e);
	int keep_redirect(sceHTTPParsedURI *from, sceHTTPParsedURI *to);
	sceHTTPParsedURI *get_redirect();
	int set_redirect();
	int verify_auth_info();
	int getAuth(String &user, String &pass);
	int get_content_type(char *buf, unsigned int size);
	int lock_index(const char *path);
	int unlock_index(int fd);
	int search_uri(int fd, const char *uri, char *type_buf, unsigned int type_len, long long *size_out);
	int get_random_filename();
	int get_localfile_name(const char *uri_str, char *type_buf, unsigned int type_len, int force_new);
	int add_index(const char *url_str, const char *type_str, int size);
	int sort_index(const char *path);
	void add_watch_list(sceHttpFileRequestArg *arg);
	void remove_watch_list(sceHttpFileRequestArg *arg);
	static void Sync();
	int Request(const String &uri_str, void *post_data, unsigned int post_len, sceHttpFileRequestArg *arg);
	static String UriToHost(const String &uri);
	unsigned int LimitCache(unsigned int max_entries, unsigned int max_bytes);
	static String CheckRedirectList(const char *uri);

	static int Init(const String &path);
	static int setProxyURI(const String &uri);
	static void Shutdown();

	HttpFile();
	~HttpFile();
};

#endif
