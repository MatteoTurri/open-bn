/*
 * Cookies of the HTTP library: the cookie list, parsing of Set-Cookie and
 * Set-Cookie2 response headers and the Cookie request header.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "http/scehttp.h"

/* Semaphore of the cookie list; the lock helpers below are empty. */
int sceHTTPLibCListSema;

static void http_cookie_list_lock(void)
{}

static void http_cookie_list_unlock(void)
{}

/* Copies the cookie src into a new node and appends it to list; if list
   already has a cookie with the same name, domain and path, that cookie
   takes the new fields instead. lock = 0 when the caller holds the lock. */
static struct sceHTTPCookieList *http_cookie_list_add(struct sceHTTPCookieList *list, struct sceHTTPCookie *src,
    int lock)
{
	struct sceHTTPCookieList *node;
	struct sceHTTPCookie *d;
	struct sceHTTPCookieList *cur;
	struct sceHTTPCookieList *next;

	node = (struct sceHTTPCookieList *)xcalloc(1, sizeof(*node));
	if (node == 0)
		return 0;
	d = &node->cookie;
	if (src->name != 0) {
		if ((d->name = sceHTTPLibStrsave(src->name)) == 0)
			goto fail;
	}
	if (src->value != 0) {
		if ((d->value = sceHTTPLibStrsave(src->value)) == 0)
			goto fail;
	}
	if (src->domain != 0) {
		if ((d->domain = sceHTTPLibStrsave(src->domain)) == 0)
			goto fail;
	}
	if (src->path != 0) {
		if ((d->path = sceHTTPLibStrsave(src->path)) == 0)
			goto fail;
	}
	if (src->comment != 0) {
		if ((d->comment = sceHTTPLibStrsave(src->comment)) == 0)
			goto fail;
	}
	d->expires = src->expires;
	d->secure = src->secure;
	d->version = src->version;
	d->max_age = src->max_age;
	if (src->set_cookie2 != 0) {
		d->set_cookie2 = src->set_cookie2;
		d->discard = src->discard;
		if (src->comment_url != 0) {
			if ((d->comment_url = sceHTTPLibStrsave(src->comment_url)) == 0)
				goto fail;
		}
		if (src->port != 0) {
			if ((d->port = sceHTTPLibStrsave(src->port)) == 0)
				goto fail;
		}
	}
	if (list == 0)
		return node;

	if (lock)
		http_cookie_list_lock();
	cur = list;
	do {
		next = cur->next;
		if (strcmp(cur->cookie.name, d->name) == 0 && strcmp(cur->cookie.domain, d->domain) == 0
		    && strcmp(cur->cookie.path, d->path) == 0) {
			if (cur->cookie.name != 0)
				xfree(cur->cookie.name);
			cur->cookie.name = d->name;
			if (cur->cookie.value != 0)
				xfree(cur->cookie.value);
			cur->cookie.value = d->value;
			if (cur->cookie.domain != 0)
				xfree(cur->cookie.domain);
			cur->cookie.domain = d->domain;
			if (cur->cookie.path != 0)
				xfree(cur->cookie.path);
			cur->cookie.path = d->path;
			cur->cookie.expires = d->expires;
			cur->cookie.secure = d->secure;
			cur->cookie.version = d->version;
			cur->cookie.max_age = d->max_age;
			cur->cookie.comment = d->comment;  /* NOTE: bug: frees the new value instead of the old one */
			if (cur->cookie.comment != 0)
				xfree(cur->cookie.comment);
			cur->cookie.set_cookie2 = d->set_cookie2;
			cur->cookie.discard = d->discard;
			cur->cookie.comment_url = d->comment_url;
			if (cur->cookie.comment_url != 0)
				xfree(cur->cookie.comment_url);
			cur->cookie.port = d->port;
			if (cur->cookie.port != 0)
				xfree(cur->cookie.port);
			if (lock)
				http_cookie_list_unlock();
			xfree(node);
			return cur;
		}
		cur = next;
	} while (cur != 0);

	while (list != 0 && list->next != 0)
		list = list->next;
	node->prev = list;
	if (list != 0)
		list->next = node;
	if (lock)
		http_cookie_list_unlock();
	return node;

fail:
	if (d->name != 0)
		xfree(d->name);
	if (d->value != 0)
		xfree(d->value);
	if (d->domain != 0)
		xfree(d->domain);
	if (d->path != 0)
		xfree(d->path);
	if (d->comment != 0)
		xfree(d->comment);
	if (d->comment_url != 0)
		xfree(d->comment_url);
	if (d->port != 0)
		xfree(d->port);
	xfree(node);
	return 0;
}

struct sceHTTPCookieList *sceHTTPAddCookieList(struct sceHTTPCookieList *list, struct sceHTTPCookie *cookie)
{
	return http_cookie_list_add(list, cookie, 1);
}

/* Unlinks entry from the list and frees it; returns the new head. */
struct sceHTTPCookieList *sceHTTPDeleteCookieListEntry(struct sceHTTPCookieList *list, struct sceHTTPCookieList *entry)
{
	struct sceHTTPCookieList *head = list;

	http_cookie_list_lock();
	while (list != 0) {
		if (list == entry) {
			if (head == list)
				head = head->next;
			if (list->prev != 0)
				list->prev->next = list->next;
			if (list->next != 0)
				list->next->prev = list->prev;
			if (list->cookie.name)
				xfree(list->cookie.name);
			if (list->cookie.value)
				xfree(list->cookie.value);
			if (list->cookie.domain)
				xfree(list->cookie.domain);
			if (list->cookie.path)
				xfree(list->cookie.path);
			if (list->cookie.comment)
				xfree(list->cookie.comment);
			if (list->cookie.comment_url)
				xfree(list->cookie.comment_url);
			if (entry->cookie.port)
				xfree(entry->cookie.port);
			xfree(entry);
			http_cookie_list_unlock();  /* NOTE: bug: unlocked again after the loop */
			break;
		}
		list = list->next;
	}
	http_cookie_list_unlock();
	return head;
}

int sceHTTPFreeCookieList(struct sceHTTPCookieList *node)
{
	struct sceHTTPCookieList *next;

	http_cookie_list_lock();
	while (node != 0) {
		next = node->next;
		if (node->cookie.name)
			xfree(node->cookie.name);
		if (node->cookie.value)
			xfree(node->cookie.value);
		if (node->cookie.domain)
			xfree(node->cookie.domain);
		if (node->cookie.path)
			xfree(node->cookie.path);
		if (node->cookie.comment)
			xfree(node->cookie.comment);
		if (node->cookie.comment_url)
			xfree(node->cookie.comment_url);
		if (node->cookie.port)
			xfree(node->cookie.port);
		xfree(node);
		node = next;
	}
	http_cookie_list_unlock();
	return 0;
}

/* Returns a new list with copies of the cookies for which callback(data,
   ctx) is true, or 0 if a copy fails. */
struct sceHTTPCookieList *sceHTTPFilterCookieList(struct sceHTTPCookieList *list, http_cookie_filter_fn callback,
    void *ctx)
{
	struct sceHTTPCookieList *newlist;

	http_cookie_list_lock();
	newlist = 0;
	while (list != 0) {
		struct sceHTTPCookie *data = &list->cookie;

		if (callback(data, ctx)) {
			struct sceHTTPCookieList *r = http_cookie_list_add(newlist, data, 0);

			if (r == 0) {
				http_cookie_list_unlock();
				if (newlist != 0)
					sceHTTPFreeCookieList(newlist);
				return 0;
			}
			if (newlist == 0)
				newlist = r;
		}
		list = list->next;
	}
	http_cookie_list_unlock();
	return newlist;
}

/* sceHTTPLibStrtodi without the NULL check. */
static long long http_strtoll(const char *p, const char **end)
{
	long long val = 0;

	while (isdigit(*p)) {
		val = val * 10 + (*p - '0');
		p++;
	}
	*end = p;
	return val;
}

/* Parses one cookie of a Set-Cookie/Set-Cookie2 value at *cur: name=value
   and its attributes. On the next name=value sets *cur to it and stops
   (*cur = 0 at the end). Max-Age becomes an absolute time from now. Returns
   0, or -1 on failure. */
/* Returns the last occurrence of c in [s, e], searching back from e. */
static __inline__ char *http_rfind(char *s, char *e, int c)
{
	for (; e >= s; e--)
		if (*e == c)
			return e;
	return 0;
}

static int http_parse_cookie_attrs(char **cur, long now, char **nv_out, char **domain_out, char **path_out,
    long long *expire_out, int *secure_out, int *version_out, long long *maxage_out, char **comment_out,
    int *discard_out, char **commenturl_out, char **port_out, int cookie2)
{
	char *p, *q, *semi;
	char c;
	char *nv, *domain, *path;
	long long expire;
	char *version_end;
	const char *maxage_end;
	char *end;
	int secure, version;
	long long maxage;
	int discard;
	char *comment, *commenturl, *port;

	p = *cur;
	*cur = 0;
	if (p == 0 || *p == 0)
		return -1;
	path = 0;
	domain = 0;
	nv = 0;
	expire = -1;
	maxage = -1;
	secure = 0;
	version = 0;
	discard = 0;
	port = 0;
	commenturl = 0;
	comment = 0;
	end = p + strlen(p);
	while (*p != 0) {
		p = sceHTTPLibSkipWS(p);
		q = sceHTTPLibSkipToken(p);
		if (q == p || q == 0)
			goto fail;
		c = *q;
		*q = 0;
		if (c != ';')
			semi = strchr(q + 1, ';');
		else
			semi = q;
		if (semi == 0)
			semi = end;
		if (sceHTTPLibStrcasecmp(p, "domain") == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto next;
			p = sceHTTPLibSkipWS(p + 1);
			q = semi;
			if (*p == '"') {
				p++;
				q = sceHTTPLibSkipToken(p);
			}
			if (p >= q)
				goto fail;
			if ((domain = sceHTTPLibStrnsave(p, q - p)) != 0)
				goto next;
			goto fail;
		} else if (sceHTTPLibStrcasecmp(p, "path") == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto next;
			p = sceHTTPLibSkipWS(p + 1);
			q = semi;
			if (*p == '"') {
				p++;
				q = sceHTTPLibSkipToken(p);
			}
			if (p >= q)
				goto fail;
			if ((path = sceHTTPLibStrnsave(p, q - p)) != 0)
				goto next;
			goto fail;
		} else if (sceHTTPLibStrcasecmp(p, "expires") == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto next;
			p = sceHTTPLibSkipWS(p + 1);
			if (*p == '"')
				p++;
			if (p >= semi)
				goto fail;
			if (sceHTTPLibDateToTime(p, semi - p, &expire) == -1)
				goto fail;
		} else if (sceHTTPLibStrcasecmp(p, "version") == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto next;
			p = sceHTTPLibSkipWS(p + 1);
			if (p == semi)
				goto fail;
			if (*p == '"')
				p++;
			version = strtol(p, &version_end, 10);
			if (version_end == p)
				goto fail;
		} else if (sceHTTPLibStrcasecmp(p, "max-age") == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto next;
			p = sceHTTPLibSkipWS(p + 1);
			if (p == semi)
				goto fail;
			if (*p == '"')
				p++;
			maxage = http_strtoll(p, &maxage_end);
			if (maxage_end == p)
				goto fail;
			expire = maxage + now;
		} else if (sceHTTPLibStrcasecmp(p, "secure") == 0) {
			*q = c;
			secure = 1;
		} else if (sceHTTPLibStrcasecmp(p, "comment") == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto next;
			p = sceHTTPLibSkipWS(p + 1);
			if (p >= semi)
				goto fail;
			if ((comment = sceHTTPLibStrnsave(p, semi - p)) != 0)
				goto next;
			goto fail;
		} else if (cookie2 && sceHTTPLibStrcasecmp(p, "discard") == 0) {
			*q = c;
			discard = 1;
		} else if (cookie2 && sceHTTPLibStrcasecmp(p, "CommentURL") == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto next;
			p = sceHTTPLibSkipWS(p + 1);
			if (*p != '"')
				goto fail;
			p++;
			q = http_rfind(p, semi, '"');
			if (p >= q)
				goto fail;
			if ((commenturl = sceHTTPLibStrnsave(p, q - p)) != 0)
				goto next;
			goto fail;
		} else if (cookie2 && sceHTTPLibStrcasecmp(p, "Port") == 0) {
			*q = c;
			p = sceHTTPLibSkipWS(q);
			if (*p != '=')
				goto next;
			p = sceHTTPLibSkipWS(p + 1);
			if (*p != '"')
				goto fail;
			p++;
			q = http_rfind(p, semi, '"');
			if (p >= q)
				goto fail;
			if ((port = sceHTTPLibStrnsave(p, q - p)) != 0)
				goto next;
			goto fail;
		} else {
			if (nv != 0) {
				*q = c;
				*cur = p;
				break;
			}
			*q = c;
			nv = sceHTTPLibStrnsave(p, semi - p);
			if (nv == 0)
				goto fail;
		}
next:
		p = semi + (*semi == ';');
	}
	*nv_out = nv;
	*domain_out = domain;
	*path_out = path;
	*expire_out = expire;
	*secure_out = secure;
	*version_out = version;
	*maxage_out = maxage;
	*discard_out = discard;
	*comment_out = comment;
	*commenturl_out = commenturl;
	*port_out = port;
	return 0;

fail:
	if (nv != 0)
		xfree(nv);
	if (domain != 0)
		xfree(domain);
	if (path != 0)
		xfree(path);
	if (comment != 0)
		xfree(comment);
	if (commenturl != 0)
		xfree(commenturl);
	if (port != 0)
		xfree(port);
	return -1;
}

/* Adds to the list every cookie of the Set-Cookie and Set-Cookie2 headers
   of resp; domain and path default to those of the request URI. */
static struct sceHTTPCookieList *http_parse_cookies(struct sceHTTPResponse *resp, struct sceHTTPParsedURI *uri)
{
	struct sceHTTPCookie rec;
	char *cur;
	char *nv;
	char *domain;
	char *path;
	long long expire;
	int secure;
	int version;
	long long maxage;
	char *comment;
	int discard;
	char *commenturl;
	char *port;
	struct sceHTTPCookieList *list;
	struct sceHTTPCookieList *n;
	struct sceHTTPHeaderList *h;
	int cookie2;
	char *p;
	char *end;
	char *eq;
	char *q;
	char *name;
	char *val;

	list = 0;
	if (resp == 0)
		return 0;
	h = resp->headers;
	while (h != 0) {
		if (sceHTTPLibStrcasecmp(h->name, "Set-Cookie") == 0
		    || sceHTTPLibStrcasecmp(h->name, "Set-Cookie2") == 0) {
			cookie2 = sceHTTPLibStrcasecmp(h->name, "Set-Cookie2") == 0;
			cur = h->value;
more:
			path = 0;
			domain = 0;
			nv = 0;
			if (http_parse_cookie_attrs(&cur, resp->date, &nv, &domain, &path, &expire, &secure, &version,
			        &maxage, &comment, &discard, &commenturl, &port, cookie2)
			    < 0)
				goto next;
			if (path == 0) {
				if ((path = sceHTTPLibStrsave(uri->path)) == 0)
					goto fail;
			}
			if (domain == 0) {
				if ((domain = sceHTTPLibStrsave(uri->host)) == 0)
					goto fail;
			}
			p = nv;
			if (p == 0)
				goto next;
			for (;;) {
				end = strchr(p, ':');
				if (end == 0)
					end = p + strlen(p);
				else
					end++;
				eq = http_rfind(p, end, '=');
				if (eq == 0) {
					xfree(nv);
					if (domain != 0)
						xfree(domain);
					if (path != 0)
						xfree(path);
					goto next;
				}
				val = sceHTTPLibSkipWS(eq + 1);
				if (*val == '"')
					val++;
				*sceHTTPLibSkipToken(val) = 0;
				while (p < val) {
					name = sceHTTPLibSkipWS(p);
					p = sceHTTPLibSkipToken(name);
					if (*p != '=') {
						p++;
						continue;
					}
					*p++ = 0;
					{
						rec.name = name;
						rec.value = val;
						rec.domain = domain;
						rec.path = path;
						rec.expires = expire;
						rec.secure = secure;
						rec.version = version;
						rec.max_age = maxage;
						rec.comment = comment;
						rec.set_cookie2 = cookie2;
						rec.discard = discard;
						rec.comment_url = commenturl;
						rec.port = port;
						n = http_cookie_list_add(list, &rec, 1);
						if (n == 0)
							goto fail;
						if (list == 0)
							list = n;
					}
				}
				if (*end == 0)
					break;
				p = end;
			}
			xfree(nv);
			if (domain != 0)
				xfree(domain);
			if (path != 0)
				xfree(path);
			if (comment != 0)
				xfree(comment);
			if (commenturl != 0)
				xfree(commenturl);
			if (port != 0)
				xfree(port);
			if (cur != 0)
				goto more;
		}
next:
		h = h->next;
	}
	return list;

fail:
	if (nv != 0)
		xfree(nv);
	if (domain != 0)
		xfree(domain);
	if (path != 0)
		xfree(path);
	if (comment != 0)
		xfree(comment);
	if (commenturl != 0)
		xfree(commenturl);
	if (port != 0)
		xfree(port);
	if (list != 0)
		sceHTTPFreeCookieList(list);
	return 0;
}

/* Adds the cookies of the response resp to the list. */
struct sceHTTPCookieList *sceHTTPParseCookie(struct sceHTTPClient *conn, struct sceHTTPResponse *resp)
{
	return http_parse_cookies(resp, conn->uri);
}

/* Returns nonzero if domain ends with pattern (case-insensitive). */
int sceHTTPLibMatchDomain(const char *domain, const char *pattern)
{
	if (strlen(domain) < strlen(pattern))
		return 0;
	return strcmp(domain + (strlen(domain) - strlen(pattern)), pattern) == 0;
}

/* Parses an IPv4 address like inet_aton (decimal, octal or hex parts;
   a.b.c and a.b forms), ended by NUL or a space. Returns 1 and stores it in
   *out if the string is an address, else 0. */
static int http_parse_ipv4(unsigned int *out, const char *s)
{
	unsigned int parts[4];
	unsigned int val, base, d;
	int n = 0;
	int c;

	for (;;) {
		base = 10;
		if (*s == '0') {
			s++;
			if (*s == 'x' || *s == 'X') {
				s++;
				base = 16;
			} else
				base = 8;
		}
		val = 0;
		while (isxdigit(c = *s)) {
			if (!isdigit(c)) {
				if (isupper(c))
					d = c - 'A' + 10;
				else
					d = c - 'a' + 10;
			} else
				d = c - '0';
			if (d >= base)
				break;
			val = val * base + d;
			s++;
		}
		if (n > 0 && parts[n - 1] >= 256)
			return 0;
		parts[n++] = val;
		if (*s != '.' || n >= 4)
			break;
		s++;
	}
	if (*s != '\0' && *s != ' ')
		return 0;
	switch (n) {
	case 1:
		break;
	case 2:
		if (val >> 24)
			return 0;
		val |= parts[0] << 24;
		break;
	case 3:
		if (val >> 16)
			return 0;
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;
	case 4:
		if (val >> 8)
			return 0;
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	default:
		return 0;
	}
	*out = val;
	return 1;
}

/* Returns nonzero if the cookie domain covers host: equal IP addresses,
   equal names (case-insensitive), or the domain is a suffix of host. */
static int http_cookie_domain_match(const char *host, const char *domain)
{
	unsigned int hostip;
	unsigned int domip;
	const char *p;

	if (http_parse_ipv4(&hostip, host) != 0) {
		if (http_parse_ipv4(&domip, domain) != 0 && domip == hostip)
			return 1;
		return 0;
	}
	if (strcmp(host, domain) == 0)
		return 1;
	p = strchr(host, '.');
	if (p == 0)
		return 0;
	if (*domain != '.')
		p = p + 1;
	if (strlen(p) < strlen(domain))
		return 0;
	return strcmp(p + (strlen(p) - strlen(domain)), domain) == 0;
}

/* Returns nonzero if the cookie domain has a dot after its leading one,
   or is ".local". */
static int http_cookie_domain_ok(const char *domain)
{
	const char *p;

	p = strchr(domain[0] == '.' ? domain + 1 : domain, '.');
	if (p == 0 && strcmp(domain, ".local") != 0)
		return 0;
	return p[1] != 0;
}

/* Returns 1 if target is in the comma-separated list of numbers. */
static int http_code_in_list(long target, char *s)
{
	char *end;

	while (*s) {
		long v = strtol(s, &end, 10);

		if (!(s < end))
			return 0;
		if (v == target)
			return 1;
		s = sceHTTPLibSkipWS(end);
		if (*s == ',')
			s = sceHTTPLibSkipWS(s + 1);
	}
	return 0;
}

static char http_hdr_fold[] = "\r\n\t";

/* Returns nonzero if the cookie should be sent with the request of conn:
   secure, expiry, domain and path, plus the RFC 2109 domain and port
   checks. */
static int http_cookie_match(struct sceHTTPClient *conn, struct sceHTTPCookie *data)
{
	struct sceHTTPParsedURI *uri;

	uri = conn->uri;
	if (data->secure != 0 && conn->https != 1)
		return 0;
	if (data->expires > 0 && data->expires < (long long)time(0))
		return 0;
	if (uri == 0)
		return 0;
	if (data->domain != 0 && uri->host != 0 && sceHTTPLibMatchDomain(uri->host, data->domain) == 0)
		return 0;
	if (data->path != 0 && uri->path != 0 && sceHTTPLibStrsub(uri->path, data->path) != 0)
		return 0;
	if (data->set_cookie2 == 0)
		return 1;
	if (data->domain != 0 && http_cookie_domain_ok(data->domain) == 0)
		return 0;
	if (data->domain != 0 && uri->host != 0 && http_cookie_domain_match(uri->host, data->domain) == 0)
		return 0;
	if (data->port != 0 && http_code_in_list(uri->port, data->port) == 0)
		return 0;
	return 1;
}

/* Returns a copy of list sorted so that longer, more specific paths come
   first; 0 if a copy fails. */
static struct sceHTTPCookieList *http_cookie_select(struct sceHTTPCookieList *src)
{
	struct sceHTTPCookieList *head;
	struct sceHTTPCookieList *n;
	struct sceHTTPCookieList *cur;
	char *key;

	head = 0;
	http_cookie_list_lock();
	for (; src != 0; src = src->next) {
		n = http_cookie_list_add(0, &src->cookie, 0);
		if (n == 0) {
			head = 0;
			break;
		}
		if (head == 0) {
			head = n;
			continue;
		}
		key = n->cookie.path;
		cur = head;
		for (;;) {
			if (strcmp(key, cur->cookie.path) != 0 && sceHTTPLibStrsub(key, cur->cookie.path) == 0) {
				n->next = cur;
				n->prev = cur->prev;
				if (cur->prev != 0)
					cur->prev->next = n;
				else
					head = n;
				cur->prev = n;
				goto next_src;
			}
			if (cur->next == 0)
				break;
			cur = cur->next;
		}
		n->prev = cur;
		if (cur != 0)
			cur->next = n;
next_src:;
	}
	http_cookie_list_unlock();
	return head;
}

/* Builds the Cookie header from a cookie list and appends it to the request
   headers. Each cookie uses the RFC 2109 form ($Version, quoted value,
   $Path, $Domain, $Port) when its version is 1, else the Netscape form. */
int sceHTTPSetCookie(struct sceHTTPClient *conn, struct sceHTTPCookieList *list)
{
	struct sceHTTPCookieList *filtered;
	struct sceHTTPCookieList *prev;
	char *hdr;
	char *hdr0;
	char *tmp;
	void *newlist;
	char portbuf[16];

	if (list == 0)
		return 0;

	hdr = 0;
	if (list->cookie.version == 1) {
		tmp = sceHTTPLibStrsave("$Version=\"1\"");
		if (tmp == 0) {
			conn->client_error = SCE_HTTP_ERR_NOMEM;
			return -1;
		}
		hdr = tmp;
	}
	filtered = http_cookie_select(list);
	hdr0 = hdr;
	if (filtered == 0) {
		conn->client_error = SCE_HTTP_ERR_NOMEM;
		return -1;
	}
	list = filtered;
	prev = 0;
	while (list != 0) {
		struct sceHTTPCookie *data = &list->cookie;

		if (http_cookie_match(conn, data)) {
			if (data->version == 1) {
				tmp = sceHTTPLibVStrcat(hdr, ";", http_hdr_fold, list->cookie.name, "=\"", data->value,
				    "\"", 0);
				if (tmp == 0)
					goto fail;
				hdr = tmp;
				if (data->path != 0) {
					tmp = sceHTTPLibVStrcat(hdr, ";$Path=\"", data->path, "\"", 0);
					if (tmp == 0)
						goto fail;
					hdr = tmp;
				}
				if (data->domain != 0) {
					tmp = sceHTTPLibVStrcat(hdr, ";$Domain=\"", data->domain, "\"", 0);
					if (tmp == 0)
						goto fail;
					hdr = tmp;
				}
				if (data->set_cookie2 != 0 && data->port != 0) {
					memset(portbuf, 0, sizeof portbuf);
					sprintf(portbuf, "%d", conn->uri->port);
					tmp = sceHTTPLibVStrcat(hdr, ";$Port=\"", portbuf, "\"", 0);
					if (tmp == 0)
						goto fail;
					hdr = tmp;
				}
			} else {
				if (prev == 0)
					tmp = sceHTTPLibVStrcat(hdr, list->cookie.name, "=", data->value, 0);
				else
					tmp = sceHTTPLibVStrcat(hdr, ";", list->cookie.name, "=", data->value, 0);
				if (tmp == 0)
					goto fail;
				hdr = tmp;
			}
			prev = list;
		}
		list = list->next;
	}
	sceHTTPFreeCookieList(filtered);
	if (hdr == 0)
		return 0;
	if (hdr == hdr0) {
		xfree(hdr);
		return 0;
	}
	newlist = sceHTTPAddHeaderList(0, "Cookie", hdr);
	xfree(hdr);
	if (newlist == 0)
		return -1;
	conn->headers = sceHTTPLibCatHeaderList(conn->headers, (struct sceHTTPHeaderList *)newlist);
	return 0;
fail:
	sceHTTPFreeCookieList(filtered);
	conn->client_error = SCE_HTTP_ERR_NOMEM;
	if (hdr != 0)
		xfree(hdr);
	return -1;
}
