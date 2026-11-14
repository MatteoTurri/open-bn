/*
 * URI parsing, rebuilding, cloning and resolution of a relative URI.
 */

#include <stdio.h>
#include <string.h>

#include <ctype.h>

#include "http/scehttp.h"

/*
 * Parses "<scheme>://[user[:password]@]host[:port]" followed by the query
 * (if flags & 0x40) and the path (if flags & 0x20, "/" when absent). The
 * port defaults to 80, or 443 for "https". Returns 0 on error.
 */
struct sceHTTPParsedURI *sceHTTPParseURI(const char *uri, int flags)
{
	struct sceHTTPParsedURI *out;
	const char *p;
	const char *a;
	const char *b;
	const char *e;
	int port;

	out = xcalloc(1, sizeof(struct sceHTTPParsedURI));
	if (out == 0)
		return 0;

	p = uri;
	a = strchr(p, ':');
	if (a == 0)
		goto fail;
	if (strncmp(a + 1, "//", 2) != 0)
		goto fail;
	out->scheme = sceHTTPLibStrnsave(p, a - p);
	if (out->scheme == 0)
		goto fail;

	p = a + 3;
	b = strchr(p, '@');
	if (b != 0) {
		e = b;
		a = strchr(p, ':');
		if (a != 0 && a < b) {
			if (p == a) {
				if (a + 1 == b)
					goto after_user;
				goto fail;
			} else {
				if (a + 1 != b) {
					out->password = sceHTTPLibStrnsave(a + 1, b - a - 1);
					if (out->password == 0)
						goto fail;
				}
				b = a;
			}
		}
		out->user = sceHTTPLibStrnsave(p, b - p);
		if (out->user == 0)
			goto fail;
after_user:
		p = e + 1;
	}

	if (out->scheme != 0 && sceHTTPLibStrcasecmp(out->scheme, "https") == 0)
		out->port = 443;
	else
		out->port = 80;

	b = strchr(p, '/');
	if (b == 0)
		b = p + strlen(p);
	a = strchr(p, ':');
	e = b;
	if (a != 0 && a < b) {
		e = a;
		a = a + 1;
		port = 0;
		while (a < b) {
			if (!isdigit(*a))
				goto fail;
			port *= 10;
			port += *a - '0';
			a++;
		}
		out->port = port;
	}
	out->host = sceHTTPLibStrnsave(p, e - p);
	if (out->host == 0)
		goto fail;

	if (*b != '/')
		goto no_query;
	a = strchr(b, '?');
	if (a == 0)
		goto no_query;
	if ((flags & 0x40) != 0)
		out->query = sceHTTPLibStrsave(a + 1);
	goto have_end;
no_query:
	a = b + strlen(b);
have_end:
	if ((flags & 0x20) != 0) {
		if (*b != '/')
			out->path = sceHTTPLibStrsave("/");
		else
			out->path = sceHTTPLibStrnsave(b, a - b);
		if (out->path == 0)
			goto fail;
	}
	return out;
fail:
	sceHTTPFreeURI(out);
	return 0;
}

/* Returns a copy of uri, or 0 if a copy of a non-NULL member fails. */
struct sceHTTPParsedURI *sceHTTPCloneURI(struct sceHTTPParsedURI *uri)
{
	struct sceHTTPParsedURI *new_uri;

	if (uri == 0)
		return 0;
	new_uri = xcalloc(1, sizeof(struct sceHTTPParsedURI));
	if (new_uri == 0)
		return 0;

	new_uri->scheme = sceHTTPLibStrsave(uri->scheme);
	if (new_uri->scheme == 0 && uri->scheme != 0)
		goto fail;

	new_uri->user = sceHTTPLibStrsave(uri->user);
	if (new_uri->user == 0 && uri->user != 0)
		goto fail;

	new_uri->password = sceHTTPLibStrsave(uri->password);
	if (new_uri->password == 0 && uri->password != 0)
		goto fail;

	new_uri->host = sceHTTPLibStrsave(uri->host);
	if (new_uri->host == 0 && uri->host != 0)
		goto fail;

	new_uri->port = uri->port;

	new_uri->path = sceHTTPLibStrsave(uri->path);
	if (new_uri->path == 0 && uri->path != 0)
		goto fail;

	new_uri->query = sceHTTPLibStrsave(uri->query);
	if (new_uri->query == 0 && uri->query != 0)
		goto fail;

	return new_uri;

fail:
	sceHTTPFreeURI(new_uri);
	return 0;
}
int sceHTTPFreeURI(struct sceHTTPParsedURI *uri)
{
	if (uri == 0)
		return 0;
	if (uri->scheme)
		xfree(uri->scheme);
	if (uri->user)
		xfree(uri->user);
	if (uri->password)
		xfree(uri->password);
	if (uri->host)
		xfree(uri->host);
	if (uri->path)
		xfree(uri->path);
	if (uri->query)
		xfree(uri->query);
	xfree(uri);
	return 0;
}

/* Rebuilds the URI string: scheme "://" user [":" password] "@" host ":"
 * port path ["?" query]. The port is written only with a host. Returns 0 on
 * failure. */
char *sceHTTPUnparseURI(struct sceHTTPParsedURI *uri)
{
	char *result = 0;
	char *tmp;
	char portbuf[16];

	if (uri->scheme != 0) {
		tmp = sceHTTPLibStrcatsave(0, uri->scheme);
		if (tmp == 0)
			goto fail;
		result = tmp;
		tmp = sceHTTPLibStrcatsave(result, "://");
		if (tmp == 0)
			goto fail;
		result = tmp;
	}

	if (uri->user != 0) {
		tmp = sceHTTPLibStrcatsave(result, uri->user);
		if (tmp == 0)
			goto fail;
		result = tmp;
		if (uri->password != 0) {
			tmp = sceHTTPLibStrcatsave(result, ":");
			if (tmp == 0)
				goto fail;
			result = tmp;
			tmp = sceHTTPLibStrcatsave(result, uri->password);
			if (tmp == 0)
				goto fail;
			result = tmp;
		}
		tmp = sceHTTPLibStrcatsave(result, "@");
		if (tmp == 0)
			goto fail;
		result = tmp;
	}

	if (uri->host != 0) {
		tmp = sceHTTPLibStrcatsave(result, uri->host);
		if (tmp == 0)
			goto fail;
		result = tmp;
		tmp = sceHTTPLibStrcatsave(result, ":");
		if (tmp == 0)
			goto fail;
		result = tmp;
		memset(portbuf, 0, 16);
		sprintf(portbuf, "%d", uri->port);
		tmp = sceHTTPLibStrcatsave(result, portbuf);
		if (tmp == 0)
			goto fail;
		result = tmp;
	}

	if (uri->path != 0) {
		tmp = sceHTTPLibStrcatsave(result, uri->path);
		if (tmp == 0)
			goto fail;
		result = tmp;
	}

	if (uri->query != 0) {
		tmp = sceHTTPLibVStrcat(result, "?", uri->query, 0);
		if (tmp == 0)
			goto fail;
		result = tmp;
	}

	return result;

fail:
	if (result != 0)
		xfree(result);
	return 0;
}

/* Nonzero if uri starts with "http" (case-insensitive). */
int sceHTTPIsAbsoluteURI(const char *uri)
{
	if (uri == 0)
		return 0;
	return sceHTTPLibStrcasesub(uri, "http") == 0;
}

/*
 * Resolves uri against base: an absolute uri is copied; otherwise the scheme
 * and host come from base, and the path is uri itself if it starts with '/',
 * else uri appended to the directory of the path of base.
 */
char *sceHTTPFindAbsoluteURI(char *uri, char *base)
{
	struct sceHTTPParsedURI u;
	void *bu;
	struct sceHTTPParsedURI *ru;
	char *p;
	char *s;
	char *t;

	bu = (void *)sceHTTPIsAbsoluteURI(uri);
	if (bu != 0)
		return sceHTTPLibStrsave(uri);

	if (base == 0)
		return 0;
	if (base[0] == 0)
		return 0;

	ru = sceHTTPParseURI(base, 32);
	if (ru == 0)
		return 0;

	memcpy(&u, bu, sizeof(u));  /* NOTE: bug: bu is always NULL here */

	u.scheme = ru->scheme;
	if (u.scheme == 0)
		u.scheme = "http";

	u.host = ru->host;
	if (u.host == 0)
		goto fail;

	if (uri != 0 && uri[0] == '/') {
		u.path = sceHTTPLibStrsave(uri);
	} else {
		p = ru->path;
		if (p != 0 && p[0] != '/')
			goto fail;
		s = sceHTTPLibStrsave(p);
		if (s == 0)
			goto fail;
		t = strrchr(s, '/');
		t[1] = 0;
		u.path = sceHTTPLibStrcatsave(s, uri);
		if (u.path == 0) {
			xfree(s);
			goto fail;
		}
	}

	s = sceHTTPUnparseURI(&u);
	sceHTTPFreeURI(ru);
	xfree(u.path);
	return s;
fail:
	sceHTTPFreeURI(ru);
	return 0;
}
