/*
 * Socket I/O of the HTTP library: open (name resolution in a thread, connect,
 * proxy CONNECT), close, and receive/send with a timeout (SIGALRM) and abort
 * (SIGUSR1), both delivered by siglongjmp to the waiting call.
 */

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "http/scehttp.h"

/* Thread-specific data keys of the I/O layer. */
pthread_key_t timer_key;
pthread_key_t abort_key;
pthread_key_t jb_key;
pthread_key_t client_key;

static void http_io_keys_create(void)
{
	pthread_key_create(&timer_key, 0);
	pthread_key_create(&abort_key, 0);
	pthread_key_create(&jb_key, 0);
	pthread_key_create(&client_key, 0);
}

static void http_io_keys_delete(void)
{
	pthread_key_delete(timer_key);
	pthread_key_delete(abort_key);
	pthread_key_delete(jb_key);
	pthread_key_delete(client_key);
}

/* SIGUSR1 handler (abort): flags abort_key and jumps back with 2. */
static void http_sigusr1_handler(int sig)
{
	void *env;

	pthread_setspecific(abort_key, (const void *)1);
	env = pthread_getspecific(jb_key);
	pthread_setspecific(jb_key, 0);
	if (env == 0)
		return;
	siglongjmp(env, 2);
}

/* SIGALRM handler (timeout): takes the jump buffer and jumps back with 1. */
static void http_sigalrm_handler(int sig)
{
	void *env = pthread_getspecific(jb_key);

	pthread_setspecific(jb_key, 0);
	if (env == 0)
		return;
	siglongjmp(env, 1);
}

static void http_block_sigusr1(void)
{
	sigset_t set, old;

	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(1, &set, &old);
}

static void http_unblock_sigusr1(void)
{
	sigset_t set, old;

	sigemptyset(&set);
	sigaddset(&set, 16);
	pthread_sigmask(2, &set, &old);
}

int sceHTTPAbort(struct sceHTTPClient *conn)
{
	if (conn->abort_flag != 0)
		return 0;
	conn->abort_flag = 1;
	if (conn->request_id != -1)
		pthread_kill(conn->request_id, SIGUSR1);
	return 0;
}

struct HttpResolve {
	/* 0x00 */ char *name;
	/* 0x04 */ struct hostent host;
	/* 0x18 */ struct hostent *result;
	/* 0x1C */ int h_err;
	/* 0x20 */ int ret;
	/* 0x24 */ int done;
	/* 0x28 */ pthread_cond_t cond;
	/* 0x34 */ sem_t sem;
	/* 0x44 */ char buf[2048];
};

/* Resolver thread: gethostbyname_r on the name, then publishes the result,
   wakes the waiter and frees the request. */
static void *http_thread_main(void *arg)
{
	struct HttpResolve *r = (struct HttpResolve *)arg;

	r->ret = gethostbyname_r(r->name, &r->host, r->buf, 2048, &r->result, &r->h_err);
	r->done = 1;
	pthread_cond_signal(&r->cond);
	sem_wait(&r->sem);
	sem_destroy(&r->sem);
	pthread_cond_destroy(&r->cond);
	xfree(r->name);
	xfree(r);
	return 0;
}

/* Starts a detached internal thread; returns 0 or the pthread error. */
static int http_spawn_thread(void *arg)
{
	pthread_t th;
	pthread_attr_t attr;
	int ret;

	ret = pthread_attr_init(&attr);
	if (ret == 0) {
		ret = pthread_create(&th, &attr, http_thread_main, arg);
		if (ret != 0)
			pthread_attr_destroy(&attr);
		else
			pthread_detach(th);
	}
	return ret;
}

/* Sets code and jumps to error. */
#define HTTP_FAIL(e)		do { code = (e); goto error; } while (0)

/* Reads one line, one byte at a time, into a buffer that doubles as needed.
   The '\n' and a preceding '\r' are removed. Returns the line and its length
   in *plen, or 0 with client_error set. */
static char *http_read_line(struct sceHTTPClient *conn, int *plen)
{
	char *buf;
	char *p;
	char *nb;
	int len;
	int cap;
	int r;

	cap = 128;
	buf = (char *)xmalloc(cap);
	if (buf == 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
		goto fail;
	}
	len = 0;
	p = buf;
	for (;;) {
		r = sceHTTPLibRecv(conn, p, 1, -1, 1);
		if (r <= 0) {
			if (conn->client_error == 0) {
				if (r < 0)
					HTTP_SET_ERROR(conn, SCE_HTTP_ERR_IO);
				else
					HTTP_SET_ERROR(conn, SCE_HTTP_ERR_RESPONSE);
			}
			goto fail;
		}
		if (*p == '\n') {
			*p = 0;
			if (len > 0 && p[-1] == '\r') {
				p[-1] = 0;
				len--;
			}
			break;
		}
		len++;
		p++;
		if (len >= cap) {
			cap *= 2;
			nb = (char *)xrealloc(buf, cap);
			if (nb == 0) {
				HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
				goto fail;
			}
			buf = nb;  /* NOTE: bug: p still points into the old buffer */
		}
	}
	*plen = len;
	return buf;
fail:
	xfree(buf);
	*plen = 0;
	return 0;
}

/* Sends "CONNECT host:port HTTP/1.0" to the proxy, checks for a "200 "
   status and skips the remaining header lines. On error sets client_error,
   closes the socket and returns -1. */
static int http_proxy_connect(struct sceHTTPClient *conn)
{
	struct sceHTTPParsedURI *uri;
	char *req;
	char *line;
	char *p;
	int len;
	int n;
	char portbuf[16];

	line = 0;
	uri = conn->uri;
	sprintf(portbuf, "%d", (unsigned short)uri->port);
	req = sceHTTPLibVStrcat(0, "CONNECT ", uri->host, ":", portbuf, " HTTP/1.0\r\n\r\n", 0);
	if (req == 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
		goto fail;
	}
	len = strlen(req);
	n = sceHTTPLibSend(conn, req, len, -1, 0);
	xfree(req);
	if (n < 0)
		goto fail;
	if (n != len) {
		if (conn->client_error == 0)
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_IO);
		goto fail;
	}
	line = http_read_line(conn, &len);
	if (line == 0)
		goto close_out;
	if (sceHTTPLibStrcasesub(line, "HTTP") != 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_RESPONSE);
		goto fail;
	}
	p = strchr(line, ' ');
	if (p != 0) {
		if (strncmp(p + 1, "200 ", 4) != 0) {
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_RESPONSE);
			goto fail;
		}
		xfree(line);
		do {
			line = http_read_line(conn, &len);
			if (line == 0)
				break;
			xfree(line);
		} while (len > 0);
		return 0;
	}
	HTTP_SET_ERROR(conn, SCE_HTTP_ERR_RESPONSE);
fail:
	if (line != 0)
		xfree(line);
close_out:
	conn->sock_state = 5;
	close(conn->sock);
	conn->sock_state = 1;
	conn->sock = -1;
	return -1;
}

static pthread_mutex_t g_http_io_mutex;

/* Opens the connection: resolves the host (in a thread, polled every 100 ms
   until done or aborted), tries socket+connect up to 5 times while refused,
   then for HTTPS the proxy CONNECT and sceHTTPSOpen. An abort jumps back
   here from the SIGUSR1 handler. */
static int http_do_open(struct sceHTTPClient *conn)
{
	struct sockaddr_in sin;
	sigjmp_buf env;
	struct timeval tv;
	struct timespec ts;
	struct sceHTTPParsedURI *uri;
	struct __jmp_buf_tag *ep;
	struct HttpResolve *r;
	struct hostent *hp;
	char *host;
	unsigned int addr;
	int herr;
	int err;
	int code;
	int s;
	int i;

	if (conn == 0 || conn->uri == 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_INVALID);
		return -1;
	}
	uri = conn->proxy_uri ? conn->proxy_uri : conn->uri;
	conn->sock_flags = 0;
	if (uri == 0 || (host = uri->host) == 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_INVALID);
		return -1;
	}
	addr = inet_addr(host);
	if (addr != (unsigned int)-1) {
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = addr;
	} else {
		r = (struct HttpResolve *)xmalloc(sizeof(struct HttpResolve));
		if (r == 0) {
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_NOMEM);
			return -1;
		}
		memset(r, 0, sizeof(struct HttpResolve));
		if ((r->name = sceHTTPLibStrsave(host)) == 0) {
			xfree(r);
			HTTP_FAIL(SCE_HTTP_ERR_NOMEM);
error:
			conn->client_error = code;
			return -1;
		}
		pthread_cond_init(&r->cond, 0);
		sem_init(&r->sem, 0, 0);
		conn->sock_state = 2;
		if (http_spawn_thread(r) != 0) {
			conn->sock_state = 1;
			pthread_cond_destroy(&r->cond);
			sem_destroy(&r->sem);
			xfree(r->name);
			xfree(r);
			HTTP_FAIL(SCE_HTTP_ERR_THREAD);
		}
		pthread_mutex_lock(&g_http_io_mutex);
		gettimeofday(&tv, 0);
		ts.tv_sec = tv.tv_sec;
		tv.tv_usec += 100000;
		if (tv.tv_usec > 1000000) {
			ts.tv_sec++;
			tv.tv_usec %= 1000000;
		}
		ts.tv_nsec = tv.tv_usec * 1000;
		while (pthread_cond_timedwait(&r->cond, &g_http_io_mutex, &ts) == ETIMEDOUT && r->done == 0
		    && pthread_getspecific(abort_key) == 0)
			;
		pthread_mutex_unlock(&g_http_io_mutex);
		conn->sock_state = 1;
		if (r->done == 0 || pthread_getspecific(abort_key) != 0) {
			sem_post(&r->sem);
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_ABORTED);
			return -1;
		}
		hp = r->result;
		if (r->ret != 0) {
			herr = r->h_err;
			sem_post(&r->sem);
			conn->sock_state = 1;
			conn->socket_error = errno;
			conn->resolve_error = herr;
			pthread_setspecific(jb_key, 0);
			if (conn->abort_flag != 0) {
				HTTP_SET_ERROR(conn, SCE_HTTP_ERR_ABORTED);
			} else {
				HTTP_SET_ERROR(conn, SCE_HTTP_ERR_RESOLVE);
			}
			return -1;
		}
		conn->sock_state = 1;
		sin.sin_family = hp->h_addrtype;
		memcpy(&sin.sin_addr, *hp->h_addr_list, hp->h_length);
		sem_post(&r->sem);
	}
	pthread_setspecific(client_key, conn);
	ep = env;
	if (sigsetjmp(ep, 1) != 0) {
		/* back from the signal handler: the connection is the one saved in
				   the thread-specific data */
		struct sceHTTPClient *c = pthread_getspecific(client_key);

		HTTP_SET_ERROR(c, SCE_HTTP_ERR_ABORTED);
		conn = c;
		goto out;
	}
	pthread_setspecific(jb_key, ep);
	if (pthread_getspecific(abort_key) != 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_ABORTED);
		goto out;
	}
	sin.sin_port = htons((unsigned short)uri->port);
	for (i = 0; i < 5; i++) {
		http_block_sigusr1();
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s < 0) {
			conn->socket_error = errno;
			pthread_setspecific(jb_key, 0);
			http_unblock_sigusr1();
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_SOCKET);
			return -1;
		}
		conn->sock = s;
		http_unblock_sigusr1();
		if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) == 0) {
			if (sceHTTPLibStrcasecmp(conn->uri->scheme, "HTTPS") == 0) {
				if (conn->proxy_uri != 0 && http_proxy_connect(conn) < 0)
					goto out;
				pthread_setspecific(jb_key, 0);
				if (sceHTTPSOpen(conn) < 0) {
					HTTP_SET_ERROR(conn, SCE_HTTP_ERR_SSL);
					goto out;
				}
			}
			pthread_setspecific(jb_key, 0);
			return 0;
		}
		http_block_sigusr1();
		conn->sock_state = 5;
		close(s);
		err = errno;
		conn->sock_state = 1;
		conn->sock = -1;
		conn->socket_error = err;
		http_unblock_sigusr1();
		if (conn->socket_error != ECONNREFUSED) {
			HTTP_SET_ERROR(conn, SCE_HTTP_ERR_CONNECT);
			goto out;
		}
	}
	HTTP_SET_ERROR(conn, SCE_HTTP_ERR_CONNECT);
out:
	if (pthread_getspecific(abort_key) != 0)
		conn->client_error = SCE_HTTP_ERR_ABORTED;
	pthread_setspecific(jb_key, 0);
	s = conn->sock;
	if (s != -1) {
		conn->sock_state = 5;
		close(s);
		conn->sock_state = 1;
		conn->sock = -1;
	}
	return -1;
}

/* Runs http_do_open unless a request is already in progress (SCE_HTTP_ERR_BUSY). */
static int http_open_impl(struct sceHTTPClient *conn)
{
	int ret;
	int code;

	if (conn->request_busy != 0)
		HTTP_FAIL(SCE_HTTP_ERR_BUSY);
	conn->request_busy = 1;
	conn->request_id = pthread_self();
	ret = http_do_open(conn);
	conn->request_id = -1;
	conn->request_busy = 0;
	return ret;
error:
	conn->client_error = code;
	return -1;
}

int sceHTTPOpen(struct sceHTTPClient *conn)
{
	return http_open_impl(conn);
}

/* Turns off the abort/timer state, closes the SSL layer and the socket,
 * and marks the connection closed. */
int sceHTTPClose(struct sceHTTPClient *conn)
{
	if (conn == 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_INVALID);
		return -1;
	}
	if (conn->request_busy != 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_BUSY);
		return -1;
	}
	conn->sock_flags = 0;
	pthread_setspecific(timer_key, 0);
	pthread_setspecific(abort_key, 0);
	conn->sock_state = 5;
	sceHTTPSClose(conn);
	if (conn->sock != -1)
		close(conn->sock);
	conn->sock_state = 1;
	conn->sock = -1;
	conn->abort_flag = 0;
	conn->request_busy = 0;
	return 0;
}

/* Receive and send loops: until len bytes are transferred (or once, with
   peek), through SSL when conn->ssl is set. On error set client_error to
   SCE_HTTP_ERR_IO if unset and socket_error to errno. Return the bytes transferred. */
static int http_recv_raw(struct sceHTTPClient *conn, char *buf, int len, int peek)
{
	int remaining = len;
	int r;

	if (len > 0) {
		do {
			if (conn->ssl != 0)
				r = sceHTTPSRecv(conn, buf, remaining);
			else
				r = recv(conn->sock, buf, remaining, 0);
			if (r < 0) {
				if (conn->client_error == 0) {
					HTTP_SET_ERROR(conn, SCE_HTTP_ERR_IO);
				}
				conn->socket_error = errno;
			}
			if (peek != 0)
				return r;
			if (r < 0)
				return r;
			if (r == 0)
				break;
			remaining -= r;
			buf += r;
		} while (remaining > 0);
	}
	return len - remaining;
}

static int http_send_raw(struct sceHTTPClient *conn, const char *buf, int len, int oob)
{
	int remaining = len;
	int r;

	if (len > 0) {
		do {
			if (conn->ssl != 0)
				r = sceHTTPSSend(conn, buf, remaining);
			else
				r = send(conn->sock, buf, remaining, 0);
			if (r < 0) {
				if (conn->client_error == 0) {
					HTTP_SET_ERROR(conn, SCE_HTTP_ERR_IO);
				}
				conn->socket_error = errno;
			}
			if (oob != 0)
				return r;
			if (r <= 0)
				return r;
			remaining -= r;
			buf += r;
		} while (remaining > 0);
	}
	return len - remaining;
}

/* Receives len bytes with a timeout in seconds (-1 = none); a timeout or
 * an abort makes the call return with client_error set. */
int sceHTTPLibRecv(struct sceHTTPClient *conn, void *buf, unsigned int len, int timeout, int flags)
{
	sigjmp_buf env;
	struct __jmp_buf_tag *ep;
	struct sceHTTPClient *c;
	int r;

	ep = env;
	if (pthread_getspecific(abort_key) != 0) {
		conn->sock_flags |= 8;
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_ABORTED);
		return -1;
	}
	if (timeout != -1)
		pthread_setspecific(timer_key, 0);
	c = conn;
	pthread_setspecific(client_key, c);
	r = sigsetjmp(ep, 1);
	if (r != 0) {
		c = pthread_getspecific(client_key);
		if (r == 1) {
			pthread_setspecific(timer_key, (const void *)1);
			HTTP_SET_ERROR(c, SCE_HTTP_ERR_TIMEOUT);
		} else {
			if (timeout != -1)
				alarm(0);
			c->sock_flags |= 8;
			HTTP_SET_ERROR(c, SCE_HTTP_ERR_ABORTED);
		}
		pthread_setspecific(jb_key, 0);
		return -1;
	}
	if (conn->ssl == 0)
		pthread_setspecific(jb_key, ep);
	if (timeout != -1)
		alarm(timeout);
	r = http_recv_raw(conn, buf, len, flags & 1);
	pthread_setspecific(jb_key, 0);
	if (timeout != -1)
		alarm(0);
	if (pthread_getspecific(abort_key) != 0)
		c->client_error = SCE_HTTP_ERR_ABORTED;
	return r;
}

/* The sending twin of sceHTTPLibRecv. */
int sceHTTPLibSend(struct sceHTTPClient *conn, const void *buf, unsigned int len, int timeout, int flags)
{
	sigjmp_buf env;
	struct __jmp_buf_tag *ep;
	struct sceHTTPClient *c;
	int r;

	ep = env;
	if (pthread_getspecific(abort_key) != 0) {
		conn->sock_flags |= 8;
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_ABORTED);
		return -1;
	}
	if (timeout != -1)
		pthread_setspecific(timer_key, 0);
	c = conn;
	pthread_setspecific(client_key, c);
	r = sigsetjmp(ep, 1);
	if (r != 0) {
		c = pthread_getspecific(client_key);
		if (r == 1) {
			pthread_setspecific(timer_key, (const void *)1);
			HTTP_SET_ERROR(c, SCE_HTTP_ERR_TIMEOUT);
		} else {
			if (timeout != -1)
				alarm(0);
			c->sock_flags |= 8;
			HTTP_SET_ERROR(c, SCE_HTTP_ERR_ABORTED);
		}
		pthread_setspecific(jb_key, 0);
		return -1;
	}
	if (conn->ssl == 0)
		pthread_setspecific(jb_key, ep);
	if (timeout != -1)
		alarm(timeout);
	r = http_send_raw(conn, buf, len, flags & 1);
	pthread_setspecific(jb_key, 0);
	if (timeout != -1)
		alarm(0);
	if (pthread_getspecific(abort_key) != 0)
		c->client_error = SCE_HTTP_ERR_ABORTED;
	return r;
}

/* Interrupts the pending request with SIGUSR1. Returns 0, or -1 with
 * client_error SCE_HTTP_ERR_THREAD. */
int sceHTTPLibAbortIO(struct sceHTTPClient *conn)
{
	if (conn->request_id == -1)
		return 0;
	if (pthread_kill(conn->request_id, SIGUSR1) != 0) {
		HTTP_SET_ERROR(conn, SCE_HTTP_ERR_THREAD);
		return -1;
	}
	return 0;
}

static pthread_once_t g_http_io_once = PTHREAD_ONCE_INIT;

/* Installs the SIGUSR1 (abort) and SIGALRM (timeout) handlers and creates
 * the thread-specific keys once. Returns 0, or -1. */
int sceHTTPLibInitIO(void)
{
	struct sigaction sa;
	struct sigaction osa;

	pthread_mutex_init(&g_http_io_mutex, 0);
	pthread_once(&g_http_io_once, http_io_keys_create);

	sa.sa_handler = http_sigusr1_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(16, &sa, &osa) < 0)
		return -1;

	sa.sa_handler = http_sigalrm_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(14, &sa, &osa) < 0)
		return -1;
	return 0;
}

int sceHTTPLibTerminateIO(void)
{
	http_io_keys_delete();
	return 0;
}

int sceHTTPGetSocketError(struct sceHTTPClient *conn)
{
	if (conn == 0)
		return -1;
	return conn->socket_error;
}

int sceHTTPGetResolveError(struct sceHTTPClient *conn)
{
	if (conn == 0)
		return -1;
	return conn->resolve_error;
}
