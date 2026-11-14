/*
 * The sceHTTP client library: the client (one connection and its request), the
 * response, parsed URIs, header lists, cookies and authentication, and the
 * functions of the scehttp*.c files.
 */

#ifndef SCEHTTP_H
#define SCEHTTP_H

#include <pthread.h>

struct sceHTTPClient;

typedef int (*http_request_cb)(struct sceHTTPClient *conn, int result, int arg);
typedef void (*http_progress_cb)(struct sceHTTPClient *conn, long long bytes, int arg);
typedef void (*http_chunk_cb)(struct sceHTTPClient *conn, char *data, int len, int arg);
typedef int (*http_write_fn)(int fd, const void *buf, int len);

/* Node of a doubly linked list of header (name, value) pairs. */
struct sceHTTPHeaderList {
	/* 0x00 */ struct sceHTTPHeaderList *next;
	/* 0x04 */ struct sceHTTPHeaderList *prev;
	/* 0x08 */ char *name;
	/* 0x0C */ char *value;
}; /* size = 0x10 */

/* A parsed URI; every member but port is a malloc'd string or NULL. */
struct sceHTTPParsedURI {
	/* 0x00 */ char *scheme;
	/* 0x04 */ char *user;
	/* 0x08 */ char *password;
	/* 0x0C */ char *host;
	/* 0x10 */ int port;
	/* 0x14 */ char *path;
	/* 0x18 */ char *query;
}; /* size = 0x1C */

/* The response of the last request, with its transfer counters. */
struct sceHTTPResponse {
	/* 0x00 */ int version_major;  /* from "HTTP/%d.%d" */
	/* 0x04 */ int version_minor;
	/* 0x08 */ unsigned int status_code;
	/* 0x0C */ char *reason;  /* reason phrase of the status line */
	/* 0x10 */ int https;  /* 1 = the status line said HTTPS/ instead of HTTP/ */
	/* 0x14 */ struct sceHTTPHeaderList *headers;
	/* 0x18 */ char *body;  /* body in memory */
	char pad_1C[0x20 - 0x1C];
	/* 0x20 */ long long length;  /* bytes of the body received */
	/* 0x28 */ int timed_out;  /* 1 = the body transfer timed out */
	char pad_2C[0x30 - 0x2C];
	/* 0x30 */ long long date;  /* time() of the response */
	/* 0x38 */ long long content_length;  /* -1 = unknown */
}; /* size = 0x40 */

/* A connection and the request made on it (sceHTTPCreate). */
struct sceHTTPClient {
	/* 0x00 */ char *user_agent;  /* option 0 */
	/* 0x04 */ int state;  /* 1 after sceHTTPCreate */
	/* 0x08 */ int http11;  /* 1 = HTTP/1.1, 0 = HTTP/1.0 */
	/* 0x0C */ int timeout;  /* seconds, -1 = none */
	/* 0x10 */ int transfer_timeout;  /* seconds for the body transfer, -1 = none */
	/* 0x14 */ long start_time;  /* time() at the start of the operation */
	/* 0x18 */ int https;  /* 1 = HTTPS connection */
	/* 0x1C */ int sock_state;  /* 2 = connecting, 5 = closing, 1 = closed */
	/* 0x20 */ int client_error;  /* last library error (sceHTTPGetClientError) */
	/* 0x24 */ int socket_error;  /* last socket error */
	/* 0x28 */ int resolve_error;  /* last resolver error */
	/* 0x2C */ int redirect_flag;  /* redirection enabled */
	/* 0x30 */ int ka_timeout;  /* Keep-Alive timeout=, -1 = default */
	/* 0x34 */ int ka_max;  /* Keep-Alive max= */
	/* 0x38 */ int unk_38;  /* option 9, stored negated */
	/* 0x3C */ int abort_flag;  /* 1 = aborted */
	/* 0x40 */ int unk_40;  /* option 21, -1 by default */
	char pad_44[0x54 - 0x44];
	/* 0x54 */ int thread_stack;  /* option 12: stack size of an internal thread */
	/* 0x58 */ int thread_prio;  /* option 12: its priority */
	/* 0x5C */ int request_id;  /* thread doing the request, -1 = none */
	char pad_60[0x68 - 0x60];
	/* 0x68 */ http_request_cb request_cb;  /* called with the result of sceHTTPRequest */
	/* 0x6C */ int request_cb_arg;
	char pad_70[0x74 - 0x70];
	/* 0x74 */ int request_busy;  /* 1 = request in progress */
	/* 0x78 */ http_progress_cb progress_cb;  /* called with the total bytes transferred */
	/* 0x7C */ int progress_arg;
	char pad_80[0x84 - 0x80];
	/* 0x84 */ int block_size;  /* bytes per send/recv and per progress call, 1024 by default */
	/* 0x88 */ unsigned int sent;  /* bytes of the body sent so far */
	/* 0x8C */ int max_body;  /* bytes, -1 = no limit */
	/* 0x90 */ struct sceHTTPParsedURI *proxy_uri;  /* 0 = no proxy */
	/* 0x94 */ int thread_stack2;  /* option 12, as thread_stack */
	/* 0x98 */ int thread_prio2;
	char pad_9C[0xA4 - 0x9C];
	/* 0xA4 */ int unk_A4;  /* option 23 */
	/* 0xA8 */ int unk_A8;
	char pad_AC[0xB0 - 0xAC];
	/* 0xB0 */ int unk_B0;  /* option 22, stored negated */
	/* 0xB4 */ int method;  /* index in g_http_methods (3 = POST, 4 = PUT) */
	/* 0xB8 */ struct sceHTTPParsedURI *uri;  /* request URI */
	/* 0xBC */ struct sceHTTPHeaderList *headers;  /* request headers */
	/* 0xC0 */ void *post_data;  /* request body; not freed if post_flags & 2 */
	/* 0xC4 */ int post_len;
	/* 0xC8 */ int post_flags;  /* 1 = URL-escape the data, 2 = data owned by the caller */
	/* 0xCC */ int out_fd;  /* fd the body is written to, -1 = keep it in memory */
	/* 0xD0 */ http_write_fn write_fn;  /* writes to out_fd, write() by default */
	char pad_D4[0xD8 - 0xD4];
	/* 0xD8 */ int std_headers;  /* standard headers to add, 15 by default */
	char pad_DC[0xE0 - 0xDC];
	/* 0xE0 */ struct sceHTTPResponse response;  /* response of the last request */
	/* 0x120 */ http_chunk_cb chunk_cb;  /* called for each chunk of a chunked body */
	/* 0x124 */ int chunk_arg;
	char pad_128[0x13C - 0x128];
	/* 0x13C */ int sock;  /* -1 = not open */
	char pad_140[0x160 - 0x140];
	/* 0x160 */ int sock_flags;  /* 0x08 = I/O aborted, 0x10 = the server closes the connection */
	char pad_164[0x168 - 0x164];
	/* 0x168 */ int ssl_errnum;
	/* 0x16C */ void *ssl_ctx;  /* SSL_CTX * */
	/* 0x170 */ void *ssl;  /* SSL * */
	/* 0x174 */ int
	    ssl_flags; /* 0x01 = verify the peer, 0x04 = check the host name, 0x18 = check the dates, 0x40 = verbose */
}; /* size = 0x178 */

/* A cookie, as passed to the sceHTTPFilterCookieList callback. */
struct sceHTTPCookie {
	/* 0x00 */ char *name;
	/* 0x04 */ char *value;
	/* 0x08 */ char *domain;
	/* 0x0C */ char *path;
	/* 0x10 */ long long expires; /* time, -1 = end of session */
	/* 0x18 */ int secure;
	/* 0x1C */ int version; /* 1 = RFC 2109 */
	/* 0x20 */ long long max_age; /* seconds, -1 = none */
	/* 0x28 */ char *comment;
	/* 0x2C */ int set_cookie2; /* 1 = from a Set-Cookie2 header */
	/* 0x30 */ int discard;
	/* 0x34 */ char *comment_url;
	/* 0x38 */ char *port; /* port list */
}; /* size = 0x40 */

/* Node of a cookie list. */
struct sceHTTPCookieList {
	/* 0x00 */ struct sceHTTPCookieList *next;
	/* 0x04 */ struct sceHTTPCookieList *prev;
	/* 0x08 */ struct sceHTTPCookie cookie;
}; /* size = 0x48 */

/* An authentication challenge (WWW-Authenticate or Proxy-Authenticate). */
struct sceHTTPAuth {
	/* 0x00 */ int type; /* 0 = Basic, 1 = Digest */
	/* 0x04 */ char *realm;
	/* 0x08 */ void **domain; /* NULL-terminated array */
	/* 0x0C */ char *reserved0c;
	/* 0x10 */ char *nonce;
	/* 0x14 */ char *opaque;
	/* 0x18 */ int stale; /* 1 = true, 2 = false */
	/* 0x1C */ int algorithm; /* 1 = MD5, 2 = MD5-sess */
	/* 0x20 */ int qop; /* bit 1 = auth, bit 2 = auth-int */
	/* 0x24 */ int proxy; /* from a Proxy-Authenticate header */
}; /* size = 0x28 */

/* Node of the challenge list returned by sceHTTPParseAuth. */
struct sceHTTPAuthList {
	/* 0x00 */ struct sceHTTPAuthList *next;
	/* 0x04 */ struct sceHTTPAuthList *prev;
	/* 0x08 */ struct sceHTTPAuth auth;
	/* 0x30 */ char reserved30[0x48 - 0x30];
}; /* size = 0x48 */

/* Digest credentials and request data, for sceHTTPSetDigestAuth. */
struct sceHTTPDigest {
	/* 0x00 */ char *username;
	/* 0x04 */ char *realm;
	/* 0x08 */ char *password;
	/* 0x0C */ char *uri;
	/* 0x10 */ char *nonce;
	/* 0x14 */ char *cnonce;
	/* 0x18 */ char *opaque;
	/* 0x1C */ int algorithm; /* 0 = none, 1 = MD5, 2 = MD5-sess */
	/* 0x20 */ int nc;
	/* 0x24 */ int qop; /* 0 = none, 1 = auth, 2 = auth-int */
	/* 0x28 */ unsigned int method; /* request method, index in the method names */
	/* 0x2C */ char *entity; /* request body, hashed for auth-int */
	/* 0x30 */ unsigned int entity_len;
}; /* size = 0x34 */

/* A parsed Authentication-Info (or Proxy-Authenticate-Info) header. */
struct sceHTTPAuthInfo {
	/* 0x00 */ char *nextnonce;
	/* 0x04 */ char *rspauth;
	/* 0x08 */ char *cnonce;
	/* 0x0C */ int nc;
	/* 0x10 */ int qop;
	/* 0x14 */ int proxy;
}; /* size = 0x18 */

/* Filter of sceHTTPFilterCookieList: called with the cookie data and ctx,
   non-zero keeps the cookie. */
typedef int (*http_cookie_filter_fn)(struct sceHTTPCookie *cookie, void *ctx);

/* A PEM certificate or key in memory, for sceHTTPSInitCertFromMemory. */
typedef struct HttpsMemCert {
	const char *data;
	unsigned int len;
} HttpsMemCert;

#ifdef __cplusplus
extern "C" {
#endif

/* The request methods (sceHTTPClient.method, SCE_HTTP_OPT_METHOD). */
enum {
	SCE_HTTP_METHOD_OPTIONS,
	SCE_HTTP_METHOD_GET,
	SCE_HTTP_METHOD_HEAD,
	SCE_HTTP_METHOD_POST,
	SCE_HTTP_METHOD_PUT,
	SCE_HTTP_METHOD_DELETE,
	SCE_HTTP_METHOD_TRACE,
	SCE_HTTP_METHOD_CONNECT,
	SCE_HTTP_METHOD_PROPFIND,
	SCE_HTTP_METHOD_PROPPATCH,
	SCE_HTTP_METHOD_MKCOL,
	SCE_HTTP_METHOD_COPY,
	SCE_HTTP_METHOD_MOVE,
	SCE_HTTP_METHOD_LOCK,
	SCE_HTTP_METHOD_UNLOCK
};

/* The options of sceHTTPSetOption and sceHTTPGetOption (9 and 21 to 23 set unnamed fields). */
enum {
	SCE_HTTP_OPT_USER_AGENT = 0, /* char * */
	SCE_HTTP_OPT_HTTP11 = 1, /* int: HTTP/1.1 */
	SCE_HTTP_OPT_METHOD = 2, /* int: request method */
	SCE_HTTP_OPT_URI = 3, /* the request URI */
	SCE_HTTP_OPT_PROXY = 4, /* the proxy URI */
	SCE_HTTP_OPT_HEADERS = 5, /* request headers */
	SCE_HTTP_OPT_POST_DATA = 6, /* POST data and length */
	SCE_HTTP_OPT_TIMEOUT = 7, /* int */
	SCE_HTTP_OPT_TRANSFER_TIMEOUT = 8, /* int */
	SCE_HTTP_OPT_REQUEST_CB = 10, /* callback and argument */
	SCE_HTTP_OPT_CHUNK_CB = 11, /* callback and argument */
	SCE_HTTP_OPT_THREAD = 12, /* stack size and priority of the threads */
	SCE_HTTP_OPT_KEEP_ALIVE = 13, /* keep-alive timeout and maximum (get only) */
	SCE_HTTP_OPT_SSL_FLAGS = 14, /* int */
	SCE_HTTP_OPT_PROGRESS_CB = 15, /* callback and argument */
	SCE_HTTP_OPT_BLOCK_SIZE = 16, /* int, at least 512 */
	SCE_HTTP_OPT_OUT_FD = 17, /* int: the body goes to this file */
	SCE_HTTP_OPT_WRITE_FN = 18, /* the write function of the body */
	SCE_HTTP_OPT_DEL_HEADER = 19, /* name of a header to delete (set only) */
	SCE_HTTP_OPT_STD_HEADERS = 20 /* int */
};

/* The library errors of a connection (client_error, sceHTTPGetClientError). */
#define SCE_HTTP_ERR_THREAD (-1001) /* a thread could not be created or signalled */
#define SCE_HTTP_ERR_NOMEM (-1002) /* out of memory */
#define SCE_HTTP_ERR_IO (-1003) /* a send or receive failed */
#define SCE_HTTP_ERR_INVALID (-1004) /* invalid argument or state */
#define SCE_HTTP_ERR_TIMEOUT (-1005) /* the request timed out */
#define SCE_HTTP_ERR_RESOLVE (-1006) /* the host name was not resolved */
#define SCE_HTTP_ERR_SOCKET (-1007) /* the socket could not be created */
#define SCE_HTTP_ERR_CONNECT (-1008) /* the connection failed */
#define SCE_HTTP_ERR_SSL (-1009) /* the SSL session could not be opened (sceHTTPGetSSLError) */
#define SCE_HTTP_ERR_ABORTED (-1011) /* the request was aborted */
#define SCE_HTTP_ERR_RESPONSE (-1012) /* the response is not HTTP */
#define SCE_HTTP_ERR_BUSY (-1013) /* a request is already in progress */
#define SCE_HTTP_ERR_WRITE (-1014) /* the body could not be written */
#define SCE_HTTP_ERR_LENGTH (-1015) /* the body is shorter or longer than its length */

/* Sets the library error code of the connection. */
#define HTTP_SET_ERROR(c, e) do { (c)->client_error = (e); } while (0)

/* scehttp.c: client and request */
int sceHTTPInit(void);
int sceHTTPTerminate(void);
int sceHTTPSetDefaultThreadValue(int stack_size, int prio);
struct sceHTTPClient *sceHTTPCreate(void);
int sceHTTPDestroy(struct sceHTTPClient *conn);
int sceHTTPRequest(struct sceHTTPClient *conn);
int sceHTTPCleanUpResponse(struct sceHTTPClient *conn);
struct sceHTTPResponse *sceHTTPGetResponse(struct sceHTTPClient *conn);
int sceHTTPGetClientError(struct sceHTTPClient *conn);
long long sceHTTPGetTransferedBytes(struct sceHTTPClient *conn);
long long sceHTTPGetPostedBytes(struct sceHTTPClient *conn);
long long sceHTTPGetContentLength(struct sceHTTPClient *conn);
char *sceHTTPLibSkipToken(char *p);
char *sceHTTPLibSkipWS(char *p);

/* scehttp_ver.c */
extern char *sceHTTPLibVersion; /* sent in the User-Agent header */
char *sceHTTPGetLibVersion(void);

/* scehttp_mem.c: memory */
extern void *(*sceHTTPLibMallocFunction)(unsigned int size);
extern void *(*sceHTTPLibReallocFunction)(void *p, unsigned int size);
extern void (*sceHTTPLibFreeFunction)(void *p);
int sceHTTPInitMemory(unsigned int pool, unsigned int size);
int sceHTTPLibTerminateMemory(void);
int sceHTTPGetMemoryStatus(unsigned int *out_used, unsigned int *out_total);
void *sceHTTPLibGetMemoryFunctionRA(void);
void sceHTTPFreeMemory(void *p);
void *sceHTTPSetMallocFunction(void *(*fn)(unsigned int));
void *sceHTTPSetReallocFunction(void *(*fn)(void *, unsigned int));
void *sceHTTPSetFreeFunction(void *(*fn)(void *));
int sceHTTPSSetDefaultOption(struct sceHTTPClient *conn);
void *xcalloc(unsigned int n, unsigned int size);
void *xmalloc(unsigned int size);
void xfree(void *p);
void *xrealloc(void *p, unsigned int size);

/* scehttpio.c: connection I/O */
extern pthread_key_t timer_key;
extern pthread_key_t abort_key;
int sceHTTPOpen(struct sceHTTPClient *conn);
int sceHTTPClose(struct sceHTTPClient *conn);
int sceHTTPAbort(struct sceHTTPClient *conn);
int sceHTTPGetSocketError(struct sceHTTPClient *conn);
int sceHTTPGetResolveError(struct sceHTTPClient *conn);
int sceHTTPLibRecv(struct sceHTTPClient *conn, void *buf, unsigned int len, int timeout, int flags);
int sceHTTPLibSend(struct sceHTTPClient *conn, const void *buf, unsigned int len, int timeout, int flags);
int sceHTTPLibAbortIO(struct sceHTTPClient *conn);
int sceHTTPLibInitIO(void);
int sceHTTPLibTerminateIO(void);

/* scehttps.c: HTTPS on SSL-C */
int sceHTTPSInit(int mode, char **certfiles, char *cafile, char *capath);
int sceHTTPSInitCertFromMemory(int mode, int ncert, HttpsMemCert *certs, HttpsMemCert *ca, HttpsMemCert *key);
int sceHTTPSTerminate(void);
int sceHTTPSClearErrnum(struct sceHTTPClient *c);
int sceHTTPSOpen(struct sceHTTPClient *c);
int sceHTTPSClose(struct sceHTTPClient *c);
int sceHTTPSSend(struct sceHTTPClient *c, const void *buf, int len);
int sceHTTPSRecv(struct sceHTTPClient *c, void *buf, unsigned int len);
int sceHTTPGetSSLError(struct sceHTTPClient *c);

/* scehttpopt.c: options */
int sceHTTPSetOption(struct sceHTTPClient *conn, int option, ...);
int sceHTTPGetOption(struct sceHTTPClient *conn, int option, ...);

/* scehttpuri.c: URIs */
struct sceHTTPParsedURI *sceHTTPParseURI(const char *uri, int flags);
struct sceHTTPParsedURI *sceHTTPCloneURI(struct sceHTTPParsedURI *uri);
int sceHTTPFreeURI(struct sceHTTPParsedURI *uri);
char *sceHTTPUnparseURI(struct sceHTTPParsedURI *uri);
int sceHTTPIsAbsoluteURI(const char *uri);
char *sceHTTPFindAbsoluteURI(char *uri, char *base);

/* scehttpheader.c: header lists */
struct sceHTTPHeaderList *sceHTTPAddHeaderList(struct sceHTTPHeaderList *list, const char *name, const char *value);
int sceHTTPFreeHeaderList(struct sceHTTPHeaderList *list);
struct sceHTTPHeaderList *sceHTTPLibCloneHeaderList(struct sceHTTPHeaderList *list);
struct sceHTTPHeaderList *sceHTTPLibCatHeaderList(struct sceHTTPHeaderList *list1, struct sceHTTPHeaderList *list2);
struct sceHTTPHeaderList *sceHTTPLibDelHeaderByName(struct sceHTTPHeaderList *list, const char *name);

/* scehttpredirect.c: redirection */
char **sceHTTPParseLocations(struct sceHTTPResponse *resp);
int sceHTTPFreeLocations(char **loc);
int sceHTTPSetRedirection(struct sceHTTPClient *conn, struct sceHTTPParsedURI *uri, int flag);

/* scehttpcookie.c: cookies */
struct sceHTTPCookieList *sceHTTPParseCookie(struct sceHTTPClient *conn, struct sceHTTPResponse *resp);
int sceHTTPSetCookie(struct sceHTTPClient *conn, struct sceHTTPCookieList *list);
int sceHTTPFreeCookieList(struct sceHTTPCookieList *list);
struct sceHTTPCookieList *sceHTTPAddCookieList(struct sceHTTPCookieList *list, struct sceHTTPCookie *cookie);
struct sceHTTPCookieList *sceHTTPDeleteCookieListEntry(struct sceHTTPCookieList *list, struct sceHTTPCookieList *entry);
struct sceHTTPCookieList *sceHTTPFilterCookieList(struct sceHTTPCookieList *list, http_cookie_filter_fn callback,
    void *ctx);
int sceHTTPLibMatchDomain(const char *domain, const char *pattern);

/* scehttpauth.c: authentication */
int sceHTTPSetBasicAuth(struct sceHTTPClient *conn, char *user, char *pass, int proxy);
int sceHTTPSetDigestAuth(struct sceHTTPClient *conn, struct sceHTTPDigest *au, int proxy);
struct sceHTTPAuthList *sceHTTPParseAuth(struct sceHTTPResponse *resp);
int sceHTTPFreeAuthList(struct sceHTTPAuthList *list);
struct sceHTTPAuthInfo *sceHTTPParseAuthInfo(struct sceHTTPResponse *resp);
int sceHTTPVerifyAuthInfo(struct sceHTTPClient *conn, struct sceHTTPAuthInfo *info, struct sceHTTPDigest *au,
    int flags);
int sceHTTPFreeAuthInfo(struct sceHTTPAuthInfo *info);

/* scehttpdate.c: HTTP dates */
int sceHTTPLibDateToTime(const char *str, int len, long long *out);
char *sceHTTPLibTimeToDate(long long t);

/* scehttpstr.c: strings */
char *sceHTTPLibStrsave(const char *s);
char *sceHTTPLibStrnsave(const char *s, unsigned int n);
char *sceHTTPLibStrcatsave(char *s0, const char *s1);
char *sceHTTPLibVStrcat(char *s, ...);
int sceHTTPLibStrcasecmp(const char *s1, const char *s2);
int sceHTTPLibStrcasesub(const char *s1, const char *s2);
int sceHTTPLibStrsub(const char *s1, const char *s2);
long long sceHTTPLibStrtodi(const char *p, const char **end);

#ifdef __cplusplus
}
#endif

#endif
