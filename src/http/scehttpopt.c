/*
 * Connection options of the HTTP library: sceHTTPSetOption and
 * sceHTTPGetOption, variadic, keyed by option number.
 */

#include <stdarg.h>
#include <string.h>

#include "http/scehttp.h"
#include "http/sceurl.h"

/* option with one int out-parameter */
#define HTTP_GET1(f) do { \
	int *p = va_arg(ap, int *); \
	if (p == 0) \
		goto err; \
	*p = (int) conn->f; \
} while (0)

/* option with two int out-parameters */
#define HTTP_GET2(f1, f2) do { \
	int *p = va_arg(ap, int *); \
	int *q = va_arg(ap, int *); \
	if (p == 0) \
		goto err; \
	if (q == 0) \
		goto err; \
	*p = (int) conn->f1; \
	*q = (int) conn->f2; \
} while (0)

/* invalid option or argument: the code goes to conn->client_error */
#define HTTP_ERR(e) do { \
	conn->client_error = (e); \
} while (0)

/* stores whether the member is zero */
#define HTTP_GETZ(f) do { \
	int *p = va_arg(ap, int *); \
	if (p == 0) \
		goto err; \
	*p = conn->f == 0; \
} while (0)

/* Sets an option. Returns 0, or -1 with conn->client_error set to SCE_HTTP_ERR_INVALID
   (invalid argument) or SCE_HTTP_ERR_NOMEM (no memory) on most errors. */
int sceHTTPSetOption(struct sceHTTPClient *conn, int option, ...)
{
	va_list ap;
	int ret;

	va_start(ap, option);
	switch (option) {
	case SCE_HTTP_OPT_USER_AGENT:
		conn->user_agent = va_arg(ap, char *);
		ret = 0;
		break;
	case SCE_HTTP_OPT_HTTP11: {
		int v = va_arg(ap, int);
		if ((unsigned int)v >= 2) {
			HTTP_ERR(SCE_HTTP_ERR_INVALID);
			ret = -1;
			break;
		}
		conn->http11 = v;
		ret = 0;
		break;
	}
	case SCE_HTTP_OPT_METHOD: {
		int v = va_arg(ap, int);
		if ((unsigned int)v >= 15) {
			HTTP_ERR(SCE_HTTP_ERR_INVALID);
			ret = -1;
			break;
		}
		conn->method = v;
		ret = 0;
		break;
	}
	case SCE_HTTP_OPT_URI: {
		void *p = va_arg(ap, void *);
		void *q;
		if (p == 0) {
			HTTP_ERR(SCE_HTTP_ERR_INVALID);
			ret = -1;
			break;
		}
		q = sceHTTPCloneURI(p);
		if (q == 0) {
			ret = -1;
			break;
		}
		if (conn->uri != 0)
			sceHTTPFreeURI(conn->uri);
		conn->uri = q;
		ret = 0;
		break;
	}
	case SCE_HTTP_OPT_PROXY: {
		void *p = va_arg(ap, void *);
		void *q;
		if (p == 0) {
			HTTP_ERR(SCE_HTTP_ERR_INVALID);
			ret = -1;
			break;
		}
		q = sceHTTPCloneURI(p);
		if (q == 0) {
			ret = -1;
			break;
		}
		if (conn->proxy_uri != 0)
			sceHTTPFreeURI(conn->proxy_uri);
		conn->proxy_uri = q;
		ret = 0;
		break;
	}
	case SCE_HTTP_OPT_HEADERS: {
		void *p = va_arg(ap, void *);
		int v = va_arg(ap, int);
		void *q = sceHTTPLibCloneHeaderList(p);
		struct sceHTTPHeaderList *n;
		if (p != 0 && q == 0) {
			ret = -1;
			break;
		}
		if (v == 2) {
			for (n = (struct sceHTTPHeaderList *)q; n != 0; n = n->next)
				conn->headers = sceHTTPLibDelHeaderByName(conn->headers, n->name);
			conn->headers = sceHTTPLibCatHeaderList(conn->headers, q);
		} else if (v == 1) {
			sceHTTPFreeHeaderList(conn->headers);
			conn->headers = q;
		} else {
			conn->headers = sceHTTPLibCatHeaderList(conn->headers, q);
		}
		ret = 0;
		break;
	}
	case SCE_HTTP_OPT_POST_DATA: {
		void *data = va_arg(ap, void *);
		int len = va_arg(ap, int);
		int flags = va_arg(ap, int);
		if (len != 0 && data == 0) {
			HTTP_ERR(SCE_HTTP_ERR_INVALID);
			ret = -1;
			break;
		}
		if (conn->post_data != 0 && (conn->post_flags & 2) == 0)
			xfree(conn->post_data);
		if (flags & 2) {
			conn->post_data = data;
			conn->post_len = len;
		} else if (flags & 1) {
			conn->post_data = sceURLLibEscapeN(data, len);
			if (conn->post_data == 0) {
				HTTP_ERR(SCE_HTTP_ERR_NOMEM);
				ret = -1;
				break;
			}
			conn->post_len = strlen((char *)conn->post_data);
		} else {
			conn->post_data = xmalloc(len);
			if (conn->post_data == 0) {
				HTTP_ERR(SCE_HTTP_ERR_NOMEM);
				ret = -1;
				break;
			}
			if (len != 0)
				memcpy(conn->post_data, data, len);
			conn->post_len = len;
		}
		conn->post_flags = flags;
		ret = 0;
		break;
	}
	case 21:
		conn->unk_40 = va_arg(ap, int);
		ret = 0;
		break;
	case SCE_HTTP_OPT_TIMEOUT:
		conn->timeout = va_arg(ap, int);
		ret = 0;
		break;
	case SCE_HTTP_OPT_TRANSFER_TIMEOUT:
		conn->transfer_timeout = va_arg(ap, int);
		ret = 0;
		break;
	case 9:
		conn->unk_38 = va_arg(ap, int) == 0;
		ret = 0;
		break;
	case 22:
		conn->unk_B0 = va_arg(ap, int) == 0;
		ret = 0;
		break;
	case SCE_HTTP_OPT_REQUEST_CB: {
		http_request_cb cb = va_arg(ap, http_request_cb);
		int arg = va_arg(ap, int);
		conn->request_cb = cb;
		conn->request_cb_arg = arg;
		ret = 0;
		break;
	}
	case SCE_HTTP_OPT_CHUNK_CB: {
		http_chunk_cb cb = va_arg(ap, http_chunk_cb);
		int arg = va_arg(ap, int);
		conn->chunk_cb = cb;
		conn->chunk_arg = arg;
		ret = 0;
		break;
	}
	case 23: {
		int v1 = va_arg(ap, int);
		int v2 = va_arg(ap, int);
		conn->unk_A4 = v1;
		conn->unk_A8 = v2;
		ret = 0;
		break;
	}
	case SCE_HTTP_OPT_THREAD: {
		int stack_size = va_arg(ap, int);
		int prio = va_arg(ap, int);
		if (stack_size < 4096) {
			HTTP_ERR(SCE_HTTP_ERR_INVALID);
			ret = -1;
			break;
		}
		if (prio <= 0) {
			HTTP_ERR(SCE_HTTP_ERR_INVALID);
			ret = -1;
			break;
		}
		if (prio >= 128) {
			HTTP_ERR(SCE_HTTP_ERR_INVALID);
			ret = -1;
			break;
		}
		conn->thread_stack = stack_size;
		conn->thread_prio = prio;
		conn->thread_stack2 = stack_size;
		conn->thread_prio2 = prio;
		ret = 0;
		break;
	}
	case SCE_HTTP_OPT_SSL_FLAGS:
		conn->ssl_flags = va_arg(ap, int);
		ret = 0;
		break;
	case SCE_HTTP_OPT_PROGRESS_CB: {
		http_progress_cb cb = va_arg(ap, http_progress_cb);
		int arg = va_arg(ap, int);
		conn->progress_cb = cb;
		conn->progress_arg = arg;
		ret = 0;
		break;
	}
	case SCE_HTTP_OPT_BLOCK_SIZE: {
		int v = va_arg(ap, int);
		if ((unsigned int)v < 512) {
			HTTP_ERR(SCE_HTTP_ERR_INVALID);
			ret = -1;
			break;
		}
		conn->block_size = v;
		ret = 0;
		break;
	}
	case SCE_HTTP_OPT_OUT_FD:
		conn->out_fd = va_arg(ap, int);
		ret = 0;
		break;
	case SCE_HTTP_OPT_WRITE_FN: {
		http_write_fn v = va_arg(ap, http_write_fn);
		if (v == 0) {
			HTTP_ERR(SCE_HTTP_ERR_INVALID);
			ret = -1;
			break;
		}
		conn->write_fn = v;
		ret = 0;
		break;
	}
	case SCE_HTTP_OPT_DEL_HEADER:
		conn->headers = sceHTTPLibDelHeaderByName(conn->headers, va_arg(ap, char *));
		ret = 0;
		break;
	case SCE_HTTP_OPT_STD_HEADERS:
		conn->std_headers = va_arg(ap, int);
		ret = 0;
		break;
	default:
		HTTP_ERR(SCE_HTTP_ERR_INVALID);
		ret = -1;
		break;
	}
	return ret;
}

/* Copies an option into the out-parameter(s). Always returns 0; on an
   invalid option or a NULL pointer it sets conn->client_error to SCE_HTTP_ERR_INVALID. */
int sceHTTPGetOption(struct sceHTTPClient *conn, int option, ...)
{
	va_list ap;

	va_start(ap, option);
	switch (option) {
	case SCE_HTTP_OPT_USER_AGENT:
		HTTP_GET1(user_agent);
		break;
	case SCE_HTTP_OPT_HTTP11:
		HTTP_GET1(http11);
		break;
	case SCE_HTTP_OPT_METHOD:
		HTTP_GET1(method);
		break;
	case SCE_HTTP_OPT_URI:
		HTTP_GET1(uri);
		break;
	case SCE_HTTP_OPT_PROXY:
		HTTP_GET1(proxy_uri);
		break;
	case SCE_HTTP_OPT_HEADERS:
		HTTP_GET1(headers);
		break;
	case SCE_HTTP_OPT_POST_DATA:
		HTTP_GET2(post_data, post_len);
		break;
	case 21:
		HTTP_GET1(unk_40);
		break;
	case SCE_HTTP_OPT_TIMEOUT:
		HTTP_GET1(timeout);
		break;
	case SCE_HTTP_OPT_TRANSFER_TIMEOUT:
		HTTP_GET1(transfer_timeout);
		break;
	case 9:
		HTTP_GETZ(unk_38);
		break;
	case 22:
		HTTP_GETZ(unk_B0);
		break;
	case SCE_HTTP_OPT_REQUEST_CB:
		HTTP_GET2(request_cb, request_cb_arg);
		break;
	case SCE_HTTP_OPT_CHUNK_CB:
		HTTP_GET2(chunk_cb, chunk_arg);
		break;
	case 23:
		HTTP_GET2(unk_A4, unk_A8);
		break;
	case SCE_HTTP_OPT_THREAD:
		HTTP_GET2(thread_stack, thread_prio);
		break;
	case SCE_HTTP_OPT_KEEP_ALIVE:
		HTTP_GET2(ka_timeout, ka_max);
		break;
	case SCE_HTTP_OPT_PROGRESS_CB:
		HTTP_GET2(progress_cb, progress_arg);
		break;
	case SCE_HTTP_OPT_BLOCK_SIZE:
		HTTP_GET1(block_size);
		break;
	case SCE_HTTP_OPT_OUT_FD:
		HTTP_GET1(out_fd);
		break;
	case SCE_HTTP_OPT_WRITE_FN:
		HTTP_GET1(write_fn);
		break;
	case SCE_HTTP_OPT_STD_HEADERS:
		HTTP_GET1(std_headers);
		break;
	default:
err:
		HTTP_ERR(SCE_HTTP_ERR_INVALID);
		break;
	}
	return 0;
}
