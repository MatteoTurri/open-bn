/*
 * HTTP client library: connections, request building and sending, response
 * parsing (status line, headers, Keep-Alive, Content-Length and chunked
 * bodies), redirects, and the library tables and settings.
 */

#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "http/scehttp.h"

/* Library ID string, never read. */
static char https_libid[16] = "PsIIlibhttps....";

/* (stack size, priority) of the three internal threads. */
int sceHTTPLibIDV[6] = { 0x1000, 63, 0x2000, 63, 0x1000, 63 };

/* HTTP methods { index, name, flags }; sceHTTPClient.method indexes this
   table. */
struct HttpMethod {
	int id;
	const char *name;
	int flag;
};

static struct HttpMethod g_http_methods[15] = {
	{ SCE_HTTP_METHOD_OPTIONS, "OPTIONS", 2 },
	{ SCE_HTTP_METHOD_GET, "GET", 3 },
	{ SCE_HTTP_METHOD_HEAD, "HEAD", 3 },
	{ SCE_HTTP_METHOD_POST, "POST", 0 },
	{ SCE_HTTP_METHOD_PUT, "PUT", 2 },
	{ SCE_HTTP_METHOD_DELETE, "DELETE", 2 },
	{ SCE_HTTP_METHOD_TRACE, "TRACE", 2 },
	{ SCE_HTTP_METHOD_CONNECT, "CONNECT", 0 },
	{ SCE_HTTP_METHOD_PROPFIND, "PROPFIND", 0 },
	{ SCE_HTTP_METHOD_PROPPATCH, "PROPPATCH", 0 },
	{ SCE_HTTP_METHOD_MKCOL, "MKCOL", 0 },
	{ SCE_HTTP_METHOD_COPY, "COPY", 0 },
	{ SCE_HTTP_METHOD_MOVE, "MOVE", 0 },
	{ SCE_HTTP_METHOD_LOCK, "LOCK", 0 },
	{ SCE_HTTP_METHOD_UNLOCK, "UNLOCK", 0 },
};

/* Byte classes of RFC 2616: bit 0x1 = separator or CTL, bit 0x2 = token. */
/* clang-format off */
static unsigned char g_http_char_class[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 1, 2, 2, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
/* clang-format on */

/* Line terminator, used by the request builder and the line parser. */
static const char http_crlf[] = { '\r', '\n', '\0' };

/* NTSC line frequency in Hz; not used. */
int sceHTTPLibHSyncFreq = 15734;

/* Records the start time of the operation. */
static long http_time_mark(struct sceHTTPClient *conn)
{
	return time(&conn->start_time);
}

/* Returns the seconds since http_time_mark. */
static long http_time_elapsed(struct sceHTTPClient *conn)
{
	long now;

	time(&now);
	return now - conn->start_time;
}

static int http_timer_or_abort(struct sceHTTPClient *conn)
{
	int t = (int)pthread_getspecific(timer_key);

	return t | (int)pthread_getspecific(abort_key);
}

static int http_abort_pending(struct sceHTTPClient *conn)
{
	return (int)pthread_getspecific(abort_key);
}

/* Returns 1 if p is a non-empty RFC 2616 token. */
static int http_is_token(const char *p)
{
	char c;

	if (p == 0)
		return 0;
	c = *p;
	if (c == 0)
		return 0;
	while (c != 0) {
		if (!(g_http_char_class[c] & 2))
			return 0;
		p++;
		c = *p;
	}
	return 1;
}
char *sceHTTPLibSkipToken(char *p)
{
	char c;

	if (p == 0)
		return 0;
	c = *p;
	while (c != 0) {
		if (!(g_http_char_class[c] & 2))
			return p;
		p++;
		c = *p;
	}
	return p;
}

char *sceHTTPLibSkipWS(char *p)
{
	char *q = p;
	int c;
	int isws;

	for (;;) {
		c = *q;
		if (c == 0)
			break;
		isws = (c == ' ' || c == '\t');
		if (!isws)
			break;
		q++;
	}
	return q;
}

/* Returns the first node named name (case-insensitive), or 0. */
static struct sceHTTPHeaderList *http_find_header(struct sceHTTPHeaderList *list, const char *name)
{
	while (list != 0) {
		if (sceHTTPLibStrcasecmp(list->name, name) == 0)
			break;
		list = list->next;
	}
	return list;
}

struct sceHTTPClient *sceHTTPCreate(void)
{
	struct sceHTTPClient *conn = xcalloc(1, sizeof(struct sceHTTPClient));

	if (conn == 0)
		return 0;
	conn->state = 1;
	conn->http11 = 0;
	conn->unk_40 = -1;
	conn->block_size = 1024;
	conn->timeout = -1;
	conn->std_headers = 15;
	conn->transfer_timeout = -1;
	conn->max_body = -1;
	conn->sock = -1;
	conn->out_fd = -1;
	conn->write_fn = (http_write_fn)write;
	pthread_setspecific(timer_key, 0);
	pthread_setspecific(abort_key, 0);
	sceHTTPSSetDefaultOption(conn);
	return conn;
}

/* Frees the response (header list, buffers) and clears the counters; with
   full also the thread-specific timer/abort values and sent. */
static int http_cleanup_response_impl(struct sceHTTPClient *conn, int full)
{
	conn->response.status_code = 0;
	conn->response.https = 0;
	sceHTTPFreeHeaderList(conn->response.headers);
	conn->response.headers = 0;
	if (conn->response.reason != 0) {
		xfree(conn->response.reason);
		conn->response.reason = 0;
	}
	if (conn->response.body != 0) {
		xfree(conn->response.body);
		conn->response.body = 0;
	}
	conn->response.length = 0;
	conn->response.timed_out = 0;
	conn->response.date = 0;
	if (full != 0) {
		pthread_setspecific(timer_key, 0);
		pthread_setspecific(abort_key, 0);
		conn->sent = 0;
	}
	return 0;
}

int sceHTTPCleanUpResponse(struct sceHTTPClient *conn)
{
	return http_cleanup_response_impl(conn, 1);
}

int sceHTTPDestroy(struct sceHTTPClient *conn)
{
	conn->https = 0;
	conn->sock_state = 0;
	sceHTTPFreeURI(conn->proxy_uri);

	conn->proxy_uri = 0;
	sceHTTPFreeURI(conn->uri);

	conn->uri = 0;
	sceHTTPFreeHeaderList(conn->headers);

	conn->headers = 0;
	if (conn->post_data != 0) {
		if (!(conn->post_flags & 2))
			xfree(conn->post_data);
	}
	conn->post_data = 0;
	conn->post_len = 0;

	sceHTTPCleanUpResponse(conn);
	xfree(conn);
	return 0;
}

/* Adds the standard headers that the request does not have yet (Host,
   Accept, Accept-Encoding, Accept-Charset, Pragma and Cache-Control,
   User-Agent, Content-Type, Content-Length; the Accept* and User-Agent ones
   only if enabled in std_headers) and sets in *mask a bit for each one added.
   Then drops a Keep-Alive "Connection" header left by the last response. */
static int http_save_std_headers(struct sceHTTPClient *conn, unsigned int *mask)
{
	struct sceHTTPHeaderList *node;
	char *ua;
	char *p;
	char buf[16];

	*mask = 0;

	if (http_find_header(conn->headers, "HOST") == 0) {
		node = sceHTTPAddHeaderList(conn->headers, "HOST", conn->uri->host);
		if (node == 0)
			return -1;
		if (conn->headers == 0)
			conn->headers = node;
		*mask |= 0x01;
	}
	if (conn->std_headers & 0x01) {
		if (http_find_header(conn->headers, "Accept") == 0) {
			node = sceHTTPAddHeaderList(conn->headers, "Accept", "*/*;q=0.01");
			if (node == 0)
				return -1;
			if (conn->headers == 0)
				conn->headers = node;
			*mask |= 0x02;
		}
	}
	if (conn->std_headers & 0x08) {
		if (http_find_header(conn->headers, "Accept-Encoding") == 0) {
			node = sceHTTPAddHeaderList(conn->headers, "Accept-Encoding", "");
			if (node == 0)
				return -1;
			if (conn->headers == 0)
				conn->headers = node;
			*mask |= 0x80;
		}
	}
	if (conn->std_headers & 0x02) {
		if (http_find_header(conn->headers, "Accept-Charset") == 0) {
			node = sceHTTPAddHeaderList(conn->headers, "Accept-Charset", "iso-8859-1;q=0.01");
			if (node == 0)
				return -1;
			if (conn->headers == 0)
				conn->headers = node;
			*mask |= 0x04;
		}
	}
	if (conn->redirect_flag != 0) {
		node = sceHTTPAddHeaderList(conn->headers, "Pragma", "no-cache");
		if (node == 0)
			return -1;
		if (conn->headers == 0)
			conn->headers = node;
		node = sceHTTPAddHeaderList(conn->headers, "Cache-Control", "no-cache");
		if (node == 0)
			return -1;
		if (conn->headers == 0)
			conn->headers = node;
		*mask |= 0x08;
	}
	if (conn->std_headers & 0x04) {
		if (http_find_header(conn->headers, "User-Agent") == 0) {
			p = conn->user_agent;
			if (p != 0) {
				p = sceHTTPLibSkipWS(p);
				if (*p != 0) {
					p = sceHTTPLibStrsave(p);
					if (p == 0)
						return -1;
					ua = p;
					goto got_ua;
				}
			}
			ua = sceHTTPLibVStrcat(0, "unknown (sceHTTPSLib-", sceHTTPLibVersion, ")", 0);
got_ua:
			if (ua == 0) {
				HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
				return -1;
			}
			node = sceHTTPAddHeaderList(conn->headers, "User-Agent", ua);
			xfree(ua);
			if (node == 0)
				return -1;
			if (conn->headers == 0)
				conn->headers = node;
			*mask |= 0x10;
		}
	}
	if (conn->method == SCE_HTTP_METHOD_POST || conn->method == SCE_HTTP_METHOD_PUT) {
		if ((conn->post_len > 0 && conn->post_data != 0) || conn->post_len == 0) {
			if (http_find_header(conn->headers, "Content-Type") == 0) {
				if (conn->post_flags & 0x01) {
					node = sceHTTPAddHeaderList(conn->headers, "Content-Type",
					    "application/x-www-form-urlencoded");
					if (node == 0)
						return -1;
					if (conn->headers == 0)
						conn->headers = node;
					*mask |= 0x20;
				}
			}
			if (http_find_header(conn->headers, "Content-Length") == 0) {
				memset(buf, 0, 16);
				sprintf(buf, "%d", conn->post_len);
				node = sceHTTPAddHeaderList(conn->headers, "Content-Length", buf);
				if (node == 0)
					return -1;
				if (conn->headers == 0)
					conn->headers = node;
				*mask |= 0x40;
			}
		}
	}
	if (conn->proxy_uri == 0)
		return 0;
	if (conn->http11 != 0)
		return 0;
	node = http_find_header(conn->headers, "Connection");
	if (node == 0)
		return 0;
	if (sceHTTPLibStrcasecmp(node->value, "Keep-Alive") != 0)
		return 0;
	if (node->prev != 0)
		node->prev->next = node->next;
	else
		conn->headers = node->next;
	if (node->next != 0) {
		node->next->prev = node->prev;
		node->prev = 0;
	} else {
		node->prev = 0;
	}
	node->next = 0;
	sceHTTPFreeHeaderList(node);
	return 0;
}

/* Removes the standard headers selected by mask (the bits of
   http_save_std_headers). */
static void http_del_std_headers(struct sceHTTPClient *conn, unsigned int mask)
{
	if (mask & 0x01)
		conn->headers = sceHTTPLibDelHeaderByName(conn->headers, "HOST");
	if (mask & 0x02)
		conn->headers = sceHTTPLibDelHeaderByName(conn->headers, "Accept");
	if (mask & 0x04)
		conn->headers = sceHTTPLibDelHeaderByName(conn->headers, "Accept-Charset");
	if (mask & 0x80)
		conn->headers = sceHTTPLibDelHeaderByName(conn->headers, "Accept-Encoding");
	if (mask & 0x08) {
		conn->headers = sceHTTPLibDelHeaderByName(conn->headers, "Pragma");
		conn->headers = sceHTTPLibDelHeaderByName(conn->headers, "Cache-Control");
	}
	if (mask & 0x10)
		conn->headers = sceHTTPLibDelHeaderByName(conn->headers, "User-Agent");
	if (mask & 0x20)
		conn->headers = sceHTTPLibDelHeaderByName(conn->headers, "Content-Type");
	if (mask & 0x40)
		conn->headers = sceHTTPLibDelHeaderByName(conn->headers, "Content-Length");
}

/* Builds the request line and all the headers in one string. Through a
   proxy (proxy_uri), except for HTTPS, the absolute URI is sent. */
static char *http_build_request(struct sceHTTPClient *conn)
{
	char *p;
	char *q;
	char *path;
	char *t;
	struct sceHTTPHeaderList *h;

	if (conn->method < 0 || conn->method >= 16) {  /* NOTE: bug: the method table has 15 entries */
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_INVALID);
		return 0;
	}
	p = sceHTTPLibStrsave(g_http_methods[conn->method].name);
	if (p == 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
		return 0;
	}
	if (sceHTTPLibStrcasecmp(conn->uri->scheme, "HTTPS") == 0 || conn->proxy_uri == 0) {
		t = sceHTTPLibStrsave(conn->uri->path);
		if (t == 0) {
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
			return 0;
		}
		if (conn->uri->query != 0) {
			t = sceHTTPLibVStrcat(t, "?", conn->uri->query, 0);
			if (t == 0) {
				HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
				return 0;
			}
			path = t;
		} else
			path = t;
	} else {
		path = sceHTTPUnparseURI(conn->uri);
	}
	q = sceHTTPLibVStrcat(p, " ", path, " ", conn->https == 1 ? "HTTPS/" : "HTTP/",
	    conn->http11 == 1 ? "1.1" : "1.0", http_crlf, 0);
	if (q == 0)
		goto free_all;
	p = q;
	for (h = conn->headers; h != 0; h = h->next) {
		q = sceHTTPLibVStrcat(p, h->name, ": ", h->value, http_crlf, 0);
		if (q == 0)
			goto free_all;
		p = q;
	}
	q = sceHTTPLibStrcatsave(p, http_crlf);
	if (q == 0)
		goto free_all;
	p = q;
	xfree(path);
	return p;
free_all:
	xfree(p);
	xfree(path);
	HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
	return 0;
}

/* Sends a whole string within the remaining timeout. Returns 0, or -1 with
   client_error SCE_HTTP_ERR_TIMEOUT (timeout) or SCE_HTTP_ERR_IO (short send). */
static int http_send_string(struct sceHTTPClient *conn, const char *buf)
{
	int len = strlen(buf);
	int remaining;
	int r;

	if (conn->timeout != -1) {
		remaining = conn->timeout - http_time_elapsed(conn);
		if (remaining <= 0) {
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_TIMEOUT);
			return -1;
		}
	} else {
		remaining = -1;
	}
	r = sceHTTPLibSend(conn, buf, len, remaining, 0);
	if (r < 0)
		return r;
	if (r == len)
		return 0;
	HTTP_SET_ERROR(conn, SCE_HTTP_ERR_IO);
	return -1;
}

/* Sends len bytes in blocks of block_size within the remaining timeout and
   calls progress_cb at each step. Returns the bytes sent, or <= 0. */
static int http_send_progress(struct sceHTTPClient *conn, const void *buf, int len)
{
	void (*cb)(struct sceHTTPClient *, long long, int);
	const char *p = (const char *)buf;
	int remaining;
	int mark;
	int timeout;
	int n;
	int r;

	conn->sent = 0;
	cb = conn->progress_cb;
	if (cb != 0 && conn->method == SCE_HTTP_METHOD_PUT)
		cb(conn, 0, conn->progress_arg);
	mark = conn->block_size;

	remaining = len;
	while (remaining > 0) {
		if (conn->transfer_timeout == -1) {
			timeout = -1;
		} else {
			timeout = conn->transfer_timeout - http_time_elapsed(conn);
			if (timeout <= 0) {
				conn->client_error = SCE_HTTP_ERR_TIMEOUT;
				return -1;
			}
		}
		n = conn->block_size;
		if (!(n < remaining))
			n = remaining;
		r = sceHTTPLibSend(conn, p, n, timeout, 0);
		if (r <= 0)
			return r;
		remaining -= r;
		conn->sent += r;
		p += r;
		while (conn->sent >= (unsigned int)mark) {
			cb = conn->progress_cb;
			if (cb != 0 && conn->method == SCE_HTTP_METHOD_PUT) {
				int done = mark;

				if (conn->sent < (unsigned int)(mark + conn->block_size))
					done = conn->sent;
				cb(conn, done, conn->progress_arg);
			}
			mark += conn->block_size;
		}
	}
	return len - remaining;
}

/* Sends len bytes within the remaining timeout, through
   http_send_progress when block_size is not -1. */
static int http_send_block(struct sceHTTPClient *conn, const void *buf, int len)
{
	int remaining;
	int r;

	if (conn->block_size != -1) {
		r = http_send_progress(conn, buf, len);
	} else {
		if (conn->transfer_timeout != -1) {
			remaining = conn->transfer_timeout - http_time_elapsed(conn);
			if (remaining <= 0) {
				HTTP_SET_ERROR(conn, SCE_HTTP_ERR_TIMEOUT);
				return -1;
			}
		} else {
			remaining = -1;
		}
		conn->sent = 0;
		r = sceHTTPLibSend(conn, buf, len, remaining, 0);
		if (r > 0)
			conn->sent = r;
	}
	if (r < 0)
		return r;
	if (r == len)
		return 0;
	HTTP_SET_ERROR(conn, SCE_HTTP_ERR_IO);
	return -1;
}

/* Appends data to the line in *pbuf up to the first "\r\n" or bare "\n",
   and stores in *consumed the bytes of data used. Returns 3 if the line is
   not complete, 1 for a non-empty line, 2 for an empty one, -1 if out of
   memory. cont: the previous block ended with '\r'. */
static int http_line_accum(char **pbuf, int len, const char *data, int datalen, int *consumed, int cont)
{
	const char *p;
	const char *end;
	char *nb;
	int n;
	int start = len;

	if (*pbuf != 0 && cont != 0 && data[0] == http_crlf[1]) {
		*consumed = 1;
		len--;
		goto grow;
	}

	end = data + datalen;
	for (p = data; p < end; p++) {
		if (*p == http_crlf[0]) {
			if (p + 1 < end && p[1] == http_crlf[1]) {
				n = p + 2 - data;
				goto found;
			}
		} else if (*p == http_crlf[1]) {
			n = p + 1 - data;
			goto found;
		}
		len++;
	}

	if (*pbuf != 0)
		nb = xrealloc(*pbuf, len);
	else
		nb = xmalloc(len);
	if (nb == 0)
		return -1;
	memcpy(nb + start, data, len - start);
	*pbuf = nb;
	*consumed = datalen;
	return 3;

found:
	*consumed = n;
grow:
	if (*pbuf != 0)
		nb = xrealloc(*pbuf, len + 1);
	else
		nb = xmalloc(len + 1);
	if (nb == 0)
		return -1;
	if (start < len)
		memcpy(nb + start, data, len - start);
	nb[len] = 0;
	*pbuf = nb;
	return len == 0 ? 2 : 1;
}

/* Parses one line of the response: the status line when status_line is
   set ("HTTP/x.y code", or "HTTPS/..."), else a "Name: value" header that
   is appended to the header list. */
static int http_parse_resp_line(struct sceHTTPClient *conn, char *line, unsigned int len, int status_line)
{
	char *buf;
	const char *fmt;
	char *p;
	char *q;
	int blank;
	int major;
	int minor;
	int status;
	char *value;
	struct sceHTTPHeaderList *node;

	if (status_line) {
		fmt = "HTTP/%d.%d %d %s";
		buf = xmalloc(len);
		if (sceHTTPLibStrcasesub(line, "HTTPS") == 0) {
			conn->response.https = 1;
			fmt = "HTTPS/%d.%d %d %s";
		}
		if (buf == 0)
			goto no_mem;
		if (sscanf(line, fmt, &major, &minor, &status, buf) != 4) {
			xfree(buf);
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_INVALID);
			return -2;
no_mem:
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
			return -2;
		}
		conn->response.version_major = major;
		conn->response.version_minor = minor;
		conn->response.status_code = status;
		conn->response.reason = buf;
		return 0;
	}

	p = strchr(line, ':');
	if (p == 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_INVALID);
		return -1;
	}
	*p = 0;
	q = p;
	while (line < q) {
		blank = (*q == ' ' || *q == '\t');
		if (!blank)
			break;
		*q = 0;
		q--;
	}
	if (!http_is_token(line)) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_INVALID);
		return -1;
	}
	value = sceHTTPLibSkipWS(p + 1);
	node = sceHTTPAddHeaderList(conn->response.headers, line, value);
	if (node == 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
		return -1;
	}
	if (conn->response.headers == 0)
		conn->response.headers = node;
	return 0;
}

/* Returns 1 for the status codes without a body (204, 205, 304). */
static int http_status_has_no_body(struct sceHTTPClient *conn)
{
	switch (conn->response.status_code / 100) {
	case 2:
		switch (conn->response.status_code) {
		case 204:
		case 205:
			return 1;
		case 200:
		case 201:
		case 202:
		case 203:
		case 206:
			break;
		}
		break;
	case 3:
		switch (conn->response.status_code) {
		case 304:
			return 1;
		case 300:
		case 301:
		case 302:
		case 303:
		case 305:
		case 306:
		case 307:
			break;
		}
		break;
	case 1:
	case 4:
	case 5:
		break;
	}
	return 0;
}

/* Returns the value of the Content-Length header of the response, -1 if
   missing or malformed. */
static long long http_find_content_length(struct sceHTTPClient *conn)
{
	struct sceHTTPHeaderList *node;
	const char *p;
	const char *end;
	long long val;

	node = conn->response.headers;
	while (node != 0) {
		if (sceHTTPLibStrcasesub(node->name, "Content-Length") == 0) {
			p = sceHTTPLibSkipWS(node->value);
			val = sceHTTPLibStrtodi(p, &end);
			if (end == p)
				goto fail;
			if (*end != 0)
				goto fail;
			return val;
		}
		node = node ? node->next : 0;
	}
fail:
	return -1;
}

/* Reads the status line and the header lines of the response, receiving
   more data as needed within the timeout; a line starting with a space or
   tab continues the previous header. Stops at the empty line. Returns the
   size of the last receive, or -1 with client_error set. */
static int http_read_header_lines(struct sceHTTPClient *conn, char *buf, int bufsize, int avail, char **pdata,
    char **pline, int *plinelen, int status_line)
{
	char *line;
	char *data;
	char *nv;
	struct sceHTTPHeaderList *node;
	struct sceHTTPHeaderList *p;
	int consumed;
	int cont;
	int rc;
	char cr;
	int len;
	int n;
	int r;
	int tmo;
	int blank;

	len = 0;
	n = avail;
	cont = 0;
	rc = 0;
	line = 0;
	cr = http_crlf[0];

	for (;;) {
		if (avail == 0) {
			data = buf;
			if (conn->timeout != -1) {
				tmo = conn->timeout - http_time_elapsed(conn);
				if (tmo <= 0) {
					HTTP_SET_ERROR(conn, SCE_HTTP_ERR_TIMEOUT);
					goto free_line;
				}
			} else {
				tmo = -1;
			}
			n = sceHTTPLibRecv(conn, buf, bufsize, tmo, 1);
			if (n <= 0)
				goto finish;
			avail = n;
		} else {
			data = *pdata;
		}
		while ((r = http_line_accum(&line, len, data, avail, &consumed, cont)) == 1) {
			{
				len += consumed;
				if (status_line != 0)
					goto parse;
				blank = (line[0] == ' ' || line[0] == '\t');
				if (!blank)
					goto parse;
				p = conn->response.headers;
				if (p == 0) {
					node = 0;
				} else {
					while (p->next != 0)
						p = p->next;
					node = p;
				}
				if (node != 0) {
					nv = sceHTTPLibStrcatsave(node->value, line);
					if (nv == 0) {
						HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
						goto free_line;
					}
					node->value = nv;
				}
				goto after;
parse:
				rc = http_parse_resp_line(conn, line, len, status_line);
after:
				status_line = 0;
				if (rc < 0) {
					HTTP_SET_ERROR(conn, SCE_HTTP_ERR_INVALID);
					goto free_line;
				}
				xfree(line);
				line = 0;
				len = 0;
				data += consumed;
				avail -= consumed;
			}
		}
		if (r == 2) {
			xfree(line);
			line = 0;
			data += consumed;
			goto finish;
		}
		if (r != 3) {
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
			goto free_line;
		}
		avail = 0;
		cont = (buf[n - 1] == cr);
		len += consumed;
	}
finish:
	if (conn->response.status_code == 0) {
		if (http_timer_or_abort(conn) == 0 && conn->client_error == 0)
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_INVALID);
		goto free_line;
	}
	*pdata = data;
	*pline = line;
	*plinelen = len;
	return n;
free_line:
	if (line != 0)
		xfree(line);
	*pdata = data;
	*pline = 0;
	*plinelen = 0;
	return -1;
}

/* Parses a Keep-Alive header value "timeout=<n>, max=<n>" into the two
   out-parameters. Returns 0, or -1 if malformed. */
static int http_parse_keepalive(char *value, int *out_timeout, int *out_max)
{
	char *tok;
	char *end;
	char *next;
	char save;
	int max;
	int timeout;

	if (value == 0)
		goto bad;
	if (*value == 0)
		goto bad;
	max = 0;
	timeout = 0;
	end = value + strlen(value);
	while (*value != 0) {
		value = sceHTTPLibSkipWS(value);
		tok = sceHTTPLibSkipToken(value);
		if (tok == value)
			goto bad;
		if (tok == 0)
			goto bad;
		save = *tok;
		*tok = 0;
		next = strchr(tok + 1, ',');
		if (next == 0)
			next = end;
		if (sceHTTPLibStrcasecmp(value, "timeout") == 0 && timeout == 0) {
			*tok = save;
			value = sceHTTPLibSkipWS(tok);
			if (*value != '=')
				goto bad;
			value = sceHTTPLibSkipWS(value + 1);
			tok = sceHTTPLibSkipToken(value);
			if (tok == 0)
				goto bad;
			save = *tok;
			*tok = 0;
			while (isdigit(*value))
				timeout = timeout * 10 + *value++ - '0';
			if (*value != 0)
				goto bad;
			*tok = save;
		} else if (sceHTTPLibStrcasecmp(value, "max") == 0 && max == 0) {
			*tok = save;
			value = sceHTTPLibSkipWS(tok);
			if (*value != '=')
				goto bad;
			value = sceHTTPLibSkipWS(value + 1);
			tok = sceHTTPLibSkipToken(value);
			if (tok == 0)
				goto bad;
			save = *tok;
			*tok = 0;
			while (isdigit(*value))
				max = max * 10 + *value++ - '0';
			if (*value != 0)
				goto bad;
			*tok = save;
		}
		value = next + (*next == ',');
	}
	*out_timeout = timeout;
	*out_max = max;
	return 0;
bad:
	return -1;
}

/* Progress notification, as in http_recv_progress. */
static __inline__ void http_notify_progress(struct sceHTTPClient *conn, long long *pnext)
{
	struct sceHTTPResponse *y = &conn->response;
	long long next = *pnext;

	if (conn->block_size != -1) {
		while (y->length >= next) {
			if (conn->progress_cb != 0 && conn->method != SCE_HTTP_METHOD_PUT) {
				long long done;

				if (y->length < next + conn->block_size)
					done = y->length;
				else
					done = next;
				conn->progress_cb(conn, done, conn->progress_arg);
			}
			next += conn->block_size;
		}
		*pnext = next;
	}
}

/* Receives up to len bytes in blocks of block_size within the remaining
   timeout, adding to response.length and calling progress_cb each time the
   total passes *pnext. Sets response.timed_out on timeout. Returns the bytes read. */
static int http_recv_progress(struct sceHTTPClient *conn, void *buf, int len, int flags, long long *pnext)
{
	struct sceHTTPResponse *x = &conn->response;
	int remaining = len;
	int timeout;
	int n;
	int r;

	for (;;) {
		if (remaining <= 0)
			break;
		if (conn->transfer_timeout != -1) {
			timeout = conn->transfer_timeout - http_time_elapsed(conn);
			if (timeout <= 0) {
				x->timed_out = 1;
				break;
			}
		} else {
			timeout = -1;
		}
		n = remaining;
		if (conn->block_size != -1)
			n = conn->block_size < remaining ? conn->block_size : remaining;
		r = sceHTTPLibRecv(conn, buf, n, timeout, flags);
		if (r < 0)
			return r;
		if (r == 0)
			return len - remaining;
		remaining -= r;
		buf = (char *)buf + r;
		x->length += r;
		http_notify_progress(conn, pnext);
	}
	return len - remaining;
}

static __inline__ int http_is_xdigit(unsigned char c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static __inline__ int http_xdigit_val(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return -1;
}

/* Reads the response: status line and headers, Connection/Keep-Alive, then
   the body with a known length, up to the close, or chunked (with chunk_cb).
   The body goes to memory, or to the file descriptor out_fd when it is
   not -1. */
static int http_read_response(struct sceHTTPClient *conn)
{
	char *data;
	char *line;
	int linelen;
	int ka_timeout;
	int ka_max;
	long long next;
	int consumed;
	char *buf;
	char *body;
	struct sceHTTPResponse *resp;
	int bufsize;
	unsigned int body_len;
	long long clen;
	int chunked;
	int fd;
	long long remaining;
	struct sceHTTPHeaderList *h;
	unsigned int alloc;
	unsigned int size;
	unsigned int space;
	char *chunk;
	char *nb;
	char *p;
	int avail;
	int rc;
	int rc2;
	int cont;
	int tmo;
	char cr;

	next = 0;
	chunked = 0;
	bufsize = conn->block_size > 511 ? conn->block_size : 512;
	resp = &conn->response;
	if (bufsize > 0x8000)
		bufsize = 0x8000;
	fd = conn->out_fd;
	buf = (char *)xmalloc(bufsize);
	if (buf == 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
		return -1;
	}
	rc = http_read_header_lines(conn, buf, bufsize, 0, &data, &line, &linelen, 1);
	if (rc < 0)
		goto fail;
	if (http_timer_or_abort(conn)) {
		if (line != 0)
			xfree(line);
		xfree(buf);
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_ABORTED);
		return -1;
	}
	if (line != 0) {
		remaining = 0;
		body = line;
		body_len = linelen;
		clen = remaining;
		goto done;
	}
	resp->date = time(0);
	conn->ka_timeout = 0;
	conn->ka_max = 0;
	h = http_find_header(resp->headers, "Connection");
	if (h != 0) {
		if (sceHTTPLibStrcasecmp(h->value, "close") == 0)
			conn->sock_flags |= 0x10;
		else if (sceHTTPLibStrcasecmp(h->value, "keep-alive") == 0) {
			h = http_find_header(resp->headers, "Keep-Alive");
			if (h == 0)
				goto ka_default;
			if (http_parse_keepalive(h->value, &ka_timeout, &ka_max) == 0) {
				conn->ka_timeout = ka_timeout;
				conn->ka_max = ka_max;
			}
		}
	} else if (conn->http11 == 1 && resp->version_major == 1 && resp->version_minor == 1) {
ka_default:
		conn->ka_timeout = -1;
		conn->ka_max = -1;
	}
	if (conn->method == SCE_HTTP_METHOD_HEAD)
		goto out;
	if (http_status_has_no_body(conn))
		goto out;
	clen = http_find_content_length(conn);
	if (clen == 0)
		goto out;
te:
	h = http_find_header(resp->headers, "Transfer-Encoding");
	if (h != 0) {
		if (sceHTTPLibStrcasecmp(h->value, "chunked") != 0)
			goto te;
		chunked = 1;
		clen = 0;
	}
	if (fd != -1 && clen > (unsigned int)conn->max_body) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
		goto fail;
	}
	http_time_mark(conn);
	body_len = 0;
	resp->length = body_len;
	if (chunked == 0)
		resp->content_length = clen;
	else
		resp->content_length = -1;
	http_notify_progress(conn, &next);
	body = 0;
	alloc = clen > 0 ? clen : bufsize;
	avail = rc - (data - buf);
	if (chunked == 0) {
		if (fd == -1) {
			body = (char *)xmalloc(alloc);
			if (body == 0) {
				HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
				goto fail;
			}
		} else
			body = 0;
		if (clen > 0 && avail >= clen) {
			body_len = clen;
			if (fd != -1) {
				if (body_len != conn->write_fn(fd, data, clen)) {
					HTTP_SET_ERROR(conn, SCE_HTTP_ERR_WRITE);
					goto fail;
				}
			} else
				memcpy(body, data, clen);
			remaining = clen - avail;
			goto done;
		}
		if (avail > 0) {
			body_len = avail;
			resp->length = body_len;
			http_notify_progress(conn, &next);
			if (fd != -1) {
				if (avail != conn->write_fn(fd, data, avail)) {
					HTTP_SET_ERROR(conn, SCE_HTTP_ERR_WRITE);
					goto fail;
				}
			} else
				memcpy(body, data, avail);
		}
		if (conn->max_body != -1) {
			remaining = clen > 0 ? clen : (unsigned int)conn->max_body;
			remaining -= avail;
		} else if (clen > 0)
			remaining = clen - avail;
		else
			remaining = -1;
	} else {
		remaining = 0;
next_chunk:
		cont = 0;
		line = 0;
		cr = http_crlf[0];
		linelen = 0;
		for (;;) {
			if (avail > 0) {
				rc2 = http_line_accum(&line, linelen, data, avail, &consumed, cont);
				if (rc2 == 1) {
					linelen += consumed;
					data += consumed;
					break;
				}
				if (rc2 == 2)
					goto line_fail;
				if (rc2 != 3) {
					HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
					goto fail;
				}
				cont = data[avail] == cr;
				linelen += consumed;
			}
			data = buf;
			if (conn->timeout != -1) {
				tmo = conn->timeout - http_time_elapsed(conn);
				if (tmo <= 0) {
					if (line != 0)
						xfree(line);
					HTTP_SET_ERROR(conn, SCE_HTTP_ERR_TIMEOUT);
					goto fail;
				}
			} else
				tmo = -1;
			rc = sceHTTPLibRecv(conn, buf, bufsize, tmo, 1);
			avail = rc;
			if (rc <= 0) {
				if (line != 0)
					xfree(line);
				goto fail;
			}
		}
		avail -= consumed;
		p = strchr(line, ';');
		if (p != 0)
			*p = 0;
		p = sceHTTPLibSkipWS(line);
		if (*p == '0') {
			xfree(line);
			rc = http_read_header_lines(conn, buf, bufsize, avail, &data, &line, &linelen, 0);
			if (rc < 0)
				goto fail;
			if (http_timer_or_abort(conn)) {
				if (line != 0)
					xfree(line);
				xfree(buf);
				HTTP_SET_ERROR(conn, SCE_HTTP_ERR_ABORTED);
				return -1;
			}
			line = 0;
			goto done;
		}
		size = 0;
		while (http_is_xdigit(*p)) {
			size = size * 16 + http_xdigit_val(*p++);
		}
		xfree(line);
		line = 0;
		if (size == 0) {
			resp->timed_out = 1;
			goto done;
		}
		if ((unsigned int)conn->max_body < body_len + size && conn->max_body != -1)
			goto too_big;
		chunk = (char *)xmalloc(size + 2);
		if (chunk == 0) {
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
			goto fail;
		}
		if (avail < size + 2) {
			if (avail > 0)
				memcpy(chunk, data, avail);
			rc = http_recv_progress(conn, chunk + avail, size + 2 - avail, 0, &next);
			if (resp->timed_out)
				goto done;
			avail = 0;
			data = buf;
			if (rc <= 0) {
				xfree(chunk);
				if (http_timer_or_abort(conn)) {
too_big:
					resp->timed_out = 1;
				}
				goto done;
			}
		} else {
			avail -= size + 2;
			memcpy(chunk, data, size + 2);
			data += size + 2;
		}
		if (conn->chunk_cb != 0) {
			conn->chunk_cb(conn, chunk, size, conn->chunk_arg);
			goto next_chunk;
		}
		if (fd == -1) {
			if (body_len == 0)
				body = (char *)xmalloc(size);
			else {
				nb = (char *)xrealloc(body, body_len + size);
				if (nb == 0) {
					resp->timed_out = 1;
					goto done;
				}
				body = nb;
			}
			memcpy(body + body_len, chunk, size);
			body_len += size;
			resp->length = body_len;
			http_notify_progress(conn, &next);
		} else {
			if (size != conn->write_fn(fd, chunk, size)) {
				xfree(chunk);
				HTTP_SET_ERROR(conn, SCE_HTTP_ERR_WRITE);
				goto fail;
			}
			body_len += size;
			resp->length = body_len;
			http_notify_progress(conn, &next);
		}
		xfree(chunk);
		goto next_chunk;
	}
	while (remaining > 0 || remaining == -1) {
		space = remaining != -1 && remaining < bufsize - avail ? remaining : bufsize - avail;
		rc = http_recv_progress(conn, buf, space, 1, &next);
		if (resp->timed_out)
			break;
		avail = 0;
		if (rc <= 0)
			break;
		if (alloc < body_len + rc) {
			space = alloc - body_len;
			if (clen > 0) {
				if (fd != -1) {
					if (space != conn->write_fn(fd, buf, space)) {
						HTTP_SET_ERROR(conn, SCE_HTTP_ERR_WRITE);
						goto fail;
					}
				} else
					memcpy(body + body_len, buf, space);
				if (remaining != -1)
					remaining -= rc;
				break;
			}
			alloc *= 2;
			if (fd == -1) {
				nb = (char *)xrealloc(body, alloc);
				if (nb == 0) {
					memcpy(body + body_len, buf, space);
					if (remaining != -1)
						remaining -= rc;
					break;
				}
				body = nb;
			}
		}
		if (fd != -1) {
			if (rc != conn->write_fn(fd, buf, rc)) {
				HTTP_SET_ERROR(conn, SCE_HTTP_ERR_WRITE);
				goto fail;
			}
		} else
			memcpy(body + body_len, buf, rc);
		body_len += rc;
		resp->length = body_len;
		http_notify_progress(conn, &next);
		if (remaining != -1)
			remaining -= rc;
	}
	if (http_timer_or_abort(conn))
		resp->timed_out = 1;
done:
	if (clen > 0 && remaining > 0 && rc >= 0 && resp->timed_out == 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_LENGTH);
		rc = -1;
	}
	if (remaining < -1) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_LENGTH);
		rc = -1;
	}
	resp->body = body;
	resp->length = body_len;
	http_notify_progress(conn, &next);
out:
	xfree(buf);
	if (resp->timed_out) {
		if (http_abort_pending(conn))
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_ABORTED);
		else
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_TIMEOUT);
		return -1;
	}
	return rc < 0 ? -1 : 0;
line_fail:
	xfree(line);
fail:
	xfree(buf);
	sceHTTPCleanUpResponse(conn);
	return -1;
}
/* Returns 1 if the response is a redirect to follow. */
static int http_status_is_redirect(struct sceHTTPClient *conn)
{
	int ret = 0;

	switch (conn->response.status_code / 100) {
	case 3:
		switch (conn->response.status_code) {
		case 300:
		case 301:
		case 302:
		case 303:
		case 305:
		case 307:
			ret = 1;
			break;
		case 304:
			ret = 0;
			break;
		}
	case 1:  /* NOTE: bug: case 3 falls through, so this always returns 0 */
	case 2:
	case 4:
	case 5:
		ret = 0;
		break;
	}
	return ret;
}

/* Runs a request: sends the request line, the headers and (POST, PUT) the
   body, reads the response, and starts over while it is a redirect. Then
   removes the standard headers it added. */
static int http_run_request(struct sceHTTPClient *conn)
{
	char *buf;
	int ret;
	unsigned int mask;

	http_cleanup_response_impl(conn, 0);
	http_save_std_headers(conn, &mask);
retry:
	http_time_mark(conn);
	buf = http_build_request(conn);
	if (buf == 0)
		goto fail;
	if (http_send_string(conn, buf) != 0)
		goto err;
	xfree(buf);
	buf = 0;
	if (conn->method == SCE_HTTP_METHOD_POST || conn->method == SCE_HTTP_METHOD_PUT) {
		if (conn->post_len > 0 && conn->post_data != 0) {
			if (http_send_block(conn, conn->post_data, conn->post_len) != 0)
				goto err;
		}
	}
	if (http_read_response(conn) != 0)
		goto err;
	ret = http_status_is_redirect(conn);
	if (ret == 1)
		goto retry;
	goto out;
err:
	if (buf != 0)
		xfree(buf);
fail:
	ret = -1;
out:
	http_del_std_headers(conn, mask);
	pthread_setspecific(timer_key, 0);
	pthread_setspecific(abort_key, 0);
	conn->request_busy = 0;
	return ret;
}

/* Runs a request on conn and calls request_cb with the result. Returns the
   result, or -1 with client_error set. */
int sceHTTPRequest(struct sceHTTPClient *conn)
{
	if (conn == 0 || conn->sock == -1) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_INVALID);  /* NOTE: bug: conn may be NULL here */
		return -1;
	}
	if (conn->sock_flags & 0x10) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_CONNECT);
		return -1;
	}
	conn->client_error = 0;
	sceHTTPSClearErrnum(conn);
	if (conn->request_busy != 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_BUSY);
		return -1;
	}
	conn->request_busy = 1;
	conn->request_id = pthread_self();
	{
		int result = http_run_request(conn);

		if (conn->request_cb != 0)
			conn->request_cb(conn, result, conn->request_cb_arg);
		conn->redirect_flag = 0;
		conn->request_id = -1;
		conn->request_busy = 0;
		return result;
	}
}
struct sceHTTPResponse *sceHTTPGetResponse(struct sceHTTPClient *conn)
{
	return &conn->response;
}

int sceHTTPGetClientError(struct sceHTTPClient *conn)
{
	return conn->client_error;
}

long long sceHTTPGetTransferedBytes(struct sceHTTPClient *conn)
{
	if (conn->method != SCE_HTTP_METHOD_PUT)
		return conn->response.length;
	return (long long)(unsigned int)conn->sent;
}

long long sceHTTPGetPostedBytes(struct sceHTTPClient *conn)
{
	if (conn->method != SCE_HTTP_METHOD_POST)
		return (long long)(unsigned int)-1;
	return (long long)(unsigned int)conn->sent;
}

long long sceHTTPGetContentLength(struct sceHTTPClient *conn)
{
	return conn->response.content_length;
}

int sceHTTPInit(void)
{
	return sceHTTPLibInitIO() ? -1 : 0;
}

int sceHTTPTerminate(void)
{
	if (!sceHTTPLibTerminateIO())
		return sceHTTPLibTerminateMemory() ? -1 : 0;
	return -1;
}
/* Sets the stack size and priority (1..127) of the three internal
   threads. */
int sceHTTPSetDefaultThreadValue(int stack_size, int prio)
{
	if ((unsigned int)(prio - 1) >= 127)
		return -1;
	sceHTTPLibIDV[0] = stack_size;
	sceHTTPLibIDV[1] = prio;
	sceHTTPLibIDV[2] = stack_size;
	sceHTTPLibIDV[3] = prio;
	sceHTTPLibIDV[4] = stack_size;
	sceHTTPLibIDV[5] = prio;
	return 0;
}
